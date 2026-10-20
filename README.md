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


