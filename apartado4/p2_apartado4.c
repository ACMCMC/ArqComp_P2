#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>

#define NUM_COLS 8
#define LINEA_CACHE 64
#define BLOCK_SIZE 20

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

    fp = fopen("medidas_4.csv", "a");
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

void liberarMatriz(double **mat, int filas)
{
    int i;
    for (i = 0; i < filas; i++)
    {
        free(mat[i]);
    }

    free(mat);
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
    double elem0, elem1, elem2, elem3, elem4, elem5, elem6, elem7, elem8, elem9, *lineaA, *lineaB0, *lineaB1, *lineaB2, *lineaB3, *lineaB4, *lineaB5, *lineaB6, *lineaB7, *lineaB8, *lineaB9, elemA, elemA2, acumulador;

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
    e = malloc(sizeof(double) * N);
    ind = malloc(sizeof(int) * N);

    c = malloc(sizeof(double) * NUM_COLS); // Reservamos memoria para C
    // Inicializamos a valores aleatorios cada elemento de C
    for (i = 0; i < NUM_COLS; i++)
    {
        c[i] = gen1();
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

    //===========================================================
    //                        CALCULOS
    //==========================================================

    // Para ahorrarnos tener que restarle c a cada columna de B a la hora de hacer los calculos, realizamos antes la computacion de esta parte.
    #pragma omp parallel private(i,lineaB0,lineaB1)
    {

    #pragma omp for
    for (i = 0; i < N; i += 2)
    {
        lineaB0 = bTrasp[i];
        lineaB1 = bTrasp[i + 1];
        lineaB0[0] -= c[0];
        lineaB0[1] -= c[1];
        lineaB0[2] -= c[2];
        lineaB0[3] -= c[3];
        lineaB0[4] -= c[4];
        lineaB0[5] -= c[5];
        lineaB0[6] -= c[6];
        lineaB0[7] -= c[7];
        lineaB1[0] -= c[0];
        lineaB1[1] -= c[1];
        lineaB1[2] -= c[2];
        lineaB1[3] -= c[3];
        lineaB1[4] -= c[4];
        lineaB1[5] -= c[5];
        lineaB1[6] -= c[6];
        lineaB1[7] -= c[7];
    }

    }

    // Realizamos la computacion principal. La hacemos por bloques para que la parte de la matriz con la que estamos trabajando quepa en la cache.
    #pragma omp parallel private(i_max,j_max,i,j,block_a,block_b,elemA,elemA2,elem0,elem1,elem2,elem3,elem4,elem5,elem6,elem7,elem8,elem9,lineaA,lineaB0,lineaB1,lineaB2,lineaB3,lineaB4,lineaB5,lineaB6,lineaB7,lineaB8,lineaB9)
    {
    
    #pragma omp for
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
                for (j = block_b; j < j_max; j += 10) // Recorremos el bloque de la matriz B, una vez por cada fila de A en el bloque
                {
                    lineaA = a[i]; // Guardando a[i] en el stack nos ahorramos tener que calcularlo cada vez que hacemos referencia a la linea
                    elemA = lineaA[0]; // Cargamos el primer elemento que vamos a usar en memoria adelantadamente
                    lineaB0 = bTrasp[j]; // La lógica de precalcular estas líneas es la misma que para lineaA
                    lineaB1 = bTrasp[j + 1];
                    lineaB2 = bTrasp[j + 2];
                    lineaB3 = bTrasp[j + 3];
                    lineaB4 = bTrasp[j + 4];
                    lineaB5 = bTrasp[j + 5];
                    lineaB6 = bTrasp[j + 6];
                    lineaB7 = bTrasp[j + 7];
                    lineaB8 = bTrasp[j + 8];
                    lineaB9 = bTrasp[j + 9];

                    elemA2 = lineaA[1]; // Vamos cargando el elemento que usaremos después
                    elem0 = elemA * lineaB0[0]; // Podriamos inicializar antes a 0, y poner esto como la suma, pero entonces tendriamos una potencial dependencia RAW innecesaria.
                    elem1 = elemA * lineaB1[0];
                    elem2 = elemA * lineaB2[0];
                    elem3 = elemA * lineaB3[0];
                    elem4 = elemA * lineaB4[0];
                    elem5 = elemA * lineaB5[0];
                    elem6 = elemA * lineaB6[0];
                    elem7 = elemA * lineaB7[0];
                    elem8 = elemA * lineaB8[0];
                    elem9 = elemA * lineaB9[0];
                    elemA = lineaA[2];
                    elem0 += elemA2 * lineaB0[1]; // Aquí es donde usamos elemA2
                    elem1 += elemA2 * lineaB1[1];
                    elem2 += elemA2 * lineaB2[1];
                    elem3 += elemA2 * lineaB3[1];
                    elem4 += elemA2 * lineaB4[1];
                    elem5 += elemA2 * lineaB5[1];
                    elem6 += elemA2 * lineaB6[1];
                    elem7 += elemA2 * lineaB7[1];
                    elem8 += elemA2 * lineaB8[1];
                    elem9 += elemA2 * lineaB9[1];
                    elemA2 = lineaA[3];
                    elem0 += elemA * lineaB0[2];
                    elem1 += elemA * lineaB1[2];
                    elem2 += elemA * lineaB2[2];
                    elem3 += elemA * lineaB3[2];
                    elem4 += elemA * lineaB4[2];
                    elem5 += elemA * lineaB5[2];
                    elem6 += elemA * lineaB6[2];
                    elem7 += elemA * lineaB7[2];
                    elem8 += elemA * lineaB8[2];
                    elem9 += elemA * lineaB9[2];
                    elemA = lineaA[4];
                    elem0 += elemA2 * lineaB0[3];
                    elem1 += elemA2 * lineaB1[3];
                    elem2 += elemA2 * lineaB2[3];
                    elem3 += elemA2 * lineaB3[3];
                    elem4 += elemA2 * lineaB4[3];
                    elem5 += elemA2 * lineaB5[3];
                    elem6 += elemA2 * lineaB6[3];
                    elem7 += elemA2 * lineaB7[3];
                    elem8 += elemA2 * lineaB8[3];
                    elem9 += elemA2 * lineaB9[3];
                    elemA2 = lineaA[5];
                    elem0 += elemA * lineaB0[4];
                    elem1 += elemA * lineaB1[4];
                    elem2 += elemA * lineaB2[4];
                    elem3 += elemA * lineaB3[4];
                    elem4 += elemA * lineaB4[4];
                    elem5 += elemA * lineaB5[4];
                    elem6 += elemA * lineaB6[4];
                    elem7 += elemA * lineaB7[4];
                    elem8 += elemA * lineaB8[4];
                    elem9 += elemA * lineaB9[4];
                    elemA = lineaA[6];
                    elem0 += elemA2 * lineaB0[5];
                    elem1 += elemA2 * lineaB1[5];
                    elem2 += elemA2 * lineaB2[5];
                    elem3 += elemA2 * lineaB3[5];
                    elem4 += elemA2 * lineaB4[5];
                    elem5 += elemA2 * lineaB5[5];
                    elem6 += elemA2 * lineaB6[5];
                    elem7 += elemA2 * lineaB7[5];
                    elem8 += elemA2 * lineaB8[5];
                    elem9 += elemA2 * lineaB9[5];
                    elemA2 = lineaA[7];
                    elem0 += elemA * lineaB0[6];
                    elem1 += elemA * lineaB1[6];
                    elem2 += elemA * lineaB2[6];
                    elem3 += elemA * lineaB3[6];
                    elem4 += elemA * lineaB4[6];
                    elem5 += elemA * lineaB5[6];
                    elem6 += elemA * lineaB6[6];
                    elem7 += elemA * lineaB7[6];
                    elem8 += elemA * lineaB8[6];
                    elem9 += elemA * lineaB9[6];
                    elem0 += elemA2 * lineaB0[7];
                    elem1 += elemA2 * lineaB1[7];
                    elem2 += elemA2 * lineaB2[7];
                    elem3 += elemA2 * lineaB3[7];
                    elem4 += elemA2 * lineaB4[7];
                    elem5 += elemA2 * lineaB5[7];
                    elem6 += elemA2 * lineaB6[7];
                    elem7 += elemA2 * lineaB7[7];
                    elem8 += elemA2 * lineaB8[7];
                    elem9 += elemA2 * lineaB9[7];

                    d[i][j] = elem0 * 2; // Guardamos el valor en la matriz
                    d[i][j + 1] = elem1 * 2;
                    d[i][j + 2] = elem2 * 2;
                    d[i][j + 3] = elem3 * 2;
                    d[i][j + 4] = elem4 * 2;
                    d[i][j + 5] = elem5 * 2;
                    d[i][j + 6] = elem6 * 2;
                    d[i][j + 7] = elem7 * 2;
                    d[i][j + 8] = elem8 * 2;
                    d[i][j + 9] = elem9 * 2;
                }
            }
        }
    }

    }

    #pragma omp parallel private(i,acumulador)
    {

    #pragma omp for
    for (i = 0; i < N; i += 10)
    {
	    acumulador = 0;
        e[i] = d[ind[i]][ind[i]] / 2;
        acumulador += e[i];

        e[i + 1] = d[ind[i + 1]][ind[i + 1]] / 2;
        acumulador += e[i+1];

        e[i + 2] = d[ind[i + 2]][ind[i + 2]] / 2;
        acumulador += e[i+2];

        e[i + 3] = d[ind[i + 3]][ind[i + 3]] / 2;
        acumulador += e[i+3];

        e[i + 4] = d[ind[i + 4]][ind[i + 4]] / 2;
        acumulador += e[i+4];

        e[i + 5] = d[ind[i + 5]][ind[i + 5]] / 2;
        acumulador += e[i+5];

        e[i + 6] = d[ind[i + 6]][ind[i + 6]] / 2;
        acumulador += e[i+6];

        e[i + 7] = d[ind[i + 7]][ind[i + 7]] / 2;
        acumulador += e[i+7];

        e[i + 8] = d[ind[i + 8]][ind[i + 8]] / 2;
        acumulador += e[i+8];

        e[i + 9] = d[ind[i + 9]][ind[i + 9]] / 2;
        acumulador += e[i+9];
	
        #pragma omp atomic
	    f+=acumulador;
    }

    }

    //===========================================================
    //                    FIN DE LOS CALCULOS
    //===========================================================

    tiempo = get_counter();

    printf("Valor de f: %f\n\n", f);

    escribir_resultado(id_prueba, N, tiempo); // Escribimos los resultados en el archivo CSV

    // Liberamos memoria
    liberarMatriz(a, N);
    liberarMatriz(bTrasp, N);
    liberarMatriz(d, N);
    free(c);
    free(e);
    free(ind);

    exit(EXIT_SUCCESS);
}
