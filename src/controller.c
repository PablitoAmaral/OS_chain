// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include "config.h"
#include "ipc_utils.h"
#include "miner.h"
#include "statistics.h"
#include "validator.h"
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> // for var_star, var_end...
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
#include <semaphore.h>  // POSIX semaphores
#include "log.h"

#define LOG_FILE "DEIChain_log.log"
#define LOG_SEMAPHORE "DEIChain_logsem"
#define TX_POOL_SEMAPHORE "DEIChain_txpoolsem"

pid_t miner_pid, validator_pid, stats_pid;  //✅
int log_fd; //✅
sem_t *log_sem, *txpool_sem; //✅

// Logs a formatted message to file and terminal with semaphore protection //✅
void log_message(const char* format, ...) {
  va_list args;
  va_start(args, format);
    

  if (sem_wait(log_sem) == -1) {
    perror("sem_wait");
  }

  // Log in the txt (DEIChain_log.txt)
  vdprintf(log_fd, format, args);
  va_end(args);

  //A va_list can only be used once, so we must reinitialize it with va_start before using it a second time.
  va_start(args, format);
  vprintf(format, args); //stdout
  

  if (sem_post(log_sem) == -1) {
    perror("sem_post");
  }

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

  // Espera pelos filhos terminarem individualmente e regista o status
  int status;
  if (miner_pid > 0) {
      waitpid(miner_pid, &status, 0);
      log_message("[Controller] Miner (pid=%d) exited with %d\n",
                  miner_pid, WEXITSTATUS(status));
  }
  if (validator_pid > 0) {
      waitpid(validator_pid, &status, 0);
      log_message("[Controller] Validator (pid=%d) exited with %d\n",
                  validator_pid, WEXITSTATUS(status));
  }
  if (stats_pid > 0) {
      waitpid(stats_pid, &status, 0);
      log_message("[Controller] Statistics (pid=%d) exited with %d\n",
                  stats_pid, WEXITSTATUS(status));
  }

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

    

    log_message("[Controller] Log fechado. A sair...\n");

    // Remove o semáforo log
    sem_close(log_sem);
    sem_unlink(LOG_SEMAPHORE);

    // Remove o semáforo transaction pool
    if (txpool_sem != SEM_FAILED) {
    sem_close(txpool_sem);
    sem_unlink(TX_POOL_SEMAPHORE);
}

  
  close(log_fd);

  exit(0);
}

void handle_sigint(int sig) {
  log_message("[Controller] Sinal SIGINT recebido. Limpando recursos...\n");
  cleanup();
}

void handle_sigusr1(int sig) {
    // dump ledger to log file and to screen
    //it should dump the ledger content in the log according to the defined format in this document
    // POR IMPLEMENTAR---------------------------------------------------------------------
    log_message("[Controller] SIGUSR1 received: dumped ledger.\n");
  }

int main() {
  signal(SIGINT, handle_sigint);
  signal(SIGUSR1, handle_sigusr1);

  //log_message function semaphore //✅
  sem_unlink(LOG_SEMAPHORE);
  log_sem = sem_open(LOG_SEMAPHORE, O_CREAT | O_EXCL, 0600, 1);
  if (log_sem == SEM_FAILED) {
    perror("sem_open (LOG_SEMAPHORE)");
    exit(1);
  }

  //Transaction pool semaphore //✅
  sem_unlink(TX_POOL_SEMAPHORE);
  txpool_sem = sem_open(TX_POOL_SEMAPHORE, O_CREAT | O_EXCL, 0600, 1);
  if (txpool_sem == SEM_FAILED) {
    perror("sem_open (TX_POOL_SEMAPHORE)");
    exit(1);
  }

  //open DEIChain_log.txt //✅
  log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0600);
  if (log_fd == -1) {
    perror("open");
    exit(1);
  }

  log_message("[Controller] Iniciando DEIChain...\n"); //✅

  // reads the config file and check everything
  Config cfg = read_config("config.cfg");
  extern size_t transactions_per_block;    //🐸 need to check wtf is that shit
  transactions_per_block = cfg.TRANSACTIONS_PER_BLOCK; //🐸 need to check wtf is that shit

  

  // shared memorys
  shm_pool_id = create_transaction_pool(cfg.TX_POOL_SIZE);
  // shm_ledger_id = create_ledger(cfg.BLOCKCHAIN_BLOCKS);
  // log_message("[Controller] Memórias partilhadas criadas com sucesso.\n");

  // if ((miner_pid = fork()) == 0) {
  //   run_miner(cfg.NUM_MINERS);
  //   exit(0);
  // } else if (miner_pid < 0) {
  //   log_message("Erro: falha ao criar processo Miner\n");
  //   cleanup(); // Liberta recursos, encerra com segurança
  // }

  // if ((validator_pid = fork()) == 0) {
  //   run_validator();
  //   exit(0);
  // } else if (validator_pid < 0) {
  //   log_message("Erro: falha ao criar processo Validator\n");
  //   cleanup();
  // }

  // if ((stats_pid = fork()) == 0) {
  //   run_statistics();
  //   exit(0);
  // } else if (stats_pid < 0) {
  //   log_message("Erro: falha ao criar processo Statistics\n");
  //   cleanup();
  // }

  // log_message("[Controller] Processos filhos criados com sucesso.\n");

  // int status;
  // waitpid(miner_pid,    &status, 0);
  // log_message("[Controller] Miner (pid=%d) exited with %d\n", miner_pid, WEXITSTATUS(status));

  // waitpid(validator_pid, &status, 0);
  // log_message("[Controller] Validator (pid=%d) exited with %d\n", validator_pid, WEXITSTATUS(status));

  // waitpid(stats_pid,     &status, 0);
  // log_message("[Controller] Statistics (pid=%d) exited with %d\n", stats_pid,    WEXITSTATUS(status));
  sleep(60);
  return 0;
}
