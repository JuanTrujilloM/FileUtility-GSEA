#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "fileManager.h"        // Para manejar la entrada/salida de archivos
#include "compression.h"         // Para compresión
#include "encryption.h"          // Para encriptación
#include <chrono>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include "ThreadPool.h"          // Thread pool para procesamiento concurrente

// Mutex global para sincronizar la salida a consola de forma thread-safe
static std::mutex cout_mutex;

/**
 * @brief Helper seguro para imprimir en consola desde múltiples hilos
 * 
 * Primero acumula toda la salida en un ostringstream y luego
 * imprime todo de una vez con un lock para evitar entrecruzamiento.
 * 
 * @param fn Función que recibe un ostream y escribe en él
 */
static void printLockedStream(const std::function<void(std::ostream&)> &fn) {
    std::ostringstream oss;
    fn(oss);
    std::lock_guard<std::mutex> lk(cout_mutex);
    std::cout << oss.str();
}

/**
 * @brief Genera nombre temporal único por archivo/operación
 * 
 * Utiliza el hash del input_path y el índice de operación para
 * crear nombres únicos de archivos temporales.
 * 
 * @param output_path Ruta base del archivo de salida
 * @param op_idx Índice de la operación actual
 * @param input_path Ruta del archivo de entrada (para generar hash único)
 * @return Nombre del archivo temporal
 */
static std::string makeTempName(const std::string &output_path, size_t op_idx, const std::string &input_path) {
    auto h = std::hash<std::string>{}(input_path);
    return output_path + ".tmp." + std::to_string(op_idx) + "." + std::to_string(h);
}

/**
 * @brief Procesa un archivo individual aplicando una cadena de operaciones
 * 
 * Aplica las operaciones en secuencia (compresión, descompresión, encriptación, desencriptación)
 * usando archivos temporales intermedios. La salida se bufferiza por archivo completo
 * para evitar entrecruzamiento de salidas entre hilos.
 * 
 * @param input_path Ruta del archivo de entrada
 * @param output_path Ruta del archivo de salida
 * @param operations Vector de operaciones a realizar ('c', 'd', 'e', 'u')
 * @param comp_algorithm Algoritmo de compresión (RLE, LZW, Huffman)
 * @param enc_algorithm Algoritmo de encriptación (VIG, AES)
 * @param key Clave para encriptación/desencriptación
 */
