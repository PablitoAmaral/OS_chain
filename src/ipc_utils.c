// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
#include "ipc_utils.h"

int shm_pool_id;
int shm_ledger_id;

int create_transaction_pool(int size) {
    key_t key = ftok("config.cfg", 'T');
    size_t total_size = sizeof(TransactionSlot) * size;
    shm_pool_id = shmget(key, total_size, IPC_CREAT | 0666);
    if (shm_pool_id == -1) {
        perror("Erro ao criar memória partilhada para o pool de transações");
        exit(1);
    }
    TransactionPool* pool = (TransactionPool*) shmat(shm_pool_id, NULL, 0);
    if (pool == (void*) -1) {
        perror("Erro ao mapear memória (shmat)");
        exit(1);
    }

    pool->size = size;

    // Inicializar os slots como vazios
for (int i = 0; i < size; i++) {
    pool->transactions_pending_set[i].empty = 1;
    pool->transactions_pending_set[i].age = 0;
    memset(&pool->transactions_pending_set[i].tx, 0, sizeof(Transaction));
}

shmdt(pool);

    return shm_pool_id;
}

int create_ledger(int size) {
    shm_ledger_id = shmget(IPC_PRIVATE, sizeof(Ledger), IPC_CREAT | 0666);
    if (shm_ledger_id == -1) {
        perror("Erro ao criar memória partilhada para o ledger");
        exit(1);
    }
    return shm_ledger_id;
}