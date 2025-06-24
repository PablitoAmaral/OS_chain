// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#include "ipc_utils.h"

volatile sig_atomic_t running = 1; //✅

#define TX_POOL_SEMAPHORE "DEIChain_txpoolsem"
#define TX_POOL_SHM_FILE "config.cfg"
#define TX_POOL_SHM_ID 'T'

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <reward> <sleep_time_ms>\n", argv[0]);
        exit(1);
    }
    srand(time(NULL)^getpid());


    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);

    // Validar reward: 1 a 3
    //✅
    if (reward < 1 || reward > 3) {
        fprintf(stderr, "Erro: reward deve estar entre 1 e 3.\n");
        exit(1);
    }
    
    // Validar sleep time: 200 a 3000
    //✅
    if (sleep_time < 200 || sleep_time > 3000) {
        fprintf(stderr, "Erro: sleep_time deve estar entre 200 e 3000 (ms).\n");
        exit(1);
    }
    printf("[TxGen] Parâmetros válidos. Reward: %d | Sleep time: %d ms\n", reward, sleep_time);

    signal(SIGINT, handle_sigint); //🐸 need to check wtf is that shit


    // Just attach to the shared memory we don’t need to create it again
    //✅
    key_t key = ftok(TX_POOL_SHM_FILE, TX_POOL_SHM_ID);
    if (key == -1) {
        perror("Erro no ftok");
        exit(1);
    }

    int shm_id = shmget(key,0,0);
    if (shm_id == -1) {
        perror("shmget Txgen");
        exit(1);
    }

    TransactionPool* pool = (TransactionPool*) shmat(shm_id, NULL, 0);
    if (pool == (void*)-1) {
        perror("shmat TxGen");
        exit(1);
    }


    // Just open the semaphore we don’t need to create it again
    //✅
    sem_t* sem = sem_open(TX_POOL_SEMAPHORE, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open TxGen(TX_POOL_SEMAPHORE)");
        shmdt(pool);
        exit(1);
    }
    //✅
    sem_t* empty = sem_open("empty", 0);
    if (empty == SEM_FAILED) {
        perror("sem_open TxGen(empty)");
        shmdt(pool);
        exit(1);
    }
    //✅
    sem_t* full = sem_open("full", 0);
    if (full == SEM_FAILED) {
        perror("sem_open TxGen(full)");
        shmdt(pool);
        exit(1);
    }

    int tx_counter = 0;
    pid_t pid = getpid();

    printf("[TxGen] Iniciado com reward=%d e intervalo=%dms\n", reward, sleep_time);

    while (running) {
        Transaction tx;
        snprintf(tx.TX_ID, TX_ID_LEN, "TX-%d-%d", pid, tx_counter++);
        tx.reward = reward;
        tx.value = ((float)(rand() % 10000)) / 100.0f;  // entre 0.00 e 99.99
        tx.timestamp = time(NULL);


        if (sem_wait(empty) == -1) {
            perror("sem_wait");
            break;
        }

        if (sem_wait(sem) == -1) {
            perror("sem_wait");
            break;
        }

        for (int i = 0; i < pool->size; i++) {
            if (pool->transactions_pending_set[i].empty) {
                pool->transactions_pending_set[i].tx = tx;
                pool->transactions_pending_set[i].empty = 0;
                pool->transactions_pending_set[i].age = 0;
                printf("[TxGen] Transação %s inserida na posição %d\n", tx.TX_ID, i);
                break;
            }
        }

        if (sem_post(sem) == -1) {
            perror("sem_post");
            break;
        }

        if (sem_post(full) == -1) {
            perror("sem_post");
            break;
        }


        usleep(sleep_time * 1000);  // sleep em milissegundos

    }

    sem_close(sem);
    sem_close(full);
    sem_close(empty);
    shmdt(pool);

    printf("[TxGen] Finalizado com sucesso.\n");


    return 0;
}


