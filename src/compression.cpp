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

    // Leer todo el archivo
    std::vector<unsigned char> data;
    const size_t BUF_SZ = 4096;
    std::vector<char> buf(BUF_SZ);
    ssize_t r;
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

    // frecuencia
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
        // no hay datos (ya manejado), pero por seguridad
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

    // Escribir cabecera: tamaño original (uint64_t), número de símbolos (uint16_t),
    // luego para cada símbolo: uint8_t ch + uint64_t freq
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

    // Escribir el bitstream codificado (empaquetado en bytes)
    unsigned char outByte = 0;
    int bitCount = 0;
    for (unsigned char c : data) {
        const std::string &code = codes[c];
        for (char bit : code) {
            outByte <<= 1;
            if (bit == '1') outByte |= 1;
            ++bitCount;
            if (bitCount == 8) {
                writeFile(outputFd, &outByte, 1);
                outByte = 0;
                bitCount = 0;
            }
        }
    }
    // padding: rellenar con ceros a la derecha en el último byte si es necesario
    if (bitCount > 0) {
        outByte <<= (8 - bitCount);
        writeFile(outputFd, &outByte, 1);
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
        // archivo vacío -> nada que escribir
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

    // Decodificar bit a bit
    HuffNode* node = root;
    uint64_t written = 0;
    const size_t BUF_SZ = 4096;
    std::vector<unsigned char> inbuf(BUF_SZ);
    ssize_t rr;
    // leer el resto del archivo por bloques
    while ((rr = readFile(inputFd, reinterpret_cast<char*>(inbuf.data()), BUF_SZ)) > 0 && written < origSize) {
        for (ssize_t i = 0; i < rr && written < origSize; ++i) {
            unsigned char b = inbuf[i];
            // procesar 8 bits, de msb a lsb (coincide con cómo escribimos)
            for (int bit = 7; bit >= 0 && written < origSize; --bit) {
                int v = (b >> bit) & 1;
                node = (v == 0) ? node->left : node->right;
                if (!node->left && !node->right) {
                    unsigned char outc = node->ch;
                    writeFile(outputFd, &outc, 1);
                    ++written;
                    node = root;
                }
            }
        }
    }

    freeTree(root);
    closeFile(inputFd);
    closeFile(outputFd);
}