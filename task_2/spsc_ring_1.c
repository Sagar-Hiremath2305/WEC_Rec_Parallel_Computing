#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>
#include <unistd.h>

#define CAPACITY 16
#define MASK (CAPACITY - 1)
#define TOTAL_ITEMS 40

typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    int buffer[CAPACITY];
} spsc_ring_t;

void spsc_ring_init(spsc_ring_t *ring) {
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
}

bool spsc_push(spsc_ring_t *ring, int item) {
    size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (head - tail >= CAPACITY) {
        return false;
    }

    ring->buffer[head & MASK] = item;

    atomic_store_explicit(&ring->head, head + 1, memory_order_release);
    return true;
}

bool spsc_pop(spsc_ring_t *ring, int *value) {
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (head == tail) {
        return false;
    }

    *value = ring->buffer[tail & MASK];

    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    return true;
}

int producer_routine(void *arg) {
    spsc_ring_t *rb = (spsc_ring_t *)arg;

    for (int i = 1; i <= TOTAL_ITEMS; i++) {
        while (!spsc_push(rb, i)) {
            thrd_yield();
        }
        printf("[PRODUCER] Added element: %d\n", i);
        fflush(stdout);
        usleep(100000);
    }
    return 0;
}

int consumer_routine(void *arg) {
    spsc_ring_t *rb = (spsc_ring_t *)arg;
    int consumed_count = 0;
    int val;

    while (consumed_count < TOTAL_ITEMS) {
        if (spsc_pop(rb, &val)) {
            printf("     [CONSUMER]       Removed element: %d\n", val);
            fflush(stdout);
            consumed_count++;
        } else {
            thrd_yield();
        }
        usleep(150000); 
    }
    return 0;
}

int main() {
    spsc_ring_t rb;
    spsc_ring_init(&rb);

    thrd_t producer, consumer;

    printf("Starting SPSC Ring Buffer (Capacity: %d)\n", CAPACITY);

    if (thrd_create(&producer, producer_routine, &rb) != thrd_success ||
        thrd_create(&consumer, consumer_routine, &rb) != thrd_success) {
        fprintf(stderr, "Failed to create threads.\n");
        return 1;
    }

    thrd_join(producer, NULL);
    thrd_join(consumer, NULL);
    printf("All items successfully produced and consumed!\n");

    return 0;
}