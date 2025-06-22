// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "log.h"

Config read_config(const char *filename) {
    Config cfg;
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror("Erro ao abrir config.cfg");
        exit(1);
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char key[64];
        int value;

        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "NUM_MINERS") == 0)
                cfg.NUM_MINERS = value;
            else if (strcmp(key, "TX_POOL_SIZE") == 0)
                cfg.TX_POOL_SIZE = value;
            else if (strcmp(key, "TRANSACTIONS_PER_BLOCK") == 0)
                cfg.TRANSACTIONS_PER_BLOCK = value;
            else if (strcmp(key, "BLOCKCHAIN_BLOCKS") == 0)
                cfg.BLOCKCHAIN_BLOCKS = value;
        }
    }

    if (cfg.NUM_MINERS <= 0 || cfg.NUM_MINERS > 100) {
        log_message("Erro: NUM_MINERS inválido: %d\n", cfg.NUM_MINERS);
        exit(1);
    }

    if (cfg.TX_POOL_SIZE <= 0) {
        log_message("Erro: TX_POOL_SIZE inválido: %d\n", cfg.TX_POOL_SIZE);
        exit(1);
    }

    if (cfg.TRANSACTIONS_PER_BLOCK <= 0) {
        log_message("Erro: TRANSACTIONS_PER_BLOCK inválido: %d\n", cfg.TRANSACTIONS_PER_BLOCK);
        exit(1);
    }

    if (cfg.BLOCKCHAIN_BLOCKS <= 0) {
        log_message("Erro: BLOCKCHAIN_BLOCKS inválido: %d\n", cfg.BLOCKCHAIN_BLOCKS);
        exit(1);
    }

    log_message("[Controller] Configuração lida: NUM_MINERS=%d, TX_POOL_SIZE=%d, "
        "TRANSACTIONS_PER_BLOCK=%d, BLOCKCHAIN_BLOCKS=%d\n",
        cfg.NUM_MINERS, cfg.TX_POOL_SIZE, cfg.TRANSACTIONS_PER_BLOCK, cfg.BLOCKCHAIN_BLOCKS);
        
    fclose(file);
    return cfg;
}
