// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include "ipc_utils.h"
#include "log.h"
#include "pow.h"
// #include <openssl/conf.h>
// #include <openssl/crypto.h>
// #include <openssl/evp.h>
#include <fcntl.h> // para open(), O_WRONLY, O_RDONLY, etc.
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h> // se fores usar mkfifo() ou outras flags de modo
#include <time.h>
#include <unistd.h>

TransactionPool *pool;
Ledger *ledger;

char check_hash[HASH_SIZE];
bool first = true;
static pthread_mutex_t tx_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tx_ready_cond = PTHREAD_COND_INITIALIZER;


static void handle_sigterm(int _);
void *miner_thread(void *arg);
static void init_miner_ipc(void);
void send_block_to_validator(TransactionBlock *block, size_t bytes);
void *tx_monitor_thread(void *arg);

void run_miner(int num_miners) {

  signal(SIGTERM, handle_sigterm);

  log_message("[Miner] Genesis hash: %s\n", INITIAL_HASH);

  // abrir as shared memory
  init_miner_ipc();

  pthread_t threads[num_miners];
  log_message("[Miner] Processo miner iniciado com %d threads\n", num_miners);

  for (int i = 0; i < num_miners; i++) {
    int *id = malloc(sizeof(int));
    *id = i + 1;
    if (pthread_create(&threads[i], NULL, miner_thread, id) != 0) {
      perror("Erro ao criar thread");
      exit(1);
    }
  }

  pthread_t monitor_thread;
  pthread_create(&monitor_thread, NULL, tx_monitor_thread, NULL);

  for (int i = 0; i < num_miners; i++) {
    pthread_join(threads[i], NULL);
  }

  log_message("[Miner] Todas as threads terminaram.\n");
}

void *miner_thread(void *arg) {
  int id = *((int *)arg);
  log_message("[Miner Thread %d] Starting block mining…\n", id);

  size_t bytes = sizeof(TransactionBlock) +
                 cfg.TRANSACTIONS_PER_BLOCK * sizeof(Transaction);

  unsigned idx;

  while (1) {
    // count will to check in the block is full;
    int count = 0;

    //help the check hash
    bool aborted = false;

    // to check if its worth to keep trying
    char check_hash[HASH_SIZE];
    sem_wait(ledger_sem);
    strncpy(check_hash, ledger->previous_hash, HASH_SIZE);
    sem_post(ledger_sem);

    // create a new empty block
    TransactionBlock *block = calloc(1, bytes);

    // bitmap to dont pick the same transaction
    size_t n = pool->size;
    bool *picked = calloc(n, sizeof(bool));
    if (!picked) {
      perror("calloc picked");
      exit(1);
    }

    idx = ledger->current_block;
    snprintf(block->txb_id, TXB_ID_LEN, "MINER-%d-BLOCK-%u", id, idx);

    strncpy(block->previous_block_hash, ledger->previous_hash, HASH_SIZE);

    while (count < cfg.TRANSACTIONS_PER_BLOCK) {
      pthread_mutex_lock(&tx_mutex);
      pthread_cond_wait(&tx_ready_cond, &tx_mutex);
      pthread_mutex_unlock(&tx_mutex);
      if (strcmp(check_hash, ledger->previous_hash) != 0) {
        break;
      }

      for (int i = 0; i < pool->size; i++) {
        if (!pool->transactions_pending_set[i].empty && !picked[i]) {
          block->transactions[count++] = pool->transactions_pending_set[i].tx;
          picked[i] = true;
          break;
        }
      }
    }

    PoWResult result;
    //if find a nonce return error==0 and exit the loop
    do {
      block->timestamp = time(NULL);
      result = proof_of_work(block);

      sem_wait(ledger_sem);
      aborted = (strcmp(ledger->previous_hash, check_hash) != 0);
      sem_post(ledger_sem);
      if(aborted){
        break;
      }

    } while (result.error == 1);

    if(aborted){
      continue;
    }
    send_block_to_validator(block, bytes);

    free(picked);
    free(block);
  }
}

void send_block_to_validator(TransactionBlock *block, size_t bytes) {
  int fd = open(VALIDATOR_FIFO, O_WRONLY);
  if (fd < 0) {
    perror("open validator FIFO");
    return;
  }

  ssize_t w = write(fd, block, bytes);
  if (w < 0) {
    perror("write bloco");
  } else if ((size_t)w != bytes) {
    fprintf(stderr, "Escreveu só %zd/%zu bytes\n", w, bytes);
  }

  close(fd);
}

void *tx_monitor_thread(void *arg) {
  while (1) {
    if (sem_wait(full) == -1) {
      perror("sem_wait");
      break;
    }

    sem_post(full);
    pthread_cond_broadcast(&tx_ready_cond); // acorda todos os miners
  }

  return NULL;
}

static void handle_sigterm(int _) {
  /* desmonta tudo e sai */
  if (pool)
    shmdt(pool);
  if (ledger)
    shmdt(ledger);
  _exit(0);
}

static void init_miner_ipc(void) {
  pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
  if (pool == (void *)-1) {
    perror("shmat TxPool");
    exit(1);
  }
  ledger = (Ledger *)shmat(shm_ledger_id, NULL, 0);
  if (ledger == (void *)-1) {
    perror("shmat Ledger");
    exit(1);
  }
  // Inicializa hash inicial apenas uma vez
  memcpy(ledger->previous_hash, INITIAL_HASH, HASH_SIZE);
}