#include <cstring>
#include <cassert>
#include <cstdio>

//TODO: replace strlen() lib function with custom version given that this exercise is suppose to simulate a lib call that has no ref to other lib
char* mystrstr(const char* s1, const char* s2) {
    // If s2 or needle points to a string with zero length, the function shall return s1.
    if(strlen(s2) == 0){
        return (char*) &s1[0]; //not sure if "const_cast" is safer practice
    }

    // Case when haystack is empty
    if(strlen(s1) == 0){
        return nullptr;
    }
    // Case when strlen(needle) > strlen(haystack) (implying the needle is a size of a haystack and vice-versa)
    if(strlen(s2) > strlen(s1)){
        return nullptr;
    }
    //PSEUDO for CHECKING FIRST OCCURRENCE:
    return nullptr;

}

int main(int argc, char* argv[]) {
    assert(argc == 3);
    printf("strstr(\"%s\", \"%s\") = %p\n",
           argv[1], argv[2], strstr(argv[1], argv[2]));
    printf("mystrstr(\"%s\", \"%s\") = %p\n",
           argv[1], argv[2], mystrstr(argv[1], argv[2]));
    assert(strstr(argv[1], argv[2]) == mystrstr(argv[1], argv[2]));
}