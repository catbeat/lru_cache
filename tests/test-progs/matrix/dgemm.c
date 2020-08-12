#include <stdio.h>

#define SIZE 50

int main(void)
{
    int matrixA[SIZE][SIZE];
    int matrixB[SIZE][SIZE];
    int matrixC[SIZE][SIZE];

    for (int i = 0; i < SIZE; ++i){
        for (int j = 0; j < SIZE; ++j){
            for (int k = 0; k < SIZE; ++k){
                matrixC[i][j] = matrixA[i][k]*matrixB[k][j];
            }
        }
    }

    return 0;
}