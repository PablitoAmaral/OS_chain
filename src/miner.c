// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* miner_thread(void* arg) {
    int id = *((int*)arg);
    printf("[Miner Thread %d] A trabalhar na mineração...\n", id);
    // Simular trabalho com sleep
    sleep(1);
    printf("[Miner Thread %d] Terminado!\n", id);
    free(arg); // Libertar memória alocada
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
