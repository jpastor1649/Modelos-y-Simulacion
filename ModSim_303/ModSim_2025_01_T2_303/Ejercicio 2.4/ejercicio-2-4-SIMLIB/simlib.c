#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simlib.h"

// Definiciones de variables globales
double current_time;
int next_event_type;
double transfer[MAX_LIST_SIZE + 1];
int list_size[MAX_LIST_SIZE + 1];

// Estructura de un nodo de la lista
struct list_node {
    double attributes[MAX_LIST_SIZE + 1];
    struct list_node *next;
};

// Arreglo de punteros a la cabeza de cada lista
static struct list_node *list_heads[MAX_LIST_SIZE + 1];

// Variables para el generador de números aleatorios (RANF)
static long seed = 44444;
static double a = 16807.0;
static double m = 2147483647.0;
static double q = 127773.0;
static double r = 2836.0;

// Variables para estadísticas de sampst (muestras)
static double sampst_sum[MAX_LIST_SIZE + 1];
static double sampst_sq_sum[MAX_LIST_SIZE + 1];
static long sampst_num_observations[MAX_LIST_SIZE + 1];

// Variables para estadísticas de timest (tiempo ponderado)
static double timest_last_update_time[MAX_LIST_SIZE + 1];
static double timest_sum_area[MAX_LIST_SIZE + 1];
static double timest_value[MAX_LIST_SIZE + 1];
static long timest_num_updates[MAX_LIST_SIZE + 1];


/* init_simlib: Inicializa SIMLIB */
void init_simlib(void) {
    current_time = 0.0;
    next_event_type = 0;
    for (int i = 0; i <= MAX_LIST_SIZE; ++i) {
        list_heads[i] = NULL;
        list_size[i] = 0;
        sampst_sum[i] = 0.0;
        sampst_sq_sum[i] = 0.0;
        sampst_num_observations[i] = 0;
        timest_last_update_time[i] = 0.0;
        timest_sum_area[i] = 0.0;
        timest_value[i] = 0.0;
        timest_num_updates[i] = 0;
    }
    init_random_number_generators(); // Inicializa el generador de números aleatorios
}

