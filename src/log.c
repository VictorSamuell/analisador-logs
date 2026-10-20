#include "log.h"
#include <stdlib.h>
#include <string.h>

int timestamp_parse(const char *s, uint64_t *valor) {
    int i, ano, mes, dia, hora, minuto, segundo, maxdia;
    const int dias[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint64_t n = 0;
    if (strlen(s) != 19) return 0;
    for (i = 0; i < 19; ++i) {
        if (i == 4 || i == 7) { if (s[i] != '-') return 0; }
        else if (i == 10) { if (s[i] != ' ') return 0; }
        else if (i == 13 || i == 16) { if (s[i] != ':') return 0; }
        else { if (s[i] < '0' || s[i] > '9') return 0; n = n*10 + (unsigned)(s[i]-'0'); }
    }
    ano = (s[0]-'0')*1000+(s[1]-'0')*100+(s[2]-'0')*10+s[3]-'0';
    mes = (s[5]-'0')*10+s[6]-'0'; dia = (s[8]-'0')*10+s[9]-'0';
    hora = (s[11]-'0')*10+s[12]-'0'; minuto = (s[14]-'0')*10+s[15]-'0';
    segundo = (s[17]-'0')*10+s[18]-'0';
    if (ano < 1 || mes < 1 || mes > 12) return 0;
    maxdia = dias[mes-1] + (mes == 2 && ano%4 == 0 && (ano%100 != 0 || ano%400 == 0));
    if (dia < 1 || dia > maxdia || hora > 23 || minuto > 59 || segundo > 59) return 0;
    *valor = n; return 1;
}
void timestamp_format(uint64_t n, char s[20]) {
    unsigned seg=(unsigned)(n%100), min, hora, dia, mes, ano;
    n/=100; min=(unsigned)(n%100); n/=100; hora=(unsigned)(n%100);
    n/=100; dia=(unsigned)(n%100); n/=100; mes=(unsigned)(n%100); ano=(unsigned)(n/100);
    (void)snprintf(s,20,"%04u-%02u-%02u %02u:%02u:%02u",ano,mes,dia,hora,min,seg);
}
int linha_ler(FILE *f, char *s, size_t cap) {
    size_t n=0; int c, longa=0;
    while ((c=fgetc(f)) != EOF && c != '\n') {
        if (n+1 < cap) s[n++]=(char)c; else longa=1;
        if (c == 0) longa=1;
    }
    s[n]='\0';
    if (ferror(f)) return -2;
    if (longa) return -1;
    if (n && s[n-1]=='\r') s[--n]='\0';
    return c==EOF && n==0 ? 0 : 1;
}
int registro_parse(char *s, Registro *r) {
    char *a=strchr(s,'|'), *b; size_t nc, ne;
    if (!a) return 0;
    *a='\0'; b=strchr(a+1,'|');
    if (!b) return 0;
    *b='\0'; nc=strlen(a+1); ne=strlen(b+1);
    if (!nc || nc>=sizeof r->categoria || !ne || ne>=sizeof r->evento ||
        !timestamp_parse(s,&r->timestamp)) return 0;
    memcpy(r->categoria,a+1,nc+1); memcpy(r->evento,b+1,ne+1); return 1;
}
int log_carregar(const char *caminho, Log *destino) {
    FILE *f=fopen(caminho,"rb"); Log novo={0}; size_t cap=0; char linha[LINE_CAP]; int status;
    if (!f) return 0;
    for (;;) {
        Registro r; r.offset=ftell(f);
        if (r.offset<0) goto falha;
        status=linha_ler(f,linha,sizeof linha);
        if (status==0) break;
        if (status==-2) goto falha;
        if (status==-1 || !registro_parse(linha,&r)) { ++novo.invalidos; continue; }
        if (novo.total==cap) {
            size_t next=cap ? cap*2 : 64; Registro *p;
            if (next<cap || next>SIZE_MAX/sizeof *p) goto falha;
            p=realloc(novo.itens,next*sizeof *p); if (!p) goto falha;
            novo.itens=p; cap=next;
        }
        novo.itens[novo.total++]=r;
    }
    if (fclose(f)!=0) { log_liberar(&novo); return 0; }
    log_liberar(destino); *destino=novo; return 1;
falha:
    fclose(f); log_liberar(&novo); return 0;
}
void log_liberar(Log *log) { free(log->itens); memset(log,0,sizeof *log); }
void registro_imprimir(FILE *f, const Registro *r) {
    char s[20]; timestamp_format(r->timestamp,s); fprintf(f,"%s|%s|%s\n",s,r->categoria,r->evento);
}
