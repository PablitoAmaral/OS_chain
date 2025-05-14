// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
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

    fclose(file);
    return cfg;
}
