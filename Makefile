# Pablo Amaral 2021242404
# Ricardo Paredes 2021221592
#Diretórios
SRC_DIR = src
BIN = controller
MINER_BIN = miner
VALIDATOR_BIN = validator
STATS_BIN = statistics
TXGEN_BIN = TxGen

# Flags de compilação
CFLAGS = -Wall -Wextra -g

# Ficheiros-fonte
SRCS = $(SRC_DIR)/controller.c \
       $(SRC_DIR)/config.c \
       $(SRC_DIR)/ipc_utils.c

MINER_SRC = $(SRC_DIR)/miner.c
VALIDATOR_SRC = $(SRC_DIR)/validator.c
STATS_SRC = $(SRC_DIR)/statistics.c
TXGEN_SRC = $(SRC_DIR)/TxGen.c

# Ficheiros objeto
OBJS = $(SRCS:.c=.o)

# Regra principal
all: $(BIN) $(MINER_BIN) $(VALIDATOR_BIN) $(STATS_BIN) $(TXGEN_BIN)

# Compilação do controller
$(BIN): $(OBJS)
	@echo "A compilar controller..."
	gcc $(CFLAGS) -o $@ $(OBJS)

# Compilação do miner
$(MINER_BIN): $(MINER_SRC)
	@echo "A compilar miner..."
	gcc $(CFLAGS) -o $@ $<

# Compilação do validator
$(VALIDATOR_BIN): $(VALIDATOR_SRC)
	@echo "A compilar validator..."
	gcc $(CFLAGS) -o $@ $<

# Compilação do statistics
$(STATS_BIN): $(STATS_SRC)
	@echo "A compilar statistics..."
	gcc $(CFLAGS) -o $@ $<

# Compilação do TxGen
$(TXGEN_BIN): $(TXGEN_SRC)
	@echo "A compilar TxGen..."
	gcc $(CFLAGS) -o $@ $<

# Regra genérica para compilar .c em .o
%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	@echo "A limpar ficheiros..."
	rm -f $(BIN) $(MINER_BIN) $(VALIDATOR_BIN) $(STATS_BIN) $(TXGEN_BIN) $(OBJS)

# Execução
run: all
	@echo "A executar o programa..."
	./$(BIN)
