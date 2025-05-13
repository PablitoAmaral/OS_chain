# Pablo Amaral 2021242404
# Ricardo Paredes 2021221592
SRC_DIR = src
BIN = controller
TXGEN_BIN = TxGen
CFLAGS = -Wall -Wextra -g

SRCS = $(SRC_DIR)/controller.c \
       $(SRC_DIR)/config.c \
       $(SRC_DIR)/ipc_utils.c \
       $(SRC_DIR)/miner.c \
       $(SRC_DIR)/validator.c \
       $(SRC_DIR)/statistics.c

OBJS = $(SRCS:.c=.o)

all: $(BIN) $(TXGEN_BIN)

$(BIN): $(OBJS)
	@echo "A compilar controller..."
	gcc $(CFLAGS) -o $@ $(OBJS)

$(TXGEN_BIN): $(SRC_DIR)/TxGen.c
	@echo "A compilar TxGen..."
	gcc $(CFLAGS) -o $@ $<

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	@echo "A limpar ficheiros..."
	rm -f $(BIN) $(TXGEN_BIN) $(OBJS)

run: all
	@echo "A executar o programa..."
	./$(BIN)