void processFile(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key) {
    std::string current_input = input_path;  // El archivo individual
    std::vector<std::string> temp_files;

    // Bufferizar toda la salida por archivo para que no se entremezcle entre hilos
    std::ostringstream oss;
    oss << "Procesando Archivo: " << current_input << "\n";

    // Iterar sobre cada operación en la cadena
    for (size_t idx = 0; idx < operations.size(); ++idx) {
        char op = operations[idx];
        bool last = (idx == operations.size() - 1);
        std::string target = last ? output_path : makeTempName(output_path, idx, input_path);
        if (!last) temp_files.push_back(target);

        // Ejecutar operaciones según el tipo
        if (op == 'c') {
            // Compresión
            if (comp_algorithm == "RLE") {
                long long before = getFileSize(current_input);
                oss << "[Compresión:RLE] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressRLE(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Compresión:RLE] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (comp_algorithm == "LZW") {
                long long before = getFileSize(current_input);
                oss << "[Compresión:LZW] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressLZW(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Compresión:LZW] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                long long before = getFileSize(current_input);
                oss << "[Compresión:Huffman] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    compressHuffman(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Compresión:Huffman] Tamaño después: " << formatFileSize(after) << "\n";
            } else {
                oss << "Algoritmo de compresión no soportado: " << comp_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
        } else if (op == 'd') {
            // Descompresión
            if (comp_algorithm == "RLE") {
                long long before = getFileSize(current_input);
                oss << "[Descompresión:RLE] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressRLE(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Descompresión:RLE] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (comp_algorithm == "LZW") {
                long long before = getFileSize(current_input);
                oss << "[Descompresión:LZW] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressLZW(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Descompresión:LZW] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                long long before = getFileSize(current_input);
                oss << "[Descompresión:Huffman] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decompressHuffman(current_input, target);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Descompresión:Huffman] Tamaño después: " << formatFileSize(after) << "\n";
            } else {
                oss << "Algoritmo de descompresión no soportado: " << comp_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
        } else if (op == 'e') {
            // Encriptación
            if (key.empty()) {
                oss << "Debe especificar la clave con -k\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                long long before = getFileSize(current_input);
                oss << "[Encriptación:Vigenère] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    encryptVigenere(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Encriptación:Vigenère] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                long long before = getFileSize(current_input);
                oss << "[Encriptación:AES-128] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    encryptAES128(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Encriptación:AES-128] Tamaño después: " << formatFileSize(after) << "\n";
            } else {
                oss << "Algoritmo de encriptación no soportado: " << enc_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
        } else if (op == 'u') {
            // Desencriptación
            if (key.empty()) {
                oss << "Debe especificar la clave con -k\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                long long before = getFileSize(current_input);
                oss << "[Desencriptación:Vigenère] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decryptVigenere(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Desencriptación:Vigenère] Tamaño después: " << formatFileSize(after) << "\n";
            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                long long before = getFileSize(current_input);
                oss << "[Desencriptación:AES-128] Tamaño antes: " << formatFileSize(before) << "\n";
                {
                    auto t1 = std::chrono::steady_clock::now();
                    decryptAES128(current_input, target, key);
                    auto t2 = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
                    oss << "[Tiempo] " << formatTime(elapsed) << "\n";
                }
                long long after = getFileSize(target);
                oss << "[Desencriptación:AES-128] Tamaño después: " << formatFileSize(after) << "\n";
            } else {
                oss << "Algoritmo de desencriptación no soportado: " << enc_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                oss << "\n";
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
        } else {
            oss << "Operación desconocida: " << op << "\n";
            for (auto &f : temp_files) std::remove(f.c_str());
            oss << "\n";
            printLockedStream([&](std::ostream &os){ os << oss.str(); });
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
            // intento de limpieza; ignorar errores
        }
    }
    // Añadir separación y volcar buffer de salida de una sola vez (mantiene orden por archivo)
    oss << "\n";
    printLockedStream([&](std::ostream &os){ os << oss.str(); });
}

/**
 * @brief Recolecta pares (input_file, output_file) recursivamente manteniendo estructura
 * 
 * Escanea recursivamente directorios y crea la estructura de carpetas necesaria
 * en el destino, acumulando pares de archivos (entrada, salida) para procesar.
 * 
 * @param in_path Ruta de entrada (archivo o directorio)
 * @param out_path Ruta de salida (archivo o directorio)
 * @param acc Vector acumulador de pares (entrada, salida)
 */
static void collectFilesRecursively(const std::string &in_path, const std::string &out_path, std::vector<std::pair<std::string,std::string>> &acc) {
    if (isDirectory(in_path)) {
        // Es un directorio: crear estructura y procesar recursivamente
        ensureDirectoryExists(out_path);
        std::vector<std::string> files = listFiles(in_path);
        for (const auto &entry : files) {
            std::string sub_in = in_path + "/" + entry;
            std::string sub_out = out_path + "/" + entry;
            collectFilesRecursively(sub_in, sub_out, acc);
        }
    } else {
        // Es un archivo: asegurar que el directorio padre existe y agregarlo a la lista
        std::string out_parent = out_path;
        while (out_parent.size() > 1 && out_parent.back() == '/') out_parent.pop_back();
        size_t pos = out_parent.find_last_of('/');
        if (pos == std::string::npos) out_parent = ".";
        else if (pos == 0) out_parent = "/";
        else out_parent = out_parent.substr(0, pos);
        ensureDirectoryExists(out_parent);
        acc.emplace_back(in_path, out_path);
    }
}

/**
 * @brief Ejecuta los trabajos con un thread pool (usando clase ThreadPool)
 * 
 * Crea un thread pool que usa hardware_concurrency() hilos y encola todas
 * las tareas de procesamiento de archivos para ejecución paralela.
 * 
 * @param tasks Vector de pares (archivo_entrada, archivo_salida)
 * @param operations Vector de operaciones a realizar
 * @param comp_algorithm Algoritmo de compresión
 * @param enc_algorithm Algoritmo de encriptación
 * @param key Clave de encriptación/desencriptación
 */
static void runThreadPool(const std::vector<std::pair<std::string,std::string>> &tasks,
                          const std::vector<char>& operations,
                          const std::string &comp_algorithm,
                          const std::string &enc_algorithm,
                          const std::string &key) {
    // Crear el thread pool (usa hardware_concurrency automáticamente)
    ThreadPool pool;
    
    // Mensaje inicial con concurrencia y número de hilos (usando mutex)
    printLockedStream([&](std::ostream &os){
        os << "Inicio de proceso con concurrencia: " << pool.getThreadCount() << " hilos\n\n";
    });

    // Encolar todas las tareas en el thread pool
    for (const auto &p : tasks) {
        pool.enqueue([p, &operations, &comp_algorithm, &enc_algorithm, &key]() {
            processFile(p.first, p.second, operations, comp_algorithm, enc_algorithm, key);
        });
    }

    // Esperar a que todas las tareas terminen
    pool.waitForCompletion();
}

/**
 * @brief Procesa un archivo o directorio completo con concurrencia
 * 
 * Punto de entrada principal para procesamiento. Recolecta todos los archivos
 * recursivamente y los procesa en paralelo usando el thread pool.
 * 
 * @param input_path Archivo o directorio de entrada
 * @param output_path Archivo o directorio de salida
 * @param operations Vector de operaciones a realizar
 * @param comp_algorithm Algoritmo de compresión
 * @param enc_algorithm Algoritmo de encriptación
 * @param key Clave de encriptación/desencriptación
 */
void processFileOrDirectory(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key) {
    // Recolectar todos los archivos (manteniendo estructura) y procesarlos en paralelo
    std::vector<std::pair<std::string,std::string>> tasks;
    collectFilesRecursively(input_path, output_path, tasks);

    if (tasks.empty()) {
        printLockedStream([&](std::ostream &os){ os << "No se encontraron archivos para procesar en: " << input_path << std::endl; });
        return;
    }

    runThreadPool(tasks, operations, comp_algorithm, enc_algorithm, key);
}

/**
 * @brief Función principal que maneja la lógica de operaciones y procesamiento de archivos
 * 
 * Parsea argumentos de línea de comandos y coordina el procesamiento de archivos.
 * Soporta múltiples operaciones encadenadas (-ce, -ced, etc.) y procesamiento
 * de archivos individuales o directorios completos.
 * 
 * @param argc Número de argumentos
 * @param argv Array de argumentos de línea de comandos
 * @return 0 si éxito, 1 si error
 */
int main(int argc, char* argv[]) {
    // Verificar argumentos mínimos
    if (argc < 2) {
        std::cout << "Uso: ./FileUtility [operaciones] [opciones]" << std::endl;
        return 1;
    }

    // Variables de operaciones y parámetros
    std::string operation;
    std::string comp_algorithm;
    std::string enc_algorithm;
    std::string input_file, output_file, key;

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
            std::string opt = argv[i];
            // Ignorar opciones largas ya detectadas (--comp-alg, --enc-alg) y opciones que toman argumento (-i, -o, -k)
            if (opt.rfind("--comp-alg", 0) == 0 || opt.rfind("--enc-alg", 0) == 0) {
                // ya manejadas arriba por igualdad exacta; no acumulamos
            } else if (opt == "-i" || opt == "-o" || opt == "-k") {
                // serán manejadas en sus ramas correspondientes, no acumulamos
            } else {
                // quitar el prefijo '-' y concatenar el resto
                if (opt.size() >= 2 && opt[0] == '-') {
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

    // Procesar archivo o directorio completo con concurrencia
    processFileOrDirectory(input_file, output_file, ops, comp_algorithm, enc_algorithm, key);

    return 0;
}