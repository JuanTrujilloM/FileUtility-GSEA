#include "compression.h"
#include "fileManager.h"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <queue>
#include <array>

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

    // Buffers optimizados para I/O por bloques
    constexpr size_t INPUT_BUF_SIZE = 4096;  // 4KB buffer de entrada
    constexpr size_t OUTPUT_BUF_SIZE = 4096; // 4KB buffer de salida
    std::vector<char> inputBuffer(INPUT_BUF_SIZE);
    std::vector<char> outputBuffer;
    outputBuffer.reserve(OUTPUT_BUF_SIZE);

    char previousChar = '\0';
    int count = 0;
    bool first = true;
    ssize_t bytesRead;

    // Procesar archivo por bloques
    while ((bytesRead = readFile(inputFd, inputBuffer.data(), INPUT_BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            char currentChar = inputBuffer[i];
            
            if (!first && currentChar == previousChar) {
                count++;
            } else {
                if (!first) {
                    // Escribir count (4 bytes) + carácter (1 byte) al buffer
                    outputBuffer.insert(outputBuffer.end(), 
                                      reinterpret_cast<char*>(&count),
                                      reinterpret_cast<char*>(&count) + sizeof(int));
                    outputBuffer.push_back(previousChar);
                    
                    // Flush buffer si está cerca del límite
                    if (outputBuffer.size() >= OUTPUT_BUF_SIZE - 5) {
                        writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
                        outputBuffer.clear();
                    }
                }
                previousChar = currentChar;
                count = 1;
                first = false;
            }
        }
    }

    // Escribir el último par count+char
    if (!first) {
        outputBuffer.insert(outputBuffer.end(), 
                          reinterpret_cast<char*>(&count),
                          reinterpret_cast<char*>(&count) + sizeof(int));
        outputBuffer.push_back(previousChar);
    }

    // Flush buffer final
    if (!outputBuffer.empty()) {
        writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
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

    // Buffer optimizado para escritura
    constexpr size_t OUTPUT_BUF_SIZE = 4096; // 4KB buffer
    std::vector<char> outputBuffer;
    outputBuffer.reserve(OUTPUT_BUF_SIZE);

    int count;
    char ch;
    
    // Leer pares de [count][char] hasta el final del archivo
    while (readFile(inputFd, &count, sizeof(int)) == sizeof(int)) {
        if (readFile(inputFd, &ch, 1) == 1) {
            // Escribir el carácter 'count' veces al buffer
            for (int j = 0; j < count; j++) {
                outputBuffer.push_back(ch);
                
                // Flush buffer si está lleno
                if (outputBuffer.size() >= OUTPUT_BUF_SIZE) {
                    writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
                    outputBuffer.clear();
                }
            }
        }
    }

    // Flush buffer final
    if (!outputBuffer.empty()) {
        writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Compress usando Lempel-Ziv-Welch (LZW)
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

    // Diccionario optimizado con reserva de capacidad
    std::unordered_map<std::string, int> dictionary;
    dictionary.reserve(4096); // Pre-reservar espacio
    for (int i = 0; i < 256; i++) {
        dictionary[std::string(1, static_cast<char>(i))] = i;    
    }

    std::string w;
    w.reserve(256); // Pre-reservar para reducir reasignaciones
    int nextCode = 256;
    std::vector<uint16_t> outputCodes;
    outputCodes.reserve(8192); // Pre-reservar espacio

    // Buffer de lectura optimizado
    constexpr size_t INPUT_BUF_SIZE = 4096; // 4KB
    std::vector<char> inputBuffer(INPUT_BUF_SIZE);
    ssize_t bytesRead;

    // Procesar archivo por bloques
    while ((bytesRead = readFile(inputFd, inputBuffer.data(), INPUT_BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            char c = inputBuffer[i];
            std::string wc = w + c;
            
            if (dictionary.count(wc)) {
                w = std::move(wc);
            } else {
                // Emitir código de w
                if (!w.empty()) {
                    outputCodes.push_back(static_cast<uint16_t>(dictionary[w]));
                }
                // Añadir nueva entrada si no se alcanzó el límite de 16 bits
                if (nextCode <= 0xFFFF) {
                    dictionary[wc] = nextCode++;
                }
                w = std::string(1, c);
            }
        }
    }
    
    // Emitir último código
    if (!w.empty()) {
        outputCodes.push_back(static_cast<uint16_t>(dictionary[w]));
    }

    // Escribir códigos en bloques para mejor rendimiento
    if (!outputCodes.empty()) {
        writeFile(outputFd, outputCodes.data(), outputCodes.size() * sizeof(uint16_t));
    }

    close(inputFd);
    close(outputFd);
}

// Descompress usando Lempel-Ziv-Welch LZW
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

    // Inicializar diccionario con 0..255 y pre-reservar
    std::vector<std::string> dictionary;
    dictionary.reserve(65536);
    for (int i = 0; i < 256; ++i) {
        dictionary.push_back(std::string(1, static_cast<char>(i)));
    }
    int nextCode = 256;

    // Buffer de salida optimizado
    constexpr size_t OUTPUT_BUF_SIZE = 4096; // 4KB
    std::vector<char> outputBuffer;
    outputBuffer.reserve(OUTPUT_BUF_SIZE);

    // Leer códigos en bloques para mejor rendimiento
    constexpr size_t CODE_BUF_SIZE = 8192; // Leer 8K códigos a la vez
    std::vector<uint16_t> codeBuffer(CODE_BUF_SIZE);
    
    // Leer el primer código
    if (readFile(inputFd, &codeBuffer[0], sizeof(uint16_t)) != sizeof(uint16_t)) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    uint16_t code = codeBuffer[0];
    if (code >= dictionary.size()) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    std::string w = dictionary[code];
    // Escribir w al buffer
    outputBuffer.insert(outputBuffer.end(), w.begin(), w.end());

    // Procesar códigos restantes por bloques
    ssize_t bytesRead;
    while ((bytesRead = readFile(inputFd, codeBuffer.data(), CODE_BUF_SIZE * sizeof(uint16_t))) > 0) {
        size_t codesRead = bytesRead / sizeof(uint16_t);
        
        for (size_t i = 0; i < codesRead; ++i) {
            uint16_t k = codeBuffer[i];
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

            // Escribir entry al buffer
            outputBuffer.insert(outputBuffer.end(), entry.begin(), entry.end());
            
            // Flush si el buffer está cerca del límite
            if (outputBuffer.size() >= OUTPUT_BUF_SIZE - 256) {
                writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
                outputBuffer.clear();
            }

            // Agregar nueva entrada al diccionario
            if (nextCode <= 0xFFFF && !entry.empty()) {
                dictionary.push_back(w + entry[0]);
                ++nextCode;
            }

            w = std::move(entry);
        }
    }

    // Flush buffer final
    if (!outputBuffer.empty()) {
        writeFile(outputFd, outputBuffer.data(), outputBuffer.size());
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Compress usando Huffman
// Formato:[Header][Payload Comprimido]

// Nodo del árbol Huffman
struct HuffNode {
    uint64_t freq;
    unsigned char ch;
    HuffNode* left;
    HuffNode* right;
    HuffNode(uint64_t f, unsigned char c) : freq(f), ch(c), left(nullptr), right(nullptr) {}
    HuffNode(uint64_t f, HuffNode* l, HuffNode* r) : freq(f), ch(0), left(l), right(r) {}
};

// Comparador para la cola de prioridad (min-heap)
struct NodeCmp {
    bool operator()(HuffNode* a, HuffNode* b) const {
        return a->freq > b->freq;
    }
};

// Liberar memoria del árbol Huffman
static void freeTree(HuffNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

// Construir códigos recursivamente
static void buildCodes(HuffNode* node, const std::string &prefix, std::array<std::string,256> &codes) {
    if (!node) return;
    if (!node->left && !node->right) {
        // hoja
        codes[node->ch] = (prefix.empty() ? "0" : prefix); // caso único símbolo
        return;
    }
    if (node->left) buildCodes(node->left, prefix + '0', codes);
    if (node->right) buildCodes(node->right, prefix + '1', codes);
}

void compressHuffman(const std::string &inputPath, const std::string &outputPath) {
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;
    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) { closeFile(inputFd); return; }

    // Leer archivo con buffer optimizado
    std::vector<unsigned char> data;
    constexpr size_t BUF_SZ = 4096; // 4KB buffer
    std::vector<char> buf(BUF_SZ);
    ssize_t r;
    
    // Pre-estimar tamaño para reducir reasignaciones
    off_t fileSize = lseek(inputFd, 0, SEEK_END);
    if (fileSize > 0) {
        lseek(inputFd, 0, SEEK_SET);
        data.reserve(fileSize);
    }
    
    while ((r = readFile(inputFd, buf.data(), BUF_SZ)) > 0) {
        data.insert(data.end(), buf.data(), buf.data() + r);
    }

    uint64_t origSize = data.size();
    if (origSize == 0) {
        // escribir cabecera vacía: tamaño 0 y sin símbolos
        uint64_t z = 0;
        uint16_t zerosyms = 0;
        writeFile(outputFd, &z, sizeof(z));
        writeFile(outputFd, &zerosyms, sizeof(zerosyms));
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Calcular frecuencias
    std::array<uint64_t,256> freq{};
    for (unsigned char c : data) freq[c]++;

    // construir cola de prioridad
    std::priority_queue<HuffNode*, std::vector<HuffNode*>, NodeCmp> pq;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            pq.push(new HuffNode(freq[i], static_cast<unsigned char>(i)));
        }
    }

    // construir árbol
    if (pq.empty()) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }
    while (pq.size() > 1) {
        HuffNode* a = pq.top(); pq.pop();
        HuffNode* b = pq.top(); pq.pop();
        HuffNode* parent = new HuffNode(a->freq + b->freq, a, b);
        pq.push(parent);
    }
    HuffNode* root = pq.top();

    // generar códigos
    std::array<std::string,256> codes;
    for (auto &s : codes) s.clear();
    buildCodes(root, "", codes);

    // Escribir cabecera
    uint64_t origSizeLE = origSize;
    uint16_t uniqueSymbols = 0;
    for (int i = 0; i < 256; ++i) if (freq[i] > 0) ++uniqueSymbols;

    writeFile(outputFd, &origSizeLE, sizeof(origSizeLE));
    writeFile(outputFd, &uniqueSymbols, sizeof(uniqueSymbols));
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            uint8_t ch = static_cast<uint8_t>(i);
            uint64_t f = freq[i];
            writeFile(outputFd, &ch, sizeof(ch));
            writeFile(outputFd, &f, sizeof(f));
        }
    }

    // Optimización: Escribir bitstream usando buffer
    std::vector<unsigned char> bitBuffer;
    bitBuffer.reserve(data.size() / 2); // Estimación conservadora
    
    unsigned char outByte = 0;
    int bitCount = 0;
    
    for (unsigned char c : data) {
        const std::string &code = codes[c];
        for (char bit : code) {
            outByte = (outByte << 1) | (bit == '1' ? 1 : 0);
            ++bitCount;
            if (bitCount == 8) {
                bitBuffer.push_back(outByte);
                outByte = 0;
                bitCount = 0;
                
                // Flush buffer periódicamente
                if (bitBuffer.size() >= BUF_SZ) {
                    writeFile(outputFd, bitBuffer.data(), bitBuffer.size());
                    bitBuffer.clear();
                }
            }
        }
    }
    
    // padding: rellenar con ceros a la derecha en el último byte si es necesario
    if (bitCount > 0) {
        outByte <<= (8 - bitCount);
        bitBuffer.push_back(outByte);
    }
    
    // Flush final
    if (!bitBuffer.empty()) {
        writeFile(outputFd, bitBuffer.data(), bitBuffer.size());
    }

    freeTree(root);
    closeFile(inputFd);
    closeFile(outputFd);
}

