#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simlib.h"

// -------------------------------------------------------------------------
// Parámetros de entrada (Variables globales)
// -------------------------------------------------------------------------
#define TIEMPO_SIMULACION_HORAS 8.0
#define TIEMPO_SIMULACION_MINUTOS (TIEMPO_SIMULACION_HORAS * 60.0)

// Medias de las distribuciones exponenciales
double MEDIA_LLEGADA_CLIENTES = 12.0;
double MEDIA_SERVICIO_CLIENTES = 6.0;
double MEDIA_LLEGADA_LLAMADAS = 10.0;
double MEDIA_SERVICIO_LLAMADAS = 5.0;

// -------------------------------------------------------------------------
// Variables globales
// -------------------------------------------------------------------------
int empleado_ocupado; // 0 = libre, 1 = ocupado

// -------------------------------------------------------------------------
// Descripción del evento y tipo de evento
// -------------------------------------------------------------------------
// Eventos (tipos de eventos para la lista de eventos futura)
#define EV_LLEGADA_CLIENTE 1      // Llegada de un cliente en persona
#define EV_LLEGADA_LLAMADA 2      // Llegada de una llamada telefónica
#define EV_FIN_SERVICIO 3         // Un evento genérico para fin de servicio
#define EV_FIN_SIMULACION 4       // Fin de la simulación

// Atributos usados en el arreglo global 'transfer'
#define ATRIB_TIEMPO_LLEGADA 1  // Tiempo en que la entidad llegó a la fila (o al sistema)
#define ATRIB_TIPO_ENTIDAD 2    // Para entidades en cola: 1 = Cliente, 2 = Llamada
#define ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO 3 // Atributo para EV_FIN_SERVICIO: 1 = Cliente, 2 = Llamada

// -------------------------------------------------------------------------
// Listas y sus atributos
// -------------------------------------------------------------------------
// Listas de espera para clientes y llamadas
#define LISTA_CLIENTES 1 // Identificador para la fila de clientes en persona
#define LISTA_LLAMADAS 2 // Identificador para la fila de llamadas telefónicas

// -------------------------------------------------------------------------
// Contadores y/o acumuladores
// -------------------------------------------------------------------------
// Para estadísticas de tiempo de espera
double total_tiempo_espera_clientes = 0.0;
long num_clientes_atendidos = 0;

double total_tiempo_espera_llamadas = 0.0;
long num_llamadas_atendidas = 0;

// Índices para sampst (estadísticas de muestra)
#define INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES 1
#define INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS 2

// -------------------------------------------------------------------------
// Subprogramas y propósito (Funciones para manejar los eventos)
// -------------------------------------------------------------------------

// Prototipos de funciones para el manejo de eventos
void llegada_cliente(void);
void llegada_llamada(void);
void fin_servicio(void);
void fin_simulacion(void);
void iniciar_proximo_servicio(void);

