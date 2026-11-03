#include "relatorio.h"
#include <stdlib.h>
#include <string.h>
typedef struct { const char *nome; size_t quantidade; } Contagem;
/* Soma uma ocorrência ou cria a contagem do nome pela primeira vez. */
static void contar(Contagem *v, size_t *n, const char *nome) {
    size_t i;
    for(i=0;i<*n;++i) if(strcmp(v[i].nome,nome)==0) { ++v[i].quantidade; return; }
    v[*n].nome=nome; v[*n].quantidade=1; ++*n;
}
static int por_nome(const void *a,const void *b) {
    /* Ordena categorias em ordem alfabética. */
    return strcmp(((const Contagem *)a)->nome,((const Contagem *)b)->nome);
}
static int por_ranking(const void *a,const void *b) {
    /* Coloca eventos mais frequentes primeiro e desempata pelo nome. */
    const Contagem *x=a,*y=b;
    if(x->quantidade!=y->quantidade) return x->quantidade>y->quantidade ? -1:1;
    return strcmp(x->nome,y->nome);
}
int relatorio_gerar(FILE *f,const Log *log,uint64_t inicio,uint64_t fim) {
    /* Conta registros do intervalo e imprime estatísticas e ranking. */
    Contagem *cats=NULL,*eventos=NULL; size_t nc=0,ne=0,i,total=0,horas[24]={0};
    if(inicio>fim || log->total>SIZE_MAX/sizeof *cats) return 0;
    if(log->total) {
        cats=calloc(log->total,sizeof *cats); eventos=calloc(log->total,sizeof *eventos);
        if(!cats || !eventos) { free(cats); free(eventos); return 0; }
    }
    /* Ignora registros fora do intervalo e atualiza as contagens. */
    for(i=0;i<log->total;++i) {
        const Registro *r=&log->itens[i];
        if(r->timestamp<inicio || r->timestamp>fim) continue;
        contar(cats,&nc,r->categoria); contar(eventos,&ne,r->evento);
        ++horas[(r->timestamp/10000)%100]; ++total;
    }
    if(nc>1) qsort(cats,nc,sizeof *cats,por_nome);
    if(ne>1) qsort(eventos,ne,sizeof *eventos,por_ranking);
    fprintf(f,"Total no intervalo: %zu\nLinhas invalidas na origem: %zu\n\nCategorias\n",total,log->invalidos);
    for(i=0;i<nc;++i) fprintf(f,"%s: %zu\n",cats[i].nome,cats[i].quantidade);
    fprintf(f,"\nFaixas de horario (todos os dias do intervalo)\n");
    for(i=0;i<24;++i) fprintf(f,"%02zu:00-%02zu:59: %zu\n",i,i,horas[i]);
    fprintf(f,"\nRanking de eventos\n");
    for(i=0;i<ne;++i) fprintf(f,"%zu. %s: %zu\n",i+1,eventos[i].nome,eventos[i].quantidade);
    free(cats); free(eventos); return !ferror(f);
}
int relatorio_salvar(const char *path,const Log *log) {
    /* Gera o relatório completo em um arquivo de texto. */
    FILE *f=fopen(path,"w"); int ok;
    if(!f) return 0;
    ok=relatorio_gerar(f,log,0,UINT64_MAX);
    if(fclose(f)!=0) ok=0;
    return ok;
}
