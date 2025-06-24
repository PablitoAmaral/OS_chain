#include "statistics.h"
#include "ipc_utils.h"
#include "log.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int max_miners;
int *valid_blocks;
int *invalid_blocks;
int *credits_sum;

long long total_tx_verify_time;
long long total_tx_count;
int total_msgs;
int total_chain_blocks;

struct sigaction sig_int, sig_usr;
sigset_t block_set;

void statistic_handle_sigusr1(int sig);
void print_statiscts(void);
void log_statiscts(void);

void termination_handler(int signum) {
  if (signum == SIGINT) {
    log_statiscts();
    exit(0);
  }
  if (signum == SIGUSR1) {
    print_statiscts();
  }
}

void run_statistics(void) {
  sigfillset(&block_set); // will have all possible signals blocked when our
                          // handler is called

  // define a handler for SIGINT, SIGTSTP and SIGUSR1; when entered all possible
  // signals are blocked

  
  sig_int.sa_flags = 0;
  sig_int.sa_mask = block_set;
  sig_int.sa_handler = &termination_handler;

    sig_usr.sa_flags = SA_RESTART;
    sig_usr.sa_mask = block_set;
    sig_usr.sa_handler = &termination_handler;

  sigaction(SIGINT, &sig_int, NULL);
  sigaction(SIGUSR1, &sig_usr, NULL);
  key_t key = ftok(BLOCK_MSG_QUEUE_FILE, BLOCK_MSG_QUEUE_ID);
  int msqid = msgget(key, IPC_CREAT | 0666);
  if (msqid < 0) {
    perror("statistics msgget");
    exit(1);
  }

  max_miners = cfg.NUM_MINERS;
  valid_blocks = calloc(max_miners + 1, sizeof(int));
  invalid_blocks = calloc(max_miners + 1, sizeof(int));
  credits_sum = calloc(max_miners + 1, sizeof(int));

  total_tx_verify_time = 0;
  total_tx_count = 0;
  total_msgs = 0;
  total_chain_blocks = 0;

  log_message("[Statistics] Processo iniciado!\n");
  StatsMsg sm;
  while (1) {
    if (msgrcv(msqid, &sm, sizeof(sm) - sizeof(long), 0, 0) < 0) {
      perror("statistics msgrcv");
      break;
    }
    total_msgs++;

    int id = sm.miner_id;
    if (id < 1 || id > max_miners)
      id = 0;

    if (sm.valid) {
      valid_blocks[id]++;
      total_chain_blocks++;
      credits_sum[id] += sm.credits;

      // compute total tx-verify delay for this block
      time_t now = time(NULL);
      long block_delay = now - sm.block_timestamp;
      // each block has cfg.TRANSACTIONS_PER_BLOCK transactions
      total_tx_verify_time +=
          (long long)block_delay * cfg.TRANSACTIONS_PER_BLOCK;
      total_tx_count += cfg.TRANSACTIONS_PER_BLOCK;
    } else {
      invalid_blocks[id]++;
    }
  }

  free(valid_blocks);
  free(invalid_blocks);
  free(credits_sum);
}


void print_statiscts(void) {
  // print report
  printf("\n===== Statistics Report =====\n");
  for (int m = 1; m <= max_miners; m++) {
    printf("Miner %d: valid=%d  invalid=%d  credits=%d\n", m, valid_blocks[m],
           invalid_blocks[m], credits_sum[m]);
  }
  double avg_tx =
      total_tx_count ? (double)total_tx_verify_time / total_tx_count : 0.0;
  printf("Average tx verify time: %.2f s\n", avg_tx);
  printf("Total messages received: %d\n", total_msgs);
  printf("Total blocks in blockchain: %d\n", total_chain_blocks);
  printf("===============================\n\n");
}

void log_statiscts(void) {
  // print report
  log_message("\n===== Statistics Report =====\n");
  for (int m = 1; m <= max_miners; m++) {
    log_message("Miner %d: valid=%d  invalid=%d  credits=%d\n", m,
                valid_blocks[m], invalid_blocks[m], credits_sum[m]);
  }
  double avg_tx =
      total_tx_count ? (double)total_tx_verify_time / total_tx_count : 0.0;
  log_message("Average tx verify time: %.2f s\n", avg_tx);
  log_message("Total messages received: %d\n", total_msgs);
  log_message("Total blocks in blockchain: %d\n", total_chain_blocks);
  log_message("===============================\n\n");
}
