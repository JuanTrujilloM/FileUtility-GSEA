#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <string>

// Algoritmo Run-Length Encoding (RLE)
void compressRLE(const std::string &inputPath, const std::string &outputPath);

void decompressRLE(const std::string &inputPath, const std::string &outputPath);    


// Algoritmo  Lempel-Ziv-Welch (LZW)
void compressLZW(const std::string &inputPath, const std::string &outputPath);

void decompressLZW(const std::string &inputPath, const std::string &outputPath);

#endif