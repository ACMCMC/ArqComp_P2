#include <stdlib.h>
#include <stdio.h>
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

void _printMat(double** m, int filas, int cols) {
    for (int i = 0; i < filas; i++) {
        printf("( ");
        for (int j = 0; j < cols; j++) {
            printf("%5.0lf,", m[i][j]);
        }
        printf(") \n");
    }
    printf("\n");
}

void escribir_resultado(int id_prueba, int N, double tiempo)
{
    FILE *fp;

    fp = fopen("medidas_1.csv", "a");
    if (fp)
    {
        fprintf(fp, "%d,%d,%.0lf\n", id_prueba, N, tiempo);
        fclose(fp);
    }
}

// Función que devuelve un 0
double gen0()
{
    return 0;
}
// Función que devuelve un 1
double gen1()
{
    return 1;
}
// Función que devuelve un 2
double gen2()
{
    return 2;
}

// Función que devuelve un número aleatorio entre 0 y 1
double getElementoAleatorio()
{
    return ((double)rand() / RAND_MAX);
}

// Se encarga de reservar las filas de la matriz y el vector que apunta a las filas. Además usa la función parámetro para generar los elementos de la matriz.
double **reservarMatriz(int filas, int cols, double(fun)())
{
    int i, j;
    double **mat;

    mat = malloc(sizeof(double) * filas);

    for (i = 0; i < filas; i++)
    {
        mat[i] = malloc(sizeof(double) * cols);
        for (j = 0; j < cols; j++)
        {
            mat[i][j] = fun();
        }
    }

    return mat;
}

// Se encarga de liberar una matriz. No necesita conocer el número de columnas, ya que llega con liberar las filas y el vector de punteros a las filas.
void liberarMatriz(double **mat, int filas)
{
    int i;
    for (i = 0; i < filas; i++)
    {
        free(mat[i]);
    }

    free(mat);
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

    a = reservarMatriz(N, NUM_COLS, getElementoAleatorio);
    b = reservarMatriz(NUM_COLS, N, getElementoAleatorio);
    d = reservarMatriz(N, N, gen0);
    c = malloc(sizeof(double) * NUM_COLS);   // Reservamos memoria para C
    e = malloc(sizeof(double) * N);
    ind = malloc(sizeof(int) * N);

    // Inicializamos a valores aleatorios cada elemento de C
    for (i = 0; i < NUM_COLS; i++)
    {
        c[i] = getElementoAleatorio();
    }

    // Entendemos que la inicialización del vector no entra dentro del tiempo de computación
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
    {
        e[i] = d[ind[i]][ind[i]] / 2;
        f += e[i];
    }

    tiempo = get_counter();

    printf("Valor de f: %f\n\n", f);

    escribir_resultado(id_prueba, N, tiempo); // Escribimos los resultados en el archivo CSV

    // Liberamos memoria
    liberarMatriz(a, N);
    liberarMatriz(b, NUM_COLS);
    liberarMatriz(d, N);
    free(c);
    free(e);
    free(ind);

    exit(EXIT_SUCCESS);
}
