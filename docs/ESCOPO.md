# Escopo do projeto T7

**Tema:** Analisador de arquivos de log. **Linguagem:** C11. **Integrantes e RAs:** preencher pelo grupo. **Responsabilidades:** atribuir os quatro módulos aos integrantes antes da entrega.

O programa lê um log textual, valida seus registros, organiza-os cronologicamente e apresenta contagens por categoria, faixa horária e ranking de eventos. Permite filtrar por intervalo de data/hora, exportar relatório e consultar um índice binário persistente após encerrar e reabrir o programa.

O escopo inclui uma interface de terminal, tratamento de opções e datas inválidas, falha de abertura, alocação e leitura/escrita, além da contagem de linhas rejeitadas. Não inclui interface gráfica, acompanhamento em tempo real, expressões regulares ou múltiplos formatos de log.

## Registros principais

```c
typedef struct {
    uint64_t timestamp; /* AAAAMMDDHHMMSS */
    long offset;        /* posição da linha na origem */
    char categoria[32];
    char evento[256];
} Registro;
typedef struct {
    Registro *itens;
    size_t total;
    size_t invalidos;
} Log;
typedef struct {
    FILE *binario;
    FILE *texto;
    uint64_t total;
} Indice;
```

## Módulos e interfaces públicas

`main.c` controla o menu, mantém os objetos `Log` e `Indice` locais e libera os recursos ao sair. As interfaces abaixo estão declaradas nos cabeçalhos correspondentes com guardas de inclusão.

```c
/* log.h: leitura, validação e registros */
int timestamp_parse(const char *texto, uint64_t *valor);
void timestamp_format(uint64_t valor, char texto[20]);
int registro_parse(char *linha, Registro *registro);
int linha_ler(FILE *arquivo, char *linha, size_t capacidade);
int log_carregar(const char *caminho, Log *log);
void log_liberar(Log *log);
void registro_imprimir(FILE *saida, const Registro *registro);

/* ordenacao.h: ordenação cronológica */
int mergesort_registros(Registro *itens, size_t total);

/* relatorio.h: estatísticas e exportação */
int relatorio_gerar(FILE *saida, const Log *log,
                   uint64_t inicio, uint64_t fim);
int relatorio_salvar(const char *caminho, const Log *log);

/* indice.h: persistência e acesso direto */
int indice_criar(const char *binario, const char *texto, const Log *log);
int indice_abrir(Indice *indice, const char *binario, const char *texto);
void indice_fechar(Indice *indice);
int indice_consultar(Indice *indice, uint64_t inicio, uint64_t fim,
                     FILE *saida, size_t *total);
```

## Persistência e recursividade

A entrada é `AAAA-MM-DD HH:MM:SS|CATEGORIA|Evento`, uma linha por registro. O relatório é salvo em texto. O índice possui cabeçalho de 32 bytes e entradas de 16 bytes contendo marca de tempo e deslocamento da linha original; campos numéricos são gravados explicitamente em little-endian. A consulta usa busca binária em disco e `fseek` no log.

O mergesort divide o intervalo em duas metades, ordena cada metade recursivamente e intercala os resultados. O caso base contém zero ou um registro. A recursão foi escolhida porque expressa diretamente a decomposição do problema e tem profundidade O(log n). Uma versão iterativa evitaria a pilha de chamadas, mas precisaria controlar manualmente o tamanho e os limites dos blocos. Ambas têm tempo O(n log n) e memória auxiliar O(n). O desempate pela metade esquerda mantém a ordem original dos eventos simultâneos.

## Critérios de aceite

O exemplo deve produzir 8 registros válidos, INFO=4, ERROR=2 e WARN=2; o evento mais frequente deve ser “Login realizado”, com 3 ocorrências. O intervalo exato de 09:30:00 deve retornar dois registros mesmo após reiniciar o programa. Entradas inválidas e índice incompatível devem ser recusados com mensagem. A compilação deve passar sem avisos e os testes automatizados devem concluir com sucesso.

Este arquivo é a base textual do escopo. Para a entrega E1 em 1–2 páginas, o grupo deve inserir sua identificação e conferir a paginação na ferramenta usada para impressão.
