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
// Formato: [magic:4bytes='HUF1'][total:uint64_t][freqs:256 x uint64_t][payload:bits empaquetados en bytes]
void compressHuffman(const std::string &inputPath, const std::string &outputPath) {
    // Abrir archivos
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Obtener tamaño y leer todo el archivo en memoria
    long long size = getFileSize(inputPath);
    std::vector<uint8_t> data;
    if (size > 0) {
        data.resize(size);
        ssize_t r = readFile(inputFd, data.data(), data.size());
        if (r <= 0) {
            // nada que comprimir
            closeFile(inputFd);
            closeFile(outputFd);
            return;
        }
    } else {
        // archivo vacío: escribir header mínimo y salir
        char magic[4] = {'H','U','F','1'};
        writeFile(outputFd, magic, 4);
        uint64_t total = 0;
        writeFile(outputFd, &total, sizeof(total));
        uint64_t zero = 0;
        for (int i = 0; i < 256; ++i) writeFile(outputFd, &zero, sizeof(zero));
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Contar frecuencias
    std::array<uint64_t, 256> freq{};
    uint64_t total = 0;
    for (uint8_t b : data) {
        ++freq[b];
        ++total;
    }

    // Construir árbol Huffman
    struct Node {
        uint64_t freq;
        int byte; // -1 para nodos internos
        std::shared_ptr<Node> left, right;
        Node(uint64_t f, int b) : freq(f), byte(b), left(nullptr), right(nullptr) {}
        Node(uint64_t f, std::shared_ptr<Node> l, std::shared_ptr<Node> r) : freq(f), byte(-1), left(l), right(r) {}
    };

    // Comparador para priority_queue
    auto cmp = [](const std::shared_ptr<Node> &a, const std::shared_ptr<Node> &b) {
        return a->freq > b->freq;
    };

    // Construir la cola de prioridad
    std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, decltype(cmp)> pq(cmp);

    // Insertar nodos hoja
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) pq.push(std::make_shared<Node>(freq[i], i));
    }

    // Manejar caso especial: archivo vacío
    if (pq.empty()) {
        // no hay datos
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Construir el árbol combinando nodos
    while (pq.size() > 1) {
        auto a = pq.top(); pq.pop();
        auto b = pq.top(); pq.pop();
        pq.push(std::make_shared<Node>(a->freq + b->freq, a, b));
    }

    // Raíz del árbol
    auto root = pq.top();

    // Generar códigos (mapa byte -> vector<bool>)
    std::array<std::vector<bool>, 256> codes;

    // Función recursiva para recorrer el árbol y asignar códigos
    std::function<void(const std::shared_ptr<Node>&, std::vector<bool>&)> buildCodes;
    buildCodes = [&](const std::shared_ptr<Node> &n, std::vector<bool> &prefix) {
        if (!n) return;
        if (!n->left && !n->right) {
            // hoja
            if (prefix.empty()) {
                // caso especial: un solo símbolo -> asignar 0
                codes[n->byte] = {false};
            } else {
                codes[n->byte] = prefix;
            }
            return;
        }
        prefix.push_back(false);
        buildCodes(n->left, prefix);
        prefix.back() = true;
        buildCodes(n->right, prefix);
        prefix.pop_back();
    };

    std::vector<bool> tmp;
    buildCodes(root, tmp);

    // Escribir header: magic 'HUF1', total (uint64_t), 256 x uint64_t freq
    char magic[4] = {'H','U','F','1'};
    writeFile(outputFd, magic, 4);
    writeFile(outputFd, &total, sizeof(total));
    for (int i = 0; i < 256; ++i) {
        writeFile(outputFd, &freq[i], sizeof(freq[i]));
    }

    // Empaquetar bits y escribir payload
    uint8_t current = 0;
    int bitCount = 0; // número de bits ya en 'current' (0..7)
    auto flushByte = [&]() {
        writeFile(outputFd, &current, 1);
        current = 0;
        bitCount = 0;
    };

    // Codificar datos
    for (uint8_t b : data) {
        const std::vector<bool> &code = codes[b];
        for (bool bit : code) {
            current = (current << 1) | (bit ? 1 : 0);
            ++bitCount;
            if (bitCount == 8) {
                flushByte();
            }
        }
    }
    if (bitCount > 0) {
        // rellenar con ceros a la izquierda
        current <<= (8 - bitCount);
        writeFile(outputFd, &current, 1);
    }

    closeFile(inputFd);
    closeFile(outputFd);
}

