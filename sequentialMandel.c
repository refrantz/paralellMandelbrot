#include <stdio.h>
#include <stdlib.h>

#define WIDTH 2000
#define HEIGHT 2000
#define MAX_ITER 5000

// definicao de um número complexo
typedef struct {
    double real;
    double imag;
} Complex;

// funcao que determina se um ponto pertence ao conjunto de Mandelbrot
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

// escreve os resultados no PPM
void write_to_ppm(int *results, const char *filename) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        int count = results[i];
        int color = (int)(255 * (count / (float)MAX_ITER));
        fprintf(fp, "%d %d %d ", color, 0, 255 - color);  // Valores RGB
    }
    fclose(fp);
}
//  principal sequencial
int main() {
    Complex c;
    int *results = malloc(WIDTH * HEIGHT * sizeof(int));
    if (results == NULL) {
        fprintf(stderr, "Alocacao de memoria falhou\n");
        return 1;
    }

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            c.real = -2.0 + j * 3.0 / WIDTH;
            c.imag = -1.5 + i * 3.0 / HEIGHT;
            results[i * WIDTH + j] = mandelbrot(c);
        }
    }

    write_to_ppm(results, "output.ppm");

    free(results);
    return 0;
}
