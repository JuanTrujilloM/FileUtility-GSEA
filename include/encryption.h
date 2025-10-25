#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

// Algoritmo Vigenère
bool encryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key);
bool decryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key);

// Algoritmo AES-128
bool encryptAES128(const std::string &inputPath, const std::string &outputPath, const std::string &key);
bool decryptAES128(const std::string &inputPath, const std::string &outputPath, const std::string &key);

#endif
