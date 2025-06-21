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

#define LEDGER_SIZE 4
TransactionBlock ledger[LEDGER_SIZE];

static Transaction          generate_random_transaction(int tx_number);
static TransactionBlock     generate_random_block(const char *prev_hash, int block_number);
static void                 print_block_info(TransactionBlock *block);
static void                 blkcpy(TransactionBlock *dest, TransactionBlock *src);
static void                 dump_ledger(void);

void* miner_thread(void* arg) {
    int id = *((int*)arg);
    printf("[Miner Thread %d] Starting block mining…\n", id);

    // 1. Build a block
    TransactionBlock block;
    memset(&block, 0, sizeof(block));
    block.transactions = calloc(transactions_per_block, sizeof(Transaction));
    for (size_t i = 0; i < transactions_per_block; i++) {
        block.transactions[i] = generate_random_transaction(i);
    }

    // 2. Run PoW (retry on max-ops)
    PoWResult result;
    do {
        block.timestamp = time(NULL);
        result = proof_of_work(&block);
    } while (result.error == 1);

    // 3. Print outcome
    printf("[Miner Thread %d] Mined block %s in %.3f s (%d ops)\n",
           id, block.txb_id, result.elapsed_time, result.operations);
    print_block_info(&block);

    // 4. Clean up
    free(block.transactions);
    free(arg);
    return NULL;
}

void run_miner(int num_miners) {
    pthread_t threads[num_miners];

    printf("[Miner] Processo miner iniciado com %d threads\n", num_miners);

    for (int i = 0; i < num_miners; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&threads[i], NULL, miner_thread, id) != 0) {
            perror("Erro ao criar thread");
            exit(1);
        }
    }

    for (int i = 0; i < num_miners; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("[Miner] Todas as threads terminaram.\n");
}

Transaction generate_random_transaction(int tx_number) {
  Transaction tx;

  // Generate a unique tx_id (e.g., PID + transaction number)
  snprintf(tx.TX_ID, TX_ID_LEN, "TX-%d-%d", getpid(), tx_number);

  // Reward: initially from 1 to 3
  tx.reward = rand() % 3 + 1;

  // 3 % chance of aging, 1% of doubling aging
  int age_chance = rand() % 101;
  if (age_chance <= 3) tx.reward++;
  if (age_chance <= 1) tx.reward++;

  // Value: random float between 0.01 and 100.00
  tx.value = ((float)(rand() % 10000)) / 100.0f + 0.01f;

  // Timestamp: now
  tx.timestamp = time(NULL);

  return tx;
}

TransactionBlock generate_random_block(const char *prev_hash,
                                       int block_number) {
  TransactionBlock block;

  // Generate a unique block ID using thread ID + number
  pthread_t tid = pthread_self();
  snprintf(block.txb_id, TXB_ID_LEN, "BLOCK-%lu-%d", (unsigned long)tid,
           block_number);

  // Copy the previous hash
  strncpy(block.previous_block_hash, prev_hash, HASH_SIZE);

  block.transactions =
      (Transaction *)malloc(sizeof(Transaction) * transactions_per_block);

  // Fill with random transactions
  for (int i = 0; i < transactions_per_block; ++i) {
    block.transactions[i] = generate_random_transaction(i);
  }

  PoWResult r;
  do {
    // Timestamp: current time
    block.timestamp = time(NULL);
    printf("Computing the PoW with timestamp %ld\n", block.timestamp);
    r = proof_of_work(&block);

  } while (r.error == 1);

  return block;
}

void print_block_info(TransactionBlock *block) {
  // Print basic block info
  printf("Block ID: %s\n", block->txb_id);
  printf("Previous Hash:\n%s\n", block->previous_block_hash);
  printf("Block Timestamp: %ld\n", block->timestamp);
  printf("Nonce: %u\n", block->nonce);
  printf("Transactions:\n");

  for (int i = 0; i < transactions_per_block; ++i) {
    Transaction tx = block->transactions[i];
    printf("  [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld\n", i,
           tx.TX_ID, tx.reward, tx.value, tx.timestamp);
  }
}

void blkcpy(TransactionBlock *dest, TransactionBlock *src) {
  dest->nonce = src->nonce;
  dest->timestamp = src->timestamp;
  dest->transactions = src->transactions;
  strcpy(dest->txb_id, src->txb_id);
  strcpy(dest->previous_block_hash, src->previous_block_hash);
}

void dump_ledger() {
  printf("\n=================== Start Ledger ===================\n");
  for (int i = 0; i < LEDGER_SIZE; i++) {
    printf("||----  Block %03d -- \n", i);
    print_block_info(&(ledger[i]));
    printf("||------------------------------ \n");
  }
  printf("=================== End   Ledger ===================\n");
}