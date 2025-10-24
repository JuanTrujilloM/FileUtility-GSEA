#include "compression.h"
#include "fileManager.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>

// Compress usando Run-Length Encoding (RLE)
void compressRLE(const std::string &inputPath, const std::string &outputPath) {

    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Lógica de compresión RLE
    char currentChar;
    char previousChar = '\0';
    int count = 0;

    while (readFile(inputFd, &currentChar, 1) == 1) {
        if (currentChar == previousChar) {
            count++;
        } else {
            if (count > 0) {
                std::string toWrite = std::to_string(count) + previousChar;
                writeFile(outputFd, toWrite.c_str(), toWrite.size());
            }
            previousChar = currentChar;
            count = 1;
        }
    }

    if (count > 0) {
        std::string toWrite = std::to_string(count) + previousChar;
        writeFile(outputFd, toWrite.c_str(), toWrite.size());
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Decompress usando Run-Length Encoding (RLE)
void decompressRLE(const std::string &inputPath, const std::string &outputPath) {

    // Abrir archivos de entrada y salida
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Lógica de descompresión RLE
    char buffer[1024];
    ssize_t bytesRead;
    std::string accumulated = "";

    while ((bytesRead = readFile(inputFd, buffer, sizeof(buffer))) > 0) {
        accumulated.append(buffer, bytesRead);
    }

    size_t i = 0;
    while (i < accumulated.size()) {

        int count = 0;

        while (i < accumulated.size() && isdigit(accumulated[i])) {
            count = count * 10 + (accumulated[i] - '0');
            i++;
        }
        
        if (i < accumulated.size()) {
            char ch = accumulated[i];
            i++;
            
            for (int j = 0; j < count; j++) {
                writeFile(outputFd, &ch, 1);
            }
        }
    }

    closeFile(inputFd);
    closeFile(outputFd);
}