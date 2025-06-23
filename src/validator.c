#include <fcntl.h>
#include <unistd.h>
#include "ipc_utils.h"
#include "pow.h"
#include <stdbool.h>
#include <pthread.h>
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>

#define VALIDATOR_FIFO "/tmp/VALIDATOR_INPUT"
pthread_mutex_t ledger_lock = PTHREAD_MUTEX_INITIALIZER;


bool validate_block(TransactionBlock *block) {
    TransactionPool *pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
    // 1) Proof-of-Work
    if (!verify_nonce(block)) {
        log_message("[Validator] Bloco %s falhou PoW\n", block->txb_id);
        return false;
    }

    // 2) Verifica se o bloco aponta para o hash que está em `previous_hash`
    pthread_mutex_lock(&ledger_lock);
    if (strcmp(block->previous_block_hash, previous_hash) != 0) {
        log_message("[Validator] Bloco %s tem previous_hash errado\n", block->txb_id);
        pthread_mutex_unlock(&ledger_lock);
        return false;
    }

    // Se passou, atualiza `previous_hash` para o hash deste bloco
    compute_sha256(block, previous_hash);
    pthread_mutex_unlock(&ledger_lock);

    // 3) Verifica todas as transações estão na pool, marca-as como empty e dá sem_post(empty)
    for (int t = 0; t < cfg.TRANSACTIONS_PER_BLOCK; t++) {
        bool ok = false;
        for (int i = 0; i < pool->size; i++) {
            if (!pool->transactions_pending_set[i].empty
             && strcmp(pool->transactions_pending_set[i].tx.TX_ID,
                       block->transactions[t].TX_ID) == 0)
            {
                ok = true;
                pool->transactions_pending_set[i].empty = 1;
                sem_post(empty);    // liberta espaço na pool
                sem_wait(full);     // decrementa full
                break;
            }
        }
        if (!ok) {
            log_message("[Validator] Transação %s não encontrada\n",
                        block->transactions[t].TX_ID);
            return false;
        }
    }

    return true;
}

void print_block_info(TransactionBlock *block) {
  // Print basic block info
  printf("Block ID: %s\n", block->txb_id);
  printf("Previous Hash:\n%s\n", block->previous_block_hash);
  printf("Block Timestamp: %ld\n", block->timestamp);
  printf("Nonce: %u\n", block->nonce);
  printf("Transactions:\n");

  for (int i = 0; i < cfg.TRANSACTIONS_PER_BLOCK; ++i) {
    Transaction tx = block->transactions[i];
    printf("  [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld\n", i,
           tx.TX_ID, tx.reward, tx.value, tx.timestamp);
  }
}

void run_validator() {
    TransactionPool *pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
    int fd = open(VALIDATOR_FIFO, O_RDONLY);
    if (fd < 0) { perror("open FIFO"); exit(1); }

    size_t bytes = sizeof(TransactionBlock)
                 + cfg.TRANSACTIONS_PER_BLOCK * sizeof(Transaction);
    TransactionBlock *block = malloc(bytes);
    if (!block) { perror("malloc"); exit(1); }

    while (1) {
        ssize_t r = read(fd, block, bytes);
        if (r == 0) break; 
        if (r < 0) { perror("read bloco"); break; }
        if ((size_t)r != bytes) {
            fprintf(stderr, "Lido só %zd/%zu bytes\n", r, bytes);
            continue;
        }

        print_block_info(block);
        if (validate_block(block) == true){
            printf("\n\n\n\n--------------------conseguimos validar--------------------\n\n\n\n");
        }
        sleep(1);
        
    }

    free(block);
    close(fd);
}