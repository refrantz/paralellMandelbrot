#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 8000
#define HEIGHT 8000
#define MAX_ITER 10000
#define WORK_REQUEST 1
#define WORK_RESPONSE 2
#define SUICIDE_TAG 3

typedef struct {
    double real;
    double imag;
} Complex;

int mandelbrot(Complex c) {
    int count = 0;
    Complex z = {0, 0};
    double temp, lengthsq = 0;
    while (count < MAX_ITER && lengthsq < 4) {
        temp = z.real * z.real - z.imag * z.imag + c.real;
        z.imag = 2 * z.real * z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real * z.real + z.imag * z.imag;
        count++;
    }
    return count;
}

void write_to_ppm(int *results, const char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        int color = (int)(255 * (results[i] / (float)MAX_ITER));
        fprintf(fp, "%d %d %d ", color, 0, 255 - color);
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Complex c;
    int *results = malloc(WIDTH * HEIGHT * sizeof(int));
    int *row_data = malloc(WIDTH * sizeof(int));

    if (rank == 0) {  // Mestre
        int num_sent = 0, num_received = 0, requester, row;
        MPI_Status status;

        while (num_received < HEIGHT) {
            if (num_sent < HEIGHT) {
                MPI_Recv(&requester, 1, MPI_INT, MPI_ANY_SOURCE, WORK_REQUEST, MPI_COMM_WORLD, &status);
                row = num_sent++;
                MPI_Send(&row, 1, MPI_INT, requester, WORK_RESPONSE, MPI_COMM_WORLD);
            }

            if (num_received < num_sent) {
                MPI_Recv(row_data, WIDTH, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                memcpy(results + status.MPI_TAG * WIDTH, row_data, WIDTH * sizeof(int));
                num_received++;
            }
        }

        // Envio do sinal de finalizacao
        for (int i = 1; i < size; i++) {
            row = -1;  // Sinal de finalizacao
            MPI_Send(&row, 1, MPI_INT, i, SUICIDE_TAG, MPI_COMM_WORLD);
        }

        write_to_ppm(results, "output.ppm");
    } else {  // Escravos
        int row, j;
        MPI_Status status;

        while (1) {
            MPI_Send(&rank, 1, MPI_INT, 0, WORK_REQUEST, MPI_COMM_WORLD);
            MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == SUICIDE_TAG || row == -1) break;

            for (j = 0; j < WIDTH; j++) {
                c.real = -2.0 + j * 3.0 / WIDTH;
                c.imag = -1.5 + row * 3.0 / HEIGHT;
                row_data[j] = mandelbrot(c);
            }
            MPI_Send(row_data, WIDTH, MPI_INT, 0, row, MPI_COMM_WORLD);
        }
    }

    free(results);
    free(row_data);
    MPI_Finalize();
    return 0;
}
