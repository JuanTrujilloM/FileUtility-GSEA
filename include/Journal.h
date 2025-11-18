#ifndef JOURNAL_H
#define JOURNAL_H

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>

class Journal {
private:
    std::string journalPath;
    std::ofstream logFile;
    std::mutex writeMutex;
    std::chrono::steady_clock::time_point startTime;
    
    // Obtiene timestamp formateado
    std::string getCurrentTimestamp();
    
    // Sanitiza el nombre de archivo/carpeta para usarlo en el nombre del journal
    std::string sanitizeName(const std::string& name);

public:
    // Constructor: crea el archivo de journal
    Journal(const std::string& operation, const std::string& targetName, bool isDirectory);
    
    // Destructor: cierra el archivo
    ~Journal();
    
    // Escribe el encabezado inicial del journal
    void writeHeader(const std::string& operation, const std::string& targetPath, 
                     const std::string& sourcePath = "", const std::string& destPath = "",
                     int totalFiles = 1, long long totalSize = 0);
    
    // Escribe una entrada de log con timestamp
    void log(const std::string& message);
    
    // Escribe un separador para cada archivo en procesamiento de carpetas
    void logFileSeparator(int fileNum, int totalFiles, const std::string& filename);
    
    // Escribe un bloque completo de texto de forma atómica (para evitar mezcla en multithreading)
    void logBlock(const std::string& block);
    
    // Escribe el resumen final
    void writeSummary(const std::string& status, int filesProcessed = 1, long long bytesProcessed = 0);
    
    // Obtiene la ruta del archivo de journal
    std::string getJournalPath() const { return journalPath; }
};

#endif
