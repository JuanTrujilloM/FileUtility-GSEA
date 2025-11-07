#include "fileManager.h"
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, close
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // stat
#include <string>
#include <vector>
#include <iostream>
#include <errno.h>

int openFile(const std::string &path, int flags, int permissions) {
    int fd = open(path.c_str(), flags, permissions);
    if (fd == -1) {
        perror(("Error al abrir archivo: " + path).c_str());
    }
    return fd;
}

ssize_t readFile(int fd, void*buffer, size_t size) {
    ssize_t bytesRead = read(fd, buffer, size);

    if (bytesRead == -1) {
        perror("Error al leer archivo");
    }

    return bytesRead;
}

ssize_t writeFile(int fd, const void* buffer, size_t size) {
    ssize_t bytesWritten = write(fd, buffer, size);

    if (bytesWritten == -1) {
        perror("Error al escribir en el archivo");
    }

    return bytesWritten;
}

void closeFile(int fd) {
    int closed = close(fd);
    if (closed == -1) {
        perror("Error al cerrar el archivo");
    }
}

bool isDirectory(const std::string &path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0)
        return false;
    return S_ISDIR(pathStat.st_mode);
}

std::vector<std::string> listFiles(const std::string &directoryPath) {
    std::vector<std::string> files;
    DIR *dir = opendir(directoryPath.c_str());
    if (dir == nullptr) {
        perror(("Error al abrir directorio: " + directoryPath).c_str());
        return files;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Filtrar "." y ".." para evitar recursión infinita y entradas no deseadas
        std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        files.push_back(name);
    }

    closedir(dir);
    return files;
}

long long getFileSize(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return static_cast<long long>(st.st_size);
    }
    perror(("Error al obtener tamaño de archivo: " + path).c_str());
    return -1;
}

bool ensureDirectoryExists(const std::string &path) {
    if (path.empty()) return false;

    std::string normalized = path;
    // quitar barra final si existe (pero mantener si es root "/")
    if (normalized.size() > 1 && normalized.back() == '/') normalized.pop_back();

    struct stat st;
    if (stat(normalized.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) return true;
        // existe pero no es dir
        return false;
    }

    // Construir recursivamente
    std::string current;
    if (!normalized.empty() && normalized[0] == '/') current = "/";

    size_t pos = 0;
    while (pos < normalized.size()) {
        size_t next = normalized.find('/', pos);
        std::string token;
        if (next == std::string::npos) {
            token = normalized.substr(pos);
            pos = normalized.size();
        } else {
            token = normalized.substr(pos, next - pos);
            pos = next + 1;
        }

        if (token.empty()) continue;

        if (!current.empty() && current != "/") current += "/";
        current += token;

        if (stat(current.c_str(), &st) != 0) {
            if (mkdir(current.c_str(), 0755) != 0) {
                if (errno == EEXIST) continue; // posible condición de carrera
                perror(("Error creando directorio: " + current).c_str());
                return false;
            }
        } else {
            if (!S_ISDIR(st.st_mode)) {
                std::cerr << "Existe un archivo con el mismo nombre que el directorio: " << current << std::endl;
                return false;
            }
        }
    }

    return true;
}

std::string formatFileSize(long long bytes) {
    if (bytes < 0) return "N/A";
    
    const long long KB = 1024;
    const long long MB = KB * 1024;
    const long long GB = MB * 1024;
    const long long TB = GB * 1024;
    
    char buffer[64];
    
    if (bytes >= TB) {
        snprintf(buffer, sizeof(buffer), "%.2f TB", static_cast<double>(bytes) / TB);
    } else if (bytes >= GB) {
        snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<double>(bytes) / GB);
    } else if (bytes >= MB) {
        snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<double>(bytes) / MB);
    } else if (bytes >= KB) {
        snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<double>(bytes) / KB);
    } else {
        snprintf(buffer, sizeof(buffer), "%lld bytes", bytes);
    }
    
    return std::string(buffer);
}

std::string formatTime(double seconds) {
    char buffer[64];
    
    if (seconds >= 3600.0) {
        // Horas
        double hours = seconds / 3600.0;
        snprintf(buffer, sizeof(buffer), "%.2f h", hours);
    } else if (seconds >= 60.0) {
        // Minutos
        double minutes = seconds / 60.0;
        snprintf(buffer, sizeof(buffer), "%.2f min", minutes);
    } else if (seconds >= 1.0) {
        // Segundos
        snprintf(buffer, sizeof(buffer), "%.2f s", seconds);
    } else if (seconds >= 0.001) {
        // Milisegundos
        double milliseconds = seconds * 1000.0;
        snprintf(buffer, sizeof(buffer), "%.2f ms", milliseconds);
    } else if (seconds >= 0.000001) {
        // Microsegundos
        double microseconds = seconds * 1000000.0;
        snprintf(buffer, sizeof(buffer), "%.2f μs", microseconds);
    } else {
        // Nanosegundos
        double nanoseconds = seconds * 1000000000.0;
        snprintf(buffer, sizeof(buffer), "%.2f ns", nanoseconds);
    }
    
    return std::string(buffer);
}