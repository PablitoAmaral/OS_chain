#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
// #include "common.h"
// #include "ipc_utils.h"
#include "config.h"

#define LOG_FILE "DEIChain_log.txt"

pid_t miner_pid, validator_pid, stats_pid;
int shm_pool_id = -1, shm_ledger_id = -1;
int msgq_id = -1;
int log_fd;

// void cleanup() {
//     dprintf(log_fd, "[Controller] Encerrando sistema...\n");

//     // Envia SIGTERM para todos os filhos
//     if (miner_pid > 0) kill(miner_pid, SIGTERM);
//     if (validator_pid > 0) kill(validator_pid, SIGTERM);
//     if (stats_pid > 0) kill(stats_pid, SIGTERM);

//     // Espera pelos processos filhos
//     wait(NULL); wait(NULL); wait(NULL);

//     // Remove recursos IPC
//     if (shm_pool_id != -1) shmctl(shm_pool_id, IPC_RMID, NULL);
//     if (shm_ledger_id != -1) shmctl(shm_ledger_id, IPC_RMID, NULL);
//     if (msgq_id != -1) msgctl(msgq_id, IPC_RMID, NULL);

//     unlink(PIPE_NAME);
//     close(log_fd);
//     exit(0);
// }

void handle_sigint(int sig) {
    dprintf(log_fd, "[Controller] Sinal SIGINT recebido. Limpando recursos...\n");
    // cleanup();
}

int main() {
    signal(SIGINT, handle_sigint);

    log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (log_fd == -1) {
        perror("Erro ao criar log");
        exit(1);
    }

    dprintf(log_fd, "[Controller] Iniciando DEIChain...\n");

    // 1. Ler ficheiro de configuração
    Config cfg = read_config("config.cfg");
    printf("%d\n", cfg.NUM_MINERS);

    // // 2. Criar memórias partilhadas
    // shm_pool_id = create_transaction_pool(cfg.TRANSACTION_POOL_SIZE);
    // shm_ledger_id = create_ledger(cfg.BLOCK_SIZE);

    // // 3. Criar fila de mensagens
    // msgq_id = create_message_queue();

    // // 4. Criar named pipe
    // create_named_pipe();

    // // 5. Criar processos filhos
    // if ((miner_pid = fork()) == 0) {
    //     execl("./miner", "miner", NULL);
    //     perror("Erro ao iniciar miner");
    //     exit(1);
    // }

    // if ((validator_pid = fork()) == 0) {
    //     execl("./validator", "validator", NULL);
    //     perror("Erro ao iniciar validator");
    //     exit(1);
    // }

    // if ((stats_pid = fork()) == 0) {
    //     execl("./statistics", "statistics", NULL);
    //     perror("Erro ao iniciar statistics");
    //     exit(1);
    // }

    // // 6. Esperar sinais ou término manual
    // pause();

    return 0;
}
