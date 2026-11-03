#include "log.h"
#include <stdlib.h>
#include <string.h>

/* Converte a data e hora para número e rejeita valores fora do calendário. */
int timestamp_parse(const char *s, uint64_t *valor) {
    int i, ano, mes, dia, hora, minuto, segundo, maxdia;
    const int dias[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint64_t n = 0;
    if (strlen(s) != 19) return 0;
    /* Confere separadores e garante que as demais posições sejam dígitos. */
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
    /* Calcula os dias do mês, incluindo a regra completa de ano bissexto. */
    maxdia = dias[mes-1] + (mes == 2 && ano%4 == 0 && (ano%100 != 0 || ano%400 == 0));
    if (dia < 1 || dia > maxdia || hora > 23 || minuto > 59 || segundo > 59) return 0;
    *valor = n; return 1;
}
/* Formata o número de volta para AAAA-MM-DD HH:MM:SS. */
void timestamp_format(uint64_t n, char s[20]) {
    unsigned seg=(unsigned)(n%100), min, hora, dia, mes, ano;
    /* Nao permite que um ano com mais de quatro digitos seja truncado. */
    if (n > UINT64_C(99999999999999)) { s[0]='\0'; return; }
    n/=100; min=(unsigned)(n%100); n/=100; hora=(unsigned)(n%100);
    n/=100; dia=(unsigned)(n%100); n/=100; mes=(unsigned)(n%100); ano=(unsigned)((n/100)%10000);
    /* Escreve cada dígito diretamente para garantir exatamente 19 caracteres. */
    s[0]=(char)('0'+ano/1000); s[1]=(char)('0'+(ano/100)%10);
    s[2]=(char)('0'+(ano/10)%10); s[3]=(char)('0'+ano%10); s[4]='-';
    s[5]=(char)('0'+mes/10); s[6]=(char)('0'+mes%10); s[7]='-';
    s[8]=(char)('0'+dia/10); s[9]=(char)('0'+dia%10); s[10]=' ';
    s[11]=(char)('0'+hora/10); s[12]=(char)('0'+hora%10); s[13]=':';
    s[14]=(char)('0'+min/10); s[15]=(char)('0'+min%10); s[16]=':';
    s[17]=(char)('0'+seg/10); s[18]=(char)('0'+seg%10); s[19]='\0';
}
/* Lê uma linha limitada e informa EOF, linha longa ou falha de leitura. */
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
/* Separa data, categoria e evento e valida os limites dos campos. */
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
/* Carrega linhas válidas e guarda o deslocamento de cada uma no arquivo. */
int log_carregar(const char *caminho, Log *destino) {
    FILE *f=fopen(caminho,"rb"); Log novo={0}; size_t cap=0; char linha[LINE_CAP]; int status;
    if (!f) return 0;
    /* Percorre o arquivo; linhas inválidas são contadas e ignoradas. */
    for (;;) {
        Registro r; r.offset=ftell(f);
        if (r.offset<0) goto falha;
        status=linha_ler(f,linha,sizeof linha);
        if (status==0) break;
        if (status==-2) goto falha;
        if (status==-1 || !registro_parse(linha,&r)) { ++novo.invalidos; continue; }
        /* Aumenta o vetor quando não há mais espaço para registros. */
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
/* Libera o vetor e zera os campos para evitar reutilizar dados antigos. */
void log_liberar(Log *log) { free(log->itens); memset(log,0,sizeof *log); }
/* Imprime um registro no mesmo formato usado no arquivo de entrada. */
void registro_imprimir(FILE *f, const Registro *r) {
    char s[20]; timestamp_format(r->timestamp,s); fprintf(f,"%s|%s|%s\n",s,r->categoria,r->evento);
}
