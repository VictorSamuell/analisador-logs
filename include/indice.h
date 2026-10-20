#ifndef INDICE_H
#define INDICE_H
#include "log.h"
typedef struct { FILE *binario; FILE *texto; uint64_t total; } Indice;
int indice_criar(const char *binario, const char *texto, const Log *log);
int indice_abrir(Indice *indice, const char *binario, const char *texto);
void indice_fechar(Indice *indice);
int indice_consultar(Indice *indice, uint64_t inicio, uint64_t fim, FILE *saida, size_t *total);
#endif
