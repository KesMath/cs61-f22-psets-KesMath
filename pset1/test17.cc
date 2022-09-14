#include "m61.hh"
#include <cstdio>
#include <cassert>
#include <cstring>
// Check diabolical m61_calloc.

int main() {
    size_t very_large_count = (size_t) -1 / 8 + 2;
    //printf("large count: %li\n", very_large_count);
    // definitely an integer overflow value is going into malloc()
    // max value of unsigned long is 2^64 - 1) ~ 1.84 * 10^19 while value going into malloc
    // by according to calloc() definition is very_large_count * 16 ~ 3.68 * 10^19
    void* p = m61_calloc(very_large_count, 16);
    assert(p == nullptr);
    m61_print_statistics();
}

//! alloc count: active          0   total          0   fail          1
//! alloc size:  active          0   total          0   fail        ???
