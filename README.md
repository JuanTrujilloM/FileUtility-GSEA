# FileUtility-GSEA
GSEA (Secure and Efficient File Management Utility) – Proyecto 3 de Sistemas Operativos. Implements concurrent file compression and encryption using low-level system calls in C/C++.

## Compilación

Para compilar el proyecto, usa el siguiente comando:

```bash
make
```

También puedes usar:
- `make all` - Compila el proyecto
- `make clean` - Limpia los archivos compilados
- `make rebuild` - Limpia y recompila todo

El ejecutable se generará en `bin/FileUtility`.

## Uso

### Sintaxis básica:
```bash
./bin/FileUtility [operaciones] [opciones]
```

### Operaciones disponibles:
- `-c` : Comprimir
- `-d` : Descomprimir
- `-e` : Encriptar
- `-u` : Desencriptar (decrypt)

**Nota:** Puedes combinar operaciones, por ejemplo: `-ce` (comprimir y encriptar), `-ud` (desencriptar y descomprimir)

### Opciones:
- `-i <archivo>` : Archivo de entrada **(obligatorio)**
- `-o <archivo>` : Archivo de salida **(obligatorio)**
- `--comp-alg <algoritmo>` : Algoritmo de compresión
- `--enc-alg <algoritmo>` : Algoritmo de encriptación
- `-k <clave>` : Clave para encriptación/desencriptación

## Ejemplos de ejecución

### 1. Comprimir un archivo:
```bash
./bin/FileUtility -c -i archivo.txt -o archivo_comprimido.dat --comp-alg <algoritmo>
```

### 2. Encriptar un archivo:
```bash
./bin/FileUtility -e -i archivo.txt -o archivo_encriptado.dat --enc-alg <algoritmo> -k miClave123
```

### 3. Comprimir y encriptar:
```bash
./bin/FileUtility -ce -i archivo.txt -o archivo_protegido.dat --comp-alg <algoritmo> --enc-alg <algoritmo> -k miClave123
```

### 4. Descomprimir un archivo:
```bash
./bin/FileUtility -d -i archivo_comprimido.dat -o archivo_descomprimido.txt --comp-alg <algoritmo>
```

### 5. Desencriptar un archivo:
```bash
./bin/FileUtility -u -i archivo_encriptado.dat -o archivo_desencriptado.txt --enc-alg <algoritmo> -k miClave123
```

### 6. Desencriptar y descomprimir:
```bash
./bin/FileUtility -ud -i archivo_protegido.dat -o archivo_original.txt --comp-alg <algoritmo> --enc-alg <algoritmo> -k miClave123
```

## Casos Actuales

- Compresión: RLE

```bash
./bin/FileUtility -c -i tests/inputFiles/compress/Test_C.txt -o tests/outputFiles/compress test_Compressed.txt --comp-alg RLE
```

- Descompresión: RLE

```bash
./bin/FileUtility -d -i tests/inputFiles/decompress/Test_D.txt -o tests/outputFiles/decompress/test_Decompressed.txt --comp-alg RLE
```