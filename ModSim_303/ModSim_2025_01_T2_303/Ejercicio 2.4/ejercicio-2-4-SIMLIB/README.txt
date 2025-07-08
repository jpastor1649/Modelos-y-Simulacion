Instrucciones para compilar y ejecutar la simulación:

Archivos incluidos:
- ejercicio-2-4.c: Código principal de la simulación.
- simlib.c: Implementación de la librería SIMLIB/C.
- simlib.h: Archivo de cabecera de la librería SIMLIB/C.
- cambios-2-4.c: Código con cambios para mejorar la simulación.

Requisitos:
- Compilador de C compatible con ANSI C.

Pasos para compilar usando GCC (en terminal/línea de comandos):

1.  Navega hasta el directorio donde se encuentran estos archivos.
2.  Compila los archivos fuente y enlaza la librería matemática:
    gcc -o ejercicio-2-4 ejercicio-2-4.c simlib.c -lm

Pasos para ejecutar:

1.  Una vez compilado, ejecuta el archivo ejecutable:
    ./ejercicio-2-4 (en Linux/macOS)
    ejercicio-2-4.exe (en Windows)

Para utilizer con Code::Blocks:
1. Crear un proyecto de "Console application" en C.
2. Añadir simulacion_teatro.c, simlib.c y simlib.h al proyecto.
3. En Project -> Build options -> Linker settings -> Link libraries, añadir 'm'.
4. Asegurarse de que el compilador del proyecto sea 'gcc.exe'.

En el caso de que se quiera compilar y ejecutar la simulación con cambios se debe reemplazar el archivo ejercicio-2-4.c por cambios-2-4.c en las instrucciones anteriores.