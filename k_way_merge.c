#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <omp.h>

static char **ler_itens(const char *caminho, size_t *qtd_saida) {
    FILE *fp = fopen(caminho, "r");
    if (fp == NULL) {
        perror("fopen");
        *qtd_saida = 0;
        return NULL;
    }

    size_t capacidade = 16;
    size_t qtd = 0;
    char **itens = malloc(capacidade * sizeof(char *));

    char linha[1024];
    while (fgets(linha, sizeof(linha), fp) != NULL) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (linha[0] == '\0') continue;

        if (qtd == capacidade) {
            capacidade *= 2;
            itens = realloc(itens, capacidade * sizeof(char *));
        }

        itens[qtd] = strdup(linha);
        qtd++;
    }

    fclose(fp);
    *qtd_saida = qtd;
    return itens;
}

static void dividir_em_blocos(char **itens, size_t qtd, size_t k, char ***blocos, size_t *tamanhos) {
    size_t tamanho_do_bloco = qtd / k;
    size_t resto = qtd % k;

    size_t cursor = 0;
    for (size_t i = 0; i < k; i++) {
        size_t n = i < resto ? tamanho_do_bloco + 1 : tamanho_do_bloco;
        blocos[i] = &itens[cursor];
        tamanhos[i] = n;
        cursor += n;
    }
}

static char **intercalar(char **lista_a, size_t tamanho_lista_a,char **lista_b, size_t tamanho_lista_b,size_t *tamanho_saida) {
    size_t tamanho_total = tamanho_lista_a + tamanho_lista_b;
    char **saida = malloc(tamanho_total * sizeof(char *));

    size_t cursor_a = 0;
    size_t cursor_b = 0;
    size_t cursor_saida = 0;

    while (cursor_a < tamanho_lista_a && cursor_b < tamanho_lista_b) {
        if (strcasecmp(lista_a[cursor_a], lista_b[cursor_b]) <= 0) {
            saida[cursor_saida] = lista_a[cursor_a];
            cursor_saida++;
            cursor_a++;
        } else {
            saida[cursor_saida] = lista_b[cursor_b];
            cursor_saida++;
            cursor_b++;
        }
    }

    while (cursor_a < tamanho_lista_a) {
        saida[cursor_saida] = lista_a[cursor_a];
        cursor_saida++;
        cursor_a++;
    }
    while (cursor_b < tamanho_lista_b) {
        saida[cursor_saida] = lista_b[cursor_b];
        cursor_saida++;
        cursor_b++;
    }

    *tamanho_saida = tamanho_total;
    return saida;
}

static void merge_sort(char **lista, size_t tamanho) {
    if (tamanho <= 1) return;

    size_t meio = tamanho / 2;
    merge_sort(lista, meio);
    merge_sort(lista + meio, tamanho - meio);

    size_t tamanho_fundido = 0;
    char **fundido = intercalar(lista, meio, lista + meio, tamanho - meio, &tamanho_fundido);
    memcpy(lista, fundido, tamanho * sizeof(char *));
    free(fundido);
}

static void escrever_saida(const char *caminho, char **lista, size_t tamanho) {
    FILE *fp = fopen(caminho, "w");
    if (fp == NULL) {
        perror("fopen (escrita)");
        return;
    }
    for (size_t i = 0; i < tamanho; i++) {
        fprintf(fp, "%s\n", lista[i]);
    }
    fclose(fp);
}

static char **merge_tree(char ***blocos, size_t *tamanhos, size_t inicio, size_t fim, size_t *tamanho_saida) {
    if (inicio == fim) {
        size_t tamanho_folha = tamanhos[inicio];
        char **copia = malloc(tamanho_folha * sizeof(char *));
        memcpy(copia, blocos[inicio], tamanho_folha * sizeof(char *));
        *tamanho_saida = tamanho_folha;
        return copia;
    }

    size_t meio = (inicio + fim) / 2;

    size_t tamanho_esquerda = 0;
    char **esquerda = merge_tree(blocos, tamanhos, inicio, meio, &tamanho_esquerda);

    size_t tamanho_direita = 0;
    char **direita = merge_tree(blocos, tamanhos, meio + 1, fim, &tamanho_direita);

    char **resultado = intercalar(esquerda, tamanho_esquerda, direita, tamanho_direita, tamanho_saida);

    free(esquerda);
    free(direita);

    return resultado;
}

static char **merge_tree_paralelo(char ***blocos, size_t *tamanhos, size_t inicio, size_t fim, size_t *tamanho_saida) {
    if (inicio == fim) {
        size_t tamanho_folha = tamanhos[inicio];
        char **copia = malloc(tamanho_folha * sizeof(char *));
        memcpy(copia, blocos[inicio], tamanho_folha * sizeof(char *));
        *tamanho_saida = tamanho_folha;
        return copia;
    }

    size_t meio = (inicio + fim) / 2;

    size_t tamanho_esquerda = 0;
    char **esquerda = NULL;

    size_t tamanho_direita = 0;
    char **direita = NULL;

    #pragma omp task shared(esquerda, tamanho_esquerda)
    esquerda = merge_tree_paralelo(blocos, tamanhos, inicio, meio, &tamanho_esquerda);

    #pragma omp task shared(direita, tamanho_direita)
    direita = merge_tree_paralelo(blocos, tamanhos, meio + 1, fim, &tamanho_direita);

    #pragma omp taskwait

    char **resultado = intercalar(esquerda, tamanho_esquerda, direita, tamanho_direita, tamanho_saida);

    free(esquerda);
    free(direita);

    return resultado;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "uso: %s <k>\n", argv[0]);
        return 1;
    }

    size_t k = (size_t) atoi(argv[1]);
    if (k < 2) {
        fprintf(stderr, "k precisa ser >= 2\n");
        return 1;
    }

    size_t qtd = 0;
    char **itens = ler_itens("lista_arquivos.txt", &qtd);
    printf("%zu itens  | k = %zu)\n", qtd, k);

    char **blocos[k];
    size_t tamanhos[k];
    dividir_em_blocos(itens, qtd, k, blocos, tamanhos);

    for (size_t i = 0; i < k; i++) {
        merge_sort(blocos[i], tamanhos[i]);
    }

    // sequencial
    double t_inicio_seq = omp_get_wtime();
    size_t tamanho_final_seq = 0;
    char **ordenado_seq = merge_tree(blocos, tamanhos, 0, k - 1, &tamanho_final_seq);
    double t_fim_seq = omp_get_wtime();
    printf("Tempo sequencial: %.6f s\n", t_fim_seq - t_inicio_seq);

    // paralela
    double t_inicio_par = omp_get_wtime();
    size_t tamanho_final_par = 0;
    char **ordenado_par = NULL;
    #pragma omp parallel
    {
        #pragma omp single
        ordenado_par = merge_tree_paralelo(blocos, tamanhos, 0, k - 1, &tamanho_final_par);
    }
    double t_fim_par = omp_get_wtime();
    printf("Tempo paralelo:   %.6f s (com %d threads)\n", t_fim_par - t_inicio_par, omp_get_max_threads());

    escrever_saida("saida_ordenada_sequencial.txt", ordenado_seq, tamanho_final_seq);
    printf("Saida gravada em 'saida_ordenada_sequencial.txt'\n");
    escrever_saida("saida_ordenada_paralela.txt", ordenado_par, tamanho_final_par);
    printf("Saida gravada em 'saida_ordenada_paralela.txt'\n");

    free(ordenado_seq);
    free(ordenado_par);

    for (size_t i = 0; i < qtd; i++) free(itens[i]);
    free(itens);
    return 0;
}
