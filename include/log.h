#ifndef LOG_H
#define LOG_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define LINE_CAP 1024
typedef struct {
    uint64_t timestamp;
    long offset;
    char categoria[32];
    char evento[256];
} Registro;
typedef struct { Registro *itens; size_t total; size_t invalidos; } Log;
int timestamp_parse(const char *texto, uint64_t *valor);
void timestamp_format(uint64_t valor, char texto[20]);
int registro_parse(char *linha, Registro *registro);
/* 1: linha completa, 0: EOF, -1: linha longa, -2: erro de leitura. */
int linha_ler(FILE *arquivo, char *linha, size_t capacidade);
int log_carregar(const char *caminho, Log *log);
void log_liberar(Log *log);
void registro_imprimir(FILE *saida, const Registro *registro);
#endif
