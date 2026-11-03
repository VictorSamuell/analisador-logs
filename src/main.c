/* Interfaces dos modulos usados pelo menu principal. */
#include "log.h"
#include "ordenacao.h"
#include "relatorio.h"
#include "indice.h"
#include <stdlib.h>
#include <string.h>

static int perguntar(const char *rotulo,char *valor,size_t cap) {
    /* Mostra a pergunta e le uma linha do teclado. */
    int status; printf("%s",rotulo); fflush(stdout); status=linha_ler(stdin,valor,cap);
    if(status<0) puts("Entrada invalida ou longa demais.");
    return status;
}
static int intervalo(uint64_t *inicio,uint64_t *fim) {
    /* Lê e valida as duas datas que limitam a consulta. */
    char a[64],b[64];
    if(perguntar("Inicio (AAAA-MM-DD HH:MM:SS): ",a,sizeof a)!=1 ||
       perguntar("Fim    (AAAA-MM-DD HH:MM:SS): ",b,sizeof b)!=1) return 0;
    if(!timestamp_parse(a,inicio) || !timestamp_parse(b,fim) || *inicio>*fim) {
        puts("Intervalo invalido."); return 0;
    }
    return 1;
}
static int carregar(const char *path,Log *log) {
    /* Lê os registros, ordena por data e troca os dados antigos. */
    Log novo={0};
    if(!log_carregar(path,&novo) || !mergesort_registros(novo.itens,novo.total)) {
        log_liberar(&novo); puts("Falha ao ler arquivo ou alocar memoria."); return 0;
    }
    log_liberar(log); *log=novo;
    printf("Carregados: %zu | Invalidos ignorados: %zu\n",log->total,log->invalidos); return 1;
}
int main(int argc,char **argv) {
    /* Mantém os dados carregados, o índice aberto e os caminhos dos arquivos. */
    Log log={0}; Indice idx={0}; char origem[512]="",binario[520],relatorio[528],op[32]; int carregado=0,status;
    /* Aceita no máximo um caminho de log como argumento. */
    if(argc>2) { puts("Uso: analisador [arquivo.log]"); return EXIT_FAILURE; }
    if(argc==2) {
        if(strlen(argv[1])>=sizeof origem) { puts("Caminho longo demais."); return EXIT_FAILURE; }
        strcpy(origem,argv[1]);
    }
    /* Repete o menu até o usuário escolher sair ou encerrar a entrada. */
    for(;;) {
        uint64_t inicio,fim; size_t i,total;
        puts("\nT7 - Analisador de logs\n1. Carregar log e gerar indice\n2. Listar em ordem cronologica\n3. Relatorio completo\n4. Relatorio por intervalo\n5. Consultar indice persistido\n6. Exportar relatorio em texto\n0. Sair");
        status=perguntar("Opcao: ",op,sizeof op);
        if(status==0 || status==-2 || (status==1 && strcmp(op,"0")==0)) break;
        if(status!=1) continue;
        /* As opções 1 e 5 precisam conhecer o caminho do arquivo de origem. */
        if(strcmp(op,"1")==0 || strcmp(op,"5")==0) {
            if(!*origem && perguntar("Arquivo de log: ",origem,sizeof origem)!=1) { origem[0]='\0'; continue; }
            if(!*origem) { puts("Informe um caminho."); continue; }
            (void)snprintf(binario,sizeof binario,"%s.idx",origem);
        }
        /* Carrega o log e cria o índice binário para consultas futuras. */
        if(strcmp(op,"1")==0) {
            indice_fechar(&idx);
            if(!(carregado=carregar(origem,&log))) { origem[0]='\0'; continue; }
            puts(indice_criar(binario,origem,&log) ? "Indice binario salvo." : "Falha ao salvar indice.");
        /* Abre o índice salvo e consulta somente o intervalo solicitado. */
        } else if(strcmp(op,"5")==0) {
            if(!idx.binario && !indice_abrir(&idx,binario,origem)) {
                puts("Indice ausente, invalido ou desatualizado. Use a opcao 1."); continue;
            }
            if(intervalo(&inicio,&fim)) {
                if(indice_consultar(&idx,inicio,fim,stdout,&total)) printf("Encontrados: %zu\n",total);
                else { puts("Falha na consulta. Recarregue o log."); indice_fechar(&idx); }
            }
        /* As outras operações usam o log já carregado na memória. */
        } else if(strcmp(op,"2")==0 || strcmp(op,"3")==0 || strcmp(op,"4")==0 || strcmp(op,"6")==0) {
            if(!carregado) { puts("Carregue o log com a opcao 1."); continue; }
            if(strcmp(op,"2")==0) for(i=0;i<log.total;++i) registro_imprimir(stdout,&log.itens[i]);
            else if(strcmp(op,"3")==0) { if(!relatorio_gerar(stdout,&log,0,UINT64_MAX)) puts("Falha no relatorio."); }
            else if(strcmp(op,"4")==0) {
                if(intervalo(&inicio,&fim) && !relatorio_gerar(stdout,&log,inicio,fim)) puts("Falha no relatorio.");
            } else {
                (void)snprintf(relatorio,sizeof relatorio,"%s.relatorio.txt",origem);
                if(relatorio_salvar(relatorio,&log)) printf("Salvo: %s\n",relatorio); else puts("Falha ao exportar.");
            }
        } else puts("Opcao invalida.");
    }
    /* Fecha arquivos e libera a memória antes de encerrar. */
    indice_fechar(&idx); log_liberar(&log); return EXIT_SUCCESS;
}
