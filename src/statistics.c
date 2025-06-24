#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>

#include "ipc_utils.h"
#include "log.h"
#include "statistics.h"

void run_statistics(void) {
  key_t key = ftok(BLOCK_MSG_QUEUE_FILE, BLOCK_MSG_QUEUE_ID);
  int msqid = msgget(key, IPC_CREAT | 0666);
  if (msqid < 0) {
    perror("statistics msgget");
    exit(1);
  }

  // Stats storage
  int max_miners = cfg.NUM_MINERS;
  int *valid_blocks = calloc(max_miners + 1, sizeof(int));
  int *invalid_blocks = calloc(max_miners + 1, sizeof(int));
  long long total_tx_verify_time = 0;
  long long total_tx_count = 0;
  int total_blocks = 0;
  int total_chain_blocks = 0;

  log_message("[Statistics] Processo iniciado!\n");
  BlockMsg msg;
  while (1) {
    if (msgrcv(msqid, &msg, sizeof(msg.block), 0, 0) < 0) {
      perror("statistics msgrcv");
      break;
    }
    total_blocks++;
    log_message("[Statistics] Got block %s\n", msg.block.txb_id);

    // Extract miner ID
    int miner_id = 0;
    if (sscanf(msg.block.txb_id, "MINER-%d-", &miner_id) != 1 || miner_id < 1 ||
        miner_id > max_miners) {
      miner_id = 0;
    }

    // Update counts
    valid_blocks[miner_id]++;
    total_chain_blocks++;

    // Compute per-tx verify times
    time_t now = time(NULL);
    for (int i = 0; i < cfg.TRANSACTIONS_PER_BLOCK; i++) {
      time_t ts = msg.block.transactions[i].timestamp;
      total_tx_verify_time += (long long)(now - ts);
      total_tx_count++;
    }

    // Direct print for testing
    printf("\n===== Statistics Report =====\n");
    for (int id = 1; id <= max_miners; id++) {
      printf("Miner %d: valid=%d  invalid=%d  credits=%d\n", id,
             valid_blocks[id], invalid_blocks[id], valid_blocks[id]);
    }
    double avg_tx =
        total_tx_count ? (double)total_tx_verify_time / total_tx_count : 0.0;
    printf("Average tx verify time: %.2f s\n", avg_tx);
    printf("Total validated blocks: %d\n", total_blocks);
    printf("Total blocks in blockchain: %d\n", total_chain_blocks);
    printf("===============================\n\n");
  }

  free(valid_blocks);
  free(invalid_blocks);
}