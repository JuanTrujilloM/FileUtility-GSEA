#include "compression.h"
#include "fileManager.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <queue>
#include <memory>
#include <array>
#include <cstring>
#include <algorithm>
#include <functional>

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
// Formato: secuencia de códigos de 16 bits (2 bytes cada uno)
void compressLZW(const std::string &inputPath, const std::string &outputPath) {

    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Logica de compresión LZW
    std::unordered_map<std::string, int> dictionary;
    for (int i = 0; i < 256; i++) {
        dictionary[std::string(1, i)] = i;    
    }

    std::string w;
    char c;
    int nextCode = 256;
    std::vector<int> outputCodes;

    // Construir códigos
    while (readFile(inputFd, &c, 1) == 1) {
        std::string wc = w + c;
        if (dictionary.count(wc)) {
            w = wc;
        } else {
            // Emitir código de w (w siempre debe existir en el diccionario cuando no es vacío)
            if (!w.empty()) {
                outputCodes.push_back(dictionary[w]);
            }
            // Añadir nueva entrada si no se alcanzó el límite de 16 bits
            if (nextCode <= 0xFFFF) {
                dictionary[wc] = nextCode++;
            }
            w = std::string(1, c);
        }
    }
    if (!w.empty()) {
        outputCodes.push_back(dictionary[w]);
    }

    // Guardar códigos como binarios (2 bytes por código) en endianness nativo
    for (int code : outputCodes) {
        if (code < 0 || code > 0xFFFF) continue; // seguridad
        uint16_t value = static_cast<uint16_t>(code);
        ssize_t written = writeFile(outputFd, &value, sizeof(value));
        (void)written; // opcional: podríamos manejar errores aquí
    }

    close(inputFd);
    close(outputFd);
    
}

// Descompresión LZW
// Formato esperado: secuencia de códigos de 16 bits (2 bytes cada uno)
void decompressLZW(const std::string &inputPath, const std::string &outputPath) {
    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Inicializar diccionario con 0..255
    std::vector<std::string> dictionary;
    dictionary.reserve(65536);
    for (int i = 0; i < 256; ++i) {
        dictionary.push_back(std::string(1, static_cast<char>(i)));
    }
    int nextCode = 256;

    // Leer el primer código 
    uint16_t code;
    ssize_t r = readFile(inputFd, &code, sizeof(code));
    if (r != sizeof(code)) {
        // Archivo vacio o lectura fallida
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }
    if (code > 0xFFFF) {
        // código inválido
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    std::string w;
    if (code < dictionary.size()) {
        w = dictionary[code];
        // escribir w al archivo de salida
        if (!w.empty()) writeFile(outputFd, w.data(), w.size());
    } else {
        // código desconocido en inicio -> nada que hacer
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Procesar códigos restantes
    uint16_t k;
    while (readFile(inputFd, &k, sizeof(k)) == sizeof(k)) {
        std::string entry;

        if (k < dictionary.size()) {
            entry = dictionary[k];
        } else if (k == nextCode) {
            // Caso especial: entry = w + first char of w
            if (!w.empty()) entry = w + w[0];
            else entry = std::string();
        } else {
            // Código inválido: abortar lectura
            break;
        }

        // Escribir entry
        if (!entry.empty()) writeFile(outputFd, entry.data(), entry.size());

        // Agregar nueva entrada al diccionario: w + entry[0]
        if (nextCode <= 0xFFFF && !entry.empty()) {
            std::string newEntry = w + entry[0];
            dictionary.push_back(newEntry);
            ++nextCode;
        }

        // Avanzar
        w = entry;
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Compresión Huffman
void compressHuffman(const std::string &inputPath, const std::string &outputPath) {
    // Implementación pendiente
}

void decompressHuffman(const std::string &inputPath, const std::string &outputPath) {
    // Implementación pendiente
}