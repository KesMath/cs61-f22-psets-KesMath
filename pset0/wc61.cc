#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>

using namespace std;
const char* NEWLINE = "\n";

int main() {    
    int line_cout = 0, word_cout = 0, byte_cout = 0, alpha_cout = 0;
    char* buffer = (char*) calloc(1, sizeof(char));
    while(fread(buffer, sizeof(char), 1 , stdin) == 1){
        if(isalpha(*buffer)){
            alpha_cout++;
        }
        else{
            if(strcmp(buffer, NEWLINE) == 0){
                line_cout++;
            }
            else if(isspace(*buffer) && alpha_cout > 0){
                word_cout++;
                alpha_cout = 0;
            }
        }
        byte_cout++;    
    }
    free(buffer);

    if(alpha_cout > 0){
        word_cout++;
    }
    fprintf(stdout, "       %i       %i       %i\n", line_cout, word_cout, byte_cout);
    exit(0);
}