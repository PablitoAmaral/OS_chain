// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <string.h>
#include <time.h>
#include "ipc_utils.h"
#include "pow.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define LEDGER_SIZE 4

// Shared chain state
static char previous_hash[HASH_SIZE];
static pthread_mutex_t header_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned block_counter = 0;

TransactionBlock ledger[LEDGER_SIZE];

static void print_block_info(TransactionBlock *block);
static void blkcpy(TransactionBlock *dest, TransactionBlock *src);
static void dump_ledger(void);

// Compute genesis hash once
static void init_genesis_hash(void)
{
  TransactionBlock empty = {0};
  empty.transactions = calloc(transactions_per_block, sizeof(Transaction));
  PoWResult r = proof_of_work(&empty);
  if (r.error)
  {
    fprintf(stderr, "Failed to compute genesis hash\n");
    exit(1);
  }
  strncpy(previous_hash, r.hash, HASH_SIZE);
  free(empty.transactions);
}

void *miner_thread(void *arg)
{
  int id = *((int *)arg);
  free(arg);

  while (1)
  {
    // 1) Build block header under lock
    TransactionBlock block;
    memset(&block, 0, sizeof(block));
    block.transactions = calloc(transactions_per_block, sizeof(Transaction));

    unsigned idx;
    pthread_mutex_lock(&header_lock);
    idx = block_counter++;
    strncpy(block.previous_block_hash, previous_hash, HASH_SIZE);
    pthread_mutex_unlock(&header_lock);

    snprintf(block.txb_id, TXB_ID_LEN,
             "MINER-%d-BLOCK-%u", id, idx);

    // 2) Pull real TXs from pool
    key_t key = ftok(TX_POOL_SHM_FILE, TX_POOL_SHM_ID);
    int shm_id = shmget(key, 0, 0);
    TransactionPool *pool = shmat(shm_id, NULL, 0);
    sem_t *txsem = sem_open(TX_POOL_SEM_NAME, 0);

    int found = 0;
    sem_wait(txsem);
    // Iterate through the transaction pool to find available transactions
    for (int i = 0; i < pool->size && found < transactions_per_block; i++)
    {
      if (!pool->transactions_pending_set[i].empty)
      {
        block.transactions[found++] =
            pool->transactions_pending_set[i].tx;
        pool->transactions_pending_set[i].empty = 1;
      }
    }
    sem_post(txsem);
    sem_close(txsem);
    shmdt(pool);

    // If not enough TXs, back off and retry
    if ((size_t)found < transactions_per_block)
    {
      free(block.transactions);
      sleep(1);
      continue;
    }

    // 3) PoW loop
    PoWResult result;
    do
    {
      block.timestamp = time(NULL);
      result = proof_of_work(&block);
    } while (result.error == 1);

    // 4) Commit hash into chain state
    pthread_mutex_lock(&header_lock);
    strncpy(previous_hash, result.hash, HASH_SIZE);
    // Optionally store into local ledger for dump
    if (idx < LEDGER_SIZE)
      ledger[idx] = block;
    pthread_mutex_unlock(&header_lock);

    // 5) Output & handoff
    printf("[Miner %d] Mined %s → %s in %.3f s (%d ops)\n",
           id, block.txb_id, result.hash,
           result.elapsed_time, result.operations);
    print_block_info(&block);

    // TODO: serialize & send block to Validator via your chosen IPC

    free(block.transactions);
  }

  return NULL;
}

void run_miner(int num_miners)
{

  // Set up genesis
  init_genesis_hash();
  printf("[Miner] Genesis hash: %s\n", previous_hash);

  pthread_t threads[num_miners];
  printf("[Miner] Processo miner iniciado com %d threads\n", num_miners);

  for (int i = 0; i < num_miners; i++)
  {
    int *id = malloc(sizeof(int));
    *id = i + 1;
    if (pthread_create(&threads[i], NULL, miner_thread, id) != 0)
    {
      perror("Erro ao criar thread");
      exit(1);
    }
  }

  for (int i = 0; i < num_miners; i++)
  {
    pthread_join(threads[i], NULL);
  }

  printf("[Miner] Todas as threads terminaram.\n");
}

void print_block_info(TransactionBlock *block)
{
  // Print basic block info
  printf("Block ID: %s\n", block->txb_id);
  printf("Previous Hash:\n%s\n", block->previous_block_hash);
  printf("Block Timestamp: %ld\n", block->timestamp);
  printf("Nonce: %u\n", block->nonce);
  printf("Transactions:\n");

  for (int i = 0; i < transactions_per_block; ++i)
  {
    Transaction tx = block->transactions[i];
    printf("  [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld\n", i,
           tx.TX_ID, tx.reward, tx.value, tx.timestamp);
  }
}

void blkcpy(TransactionBlock *dest, TransactionBlock *src)
{
  dest->nonce = src->nonce;
  dest->timestamp = src->timestamp;
  dest->transactions = src->transactions;
  strcpy(dest->txb_id, src->txb_id);
  strcpy(dest->previous_block_hash, src->previous_block_hash);
}

void dump_ledger()
{
  printf("\n=================== Start Ledger ===================\n");
  for (int i = 0; i < LEDGER_SIZE; i++)
  {
    printf("||----  Block %03d -- \n", i);
    print_block_info(&(ledger[i]));
    printf("||------------------------------ \n");
  }
  printf("=================== End   Ledger ===================\n");
}