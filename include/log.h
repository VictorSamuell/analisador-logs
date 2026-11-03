#ifndef LOG_H
#define LOG_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* Tamanho máximo do buffer usado para ler uma linha do log. */
#define LINE_CAP 1024
/* Um registro guarda data, deslocamento original e os dois campos de texto. */
typedef struct {
    uint64_t timestamp;
    long offset;
    char categoria[32];
    char evento[256];
} Registro;
/* O log mantém o vetor carregado e as quantidades válidas e inválidas. */
typedef struct { Registro *itens; size_t total; size_t invalidos; } Log;
/* Converte e valida uma data no formato AAAA-MM-DD HH:MM:SS. */
int timestamp_parse(const char *texto, uint64_t *valor);
/* Retorna texto vazio se o valor exceder os 14 digitos do formato. */
void timestamp_format(uint64_t valor, char texto[20]);
/* Divide uma linha nos campos e preenche a estrutura Registro. */
int registro_parse(char *linha, Registro *registro);
/* 1: linha completa, 0: EOF, -1: linha longa, -2: erro de leitura. */
int linha_ler(FILE *arquivo, char *linha, size_t capacidade);
/* Carrega do arquivo as linhas válidas e guarda seus deslocamentos. */
int log_carregar(const char *caminho, Log *log);
/* Libera os registros alocados pelo carregamento. */
void log_liberar(Log *log);
/* Imprime um registro no formato de linha do arquivo. */
void registro_imprimir(FILE *saida, const Registro *registro);
#endif