// Decompress usando Huffman
// Formato esperado: [magic:4bytes='HUF1'][total:uint64_t][freqs:256 x uint64_t][payload:bits empaquetados en bytes]
void decompressHuffman(const std::string &inputPath, const std::string &outputPath) {
    // Abrir archivos
    int inputFd = openFile(inputPath, O_RDONLY);
    if (inputFd == -1) return;

    int outputFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outputFd == -1) {
        closeFile(inputFd);
        return;
    }

    // Leer magic
    char magic[4];
    if (readFile(inputFd, magic, 4) != 4) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }
    if (std::memcmp(magic, "HUF1", 4) != 0) {
        // formato desconocido
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Leer total de símbolos
    uint64_t totalSymbols = 0;
    if (readFile(inputFd, &totalSymbols, sizeof(totalSymbols)) != sizeof(totalSymbols)) {
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Leer tabla de frecuencias (256 entradas)
    std::array<uint64_t, 256> freq{};
    for (int i = 0; i < 256; ++i) {
        uint64_t f = 0;
        if (readFile(inputFd, &f, sizeof(f)) != sizeof(f)) {
            closeFile(inputFd);
            closeFile(outputFd);
            return;
        }
        freq[i] = f;
    }

    // Construir árbol Huffman
    struct Node {
        uint64_t freq;
        int byte; // -1 para nodos internos
        std::shared_ptr<Node> left, right;
        Node(uint64_t f, int b) : freq(f), byte(b), left(nullptr), right(nullptr) {}
        Node(uint64_t f, std::shared_ptr<Node> l, std::shared_ptr<Node> r) : freq(f), byte(-1), left(l), right(r) {}
    };

    // Comparador para priority_queue
    auto cmp = [](const std::shared_ptr<Node> &a, const std::shared_ptr<Node> &b) {
        return a->freq > b->freq;
    };

    // Construir la cola de prioridad
    std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, decltype(cmp)> pq(cmp);

    // Insertar nodos hoja
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            pq.push(std::make_shared<Node>(freq[i], i));
        }
    }

    // Manejar caso especial: archivo vacío
    if (pq.empty()) {
        // Nada que decodificar
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Construir el árbol combinando nodos
    while (pq.size() > 1) {
        auto a = pq.top(); pq.pop();
        auto b = pq.top(); pq.pop();
        pq.push(std::make_shared<Node>(a->freq + b->freq, a, b));
    }

    // Raíz del árbol
    auto root = pq.top();

    // Caso especial: solo un símbolo en todo el archivo
    if (!root->left && !root->right) {
        // escribir el byte 'totalSymbols' veces
        uint8_t value = static_cast<uint8_t>(root->byte);
        for (uint64_t i = 0; i < totalSymbols; ++i) {
            writeFile(outputFd, &value, 1);
        }
        closeFile(inputFd);
        closeFile(outputFd);
        return;
    }

    // Ahora leer el resto del archivo (payload codificado en bits)
    // Leer en bloques y decodificar bit por bit
    const size_t BUF_SIZE = 4096;
    std::vector<uint8_t> buffer(BUF_SIZE);
    std::shared_ptr<Node> node = root;
    uint64_t decoded = 0;

    ssize_t r;
    while ((r = readFile(inputFd, buffer.data(), BUF_SIZE)) > 0 && decoded < totalSymbols) {
        for (ssize_t i = 0; i < r && decoded < totalSymbols; ++i) {
            uint8_t byte = buffer[i];
            for (int bit = 7; bit >= 0 && decoded < totalSymbols; --bit) {
                int b = (byte >> bit) & 1;
                if (b == 0) node = node->left;
                else node = node->right;

                if (!node) {
                    // error en los bits
                    closeFile(inputFd);
                    closeFile(outputFd);
                    return;
                }

                if (!node->left && !node->right) {
                    uint8_t out = static_cast<uint8_t>(node->byte);
                    writeFile(outputFd, &out, 1);
                    ++decoded;
                    node = root;
                }
            }
        }
    }

    closeFile(inputFd);
    closeFile(outputFd);
}