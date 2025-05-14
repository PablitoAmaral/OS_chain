// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "config.h"
#include <time.h>

#define TX_ID_LEN 32

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