#ifndef INDICE_H
#define INDICE_H
#include "log.h"
/* Mantém abertos o índice binário e o log original consultado. */
typedef struct { FILE *binario; FILE *texto; uint64_t total; } Indice;
/* Cria um índice binário para os registros ordenados do log. */
int indice_criar(const char *binario, const char *texto, const Log *log);
/* Abre o índice depois de validar seu formato e o arquivo de origem. */
int indice_abrir(Indice *indice, const char *binario, const char *texto);
/* Fecha os arquivos e limpa os dados do índice. */
void indice_fechar(Indice *indice);
/* Imprime registros do intervalo sem carregar o vetor completo do log. */
int indice_consultar(Indice *indice, uint64_t inicio, uint64_t fim, FILE *saida, size_t *total);
#endif
