#include "ipc_utils.h"
#include "log.h"
#include "pow.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#define VALIDATOR_FIFO "/tmp/VALIDATOR_INPUT"

bool validate_block(TransactionBlock *block, Ledger *ledger) {
  TransactionPool *pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
  // 1) Proof-of-Work
  if (!verify_nonce(block)) {
    log_message("[Validator] Bloco %s falhou PoW\n", block->txb_id);
    return false;
  }

  // 2) Verifica se o bloco aponta para o hash que está em `previous_hash`

  sem_wait(ledger_sem);
  if (strcmp(block->previous_block_hash, ledger->previous_hash) != 0) {
    log_message("[Validator] Bloco %s tem previous_hash errado\n",
                block->txb_id);
    sem_post(ledger_sem);
    return false;
  }

  // Se passou, atualiza `previous_hash` para o hash deste bloco
  compute_sha256(block, ledger->previous_hash);

  // 3) Verifica todas as transações estão na pool, marca-as como empty e dá
  // sem_post(empty)
  sem_wait(txpool_sem);
  for (int t = 0; t < cfg.TRANSACTIONS_PER_BLOCK; t++) {
    bool ok = false;
    for (int i = 0; i < pool->size; i++) {
      if (!pool->transactions_pending_set[i].empty &&
          strcmp(pool->transactions_pending_set[i].tx.TX_ID,
                 block->transactions[t].TX_ID) == 0) {
        ok = true;
        pool->transactions_pending_set[i].empty = 1;
        sem_post(empty); // liberta espaço na pool
        sem_wait(full);  // decrementa full
        break;
      }
    }
    if (!ok) {
      sem_post(txpool_sem);
      log_message("[Validator] Transação %s não encontrada\n",
                  block->transactions[t].TX_ID);
      return false;
    }
  }
  sem_post(txpool_sem);
  sem_post(ledger_sem);

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
  // 1) Acede à pool de transações
  TransactionPool *pool = (TransactionPool *)shmat(shm_pool_id, NULL, 0);
  if (pool == (void *)-1) {
    perror("shmat TxPool (validator)");
    exit(1);
  }

  // 2) Acede à ledger em shared memory
  Ledger *ledger = (Ledger *)shmat(shm_ledger_id, NULL, 0);
  if (ledger == (void *)-1) {
    perror("shmat Ledger (validator)");
    exit(1);
  }

  // 3) Abre o pipe em O_RDWR para evitar EOF quando o escritor fechar
  int fd = open(VALIDATOR_FIFO, O_RDWR);
  if (fd < 0) {
    perror("open FIFO");
    exit(1);
  }

  // 4) Calcula o tamanho em bytes de cada bloco
  size_t block_bytes = sizeof(TransactionBlock) +
                       cfg.TRANSACTIONS_PER_BLOCK * sizeof(Transaction);

  TransactionBlock *blk = malloc(block_bytes);
  if (!blk) {
    perror("malloc blk");
    close(fd);
    exit(1);
  }

  // 5) Loop infinito: lê cada bloco do pipe e processa
  for (;;) {
    ssize_t r = read(fd, blk, block_bytes);
    if (r < 0) {
      perror("read bloco");
      continue;
    }
    if (r == 0) {
      // sem break aqui, apenas continua a ler
      continue;
    }
    if ((size_t)r != block_bytes) {
      fprintf(stderr, "Lido só %zd/%zu bytes\n", r, block_bytes);
      continue;
    }

    // imprime para debug
    print_block_info(blk);

    // valida e, se OK, insere na ledger
    if (validate_block(blk, ledger)) {
      if (ledger->current_block < ledger->size) {
        TransactionBlock *dst = &ledger->blocks[ledger->current_block];
        memcpy(dst, blk, block_bytes);
        log_message("[Validator] Inserido bloco %s na ledger (idx=%d)\n",
                    blk->txb_id, ledger->current_block);
        ledger->current_block++;
        key_t key = ftok(BLOCK_MSG_QUEUE_FILE, BLOCK_MSG_QUEUE_ID);
        int msqid = msgget(key, IPC_CREAT | 0666);
        if (msqid < 0) {
          perror("validator msgget");
        } else {
          BlockMsg msg;
          msg.mtype = 1;
          msg.block = *blk; // copy the block struct
          if (msgsnd(msqid, &msg, sizeof(msg.block), 0) < 0) {
            perror("validator msgsnd");
          } else {
            log_message("[Validator] Sent block %s to stats\n", blk->txb_id);
          }
        }
      } else {
        log_message("[Validator] Ledger cheia, ignorando bloco %s\n",
                    blk->txb_id);
      }
    }

    // opcional: pequeno atraso para não busy-wait
    sleep(1);
  }

  // nunca chega aqui, mas para completar:
  free(blk);
  close(fd);
  shmdt(pool);
  shmdt(ledger);
}
