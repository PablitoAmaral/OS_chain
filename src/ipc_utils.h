// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "config.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TX_ID_LEN 64
#define TXB_ID_LEN 64
#define HASH_SIZE 65  // SHA256_DIGEST_LENGTH * 2 + 1

extern size_t transactions_per_block;

// IDs de memória partilhada
extern int shm_pool_id;
extern int shm_ledger_id;

// Estrutura da transação
typedef struct {
    char TX_ID[TX_ID_LEN];     // ex: TX-<PID>-<número>
    int reward;                // de 1 a 3, ou mais com aging
    float value;               // valor da transação
    time_t timestamp;          // timestamp da criação
} Transaction;

// Transaction Block structure
typedef struct {
  char txb_id[TXB_ID_LEN];              // Unique block ID (e.g., ThreadID + #)
  char previous_block_hash[HASH_SIZE];  // Hash of the previous block
  time_t timestamp;                     // Time when block was created
  Transaction *transactions;            // Array of transactions
  unsigned int nonce;                   // PoW solution
} TransactionBlock;

// Inline function to compute the size of a TransactionBlock
static inline size_t get_transaction_block_size() {
  if (transactions_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TransactionBlock) +
         transactions_per_block * sizeof(Transaction);
}

// Entrada na pool de transações
typedef struct {
    int empty;        // 1 = slot livre, 0 = ocupado
    int age;          // idade da transação
    Transaction tx;   // dados da transação
} TransactionSlot;

typedef struct {
    int size;  // campo fixo obrigatório
    TransactionSlot transactions_pending_set[];  // array flexível
} TransactionPool;

typedef struct {
    int current_block;
    int size;
    Transaction blocks[];  // array de blocos de transações
} Ledger;

int create_transaction_pool(int size);
int create_ledger(int size);

#endif