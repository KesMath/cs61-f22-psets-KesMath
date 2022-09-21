#include "m61.hh"
#include <cstdio>
#include <cassert>
#include <vector>
// Check for memory reuse: at most one active allocation.

int main() {
    for (int i = 0; i != 10000; ++i) {
        void* ptr = m61_malloc(1000);
        // TO GET ITERATION OF NULLPTR!
        // if(ptr == nullptr){
        //     printf("nulled at iteration: %i\n", i);
        //     return -1;
        // }
        assert(ptr);
        m61_free(ptr);
    }

    // =================
    // void* ptr0 = m61_malloc(1000);
    // printf("ptr0: %li\n", (uintptr_t) ptr0);
    // m61_free(ptr0);

    // void* ptr1 = m61_malloc(1000);
    // printf("ptr1: %li\n", (uintptr_t) ptr1);
    // m61_free(ptr1);

    // void* ptr2 = m61_malloc(1000);
    // printf("ptr2: %li\n", (uintptr_t) ptr2);
    // =================

    //assert(ptr2);
    //printf("ptr0: %li\n", (uintptr_t) ptr0);
    //printf("ptr1: %li\n", (uintptr_t) ptr1);
    //printf("ptr difference: %li\n", (uintptr_t) ptr1 - (uintptr_t) ptr0);
    m61_print_statistics();
}

//! alloc count: active          0   total      10000   fail          0
//! alloc size:  active        ???   total   10000000   fail          0
