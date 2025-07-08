#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simlib.h"

// --- Parámetros de la Simulación ---
#define TIEMPO_SIMULACION_HORAS 8.0
#define TIEMPO_SIMULACION_MINUTOS (TIEMPO_SIMULACION_HORAS * 60.0)

double MEDIA_LLEGADA_CLIENTES = 12.0;
double MEDIA_SERVICIO_CLIENTES = 6.0;
double MEDIA_LLEGADA_LLAMADAS = 10.0;
double MEDIA_SERVICIO_LLAMADAS = 5.0;

// Umbral para la prioridad dinámica de llamadas
// Aunque la lógica será de "prioridad de llamadas si hay disponibles",
// mantengo el #define si quieres experimentar con umbrales más tarde.
#define UMBRAL_COLA_LLAMADAS 0 // Un umbral de 0 o 1 significa "siempre que haya una llamada"

// --- Variables de Estado del Modelo ---
int num_empleados_ocupados;
#define MAX_EMPLEADOS 2

// --- Tipos de Eventos ---
#define EV_LLEGADA_CLIENTE 1
#define EV_LLEGADA_LLAMADA 2
#define EV_FIN_SERVICIO 3
#define EV_FIN_SIMULACION 4

// --- Atributos para 'transfer' ---
#define ATRIB_TIEMPO_LLEGADA 1
#define ATRIB_TIPO_ENTIDAD 2
#define ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO 3 // Para EV_FIN_SERVICIO: 1 = Cliente, 2 = Llamada
#define ATRIB_ID_EMPLEADO_SERVICIO 4             // Nuevo: Identifica qué empleado termina el servicio (1 o 2)

// --- Identificadores de Listas ---
#define LISTA_CLIENTES 1
#define LISTA_LLAMADAS 2

// --- Contadores y Acumuladores para Estadísticas ---
double total_tiempo_espera_clientes = 0.0;
long num_clientes_atendidos = 0;

double total_tiempo_espera_llamadas = 0.0;
long num_llamadas_atendidas = 0;

// Índices para las estadísticas de muestra (sampst)
#define INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES 1
#define INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS 2

// --- Prototipos de Funciones del Modelo ---
void llegada_cliente(void);
void llegada_llamada(void);
void fin_servicio(void);
void iniciar_proximo_servicio(void);
void model(void);

void model(void) {
    init_simlib();

    num_empleados_ocupados = 0;

    schedule(current_time + 2.0, EV_LLEGADA_CLIENTE);
    schedule(current_time + 3.0, EV_LLEGADA_LLAMADA);
    schedule(TIEMPO_SIMULACION_MINUTOS, EV_FIN_SIMULACION);

    do {
        timing();

        switch (next_event_type) {
            case EV_LLEGADA_CLIENTE:
                llegada_cliente();
                break;
            case EV_LLEGADA_LLAMADA:
                llegada_llamada();
                break;
            case EV_FIN_SERVICIO:
                fin_servicio();
                break;
            case EV_FIN_SIMULACION:
                break;
        }
    } while (next_event_type != EV_FIN_SIMULACION);
}

void llegada_cliente(void) {
    schedule(current_time + expon(MEDIA_LLEGADA_CLIENTES), EV_LLEGADA_CLIENTE);

    // Si hay algún empleado libre
    if (num_empleados_ocupados < MAX_EMPLEADOS) {
        num_empleados_ocupados++; // Ocupa a un empleado
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 1; // Servicio para un Cliente
        // No necesitamos ATRIB_ID_EMPLEADO_SERVICIO aquí, solo al finalizar.
        schedule(current_time + expon(MEDIA_SERVICIO_CLIENTES), EV_FIN_SERVICIO);
    } else {
        // Todos los empleados están ocupados, cliente va a la cola
        transfer[ATRIB_TIEMPO_LLEGADA] = current_time;
        transfer[ATRIB_TIPO_ENTIDAD] = 1; // Tipo Cliente
        list_file(LAST, LISTA_CLIENTES);
    }
}

void llegada_llamada(void) {
    schedule(current_time + expon(MEDIA_LLEGADA_LLAMADAS), EV_LLEGADA_LLAMADA);

    // Si hay algún empleado libre
    if (num_empleados_ocupados < MAX_EMPLEADOS) {
        num_empleados_ocupados++; // Ocupa a un empleado
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 2; // Servicio para una Llamada
        schedule(current_time + expon(MEDIA_SERVICIO_LLAMADAS), EV_FIN_SERVICIO);
    } else {
        // Todos los empleados están ocupados, llamada va a la cola
        transfer[ATRIB_TIEMPO_LLEGADA] = current_time;
        transfer[ATRIB_TIPO_ENTIDAD] = 2; // Tipo Llamada
        list_file(LAST, LISTA_LLAMADAS);
    }
}

