#include "m61.hh"
#include <cstdio>
#include <cassert>
// Check diabolical failed allocation.

int main() {
    void* ptrs[10];
    for (int i = 0; i != 10; ++i) {
        ptrs[i] = m61_malloc(i + 1);
    }
    for (int i = 0; i != 5; ++i) {
        m61_free(ptrs[i]);
    }
    //weird af ... confusingly prints as -1 but enters function as 2^64 - 1
    size_t very_large_size = SIZE_MAX;
    //printf("Size about to be allocated: %li\n", very_large_size);
    void* garbage = m61_malloc(very_large_size);
    //printf("garbage pointer: %p\n", garbage);
    assert(!garbage);
    m61_print_statistics();
}

//! alloc count: active          5   total         10   fail          1
//! alloc size:  active        ???   total         55   fail ??{4294967295|18446744073709551615}??
