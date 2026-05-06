#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <omp.h>
#include <time.h>

#define CUTOFF 1000


typedef struct { float x, y; } Ponto;

typedef struct QuadNode {
    float x_min, x_max, y_min, y_max;
    struct QuadNode *NW, *NE, *SW, *SE;
    Ponto *pontos;
    size_t numero_de_pontos;
    size_t capacidade;
    bool eh_folha;
} QuadNode;

static QuadNode *criar_no(float x_min, float x_max, float y_min, float y_max) {
    QuadNode *no = malloc(sizeof(QuadNode));
    no->x_min = x_min;
    no->x_max = x_max;
    no->y_min = y_min;
    no->y_max = y_max;
    no->NW = no->NE = no->SW = no->SE = NULL;
    no->pontos = NULL;
    no->numero_de_pontos = 0;
    no->capacidade = 50;
    no->eh_folha = true;
    return no;
}

static Ponto *gerar_pontos(size_t qtd, float tamanho) {
    
    Ponto *pontos = malloc(qtd * sizeof(Ponto));
    for (size_t i = 0; i < qtd; i++) {
        float x = ((float) rand() / (float) RAND_MAX) * tamanho;
        float y = ((float) rand() / (float) RAND_MAX) * tamanho;
        pontos[i].x = x;
        pontos[i].y = y;
    }

    return pontos;
}


static void inserir_ponto(QuadNode *no, Ponto ponto) {
    if (ponto.x < no->x_min || ponto.x > no->x_max || ponto.y < no->y_min || ponto.y > no->y_max) {
        return;
    }

    if (no->eh_folha) {
        if (no->numero_de_pontos < no->capacidade) {
            if (no->pontos == NULL) {
                no->pontos = malloc(no->capacidade * sizeof(Ponto));
            }
            no->pontos[no->numero_de_pontos] = ponto;
            no->numero_de_pontos++;
            return;
        } else {
            no->eh_folha = false;
            float ponto_medio_x = (no->x_min + no->x_max) / 2.0f;
            float ponto_medio_y = (no->y_min + no->y_max) / 2.0f;

            no->NW = criar_no(no->x_min, ponto_medio_x, ponto_medio_y, no->y_max);
            no->NE = criar_no(ponto_medio_x, no->x_max, ponto_medio_y, no->y_max);
            no->SW = criar_no(no->x_min, ponto_medio_x, no->y_min, ponto_medio_y);
            no->SE = criar_no(ponto_medio_x, no->x_max, no->y_min, ponto_medio_y);

            for (size_t i = 0; i < no->numero_de_pontos; i++) {
                inserir_ponto(no, no->pontos[i]);
            }
            free(no->pontos);
            no->pontos = NULL;
        }
        
    } 

    if (ponto.x <= (no->x_min + no->x_max) / 2.0f) {
        if (ponto.y <= (no->y_min + no->y_max) / 2.0f) {
            inserir_ponto(no->SW, ponto);
        } else {
            inserir_ponto(no->NW, ponto);
        }
    } else {
        if (ponto.y <= (no->y_min + no->y_max) / 2.0f) {
            inserir_ponto(no->SE, ponto);
        } else {
            inserir_ponto(no->NE, ponto);
        }
    }
}

static void buscar_no_raio(QuadNode *no, Ponto centro, float raio, Ponto *saida, size_t *qtd_saida) {
    if (no == NULL) return;

    float x_min = centro.x - raio;
    float x_max = centro.x + raio;
    float y_min = centro.y - raio;
    float y_max = centro.y + raio;

    if (no->x_max < x_min || no->x_min > x_max || no->y_max < y_min || no->y_min > y_max) {
        return;
    }

    if (no->eh_folha) {
        for (size_t i = 0; i < no->numero_de_pontos; i++) {
            float dx = no->pontos[i].x - centro.x;
            float dy = no->pontos[i].y - centro.y;
            if (dx * dx + dy * dy <= raio * raio) {
                saida[*qtd_saida] = no->pontos[i];
                (*qtd_saida)++;
            }
        }
        return;
    }

    buscar_no_raio(no->NW, centro, raio, saida, qtd_saida);
    buscar_no_raio(no->NE, centro, raio, saida, qtd_saida);
    buscar_no_raio(no->SW, centro, raio, saida, qtd_saida);
    buscar_no_raio(no->SE, centro, raio, saida, qtd_saida);
}

