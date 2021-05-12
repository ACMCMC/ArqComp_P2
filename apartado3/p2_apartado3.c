#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <immintrin.h>

#define NUM_COLS 8
#define LINEA_CACHE 64
#define BLOCK_SIZE 20
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
    unsigned int N, id_prueba, i, i_max, j, j_max, *ind, swap, swap_i, block_a, block_b;
    double **a, **b, **bTrasp, *c, **d, *e, f, tiempo;
    double *lineaA, *lineaB0, *lineaB1, *lineaB2, *lineaB3, *lineaB4, *lineaB5, *lineaB6, *lineaB7, *lineaB8, *lineaB9, *vectorReduccion, *vectorAuxiliar;
    __m128d regA, reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9, regResult, regResult2, regResult0_1, regResult0_2, regResult1_1, regResult1_2, regResult2_1, regResult2_2, regResult3_1, regResult3_2, regResult4_1, regResult4_2, regResult5_1, regResult5_2, regResult6_1, regResult6_2, regResult7_1, regResult7_2, regResult8_1, regResult8_2, regResult9_1, regResult9_2;

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

    vectorReduccion = _mm_malloc(2 * sizeof(double), ALINEAMIENTO); // Lo usaremos despues al computar el resultado de la suma
    vectorAuxiliar = _mm_malloc(2 * sizeof(double), ALINEAMIENTO);  // Lo usaremos despues al computar f

    start_counter(); // Iniciamos el contador

    //===========================================================
    //                        CALCULOS
    //==========================================================

    // Para ahorrarnos tener que restarle c a cada columna de B a la hora de hacer los calculos, realizamos antes la computacion de esta parte.
    for (j = 0; j < NUM_COLS; j += 2)
    {
        reg3 = _mm_load_pd(c + j);
        for (i = 0; i < N; i += 2)
        {
            lineaB0 = &bTrasp[i][j];
            lineaB1 = &bTrasp[i + 1][j];
            reg1 = _mm_load_pd(lineaB0);
            reg2 = _mm_load_pd(lineaB1);
            regResult = _mm_sub_pd(reg1, reg3);
            regResult2 = _mm_sub_pd(reg2, reg3);
            _mm_store_pd(lineaB0, regResult);
            _mm_store_pd(lineaB1, regResult2);
        }
    }

    // Realizamos la computacion principal. La hacemos por bloques para que la parte de la matriz con la que estamos trabajando quepa en la cache.
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
                    lineaA = a[i];       // Guardando a[i] en el stack nos ahorramos tener que calcularlo cada vez que hacemos referencia a la linea
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

                    //Para que el codigo sea mas rapido, vamos a usar un registro donde ir acumulando resultados (regResultX_2), y uno donde ir calculando las multiplicaciones que sumaremos en ese registro (regResultX_1).
                    regA = _mm_load_pd(lineaA);  // reg1 = (a[i][0] , a[i][1])
                    reg0 = _mm_load_pd(lineaB0); // reg0 = (b[j][0] , b[j][1])
                    reg1 = _mm_load_pd(lineaB1); // ...
                    reg2 = _mm_load_pd(lineaB2);
                    reg3 = _mm_load_pd(lineaB3);
                    reg4 = _mm_load_pd(lineaB4);
                    reg5 = _mm_load_pd(lineaB5);
                    reg6 = _mm_load_pd(lineaB6);
                    reg7 = _mm_load_pd(lineaB7);
                    reg8 = _mm_load_pd(lineaB8);
                    reg9 = _mm_load_pd(lineaB9);
                    regResult0_2 = _mm_mul_pd(regA, reg0); // regResult0_2 = (a[i][0] * b[j][0] , a[i][1] * b[j][1]). En la primera iteracion del bucle usamos el registro regResult0_2, porque podemos inicializarlo al valor de la primera multiplicacion. Otra opcion seria inicializar regResult0_2 a (0 , 0), calcular el resultado de la multiplicacion en regResultX_1, y sumárselo a regResultX_2. Esto es lo que haremos a partir de aqui, pero en este caso no es necesario, ya que asumimos que regResultX_2 empieza en (0 , 0)
                    regResult1_2 = _mm_mul_pd(regA, reg1); // regResult1_2 = (a[i][0] * b[j+1][0] , a[i][1] * b[j+1][1])
                    regResult2_2 = _mm_mul_pd(regA, reg2);
                    regResult3_2 = _mm_mul_pd(regA, reg3);
                    regResult4_2 = _mm_mul_pd(regA, reg4);
                    regResult5_2 = _mm_mul_pd(regA, reg5);
                    regResult6_2 = _mm_mul_pd(regA, reg6);
                    regResult7_2 = _mm_mul_pd(regA, reg7);
                    regResult8_2 = _mm_mul_pd(regA, reg8);
                    regResult9_2 = _mm_mul_pd(regA, reg9);

                    // A partir de aqui, si que tendremos que usar regResultX_1 y regResultX_2
                    regA = _mm_load_pd(lineaA + 2);
                    reg0 = _mm_load_pd(lineaB0 + 2);
                    reg1 = _mm_load_pd(lineaB1 + 2);
                    reg2 = _mm_load_pd(lineaB2 + 2);
                    reg3 = _mm_load_pd(lineaB3 + 2);
                    reg4 = _mm_load_pd(lineaB4 + 2);
                    reg5 = _mm_load_pd(lineaB5 + 2);
                    reg6 = _mm_load_pd(lineaB6 + 2);
                    reg7 = _mm_load_pd(lineaB7 + 2);
                    reg8 = _mm_load_pd(lineaB8 + 2);
                    reg9 = _mm_load_pd(lineaB9 + 2);
                    regResult0_1 = _mm_mul_pd(regA, reg0); // regResult0_1 = (a[i][0] * b[j][0] , a[i][1] * b[j][1]).
                    regResult1_1 = _mm_mul_pd(regA, reg1);
                    regResult2_1 = _mm_mul_pd(regA, reg2);
                    regResult3_1 = _mm_mul_pd(regA, reg3);
                    regResult4_1 = _mm_mul_pd(regA, reg4);
                    regResult5_1 = _mm_mul_pd(regA, reg5);
                    regResult6_1 = _mm_mul_pd(regA, reg6);
                    regResult7_1 = _mm_mul_pd(regA, reg7);
                    regResult8_1 = _mm_mul_pd(regA, reg8);
                    regResult9_1 = _mm_mul_pd(regA, reg9);
                    regResult0_2 = _mm_add_pd(regResult0_2, regResult0_1);
                    regResult1_2 = _mm_add_pd(regResult1_2, regResult1_1);
                    regResult2_2 = _mm_add_pd(regResult2_2, regResult2_1);
                    regResult3_2 = _mm_add_pd(regResult3_2, regResult3_1);
                    regResult4_2 = _mm_add_pd(regResult4_2, regResult4_1);
                    regResult5_2 = _mm_add_pd(regResult5_2, regResult5_1);
                    regResult6_2 = _mm_add_pd(regResult6_2, regResult6_1);
                    regResult7_2 = _mm_add_pd(regResult7_2, regResult7_1);
                    regResult8_2 = _mm_add_pd(regResult8_2, regResult8_1);
                    regResult9_2 = _mm_add_pd(regResult9_2, regResult9_1);

                    regA = _mm_load_pd(lineaA + 4);
                    reg0 = _mm_load_pd(lineaB0 + 4);
                    reg1 = _mm_load_pd(lineaB1 + 4);
                    reg2 = _mm_load_pd(lineaB2 + 4);
                    reg3 = _mm_load_pd(lineaB3 + 4);
                    reg4 = _mm_load_pd(lineaB4 + 4);
                    reg5 = _mm_load_pd(lineaB5 + 4);
                    reg6 = _mm_load_pd(lineaB6 + 4);
                    reg7 = _mm_load_pd(lineaB7 + 4);
                    reg8 = _mm_load_pd(lineaB8 + 4);
                    reg9 = _mm_load_pd(lineaB9 + 4);
                    regResult0_1 = _mm_mul_pd(regA, reg0); // regResult0_1 = (a[i][0] * b[j][0] , a[i][1] * b[j][1]).
                    regResult1_1 = _mm_mul_pd(regA, reg1);
                    regResult2_1 = _mm_mul_pd(regA, reg2);
                    regResult3_1 = _mm_mul_pd(regA, reg3);
                    regResult4_1 = _mm_mul_pd(regA, reg4);
                    regResult5_1 = _mm_mul_pd(regA, reg5);
                    regResult6_1 = _mm_mul_pd(regA, reg6);
                    regResult7_1 = _mm_mul_pd(regA, reg7);
                    regResult8_1 = _mm_mul_pd(regA, reg8);
                    regResult9_1 = _mm_mul_pd(regA, reg9);
                    regResult0_2 = _mm_add_pd(regResult0_2, regResult0_1);
                    regResult1_2 = _mm_add_pd(regResult1_2, regResult1_1);
                    regResult2_2 = _mm_add_pd(regResult2_2, regResult2_1);
                    regResult3_2 = _mm_add_pd(regResult3_2, regResult3_1);
                    regResult4_2 = _mm_add_pd(regResult4_2, regResult4_1);
                    regResult5_2 = _mm_add_pd(regResult5_2, regResult5_1);
                    regResult6_2 = _mm_add_pd(regResult6_2, regResult6_1);
                    regResult7_2 = _mm_add_pd(regResult7_2, regResult7_1);
                    regResult8_2 = _mm_add_pd(regResult8_2, regResult8_1);
                    regResult9_2 = _mm_add_pd(regResult9_2, regResult9_1);

                    regA = _mm_load_pd(lineaA + 6);
                    reg0 = _mm_load_pd(lineaB0 + 6);
                    reg1 = _mm_load_pd(lineaB1 + 6);
                    reg2 = _mm_load_pd(lineaB2 + 6);
                    reg3 = _mm_load_pd(lineaB3 + 6);
                    reg4 = _mm_load_pd(lineaB4 + 6);
                    reg5 = _mm_load_pd(lineaB5 + 6);
                    reg6 = _mm_load_pd(lineaB6 + 6);
                    reg7 = _mm_load_pd(lineaB7 + 6);
                    reg8 = _mm_load_pd(lineaB8 + 6);
                    reg9 = _mm_load_pd(lineaB9 + 6);
                    regResult0_1 = _mm_mul_pd(regA, reg0); // regResult0_1 = (a[i][0] * b[j][0] , a[i][1] * b[j][1]).
                    regResult1_1 = _mm_mul_pd(regA, reg1);
                    regResult2_1 = _mm_mul_pd(regA, reg2);
                    regResult3_1 = _mm_mul_pd(regA, reg3);
                    regResult4_1 = _mm_mul_pd(regA, reg4);
                    regResult5_1 = _mm_mul_pd(regA, reg5);
                    regResult6_1 = _mm_mul_pd(regA, reg6);
                    regResult7_1 = _mm_mul_pd(regA, reg7);
                    regResult8_1 = _mm_mul_pd(regA, reg8);
                    regResult9_1 = _mm_mul_pd(regA, reg9);
                    regResult0_2 = _mm_add_pd(regResult0_2, regResult0_1);
                    regResult1_2 = _mm_add_pd(regResult1_2, regResult1_1);
                    regResult2_2 = _mm_add_pd(regResult2_2, regResult2_1);
                    regResult3_2 = _mm_add_pd(regResult3_2, regResult3_1);
                    regResult4_2 = _mm_add_pd(regResult4_2, regResult4_1);
                    regResult5_2 = _mm_add_pd(regResult5_2, regResult5_1);
                    regResult6_2 = _mm_add_pd(regResult6_2, regResult6_1);
                    regResult7_2 = _mm_add_pd(regResult7_2, regResult7_1);
                    regResult8_2 = _mm_add_pd(regResult8_2, regResult8_1);
                    regResult9_2 = _mm_add_pd(regResult9_2, regResult9_1);

                    _mm_store_pd(vectorReduccion, regResult0_2);             // Tenemos que sumar los dos elementos para que pasen a ser solamente 1, este paso no se puede hacer de forma vectorizada
                    d[i][j] = 2 * (vectorReduccion[0] + vectorReduccion[1]); // d[i][j] = ( regResult[0] + regResult[1] ) * 2

                    _mm_store_pd(vectorReduccion, regResult1_2);
                    d[i][j + 1] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult2_2);
                    d[i][j + 2] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult3_2);
                    d[i][j + 3] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult4_2);
                    d[i][j + 4] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult5_2);
                    d[i][j + 5] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult6_2);
                    d[i][j + 6] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult7_2);
                    d[i][j + 7] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult8_2);
                    d[i][j + 8] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                    _mm_store_pd(vectorReduccion, regResult9_2);
                    d[i][j + 9] = 2 * (vectorReduccion[0] + vectorReduccion[1]);
                }
            }
        }
    }

    regResult = _mm_set1_pd(0); // regResult = (0 , 0)
    reg2 = _mm_set1_pd(2);      // reg2 = (2 , 2)
    for (i = 0; i < N; i += 10)
    {
        // Necesitamos usar el vector auxiliar porque d[ind[i]][ind[i]] y d[ind[i+1]][ind[i+1]] no son elementos contiguos en memoria. Esta operacion va a hacer que usar vectorizacion no sea eficiente.
        vectorAuxiliar[0] = d[ind[i]][ind[i]];
        vectorAuxiliar[1] = d[ind[i + 1]][ind[i + 1]];

        reg1 = _mm_load_pd(vectorAuxiliar);
        vectorAuxiliar[0] = d[ind[i + 2]][ind[i + 2]];
        vectorAuxiliar[1] = d[ind[i + 3]][ind[i + 3]];
        reg3 = _mm_div_pd(reg1, reg2);
        regResult = _mm_add_pd(regResult, reg3);

        reg1 = _mm_load_pd(vectorAuxiliar);
        vectorAuxiliar[0] = d[ind[i + 4]][ind[i + 4]];
        vectorAuxiliar[1] = d[ind[i + 5]][ind[i + 5]];
        reg3 = _mm_div_pd(reg1, reg2);
        regResult = _mm_add_pd(regResult, reg3);

        reg1 = _mm_load_pd(vectorAuxiliar);
        vectorAuxiliar[0] = d[ind[i + 6]][ind[i + 6]];
        vectorAuxiliar[1] = d[ind[i + 7]][ind[i + 7]];
        reg3 = _mm_div_pd(reg1, reg2);
        regResult = _mm_add_pd(regResult, reg3);

        reg1 = _mm_load_pd(vectorAuxiliar);
        vectorAuxiliar[0] = d[ind[i + 8]][ind[i + 8]];
        vectorAuxiliar[1] = d[ind[i + 9]][ind[i + 9]];
        reg3 = _mm_div_pd(reg1, reg2);
        regResult = _mm_add_pd(regResult, reg3);

        reg1 = _mm_load_pd(vectorAuxiliar);
        reg3 = _mm_div_pd(reg1, reg2);
        regResult = _mm_add_pd(regResult, reg3);
    }
    _mm_store_pd(vectorReduccion, regResult);
    f = (vectorReduccion[0] + vectorReduccion[1]);

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
    _mm_free(c);
    _mm_free(e);
    _mm_free(ind);
    _mm_free(vectorReduccion);
    _mm_free(vectorAuxiliar);

    exit(EXIT_SUCCESS);
}
