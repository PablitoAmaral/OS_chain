// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

    printf("[TxGen] Parâmetros válidos. Reward: %d | Sleep time: %d ms\n", reward, sleep_time);

    return 0;
}
