#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>
#define NUM_MAX_THREADS 15

typedef struct requisicao {
    int numCasas;
    int tempoEspera;
} Requisicao;

typedef struct thread_trabalhadora {
    pthread_t thread;
    int is_empty;
    int processed_requests;
    int id;
} Thread_w;

typedef struct informacoes {
    int num_req;
    int num_threads;
} Informacoes;

char* calcular_pi(int n);
pthread_mutex_t req_mutex;
pthread_mutex_t file_mutex;

Requisicao* fila_requisicoes;
int fila_inicio = 0;
int fila_fim = 0;
const int TAMANHO_FILA = 1000;

int fila_requisicoes_vazia() {
    return fila_inicio == fila_fim;
}

Requisicao* remover_requisicao() {
    if (fila_requisicoes_vazia()) {
        return NULL;
    }

    Requisicao* req = &fila_requisicoes[fila_inicio];
    fila_inicio = (fila_inicio + 1) % TAMANHO_FILA;
    return req;
}

void* trabalho_thread(void* arg) {
    Thread_w* info = (Thread_w*)arg;
    int thread_id = info->id;
    char nome_arq[20];
    sprintf(nome_arq, "thread%d.txt", thread_id);
    FILE* arquivo = fopen(nome_arq, "w");

    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s\n", nome_arq);
        pthread_exit(NULL);
    }

    int req_count = 0;
    Requisicao* req;

    while (1) {
        pthread_mutex_lock(&req_mutex);

        req = remover_requisicao();

        pthread_mutex_unlock(&req_mutex);

        if (req == NULL) {
            break;
        }

        char* pi = calcular_pi(req->numCasas);
        usleep(req->tempoEspera * 1000);
        char result[200];
        sprintf(result, "%d;%d;%d;%s\n", req_count + 1, req->numCasas, req->tempoEspera, pi);

        pthread_mutex_lock(&file_mutex);
        fprintf(arquivo, "%s", result);
        fflush(arquivo);
        pthread_mutex_unlock(&file_mutex);
        printf("A thread %d processou uma requisição!\n", thread_id);
        req_count++;
        
    }

    fclose(arquivo);
    pthread_exit(NULL);
}

void* t_thread_dispatcher(void* arg) {
    pthread_mutex_init(&req_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    
    Informacoes* info = (Informacoes*)arg;
    FILE* arq_requisicoes = fopen("requisicoes.txt", "r");
    int num_requisicoes = info->num_req;
    int num_threads = info->num_threads;
    printf("Thread dispathcher criada!\n");

    if (arq_requisicoes == NULL) {
        printf("Erro ao abrir o arquivo de requisições\n");
        return NULL;
    }

    fila_requisicoes = malloc(sizeof(Requisicao) * num_requisicoes);

    for (int i = 0; i < num_requisicoes; i++) {
        Requisicao* req = &fila_requisicoes[fila_fim];
        fscanf(arq_requisicoes, "%d;%d", &req->numCasas, &req->tempoEspera);
        fila_fim = (fila_fim + 1) % TAMANHO_FILA;
        usleep(200* 1000);
        if(i==0) {
        printf("Lendo requisições...\n");
        }       
        
    }

    fclose(arq_requisicoes);

    Thread_w* threads = malloc(sizeof(Thread_w) * num_threads);

    for (int i = 0; i < num_threads; i++) {
        threads[i].id = i + 1;
        pthread_create(&threads[i].thread, NULL, trabalho_thread, (void*)&threads[i]);
        printf("Thread %d criada!\n", i+1);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    pthread_mutex_destroy(&req_mutex);
    pthread_mutex_destroy(&file_mutex);
    printf("Todas as requisições foram processadas!\n");
    free(threads);
    free(fila_requisicoes);

    return NULL;
}

void preencher_requisicao(int n) {
    FILE* requisicoes = fopen("requisicoes.txt", "w");
    srand(time(NULL));

    if (requisicoes == NULL) {
        printf("Erro ao abrir o arquivo requisicoes.txt\n");
        return;
    }

    for (int j = 0; j < n; j++) {
        int i = rand() % 91 + 10;
        int tempo_espera = rand() % 1001 + 500;
        fprintf(requisicoes, "%d;%d\n", i, tempo_espera);
    }

    fclose(requisicoes);
}

char* calcular_pi(int n) {
    mpfr_t pi;
    mpfr_init2(pi, n + 2); 
    mpfr_const_pi(pi, MPFR_RNDN);
    
    size_t str_size = mpfr_snprintf(NULL, 0, "%.*Rf", n, pi);
    char *pi_str = malloc((str_size + 1) * sizeof(char));

    
    mpfr_sprintf(pi_str, "%.*Rf", n, pi);
    
    mpfr_clear(pi);
    
    return pi_str;
}

int main() {
    for (int i = 1; i <= NUM_MAX_THREADS; i++) {
        char nome_arq[20];
        sprintf(nome_arq, "thread%d.txt", i);
        remove(nome_arq);
    }

    int num_requisicoes;
    int num_threads;

    printf("Informe o número de requisições: ");
    scanf("%d", &num_requisicoes);

    printf("Informe o número de threads trabalhadoras: ");
    scanf("%d", &num_threads);

    
    preencher_requisicao(num_requisicoes);
    printf("Requisições preenchidas!\n");
    Informacoes* inf = malloc(sizeof(Informacoes));
    inf->num_req = num_requisicoes;
    inf->num_threads = num_threads;

    pthread_t thread_dispatcher;
    pthread_create(&thread_dispatcher, NULL, t_thread_dispatcher, (void*)inf);
    pthread_join(thread_dispatcher, NULL);

    free(inf);

    return 0;
}
