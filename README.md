# T7 — Analisador de arquivos de log

Projeto em C11 para ler logs, ordenar registros por data/hora com **mergesort recursivo**, contar categorias e horários, produzir ranking de eventos e consultar um índice binário persistente. Interface de terminal, sem bibliotecas externas e sem variáveis globais.

## Executar no Windows

Com Visual Studio e as ferramentas de desenvolvimento em C instaladas, abra o **Developer Command Prompt**, entre nesta pasta e execute:

```bat
build.bat
build\analisador.exe exemplos\sistema.log
```

`build.bat` compila com avisos tratados como erros e executa os testes. Nesta entrega, `build/analisador.exe` também está disponível para execução no Windows x64. A pasta `build/` é ignorada pelo Git; outras pessoas devem compilar a partir dos fontes.

Com GCC/MinGW no Windows, crie a pasta `build` e compile:

```bat
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude src/main.c src/log.c src/ordenacao.c src/relatorio.c src/indice.c -o build/analisador.exe
```

## Linux ou macOS

Com compilador C e Make instalados:

```sh
make
make test
./build/analisador exemplos/sistema.log
```

Os fontes usam a biblioteca padrão de C11. A compilação e os testes desta entrega foram executados no Windows x64 com MSVC 2022; Linux/macOS não foram executados neste ambiente.

## Formato de entrada

Uma linha por registro:

```text
2026-10-19 08:00:00|INFO|Login realizado
```

- Data e hora: exatamente `AAAA-MM-DD HH:MM:SS`, ano entre 0001 e 9999, com validação de dias do mês e anos bissextos.
- Categoria: de 1 a 31 bytes. Exemplos: `INFO`, `WARN`, `ERROR`; outras categorias também são aceitas.
- Evento: de 1 a 255 bytes. O texto completo identifica o evento no ranking; pode conter `|` depois do segundo separador.
- Use texto UTF-8 sem BOM, com LF ou CRLF. Os limites são em bytes, não em caracteres. Categorias e eventos diferenciam maiúsculas/minúsculas; espaços são preservados.
- Linhas malformadas, vazias, com byte NUL ou acima do limite de leitura são ignoradas e contabilizadas. Uma última linha sem quebra também é aceita.
- Todos os horários são locais, sem conversão de fuso. Os dois limites de um intervalo são inclusivos. A contagem horária agrupa a mesma hora em todos os dias selecionados.

## Menu

| Opção | Operação |
| --- | --- |
| 1 | Carrega o texto, ordena e grava `<arquivo>.idx` |
| 2 | Lista os registros ordenados, preservando a ordem original em empates |
| 3 | Exibe total, categorias, 24 faixas horárias e ranking |
| 4 | Gera as mesmas estatísticas para um intervalo de datas/horas |
| 5 | Consulta o índice persistido por intervalo, sem carregar o vetor completo |
| 6 | Exporta o relatório completo para `<arquivo>.relatorio.txt` |
| 0 | Encerra e libera recursos |

O caminho pode ser passado como argumento, entre aspas se tiver espaços. Sem argumento, o programa pergunta pelo caminho ao usar 1 ou 5. Para analisar outro arquivo, reinicie com o novo caminho. Para usar 2, 3, 4 ou 6, carregue o log com 1 primeiro. A opção 5 pode ser a primeira operação de uma nova execução, desde que o índice já exista.

## Roteiro de demonstração com resultados esperados

1. Execute com `exemplos/sistema.log` e escolha **1**: `Carregados: 8 | Invalidos ignorados: 0` e índice salvo.
2. Escolha **2**: a primeira linha será `08:00:00`, a última `23:59:59`; no empate de `09:30:00`, `WARN` aparece antes de `INFO`.
3. Escolha **3**: `INFO: 4`, `WARN: 2`, `ERROR: 2`. As faixas 08h, 09h e 10h têm 2 registros cada; 11h e 23h têm 1 cada.
4. O ranking será `Login realizado: 3`, `Falha de conexao: 2`, `Memoria elevada: 2`, `Backup concluido: 1`. Empates usam ordem lexicográfica por bytes.
5. Escolha **4**, início `2026-10-19 09:00:00` e fim `2026-10-19 10:00:00`: total 2, uma ocorrência de INFO e uma de WARN.
6. Escolha **6**: confira `exemplos/sistema.log.relatorio.txt`.
7. Saia e abra novamente com o mesmo caminho. Escolha **5** sem usar 1. Informe início e fim iguais a `2026-10-19 09:30:00`: aparecem 2 registros.
8. Entre com opção `abc` e data `2026-02-30 10:00:00`: o programa informa entrada inválida. EOF encerra o menu.
9. Com o programa fechado, altere o log e tente 5 novamente: o índice é recusado como desatualizado. Use 1 para reconstruir.

## Organização e requisitos

```text
include/       interfaces públicas e guardas de inclusão
src/main.c     menu e ciclo de vida dos dados
src/log.c      leitura, validação, struct Registro e vetor dinâmico
src/ordenacao.c mergesort recursivo e estável
src/relatorio.c contagem, ranking e exportação em texto
src/indice.c   persistência binária e consulta por deslocamento
tests/         testes automatizados em C
exemplos/      log de demonstração
docs/          escopo, decisões e cartaz de arquitetura imprimível
```

