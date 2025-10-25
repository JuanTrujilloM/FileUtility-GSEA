#include "compression.h"
#include "fileManager.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>

// Compress usando Run-Length Encoding (RLE)
// Formato: [count:4bytes][char:1byte] repetido
void compressRLE(const std::string &inputPath, const std::string &outputPath) {

    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Lógica de compresión RLE con formato binario
    char currentChar;
    char previousChar = '\0';
    int count = 0;
    bool first = true;

    while (readFile(inputFd, &currentChar, 1) == 1) {
        if (!first && currentChar == previousChar) {
            count++;
        } else {
            if (!first) {
                // Escribir count (4 bytes) + carácter (1 byte)
                writeFile(outputFd, &count, sizeof(int));
                writeFile(outputFd, &previousChar, 1);
            }
            previousChar = currentChar;
            count = 1;
            first = false;
        }
    }

    // Escribir el último par count+char
    if (!first) {
        writeFile(outputFd, &count, sizeof(int));
        writeFile(outputFd, &previousChar, 1);
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Decompress usando Run-Length Encoding (RLE)
// Formato esperado: [count:4bytes][char:1byte] repetido
void decompressRLE(const std::string &inputPath, const std::string &outputPath) {

    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Lógica de descompresión RLE con formato binario
    int count;
    char ch;
    
    // Leer pares de [count][char] hasta el final del archivo
    while (readFile(inputFd, &count, sizeof(int)) == sizeof(int)) {
        if (readFile(inputFd, &ch, 1) == 1) {
            // Escribir el carácter 'count' veces
            for (int j = 0; j < count; j++) {
                writeFile(outputFd, &ch, 1);
            }
        }
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Algoritmo Lempel-Ziv-Welch (LZW)

void compressLZW(const std::string &inputPath, const std::string &outputPath) {
    // Implementación pendiente
}

void decompressLZW(const std::string &inputPath, const std::string &outputPath) {
    // Implementación pendiente
}