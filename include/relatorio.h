#ifndef RELATORIO_H
#define RELATORIO_H
#include "log.h"
/* Gera no fluxo informado as estatísticas do intervalo inclusivo. */
int relatorio_gerar(FILE *saida, const Log *log, uint64_t inicio, uint64_t fim);
/* Salva em arquivo o relatório completo do log. */
int relatorio_salvar(const char *caminho, const Log *log);
#endif
