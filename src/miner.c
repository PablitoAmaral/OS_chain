// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include "ipc_utils.h"
#include "log.h"
#include "pow.h"
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>    // para open(), O_WRONLY, O_RDONLY, etc.
#include <sys/stat.h> // se fores usar mkfifo() ou outras flags de modo


#define VALIDATOR_FIFO "/tmp/VALIDATOR_INPUT"


char check_hash[HASH_SIZE];
static pthread_mutex_t header_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tx_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tx_ready_cond = PTHREAD_COND_INITIALIZER;

#define LEDGER_SIZE 4
TransactionBlock ledger[LEDGER_SIZE];

static Transaction generate_random_transaction(int tx_number);
static TransactionBlock generate_random_block(const char *prev_hash,
                                              int block_number);
static void print_block_info(TransactionBlock *block);
static void blkcpy(TransactionBlock *dest, TransactionBlock *src);
static void dump_ledger(void);


static void send_block_to_validator(TransactionBlock *block, size_t bytes) {
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

    pthread_mutex_lock(&tx_mutex);
    pthread_cond_broadcast(&tx_ready_cond); // acorda todos os miners
    pthread_mutex_unlock(&tx_mutex);
  }

  return NULL;
}

void *miner_thread(void *arg) {
  int id = *((int *)arg);
  log_message("[Miner Thread %d] Starting block mining…\n", id);

  TransactionPool *pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
  if (pool == (void *)-1) {
    perror("shmat TxGen");
    exit(1);
  }

  size_t bytes = sizeof(TransactionBlock) +
                 cfg.TRANSACTIONS_PER_BLOCK * sizeof(Transaction);

  while (1) {
    int count = 0;
    char check_hash[HASH_SIZE];
    strncpy(check_hash, previous_hash, HASH_SIZE);
    TransactionBlock *block = calloc(1, bytes);
    size_t n = pool->size;
    bool *picked = calloc(n, sizeof(bool));
    if (!picked) {
      perror("calloc picked");
      exit(1);
    }

    unsigned idx;
    pthread_mutex_lock(&header_lock);
    idx = block_counter;
    strncpy(block->previous_block_hash, previous_hash, HASH_SIZE);
    pthread_mutex_unlock(&header_lock);

    snprintf(block->txb_id, TXB_ID_LEN, "MINER-%d-BLOCK-%u", id, idx);

    while (count < cfg.TRANSACTIONS_PER_BLOCK) {
      pthread_mutex_lock(&tx_mutex);
      pthread_cond_wait(&tx_ready_cond, &tx_mutex);
      pthread_mutex_unlock(&tx_mutex);
      if (strcmp(check_hash, previous_hash) != 0) {
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
    do
    {
      block->timestamp = time(NULL);
      result = proof_of_work(block);
    } while (result.error == 1);

    print_block_info(block);

    send_block_to_validator(block, bytes);

    

    // enviar para o validator
    free(picked);
    free(block);
  }
}

void run_miner(int num_miners) {

  log_message("[Miner] Genesis hash: %s\n", previous_hash);

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

// Transaction generate_random_transaction(int tx_number) {
//   Transaction tx;

//   // Generate a unique tx_id (e.g., PID + transaction number)
//   snprintf(tx.TX_ID, TX_ID_LEN, "TX-%d-%d", getpid(), tx_number);

//   // Reward: initially from 1 to 3
//   tx.reward = rand() % 3 + 1;

//   // 3 % chance of aging, 1% of doubling aging
//   int age_chance = rand() % 101;
//   if (age_chance <= 3) tx.reward++;
//   if (age_chance <= 1) tx.reward++;

//   // Value: random float between 0.01 and 100.00
//   tx.value = ((float)(rand() % 10000)) / 100.0f + 0.01f;

//   // Timestamp: now
//   tx.timestamp = time(NULL);

//   return tx;
// }

// TransactionBlock generate_random_block(const char *prev_hash,
//                                        int block_number) {
//   TransactionBlock block;

//   // Generate a unique block ID using thread ID + number
//   pthread_t tid = pthread_self();
//   snprintf(block.txb_id, TXB_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
//            block_number);

//   // Copy the previous hash
//   strncpy(block.previous_block_hash, prev_hash, HASH_SIZE);

//   block.transactions =
//       (Transaction *)malloc(sizeof(Transaction) * transactions_per_block);

//   // Fill with random transactions
//   for (int i = 0; i < transactions_per_block; ++i) {
//     block.transactions[i] = generate_random_transaction(i);
//   }

//   PoWResult r;
//   do {
//     // Timestamp: current time
//     block.timestamp = time(NULL);
//     printf("Computing the PoW with timestamp %ld\n", block.timestamp);
//     r = proof_of_work(&block);

//   } while (r.error == 1);

//   return block;
// }

void print_block_info(TransactionBlock *block) {
  // Print basic block info
  printf("Block ID: %s\n", block->txb_id);
  printf("Previous Hash:\n%s\n", block->previous_block_hash);
  printf("Block Timestamp: %ld\n", block->timestamp);
  printf("Nonce: %u\n", block->nonce);
  printf("Transactions:\n");

  for (int i = 0; i < cfg.TRANSACTIONS_PER_BLOCK; ++i) {
    Transaction tx = block->transactions[i];
    printf("  [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld\n", i,
           tx.TX_ID, tx.reward, tx.value, tx.timestamp);
  }
}

// void blkcpy(TransactionBlock *dest, TransactionBlock *src) {
//   dest->nonce = src->nonce;
//   dest->timestamp = src->timestamp;
//   dest->transactions = src->transactions;
//   strcpy(dest->txb_id, src->txb_id);
//   strcpy(dest->previous_block_hash, src->previous_block_hash);
// }