/* schedule: Programa un evento */
void schedule(double time, int type) {
    struct list_node *ptr, *prev_ptr;
    struct list_node *new_node = (struct list_node *) malloc(sizeof(struct list_node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nuevo evento.\n");
        exit(1);
    }

    new_node->attributes[1] = time; // El tiempo del evento siempre es el primer atributo
    new_node->attributes[2] = type; // El tipo de evento siempre es el segundo atributo

    // Copia los atributos adicionales de 'transfer' al nuevo nodo
    for (int i = 3; i <= MAX_LIST_SIZE; ++i) {
        new_node->attributes[i] = transfer[i];
    }

    // Inserta el evento en la lista de eventos ordenada por tiempo
    if (list_heads[EVENT_LIST] == NULL || time < list_heads[EVENT_LIST]->attributes[1]) {
        new_node->next = list_heads[EVENT_LIST];
        list_heads[EVENT_LIST] = new_node;
    } else {
        prev_ptr = list_heads[EVENT_LIST];
        while (prev_ptr->next != NULL && time >= prev_ptr->next->attributes[1]) {
            prev_ptr = prev_ptr->next;
        }
        new_node->next = prev_ptr->next;
        prev_ptr->next = new_node;
    }
    list_size[EVENT_LIST]++;
}

/* timing: Determina el próximo evento */
void timing(void) {
    struct list_node *event_node;

    if (list_heads[EVENT_LIST] == NULL) {
        fprintf(stderr, "Error: La lista de eventos está vacía. Fin de la simulación inesperado.\n");
        exit(1);
    }

    event_node = list_heads[EVENT_LIST];
    current_time = event_node->attributes[1];
    next_event_type = (int)event_node->attributes[2];

    // Copia los atributos del evento a 'transfer'
    for (int i = 1; i <= MAX_LIST_SIZE; ++i) {
        transfer[i] = event_node->attributes[i];
    }

    list_heads[EVENT_LIST] = event_node->next;
    free(event_node);
    list_size[EVENT_LIST]--;
}

/* list_file: Añade una entidad a una lista */
int list_file(int option, int list) {
    struct list_node *ptr, *prev_ptr;
    struct list_node *new_node = (struct list_node *) malloc(sizeof(struct list_node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nuevo nodo de lista.\n");
        exit(1);
    }

    // Copia los atributos de 'transfer' al nuevo nodo
    for (int i = 1; i <= MAX_LIST_SIZE; ++i) {
        new_node->attributes[i] = transfer[i];
    }
    new_node->next = NULL;

    if (option == FIRST) { // Insertar al principio
        new_node->next = list_heads[list];
        list_heads[list] = new_node;
    } else if (option == LAST) { // Insertar al final
        if (list_heads[list] == NULL) {
            list_heads[list] = new_node;
        } else {
            ptr = list_heads[list];
            while (ptr->next != NULL) {
                ptr = ptr->next;
            }
            ptr->next = new_node;
        }
    } else {
        fprintf(stderr, "Error: Opcion invalida para list_file.\n");
        return 0; // Fallo
    }
    list_size[list]++;
    return 1; // Éxito
}

/* list_remove: Quita una entidad de una lista */
int list_remove(int option, int list) {
    struct list_node *node_to_remove;

    if (list_heads[list] == NULL) {
        // La lista está vacía, no hay nada que remover
        return 0; // Fallo
    }

    if (option == FIRST) {
        node_to_remove = list_heads[list];
        list_heads[list] = node_to_remove->next;
    } else {
        // En esta implementación simple, solo FIRST es compatible
        fprintf(stderr, "Error: Opcion invalida para list_remove. Solo FIRST es soportado en esta version.\n");
        return 0; // Fallo
    }

    // Copia los atributos del nodo removido a 'transfer'
    for (int i = 1; i <= MAX_LIST_SIZE; ++i) {
        transfer[i] = node_to_remove->attributes[i];
    }

    free(node_to_remove);
    list_size[list]--;
    return 1; // Éxito
}

/* ranf: Generador de números aleatorios de 0 a 1 */
double ranf(void) {
    long hi = seed / q;
    long lo = seed - q * hi;
    seed = a * lo - r * hi;
    if (seed < 0) {
        seed = seed + m;
    }
    return (double) seed / m;
}

/* set_random_seed: Establece la semilla del generador */
void set_random_seed(long s) {
    seed = s;
}

/* init_random_number_generators: Inicializa el generador, opcionalmente con una semilla */
void init_random_number_generators(void) {
    // Puedes llamar a set_random_seed aquí si quieres una semilla fija
    // set_random_seed(12345);
    // O puedes leer una semilla de la entrada, etc.
}

/* expon: Genera un número de una distribución exponencial */
double expon(double mean) {
    return -mean * log(ranf());
}

/* uniform: Genera un número de una distribución uniforme */
double uniform(double a_val, double b_val) {
    return a_val + (b_val - a_val) * ranf();
}

/* normal: Genera un número de una distribución normal (Box-Muller) */
double normal(double mean, double stdev) {
    static int have_spare = 0;
    static double spare;
    double u1, u2, s, val;

    if (have_spare) {
        have_spare = 0;
        return mean + stdev * spare;
    }

    do {
        u1 = ranf();
        u2 = ranf();
        s = u1 * u1 + u2 * u2;
    } while (s >= 1.0 || s == 0.0);

    val = sqrt(-2.0 * log(s) / s);
    spare = u2 * val;
    have_spare = 1;
    return mean + stdev * u1 * val;
}


/* init_sampst: Inicializa un registro de estadísticas de muestra */
void init_sampst(void) {
    // Ya inicializado en init_simlib
}

/* sampst: Registra una observación en una serie de estadísticas de muestra */
void sampst(double value, int index) {
    if (index < 1 || index > MAX_LIST_SIZE) {
        fprintf(stderr, "Error: Indice de sampst fuera de rango.\n");
        return;
    }
    sampst_sum[index] += value;
    sampst_sq_sum[index] += value * value;
    sampst_num_observations[index]++;
}

/* out_sampst: Imprime estadísticas de muestra */
void out_sampst(FILE *f, int index) {
    if (index < 1 || index > MAX_LIST_SIZE) {
        fprintf(stderr, "Error: Indice de sampst fuera de rango.\n");
        return;
    }
    fprintf(f, "Estadisticas de la muestra %d:\n", index);
    if (sampst_num_observations[index] > 0) {
        double avg = sampst_sum[index] / sampst_num_observations[index];
        double var = (sampst_sq_sum[index] - sampst_sum[index] * avg) / (sampst_num_observations[index] > 1 ? (sampst_num_observations[index] - 1) : 1);
        double std_dev = sqrt(var > 0 ? var : 0); // Evitar sqrt de negativo
        fprintf(f, "  Observaciones: %ld\n", sampst_num_observations[index]);
        fprintf(f, "  Promedio:      %.4f\n", avg);
        fprintf(f, "  Desv. Est.:    %.4f\n", std_dev);
    } else {
        fprintf(f, "  No hay observaciones.\n");
    }
}

/* init_timest: Inicializa un registro de estadísticas de tiempo ponderado */
void init_timest(void) {
    // Ya inicializado en init_simlib
}

/* timest: Actualiza una variable de estado para estadísticas de tiempo ponderado */
void timest(double value, int index) {
    if (index < 1 || index > MAX_LIST_SIZE) {
        fprintf(stderr, "Error: Indice de timest fuera de rango.\n");
        return;
    }
    // Suma el área del rectángulo anterior
    timest_sum_area[index] += timest_value[index] * (current_time - timest_last_update_time[index]);
    timest_value[index] = value; // Nuevo valor
    timest_last_update_time[index] = current_time; // Actualiza el tiempo de la última actualización
    timest_num_updates[index]++;
}

/* out_timest: Imprime estadísticas de tiempo ponderado */
void out_timest(FILE *f, int index) {
    if (index < 1 || index > MAX_LIST_SIZE) {
        fprintf(stderr, "Error: Indice de timest fuera de rango.\n");
        return;
    }
    fprintf(f, "Estadisticas de tiempo ponderado %d:\n", index);
    // Asegurarse de agregar el área del último período
    double final_sum_area = timest_sum_area[index] + timest_value[index] * (current_time - timest_last_update_time[index]);

    if (current_time > 0) {
        fprintf(f, "  Promedio: %.4f\n", final_sum_area / current_time);
    } else {
        fprintf(f, "  No hay tiempo registrado.\n");
    }
}