// Decompress usando Huffman
// Formato esperado: [Header][Payload Descomprimido]
void decompressHuffman(const std::string &inputPath, const std::string &outputPath) {
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;
    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) { closeFile(inputFd); return; }

    // leer cabecera
    uint64_t origSize = 0;
    if (readFile(inputFd, &origSize, sizeof(origSize)) != (ssize_t)sizeof(origSize)) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }
    uint16_t uniqueSymbols = 0;
    if (readFile(inputFd, &uniqueSymbols, sizeof(uniqueSymbols)) != (ssize_t)sizeof(uniqueSymbols)) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }
    if (origSize == 0 || uniqueSymbols == 0) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // reconstruir tabla de frecuencias
    std::array<uint64_t,256> freq{};
    for (int i = 0; i < uniqueSymbols; ++i) {
        uint8_t ch;
        uint64_t f;
        if (readFile(inputFd, &ch, sizeof(ch)) != (ssize_t)sizeof(ch) ||
            readFile(inputFd, &f, sizeof(f)) != (ssize_t)sizeof(f)) {
            closeFile(inputFd);
            closeFile(outputFd);
            return;
        }
        freq[ch] = f;
    }

    // reconstruir árbol Huffman
    std::priority_queue<HuffNode*, std::vector<HuffNode*>, NodeCmp> pq;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) pq.push(new HuffNode(freq[i], static_cast<unsigned char>(i)));
    }
    if (pq.empty()) { closeFile(inputFd); closeFile(outputFd); return; }
    while (pq.size() > 1) {
        HuffNode* a = pq.top(); pq.pop();
        HuffNode* b = pq.top(); pq.pop();
        HuffNode* parent = new HuffNode(a->freq + b->freq, a, b);
        pq.push(parent);
    }
    HuffNode* root = pq.top();

    // Buffer optimizado para decodificación y escritura
    constexpr size_t INPUT_BUF_SZ = 4096;  // 4KB input buffer
    constexpr size_t OUTPUT_BUF_SZ = 4096; // 4KB output buffer
    std::vector<unsigned char> inbuf(INPUT_BUF_SZ);
    std::vector<unsigned char> outbuf;
    outbuf.reserve(OUTPUT_BUF_SZ);

    // Decodificar bit a bit con buffer
    HuffNode* node = root;
    uint64_t written = 0;
    ssize_t rr;
    
    while ((rr = readFile(inputFd, inbuf.data(), INPUT_BUF_SZ)) > 0 && written < origSize) {
        for (ssize_t i = 0; i < rr && written < origSize; ++i) {
            unsigned char b = inbuf[i];
            // procesar 8 bits, de msb a lsb
            for (int bit = 7; bit >= 0 && written < origSize; --bit) {
                int v = (b >> bit) & 1;
                node = (v == 0) ? node->left : node->right;
                
                if (!node->left && !node->right) {
                    // Nodo hoja encontrado
                    outbuf.push_back(node->ch);
                    ++written;
                    node = root;
                    
                    // Flush buffer cuando esté lleno
                    if (outbuf.size() >= OUTPUT_BUF_SZ) {
                        writeFile(outputFd, outbuf.data(), outbuf.size());
                        outbuf.clear();
                    }
                }
            }
        }
    }

    // Flush buffer final
    if (!outbuf.empty()) {
        writeFile(outputFd, outbuf.data(), outbuf.size());
    }

    freeTree(root);
    closeFile(inputFd);
    closeFile(outputFd);
}