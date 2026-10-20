#ifndef RELATORIO_H
#define RELATORIO_H
#include "log.h"
int relatorio_gerar(FILE *saida, const Log *log, uint64_t inicio, uint64_t fim);
int relatorio_salvar(const char *caminho, const Log *log);
#endif
