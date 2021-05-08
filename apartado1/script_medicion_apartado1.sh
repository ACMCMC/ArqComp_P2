#!/bin/bash
make -f Makefile_O0 clean # Compilamos
make -f Makefile_O0 # Compilamos
rm medidas_1_O0.csv # Limpiamos el fichero de resultados
rm medidas_1_O2.csv # Limpiamos el fichero de resultados
rm medidas_1.csv # Limpiamos el fichero de resultados
echo 'ID_PRUEBA,N,T_TOTAL' > medidas_1.csv # Cabecera del fichero
lista_n="250 500 750 1000 1500 2000 2550 3000" # Valores de C para probar
for id_prueba in {1..20}; # Repetimos las pruebas 50 veces
do
    echo "=========== ID DE PRUEBA: $id_prueba ==========="
    for n in $lista_n; # Para cada valor de C...
    do
        echo "Ejecutando prueba con N=$n" # Informacion por pantalla
        eval "./p2_apartado1.out $n $id_prueba" # Ejecutamos el primer experimento
    done
done

mv medidas_1.csv medidas_1_O0.csv

#!/bin/bash
make -f Makefile_O2 clean # Compilamos
make -f Makefile_O2 # Compilamos
rm medidas_1.csv # Limpiamos el fichero de resultados
echo 'ID_PRUEBA,N,T_TOTAL' > medidas_1.csv # Cabecera del fichero
lista_n="250 500 750 1000 1500 2000 2550 3000" # Valores de C para probar
for id_prueba in {1..20}; # Repetimos las pruebas 50 veces
do
    echo "=========== ID DE PRUEBA: $id_prueba ==========="
    for n in $lista_n; # Para cada valor de C...
    do
        echo "Ejecutando prueba con N=$n" # Informacion por pantalla
        eval "./p2_apartado1.out $n $id_prueba" # Ejecutamos el primer experimento
    done
done

mv medidas_1.csv medidas_1_O2.csv