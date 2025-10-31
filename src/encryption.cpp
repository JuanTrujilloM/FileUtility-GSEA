#include "encryption.h"
#include "fileManager.h"

#include <fcntl.h>
#include <cstddef>
#include <cctype>
#include <iostream>
#include <vector>
#include <cstdint>
#include <array>
#include <cstring>
#include <algorithm>


// Encrypt usando Vigenere
// Formato: [Payload cifrado]
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

// Decrypt usando Vigenere
// Formato esperado: [Payload descifrado]
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

// Encrypt usando AES-128
// Modo CBC con padding PKCS#7
// La clave se ajusta a 16 bytes (truncando o repitiendo)
// El vector de inicialización (IV) se genera aleatoriamente y se escribe al inicio del archivo cifrado
// Formato: [IV:16 bytes][Payload cifrado]
bool encryptAES128(const std::string &inputPath, const std::string &outputPath, const std::string &key) {
	// AES-128 CBC streaming + PKCS#7 padding.
	// Escribe IV de 16 bytes al inicio del archivo cifrado.
	if (key.empty()) {
		std::cerr << "Error: la clave no puede estar vacía para AES-128" << std::endl;
		return false;
	}

	// Preparar clave de 16 bytes (truncar o repetir)
	std::array<uint8_t, 16> keyBytes{};
	for (size_t i = 0; i < 16 && i < key.size(); ++i) keyBytes[i] = static_cast<uint8_t>(key[i]);
	if (key.size() < 16 && !key.empty()) {
		for (size_t i = key.size(); i < 16; ++i) keyBytes[i] = static_cast<uint8_t>(key[i % key.size()]);
	}

	int inFd = openFile(inputPath, O_RDONLY);
	if (inFd == -1) return false;
	int outFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outFd == -1) { closeFile(inFd); return false; }

	// --- AES core implementation (inline helpers) ---
	static const uint8_t sbox[256] = {
        // 0     1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
        0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
        0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
        0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
        0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
        0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
        0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
        0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
        0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
        0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
        0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
        0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
        0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
        0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
        0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
        0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
        0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
    };

	static const uint8_t rcon[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};

	auto subWord = [&](uint8_t a)->uint8_t { return sbox[a]; };

	// Key expansion: expand 16-byte key into 176 bytes (11 round keys)
	std::array<uint8_t, 176> roundKeys{};
	for (int i = 0; i < 16; ++i) roundKeys[i] = keyBytes[i];
	int bytesGenerated = 16;
	int rconIter = 1;
	uint8_t temp[4];
	while (bytesGenerated < 176) {
		for (int i = 0; i < 4; ++i) temp[i] = roundKeys[bytesGenerated - 4 + i];
		if (bytesGenerated % 16 == 0) {
			// rotate
			uint8_t t = temp[0]; temp[0]=temp[1]; temp[1]=temp[2]; temp[2]=temp[3]; temp[3]=t;
			// subWord
			for (int i = 0; i < 4; ++i) temp[i] = subWord(temp[i]);
			// Rcon
			temp[0] ^= rcon[rconIter++];
		}
		for (int i = 0; i < 4; ++i) {
			roundKeys[bytesGenerated] = roundKeys[bytesGenerated - 16] ^ temp[i];
			bytesGenerated++;
		}
	}

	auto xtime = [](uint8_t x)->uint8_t { return static_cast<uint8_t>((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00)); };
	auto mul = [&](uint8_t a, uint8_t b)->uint8_t {
		uint8_t res = 0;
		uint8_t tempA = a;
		uint8_t tempB = b;
		while (tempB) {
			if (tempB & 1) res ^= tempA;
			tempB >>= 1;
			tempA = xtime(tempA);
		}
		return res;
	};

	auto subBytes = [&](uint8_t state[16]){
		for (int i = 0; i < 16; ++i) state[i] = sbox[state[i]];
	};

	auto shiftRows = [&](uint8_t state[16]){
		uint8_t tmp[16];
		// Row 0
		tmp[0]  = state[0]; tmp[4]  = state[4]; tmp[8]  = state[8]; tmp[12] = state[12];
		// Row 1
		tmp[1]  = state[5]; tmp[5]  = state[9]; tmp[9]  = state[13]; tmp[13] = state[1];
		// Row 2
		tmp[2]  = state[10]; tmp[6]  = state[14]; tmp[10] = state[2]; tmp[14] = state[6];
		// Row 3
		tmp[3]  = state[15]; tmp[7]  = state[3]; tmp[11] = state[7]; tmp[15] = state[11];
		std::memcpy(state, tmp, 16);
	};

	auto mixColumns = [&](uint8_t state[16]){
		for (int c = 0; c < 4; ++c) {
			int i = c*4;
			uint8_t a0 = state[i+0];
			uint8_t a1 = state[i+1];
			uint8_t a2 = state[i+2];
			uint8_t a3 = state[i+3];
			uint8_t r0 = static_cast<uint8_t>(mul(0x02,a0) ^ mul(0x03,a1) ^ a2 ^ a3);
			uint8_t r1 = static_cast<uint8_t>(a0 ^ mul(0x02,a1) ^ mul(0x03,a2) ^ a3);
			uint8_t r2 = static_cast<uint8_t>(a0 ^ a1 ^ mul(0x02,a2) ^ mul(0x03,a3));
			uint8_t r3 = static_cast<uint8_t>(mul(0x03,a0) ^ a1 ^ a2 ^ mul(0x02,a3));
			state[i+0]=r0; state[i+1]=r1; state[i+2]=r2; state[i+3]=r3;
		}
	};

	auto addRoundKey = [&](uint8_t state[16], const uint8_t* roundKey){
		for (int i = 0; i < 16; ++i) state[i] ^= roundKey[i];
	};

	auto aesEncryptBlock = [&](const uint8_t in[16], uint8_t out[16]){
		uint8_t state[16];
		std::memcpy(state, in, 16);
		// initial round key
		addRoundKey(state, roundKeys.data());
		for (int round = 1; round <= 9; ++round) {
			subBytes(state);
			shiftRows(state);
			mixColumns(state);
			addRoundKey(state, roundKeys.data() + round*16);
		}
		// final round
		subBytes(state);
		shiftRows(state);
		addRoundKey(state, roundKeys.data() + 160);
		std::memcpy(out, state, 16);
	};

	// Helper: obtener 16 bytes aleatorios desde /dev/urandom
	auto getRandom16 = [&]()->std::array<uint8_t,16>{
		std::array<uint8_t,16> rv{};
		int rnd = openFile("/dev/urandom", O_RDONLY);
		if (rnd != -1) {
			ssize_t r = readFile(rnd, rv.data(), 16);
			(void)r;
			closeFile(rnd);
		} else {
			for (int i=0;i<16;++i) rv[i] = static_cast<uint8_t>(i);
		}
		return rv;
	};

	// Generar IV y escribirlo al inicio
	std::array<uint8_t,16> iv = getRandom16();
	if (writeFile(outFd, iv.data(), 16) != 16) { closeFile(inFd); closeFile(outFd); return false; }

	std::array<uint8_t,16> prevCipher = iv;

	// Streaming: leer y procesar bloques. Mantener buffer para gestionar padding
	const size_t READBUF = 4096;
	char rbuf[READBUF];
	std::vector<uint8_t> buffer;
	buffer.reserve(READBUF + 16);
	while (true) {
		ssize_t n = readFile(inFd, rbuf, READBUF);
		if (n == -1) { closeFile(inFd); closeFile(outFd); return false; }
		if (n == 0) break;
		buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(rbuf), reinterpret_cast<uint8_t*>(rbuf) + n);
		// process all full blocks except keep last partial block (if any)
		while (buffer.size() > 16) {
			uint8_t block[16];
			for (int i=0;i<16;++i) block[i] = buffer[i];
			for (int i=0;i<16;++i) block[i] ^= prevCipher[i];
			uint8_t outBlock[16];
			aesEncryptBlock(block, outBlock);
			if (writeFile(outFd, outBlock, 16) != 16) { closeFile(inFd); closeFile(outFd); return false; }
			for (int i=0;i<16;++i) prevCipher[i] = outBlock[i];
			buffer.erase(buffer.begin(), buffer.begin()+16);
		}
	}

	// Ahora buffer.size() <= 16; aplicar PKCS#7 padding
	uint8_t padLen = static_cast<uint8_t>(16 - (buffer.size() % 16));
	if (padLen == 0) padLen = 16;
	while (buffer.size() % 16 != 0) buffer.push_back(padLen);
	if (buffer.empty()) {
		for (int i=0;i<16;++i) buffer.push_back(padLen);
	}
	for (size_t off = 0; off < buffer.size(); off += 16) {
		uint8_t block[16];
		for (int i=0;i<16;++i) block[i] = buffer[off + i];
		for (int i=0;i<16;++i) block[i] ^= prevCipher[i];
		uint8_t outBlock[16];
		aesEncryptBlock(block, outBlock);
		if (writeFile(outFd, outBlock, 16) != 16) { closeFile(inFd); closeFile(outFd); return false; }
		for (int i=0;i<16;++i) prevCipher[i] = outBlock[i];
	}

	closeFile(inFd);
	closeFile(outFd);
	return true;
}

