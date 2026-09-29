#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<stdint.h>

#define N 10000000
#define Threads_count 4

typedef struct{
    const uint64_t * array;
    size_t start;
    size_t end; 
    uint64_t sum;
}continuous_sum_args;

typedef struct{
    const uint64_t * array;
    int thread_id;
    size_t end; 
    uint64_t sum;
}discontinuous_sum_args;

void* discontinuous_sum(void* args){
    discontinuous_sum_args *s = (discontinuous_sum_args*)args;
    s->sum=0;
    for(size_t i=s->thread_id; i<s->end; i+=Threads_count){
        s->sum += s->array[i];
    }
    return NULL;
}

void *continuous_sum(void* args){
    continuous_sum_args *c=(continuous_sum_args*)args;
    c->sum=0;
    for(size_t i=c->start; i<c->end; i++){
        c->sum += c->array[i];  
    }
    return NULL;
}

//Single thread sum
uint64_t normal_sum(const uint64_t* array,size_t n){
    uint64_t sum=0;
    for(size_t i=0;i<n;i++ ){
        sum+=array[i];
    }
    return sum;
}

int main(){
    size_t n=N;
    uint64_t *arr=(uint64_t*)malloc(n*sizeof(uint64_t));
    if(arr==NULL){
        printf("Array didn't create.Memory allocation failed\n");
        return 1;
    }
    
    srand(42);
    for (size_t i = 0; i < n; i++) {
        arr[i] = ((uint64_t)rand() << 32) | rand();
    }

    struct timespec start, end;
    double normal_time, cont_time, discont_time; 

    // Normal sum
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t normal_result = normal_sum(arr, n);
    clock_gettime(CLOCK_MONOTONIC, &end);
    normal_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Continuous sum
    pthread_t threads[Threads_count];       
    continuous_sum_args cont_args[Threads_count];
    size_t chunk_size = n / Threads_count;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < Threads_count; i++) {
        cont_args[i].array = arr;
        cont_args[i].start = i * chunk_size;
        cont_args[i].end = (i == Threads_count - 1) ? n : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, continuous_sum, &cont_args[i]);
    }
    uint64_t continuous_result = 0;
    for (int i = 0; i < Threads_count; i++) {   
        pthread_join(threads[i], NULL);
        continuous_result += cont_args[i].sum;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cont_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Discontinuous sum
    discontinuous_sum_args discont_args[Threads_count];                 
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < Threads_count; i++) {
        discont_args[i].array = arr;
        discont_args[i].thread_id = i;  
        discont_args[i].end = n;
        pthread_create(&threads[i], NULL, discontinuous_sum, &discont_args[i]);
    }
    uint64_t discontinuous_result = 0;
    for (int i = 0; i < Threads_count; i++) {
        pthread_join(threads[i], NULL);
        discontinuous_result += discont_args[i].sum;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    discont_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;   

    printf("Normal sum.      : %llu, Time taken: %f seconds\n", normal_result, normal_time);
    printf("Continuous sum.  : %llu, Time taken: %f seconds\n", continuous_result, cont_time);
    printf("Discontinuous sum: %llu, Time taken: %f seconds\n", discontinuous_result, discont_time); 

    free(arr);


    return 0;
}


