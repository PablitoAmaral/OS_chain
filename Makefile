# Diretórios
SRC_DIR = src
BIN = controller

# Flags de compilação
CFLAGS = -Wall -Wextra -g

# Ficheiros-fonte
SRCS = $(SRC_DIR)/controller.c \
       $(SRC_DIR)/config.c \
       $(SRC_DIR)/ipc_utils.c

# Ficheiros objeto
OBJS = $(SRCS:.c=.o)

# Regra principal
all: $(BIN)

# Compilação do binário
$(BIN): $(OBJS)
	@echo "🔧 A compilar..."
	gcc $(CFLAGS) -o $@ $(OBJS)

# Regra genérica para compilar .c em .o
%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	@echo "🧹 A limpar ficheiros..."
	rm -f $(BIN) $(OBJS)

# Execução
run: all
	@echo "🚀 A executar o programa..."
	./$(BIN)
