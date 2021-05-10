#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NUM_COLS 8
#define LINEA_CACHE 64
#define BLOCK_SIZE 18

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

void _printMat(double **m, int filas, int cols)
{
    for (int i = 0; i < filas; i++)
    {
        printf("( ");
        for (int j = 0; j < cols; j++)
        {
            printf("%5.0lf,", m[i][j]);
        }
        printf(") \n");
    }
    printf("\n");
}

void escribir_resultado(int id_prueba, int N, double tiempo)
{
    FILE *fp;

    fp = fopen("medidas_2.csv", "a");
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

// Se encarga de trasponer una matriz, el uso que le vamos a dar es el de trasponer la matriz B para que los cálculos sean más eficientes. No se encarga de liberar la matriz anterior, sino que devuelve una nueva que es la matriz original, pero traspuesta.
double **matTraspuesta(double **mat, int filas, int cols)
{
    int i, j;
    double **matTrasp = reservarMatriz(cols, filas, gen0);

    for (i = 0; i < filas; i++)
    {
        for (j = 0; j < cols; j++)
        {
            matTrasp[j][i] = mat[i][j];
        }
    }
    return matTrasp;
}

int main(int argc, char **argv)
{
    unsigned int N, id_prueba, i, i_max, j, j_max, *ind, swap, swap_i, block_a, block_b;
    double **a, **b, **bTrasp, *c, **d, *e, f, tiempo, elem1, elem2, *lineaA, *lineaB, *lineaB2;

    // Comprobamos los argumentos que pasamos al programa
    if (argc != 3)
    {
        fprintf(stderr, "Formato del comando: %s [N] [ID de prueba]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    N = atoi(argv[1]);
    id_prueba = atoi(argv[2]);

    srand(clock()); // Semilla del generador aleatorio de números

    // Reservamos las matrices
    a = reservarMatriz(N, NUM_COLS, getElementoAleatorio);
    b = reservarMatriz(NUM_COLS, N, getElementoAleatorio);
    d = reservarMatriz(N, N, gen0);
    e = malloc(sizeof(double) * N);
    ind = malloc(sizeof(int) * N);

    c = malloc(sizeof(double) * NUM_COLS); // Reservamos memoria para C
    // Inicializamos a valores aleatorios cada elemento de C
    for (i = 0; i < NUM_COLS; i++)
    {
        c[i] = getElementoAleatorio();
    }

    // Trasponemos B. Entendemos que esto es parte del la configuracion previa, asi que no lo incluimos en el tiempo de computacion.
    bTrasp = matTraspuesta(b, NUM_COLS, N);
    liberarMatriz(b, NUM_COLS);

    // Entendemos que la inicializacion del vector no entra dentro del tiempo de computacion.
    for (i = 0; i < N; i++)
    { // Inicializamos los elementos del vector a 0, 1, 2, ...
        ind[i] = i;
    }

    for (i = 0; i < N * 5; i++)
    { // Barajamos los elementos del vector. Recorremos multiples veces el vector, haciendo intercambios de forma aleatoria.
        swap_i = rand() % N;
        swap = ind[i % N];
        ind[i % N] = ind[swap_i];
        ind[swap_i] = swap;
    }

    f = 0;

    start_counter(); // Iniciamos el contador

    // Para ahorrarnos tener que restarle c a cada columna de B a la hora de hacer los calculos, realizamos antes la computacion de esta parte.
    for (i = 0; i < N; i += 2)
    {
        lineaB = bTrasp[i];
        lineaB2 = bTrasp[i + 1];
        lineaB[0] -= c[0];
        lineaB[1] -= c[1];
        lineaB[2] -= c[2];
        lineaB[3] -= c[3];
        lineaB[4] -= c[4];
        lineaB[5] -= c[5];
        lineaB[6] -= c[6];
        lineaB[7] -= c[7];
        lineaB2[0] -= c[0];
        lineaB2[1] -= c[1];
        lineaB2[2] -= c[2];
        lineaB2[3] -= c[3];
        lineaB2[4] -= c[4];
        lineaB2[5] -= c[5];
        lineaB2[6] -= c[6];
        lineaB2[7] -= c[7];
    }

    // Realizamos la computacion principal. La hacemos por bloques para que la parte de la matriz con la que estamos trabajando quepa en la cache.
    for (block_a = 0; block_a < N; block_a += BLOCK_SIZE) // Bloque de la matriz A
    {
        i_max = block_a + BLOCK_SIZE; // i_max = min(block_a + BLOCK_SIZE, N)
        if (i_max > N)
        {
            i_max = N;
        }
        for (block_b = 0; block_b < N; block_b += BLOCK_SIZE) // Bloque de la matriz B
        {
            j_max = block_b + BLOCK_SIZE; // j_max = min(block_b + BLOCK_SIZE, N)
            if (j_max > N)
            {
                j_max = N;
            }
            for (i = block_a; i < i_max; i++) // Recorremos el bloque de la matriz A
            {
                for (j = block_b; j < j_max; j += 2) // Recorremos el bloque de la matriz B, una vez por cada fila de A en el bloque
                {
                    lineaA = a[i]; // Guardando a[i] en el stack nos ahorramos tener que calcularlo cada vez que hacemos referencia a la linea
                    lineaB = bTrasp[j];
                    lineaB2 = bTrasp[j + 1];
                    elem1 = lineaA[0] * lineaB[0]; // Podriamos inicializar antes a 0, y poner esto como la suma, pero entonces tendriamos una potencial dependencia RAW.
                    elem2 = lineaA[0] * lineaB2[0]; // Intercalamos las operaciones entre elem1 y elem2 para reducir la magnitud de las dependencias RAW.
                    elem1 += lineaA[1] * lineaB[1];
                    elem2 += lineaA[1] * lineaB2[1];
                    elem1 += lineaA[2] * lineaB[2];
                    elem2 += lineaA[2] * lineaB2[2];
                    elem1 += lineaA[3] * lineaB[3];
                    elem2 += lineaA[3] * lineaB2[3];
                    elem1 += lineaA[4] * lineaB[4];
                    elem2 += lineaA[4] * lineaB2[4];
                    elem1 += lineaA[5] * lineaB[5];
                    elem2 += lineaA[5] * lineaB2[5];
                    elem1 += lineaA[6] * lineaB[6];
                    elem2 += lineaA[6] * lineaB2[6];
                    elem1 += lineaA[7] * lineaB[7];
                    elem2 += lineaA[7] * lineaB2[7];
                    elem1 *= 2;
                    elem2 *= 2;
                    d[i][j] = elem1;
                    d[i][j + 1] = elem2;
                }
            }
        }
    }

    for (i = 0; i < N; i += 10)
    {
        e[i] = d[ind[i]][ind[i]] / 2;
        f += e[i];

        e[i + 1] = d[ind[i + 1]][ind[i + 1]] / 2;
        f += e[i + 1];

        e[i + 2] = d[ind[i + 2]][ind[i + 2]] / 2;
        f += e[i + 2];

        e[i + 3] = d[ind[i + 3]][ind[i + 3]] / 2;
        f += e[i + 3];

        e[i + 4] = d[ind[i + 4]][ind[i + 4]] / 2;
        f += e[i + 4];

        e[i + 5] = d[ind[i + 5]][ind[i + 5]] / 2;
        f += e[i + 5];

        e[i + 6] = d[ind[i + 6]][ind[i + 6]] / 2;
        f += e[i + 6];

        e[i + 7] = d[ind[i + 7]][ind[i + 7]] / 2;
        f += e[i + 7];

        e[i + 8] = d[ind[i + 8]][ind[i + 8]] / 2;
        f += e[i + 8];

        e[i + 9] = d[ind[i + 9]][ind[i + 9]] / 2;
        f += e[i + 9];
    }

    tiempo = get_counter();

    printf("Valor de f: %f\n\n", f);

    escribir_resultado(id_prueba, N, tiempo); // Escribimos los resultados en el archivo CSV

    liberarMatriz(a, N);
    liberarMatriz(bTrasp, N);
    liberarMatriz(d, N);

    free(c);
    free(e);
    free(ind);

    exit(EXIT_SUCCESS);
}
