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
#include "ipc_utils.h"
#include "config.h"

#define LOG_FILE "DEIChain_log.txt"

pid_t miner_pid, validator_pid, stats_pid;
int log_fd;

void cleanup() {
    dprintf(log_fd, "[Controller] Encerrando sistema...\n");

    // Envia SIGTERM para os processos filhos (caso existam)
    if (miner_pid > 0) kill(miner_pid, SIGTERM);
    if (validator_pid > 0) kill(validator_pid, SIGTERM);
    if (stats_pid > 0) kill(stats_pid, SIGTERM);

    // Espera pelos filhos terminarem
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // Remove as memórias partilhadas
    if (shm_pool_id != -1) {
        shmctl(shm_pool_id, IPC_RMID, NULL);
        dprintf(log_fd, "[Controller] Memória partilhada do pool de transações removida.\n");
    }

    if (shm_ledger_id != -1) {
        shmctl(shm_ledger_id, IPC_RMID, NULL);
        dprintf(log_fd, "[Controller] Memória partilhada do ledger removida.\n");
    }

    // Fecha o ficheiro de log
    close(log_fd);
    dprintf(STDOUT_FILENO, "[Controller] Log fechado. A sair...\n");

    exit(0);
}

void handle_sigint(int sig) {
    dprintf(log_fd, "[Controller] Sinal SIGINT recebido. Limpando recursos...\n");
    cleanup();
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

    // Talvez colocar as validações em uma função? se tiver mais claro
    if (cfg.NUM_MINERS <= 0 || cfg.NUM_MINERS > 100) {
        dprintf(log_fd, "Erro: NUM_MINERS inválido: %d\n", cfg.NUM_MINERS);
        exit(1);
    }

    // 2. Criar memórias partilhadas
    shm_pool_id = create_transaction_pool(cfg.TRANSACTION_POOL_SIZE);
    shm_ledger_id = create_ledger(cfg.BLOCK_SIZE);

    // // 3. Criar fila de mensagens
    // msgq_id = create_message_queue();

    // // 4. Criar named pipe
    // create_named_pipe();

    // 5. Criar processos filhos
    char num_miners_str[10];
    sprintf(num_miners_str, "%d", cfg.NUM_MINERS);
    if ((miner_pid = fork()) == 0) {
        execl("./miner", "miner", num_miners_str, NULL);
        perror("Erro ao iniciar miner");
        exit(1);
    }

    if ((validator_pid = fork()) == 0) {
        execl("./validator", "validator", NULL);
        perror("Erro ao iniciar validator");
        exit(1);
    }

    if ((stats_pid = fork()) == 0) {
        execl("./statistics", "statistics", NULL);
        perror("Erro ao iniciar statistics");
        exit(1);
    }

    // 6. Esperar sinais ou término manual
    pause();

    return 0;
}
