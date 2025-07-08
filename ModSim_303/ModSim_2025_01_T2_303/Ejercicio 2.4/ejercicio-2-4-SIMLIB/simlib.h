// Definiciones de tipos y constantes
#define MAX_LIST_SIZE 25 // Tama�o m�ximo de las listas y del arreglo transfer
#define EVENT_LIST 0     // �ndice de la lista de eventos futura

// Opciones para list_file y list_remove
#define FIRST 0
#define LAST  1

// Variables globales de SIMLIB (declaradas como extern)
extern double current_time;       /* Hora actual de la simulaci�n */
extern int next_event_type;       /* Tipo del pr�ximo evento */
extern double transfer[MAX_LIST_SIZE + 1]; /* Arreglo para atributos de entidad */
extern int list_size[MAX_LIST_SIZE + 1]; /* Tama�o actual de cada lista */

// Prototipos de funciones de SIMLIB
void init_simlib(void);
void schedule(double time, int type);
void timing(void);
int  list_file(int option, int list);
int  list_remove(int option, int list);
double expon(double mean);
double uniform(double a, double b);
double normal(double mean, double stdev);

// Funciones para estad�sticas
void init_sampst(void);
void sampst(double value, int index);
void out_sampst(FILE *f, int index);
void init_timest(void);
void timest(double value, int index);
void out_timest(FILE *f, int index);
void pr_report(FILE *f); // Para un reporte general (puede que no est� en todas las versiones)

// Funciones de utilidad (pueden variar entre implementaciones)
double ranf(void); // Generador de n�meros aleatorios b�sico
void   set_random_seed(long seed);
void   init_random_number_generators(void);
