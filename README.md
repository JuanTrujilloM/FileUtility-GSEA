# FileUtility-GSEA

**GSEA** (Secure and Efficient File Management Utility) – Proyecto 3 de Sistemas Operativos.

Utilidad de línea de comandos para compresión, descompresión, encriptación y desencriptación de archivos con soporte para procesamiento concurrente y journaling automático.

## Tabla de Contenidos

- [Características](#características)
- [Requisitos](#requisitos)
- [Compilación](#compilación)
- [Uso](#uso)
- [Algoritmos Disponibles](#algoritmos-disponibles)
- [Ejemplos](#ejemplos)
- [Operaciones con Carpetas](#operaciones-con-carpetas)
- [Sistema de Journaling](#sistema-de-journaling)
- [Casos de Prueba](#casos-de-prueba)

## Características

- **Compresión/Descompresión**: RLE, LZW, Huffman
- **Encriptación/Desencriptación**: Vigenere, AES-128 (CBC)
- **Operaciones combinadas**: Comprimir + Encriptar en una sola ejecución
- **Procesamiento concurrente**: Usa thread pool para carpetas con múltiples archivos
- **Journaling automático**: Registro detallado de todas las operaciones
- **Soporte para carpetas**: Procesamiento recursivo de directorios completos
- **Validación de claves**: Verifica complejidad y seguridad de contraseñas

## Requisitos

- **Sistema Operativo**: Linux
- **Compilador**: g++ con soporte para C++11 o superior
- **Bibliotecas**: Estándar de C++ (no requiere dependencias externas)

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
- `--comp-alg <algoritmo>` : Algoritmo de compresión (RLE, LZW, Huff)
- `--enc-alg <algoritmo>` : Algoritmo de encriptación (VIG, AES128)
- `-k <clave>` : Clave para encriptación/desencriptación

## Algoritmos Disponibles

### Compresión
- **RLE** (Run-Length Encoding): Ideal para archivos con datos repetitivos
- **LZW** (Lempel-Ziv-Welch): Compresión basada en diccionario, buena relación velocidad/tamaño
- **Huff/Huffman**: Compresión basada en frecuencia de símbolos, excelente para texto

### Encriptación
- **VIG/Vigenere**: Cifrado por sustitución polialfabética, requiere clave alfanumérica
- **AES/AES128**: AES-128 en modo CBC, requiere clave de mínimo 16 caracteres

## Recomendaciones por Tipo de Archivo

El programa incluye un **sistema de sugerencias inteligente** que recomienda el mejor algoritmo según el tipo de archivo:

### Archivos de Texto
**Extensiones:** `.txt`, `.log`, `.md`, `.csv`  
**Algoritmo recomendado:** **Huffman**  
- Mejor compresión para texto plano y estructurado
- Eficiencia: 60-70% de reducción típica

### Imágenes sin Comprimir
**Extensiones:** `.bmp`, `.pgm`, `.ppm`  
**Algoritmos recomendados:** **Huffman** o **LZW**  
- Huffman: Ligeramente mejor en general
- LZW: Excelente para imágenes con patrones
- Evitar RLE a menos que sean imágenes muy simples (logos, iconos)
- Eficiencia: 50-70% de reducción

### Audio sin Comprimir
**Extensiones:** `.wav`, `.aiff`, `.au`  
**Algoritmos recomendados:** **Huffman** o **LZW**  
- Huffman: Mejor rendimiento general
- LZW: Alternativa válida
- Evitar RLE
- Eficiencia: 15-35% de reducción

### Video
**Extensiones:** `.avi`, `.mov`  
**Algoritmo recomendado:** **Huffman**  
- Nota: Videos modernos (MP4, MKV) ya están comprimidos y no se benefician de compresión adicional

### Binarios y Ejecutables
**Extensiones:** `.bin`, `.exe`, archivos sin extensión (ejecutables Linux)  
**Algoritmo recomendado:** **LZW**  
- Mejor para código binario y estructuras de datos complejas
- Eficiencia: 40-60% de reducción

### Sistema Interactivo de Sugerencias

Cuando comprimes un **archivo individual** con un algoritmo subóptimo, el programa te preguntará si deseas cambiar:

```bash
$ ./bin/FileUtility -c -i documento.txt -o output.dat --comp-alg LZW

   SUGERENCIA para documento.txt:
   Este es un archivo de texto (.txt).
   El algoritmo Huffman suele ofrecer mejor compresión.
   Algoritmo actual: LZW
   ¿Desea cambiar a Huffman? (s/n):
```

**Para imágenes y audio con RLE**, se ofrecen múltiples opciones:

```bash
$ ./bin/FileUtility -c -i imagen.bmp -o output.dat --comp-alg RLE

   SUGERENCIA para imagen.bmp:
   Este es un imagen sin comprimir (.bmp).
   RLE no es óptimo para este tipo de archivo.
   Algoritmo actual: RLE
   ¿Desea cambiar el algoritmo?
   1) Huffman
   2) LZW
   3) Continuar con RLE
   Seleccione (1/2/3):
```

**Nota:** Las sugerencias solo aparecen para archivos individuales, no cuando se procesan carpetas completas.

## Ejemplos

### 1. Comprimir un archivo:
```bash
./bin/FileUtility -c -i archivo.txt -o archivo_comprimido.dat --comp-alg RLE
```

### 2. Encriptar un archivo:
```bash
./bin/FileUtility -e -i archivo.txt -o archivo_encriptado.dat --enc-alg VIG -k "Encrypt3*PassK3y@"
```

### 3. Comprimir y encriptar:
```bash
./bin/FileUtility -ce -i archivo.txt -o archivo_protegido.dat --comp-alg LZW --enc-alg AES128 -k "Encrypt3*PassK3y@"
```

### 4. Descomprimir un archivo:
```bash
./bin/FileUtility -d -i archivo_comprimido.dat -o archivo_descomprimido.txt --comp-alg RLE
```

### 5. Desencriptar un archivo:
```bash
./bin/FileUtility -u -i archivo_encriptado.dat -o archivo_desencriptado.txt --enc-alg VIG -k "Encrypt3*PassK3y@"
```

### 6. Desencriptar y descomprimir:
```bash
./bin/FileUtility -ud -i archivo_protegido.dat -o archivo_original.txt --comp-alg LZW --enc-alg AES128 -k "Encrypt3*PassK3y@"
```

## Operaciones con Carpetas

El programa soporta procesamiento de carpetas completas de forma recursiva. Utiliza un thread pool para procesar múltiples archivos en paralelo, mejorando significativamente el rendimiento.

**Ejemplo:**
```bash
./bin/FileUtility -ce -i tests -o testsOut --comp-alg LZW --enc-alg AES128 -k "Encrypt3*PassK3y@"
```

Esto procesará todos los archivos dentro de `tests/` y sus subdirectorios, manteniendo la estructura de carpetas en `testsOut/`.

## Sistema de Journaling

El programa implementa un sistema de journaling que registra todas las operaciones realizadas, creando logs detallados para trazabilidad y auditoría.

### Características del Journaling

#### Para Archivos Individuales
- Crea un archivo `.log` por cada operación
- Registra: tipo de operación, tamaños, timestamps, cada paso del proceso
- Formato: `journal/journal_[OPERACION]_[ARCHIVO]_[TIMESTAMP].log`

#### Para Carpetas
- Crea un único archivo `.log` para toda la carpeta
- Registra cada archivo procesado dentro de la carpeta
- Incluye separadores visuales entre archivos
- Formato: `journal/journal_[OPERACION]_[CARPETA]_[TIMESTAMP].log`

### Ejemplo de Journal

Al ejecutar:
```bash
./bin/FileUtility -c -i tests/Test1C.txt -o tests/Test1C_compressed.txt --comp-alg RLE
```

Se genera: `journal/journal_COMPRESS_Test1C.txt_20251117_143025.log`

**Contenido del archivo journal:**
```
========================================
JOURNAL DE OPERACIÓN - ARCHIVO
========================================
Tipo: COMPRESS
Archivo: Test1C.txt
Origen: tests/Test1C.txt
Destino: tests/Test1C_compressed.txt
Tamaño: 126.08 KB
Timestamp inicio: 2025-11-17 14:30:25
========================================

[14:30:25] Inicio de proceso...
[14:30:25] Comprimiendo con RLE...
[14:30:26] Compresión completada
[14:30:26] Archivo completado
[14:30:26] Tamaño final: 85.2 KB
[14:30:26] Tiempo procesamiento: 12 ms

========================================
[14:30:26] Proceso completado: EXITOSO
Tiempo total: 12 ms
========================================
```

### Operaciones Registradas

- **COMPRESS**: Compresión de archivos
- **DECOMPRESS**: Descompresión de archivos
- **ENCRYPT**: Encriptación de archivos
- **DECRYPT**: Desencriptación de archivos
- **Combinaciones**: COMPRESS_ENCRYPT, DECOMPRESS_DECRYPT, etc.

### Ubicación

Todos los journals se guardan en `journal/` (ignorado por git según `.gitignore`)

### Ventajas

1. **Trazabilidad**: Registro completo de todas las operaciones
2. **Debugging**: Facilita la identificación de problemas
3. **Auditoría**: Permite revisar qué se procesó y cuándo
4. **Thread-safe**: Funciona correctamente en procesamiento concurrente
5. **Educativo**: Simula sistemas de journaling reales en sistemas operativos

## Casos de Prueba

El directorio `tests/` contiene archivos de prueba para validar las diferentes funcionalidades.

### Compresión y Descompresión

Usa `tests/Test1C.txt` para probar algoritmos de compresión:

**RLE:**
```bash
# Comprimir
./bin/FileUtility -c -i tests/Test1C.txt -o tests/TestComp.dat --comp-alg RLE
# Descomprimir
./bin/FileUtility -d -i tests/TestComp.dat -o tests/TestDecomp.txt --comp-alg RLE
```

**LZW:**
```bash
# Comprimir
./bin/FileUtility -c -i tests/Test1C.txt -o tests/TestCompLZW.dat --comp-alg LZW
# Descomprimir
./bin/FileUtility -d -i tests/TestCompLZW.dat -o tests/TestDecompLZW.txt --comp-alg LZW
```

**Huffman:**
```bash
# Comprimir
./bin/FileUtility -c -i tests/Test1C.txt -o tests/TestCompHuff.dat --comp-alg Huff
# Descomprimir
./bin/FileUtility -d -i tests/TestCompHuff.dat -o tests/TestDecompHuff.txt --comp-alg Huff
```

### Encriptación y Desencriptación

Usa `tests/Test1E.txt` para probar algoritmos de encriptación:

**Vigenere:**
```bash
# Encriptar
./bin/FileUtility -e -i tests/Test1E.txt -o tests/TestEnc.dat --enc-alg VIG -k "Encrypt3*PassK3y@"
# Desencriptar
./bin/FileUtility -u -i tests/TestEnc.dat -o tests/TestDec.txt --enc-alg VIG -k "Encrypt3*PassK3y@"
```

**AES-128:**
```bash
# Encriptar
./bin/FileUtility -e -i tests/Test1E.txt -o tests/TestEncAES.dat --enc-alg AES -k "Encrypt3*PassK3y@"
# Desencriptar
./bin/FileUtility -u -i tests/TestEncAES.dat -o tests/TestDecAES.txt --enc-alg AES -k "Encrypt3*PassK3y@"
```

### Operaciones Combinadas

```bash
# Comprimir + Encriptar
./bin/FileUtility -ce -i tests/Test1C.txt -o tests/out.dat --comp-alg LZW --enc-alg AES128 -k "Encrypt3*PassK3y@"

# Desencriptar + Descomprimir
./bin/FileUtility -ud -i tests/out.dat -o tests/recovered.txt --enc-alg AES128 --comp-alg LZW -k "Encrypt3*PassK3y@"
```

### Procesamiento de Carpetas

```bash
# Comprimir toda la carpeta tests/
./bin/FileUtility -c -i tests/ -o testsOut --comp-alg Huff
```

---

## Notas Importantes

- Las claves de encriptación deben ser seguras (mínimo 8 caracteres, se recomienda 16+ para AES)
- El programa valida la complejidad de las claves antes de procesar
- Para operaciones combinadas, el orden de descompresión/desencriptación debe invertirse
- Los journals se generan automáticamente en `journal/` y no se incluyen en git
- El procesamiento concurrente se adapta automáticamente al número de núcleos disponibles