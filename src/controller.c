// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include "config.h"
#include "ipc_utils.h"
#include "miner.h"
#include "statistics.h"
#include "validator.h"
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> // Para wait()
#include <unistd.h>

#define LOG_FILE "DEIChain_log.txt"

pid_t miner_pid, validator_pid, stats_pid;
int log_fd;

// Função auxiliar para escrever no log E no terminal
void log_message(const char *format, ...) {
  va_list args;
  va_start(args, format);

  // Log no ficheiro
  vdprintf(log_fd, format, args);

  // Reutiliza a lista de argumentos para imprimir no ecrã
  va_end(args);
  va_start(args, format);
  vprintf(format, args);

  va_end(args);
}

void cleanup() {
  log_message("[Controller] Encerrando sistema...\n");

  // Envia SIGTERM para os processos filhos
  if (miner_pid > 0)
    kill(miner_pid, SIGTERM);
  if (validator_pid > 0)
    kill(validator_pid, SIGTERM);
  if (stats_pid > 0)
    kill(stats_pid, SIGTERM);

  // Espera pelos filhos terminarem
  wait(NULL);
  wait(NULL);
  wait(NULL);

  // Remove as memórias partilhadas
  if (shm_pool_id != -1) {
    shmctl(shm_pool_id, IPC_RMID, NULL);
    log_message(
        "[Controller] Memória partilhada do pool de transações removida.\n");
  }

  if (shm_ledger_id != -1) {
    shmctl(shm_ledger_id, IPC_RMID, NULL);
    log_message("[Controller] Memória partilhada do ledger removida.\n");
  }

  close(log_fd);
  log_message("[Controller] Log fechado. A sair...\n");
  exit(0);
}

void handle_sigint(int sig) {
  log_message("[Controller] Sinal SIGINT recebido. Limpando recursos...\n");
  cleanup();
}

int main() {
  signal(SIGINT, handle_sigint);

  log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (log_fd == -1) {
    perror("Erro ao criar log");
    exit(1);
  }

  log_message("[Controller] Iniciando DEIChain...\n");

  // 1. Ler ficheiro de configuração
  Config cfg = read_config("config.cfg");
  log_message("[Controller] Configuração lida: NUM_MINERS=%d, POOL=%d, "
              "BLOCKCHAIN_BLOCKS=%d\n",
              cfg.NUM_MINERS, cfg.TRANSACTION_POOL_SIZE, cfg.BLOCKCHAIN_BLOCKS);

  if (cfg.NUM_MINERS <= 0 || cfg.NUM_MINERS > 100) {
    log_message("Erro: NUM_MINERS inválido: %d\n", cfg.NUM_MINERS);
    exit(1);
  }

  // Criar memórias partilhadas
  shm_pool_id = create_transaction_pool(cfg.TRANSACTION_POOL_SIZE);
  shm_ledger_id = create_ledger(cfg.BLOCKCHAIN_BLOCKS);
  log_message("[Controller] Memórias partilhadas criadas com sucesso.\n");

  if ((miner_pid = fork()) == 0) {
    run_miner(cfg.NUM_MINERS);
    exit(0);
  } else if (miner_pid < 0) {
    log_message("Erro: falha ao criar processo Miner\n");
    cleanup(); // Liberta recursos, encerra com segurança
  }

  if ((validator_pid = fork()) == 0) {
    run_validator();
    exit(0);
  } else if (validator_pid < 0) {
    log_message("Erro: falha ao criar processo Validator\n");
    cleanup();
  }

  if ((stats_pid = fork()) == 0) {
    run_statistics();
    exit(0);
  } else if (stats_pid < 0) {
    log_message("Erro: falha ao criar processo Statistics\n");
    cleanup();
  }

  log_message("[Controller] Processos filhos criados com sucesso.\n");

  // Esperar sinais ou término manual
  pause();

  return 0;
}
