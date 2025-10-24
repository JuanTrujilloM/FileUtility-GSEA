#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

// Cifrado/Descifrado Vigenère a nivel de bytes (mod 256)
// Retorna true en éxito, false en error (I/O o clave vacía)
bool encryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key);
bool decryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key);

#endif
