#include "fileManager.h"
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, close
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // stat
#include <iostream>     // std::cerr

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
        files.push_back(entry->d_name);
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