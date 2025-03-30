#ifndef CONFIG_H
#define CONFIG_H

// Estrutura que guarda os parâmetros do ficheiro config.cfg
typedef struct {
    int NUM_MINERS;
    int TRANSACTION_POOL_SIZE;
    int BLOCK_SIZE;
    int MAX_TRANSACTIONS;
    int REWARD;
    int MAX_TTGEN;
    int TTGEN_SLEEP;
} Config;

// Função que lê o ficheiro de configuração e retorna a struct preenchida
Config read_config(const char *filename);

#endif // CONFIG_H