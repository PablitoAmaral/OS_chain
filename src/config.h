// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef CONFIG_H
#define CONFIG_H

// 
typedef struct {
    int NUM_MINERS;
    int TX_POOL_SIZE;
    int TRANSACTIONS_PER_BLOCK;
    int BLOCKCHAIN_BLOCKS;
} Config;

Config read_config(const char *filename);

#endif // CONFIG_H
