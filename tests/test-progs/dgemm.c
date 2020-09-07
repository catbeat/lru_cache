#include <stdio.h>

#define SIZE 40

int main(void)
{
    int a[SIZE][SIZE];
    int b[SIZE][SIZE];
    int c[SIZE][SIZE];

    for (int i = 0; i < SIZE; ++i){
        for (int j = 0; j < SIZE; ++j){
            for (int k = 0; k < SIZE; ++k){
                c[i][j] = a[i][k]*b[k][j];
            }
        }
    }

    return 0;
}