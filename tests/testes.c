#include "log.h"
#include "ordenacao.h"
#include "relatorio.h"
#include "indice.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint64_t data(const char *s) { uint64_t t; assert(timestamp_parse(s,&t)); return t; }
static void escrever(const char *s) {
    FILE *f=fopen("build/teste.log","wb"); assert(f); assert(fputs(s,f)>=0); assert(fclose(f)==0);
}
static void contem(FILE *f,const char *esperado) {
    char buf[8192]; size_t n; rewind(f); n=fread(buf,1,sizeof buf-1,f); buf[n]='\0'; assert(strstr(buf,esperado));
}
int main(void) {
    Log log={0}; Indice idx={0}; uint64_t t; size_t n,i; FILE *f,*saida;
    char s[20];
    timestamp_format(data("0001-01-01 00:00:00"),s); assert(strcmp(s,"0001-01-01 00:00:00")==0);
    timestamp_format(data("9999-12-31 23:59:59"),s); assert(strcmp(s,"9999-12-31 23:59:59")==0);
    timestamp_format(UINT64_C(100000000000000),s); assert(s[0]=='\0');
    timestamp_format(UINT64_MAX,s); assert(s[0]=='\0');
    assert(timestamp_parse("2024-02-29 23:59:59",&t)); timestamp_format(t,s); assert(strcmp(s,"2024-02-29 23:59:59")==0);
    assert(!timestamp_parse("2026-02-29 12:00:00",&t)); assert(!timestamp_parse("1900-02-29 12:00:00",&t));
    assert(timestamp_parse("2000-02-29 12:00:00",&t)); assert(!timestamp_parse("2026-04-31 12:00:00",&t));
    assert(!timestamp_parse("2026-01-01 24:00:00",&t)); assert(!timestamp_parse("2026-1-01 12:00:00",&t));
    assert(!log_carregar("build/arquivo_inexistente.log",&log));
    escrever("2026-10-19 10:00:00|ERROR|Falha\r\n2026-10-19 08:00:00|INFO|Login\r\ninvalida\n2026-10-19 08:00:00|WARN|Aviso\n2026-02-30 12:00:00|INFO|Data ruim\n2026-10-19 08:30:00|INFO|Login");
    assert(log_carregar("build/teste.log",&log)); assert(log.total==4 && log.invalidos==2);
    assert(mergesort_registros(log.itens,log.total));
    assert(strcmp(log.itens[0].categoria,"INFO")==0 && strcmp(log.itens[1].categoria,"WARN")==0);
    assert(indice_criar("build/teste.idx","build/teste.log",&log));
    log_liberar(&log); /* Consulta funciona sem carregar o vetor. */
    assert(indice_abrir(&idx,"build/teste.idx","build/teste.log"));
    saida=tmpfile(); assert(saida);
    t=data("2026-10-19 08:00:00");
    assert(indice_consultar(&idx,t,t,saida,&n) && n==2); contem(saida,"|WARN|Aviso");
    assert(indice_consultar(&idx,0,UINT64_MAX,saida,&n) && n==4);
    assert(indice_consultar(&idx,data("2027-01-01 00:00:00"),UINT64_MAX,saida,&n) && n==0);
    assert(indice_consultar(&idx,0,1,saida,&n) && n==0);
    assert(!indice_consultar(&idx,2,1,saida,&n)); fclose(saida); indice_fechar(&idx);
    assert(log_carregar("build/teste.log",&log)); saida=tmpfile(); assert(saida);
    assert(relatorio_gerar(saida,&log,0,UINT64_MAX)); contem(saida,"INFO: 2"); contem(saida,"1. Login: 2");
    contem(saida,"08:00-08:59: 3"); fclose(saida);
    saida=tmpfile(); assert(saida); assert(relatorio_gerar(saida,&log,t,t)); contem(saida,"Total no intervalo: 2"); fclose(saida);
    f=fopen("build/teste.idx","r+b"); assert(f); assert(fseek(f,32,SEEK_SET)==0); assert(fputc(0,f)!=EOF); fclose(f);
    assert(!indice_abrir(&idx,"build/teste.idx","build/teste.log"));
    assert(mergesort_registros(log.itens,log.total)); assert(indice_criar("build/teste.idx","build/teste.log",&log));
    f=fopen("build/teste.log","ab"); assert(f); fputs("\n",f); fclose(f);
    assert(!indice_abrir(&idx,"build/teste.idx","build/teste.log")); log_liberar(&log);
    escrever(""); assert(log_carregar("build/teste.log",&log) && log.total==0);
    assert(mergesort_registros(log.itens,log.total)); assert(indice_criar("build/teste.idx","build/teste.log",&log));
    assert(indice_abrir(&idx,"build/teste.idx","build/teste.log")); saida=tmpfile(); assert(saida);
    assert(indice_consultar(&idx,0,UINT64_MAX,saida,&n) && n==0); fclose(saida); indice_fechar(&idx);
    f=fopen("build/teste.idx","wb"); assert(f); fputs("T7IDX001",f); fclose(f);
    assert(!indice_abrir(&idx,"build/teste.idx","build/teste.log"));
    f=fopen("build/teste.log","wb"); assert(f); for(i=0;i<2000;++i) fputc('x',f); fputs("\n2026-10-19 12:00:00|INFO|OK\n",f); fclose(f);
    assert(log_carregar("build/teste.log",&log) && log.total==1 && log.invalidos==1); log_liberar(&log);
    /* Vetor maior, desordenado e com empates. */
    log.total=2000; log.itens=calloc(log.total,sizeof *log.itens); assert(log.itens);
    for(i=0;i<log.total;++i) { log.itens[i].timestamp=(uint64_t)((1999-i)%37); log.itens[i].offset=(long)i; }
    assert(mergesort_registros(log.itens,log.total));
    for(i=1;i<log.total;++i) { assert(log.itens[i-1].timestamp<=log.itens[i].timestamp); if(log.itens[i-1].timestamp==log.itens[i].timestamp) assert(log.itens[i-1].offset<log.itens[i].offset); }
    log_liberar(&log);
    remove("build/teste.log"); remove("build/teste.idx");
    puts("Todos os testes passaram."); return 0;
}
