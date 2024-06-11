#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define WIDTH 4000
#define HEIGHT 4000
#define MAX_ITER 5000
#define ROWS_PER_CHUNK 10

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
        int count = results[i];
        int color = (int)(255 * (count / (float)MAX_ITER));
        fprintf(fp, "%d %d %d ", color, 0, 255 - color);
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    int rank, size, provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *results = NULL, *recv_buffer = NULL;
    if (rank == 0) {
        results = malloc(WIDTH * HEIGHT * sizeof(int));
        if (results == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    } else {
        recv_buffer = malloc(WIDTH * ROWS_PER_CHUNK * sizeof(int));
        if (recv_buffer == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    int rows_remaining = HEIGHT;
    MPI_Status status;
    int request, start_row;

    while (1) {
        if (rank == 0) {
            MPI_Recv(&request, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (rows_remaining > 0) {
                int rows_to_send = (rows_remaining < ROWS_PER_CHUNK) ? rows_remaining : ROWS_PER_CHUNK;
                start_row = HEIGHT - rows_remaining;
                rows_remaining -= rows_to_send;
                MPI_Send(&start_row, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                MPI_Send(&rows_to_send, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            } else {
                int stop_signal = -1;
                MPI_Send(&stop_signal, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            }
        } else {
            MPI_Send(&request, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Recv(&start_row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (start_row == -1) break;
            MPI_Recv(&request, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < request; i++) {
                for (int j = 0; j < WIDTH; j++) {
                    Complex c;
                    c.real = -2.0 + j * 3.0 / WIDTH;
                    c.imag = -1.5 + (start_row + i) * 3.0 / HEIGHT;
                    recv_buffer[i * WIDTH + j] = mandelbrot(c);
                }
            }
            MPI_Send(recv_buffer, request * WIDTH, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        write_to_ppm(results, "output.ppm");
        free(results);
    } else {
        free(recv_buffer);
    }

    MPI_Finalize();
    return 0;
}
