# Caminho da pasta com os .c e .h
SRC_DIR = src
OBJ_DIR = obj
BIN = controller

# Ficheiros-fonte e objetos
SRCS = $(SRC_DIR)/controller.c $(SRC_DIR)/config.c
OBJS = $(SRCS:.c=.o)

# Flags de compilação
CFLAGS = -Wall -Wextra -g

# Regra principal
all: $(BIN)

# Regra de compilação
$(BIN): $(SRCS)
	@echo "🔧 A compilar..."
	gcc $(CFLAGS) -o $@ $(SRCS)

# Clean
clean:
	@echo "🧹 A limpar ficheiros..."
	rm -f $(BIN) $(SRC_DIR)/*.o

# Executar o programa
run: all
	@echo "🚀 A executar o programa..."
	./$(BIN)
