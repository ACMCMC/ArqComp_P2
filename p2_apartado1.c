#include <stdlib.h>
#include <stdio.h>
#include <pmmintrin.h>
#include <time.h>

#define NUM_COLS 8

static unsigned cyc_hi = 0;
static unsigned cyc_lo = 0;
/* Set *hi and *lo to the high and low order bits of the cycle counter.
 Implementation requires assembly code to use the rdtsc instruction. */
void access_counter(unsigned *hi, unsigned *lo)
{
    asm("rdtsc; movl %%edx,%0; movl %%eax,%1" /* Read cycle counter */
        : "=r"(*hi), "=r"(*lo)                /* and move results to */
        : /* No input */                      /* the two outputs */
        : "%edx", "%eax");
}

/* Record the current value of the cycle counter. */
void start_counter()
{
    access_counter(&cyc_hi, &cyc_lo);
}

/* Return the number of cycles since the last call to start_counter. */
double get_counter()
{
    unsigned ncyc_hi, ncyc_lo;
    unsigned hi, lo, borrow;
    double result;

    /* Get cycle counter */
    access_counter(&ncyc_hi, &ncyc_lo);

    /* Do double precision subtraction */
    lo = ncyc_lo - cyc_lo;
    borrow = lo > ncyc_lo;
    hi = ncyc_hi - cyc_hi - borrow;
    result = (double)hi * (1 << 30) * 4 + lo;
    if (result < 0)
    {
        fprintf(stderr, "Error: counter returns neg value: %.0f\n", result);
    }
    return result;
}

void escribir_resultado(int id_prueba, int N, double tiempo)
{
    FILE *fp;

    fp = fopen("medidas_1.csv", "a");
    if (fp)
    {
        fprintf(fp, "%d,%d,%lf\n", id_prueba, N, tiempo);
        fclose(fp);
    }
}

double getElementoAleatorio()
{
    return ((double)rand() / RAND_MAX);
}

int main(int argc, char **argv)
{
    unsigned int N, id_prueba, i, j, k, *ind, swap, swap_i;
    double **a, **b, *c, **d, *e, f, tiempo;

    if (argc != 3)
    {
        fprintf(stderr, "Formato del comando: %s [N] [ID de prueba]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    id_prueba = atoi(argv[2]);

    srand(clock());

    a = malloc(sizeof(double *) * N);        // Reservamos memoria para los punteros a las filas de A
    b = malloc(sizeof(double *) * NUM_COLS); // Reservamos memoria para los punteros a las filas de B
    c = malloc(sizeof(double) * NUM_COLS);   // Reservamos memoria para C
    d = malloc(sizeof(double *) * N);        // Reservamos memoria para los punteros a las filas de D
    e = malloc(sizeof(double) * N);
    ind = malloc(sizeof(int) * N);

    // Reservamos memoria para las filas de A e inicializamos a valores aleatorios cada elemento
    for (i = 0; i < N; i++)
    {
        a[i] = malloc(sizeof(double) * NUM_COLS);
        for (j = 0; j < NUM_COLS; j++)
        {
            a[i][j] = getElementoAleatorio();
        }
    }

    // Reservamos memoria para las filas de B e inicializamos a valores aleatorios cada elemento
    for (i = 0; i < NUM_COLS; i++)
    {
        b[i] = malloc(sizeof(double) * N);
        for (j = 0; j < N; j++)
        {
            b[i][j] = getElementoAleatorio();
        }
    }

    // Inicializamos a valores aleatorios cada elemento de C
    for (i = 0; i < NUM_COLS; i++)
    {
        c[i] = getElementoAleatorio();
    }

    // Reservamos memoria para las filas de D e inicializamos a 0 sus elementos
    for (i = 0; i < N; i++)
    {
        d[i] = malloc(sizeof(double) * N);
        for (j = 0; j < N; j++)
        {
            d[i][j] = 0;
        }
    }

    start_counter(); // Iniciamos el contador

    // Bucle del PDF
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < 8; k++)
            {
                d[i][j] += 2 * a[i][k] * (b[k][j] - c[k]);
            }
        }
    }

    for (i = 0; i < N; i++)
    { // Inicializamos los elementos del vector a 0, 1, 2, ...
        ind[i] = i;
    }

    for (i = 0; i < N * 5; i++)
    { // Barajamos los elementos del vector. Recorremos multiples veces el vector, haciendo intercambios de forma aleatoria
        swap_i = rand() % N;
        swap = ind[i % N];
        ind[i % N] = ind[swap_i];
        ind[swap_i] = swap;
    }

    f = 0;

    for (i = 0; i < N; i++)
    {
        e[i] = d[ind[i]][ind[i]] / 2;
        f += e[i];
    }

    tiempo = get_counter();

    printf("Valor de f: %f\n", f);

    escribir_resultado(id_prueba, N, tiempo); // Escribimos los resultados en el archivo CSV

    for (i = 0; i < N; i++)
    {
        free(a[i]);
        free(d[i]);
    }
    for (i = 0; i < NUM_COLS; i++)
    {
        free(b[i]);
    }
    
    free(a);
    free(b);
    free(c);
    free(d);
    free(e);
    free(ind);

    exit(EXIT_SUCCESS);
}