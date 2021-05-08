#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <immintrin.h>

#define NUM_COLS 8
#define LINEA_CACHE 64
#define BLOCK_SIZE 18
#define ALINEAMIENTO 16

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

    fp = fopen("medidas_3.csv", "a");
    if (fp)
    {
        fprintf(fp, "%d,%d,%.0lf\n", id_prueba, N, tiempo);
        fclose(fp);
    }
}

double gen0()
{
    return 0;
}

// a y c a 1
double gen1()
{
    return 1;
}
//b a 2
double gen2()
{
    return 2;
}
//dan 8*N

double getElementoAleatorio()
{
    return ((double)rand() / RAND_MAX);
}

double **reservarMatriz(int filas, int cols, double(fun)())
{
    int i, j;
    double **mat;

    mat = _mm_malloc(sizeof(double) * filas, ALINEAMIENTO);

    for (i = 0; i < filas; i++)
    {
        mat[i] = _mm_malloc(sizeof(double) * cols, ALINEAMIENTO);
        for (j = 0; j < cols; j++)
        {
            mat[i][j] = fun();
        }
    }

    return mat;
}

void liberarMatriz(double **mat, int filas)
{
    int i;
    for (i = 0; i < filas; i++)
    {
        _mm_free(mat[i]);
    }

    _mm_free(mat);
}

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
    unsigned int N, id_prueba, i, i_max, j, j_max, k, *ind, swap, swap_i, block_a, block_b;
    double **a, **b, **bTrasp, *c, **d, *e, f, tiempo;
    double elem1, elem2, *lineaA, *lineaB, *lineaB2;
    __m128d reg1, reg2, regResult;

    if (argc != 3)
    {
        fprintf(stderr, "Formato del comando: %s [N] [ID de prueba]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    id_prueba = atoi(argv[2]);

    srand(clock());

    a = reservarMatriz(N, NUM_COLS, gen1);
    b = reservarMatriz(NUM_COLS, N, gen2);
    d = reservarMatriz(N, N, gen0);
    e = _mm_malloc(sizeof(double) * N, ALINEAMIENTO);
    ind = _mm_malloc(sizeof(int) * N, ALINEAMIENTO);

    c = _mm_malloc(sizeof(double) * NUM_COLS, ALINEAMIENTO); // Reservamos memoria para C
    // Inicializamos a valores aleatorios cada elemento de C
    for (i = 0; i < NUM_COLS; i++)
    {
        c[i] = 1;
    }

    bTrasp = matTraspuesta(b, NUM_COLS, N);
    liberarMatriz(b, NUM_COLS);

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

    for (i = 0; i < N; i += 2)
    {
        lineaB = bTrasp[i];
        lineaB2 = bTrasp[i + 1];
        reg1 = _mm_load_pd(lineaB);
        reg2 = _mm_load_pd(c);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB, regResult);
        reg1 = _mm_load_pd(lineaB + 2);
        reg2 = _mm_load_pd(c + 2);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB + 2, regResult);
        reg1 = _mm_load_pd(lineaB + 4);
        reg2 = _mm_load_pd(c + 4);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB + 4, regResult);
        reg1 = _mm_load_pd(lineaB + 6);
        reg2 = _mm_load_pd(c + 6);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB + 6, regResult);
        reg1 = _mm_load_pd(lineaB2);
        reg2 = _mm_load_pd(c);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB2, regResult);
        reg1 = _mm_load_pd(lineaB2 + 2);
        reg2 = _mm_load_pd(c + 2);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB2 + 2, regResult);
        reg1 = _mm_load_pd(lineaB2 + 4);
        reg2 = _mm_load_pd(c + 4);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB2 + 4, regResult);
        reg1 = _mm_load_pd(lineaB2 + 6);
        reg2 = _mm_load_pd(c + 6);
        regResult = _mm_sub_pd(reg1, reg2);
        _mm_store_pd(lineaB2 + 6, regResult);
    }

    for (block_a = 0; block_a < N; block_a += BLOCK_SIZE) // Bloque de la matriz A
    {
        i_max = block_a + BLOCK_SIZE;
        if (i_max > N)
        {
            i_max = N;
        }
        for (block_b = 0; block_b < N; block_b += BLOCK_SIZE) // Bloque de la matriz B
        {
            j_max = block_b + BLOCK_SIZE;
            if (j_max > N)
            {
                j_max = N;
            }
            for (i = block_a; i < i_max; i++) // Recorremos el bloque de la matriz A
            {
                for (j = block_b; j < j_max; j += 2) // Recorremos el bloque de la matriz B, una vez por cada fila de A en el bloque
                {
                    lineaA = a[i];
                    lineaB = bTrasp[j];
                    lineaB2 = bTrasp[j + 1];
                    elem1 = 0;
                    elem2 = 0;
                    elem1 += lineaA[0] * lineaB[0];
                    elem1 += lineaA[1] * lineaB[1];
                    elem1 += lineaA[2] * lineaB[2];
                    elem1 += lineaA[3] * lineaB[3];
                    elem1 += lineaA[4] * lineaB[4];
                    elem1 += lineaA[5] * lineaB[5];
                    elem1 += lineaA[6] * lineaB[6];
                    elem1 += lineaA[7] * lineaB[7];
                    elem1 *= 2;
                    d[i][j] = elem1;
                    elem2 += lineaA[0] * lineaB2[0];
                    elem2 += lineaA[1] * lineaB2[1];
                    elem2 += lineaA[2] * lineaB2[2];
                    elem2 += lineaA[3] * lineaB2[3];
                    elem2 += lineaA[4] * lineaB2[4];
                    elem2 += lineaA[5] * lineaB2[5];
                    elem2 += lineaA[6] * lineaB2[6];
                    elem2 += lineaA[7] * lineaB2[7];
                    elem2 *= 2;
                    d[i][j + 1] = elem2;
                }
            }
        }
    }

    for (i = 0; i < N; i += 5)
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
    }

    tiempo = get_counter();

    printf("Valor de f: %f\n\n", f);

    escribir_resultado(id_prueba, N, tiempo); // Escribimos los resultados en el archivo CSV

    liberarMatriz(a, N);
    liberarMatriz(bTrasp, N);
    liberarMatriz(d, N);

    _mm_free(c);
    _mm_free(e);
    _mm_free(ind);

    exit(EXIT_SUCCESS);
}
