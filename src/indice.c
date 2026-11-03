#include "indice.h"
#include <limits.h>
#include <string.h>
/* Grava um inteiro em little-endian sem depender do formato da máquina. */
static int gravar_u64(FILE *f,uint64_t n) {
    unsigned i; for(i=0;i<8;++i) if(fputc((int)((n>>(8*i))&255),f)==EOF) return 0;
    return 1;
}
static int ler_u64(FILE *f,uint64_t *n) {
    /* Lê os oito bytes little-endian e reconstrói o inteiro. */
    unsigned i; int c; *n=0;
    for(i=0;i<8;++i) { c=fgetc(f); if(c==EOF) return 0; *n|=(uint64_t)(unsigned)c<<(8*i); }
    return 1;
}
static int assinatura(FILE *f,uint64_t *tamanho,uint64_t *hash) {
    /* Calcula tamanho e hash do log para detectar mudanças na origem. */
    int c; *tamanho=0; *hash=UINT64_C(14695981039346656037);
    if(fseek(f,0,SEEK_SET)!=0) return 0;
    while((c=fgetc(f))!=EOF) { ++*tamanho; *hash^=(unsigned)c; *hash*=UINT64_C(1099511628211); }
    return !ferror(f) && *tamanho<=(uint64_t)LONG_MAX;
}
static int entrada(FILE *f,uint64_t pos,uint64_t *tempo,uint64_t *offset) {
    /* Busca diretamente no índice a entrada da posição informada. */
    if(pos>((uint64_t)LONG_MAX-32)/16 || fseek(f,(long)(32+16*pos),SEEK_SET)!=0) return 0;
    return ler_u64(f,tempo) && ler_u64(f,offset);
}
static int registro_no_offset(FILE *f,uint64_t off,Registro *r) {
    /* Volta ao log original e lê o registro indicado pelo deslocamento. */
    char s[LINE_CAP];
    if(off>(uint64_t)LONG_MAX || fseek(f,(long)off,SEEK_SET)!=0) return 0;
    return linha_ler(f,s,sizeof s)==1 && registro_parse(s,r);
}
int indice_criar(const char *bin,const char *txt,const Log *log) {
    /* Valida os dados e grava o índice ordenado associado ao log. */
    FILE *origem,*f; uint64_t tamanho,hash; size_t i; int ok;
    if(strcmp(bin,txt)==0 || log->total>((uint64_t)LONG_MAX-32)/16) return 0;
    origem=fopen(txt,"rb"); if(!origem) return 0;
    ok=assinatura(origem,&tamanho,&hash);
    /* Confere que o vetor ainda corresponde ao arquivo de origem. */
    /* Confere que cada registro ainda existe na posição guardada. */
    for(i=0;ok && i<log->total;++i) {
        Registro r; const Registro *v=&log->itens[i];
        ok=v->offset>=0 && registro_no_offset(origem,(uint64_t)v->offset,&r) &&
            r.timestamp==v->timestamp && strcmp(r.categoria,v->categoria)==0 && strcmp(r.evento,v->evento)==0;
        if(i && log->itens[i-1].timestamp>v->timestamp) ok=0;
    }
    fclose(origem); if(!ok) return 0;
    f=fopen(bin,"wb"); if(!f) return 0;
    /* Escreve assinatura, quantidade, tamanho, hash e entradas do índice. */
    ok=fwrite("T7IDX001",1,8,f)==8 && gravar_u64(f,(uint64_t)log->total) && gravar_u64(f,tamanho) && gravar_u64(f,hash);
    for(i=0;ok && i<log->total;++i) ok=gravar_u64(f,log->itens[i].timestamp) && gravar_u64(f,(uint64_t)log->itens[i].offset);
    if(fclose(f)!=0) ok=0;
    return ok;
}
void indice_fechar(Indice *idx) {
    /* Fecha os arquivos associados ao índice e limpa a estrutura. */
    if(idx->binario) fclose(idx->binario);
    if(idx->texto) fclose(idx->texto);
    memset(idx,0,sizeof *idx);
}
int indice_abrir(Indice *dest,const char *bin,const char *txt) {
    /* Confere formato, integridade e correspondência antes de aceitar o índice. */
    Indice idx={0}; char magic[8]; uint64_t tamanho,hash,real,hreal,i,anterior=0,offanterior=0;
    long bytes;
    idx.binario=fopen(bin,"rb"); idx.texto=fopen(txt,"rb");
    if(!idx.binario || !idx.texto) goto falha;
    if(fread(magic,1,8,idx.binario)!=8 || memcmp(magic,"T7IDX001",8)!=0 ||
       !ler_u64(idx.binario,&idx.total) || !ler_u64(idx.binario,&tamanho) || !ler_u64(idx.binario,&hash)) goto falha;
    if(idx.total>((uint64_t)LONG_MAX-32)/16 || fseek(idx.binario,0,SEEK_END)!=0) goto falha;
    bytes=ftell(idx.binario);
    if(bytes<0 || (uint64_t)bytes!=32+16*idx.total || !assinatura(idx.texto,&real,&hreal) || tamanho!=real || hash!=hreal) goto falha;
    /* Verifica ordenação e confirma cada entrada com o log original. */
    for(i=0;i<idx.total;++i) {
        uint64_t tempo,off; Registro r;
        if(!entrada(idx.binario,i,&tempo,&off) || off>=tamanho ||
           (i && (tempo<anterior || (tempo==anterior && off<=offanterior))) ||
           !registro_no_offset(idx.texto,off,&r) || r.timestamp!=tempo) goto falha;
        anterior=tempo; offanterior=off;
    }
    indice_fechar(dest); *dest=idx; return 1;
falha:
    indice_fechar(&idx); return 0;
}
int indice_consultar(Indice *idx,uint64_t inicio,uint64_t fim,FILE *saida,size_t *total) {
    /* Localiza o início do intervalo e imprime as entradas encontradas. */
    uint64_t lo=0,hi=idx->total,t,off; *total=0;
    if(!idx->binario || !idx->texto || inicio>fim) return 0;
    /* lower_bound em disco: cada leitura usa fseek(32 + posicao * 16). */
    /* Busca binária a primeira entrada cuja data seja maior ou igual ao início. */
    while(lo<hi) {
        uint64_t meio=lo+(hi-lo)/2;
        if(!entrada(idx->binario,meio,&t,&off)) return 0;
        if(t<inicio) lo=meio+1; else hi=meio;
    }
    /* Percorre as entradas até ultrapassar a data final. */
    while(lo<idx->total) {
        Registro r;
        if(!entrada(idx->binario,lo++,&t,&off)) return 0;
        if(t>fim) break;
        if(!registro_no_offset(idx->texto,off,&r) || r.timestamp!=t) return 0;
        registro_imprimir(saida,&r); ++*total;
    }
    return !ferror(saida);
}
