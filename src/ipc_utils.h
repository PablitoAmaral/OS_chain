// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "config.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define TX_ID_LEN 64 //???
#define TXB_ID_LEN 64 // ???
#define HASH_SIZE 65  // SHA256_DIGEST_LENGTH * 2 + 1
#define BLOCK_MSG_QUEUE_FILE "/tmp/DEIChain"
#define BLOCK_MSG_QUEUE_ID   'B'
#define VALIDATOR_FIFO "/tmp/VALIDATOR_INPUT"


extern sem_t *empty, *full, *ledger_sem,*txpool_sem;
extern Config cfg; 
extern int stats_msqid;

// IDs de memória partilhada
extern char previous_hash[HASH_SIZE];
extern int shm_pool_id;
extern int shm_ledger_id;
extern Config cfg;



// ✅
typedef struct {
    char TX_ID[TX_ID_LEN];     // ex: TX-<PID>-<#>
    int reward;                // 1-3 or more with aging
    float value;               // transaction value
    time_t timestamp;          // timestamp da criação
} Transaction;



// Transaction Block structure
typedef struct {
  char txb_id[TXB_ID_LEN];              // Unique block ID (e.g., ThreadID + #)
  char previous_block_hash[HASH_SIZE];  // Hash of the previous block
  time_t timestamp;                     // Time when block was created
  unsigned int nonce;                   // PoW solution
  Transaction transactions[];            // Array of transactions
} TransactionBlock;

typedef struct {
    long   mtype;
    int    miner_id;
    int    valid;             // 1 = accepted by validator, 0 = rejected
    int    credits;           // (0 if invalid)
    time_t block_timestamp;
} StatsMsg;

// Entrada na pool de transações ✅
typedef struct {
    int empty;        // 1 = slot livre, 0 = ocupado
    int age;          // idade da transação
    Transaction tx;   // dados da transação
} TransactionSlot;

typedef struct {
    int size;  // campo fixo obrigatório ❌ 
    TransactionSlot transactions_pending_set[]; 
} TransactionPool;

typedef struct {
    char previous_hash[HASH_SIZE];
    int current_block;
    int size;
    TransactionBlock blocks[];  // array de blocos de transações
} Ledger;

int create_transaction_pool(int size);
int create_ledger(int size, int transaction_size);

static inline size_t get_transaction_block_size() {
  if (cfg.TRANSACTIONS_PER_BLOCK == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TransactionBlock) +
         cfg.TRANSACTIONS_PER_BLOCK * sizeof(Transaction);
}

#endif