#include <cstdio>
#include <cstdlib>

using namespace std;

int main() {
    int cout = 0;
    while(fgetc(stdin) != EOF){
        cout++;
    }
    fprintf(stdout, "%i\n", cout);
    exit(0);
}