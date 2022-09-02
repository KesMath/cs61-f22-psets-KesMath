#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include <iostream>
#include <cstring>

using namespace std;
const char* NEWLINE = "\n";

int main() {    
    int line_cout = 0, word_cout = 0, byte_cout = 0;
    char* buffer = (char*) calloc(1, sizeof(char));
    while(fread(buffer, sizeof(char), 1 , stdin) == 1){
        byte_cout++;
        //TODO: to get word count, we can iterate 
        if(strcmp(buffer, NEWLINE) == 0){
            line_cout++;
        }
    }
    free(buffer);
    fprintf(stdout, "       %i       %i       %i\n", line_cout, word_cout, byte_cout);
    exit(0);
}