São quatro módulos além do `main`, cada um com `.h/.c`. Toda comunicação ocorre por parâmetros e retornos. Veja `docs/ESCOPO.md`, `docs/DECISOES.md` e abra `docs/cartaz.html` no navegador para imprimir o cartaz em A4 paisagem.

## Como o programa funciona por dentro

O programa é dividido em módulos para que cada arquivo cuide de uma parte do trabalho. O `main.c` coordena o fluxo: recebe as escolhas do usuário, chama os outros módulos e apresenta os resultados. Os módulos não compartilham variáveis globais; eles trocam dados por parâmetros e valores de retorno.

### 1. Ponto de partida: `src/main.c`

A função `main()` é o ponto de entrada. Ao iniciar, ela prepara duas estruturas:

- `Log log`: guarda na memória os registros carregados e a quantidade de linhas inválidas.
- `Indice idx`: mantém abertos o arquivo de índice binário e o log original quando a opção 5 é usada.

O caminho do log pode vir como argumento ao iniciar o programa. Por exemplo, em `analisador exemplos/sistema.log`, o texto `exemplos/sistema.log` é recebido pelo `main()` como argumento. Se nenhum caminho for informado, o programa pergunta pelo caminho quando uma operação precisar dele.

O menu roda dentro de um laço `for (;;)` e continua aparecendo até o usuário escolher 0 ou encerrar a entrada. A função `perguntar()` mostra cada pergunta e chama `linha_ler()`, do módulo `log.c`, para ler a resposta com limite de tamanho. A função `intervalo()` pede duas datas, valida as duas e só aceita o intervalo quando a data inicial é menor ou igual à final.

### 2. Opção 1: carregar, ordenar e criar o índice

Ao escolher 1, o programa executa estas etapas:

1. **O `main.c` chama `carregar()`.** Essa função pede ao `log_carregar()`, em `src/log.c`, para abrir e ler o arquivo informado.
2. **O `log.c` lê e valida cada linha.** Antes de ler uma linha, guarda sua posição em bytes no arquivo, chamada de `offset`. A função `linha_ler()` lê a linha e `registro_parse()` separa data, categoria e evento usando os dois primeiros caracteres `|` como separadores. O restante da linha fica no campo do evento.
3. **A data vira um número.** `registro_parse()` chama `timestamp_parse()`, também em `log.c`. Essa função confere o formato e se a data existe, incluindo meses, horários e anos bissextos. Uma data válida é representada pelo número `AAAAMMDDHHMMSS`, que facilita comparar horários. Linhas inválidas são ignoradas e contabilizadas.
4. **Os registros válidos vão para a memória.** `log_carregar()` armazena cada registro em um vetor dinâmico. Quando o vetor fica cheio, usa `realloc()` para aumentar o espaço.
5. **O `main.c` chama o módulo `ordenacao.c`.** A função `mergesort_registros()` ordena o vetor por data e hora crescente usando mergesort recursivo. Ela divide o vetor em partes menores, ordena cada parte e intercala os resultados. Se dois registros tiverem a mesma data e hora, a ordem em que apareciam no arquivo é mantida.
6. **O `main.c` chama o módulo `indice.c`.** `indice_criar()` confere se o vetor ordenado corresponde ao log original e grava o arquivo `<arquivo>.idx`. O índice guarda a data e o `offset` de cada registro, além do tamanho e do hash FNV-1a do log, usados para detectar alterações no arquivo de origem.

Ao final, o menu informa quantos registros foram carregados e quantas linhas inválidas foram ignoradas. O vetor ordenado continua disponível na memória para as opções 2, 3, 4 e 6.

### 3. Opção 2: listar em ordem cronológica

O `main.c` percorre o vetor que já foi ordenado pela opção 1. Para cada elemento, chama `registro_imprimir()`, em `log.c`. Essa função converte a data numérica de volta para o formato `AAAA-MM-DD HH:MM:SS` usando `timestamp_format()` e imprime a linha na tela.

### 4. Opções 3, 4 e 6: gerar relatórios

- **Opção 3 — relatório completo:** o `main.c` chama `relatorio_gerar()` com um intervalo que inclui todas as datas. O relatório mostra o total de registros, as linhas inválidas, as quantidades por categoria, uma contagem para cada uma das 24 faixas de hora e o ranking dos eventos.
- **Opção 4 — relatório por intervalo:** o `main.c` primeiro chama `intervalo()`. As datas digitadas são verificadas e convertidas em números por `timestamp_parse()`. Depois, o intervalo é enviado para `relatorio_gerar()`, que inclui os registros entre as duas datas, inclusive os limites.
- **Opção 6 — exportar relatório:** o `main.c` chama `relatorio_salvar()`, em `relatorio.c`, que cria `<arquivo>.relatorio.txt` e usa `relatorio_gerar()` para escrever nele o mesmo conteúdo do relatório completo.

