#include <cstdio>
#include <cstdlib>

using namespace std;

int cout_lines(FILE *std_in);
int cout_words(FILE *std_in);
int cout_bytes(FILE *std_in);

int main() {
    //TODO: given that each function call will move stdin file pointer to EOF, we need to call rewind() twice because of this modular design choice
    // For performance benefits, we can perform counting by one pass through of stdin!!
    int line_cout = cout_lines(stdin);
    int word_cout = cout_words(stdin);
    int byte_cout = cout_bytes(stdin);

    exit(0);
}

int cout_lines(FILE *std_in){

}

int cout_words(FILE *std_in){
    
}

int cout_bytes(FILE *std_in){
    
}