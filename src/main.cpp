#include <iostream>
#include <string>
#include "fileManager.h"        // Para manejar la entrada/salida de archivos
#include "compression.h"         // Para compresión
#include "encryption.h"          // Para encriptación

// Función principal que maneja la lógica de operaciones y procesamiento de archivos
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: ./mi_programa [operaciones] [opciones]" << std::endl;
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
            operation = argv[i];  

        }
    }

    // Verificar que los archivos de entrada y salida están definidos
    if (input_file.empty() || output_file.empty()) {
        std::cout << "Debe especificar los archivos de entrada y salida!" << std::endl;
        return 1;
    }

    // Procesar cada operación de la cadena de operaciones (como -ce)
    for (char op : operation) {
        if (op == 'c') {
            compress_flag = true;
        } else if (op == 'd') {
            decompress_flag = true;
        } else if (op == 'e') {
            encrypt_flag = true;
        } else if (op == 'u') {
            decrypt_flag = true;
        }
    }

    // Ejecutar operaciones según las banderas activadas

    if (input_file.empty() || output_file.empty()) {
        std::cout << "Debe especificar los archivos de entrada y salida!" << std::endl;
        return 1;
    }
    
    // Si se requiere compresión
    if (compress_flag) {
        // Seleccionar el algoritmo de compresión
        // Ejecutar la compresión
    }

    // Si se requiere descompresión
    if (decompress_flag) {
        // Seleccionar el algoritmo de compresión
        // Ejecutar la descompresión
    }

    // Si se requiere encriptación
    if (encrypt_flag) {
        // Seleccionar el algoritmo de encriptación
        // Ejecutar la encriptación
    }

    // Si se requiere desencriptación
    if (decrypt_flag) {
        // Seleccionar el algoritmo de encriptación
        // Ejecutar la desencriptación
    }
    return 0;
}

