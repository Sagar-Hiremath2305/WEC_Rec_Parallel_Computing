# WEC_Rec_Parallel_Computing

## Repo structure

### Task_1_Array_Sum/
Array_sum.c
 Makefile
### Task_2_SPSC_Ring/
spsc_ring.c
### Task_3_Forest_Fire/
main.c
forest_fire.comp

## Tasks

### Task 1:Multi-Threaded Array Summation
This program computes the sum of a randomly generated array of 64-bit unsigned integers ($N \ge 1024$) using 4 threads. It compares performance across three configurations:
* Single-Threaded Baseline
* Continuous Summation: Thread $i$ sums elements from index $i \times \frac{N}{4}$ to $(i + 1) \times \frac{N}{4}$.
* Discontinuous Summation: Thread $i$ sums elements where index $x \pmod 4 = i$.

### Task 2 :Lock-Free SPSC Ring Buffer
A fixed-capacity, lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer built completely from scratch using an array backing store.

Uses a power-of-2 capacity (capacity = 8) with bitwise masking (& MASK) for indexing, avoiding costly modulo operations. It relies on <stdatomic.h> with strict memory_order_acquire and memory_order_release semantics to ensure thread safety without locks or mutexes.

A producer thread pushes elements and a consumer thread pops elements concurrently, with live elements logged directly to the terminal.

Here I have added 2 files
spsc_ring.c using pthreads.h(because I was using clang of my mac)
spsc_ring_1.c using threads.h 

### Task 3

A GPU-accelerated forest fire simulation implemented using an ESSL 310 es Compute Shader and a C host controller leveraging EGL and OpenGL ES 3.1 APIs.
Simulation Rules:
* Healthy (H) tree catches fire with probability $p = 0.15$ if at least one neighbor is Burning (B).
* Burning (B) tree turns into Nothing (N) in the next epoch.
* Simulation runs iteratively until no burning cells remain, tracking the total epochs required to extinguish the fire.
* For small grid sizes ($M \le 20$), the grid state is printed to the terminal every epoch.

## Running codes

### Task 1
Open the folder and run these commands in terminal
make 
./Array_sum

### Task 2
Open the folder and run these commands in terminal
gcc -std=c11 -pthread spsc_ring.c -o spsc_ring
./spsc_ring

### task 3
Install the dependencies 
pkg update && pkg install clang make libandroid-gl-dev

Open the folder in terminal and run these commands
clang main.c -O3 -lEGL -lGLESv3 -o forest_fire
./forest_fire 15

