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
    shm_pool_id = shmget(key, sizeof(TransactionPool) + sizeof(Transaction) * size, IPC_CREAT | 0666);
    if (shm_pool_id == -1) {
        perror("Erro ao criar memória partilhada para o pool de transações");
        exit(1);
    }
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