static void buscar_no_raio_forca_bruta(Ponto *pontos, size_t qtd, Ponto centro, float raio, Ponto *saida, size_t *qtd_saida) {
    for (size_t i = 0; i < qtd; i++) {
        float dx = pontos[i].x - centro.x;
        float dy = pontos[i].y - centro.y;
        if (dx * dx + dy * dy <= raio * raio) {
            saida[*qtd_saida] = pontos[i];
            (*qtd_saida)++;
        }
    }
}

static QuadNode *construir_quadtree_paralelo(Ponto *pontos, size_t qtd, float x_min, float x_max, float y_min, float y_max) {
    QuadNode *no = criar_no(x_min, x_max, y_min, y_max);

    if (qtd <= CUTOFF) {
        for (size_t i = 0; i < qtd; i++) {
            inserir_ponto(no, pontos[i]);
        }
        return no;
    }

    no->eh_folha = false;
    float meio_x = (x_min + x_max) / 2.0f;
    float meio_y = (y_min + y_max) / 2.0f;

    size_t qtd_NW = 0, qtd_NE = 0, qtd_SW = 0, qtd_SE = 0;
    for (size_t i = 0; i < qtd; i++) {
        if (pontos[i].x <= meio_x) {
            if (pontos[i].y <= meio_y) qtd_SW++;
            else qtd_NW++;
        } else {
            if (pontos[i].y <= meio_y) qtd_SE++;
            else qtd_NE++;
        }
    }

    Ponto *p_NW = malloc(qtd_NW * sizeof(Ponto));
    Ponto *p_NE = malloc(qtd_NE * sizeof(Ponto));
    Ponto *p_SW = malloc(qtd_SW * sizeof(Ponto));
    Ponto *p_SE = malloc(qtd_SE * sizeof(Ponto));

    size_t i_NW = 0, i_NE = 0, i_SW = 0, i_SE = 0;
    for (size_t i = 0; i < qtd; i++) {
        if (pontos[i].x <= meio_x) {
            if (pontos[i].y <= meio_y) p_SW[i_SW++] = pontos[i];
            else p_NW[i_NW++] = pontos[i];
        } else {
            if (pontos[i].y <= meio_y) p_SE[i_SE++] = pontos[i];
            else p_NE[i_NE++] = pontos[i];
        }
    }

    #pragma omp taskgroup
    {
        #pragma omp task shared(no)
        no->NW = construir_quadtree_paralelo(p_NW, qtd_NW, x_min, meio_x, meio_y, y_max);

        #pragma omp task shared(no)
        no->NE = construir_quadtree_paralelo(p_NE, qtd_NE, meio_x, x_max, meio_y, y_max);

        #pragma omp task shared(no)
        no->SW = construir_quadtree_paralelo(p_SW, qtd_SW, x_min, meio_x, y_min, meio_y);

        #pragma omp task shared(no)
        no->SE = construir_quadtree_paralelo(p_SE, qtd_SE, meio_x, x_max, y_min, meio_y);
    }

    free(p_NW); free(p_NE); free(p_SW); free(p_SE);
    return no;
}

int contar_pontos(QuadNode *no) {
    if (no == NULL) return 0;
    if (no->eh_folha) return no->numero_de_pontos;
    return contar_pontos(no->NW) + contar_pontos(no->NE) + contar_pontos(no->SW) + contar_pontos(no->SE);
}