void fin_servicio(void) {
    int tipo_entidad_terminado = (int)transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO];

    if (tipo_entidad_terminado == 1) {
        num_clientes_atendidos++;
    } else if (tipo_entidad_terminado == 2) {
        num_llamadas_atendidas++;
    } else {
        fprintf(stderr, "Error: Tipo de entidad desconocido en fin_servicio.\n");
    }

    num_empleados_ocupados--; // Un empleado se libera

    // Un empleado terminó un servicio, intenta asignar la próxima tarea
    iniciar_proximo_servicio();
    // Es importante llamar iniciar_proximo_servicio() cada vez que un empleado termina.
    // Si hay más de un empleado libre (raro, pero posible si el sistema se vacía rápido),
    // la próxima llamada a timing() y fin_servicio() liberará otro empleado y volverá a llamar.
    // Sin embargo, para asegurarnos de que se reasigne trabajo si hay múltiples empleados libres
    // y colas llenas, lo mejor es un bucle aquí.
    while (num_empleados_ocupados < MAX_EMPLEADOS && (list_size[LISTA_LLAMADAS] > 0 || list_size[LISTA_CLIENTES] > 0)) {
        iniciar_proximo_servicio();
    }
}

void iniciar_proximo_servicio(void) {
    // Solo intentar asignar si hay un empleado libre
    if (num_empleados_ocupados < MAX_EMPLEADOS) {
        // Prioridad: Llamadas primero, si hay
        if (list_size[LISTA_LLAMADAS] > 0) {
            list_remove(FIRST, LISTA_LLAMADAS);
            total_tiempo_espera_llamadas += (current_time - transfer[ATRIB_TIEMPO_LLEGADA]);
            sampst(current_time - transfer[ATRIB_TIEMPO_LLEGADA], INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS);

            num_empleados_ocupados++;
            transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 2; // Es una Llamada
            schedule(current_time + expon(MEDIA_SERVICIO_LLAMADAS), EV_FIN_SERVICIO);
        }
        // Si no hay llamadas, atender clientes si los hay
        else if (list_size[LISTA_CLIENTES] > 0) {
            list_remove(FIRST, LISTA_CLIENTES);
            total_tiempo_espera_clientes += (current_time - transfer[ATRIB_TIEMPO_LLEGADA]);
            sampst(current_time - transfer[ATRIB_TIEMPO_LLEGADA], INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES);

            num_empleados_ocupados++;
            transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 1; // Es un Cliente
            schedule(current_time + expon(MEDIA_SERVICIO_CLIENTES), EV_FIN_SERVICIO);
        }

    }
}

int main() {
    model();

    printf("REPORTE DE SIMULACION DE TEATRO (MEJORA: 2 EMPLEADOS)\n");
    printf("\nParametros de Entrada:\n");
    printf("  Tiempo de Simulacion: %.2f horas (%.2f minutos)\n", TIEMPO_SIMULACION_HORAS, TIEMPO_SIMULACION_MINUTOS);
    printf("  Numero de Empleados: %d\n", MAX_EMPLEADOS);
    printf("  Media Llegada Clientes: %.2f min\n", MEDIA_LLEGADA_CLIENTES);
    printf("  Media Servicio Clientes: %.2f min\n", MEDIA_SERVICIO_CLIENTES);
    printf("  Media Llegada Llamadas: %.2f min\n", MEDIA_LLEGADA_LLAMADAS);
    printf("  Media Servicio Llamadas: %.2f min\n", MEDIA_SERVICIO_LLAMADAS);
    printf("  Politica de Prioridad: Llamadas con prioridad alta (siempre que haya una en cola).\n");


    printf("\nMedidas de Desempeño:\n");
    if (num_clientes_atendidos > 0) {
        printf("  Tiempo Promedio Espera Clientes: %.2f min\n", total_tiempo_espera_clientes / num_clientes_atendidos);
    } else {
        printf("  Tiempo Promedio Espera Clientes: N/A (No clientes atendidos)\n");
    }

    if (num_llamadas_atendidas > 0) {
        printf("  Tiempo Promedio Espera Llamadas: %.2f min\n", total_tiempo_espera_llamadas / num_llamadas_atendidas);
    } else {
        printf("  Tiempo Promedio Espera Llamadas: N/A (No llamadas atendidas)\n");
    }

    printf("  Total Clientes Atendidos: %ld\n", num_clientes_atendidos);
    printf("  Total Llamadas Atendidas: %ld\n", num_llamadas_atendidas);
    printf("  Total Servicios Completados: %ld\n", num_clientes_atendidos + num_llamadas_atendidas);

    printf("\nEstadisticas Detalladas (sampst):\n");
    printf("  Tiempos de Espera Clientes:\n");
    out_sampst(stdout, INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES);
    printf("  Tiempos de Espera Llamadas:\n");
    out_sampst(stdout, INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS);

    return EXIT_SUCCESS;
}