#!/bin/bash
make clean # Compilamos
make # Compilamos
lista_n="250 500 750 1000 1500 2000 2550 3000" # Valores de C para probar
lista_num_threads="1 2 4 6 8" # Valores de C para probar
for num_threads in $lista_num_threads;
do
    rm medidas_4.csv # Limpiamos el fichero de resultados
    echo 'ID_PRUEBA,N,T_TOTAL' > medidas_4.csv # Cabecera del fichero
    export OMP_NUM_THREADS=$num_threads
    echo "=========== NUMERO DE HILOS: $OMP_NUM_THREADS ==========="
    for id_prueba in {1..20}; # Repetimos las pruebas 50 veces
    do
        echo "=========== ID DE PRUEBA: $id_prueba ==========="
        for n in $lista_n; # Para cada valor de C...
        do
            echo "Ejecutando prueba con N=$n" # Informacion por pantalla
            eval "./p2_apartado4.out $n $id_prueba" # Ejecutamos el primer experimento
        done
    done
    mv medidas_4.csv "medidas_4_$num_threads.csv"
done