Dentro de `relatorio.c`, a função `contar()` soma ocorrências de cada categoria e evento. As categorias são exibidas em ordem alfabética. O ranking coloca os eventos mais frequentes primeiro e desempata pelo nome. As faixas de horário somam registros com a mesma hora, mesmo que estejam em dias diferentes dentro do intervalo.

### 5. Opção 5: consultar o índice persistido

Esta opção permite consultar registros sem carregar o vetor completo do log na memória. Ela usa o índice `.idx` criado anteriormente e o arquivo de log original:

1. **Abrir e conferir:** se ainda não estiver aberto, `main.c` chama `indice_abrir()`, em `indice.c`. O módulo confere a assinatura `T7IDX001`, o tamanho esperado, a ordenação das entradas e o hash FNV-1a. Também confere se as entradas apontam para registros compatíveis no log. Se o log mudou depois da criação do índice, a consulta é recusada e o usuário deve reconstruí-lo com a opção 1.
2. **Receber o intervalo:** `main.c` chama `intervalo()` para pedir as datas inicial e final e validar os limites.
3. **Encontrar as entradas:** `indice_consultar()` faz uma busca binária no arquivo `.idx`, usando `fseek()` para acessar diretamente as posições do intervalo.
4. **Ler os resultados:** para cada entrada encontrada, o índice usa o `offset` para localizar a linha correspondente no log original. As funções `linha_ler()` e `registro_parse()`, de `log.c`, leem e interpretam a linha; `registro_imprimir()` mostra o resultado no terminal.

Assim, o índice reduz o trabalho de localizar os registros pedidos. A primeira abertura ainda verifica o índice e o arquivo de origem; as consultas seguintes na mesma execução reutilizam os arquivos abertos.

### 6. Opção 0: encerrar com limpeza

Ao escolher 0, o laço do menu termina. Antes de sair, `main()` chama `indice_fechar()` para fechar os arquivos do índice e `log_liberar()` para liberar o vetor de registros alocado na memória.

### Resumo dos módulos

| Módulo | Responsabilidade |
| --- | --- |
| `main.c` | Controla o menu e coordena as operações. |
| `log.c` | Lê linhas, valida datas e registros, guarda dados na memória e imprime registros. |
| `ordenacao.c` | Ordena os registros por data com mergesort estável. |
| `relatorio.c` | Conta categorias, horários e eventos; imprime ou salva relatórios. |
| `indice.c` | Cria, valida e consulta o índice binário junto ao log original. |

Um jeito simples de lembrar o fluxo é: **o `main.c` coordena; o `log.c` lê e interpreta; o `ordenacao.c` organiza; o `relatorio.c` resume; e o `indice.c` localiza registros no disco.**

## Índice binário

| Deslocamento | Bytes | Conteúdo |
| --- | --- | --- |
| 0 | 8 | Assinatura e versão ASCII `T7IDX001` |
| 8 | 8 | Quantidade de entradas |
| 16 | 8 | Tamanho do log em bytes |
| 24 | 8 | Hash FNV-1a de 64 bits do texto original |
| 32 + 16 × i | 8 | Marca de tempo numérica `AAAAMMDDHHMMSS` |
| 40 + 16 × i | 8 | Deslocamento em bytes da linha no texto original |

Os inteiros usam little-endian explícito: o arquivo não depende do padding da struct. O tamanho total é **32 + 16 × n bytes**. Categoria e evento permanecem somente no log. Arquivos são abertos em modo binário para que CRLF não altere os deslocamentos.

Ao abrir o índice pela primeira vez na sessão, o programa confere formato, tamanho, hash da origem, ordenação e correspondência das entradas. Essa validação lê a origem e as entradas; portanto, não é O(log n). Depois, mantém os arquivos abertos e cada consulta custa O(log n + k), desconsiderando o tamanho das linhas, com `k` resultados. A busca binária lê entradas por `fseek(32 + 16 × i)` e recupera apenas as linhas selecionadas.

## Limites e decisões conscientes

- Projeto didático para arquivos pequenos/médios. O carregamento usa memória O(n), e o mergesort usa vetor auxiliar O(n), pilha O(log n) e tempo O(n log n).
- Contagens usam busca linear por categoria/evento: O(n²) no pior caso de todos os eventos distintos. O ranking usa `qsort`; a ordenação cronológica obrigatória é implementada manualmente com mergesort recursivo.
- O uso de `ftell/fseek` limita arquivos a `LONG_MAX` bytes (aproximadamente 2 GiB no Windows). O programa verifica deslocamentos e falhas de leitura.
- Não edite a origem nem o índice durante a sessão. A validação de integridade ocorre na abertura do índice; para logs alterados, reinicie e reconstrua com 1. Não é um monitor de logs em tempo real.
- O hash detecta alterações acidentais; não é uma assinatura criptográfica. O log original é necessário para consultar o índice.
- A opção 1 sobrescreve o índice associado e a opção 6 sobrescreve o relatório associado. A pasta de origem precisa permitir escrita. Se uma gravação falhar, reconstrua o arquivo gerado; a gravação não é transacional.


