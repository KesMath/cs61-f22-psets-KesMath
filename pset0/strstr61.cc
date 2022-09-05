#include <cstring>
#include <cassert>
#include <cstdio>

using namespace std;

int strlen61(const char *s){
    int cout = 0;
    while(s[cout] != '\0'){
        cout++;
    }
    return cout;
}
//TODO: replace strlen() lib function with custom version given that this exercise is suppose to simulate a lib call that has no ref to other lib
char* mystrstr(const char* s1, const char* s2) {
    // If s2 or needle points to a string with zero length, the function shall return s1.
    if(strlen61(s2) == 0){
        return (char*) &s1[0]; //not sure if "const_cast" is safer practice
    }

    // Case when haystack is empty
    if(strlen61(s1) == 0){
        return nullptr;
    }
    // Case when strlen(needle) > strlen(haystack) (implying the needle is a size of a haystack and vice-versa)
    if(strlen61(s2) > strlen61(s1)){
        return nullptr;
    }
    //PSEUDO for CHECKING FIRST OCCURRENCE:
    // for every char in needle, check char in haystack
    // if first char found in haystack, we check if every subsequent char in needle == char in haystack
    // if all match, return pointer to first char, else return nullptr
    for(int i = 0; i < strlen61(s2); i++){
        for(int j = 0; j < strlen61(s1); j++){
            if(s2[i] == s1[j]){
                // found a match in first char now we check if every subsequent char in needle == char in haystack
                // if all subsequent chars match, then we return pointer from haystack
                char* haystackPtr = (char*) &s2[i];
                for(; i < strlen61(s2); i++){
                    if(s2[i] != s1[i]){
                        return nullptr;
                    }
                    return haystackPtr;
                }
            }
        }
    }
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