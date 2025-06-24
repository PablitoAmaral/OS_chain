// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include "config.h"
#include "ipc_utils.h"
#include "log.h"
#include "miner.h"
#include "pow.h"
#include "statistics.h"
#include "validator.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h> // POSIX semaphores
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

#define LOG_FILE "DEIChain_log.log"
#define LOG_SEMAPHORE "DEIChain_logsem"
#define TX_POOL_SEMAPHORE "DEIChain_txpoolsem"
#define HASH_SIZE 65


static pid_t validators[3];
static int n_validators;

unsigned block_counter = 0;
Config cfg;
pid_t miner_pid, validator_pid, stats_pid;
int log_fd;
sem_t *log_sem, *txpool_sem, *empty, *full, *ledger_sem;

void init_ipc(void);
void *validator_spawner_thread(void *arg);
void spawn_validator();
void kill_extra_validator();
void cleanup();
void log_message(const char *format, ...);
void handle_sigint(int sig);
void handle_sigusr1(int sig);
void initi_processes(void);

int main() {
  signal(SIGINT, handle_sigint);
  signal(SIGUSR1, handle_sigusr1);

  // mascara para bloquear sigurs2
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);

  // creating all the ipcs
  init_ipc();

  log_message("[Controller] Iniciando DEIChain...\n");

  // creating all processes
  initi_processes();

  // creating the control thread
  pthread_t monitor_ledger;
  pthread_create(&monitor_ledger, NULL, validator_spawner_thread, NULL);

  while (1) {
    pause();
  }
  return 0;
}

void init_ipc(void) {

  // --------------   2) Semáforos and Log file----------------
  // log_message function semaphore
  sem_unlink(LOG_SEMAPHORE);
  log_sem = sem_open(LOG_SEMAPHORE, O_CREAT | O_EXCL, 0600, 1);
  if (log_sem == SEM_FAILED)
    perror("sem_open LOG"), exit(1);


  log_fd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0600);
  if (log_fd == -1)
    perror("open log"), exit(1);

  // reads the config file and check everything
  cfg = read_config("config.cfg");

  

  
  // Transaction pool semaphore
  sem_unlink(TX_POOL_SEMAPHORE);
  txpool_sem = sem_open(TX_POOL_SEMAPHORE, O_CREAT | O_EXCL, 0600, 1);
  if (txpool_sem == SEM_FAILED)
    perror("sem_open TX_POOL"), exit(1);

  // empty semaphote -- when we need to wait it means that we have no empty
  // spots available.
  sem_unlink("empty");
  empty = sem_open("empty", O_CREAT | O_EXCL, 0600, cfg.TX_POOL_SIZE);
  if (empty == SEM_FAILED)
    perror("sem_open empty"), exit(1);

  // full semaphore.
  sem_unlink("full");
  full = sem_open("full", O_CREAT | O_EXCL, 0600, 0);
  if (full == SEM_FAILED)
    perror("sem_open full"), exit(1);

  // Ledger semaphore
  sem_unlink("Ledger");
  ledger_sem = sem_open("Ledger", O_CREAT | O_EXCL, 0600, 1);
  if (ledger_sem == SEM_FAILED)
    perror("sem_open ledger"), exit(1);

  log_message("[Controller] Semáforos criados com sucesso\n");

  // 3) ---------------   Named pipe. -------------------
  if (mkfifo(VALIDATOR_FIFO, 0600) < 0 && errno != EEXIST)
    perror("mkfifo"), exit(1);

  log_message("[Controller] NAMED PIPE criado\n");

  // 4) --------------- Shared memories ---------------
  shm_pool_id = create_transaction_pool(cfg.TX_POOL_SIZE);
  shm_ledger_id = create_ledger(cfg.BLOCKCHAIN_BLOCKS, cfg.TX_POOL_SIZE);
  log_message("[Controller] Memórias partilhadas criadas com sucesso.\n");

  log_message("[Controller] IPCs inicializados com sucesso.\n");
}

void initi_processes(void) {
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
  validators[0] = validator_pid;
  n_validators = 1;

  if ((stats_pid = fork()) == 0) {
    run_statistics();
    exit(0);
  } else if (stats_pid < 0) {
    log_message("Erro: falha ao criar processo Statistics\n");
    cleanup();
  }
}

void *validator_spawner_thread(void *arg) {
  // 1) preparar conjunto com SIGUSR2
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);

  TransactionPool *pool = shmat(shm_pool_id, NULL, 0);
  if (pool == (void *)-1) {
    perror("shmat pool");
    exit(1);
  }

  for (;;) {
    int sig;

    // aguarda pelo sigurs2
    sigwait(&mask, &sig);

    // calcula ocupação actual
    sem_wait(txpool_sem);
    int pend = 0;
    for (int i = 0; i < pool->size; i++)
      if (!pool->transactions_pending_set[i].empty)
        pend++;
    sem_post(txpool_sem);

    float occ = (float)pend / pool->size;

    if (occ >= 0.80f && n_validators < 3) {
      spawn_validator();
    } else if (occ >= 0.60f && n_validators < 2) {
      spawn_validator();
    } else if (occ < 0.40f && n_validators > 1) {
      while (n_validators > 1) {
        kill_extra_validator();
      }
    }
  }

  return NULL;
}

void spawn_validator() {
  pid_t pid = fork();
  if (pid == 0) {
    run_validator();
    _exit(0);
  }
  if (pid > 0) {
    validators[n_validators++] = pid;
    log_message("[Controller] Spawned Validator #%d (pid=%d)\n", n_validators,
                pid);
  } else
    perror("fork");
}

void kill_extra_validator() {
  pid_t v = validators[--n_validators];
  kill(v, SIGTERM);
  waitpid(v, NULL, 0);
  log_message("[Controller] Killed Validator #%d (pid=%d)\n", n_validators + 1,
              v);
}

void handle_sigint(int sig) {
  log_message("[Controller] Sinal SIGINT recebido. Limpando recursos...\n");
  cleanup();
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
    log_message("[Controller] Miner (pid=%d) exited with %d\n", miner_pid,
                WEXITSTATUS(status));
  }
  if (validator_pid > 0) {
    waitpid(validator_pid, &status, 0);
    log_message("[Controller] Validator (pid=%d) exited with %d\n",
                validator_pid, WEXITSTATUS(status));
  }
  if (stats_pid > 0) {
    waitpid(stats_pid, &status, 0);
    log_message("[Controller] Statistics (pid=%d) exited with %d\n", stats_pid,
                WEXITSTATUS(status));
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

void handle_sigusr1(int sig) {
  // dump ledger to log file and to screen
  // it should dump the ledger content in the log according to the defined
  // format in this document
  // POR
  // IMPLEMENTAR---------------------------------------------------------------------
  log_message("[Controller] SIGUSR1 received: dumped ledger.\n");
}

// Logs a formatted message to file and terminal with semaphore protection
void log_message(const char *format, ...) {
  va_list args;
  va_start(args, format);

  if (sem_wait(log_sem) == -1) {
    perror("sem_wait");
  }

  // Log in the txt (DEIChain_log.log)
  vdprintf(log_fd, format, args);
  va_end(args);

  // A va_list can only be used once, so we must reinitialize it with va_start
  // before using it a second time.
  va_start(args, format);
  vprintf(format, args); // stdout

  if (sem_post(log_sem) == -1) {
    perror("sem_post");
  }

  va_end(args);
}