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
#include "config.h"

volatile sig_atomic_t running = 1;

#define TX_POOL_SEM_NAME "/DEIChain_txpoolsem"
#define TX_POOL_SHM_FILE "config.cfg"
#define TX_POOL_SHM_ID 'T'


void handle_sigint(int sig) {
    running = 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <reward> <sleep_time_ms>\n", argv[0]);
        exit(1);
    }

    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);

    // Validar reward: 1 a 3
    if (reward < 1 || reward > 3) {
        fprintf(stderr, "Erro: reward deve estar entre 1 e 3.\n");
        exit(1);
    }

    // Validar sleep time: 200 a 3000
    if (sleep_time < 200 || sleep_time > 3000) {
        fprintf(stderr, "Erro: sleep_time deve estar entre 200 e 3000 (ms).\n");
        exit(1);
    }

    signal(SIGINT, handle_sigint);

    printf("[TxGen] Parâmetros válidos. Reward: %d | Sleep time: %d ms\n", reward, sleep_time);

    // Aceder à memória partilhada
    key_t key = ftok(TX_POOL_SHM_FILE, TX_POOL_SHM_ID);
    if (key == -1) {
        perror("Erro no ftok");
        exit(1);
    }

    // Obter TX_POOL_SIZE
    Config cfg = read_config("config.cfg");

    int shm_id = shmget(key, sizeof(TransactionSlot) * cfg.TX_POOL_SIZE, 0666);
    if (shm_id == -1) {
        perror("Erro ao obter memória partilhada");
        exit(1);
    }

    TransactionPool* pool = (TransactionPool*) shmat(shm_id, NULL, 0);
    if (pool == (void*)-1) {
        perror("Erro ao mapear memória");
        exit(1);
    }

    sem_t* sem = sem_open(TX_POOL_SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("Erro ao abrir semáforo do pool");
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

        bool inserted = false;

        if (sem_wait(sem) == -1) {
            perror("sem_wait");
            break;
        }

        for (int i = 0; i < cfg.TX_POOL_SIZE; i++) {
            if (pool->transactions_pending_set[i].empty) {
                pool->transactions_pending_set[i].tx = tx;
                pool->transactions_pending_set[i].empty = 0;
                pool->transactions_pending_set[i].age = 0;
                inserted = true;
                printf("[TxGen] Transação %s inserida na posição %d\n", tx.TX_ID, i);
                break;
            }
        }

        if (sem_post(sem) == -1) {
            perror("sem_post");
            break;
        }

        if (!inserted) {
            printf("[TxGen] Pool cheia. Transação descartada: %s\n", tx.TX_ID);
        }

        usleep(sleep_time * 1000);  // sleep em milissegundos
    }

    sem_close(sem);
    shmdt(pool);

    printf("[TxGen] Finalizado com sucesso.\n");


    return 0;
}
