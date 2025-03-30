#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

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
            if (strcmp(key, "NUM_MINERS") == 0) cfg.NUM_MINERS = value;
            else if (strcmp(key, "TRANSACTION_POOL_SIZE") == 0) cfg.TRANSACTION_POOL_SIZE = value;
            else if (strcmp(key, "BLOCK_SIZE") == 0) cfg.BLOCK_SIZE = value;
            else if (strcmp(key, "MAX_TRANSACTIONS") == 0) cfg.MAX_TRANSACTIONS = value;
            else if (strcmp(key, "REWARD") == 0) cfg.REWARD = value;
            else if (strcmp(key, "MAX_TTGEN") == 0) cfg.MAX_TTGEN = value;
            else if (strcmp(key, "TTGEN_SLEEP") == 0) cfg.TTGEN_SLEEP = value;
        }
    }

    fclose(file);
    return cfg;
}