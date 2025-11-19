#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include "fileManager.h"        // Para manejar la entrada/salida de archivos
#include "compression.h"         // Para compresión
#include "encryption.h"          // Para encriptación
#include "Journal.h"             // Para journaling de operaciones
#include <chrono>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include "ThreadPool.h"          // Thread pool para procesamiento concurrente
#include "TableFormatter.h"      // Para formatear salida en tablas

// Mutex global para sincronizar la salida a consola de forma thread-safe
static std::mutex cout_mutex;

// Estructura para almacenar resultados de procesamiento
struct FileResult {
    std::string filename;
    long long originalSize;
    long long finalSize;
    double ratio;
    long long timeMs;
    std::string status;
};

// Vector global thread-safe para acumular resultados
static std::vector<FileResult> globalResults;
static std::mutex results_mutex;

// Función auxiliar para imprimir de forma thread-safe
static void printLockedStream(const std::function<void(std::ostream&)> &fn) {
    std::ostringstream oss;
    fn(oss);
    std::lock_guard<std::mutex> lk(cout_mutex);
    std::cout << oss.str();
}

// Función auxiliar para generar nombres de archivos temporales únicos
static std::string makeTempName(const std::string &output_path, size_t op_idx, const std::string &input_path) {
    auto h = std::hash<std::string>{}(input_path);
    return output_path + ".tmp." + std::to_string(op_idx) + "." + std::to_string(h);
}

