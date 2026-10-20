# Decisões e guia para a defesa

## Por que armazenar a data como número?

Em `AAAAMMDDHHMMSS`, datas válidas podem ser comparadas numericamente em ordem cronológica. A validação acontece antes da conversão. Não se trata de segundos desde uma época: não se deve subtrair dois valores para calcular duração. Isso evita dependência do fuso do computador e de `time_t`.

## Onde está a recursão?

Em `ordenacao.c`, a função interna `ordenar` chama a si mesma para `[inicio, meio)` e `[meio, fim)`. O caso base é `fim - inicio < 2`. O tamanho dos subproblemas diminui a cada chamada, garantindo término. Depois das duas chamadas, as metades ordenadas são intercaladas em um vetor auxiliar. O operador `<=` escolhe a metade esquerda em empates, preservando estabilidade.

Uma alternativa bottom-up iterativa intercalaria blocos de tamanhos 1, 2, 4, 8 e assim por diante. Evita a pilha recursiva, mas exige gerenciar os blocos explicitamente. A versão recursiva foi escolhida pela correspondência direta com dividir, resolver e combinar. Não usa a função pronta `qsort` para ordenar os registros.

## Como provar o acesso direto?

Na consulta, `entrada` posiciona o cursor em `32 + 16 * posicao` com `fseek`. A busca binária localiza a primeira marca de tempo maior ou igual ao início. Depois percorre apenas as entradas até o fim inclusivo e usa o deslocamento de cada uma para obter a linha no arquivo original. Reinicie o executável e escolha 5 para demonstrar que o vetor não precisa ser carregado.

A validação inicial é linear e intencional: verifica a origem e a coerência do índice antes de aceitar consultas. A complexidade O(log n + k) vale para consultas depois dessa abertura, não para o tempo total de inicialização.

## Por que não gravar a struct inteira?

O índice não precisa repetir as strings. Dois inteiros de 64 bits custam 16 bytes por registro. Gravar campos byte a byte evita depender do alinhamento das structs e da representação nativa dos inteiros. O cabeçalho identifica a versão e vincula o índice ao conteúdo do log. Se o texto mudar, os deslocamentos podem ficar inválidos: por isso o índice precisa ser reconstruído.

## Propriedade da memória e dos arquivos

`main` inicializa `Log` e `Indice` com zero. `log_carregar` monta um vetor temporário e só substitui o anterior após sucesso. `log_liberar` libera e zera o objeto. O mergesort aloca um auxiliar uma única vez e o libera no retorno. Relatórios liberam suas contagens após escrever. `indice_abrir` fecha recursos temporários em caso de erro; `indice_fechar` encerra os dois arquivos. O encerramento por EOF segue a mesma limpeza que a opção 0.

As APIs que recebem `Log` ou `Indice` como destino exigem objetos inicialmente zerados ou já gerenciados por esses módulos. `registro_parse` modifica a linha recebida para separar os campos. Essas escolhas simplificam a implementação sem compartilhar estado global.

## Divisão sugerida para até quatro integrantes

| Parte | O que demonstrar e explicar |
| --- | --- |
| Leitura e menu | Struct, vetor dinâmico, validação de datas e linhas rejeitadas |
| Ordenação | Caso base, chamadas recursivas, intercalação e estabilidade |
| Relatórios | Agrupamento por categoria/hora, ranking e arquivo-texto |
| Índice | Cabeçalho, endian, hash, busca binária e acesso por deslocamento |

Distribuam também os testes entre os integrantes. Cada pessoa deve conseguir explicar o caminho de um registro desde a linha original até a saída. Nenhum nome ou RA foi preenchido automaticamente.
