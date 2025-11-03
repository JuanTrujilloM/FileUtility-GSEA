#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

// Abre un archivo y retorna su descriptor
int openFile(const std::string &path, int flags, int permissions = 0644);

// Lee bytes desde un archivo abierto
ssize_t readFile(int fd, void* buffer, size_t size);

// Escribe bytes en un archivo abierto
ssize_t writeFile(int fd, const void* buffer, size_t size);

// Cierra el archivo
void closeFile(int fd);

// Verifica si la ruta corresponde a un directorio
bool isDirectory(const std::string &path);

// Lista los archivos en un directorio
std::vector<std::string> listFiles(const std::string &directoryPath);

// Obtiene el tamaño de un archivo (en bytes). Retorna -1 si falla.
long long getFileSize(const std::string &path);

// Crea directorios recursivamente (como mkdir -p). Retorna true si existe o fue creado.
bool ensureDirectoryExists(const std::string &path);

#endif