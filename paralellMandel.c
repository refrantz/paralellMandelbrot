#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Include this header

#define WIDTH 4000
#define HEIGHT 4000
#define MAX_ITER 5000
#define WORK_TAG 1
#define SUICIDE_TAG 2

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
        fprintf(fp, "%d %d %d ", color, 0, 255 - color);  // RGB values
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    int rank, size, i, j, row;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Complex c;
    int *results = malloc(WIDTH * HEIGHT * sizeof(int));
    int *row_data = malloc(WIDTH * sizeof(int));

    if (rank == 0) {  // Master
        int num_sent = 0;  // Number of rows sent to slaves
        MPI_Status status;

        // Distribute initial rows of work to each slave
        for (i = 1; i < size; i++) {
            MPI_Send(&num_sent, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);
            num_sent++;
        }

        // Receive results and distribute remaining work
        while (num_sent < HEIGHT) {
            MPI_Recv(row_data, WIDTH, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            memcpy(results + status.MPI_TAG * WIDTH, row_data, WIDTH * sizeof(int));
            MPI_Send(&num_sent, 1, MPI_INT, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);
            num_sent++;
        }

        // Receive the final rows of data
        for (i = 1; i < size; i++) {
            MPI_Recv(row_data, WIDTH, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            memcpy(results + status.MPI_TAG * WIDTH, row_data, WIDTH * sizeof(int));
        }

        // Send termination signal
        for (i = 1; i < size; i++) {
            MPI_Send(0, 0, MPI_INT, i, SUICIDE_TAG, MPI_COMM_WORLD);
        }

        // Write results to file
        write_to_ppm(results, "output.ppm");
    } else {  // Slaves
        MPI_Status status;
        while (1) {
            MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == SUICIDE_TAG) break;

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
