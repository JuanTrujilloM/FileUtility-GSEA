# Definición del compilador
CXX = g++

# Directorios
SRC_DIR = src
BIN_DIR = bin
TEST_DIR = tests
INCLUDE_DIR = include

# Nombre del ejecutable
TARGET = $(BIN_DIR)/FileUtility

# Regla predeterminada (compilar todo)
all: $(TARGET)

# Regla para crear el ejecutable directamente desde los .cpp
$(TARGET):
	@mkdir -p $(BIN_DIR)
	$(CXX) -I$(INCLUDE_DIR) $(SRC_DIR)/*.cpp -o $(TARGET)

# Limpiar los archivos generados
clean:
	rm -f $(BIN_DIR)/*

# Limpiar y recompilar
rebuild: clean all
	./bin/FileUtility -c -i tests_copy/ -o testsOut/ --comp-alg RLE