#include "encryption.h"
#include "fileManager.h"

#include <fcntl.h>
#include <cstddef>
#include <cctype>
#include <iostream>

// Normaliza una letra de la clave a valor 0-25 (A/a=0, B/b=1, etc.)
static int getKeyValue(char c) {
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a';
	return 0;
}

bool encryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key) {
	if (key.empty()) {
		std::cerr << "Error: la clave no puede estar vacía" << std::endl;
		return false;
	}

	// Abrir archivos de entrada y salida
	int inFd = openFile(inputPath, O_RDONLY);
	if (inFd == -1) return false;

	int outFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outFd == -1) {
		closeFile(inFd);
		return false;
	}

	// Buffers de lectura y escritura
	const std::size_t BUF = 4096;
	char inBuf[BUF];
	char outBuf[BUF];

	// Lógica de encriptación
	const std::size_t kLen = key.size();
	std::size_t kIdx = 0;

	while (true) {
		ssize_t n = readFile(inFd, inBuf, BUF);
		if (n == -1) {
			closeFile(inFd);
			closeFile(outFd);
			return false;
		}
		if (n == 0) break;

		for (ssize_t i = 0; i < n; ++i) {
			char ch = inBuf[i];
			
			if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
				int keyVal = getKeyValue(key[kIdx % kLen]);
				
				if (ch >= 'A' && ch <= 'Z') {
					int offset = (ch - 'A' + keyVal) % 26;
					outBuf[i] = 'A' + offset;
				} else {
					int offset = (ch - 'a' + keyVal) % 26;
					outBuf[i] = 'a' + offset;
				}
				
				kIdx++;
			} else {
				outBuf[i] = ch;
			}
		}

		std::size_t written = 0;
		while (written < static_cast<std::size_t>(n)) {
			ssize_t w = writeFile(outFd, outBuf + written, static_cast<std::size_t>(n) - written);
			if (w == -1) {
				closeFile(inFd);
				closeFile(outFd);
				return false;
			}
			written += static_cast<std::size_t>(w);
		}
	}

	closeFile(inFd);
	closeFile(outFd);
	return true;
}

bool decryptVigenere(const std::string &inputPath, const std::string &outputPath, const std::string &key) {
	if (key.empty()) {
		std::cerr << "Error: la clave no puede estar vacía" << std::endl;
		return false;
	}

	// Abrir archivos de entrada y salida
	int inFd = openFile(inputPath, O_RDONLY);
	if (inFd == -1) return false;

	int outFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outFd == -1) {
		closeFile(inFd);
		return false;
	}

	// Logica de desenciptacion
	const std::size_t BUF = 4096;
	char inBuf[BUF];
	char outBuf[BUF];

	const std::size_t kLen = key.size();
	std::size_t kIdx = 0;

	while (true) {
		ssize_t n = readFile(inFd, inBuf, BUF);
		if (n == -1) {
			closeFile(inFd);
			closeFile(outFd);
			return false;
		}
		if (n == 0) break;

		for (ssize_t i = 0; i < n; ++i) {
			char ch = inBuf[i];
			
			if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
				int keyVal = getKeyValue(key[kIdx % kLen]);
				
				if (ch >= 'A' && ch <= 'Z') {
					int offset = ((ch - 'A' - keyVal + 26) % 26);
					outBuf[i] = 'A' + offset;
				} else {
					int offset = ((ch - 'a' - keyVal + 26) % 26);
					outBuf[i] = 'a' + offset;
				}
				
				kIdx++;
			} else {
				outBuf[i] = ch;
			}
		}

		std::size_t written = 0;
		while (written < static_cast<std::size_t>(n)) {
			ssize_t w = writeFile(outFd, outBuf + written, static_cast<std::size_t>(n) - written);
			if (w == -1) {
				closeFile(inFd);
				closeFile(outFd);
				return false;
			}
			written += static_cast<std::size_t>(w);
		}
	}

	closeFile(inFd);
	closeFile(outFd);
	return true;
}
