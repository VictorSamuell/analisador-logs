#include "log.h" // Traz struct Log, struct Registro, linha_ler(), timestamp_parse(), log_carregar(), etc. (inclui <stdio.h> e <stdint.h> por tabela)
#include "ordenacao.h" // mergesort do mergesort_registros()
#include "relatorio.h"// Traz relatorio_gerar() e relatorio_salvar()
#include "indice.h" // Traz struct Indice, indice_criar(), indice_abrir(), indice_consultar(), indice_fechar()
#include <stdlib.h>
#include <string.h>

/* Função para garantir que uma linha digitada pelo usuário no teclado (stdin) será lida corretamente e com segurança */
static int perguntar(const char *rotulo,char *valor,size_t cap) {
    int status; printf("%s",rotulo); fflush(stdout); status=linha_ler(stdin,valor,cap);
    if(status<0) puts("Entrada invalida ou longa demais.");
    return status;
}
/* Pede duas datas ao usuário, valida se existem no calendário e se Inicio <= Fim */
static int intervalo(uint64_t *inicio,uint64_t *fim) {
    char a[64],b[64];
    if(perguntar("Inicio (AAAA-MM-DD HH:MM:SS): ",a,sizeof a)!=1 ||
       perguntar("Fim    (AAAA-MM-DD HH:MM:SS): ",b,sizeof b)!=1) return 0;
    if(!timestamp_parse(a,inicio) || !timestamp_parse(b,fim) || *inicio>*fim) {
        puts("Intervalo invalido."); return 0;
    }
    return 1;
}
/* Carrega o arquivo .log para a RAM e já ordena tudo com Mergesort */
static int carregar(const char *path,Log *log) {
    Log novo={0};
    if(!log_carregar(path,&novo) || !mergesort_registros(novo.itens,novo.total)) {
        log_liberar(&novo); puts("Falha ao ler arquivo ou alocar memoria."); return 0;
    }
    log_liberar(log); *log=novo;
    printf("Carregados: %zu | Invalidos ignorados: %zu\n",log->total,log->invalidos); return 1;
}


int main(int argc,char **argv) {
    Log log={0}; Indice idx={0}; char origem[512]="",binario[520],relatorio[528],op[32]; int carregado=0,status;
    if(argc>2) { puts("Uso: analisador [arquivo.log]"); return EXIT_FAILURE; }
    if(argc==2) {
        if(strlen(argv[1])>=sizeof origem) { puts("Caminho longo demais."); return EXIT_FAILURE; }
        strcpy(origem,argv[1]);
    }
    for(;;) {
        uint64_t inicio,fim; size_t i,total;
        puts("\nT7 - Analisador de logs\n1. Carregar log e gerar indice\n2. Listar em ordem cronologica\n3. Relatorio completo\n4. Relatorio por intervalo\n5. Consultar indice persistido\n6. Exportar relatorio em texto\n0. Sair");
        status=perguntar("Opcao: ",op,sizeof op);
        if(status==0 || status==-2 || (status==1 && strcmp(op,"0")==0)) break;
        if(status!=1) continue;
        if(strcmp(op,"1")==0 || strcmp(op,"5")==0) {
            if(!*origem && perguntar("Arquivo de log: ",origem,sizeof origem)!=1) { origem[0]='\0'; continue; }
            if(!*origem) { puts("Informe um caminho."); continue; }
            (void)snprintf(binario,sizeof binario,"%s.idx",origem);
        }
        if(strcmp(op,"1")==0) {
            indice_fechar(&idx);
            if(!(carregado=carregar(origem,&log))) { origem[0]='\0'; continue; }
            puts(indice_criar(binario,origem,&log) ? "Indice binario salvo." : "Falha ao salvar indice.");
        } else if(strcmp(op,"5")==0) {
            if(!idx.binario && !indice_abrir(&idx,binario,origem)) {
                puts("Indice ausente, invalido ou desatualizado. Use a opcao 1."); continue;
            }
            if(intervalo(&inicio,&fim)) {
                if(indice_consultar(&idx,inicio,fim,stdout,&total)) printf("Encontrados: %zu\n",total);
                else { puts("Falha na consulta. Recarregue o log."); indice_fechar(&idx); }
            }
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
    indice_fechar(&idx); log_liberar(&log); return EXIT_SUCCESS;
}
