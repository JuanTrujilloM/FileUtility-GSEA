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
<<<<<<< HEAD
=======

.PHONY: test
test: all
	@echo "Running automated round-trip tests for compression/encryption..."
	@set -e; \
	TARGET=./bin/FileUtility; \
	# RLE
	echo "\n[TEST] RLE"; \
	./bin/FileUtility -c -i tests/Test1C.txt -o tests/TestCompRLE.dat --comp-alg RLE; \
	./bin/FileUtility -d -i tests/TestCompRLE.dat -o tests/TestDecompRLE.txt --comp-alg RLE; \
	(diff -q tests/Test1C.txt tests/TestDecompRLE.txt >/dev/null && echo "RLE: OK" || (echo "RLE: FAIL"; exit 1)); \
	# LZW
	echo "\n[TEST] LZW"; \
	./bin/FileUtility -c -i tests/Test1C.txt -o tests/TestCompLZW.dat --comp-alg LZW; \
	./bin/FileUtility -d -i tests/TestCompLZW.dat -o tests/TestDecompLZW.txt --comp-alg LZW; \
	(diff -q tests/Test1C.txt tests/TestDecompLZW.txt >/dev/null && echo "LZW: OK" || (echo "LZW: FAIL"; exit 1)); \
	# Vigenere
	echo "\n[TEST] Vigenere"; \
	./bin/FileUtility -e -i tests/Test1E.txt -o tests/TestEncVig.dat --enc-alg VIG -k MiClave; \
	./bin/FileUtility -u -i tests/TestEncVig.dat -o tests/TestDecVig.txt --enc-alg VIG -k MiClave; \
	(diff -q tests/Test1E.txt tests/TestDecVig.txt >/dev/null && echo "Vigenere: OK" || (echo "Vigenere: FAIL"; exit 1)); \
	# AES-128 CBC
	echo "\n[TEST] AES-128 (CBC)"; \
	./bin/FileUtility -e -i tests/Test1E.txt -o tests/TestEncAES.dat --enc-alg AES -k mysecretkey12345; \
	./bin/FileUtility -u -i tests/TestEncAES.dat -o tests/TestDecAES.txt --enc-alg AES -k mysecretkey12345; \
	(diff -q tests/Test1E.txt tests/TestDecAES.txt >/dev/null && echo "AES-128: OK" || (echo "AES-128: FAIL"; exit 1)); \
	echo "\nAll tests passed.";
>>>>>>> 88093fb (implementacion del algoritmo AES)
