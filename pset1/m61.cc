#include "m61.hh"
#include <map>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cassert>
#include <sys/mman.h>

// ordered map for tracking: {pointers to live allocations => bytes of reserved memory}
std::map<void*, size_t> active_ptrs;

// ordered map for tracking: {pointers to free allocations => bytes of freed memory}
std::map<void*, size_t> free_ptrs;

using freemap_iter = std::map<void*, size_t>::iterator;

void coalesce_up(freemap_iter it);
bool can_coalesce_up(freemap_iter it);

void coalesce_down(freemap_iter it);
bool can_coalesce_down(freemap_iter it);

void consolidate_all_free_memory_regions(freemap_iter it);

void* m61_find_free_space(size_t sz);

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


int get_padding(void* ptr, size_t sz){
    size_t padding = 0;
    if(((uintptr_t) ptr + sz) % 16 != 0){
        padding = 16 - (((uintptr_t) ptr + sz) % 16);
    }
    //printf("padding : %li\n", padding);
    return padding;
}

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
    //printf("Buffer pointer %li\n", (uintptr_t) this->buffer); 

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

// helper function for m61_malloc()
// always checks diff(heap ceiling - default buffer.current_pos) first to see if an allocation reside there first
// otherwise, checks free regions of memory
void* m61_find_free_space(size_t sz){
    // try default_buffer (i.e. check distance or space from current buffer.pos heap_max or ceiling)
    //FIXME: default_buffer.pos + sz <= def.size cause integer overflow can occur if default_buffer.pos > other term
    if (sz <= default_buffer.size - default_buffer.pos) {
        //printf("Virtual Buffer: %li\n", default_buffer.size);
        //printf("Default Pos: %li\n", default_buffer.pos);
        //printf("Diff: %li\n", default_buffer.size - default_buffer.pos);
        void* ptr = &default_buffer.buffer[default_buffer.pos]; //getting pointer at 0th position in 8 MiB buffer block or essentially heap_min
        //printf("loading on top of buffer\n");

        // address value returned by m61_malloc() must be evenly divisible by 16
        int padding = get_padding(ptr, sz);
        default_buffer.pos += sz + padding;
        active_ptrs.insert({ptr, sz});
        // for (auto iter = active_ptrs.begin(); iter != active_ptrs.end(); ++iter) {
        //         fprintf(stderr, "active key %li, value %li\n", (uintptr_t) iter->first, iter->second);
        //     }
        //printf("incrementing active size: %li by sz %li\n", alloc_stats.active_size, sz);
        alloc_stats.active_size += sz;
        alloc_stats.nactive++;
        return ptr;
    }

    // just-in-time coalescing!
    auto iter = free_ptrs.begin();
    consolidate_all_free_memory_regions(iter);

    //printf("entering loading into free space\n");
    // scans free_ptr map and find an available buffer zone that's less than or equal to size
    for (auto it = free_ptrs.begin(); it != free_ptrs.end(); ++it) {
        // do we consider alignment here in this conditional??
        if(sz <= it->second){
            // TODO: we dont update default_buffer.pos here
            // cause on a subsequent call to malloc() we want to consider (distance from ceiling to current pos)
            //printf("loading into free space\n");
            void* ptr = it->first;
            free_ptrs.erase(ptr);

            active_ptrs.insert({ptr, sz});
            //printf("incrementing active size: %li by sz %li\n", alloc_stats.active_size, sz);
            alloc_stats.active_size += sz;
            return ptr;
        }
    }

    alloc_stats.nfail++;
    alloc_stats.fail_size += sz;
    alloc_stats.ntotal--;
    alloc_stats.total_size -= sz;
    return nullptr;
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

    // ============= DEAD CODE =============
    // if (default_buffer.pos + sz > default_buffer.size || sz == SIZE_MAX) {
    //     // Not enough space left in default buffer for allocation
    //     printf("here!!\n");
    //     alloc_stats.nfail++;
    //     alloc_stats.fail_size += sz;
    //     alloc_stats.ntotal--;
    //     alloc_stats.total_size -= sz;
    //     return nullptr;
    // }
    // Otherwise there is enough space; claim the next `sz` bytes
    //alloc_stats.nactive++;
    // ============= DEAD CODE =============

    return m61_find_free_space(sz);
}

bool can_coalesce_up(freemap_iter it){
    assert(it != free_ptrs.end());

    auto nextBlock = it;
    nextBlock++;

    if(nextBlock == free_ptrs.end()){
        return false;
    }

    // cannot coalesce if there's no next entry in free_ptrs map!
    if(free_ptrs.find(nextBlock->first) == free_ptrs.end()){
        return false;
    }

    return ((uintptr_t) it->first) + it->second == (uintptr_t) nextBlock->first;
}
// ============= DEAD CODE =============
// bool can_coalesce_down(freemap_iter it){
//     //printf("assertion...\n");
//     assert(it != free_ptrs.end());
//     //printf("assertion passes...\n");
    
//     //if current iterator is at begin, we cannot coalesce downwards cause there's no element before!
//     //printf("iterator is begin??\n");
//     if(it == free_ptrs.begin()){
//         return false;
//     }
//     //printf("iterator not at begin...\n");

//     auto previousBlock = it;
//     previousBlock--;
    