// Función principal del modelo
void model(void) {
    // Inicializar SIMLIB
    init_simlib();

    empleado_ocupado = 0; // El empleado comienza libre al inicio de la simulación

    // Programar la primera llegada de clientes y llamadas
    schedule(current_time + 2.0, EV_LLEGADA_CLIENTE); // Primera persona llega a los 2 minutos
    schedule(current_time + 3.0, EV_LLEGADA_LLAMADA);     // Primera llamada a los 3 minutos

    // Programar el evento de fin de simulación
    schedule(TIEMPO_SIMULACION_MINUTOS, EV_FIN_SIMULACION);

    // Bucle principal de la simulación
    do {
        // Obtenemos el próximo evento de la lista de eventos futura (simlib.c)
        // La información del evento se carga en el arreglo global 'transfer'.
        timing(); // Esta función actualiza 'current_time' y carga 'transfer'

        // Despachar el evento actual
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

// Función para manejar la llegada de un cliente en persona
void llegada_cliente(void) {
    // Programar la próxima llegada de un cliente
    schedule(current_time + expon(MEDIA_LLEGADA_CLIENTES), EV_LLEGADA_CLIENTE);

    // Si el empleado está libre y no hay llamadas esperando (prioridad clientes)
    if (empleado_ocupado == 0 && list_size[LISTA_LLAMADAS] == 0) {
        empleado_ocupado = 1; // Ocupar empleado
        // Programar fin de servicio para este cliente
        // ¡Importante!: Cargar el atributo para el evento FIN_SERVICIO en transfer[3] (o mayor)
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 1; // 1 = Cliente
        schedule(current_time + expon(MEDIA_SERVICIO_CLIENTES), EV_FIN_SERVICIO);
    } else {
        // El empleado está ocupado o hay llamadas esperando, el cliente entra en la fila
        transfer[ATRIB_TIEMPO_LLEGADA] = current_time; // Registrar tiempo de llegada
        transfer[ATRIB_TIPO_ENTIDAD] = 1;             // Tipo: Cliente (para la entidad en cola)
        list_file(LAST, LISTA_CLIENTES);              // Añadir a la fila de clientes (FIFO)
    }
}

// Función para manejar la llegada de una llamada telefónica
void llegada_llamada(void) {
    // Programar la próxima llegada de una llamada
    schedule(current_time + expon(MEDIA_LLEGADA_LLAMADAS), EV_LLEGADA_LLAMADA);

    // Si el empleado está libre Y no hay clientes en la fila (prioridad clientes)
    if (empleado_ocupado == 0 && list_size[LISTA_CLIENTES] == 0) {
        empleado_ocupado = 1; // Ocupar empleado
        // Programar fin de servicio para esta llamada
        // ¡Importante!: Cargar el atributo para el evento FIN_SERVICIO en transfer[3] (o mayor)
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 2; // 2 = Llamada
        schedule(current_time + expon(MEDIA_SERVICIO_LLAMADAS), EV_FIN_SERVICIO);
    } else {
        // El empleado está ocupado o hay clientes esperando, la llamada entra en la fila
        transfer[ATRIB_TIEMPO_LLEGADA] = current_time; // Registrar tiempo de llegada
        transfer[ATRIB_TIPO_ENTIDAD] = 2;             // Tipo: Llamada (para la entidad en cola)
        list_file(LAST, LISTA_LLAMADAS);              // Añadir a la fila de llamadas (FIFO)
    }
}

// Función para manejar el fin de servicio para cualquier tipo de entidad
void fin_servicio(void) {
    // Recuperar el tipo de entidad que terminó el servicio
    // ¡Importante!: Ahora se lee desde el slot correcto (transfer[3]) que se guardó con el evento
    int tipo_entidad_terminado = (int)transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO];

    // Registrar que el servicio para la entidad actual ha terminado
    if (tipo_entidad_terminado == 1) { // Cliente
        num_clientes_atendidos++;
    } else if (tipo_entidad_terminado == 2) { // Llamada
        num_llamadas_atendidas++;
    } else {
        // Esto no debería pasar si la lógica es correcta
        fprintf(stderr, "Error: Tipo de entidad desconocido en fin_servicio.\n");
    }

    iniciar_proximo_servicio(); // Intentar iniciar el próximo servicio
}

// Función auxiliar para decidir qué atender a continuación
void iniciar_proximo_servicio(void) {
    // 1. Dar prioridad a los clientes en la fila
    if (list_size[LISTA_CLIENTES] > 0) {
        list_remove(FIRST, LISTA_CLIENTES); // Sacar al primer cliente de la fila
        // Calcular y registrar tiempo de espera del cliente
        total_tiempo_espera_clientes += (current_time - transfer[ATRIB_TIEMPO_LLEGADA]);
        sampst(current_time - transfer[ATRIB_TIEMPO_LLEGADA], INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES);

        empleado_ocupado = 1; // Ocupar empleado
        // Programar el fin de servicio para este cliente
        // ¡Importante!: Cargar el atributo para el evento FIN_SERVICIO en transfer[3]
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 1; // Siguiente es un Cliente
        schedule(current_time + expon(MEDIA_SERVICIO_CLIENTES), EV_FIN_SERVICIO);
    }
    // 2. Si no hay clientes, atender llamadas
    else if (list_size[LISTA_LLAMADAS] > 0) {
        list_remove(FIRST, LISTA_LLAMADAS); // Sacar la primera llamada de la fila
        // Calcular y registrar tiempo de espera de la llamada
        total_tiempo_espera_llamadas += (current_time - transfer[ATRIB_TIEMPO_LLEGADA]);
        sampst(current_time - transfer[ATRIB_TIEMPO_LLEGADA], INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS);

        empleado_ocupado = 1; // Ocupar empleado
        // Programar el fin de servicio para esta llamada
        // ¡Importante!: Cargar el atributo para el evento FIN_SERVICIO en transfer[3]
        transfer[ATRIB_TIPO_ENTIDAD_EVENTO_FIN_SERVICIO] = 2; // Siguiente es una Llamada
        schedule(current_time + expon(MEDIA_SERVICIO_LLAMADAS), EV_FIN_SERVICIO);
    } else {
        // Si no hay nadie en ninguna fila, el empleado queda libre
        empleado_ocupado = 0;
    }
}


int main() {
    model();

    printf("RESULTADOS SIMULACION TEATRO:\n");
    printf("\nTiempo de simulacion: %.2f horas (%.2f minutos)\n\n", TIEMPO_SIMULACION_HORAS, TIEMPO_SIMULACION_MINUTOS);
    printf("Media de tiempo entre llegadas de clientes (min): %.2f\n", MEDIA_LLEGADA_CLIENTES);
    printf("Media de tiempo de servicio de clientes (min):   %.2f\n", MEDIA_SERVICIO_CLIENTES);
    printf("Media de tiempo entre llegadas de llamadas (min): %.2f\n", MEDIA_LLEGADA_LLAMADAS);
    printf("Media de tiempo de servicio de llamadas (min):   %.2f\n", MEDIA_SERVICIO_LLAMADAS);

    printf("\nMedidas de Desempeno\n");
    // Calcular y mostrar tiempos de espera promedio
    printf("Tiempo de espera promedio de clientes en persona (min): %.2f\n", total_tiempo_espera_clientes / num_clientes_atendidos);
    printf("Tiempo de espera promedio de llamadas (min):           %.2f\n", total_tiempo_espera_llamadas / num_llamadas_atendidas);

    printf("\nNumero total de clientes atendidos: %ld\n", num_clientes_atendidos);
    printf("Numero total de llamadas atendidas: %ld\n", num_llamadas_atendidas);
    printf("Total de servicios completados: %ld\n", num_clientes_atendidos+num_llamadas_atendidas);

    printf("\nEstadisticas de Tiempos de Espera (sampst):\n");
    printf("Clientes en persona:\n");
    out_sampst(stdout, INDICE_SAMPST_TIEMPO_ESPERA_CLIENTES);
    printf("\nLlamadas telefónicas:\n");
    out_sampst(stdout, INDICE_SAMPST_TIEMPO_ESPERA_LLAMADAS);

    return EXIT_SUCCESS;
}
