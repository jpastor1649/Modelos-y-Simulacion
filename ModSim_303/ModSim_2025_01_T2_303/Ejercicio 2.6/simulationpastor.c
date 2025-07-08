#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "simlib.h"

#define SIM_TIME (60 * 24)
#define MEAN_ARRIVAL_AIRPORT 2.5
#define MEAN_ARRIVAL_HOTEL 5
#define MEAN_BUS_TIME 20
#define STD_BUS_TIME 2
#define BUS_START_DELAYS_1 0
#define BUS_START_DELAYS_2 30
#define MAX_QUEUE 10000

#define EVENT_ARRIVAL_AIRPORT 1
#define EVENT_ARRIVAL_HOTEL1 2
#define EVENT_ARRIVAL_HOTEL2 3
#define EVENT_BUS1 4
#define EVENT_BUS2 5
#define EVENT_END 6

void advance_time(int id);

int airport_to_hotel1 = 0, airport_to_hotel2 = 0;
int hotel1_to_airport = 0, hotel2_to_airport = 0;
int failures = 0;
int capacity;

void arrival_airport() {
    if (airport_to_hotel1 <= airport_to_hotel2)
        airport_to_hotel1++;
    else
        airport_to_hotel2++;

    event_schedule(sim_time + expon(MEAN_ARRIVAL_AIRPORT, 1), EVENT_ARRIVAL_AIRPORT);
}

void arrival_hotel1() {
    hotel1_to_airport++;
    event_schedule(sim_time + expon(MEAN_ARRIVAL_HOTEL, 2), EVENT_ARRIVAL_HOTEL1);
}

void arrival_hotel2() {
    hotel2_to_airport++;
    event_schedule(sim_time + expon(MEAN_ARRIVAL_HOTEL, 3), EVENT_ARRIVAL_HOTEL2);
}

void bus_trip(int bus_id) {
    int onboard_H1 = 0, onboard_H2 = 0, onboard_A = 0, occupancy = 0;
    int space, boarded;

    space = capacity;

    // Airport pickup
    if ((airport_to_hotel1 + airport_to_hotel2) > space)
        failures++;

    boarded = (space / 2 < airport_to_hotel1) ? space / 2 : airport_to_hotel1;
    airport_to_hotel1 -= boarded;
    onboard_H1 += boarded;
    occupancy += boarded;
    space -= boarded;

    boarded = (space < airport_to_hotel2) ? space : airport_to_hotel2;
    airport_to_hotel2 -= boarded;
    onboard_H2 += boarded;
    occupancy += boarded;
    space -= boarded;

    // Airport to Hotel 1
    advance_time(bus_id);
    occupancy -= onboard_H1;
    onboard_H1 = 0;
    space = capacity - occupancy;

    if (hotel1_to_airport > space)
        failures++;

    boarded = (space < hotel1_to_airport) ? space : hotel1_to_airport;
    hotel1_to_airport -= boarded;
    onboard_A += boarded;
    occupancy += boarded;
    space -= boarded;

    // Hotel 1 to Hotel 2
    advance_time(bus_id);
    occupancy -= onboard_H2;
    onboard_H2 = 0;
    space = capacity - occupancy;

    if (hotel2_to_airport > space)
        failures++;

    boarded = (space < hotel2_to_airport) ? space : hotel2_to_airport;
    hotel2_to_airport -= boarded;
    onboard_A += boarded;
    occupancy += boarded;
    space -= boarded;

    // Hotel 2 to Airport
    advance_time(bus_id);
    occupancy -= onboard_A;
    onboard_A = 0;

    // Reprogramar el mismo bus para el siguiente viaje
    event_schedule(sim_time, (bus_id == 1) ? EVENT_BUS1 : EVENT_BUS2);
}

void advance_time(int id) {
    float t = normal(MEAN_BUS_TIME, STD_BUS_TIME);
    if (t < 0.1) t = 0.1;
    sim_time += t;
}

int simulation(int cap) {
    srand(time(NULL) + rand());  // Semilla distinta para cada simulación
    init_simlib();
    sim_time = 0.0;
    capacity = cap;
    airport_to_hotel1 = airport_to_hotel2 = 0;
    hotel1_to_airport = hotel2_to_airport = 0;
    failures = 0;

    event_schedule(expon(MEAN_ARRIVAL_AIRPORT, 1), EVENT_ARRIVAL_AIRPORT);
    event_schedule(expon(MEAN_ARRIVAL_HOTEL, 2), EVENT_ARRIVAL_HOTEL1);
    event_schedule(expon(MEAN_ARRIVAL_HOTEL, 3), EVENT_ARRIVAL_HOTEL2);
    event_schedule(BUS_START_DELAYS_1, EVENT_BUS1);
    event_schedule(BUS_START_DELAYS_2, EVENT_BUS2);

    while (sim_time < SIM_TIME) {
        timing();
        switch (event_type) {
            case EVENT_ARRIVAL_AIRPORT: arrival_airport(); break;
            case EVENT_ARRIVAL_HOTEL1: arrival_hotel1(); break;
            case EVENT_ARRIVAL_HOTEL2: arrival_hotel2(); break;
            case EVENT_BUS1: bus_trip(1); break;
            case EVENT_BUS2: bus_trip(2); break;
        }
    }

    return failures;
}

int main() {
    int rep = 5000;
    int max_cap = 100;
    int cap;

    for (cap = 10; cap <= max_cap; cap++) {
        int success = 0;
        for (int i = 0; i < rep; i++) {
            if (simulation(cap) == 0)
                success++;
        }
        printf("Capacity %d: %.2f%% success rate\n", cap, (100.0 * success) / rep);
        if (success == rep) {
            printf("Minimum capacity per bus: %d\n", cap);
            break;
        }
    }

    return 0;
}