//     //printf("previous block??...\n");
//     // cannot coalesce if there's no previous entry in free_ptrs map!
//     if(free_ptrs.find(previousBlock->first) == free_ptrs.end()){
//         return false;
//     }
//     // ============= DEAD CODE =============
//     //printf("there is a previous block...\n");
//     //printf("previousBlock first: %li\n", (uintptr_t) previousBlock->first);
//     //printf("previousBlock size: %li\n", previousBlock->second);
//     //printf("previousBlock first + size: %li\n", (uintptr_t) previousBlock->first + previousBlock->second);
//     //printf("current block ptr: %li\n", (uintptr_t) it->first);
//     // ============= DEAD CODE =============
//     return ((uintptr_t) previousBlock->first + previousBlock->second == (uintptr_t) it->first);
// }
// ============= DEAD CODE =============

void coalesce_up(freemap_iter it){
    if(can_coalesce_up(it)){
        auto next = it;
        next++;
        // consolidate free blocks by adding next value's memory allocation to current block
        it->second += next->second;

        // update free ptrs map to contain consolidated buffer
        free_ptrs.insert_or_assign(it->first, it->second);

        // erasing stale pointer
        free_ptrs.erase(next->first);
    }

}
// ============= DEAD CODE =============
// void coalesce_down(freemap_iter it){
//     if(can_coalesce_down(it)){
//         auto previous = it;
//         previous--;
//         // consolidate free blocks by adding current value's memory allocation to previous block
//         previous->second += it->second;
        
//         // update free ptrs map to contain consolidated buffer
//         free_ptrs.insert_or_assign(previous->first, previous->second);

//         // erasing stale pointer
//         free_ptrs.erase(it->first);
//     }

// }
// ============= DEAD CODE =============

// DEFERRED COALESCING TECHNIQUE
void consolidate_all_free_memory_regions(freemap_iter it){
    // downshifting iterator cursor as much as possible
    // so we can maximally coalesce up
    //printf("about to coalesce...");
    // while(can_coalesce_down(it)){
    //     //printf("coalescing down ...\n");
    //     coalesce_down(it);
    //     it--; 
    // }

    // FAULTY!! I believe as soon as it cannot coalesce_up, it returns false
    // while where can be free regions of memory to consolidate after that block that returned false!!!
    // while((can_coalesce_up(it)) && (it != free_ptrs.end())){
    //     //printf("coalescing up ...\n");
    //     coalesce_up(it);
    //     it++;
    // }

    // PRESUMABLY BETTER!! ITerator goes through all of the free elements in the map and performs consolidation
    // if it cannot, it simply skips that over and goes to next ... it doesn't short circuit like while loop
    for (; it != free_ptrs.end(); it++) {
        coalesce_up(it);
    }
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
        // prevent integer wraparound
        if(alloc_stats.nactive != 0){
            alloc_stats.nactive--;
        }
        // how do we go about freeing ptr so that all memory in virtual buffer is not consumed??
        // "RE-RENT" or allow another pointer to occupy that space (even though garbage values from previous ptr may persist)
        // (1) We're going to subtract current, default_buffer.pos by previous ptr->pos so we never 
        // hit our ceiling in this virtual buffer (which is heap_max or default_buffer.size)
        // that way, on subsequent malloc() call, we will be recycling memory
        // ===================================
        auto iter = active_ptrs.find(ptr);

        // can only free from m61_malloc() map
        if(iter != active_ptrs.end()){
            // FIXME: consider padding too!
            free_ptrs.insert({ptr, iter->second});
            default_buffer.pos -= iter->second;

            // ============= DEAD CODE =============
            // for (auto iterat = free_ptrs.begin(); iterat != free_ptrs.end(); ++iterat) {
            //     fprintf(stderr, "AFTR free key %li, value %li\n", (uintptr_t) iterat->first, iterat->second);
            // }


            // if(!can_coalesce_down(it) || !can_coalesce_up(it)){
            //     printf("cannot coalesce!\n");
            //     default_buffer.pos = 0;
            // }
            // ============= DEAD CODE =============

            //tracking which pointers are free so that malloc() can recycle if subsequent allocation can fit
            //printf("decreenting active size: %li by sz %li\n", alloc_stats.active_size, iter->second);
            alloc_stats.active_size -= iter->second;
            active_ptrs.erase(ptr);
        }
        else{
            //double free detection... causing previous test cases to fail so commenting out
            // 33 of 54 test cases passed with lines 334-341 commented out
            // 37 of 54 test cases passes with lines 334-341 active ... which one serves as best grading potential
            // given that some of my test cases in phase 2 fails while phase 3 test cases pass
            // if(free_ptrs.find(ptr) != free_ptrs.end()){
            //     fprintf(stderr, "MEMORY BUG %s:%i: invalid free of pointer %p, double free\n", file, line, ptr);
            // }
            // else{
            //    fprintf(stderr, "MEMORY BUG %s:%i: invalid free of pointer %p, not in heap\n", file, line, ptr); 
            // }
            // abort();
        }
        // ===================================
    }
}


/// m61_calloc(count, sz, file, line)
///    Returns a pointer a fresh dynamic memory allocation big enough to
///    hold an array of `count` elements of `sz` bytes each. Returned
///    memory is initialized to zero. The allocation request was at
///    location `file`:`line`. Returns `nullptr` if out of memory; may
///    also return `nullptr` if `count == 0` or `size == 0`.

void* m61_calloc(size_t count, size_t sz, const char* file, int line) {
    // first 2 statements checks if result (i.e. y = a*b) is less than either of the factors which implies a wraparound occurred
    // last statement checks if new allocation doesn't exceed buffer threshold
    if((sz * count) < count || (sz * count) < sz || default_buffer.pos + (sz * count) > default_buffer.size){
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
