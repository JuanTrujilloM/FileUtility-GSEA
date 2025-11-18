#include "Journal.h"
#include "fileManager.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>

// Constructor
Journal::Journal(const std::string& operation, const std::string& targetName, bool isDirectory) {
    startTime = std::chrono::steady_clock::now();
    
    // Crear directorio journal si no existe
    ensureDirectoryExists("journal");
    
    // Obtener timestamp para el nombre del archivo
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm_now, "%Y%m%d_%H%M%S");
    
    // Sanitizar el nombre del target
    std::string safeName = sanitizeName(targetName);
    
    // Construir el nombre del archivo de journal
    std::ostringstream filename;
    filename << "journal/journal_" << operation << "_" << safeName << "_" << timestamp.str() << ".log";
    journalPath = filename.str();
    
    // Abrir archivo de journal
    logFile.open(journalPath, std::ios::out);
    if (!logFile.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo de journal: " + journalPath);
    }
}

// Destructor
Journal::~Journal() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// Obtiene timestamp formateado
std::string Journal::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Sanitiza el nombre para usarlo en el nombre del archivo
std::string Journal::sanitizeName(const std::string& name) {
    std::string safe = name;
    
    // Obtener solo el nombre base (sin ruta completa)
    size_t lastSlash = safe.find_last_of('/');
    if (lastSlash != std::string::npos) {
        safe = safe.substr(lastSlash + 1);
    }
    
    // Reemplazar caracteres problemáticos
    std::replace(safe.begin(), safe.end(), '/', '_');
    std::replace(safe.begin(), safe.end(), '\\', '_');
    std::replace(safe.begin(), safe.end(), ' ', '_');
    std::replace(safe.begin(), safe.end(), ':', '_');
    std::replace(safe.begin(), safe.end(), '*', '_');
    std::replace(safe.begin(), safe.end(), '?', '_');
    std::replace(safe.begin(), safe.end(), '"', '_');
    std::replace(safe.begin(), safe.end(), '<', '_');
    std::replace(safe.begin(), safe.end(), '>', '_');
    std::replace(safe.begin(), safe.end(), '|', '_');
    
    // Limitar longitud
    if (safe.length() > 50) {
        safe = safe.substr(0, 50);
    }
    
    return safe;
}

// Escribe el encabezado inicial
void Journal::writeHeader(const std::string& operation, const std::string& targetPath,
                          const std::string& sourcePath, const std::string& destPath,
                          int totalFiles, long long totalSize) {
    std::lock_guard<std::mutex> lock(writeMutex);
    
    logFile << "========================================\n";
    
    if (totalFiles > 1) {
        logFile << "JOURNAL DE OPERACIÓN - CARPETA\n";
    } else {
        logFile << "JOURNAL DE OPERACIÓN - ARCHIVO\n";
    }
    
    logFile << "========================================\n";
    logFile << "Tipo: " << operation << "\n";
    
    if (totalFiles > 1) {
        logFile << "Carpeta: " << targetPath << "\n";
        if (!sourcePath.empty()) {
            logFile << "Ruta: " << sourcePath << "\n";
        }
        logFile << "Total archivos: " << totalFiles << "\n";
        if (totalSize > 0) {
            logFile << "Tamaño total: " << formatFileSize(totalSize) << "\n";
        }
    } else {
        logFile << "Archivo: " << targetPath << "\n";
        if (!sourcePath.empty()) {
            logFile << "Origen: " << sourcePath << "\n";
        }
        if (!destPath.empty()) {
            logFile << "Destino: " << destPath << "\n";
        }
        if (totalSize > 0) {
            logFile << "Tamaño: " << formatFileSize(totalSize) << "\n";
        }
    }
    
    logFile << "Timestamp inicio: " << getCurrentTimestamp() << "\n";
    logFile << "========================================\n\n";
    logFile.flush();
}

// Escribe una entrada de log
void Journal::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(writeMutex);
    
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    logFile << "[" << std::put_time(&tm_now, "%H:%M:%S") << "] " << message << "\n";
    logFile.flush();
}

// Escribe separador para archivos en carpetas
void Journal::logFileSeparator(int fileNum, int totalFiles, const std::string& filename) {
    std::lock_guard<std::mutex> lock(writeMutex);
    
    logFile << "\n";
    logFile << "----------------------------------------\n";
    logFile << "Archivo " << fileNum << "/" << totalFiles << ": " << filename << "\n";
    logFile << "----------------------------------------\n";
    logFile.flush();
}

// Escribe un bloque completo de texto de forma atómica
void Journal::logBlock(const std::string& block) {
    std::lock_guard<std::mutex> lock(writeMutex);
    logFile << block;
    logFile.flush();
}

// Escribe el resumen final
void Journal::writeSummary(const std::string& status, int filesProcessed, long long bytesProcessed) {
    std::lock_guard<std::mutex> lock(writeMutex);
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    logFile << "\n========================================\n";
    logFile << "[" << getCurrentTimestamp() << "] Proceso completado: " << status << "\n";
    
    if (filesProcessed > 1) {
        logFile << "Total procesado: " << filesProcessed << " archivos";
        if (bytesProcessed > 0) {
            logFile << " (" << formatFileSize(bytesProcessed) << ")";
        }
        logFile << "\n";
    }
    
    logFile << "Tiempo total: " << duration.count() << " ms\n";
    logFile << "========================================\n";
    logFile.flush();
}
