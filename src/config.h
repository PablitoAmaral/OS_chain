// Pablo Amaral 2021242404
// Ricardo Paredes 2021221592
#ifndef CONFIG_H
#define CONFIG_H

// 
typedef struct {
    int NUM_MINERS;
    int POOL_SIZE;
    int TRANSACTIONS_PER_BLOCK;
    int BLOCKCHAIN_BLOCKS;
    int TRANSACTION_POOL_SIZE;
} Config;

Config read_config(const char *filename);

#endif // CONFIG_H