// Función para procesar un solo archivo con las operaciones especificadas
void processFile(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key, Journal* journal = nullptr, int fileNum = 1, int totalFiles = 1) {
    std::string current_input = input_path;  // El archivo individual
    std::vector<std::string> temp_files;
    
    long long originalSize = getFileSize(current_input);
    long long totalTime = 0;
    std::string status = "OK";

    // Obtener nombre base del archivo
    std::string baseName = input_path;
    size_t lastSlash = baseName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        baseName = baseName.substr(lastSlash + 1);
    }

    // Buffer local para acumular logs (evita mezcla en multithreading)
    std::ostringstream logBuffer;
    
    // Función auxiliar para agregar timestamp al buffer
    auto addLogToBuffer = [&logBuffer]() -> std::ostream& {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
        localtime_r(&time_t_now, &tm_now);
        logBuffer << "[" << std::put_time(&tm_now, "%H:%M:%S") << "] ";
        return logBuffer;
    };

    // Si hay journal y es una carpeta, escribir separador en buffer
    if (journal && totalFiles > 1) {
        logBuffer << "\n";
        logBuffer << "----------------------------------------\n";
        logBuffer << baseName << "\n";
        logBuffer << "----------------------------------------\n";
        addLogToBuffer() << "Procesando archivo " << baseName << "...\n";
        addLogToBuffer() << "Tamaño: " << formatFileSize(originalSize) << "\n";
    }

    // Iterar sobre cada operación en la cadena
    for (size_t idx = 0; idx < operations.size(); ++idx) {
        char op = operations[idx];
        bool last = (idx == operations.size() - 1);
        std::string target = last ? output_path : makeTempName(output_path, idx, input_path);
        if (!last) temp_files.push_back(target);

        // Registrar inicio de operación específica
        std::string opName;
        if (op == 'c') opName = "Comprimiendo con " + comp_algorithm;
        else if (op == 'd') opName = "Descomprimiendo con " + comp_algorithm;
        else if (op == 'e') opName = "Encriptando con " + enc_algorithm;
        else if (op == 'u') opName = "Desencriptando con " + enc_algorithm;
        
        if (journal) {
            addLogToBuffer() << opName << "...\n";
        }

        // Ejecutar operaciones según el tipo
        if (op == 'c') {
            // Detectar extensión del archivo
            std::string extension;
            size_t dotPos = input_path.find_last_of('.');
            if (dotPos != std::string::npos) {
                extension = input_path.substr(dotPos);
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            }
            
            // Sugerencias basadas en el tipo de archivo
            std::string suggestedAlgorithm;
            std::string reason;
            bool shouldSuggest = false;
            
            // Texto: .txt, .log, .md, .csv
            if (extension == ".txt" || extension == ".log" || extension == ".md" || extension == ".csv") {
                if (comp_algorithm != "Huff" && comp_algorithm != "Huffman") {
                    suggestedAlgorithm = "Huffman";
                    reason = "archivo de texto";
                    shouldSuggest = true;
                }
            }
            // Imágenes: .bmp, .pgm, .ppm
            else if (extension == ".bmp" || extension == ".pgm" || extension == ".ppm") {
                if (comp_algorithm == "RLE") {
                    suggestedAlgorithm = "Huffman o LZW";
                    reason = "imagen sin comprimir";
                    shouldSuggest = true;
                }
            }
            // Audio: .wav, .aiff, .au
            else if (extension == ".wav" || extension == ".aiff" || extension == ".au") {
                if (comp_algorithm == "RLE") {
                    suggestedAlgorithm = "Huffman o LZW";
                    reason = "audio sin comprimir";
                    shouldSuggest = true;
                }
            }
            // Video: .avi, .mov
            else if (extension == ".avi" || extension == ".mov") {
                if (comp_algorithm != "Huff" && comp_algorithm != "Huffman") {
                    suggestedAlgorithm = "Huffman";
                    reason = "video";
                    shouldSuggest = true;
                }
            }
            // Binarios/Ejecutables: .bin, .exe
            else if (extension == ".bin" || extension == ".exe" || extension == "") {
                // Para ejecutables sin extensión (Linux)
                bool isExecutable = (extension == "" && baseName.find('.') == std::string::npos);
                if (isExecutable || extension == ".bin" || extension == ".exe") {
                    if (comp_algorithm != "LZW") {
                        suggestedAlgorithm = "LZW";
                        reason = "binario/ejecutable";
                        shouldSuggest = true;
                    }
                }
            }
            
            // Mostrar sugerencia solo para archivos individuales (no para carpetas)
            if (shouldSuggest && totalFiles == 1) {
                // Si la sugerencia incluye múltiples opciones (Huffman o LZW)
                if (suggestedAlgorithm.find("o") != std::string::npos) {
                    std::ostringstream suggestion;
                    suggestion << "\nSUGERENCIA para " << baseName << ":\n";
                    suggestion << "   Este es un " << reason << " (" << extension << ").\n";
                    suggestion << "   RLE no es óptimo para este tipo de archivo.\n";
                    suggestion << "   Algoritmo actual: " << comp_algorithm << "\n";
                    suggestion << "   ¿Desea cambiar el algoritmo?\n";
                    suggestion << "   1) Huffman\n";
                    suggestion << "   2) LZW\n";
                    suggestion << "   3) Continuar con RLE\n";
                    suggestion << "   Seleccione (1/2/3): ";
                    
                    printLockedStream([&](std::ostream &os){ os << suggestion.str(); });
                    
                    std::string response;
                    std::getline(std::cin, response);
                    
                    if (response == "1") {
                        const_cast<std::string&>(comp_algorithm) = "Huff";
                        printLockedStream([](std::ostream &os){ 
                            os << "✓ Cambiando a Huffman para mejor compresión.\n\n"; 
                        });
                        if (journal) {
                            addLogToBuffer() << "Usuario cambió algoritmo a Huffman\n";
                        }
                    } else if (response == "2") {
                        const_cast<std::string&>(comp_algorithm) = "LZW";
                        printLockedStream([](std::ostream &os){ 
                            os << "✓ Cambiando a LZW para mejor compresión.\n\n"; 
                        });
                        if (journal) {
                            addLogToBuffer() << "Usuario cambió algoritmo a LZW\n";
                        }
                    } else {
                        printLockedStream([](std::ostream &os){ 
                            os << "✓ Continuando con RLE.\n\n"; 
                        });
                    }
                } else {
                    // Sugerencia de un solo algoritmo
                    std::ostringstream suggestion;
                    suggestion << "\n⚠️  SUGERENCIA para " << baseName << ":\n";
                    suggestion << "   Este es un " << reason << " (" << extension << ").\n";
                    suggestion << "   El algoritmo " << suggestedAlgorithm << " suele ofrecer mejor compresión.\n";
                    suggestion << "   Algoritmo actual: " << comp_algorithm << "\n";
                    suggestion << "   ¿Desea cambiar a " << suggestedAlgorithm << "? (s/n): ";
                    
                    printLockedStream([&](std::ostream &os){ os << suggestion.str(); });
                    
                    std::string response;
                    std::getline(std::cin, response);
                    if (response == "s" || response == "S" || response == "si" || response == "Si" || response == "SI") {
                        const_cast<std::string&>(comp_algorithm) = suggestedAlgorithm;
                        printLockedStream([&](std::ostream &os){ 
                            os << "✓ Cambiando a " << suggestedAlgorithm << " para mejor compresión.\n\n"; 
                        });
                        if (journal) {
                            addLogToBuffer() << "Usuario cambió algoritmo a " << suggestedAlgorithm << "\n";
                        }
                    } else {
                        printLockedStream([](std::ostream &os){ 
                            os << "✓ Continuando con el algoritmo original.\n\n"; 
                        });
                    }
                }
            }  
            // Compresión
            auto t1 = std::chrono::steady_clock::now();
            if (comp_algorithm == "RLE") {
                compressRLE(current_input, target);
            } else if (comp_algorithm == "LZW") {
                compressLZW(current_input, target);
            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                compressHuffman(current_input, target);
            } else {
                std::ostringstream oss;
                oss << "Algoritmo de compresión no soportado: " << comp_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t2 = std::chrono::steady_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (journal) addLogToBuffer() << "Compresión completada\n";
        } else if (op == 'd') {
            // Descompresión
            auto t1 = std::chrono::steady_clock::now();
            if (comp_algorithm == "RLE") {
                decompressRLE(current_input, target);
            } else if (comp_algorithm == "LZW") {
                decompressLZW(current_input, target);
            } else if (comp_algorithm == "Huff" || comp_algorithm == "Huffman") {
                decompressHuffman(current_input, target);
            } else {
                std::ostringstream oss;
                oss << "Algoritmo de descompresión no soportado: " << comp_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t2 = std::chrono::steady_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (journal) addLogToBuffer() << "Descompresión completada\n";
        } else if (op == 'e') {
            // Encriptación
            if (key.empty()) {
                std::ostringstream oss;
                oss << "Debe especificar la clave con -k\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t1 = std::chrono::steady_clock::now();
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                encryptVigenere(current_input, target, key);
            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                encryptAES128(current_input, target, key);
            } else {
                std::ostringstream oss;
                oss << "Algoritmo de encriptación no soportado: " << enc_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t2 = std::chrono::steady_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (journal) addLogToBuffer() << "Encriptación completada\n";
        } else if (op == 'u') {
            // Desencriptación
            if (key.empty()) {
                std::ostringstream oss;
                oss << "Debe especificar la clave con -k\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t1 = std::chrono::steady_clock::now();
            if (enc_algorithm == "VIG" || enc_algorithm == "VIGENERE" || enc_algorithm == "Vigenere") {
                decryptVigenere(current_input, target, key);
            } else if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
                decryptAES128(current_input, target, key);
            } else {
                std::ostringstream oss;
                oss << "Algoritmo de desencriptación no soportado: " << enc_algorithm << "\n";
                for (auto &f : temp_files) std::remove(f.c_str());
                if (journal) {
                    addLogToBuffer() << "ERROR: " << oss.str();
                    journal->logBlock(logBuffer.str());
                }
                printLockedStream([&](std::ostream &os){ os << oss.str(); });
                return;
            }
            auto t2 = std::chrono::steady_clock::now();
            totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (journal) addLogToBuffer() << "Desencriptación completada\n";
        } else {
            std::ostringstream oss;
            oss << "Operación desconocida: " << op << "\n";
            for (auto &f : temp_files) std::remove(f.c_str());
            if (journal) {
                addLogToBuffer() << "ERROR: " << oss.str();
                journal->logBlock(logBuffer.str());
            }
            printLockedStream([&](std::ostream &os){ os << oss.str(); });
            return;
        }

        // Preparar input para la siguiente operación (si no es la última)
        if (!last) {
            current_input = target;
        }
    }

    // Obtener tamaño final y calcular ratio
    long long finalSize = getFileSize(output_path);
    double ratio = (originalSize > 0) ? (100.0 * (originalSize - finalSize) / originalSize) : 0.0;
    
    // Registrar completado en journal (en buffer)
    if (journal) {
        addLogToBuffer() << "Archivo completado\n";
        addLogToBuffer() << "Tamaño final: " << formatFileSize(finalSize) << "\n";
        addLogToBuffer() << "Tiempo procesamiento: " << formatTime(totalTime / 1000.0) << "\n";
        
        // Escribir todo el buffer de una vez (atómico)
        journal->logBlock(logBuffer.str());
    }
    
    // Crear resultado y agregarlo al vector global
    FileResult result;
    result.filename = input_path;
    result.originalSize = originalSize;
    result.finalSize = finalSize;
    result.ratio = ratio;
    result.timeMs = totalTime;
    result.status = status;
    
    // Agregar resultado de forma thread-safe
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        globalResults.push_back(result);
    }

    // Limpiar temporales intermedios
    for (auto &f : temp_files) {
        if (std::remove(f.c_str()) != 0) {
            // intento de limpieza; ignorar errores
        }
    }
}

// Función recursiva para recolectar todos los archivos en un directorio
static void collectFilesRecursively(const std::string &in_path, const std::string &out_path, std::vector<std::pair<std::string,std::string>> &acc) {
    if (isDirectory(in_path)) {
        // Es un directorio: crear estructura y procesar recursivamente
        ensureDirectoryExists(out_path);
        std::vector<std::string> files = listFiles(in_path);
        
        // Normalizar in_path: eliminar barra final si existe
        std::string normalized_in = in_path;
        if (!normalized_in.empty() && normalized_in.back() == '/') {
            normalized_in.pop_back();
        }
        
        // Normalizar out_path: eliminar barra final si existe
        std::string normalized_out = out_path;
        if (!normalized_out.empty() && normalized_out.back() == '/') {
            normalized_out.pop_back();
        }
        
        for (const auto &entry : files) {
            std::string sub_in = normalized_in + "/" + entry;
            std::string sub_out = normalized_out + "/" + entry;
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

// Función para ejecutar el thread pool y procesar todas las tareas
static void runThreadPool(const std::vector<std::pair<std::string,std::string>> &tasks,
                          const std::vector<char>& operations,
                          const std::string &comp_algorithm,
                          const std::string &enc_algorithm,
                          const std::string &key,
                          const std::string &input_path) {
    // Crear el thread pool (usa hardware_concurrency automáticamente)
    ThreadPool pool;
    
    // Mensaje inicial con concurrencia y número de hilos (usando mutex)
    printLockedStream([&](std::ostream &os){
        os << "Inicio de proceso con concurrencia: " << pool.getThreadCount() << " hilos\n\n";
    });

    // Determinar el nombre de la operación para el journal
    std::string opName;
    for (char op : operations) {
        if (op == 'c') opName += "COMPRESS_";
        else if (op == 'd') opName += "DECOMPRESS_";
        else if (op == 'e') opName += "ENCRYPT_";
        else if (op == 'u') opName += "DECRYPT_";
    }
    if (!opName.empty() && opName.back() == '_') {
        opName.pop_back();
    }

    // Determinar si es un archivo o carpeta
    bool isDirectory = tasks.size() > 1;
    
    // Obtener nombre base del target
    std::string targetName = input_path;
    size_t lastSlash = targetName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        targetName = targetName.substr(lastSlash + 1);
    }

    // Calcular tamaño total
    long long totalSize = 0;
    for (const auto &p : tasks) {
        long long sz = getFileSize(p.first);
        if (sz > 0) totalSize += sz;
    }

    // Crear el journal
    Journal* journal = nullptr;
    try {
        journal = new Journal(opName, targetName, isDirectory);
        
        // Escribir encabezado
        if (isDirectory) {
            journal->writeHeader(opName, targetName, input_path, "", tasks.size(), totalSize);
            journal->log("Inicio de proceso...");
            journal->log("Escaneando carpeta...");
            journal->log("Procesando " + std::to_string(tasks.size()) + " archivos...");
        } else {
            journal->writeHeader(opName, targetName, tasks[0].first, tasks[0].second, 1, totalSize);
            journal->log("Inicio de proceso...");
        }
    } catch (const std::exception &e) {
        printLockedStream([&](std::ostream &os){
            os << "Advertencia: No se pudo crear el journal: " << e.what() << "\n";
        });
    }

    // Encolar todas las tareas en el thread pool
    int fileCounter = 0;
    for (const auto &p : tasks) {
        fileCounter++;
        int currentFileNum = fileCounter;
        pool.enqueue([p, &operations, &comp_algorithm, &enc_algorithm, &key, journal, currentFileNum, &tasks]() {
            processFile(p.first, p.second, operations, comp_algorithm, enc_algorithm, key, 
                       journal, currentFileNum, tasks.size());
        });
    }

    // Esperar a que todas las tareas terminen
    pool.waitForCompletion();
    
    // Determinar el encabezado apropiado según las operaciones
    std::string sizeHeader = "Procesado";
    if (!operations.empty()) {
        bool hasCompress = std::find(operations.begin(), operations.end(), 'c') != operations.end();
        bool hasDecompress = std::find(operations.begin(), operations.end(), 'd') != operations.end();
        bool hasEncrypt = std::find(operations.begin(), operations.end(), 'e') != operations.end();
        bool hasDecrypt = std::find(operations.begin(), operations.end(), 'u') != operations.end();
        
        // Operaciones combinadas
        if (hasCompress && hasEncrypt) {
            sizeHeader = "Comp+Encript";
        }
        else if (hasDecompress && hasDecrypt) {
            sizeHeader = "Desc+Desencript";
        }
        else if (hasCompress && hasDecrypt) {
            sizeHeader = "Comp+Desencript";
        }
        else if (hasDecompress && hasEncrypt) {
            sizeHeader = "Desc+Encript";
        }
        // Operaciones individuales
        else if (hasCompress) {
            sizeHeader = "Comprimido";
        }
        else if (hasDecompress) {
            sizeHeader = "Descomprimido";
        }
        else if (hasEncrypt) {
            sizeHeader = "Encriptado";
        }
        else if (hasDecrypt) {
            sizeHeader = "Desencriptado";
        }
    }
    
    // Imprimir tabla con todos los resultados
    TableFormatter table({"Archivo", "Original", sizeHeader, "Rendimiento", "Tiempo", "Estado"});
    
    long long totalOriginal = 0;
    long long totalCompressed = 0;
    long long totalTime = 0;
    int totalFiles = 0;
    
    for (const auto& result : globalResults) {
        std::ostringstream ratioStream;
        ratioStream << std::fixed << std::setprecision(2) << result.ratio;
        
        table.addRow({
            result.filename,
            formatFileSize(result.originalSize),
            formatFileSize(result.finalSize),
            ratioStream.str(),
            formatTime(result.timeMs / 1000.0),
            "✓ " + result.status
        });
        
        totalOriginal += result.originalSize;
        totalCompressed += result.finalSize;
        totalTime += result.timeMs;
        totalFiles++;
    }
    
    // Agregar fila de totales
    double totalRatio = (totalOriginal > 0) ? (100.0 * (totalOriginal - totalCompressed) / totalOriginal) : 0.0;
    std::ostringstream totalRatioStream;
    totalRatioStream << std::fixed << std::setprecision(2) << totalRatio;
    
    table.addRow({
        "TOTAL",
        formatFileSize(totalOriginal),
        formatFileSize(totalCompressed),
        totalRatioStream.str(),
        formatTime(totalTime / 1000.0),
        std::to_string(totalFiles) + "/" + std::to_string(totalFiles)
    });
    
    // Imprimir la tabla
    std::cout << "\n" << table.toString() << "\n";
    
    // Imprimir resumen adicional
    std::cout << "Tiempo Total: " << formatTime(totalTime / 1000.0) << "\n";
    std::cout << "Tasa de Procesamiento: " << std::fixed << std::setprecision(2) 
              << (totalFiles / (totalTime / 1000.0)) << " archivos/s\n";
    
    // Escribir resumen final y tabla en el journal
    if (journal) {
        long long totalProcessed = 0;
        for (const auto &result : globalResults) {
            totalProcessed += result.originalSize;
        }
        journal->writeSummary("EXITOSO", tasks.size(), totalProcessed);
        
        // Escribir la tabla de resultados en el journal
        std::ostringstream tableStream;
        tableStream << "\n" << std::string(60, '=') << "\n";
        tableStream << "TABLA DE RESULTADOS\n";
        tableStream << std::string(60, '=') << "\n\n";
        tableStream << table.toString();
        tableStream << "\nTiempo Total: " << formatTime(totalTime / 1000.0) << "\n";
        tableStream << "Tasa de Procesamiento: " << std::fixed << std::setprecision(2) 
                     << (totalFiles / (totalTime / 1000.0)) << " archivos/s\n";
        
        // Escribir todo el bloque junto sin timestamps
        journal->logBlock(tableStream.str());
        
        printLockedStream([&](std::ostream &os){
            os << "\n✓ Journal creado: " << journal->getJournalPath() << "\n";
        });
        
        delete journal;
    }
}

// Función para procesar un archivo o directorio completo
void processFileOrDirectory(const std::string& input_path, const std::string& output_path, const std::vector<char>& operations, const std::string& comp_algorithm, const std::string& enc_algorithm, const std::string& key) {
    // Limpiar resultados globales de ejecuciones anteriores
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        globalResults.clear();
    }
    
    // Recolectar todos los archivos (manteniendo estructura) y procesarlos en paralelo
    std::vector<std::pair<std::string,std::string>> tasks;
    collectFilesRecursively(input_path, output_path, tasks);

    if (tasks.empty()) {
        printLockedStream([&](std::ostream &os){ os << "No se encontraron archivos para procesar en: " << input_path << std::endl; });
        return;
    }

    runThreadPool(tasks, operations, comp_algorithm, enc_algorithm, key, input_path);
}

// Función para validar la clave de encriptación
bool validateSecureKey(const std::string& key, const std::string& enc_algorithm) {
    // Determinar longitud mínima según el algoritmo
    size_t minLength = 8;
    if (enc_algorithm == "AES" || enc_algorithm == "AES128" || enc_algorithm == "AES-128") {
        minLength = 16; // AES-128 requiere clave de 16 bytes (128 bits)
    }
    
    // Verificar longitud mínima
    if (key.length() < minLength) {
        std::cout << "⚠️  ADVERTENCIA: La clave es muy corta. ";
        std::cout << "Se requieren al menos " << minLength << " caracteres para " << enc_algorithm << ".\n";
        std::cout << "Longitud actual: " << key.length() << " caracteres.\n";
        return false;
    }
    
    // Verificar complejidad de la clave
    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;
    
    for (char c : key) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else if (std::ispunct(c) || std::isspace(c)) hasSpecial = true;
    }
    
    int complexity = hasUpper + hasLower + hasDigit + hasSpecial;
    
    if (complexity < 3) {
        std::cout << "⚠️  ADVERTENCIA: La clave es débil. Se recomienda usar:\n";
        if (!hasUpper) std::cout << "  - Al menos una letra MAYÚSCULA\n";
        if (!hasLower) std::cout << "  - Al menos una letra minúscula\n";
        if (!hasDigit) std::cout << "  - Al menos un número\n";
        if (!hasSpecial) std::cout << "  - Al menos un carácter especial (!@#$%^&*)\n";
        std::cout << "¿Desea continuar de todas formas? (s/n): ";
        
        std::string response;
        std::getline(std::cin, response);
        if (response != "s" && response != "S" && response != "si" && response != "Si" && response != "SI") {
            return false;
        }
    }
    
    // Verificar que no sea una clave común
    std::vector<std::string> commonKeys = {
        "password", "12345678", "abc12345", "password123",
        "admin123"
    };
    
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    
    for (const auto& common : commonKeys) {
        if (lowerKey.find(common) != std::string::npos) {
            std::cout << "⚠️  ADVERTENCIA: La clave contiene patrones comunes y es insegura.\n";
            std::cout << "Por favor, use una clave más compleja y única.\n";
            return false;
        }
    }
    
    return true;
}

// Función principal
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

    // Validar clave si hay operaciones de encriptación/desencriptación
    bool needsKey = false;
    for (char op : ops) {
        if (op == 'e') {
            needsKey = true;
            break;
        }
    }
    
    if (needsKey) {
        if (key.empty()) {
            std::cout << "Error: Se requiere una clave (-k) para operaciones de encriptación/desencriptación.\n";
            return 1;
        }
        
        // Validar que la clave sea segura
        if (!validateSecureKey(key, enc_algorithm)) {
            std::cout << "Error: La clave no cumple con los requisitos de seguridad.\n";
            return 1;
        }
        
        std::cout << "✓ Clave validada correctamente.\n\n";
    }

    // Procesar archivo o directorio completo con concurrencia
    processFileOrDirectory(input_file, output_file, ops, comp_algorithm, enc_algorithm, key);

    return 0;
}