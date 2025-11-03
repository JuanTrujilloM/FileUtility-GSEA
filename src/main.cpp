#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "fileManager.h"        // Para manejar la entrada/salida de archivos
#include "compression.h"         // Para compresión
#include "encryption.h"          // Para encriptación
#include <chrono>

void processFile(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key) {
    std::string current_input = input_path;  // El archivo individual
    std::vector<std::string> temp_files;

    for (size_t idx = 0; idx < operations.size(); ++idx) {
        char op = operations[idx];
        bool last = (idx == operations.size() - 1);
        std::string target = last ? output_path : (output_path + ".tmp" + std::to_string(idx));
        if (!last) temp_files.push_back(target);

        std::cout << "Procesando Archivo: " << current_input << std::endl;

        // Ejecutar operaciones según el tipo
        if (op == 'c') {
            // compresión
            if (comp_algorithm == "RLE") {
                long long before = getFileSize(current_input);
                std::cout << "[Compresión:RLE] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressRLE(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Compresión:RLE] Tamaño después: " << after << " bytes" << std::endl;

            } else if (comp_algorithm == "LZW") {
                long long before = getFileSize(current_input);
                std::cout << "[Compresión:LZW] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressLZW(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Compresión:LZW] Tamaño después: " << after << " bytes" << std::endl;

            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                long long before = getFileSize(current_input);
                std::cout << "[Compresión:Huffman] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressHuffman(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Compresión:Huffman] Tamaño después: " << after << " bytes" << std::endl;

            } else {
                std::cout << "Algoritmo de compresión no soportado: " << comp_algorithm << std::endl;
                // Limpiar archivos temporales
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
        } else if (op == 'd') {
            // descompresión
            if (comp_algorithm == "RLE") {
                long long before = getFileSize(current_input);
                std::cout << "[Descompresión:RLE] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressRLE(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Descompresión:RLE] Tamaño después: " << after << " bytes" << std::endl;

            } else if (comp_algorithm == "LZW") {
                long long before = getFileSize(current_input);
                std::cout << "[Descompresión:LZW] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressLZW(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Descompresión:LZW] Tamaño después: " << after << " bytes" << std::endl;

            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                long long before = getFileSize(current_input);
                std::cout << "[Descompresión:Huffman] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressHuffman(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Descompresión:Huffman] Tamaño después: " << after << " bytes" << std::endl;

            } else {
                std::cout << "Algoritmo de descompresión no soportado: " << comp_algorithm << std::endl;
                // Limpiar archivos temporales
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
        } else if (op == 'e') {
            // encriptación
            if (key.empty()) {
                std::cout << "Debe especificar la clave con -k" << std::endl;
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                long long before = getFileSize(current_input);
                std::cout << "[Encriptación:Vigenère] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    encryptVigenere(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Encriptación:Vigenère] Tamaño después: " << after << " bytes" << std::endl;

            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                long long before = getFileSize(current_input);
                std::cout << "[Encriptación:AES-128] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    encryptAES128(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Encriptación:AES-128] Tamaño después: " << after << " bytes" << std::endl;

            } else {
                std::cout << "Algoritmo de encriptación no soportado: " << enc_algorithm << std::endl;
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
        } else if (op == 'u') {
            // desencriptación
            if (key.empty()) {
                std::cout << "Debe especificar la clave con -k" << std::endl;
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                long long before = getFileSize(current_input);
                std::cout << "[Desencriptación:Vigenère] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decryptVigenere(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Desencriptación:Vigenère] Tamaño después: " << after << " bytes" << std::endl;

            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                long long before = getFileSize(current_input);
                std::cout << "[Desencriptación:AES-128] Tamaño antes: " << before << " bytes" << std::endl;
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decryptAES128(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    std::cout << "[Tiempo] " << elapsed << " s" << std::endl;
                }
                long long after = getFileSize(target);
                std::cout << "[Desencriptación:AES-128] Tamaño después: " << after << " bytes" << std::endl;
                
            } else {
                std::cout << "Algoritmo de desencriptación no soportado: " << enc_algorithm << std::endl;
                for (auto &f : temp_files) std::remove(f.c_str());
                return;
            }
        } else {
            std::cout << "Operación desconocida: " << op << std::endl;
            for (auto &f : temp_files) std::remove(f.c_str());
            return;
        }

        // Preparar input para la siguiente operación (si no es la última)
        if (!last) {
            current_input = target;
        }
    }

    // Limpiar temporales intermedios
    for (auto &f : temp_files) {
        if (std::remove(f.c_str()) != 0) {
            // No hacemos nada; es sólo intento de limpieza
        }
    }
    // Añadir una línea en blanco para separar la salida entre archivos procesados
    std::cout << std::endl;
}

void processFileOrDirectory(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key) {
    if (isDirectory(input_path)) {
        // Procesar cada archivo en el directorio
        // Asegurarse de que el directorio de salida exista
        ensureDirectoryExists(output_path);

        std::vector<std::string> files = listFiles(input_path);

        for (const auto& file : files) {
            std::string input_file = input_path + "/" + file;
            std::string output_file = output_path + "/" + file; // ajustar según sea necesario

            if (isDirectory(input_file)) {
                // Si es subdirectorio, crear el subdirectorio en la salida y llamar recursivamente
                ensureDirectoryExists(output_file);
                processFileOrDirectory(input_file, output_file, operations, comp_algorithm, enc_algorithm, key);
            } else {
                processFile(input_file, output_file, operations, comp_algorithm, enc_algorithm, key);
            }
        }
    } else {
        // Si es un archivo individual, procesarlo de la misma manera
        // Asegurar que exista el directorio padre del archivo de salida
        std::string out_parent = output_path;
        while (out_parent.size() > 1 && out_parent.back() == '/') out_parent.pop_back();
        size_t pos = out_parent.find_last_of('/');
        if (pos == std::string::npos) out_parent = ".";
        else if (pos == 0) out_parent = "/";
        else out_parent = out_parent.substr(0, pos);
        ensureDirectoryExists(out_parent);

        processFile(input_path, output_path, operations, comp_algorithm, enc_algorithm, key);
    }   
}

// Función principal que maneja la lógica de operaciones y procesamiento de archivos
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: ./FileUtility [operaciones] [opciones]" << std::endl;
        return 1;
    }

    // Variables de operaciones y parámetros
    std::string operation;
    std::string comp_algorithm;
    std::string enc_algorithm;
    std::string input_file, output_file, key;
    bool compress_flag = false, encrypt_flag = false, decompress_flag = false, decrypt_flag = false;

    // Parsear los argumentos
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-i") {
            input_file = argv[++i];  // Archivo o directorio de entrada

        } else if (std::string(argv[i]) == "-o") {
            output_file = argv[++i];  // Archivo o directorio de salida

        } else if (std::string(argv[i]) == "--comp-alg") {
            comp_algorithm = argv[++i];  // Algoritmo de compresión

        } else if (std::string(argv[i]) == "--enc-alg") {
            enc_algorithm = argv[++i];  // Algoritmo de encriptación

        } else if (std::string(argv[i]) == "-k") {
            key = argv[++i];  // Clave de encriptación/desencriptación (si es necesario)

        } else if (argv[i][0] == '-') {
            // Acumular flags cortas como -c, -e, -ce, -ed, etc.
            // Evitar sobrescribir si se pasan varias veces; concatenar los caracteres relevantes.
            std::string opt = argv[i];
            // Ignorar opciones largas ya detectadas (--comp-alg, --enc-alg) y opciones que toman argumento (-i, -o, -k)
            if (opt.rfind("--comp-alg", 0) == 0 || opt.rfind("--enc-alg", 0) == 0) {
                // ya manejadas arriba por igualdad exacta; no acumulamos
            } else if (opt == "-i" || opt == "-o" || opt == "-k") {
                // serán manejadas en sus ramas correspondientes, no acumulamos
            } else {
                // quitar el prefijo '-' y concatenar el resto
                if (opt.size() >= 2 && opt[0] == '-' ) {
                    operation += opt.substr(1);
                }
            }
        }
    }

    // Verificar que los archivos de entrada y salida están definidos
    if (input_file.empty() || output_file.empty()) {
        std::cout << "Debe especificar los archivos de entrada y salida!" << std::endl;
        return 1;
    }

    // Construir la lista de operaciones en orden y ejecutar encadenadas usando archivos temporales
    std::vector<char> ops;
    for (char ch : operation) {
        if (ch == '-') continue;
        ops.push_back(ch);
    }

    if (ops.empty()) {
        std::cout << "No se especificaron operaciones (por ejemplo -ce)." << std::endl;
        return 1;
    }

    if (input_file.empty() || output_file.empty()) {
        std::cout << "Debe especificar los archivos de entrada y salida!" << std::endl;
        return 1;
    }

    processFileOrDirectory(input_file, output_file, ops, comp_algorithm, enc_algorithm, key);

    return 0;
}

