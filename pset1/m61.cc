#include "m61.hh"
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cassert>
#include <sys/mman.h>


const uint16_t APPROX_KILOBYTE = (1 << 10) - 24; //1000 bytes

struct m61_memory_buffer {
    char* buffer; // pointer reference to first byte in buffer
    size_t pos = 0;
    size_t size = 8 << 20; /* 8 MiB ceiling of virtual memory*/

    //constructor
    m61_memory_buffer();
    //deconstructor
    ~m61_memory_buffer();
};

static m61_memory_buffer default_buffer;

static m61_statistics alloc_stats = {
    .nactive = 0,
    .active_size = 0,
    .ntotal = 0,
    .total_size = 0,
    .nfail = 0,
    .fail_size = 0,
    .heap_min = 0,
    .heap_max = 0
};

m61_memory_buffer::m61_memory_buffer() {
    /*
    mmap() function asks the kernel to create new virtual memory area,
    preferably one that starts at address "nullptr" and map to a contiguous object
    chunk of the object specified by file descriptor fd = -1 to the new area    
    */
    void* buf = mmap(nullptr,    // Place the buffer at a random address
        this->size,              // Buffer/Virtual Memory should be 8 MiB big or 2^23 = 8,388,608 bytes
        PROT_WRITE,              // We want to read and write the buffer
        MAP_ANON | MAP_PRIVATE, -1, 0);
                                 // We want memory freshly allocated by the OS
    assert(buf != MAP_FAILED);

    //pointer to virtual memory returned from mmap() persists in buffer attribute of "m61_memory_buffer" struct
    this->buffer = (char*) buf; 

    // according to diagram 838 in textbook, I assume the smallest address is the value
    // that's returned by mmap() sys call and largest address of that virtual memory block
    // is that address returned from mmap() + the size
    alloc_stats.heap_min = (uintptr_t) buf;
    alloc_stats.heap_max = (uintptr_t) buf + this->size;
    //printf("HEAP MIN: %li\n", alloc_stats.heap_min);
    //printf("HEAP MAX: %li\n", alloc_stats.heap_max);
}

m61_memory_buffer::~m61_memory_buffer() {
    //deletes the area starting at virtual address "this.buffer" and consisting of next "size" bytes 
    munmap(this->buffer, this->size);
}




/// m61_malloc(sz, file, line)
///    Returns a pointer to `sz` bytes of freshly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then m61_malloc may
///    return either `nullptr` or a pointer to a unique allocation.
///    The allocation request was made at source code location `file`:`line`.

void* m61_malloc(size_t sz, const char* file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    alloc_stats.ntotal++;
    alloc_stats.total_size += sz;

    if (default_buffer.pos + sz > default_buffer.size || sz == SIZE_MAX) {
        // Not enough space left in default buffer for allocation
        alloc_stats.nfail++;
        alloc_stats.fail_size += sz;
        alloc_stats.ntotal--;
        alloc_stats.total_size -= sz;
        return nullptr;
    }
    // Otherwise there is enough space; claim the next `sz` bytes
    alloc_stats.nactive++;
    void* ptr = &default_buffer.buffer[default_buffer.pos]; //getting pointer at 0th position in 8 MiB buffer block or essentially heap_min

    // address value returned by m61_malloc() must be evenly divisible by 16 ... this returns 8!
    // pointer address must be shifted or added by 8 in order for it to be divisible by 16!
    default_buffer.pos += sz + 8;
    return ptr;
}


/// m61_free(ptr, file, line)
///    Frees the memory allocation pointed to by `ptr`. If `ptr == nullptr`,
///    does nothing. Otherwise, `ptr` must point to a currently active
///    allocation returned by `m61_malloc`. The free was called at location
///    `file`:`line`.

void m61_free(void* ptr, const char* file, int line) {
    // avoid uninitialized variable warnings
    (void) ptr, (void) file, (void) line;
    if(ptr != nullptr){
        alloc_stats.nactive--;
        // how do we go about freeing ptr so that all memory in virtual buffer is not consumed??
        // before we go onto use maps etc, let's get basics downpack so we know what we're doing
        // we're not deleting the memory that is consumed by that pointer but in fact we're going to 
        // "RE-RENT" or allow another pointer to occupy that space (even though garbage values from previous ptr may persist)
        // (1) We're going to subtract current, default_buffer.pos by previous ptr->pos so we never 
        // hit our ceiling in this virtual buffer (which is heap_max or default_buffer.size)
        // that way, on subsequent malloc() call, we will be recycling memory

        // we can only get away with this if we're allocating and freeing cyclically...
        // this implementation will not pass if we call free on other pointers at various positions in 8MiB buffer
        // in that case, we will need to keep track of them by ordered map!!! 
    }
}


/// m61_calloc(count, sz, file, line)
///    Returns a pointer a fresh dynamic memory allocation big enough to
///    hold an array of `count` elements of `sz` bytes each. Returned
///    memory is initialized to zero. The allocation request was at
///    location `file`:`line`. Returns `nullptr` if out of memory; may
///    also return `nullptr` if `count == 0` or `size == 0`.

void* m61_calloc(size_t count, size_t sz, const char* file, int line) {
    // check if result (i.e. y = a*b) is less than either of the factors which implies a wraparound occurred
    if(sz * count < count || sz * count < sz){
        alloc_stats.nfail++;
        return nullptr;
    }
    void* ptr = m61_malloc(count * sz, file, line);
    if (ptr) {
        memset(ptr, 0, count * sz);
    }
    return ptr;
}


/// m61_get_statistics()
///    Return the current memory statistics.

m61_statistics m61_get_statistics() {
    return alloc_stats;
}


/// m61_print_statistics()
///    Prints the current memory statistics.

void m61_print_statistics() {
    m61_statistics stats = m61_get_statistics();
    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}


/// m61_print_leak_report()
///    Prints a report of all currently-active allocated blocks of dynamic
///    memory.

void m61_print_leak_report() {
    // Your code here.
}