int main(void) {
    srand(42);
    size_t NUMERO_PARTICULAS = 100000;
    float TAMANHO_LIMITE = 1000.0f;

    printf("CONSTRUÇÃO PARALELA DA QUADTREE\n");
    printf("CONSTRUINDO QUADTREE COM %zu PONTOS NO ESPAÇO %.0fx%.0f\n", NUMERO_PARTICULAS, TAMANHO_LIMITE, TAMANHO_LIMITE);

    Ponto *pontos = gerar_pontos(NUMERO_PARTICULAS, TAMANHO_LIMITE);
    double t_inicio = omp_get_wtime();
    QuadNode *raiz = NULL;
    #pragma omp parallel
    {
        #pragma omp single
        raiz = construir_quadtree_paralelo(pontos, NUMERO_PARTICULAS, 0.0f, TAMANHO_LIMITE, 0.0f, TAMANHO_LIMITE);
    }
    double t_fim = omp_get_wtime();


    printf("Pontos inseridos: %d\n", contar_pontos(raiz));
    printf("Construção levou %.6f s (com %d threads)\n", t_fim - t_inicio, omp_get_max_threads());

    float raio = 20.0f;
    size_t qtd_amostras = 10;
    size_t passo = NUMERO_PARTICULAS / qtd_amostras;
    printf("\n\nTESTE: AMOSTRA DE DISTRIBUIÇÃO DE VIZNHOS\n");
    printf("BUCANDO VIZINHOS PARA %zu AMOSTRAS NO RAIO %.1f\n", qtd_amostras, raio);
    #pragma omp parallel
    {
        Ponto *buffer = malloc(NUMERO_PARTICULAS * sizeof(Ponto));
        #pragma omp for
        for (size_t amostra = 0; amostra < qtd_amostras; amostra++) {
            size_t indice = amostra * passo;
            Ponto ponto_base = pontos[indice];
            size_t qtd_vizinhos = 0;
            buscar_no_raio(raiz, ponto_base, raio, buffer, &qtd_vizinhos);
            #pragma omp critical
            printf("particula[%zu] (%.2f, %.2f): %zu vizinhos\n", indice, ponto_base.x, ponto_base.y, qtd_vizinhos);
        }
        free(buffer);
    }

    Ponto ponto_central = {500.0f, 500.0f};
    raio = 10.0f;

    printf("\n\nTESTE: BUSCA DE VIZINHOS PELA QUADTREE\n");
    printf("BUSCANDO VIZINHOS DE (%.1f, %.1f) NO RAIO %.1f\n", ponto_central.x, ponto_central.y, raio);
    Ponto *vizinhos = malloc(NUMERO_PARTICULAS * sizeof(Ponto));
    size_t qtd_quadtree = 0;
    t_inicio = omp_get_wtime();
    buscar_no_raio(raiz, ponto_central, raio, vizinhos, &qtd_quadtree);
    t_fim = omp_get_wtime();
    printf("Vizinhos encontrados: %zu\n", qtd_quadtree);
    printf("Busca levou %.6f s\n", t_fim - t_inicio);
    for (size_t i = 0; i < qtd_quadtree; i++) {
        if (i % 5 == 0) printf("\n");
        printf("(%.2f, %.2f);", vizinhos[i].x, vizinhos[i].y);
    }
    free(vizinhos);

    printf("\n\nTESTE: BUSCA DE VIZINHOS POR FORÇA BRUTA:\n");
    printf("BUSCANDO VIZINHOS DE (%.1f, %.1f) NO RAIO %.1f\n", ponto_central.x, ponto_central.y, raio);
    vizinhos = malloc(NUMERO_PARTICULAS * sizeof(Ponto));
    size_t qtd_bruta = 0;
    t_inicio = omp_get_wtime();
    buscar_no_raio_forca_bruta(pontos, NUMERO_PARTICULAS, ponto_central, raio, vizinhos, &qtd_bruta);
    t_fim = omp_get_wtime();
    printf("Vizinhos encontrados: %zu\n", qtd_bruta);
    printf("Busca levou %.6f s\n", t_fim - t_inicio);
    for (size_t i = 0; i < qtd_bruta; i++) {
        if (i % 5 == 0) printf("\n");
        printf("(%.2f, %.2f); ", vizinhos[i].x, vizinhos[i].y);
    }
    printf("\n");
    free(vizinhos);

    free(pontos);
    return 0;
}