// Decrypt usando AES-128
// Modo CBC con padding PKCS#7
// La clave se ajusta a 16 bytes (truncando o repitiendo)
// Formato esperado: [IV:16 bytes][Payload descifrado]
bool decryptAES128(const std::string &inputPath, const std::string &outputPath, const std::string &key) {
	// AES-128 CBC streaming decryption
	if (key.empty()) {
		std::cerr << "Error: la clave no puede estar vacía para AES-128" << std::endl;
		return false;
	}

	std::array<uint8_t, 16> keyBytes{};
	for (size_t i = 0; i < 16 && i < key.size(); ++i) keyBytes[i] = static_cast<uint8_t>(key[i]);
	if (key.size() < 16 && !key.empty()) {
		for (size_t i = key.size(); i < 16; ++i) keyBytes[i] = static_cast<uint8_t>(key[i % key.size()]);
	}

	int inFd = openFile(inputPath, O_RDONLY);
	if (inFd == -1) return false;
	int outFd = openFile(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outFd == -1) { closeFile(inFd); return false; }

	// Leer IV (primeros 16 bytes)
	uint8_t iv[16];
	if (readFile(inFd, iv, 16) != 16) { closeFile(inFd); closeFile(outFd); return false; }

	// Key expansion (same as encrypt)
	static const uint8_t sbox[256] = {
		0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
		0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
		0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
		0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
		0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
		0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
		0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
		0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
		0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
		0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
		0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
		0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
		0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
		0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
		0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
		0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
	};

	static const uint8_t rcon[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};

	std::array<uint8_t, 176> roundKeys{};
	for (int i = 0; i < 16; ++i) roundKeys[i] = keyBytes[i];
	int bytesGenerated = 16;
	int rconIter = 1;
	uint8_t temp[4];
	auto subWord = [&](uint8_t a)->uint8_t { return sbox[a]; };
	while (bytesGenerated < 176) {
		for (int i = 0; i < 4; ++i) temp[i] = roundKeys[bytesGenerated - 4 + i];
		if (bytesGenerated % 16 == 0) {
			uint8_t t = temp[0]; temp[0]=temp[1]; temp[1]=temp[2]; temp[2]=temp[3]; temp[3]=t;
			for (int i = 0; i < 4; ++i) temp[i] = subWord(temp[i]);
			temp[0] ^= rcon[rconIter++];
		}
		for (int i = 0; i < 4; ++i) {
			roundKeys[bytesGenerated] = roundKeys[bytesGenerated - 16] ^ temp[i];
			bytesGenerated++;
		}
	}

	auto xtime = [](uint8_t x)->uint8_t { return static_cast<uint8_t>((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00)); };
	auto mul = [&](uint8_t a, uint8_t b)->uint8_t {
		uint8_t res = 0;
		uint8_t tempA = a;
		uint8_t tempB = b;
		while (tempB) {
			if (tempB & 1) res ^= tempA;
			tempB >>= 1;
			tempA = xtime(tempA);
		}
		return res;
	};

	auto invSubBytes = [&](uint8_t state[16]){
		// build inverse sbox on first use
		static uint8_t invs[256];
		static bool inited = false;
		if (!inited) {
			for (int i = 0; i < 256; ++i) invs[sbox[i]] = static_cast<uint8_t>(i);
			inited = true;
		}
		for (int i = 0; i < 16; ++i) state[i] = invs[state[i]];
	};

	auto invShiftRows = [&](uint8_t state[16]){
		uint8_t tmp[16];
		// Row0
		tmp[0]=state[0]; tmp[4]=state[4]; tmp[8]=state[8]; tmp[12]=state[12];
		// Row1
		tmp[1]=state[13]; tmp[5]=state[1]; tmp[9]=state[5]; tmp[13]=state[9];
		// Row2
		tmp[2]=state[10]; tmp[6]=state[14]; tmp[10]=state[2]; tmp[14]=state[6];
		// Row3
		tmp[3]=state[7]; tmp[7]=state[11]; tmp[11]=state[15]; tmp[15]=state[3];
		std::memcpy(state, tmp, 16);
	};

	auto invMixColumns = [&](uint8_t state[16]){
		for (int c = 0; c < 4; ++c) {
			int i = c*4;
			uint8_t a0 = state[i+0];
			uint8_t a1 = state[i+1];
			uint8_t a2 = state[i+2];
			uint8_t a3 = state[i+3];
			uint8_t r0 = static_cast<uint8_t>(mul(0x0e,a0) ^ mul(0x0b,a1) ^ mul(0x0d,a2) ^ mul(0x09,a3));
			uint8_t r1 = static_cast<uint8_t>(mul(0x09,a0) ^ mul(0x0e,a1) ^ mul(0x0b,a2) ^ mul(0x0d,a3));
			uint8_t r2 = static_cast<uint8_t>(mul(0x0d,a0) ^ mul(0x09,a1) ^ mul(0x0e,a2) ^ mul(0x0b,a3));
			uint8_t r3 = static_cast<uint8_t>(mul(0x0b,a0) ^ mul(0x0d,a1) ^ mul(0x09,a2) ^ mul(0x0e,a3));
			state[i+0]=r0; state[i+1]=r1; state[i+2]=r2; state[i+3]=r3;
		}
	};

	auto addRoundKey = [&](uint8_t state[16], const uint8_t* roundKey){ for (int i=0;i<16;++i) state[i]^=roundKey[i]; };

	auto aesDecryptBlock = [&](const uint8_t in[16], uint8_t out[16]){
		uint8_t state[16]; std::memcpy(state, in, 16);
		addRoundKey(state, roundKeys.data() + 160);
		for (int round = 9; round >= 1; --round) {
			invShiftRows(state);
			invSubBytes(state);
			addRoundKey(state, roundKeys.data() + round*16);
			invMixColumns(state);
		}
		invShiftRows(state);
		invSubBytes(state);
		addRoundKey(state, roundKeys.data());
		std::memcpy(out, state, 16);
	};

	// Streaming decryption CBC: leer bloques de 16 bytes, mantener prevCipher para XOR
	uint8_t prevCipher[16];
	for (int i=0;i<16;++i) prevCipher[i] = iv[i];

	const size_t READBUF = 4096;
	char cbuf[READBUF];
	std::vector<uint8_t> cbuffer;
	cbuffer.reserve(READBUF + 16);
	// leer todo resto del archivo en buffer por bloques; we'll process block-by-block keeping last block to handle padding
	while (true) {
		ssize_t n = readFile(inFd, cbuf, READBUF);
		if (n == -1) { closeFile(inFd); closeFile(outFd); return false; }
		if (n == 0) break;
		cbuffer.insert(cbuffer.end(), reinterpret_cast<uint8_t*>(cbuf), reinterpret_cast<uint8_t*>(cbuf) + n);
		// process full ciphertext blocks except leave last block unprocessed (for padding)
		while (cbuffer.size() > 16) {
			uint8_t cblock[16];
			for (int i=0;i<16;++i) cblock[i] = cbuffer[i];
			uint8_t dec[16];
			aesDecryptBlock(cblock, dec);
			// plaintext = dec XOR prevCipher
			uint8_t plain[16];
			for (int i=0;i<16;++i) plain[i] = dec[i] ^ prevCipher[i];
			// write plaintext
			if (writeFile(outFd, plain, 16) != 16) { closeFile(inFd); closeFile(outFd); return false; }
			// shift prevCipher
			for (int i=0;i<16;++i) prevCipher[i] = cblock[i];
			// remove processed
			cbuffer.erase(cbuffer.begin(), cbuffer.begin()+16);
		}
	}

	// Now cbuffer.size() should be 0 or 16 (last ciphertext block)
	if (cbuffer.size() != 16) { // malformed ciphertext size
		// no data left -> nothing to unpad
		closeFile(inFd); closeFile(outFd); return false;
	}
	uint8_t lastC[16]; for (int i=0;i<16;++i) lastC[i] = cbuffer[i];
	uint8_t decLast[16]; aesDecryptBlock(lastC, decLast);
	uint8_t plainLast[16]; for (int i=0;i<16;++i) plainLast[i] = decLast[i] ^ prevCipher[i];

	// remove padding PKCS#7 from plainLast
	uint8_t last = plainLast[15];
	if (last == 0 || last > 16) { closeFile(inFd); closeFile(outFd); std::cerr<<"Error: padding inválido"<<std::endl; return false; }
	for (size_t i = 0; i < last; ++i) if (plainLast[16-1-i] != last) { closeFile(inFd); closeFile(outFd); std::cerr<<"Error: padding inconsistente"<<std::endl; return false; }
	// write final plaintext without padding
	size_t writeLen = 16 - last;
	if (writeLen > 0) {
		if (writeFile(outFd, plainLast, writeLen) != static_cast<ssize_t>(writeLen)) { closeFile(inFd); closeFile(outFd); return false; }
	}

	closeFile(inFd);
	closeFile(outFd);
	return true;
}
