// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "config.h"

// IDs de memória partilhada
extern int shm_pool_id;
extern int shm_ledger_id;

// Estruturas partilhadas
typedef struct {
    int id;
    char data[256];  // Placeholder para uma transação
} Transaction;

typedef struct {
    Transaction *transactions;
    int next;
    int size;  // guarda o tamanho real
} TransactionPool;

typedef struct {
    Transaction *blocks;
    int current_block;
    int size;
} Ledger;

// Funções de criação
int create_transaction_pool(int size);
int create_ledger(int size);

#endif