#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "simlib.h"

#define MAX_EVENTS 1000

typedef struct {
    float time;
    int type;
} Event;

Event event_list[MAX_EVENTS];
int num_events = 0;
float sim_time = 0.0;
int event_type;

void init_simlib() {
    num_events = 0;
    sim_time = 0.0;
}

void event_schedule(float time, int type) {
    if (num_events >= MAX_EVENTS) {
        fprintf(stderr, "Too many events\n");
        exit(1);
    }
    event_list[num_events].time = time;
    event_list[num_events].type = type;
    num_events++;
}

void timing() {
    int min_index = -1;
    float min_time = FLT_MAX;

    for (int i = 0; i < num_events; i++) {
        if (event_list[i].time < min_time) {
            min_time = event_list[i].time;
            min_index = i;
        }
    }

    if (min_index == -1) {
        fprintf(stderr, "No events left\n");
        exit(1);
    }

    sim_time = event_list[min_index].time;
    event_type = event_list[min_index].type;

    // remove the event
    for (int i = min_index; i < num_events - 1; i++) {
        event_list[i] = event_list[i + 1];
    }
    num_events--;
}

float expon(float mean, int stream) {
    return -mean * log(1.0 - ((float)rand() / RAND_MAX));
}

float normal(float mean, float stddev) {
    static int haveSpare = 0;
    static double spare;

    if (haveSpare) {
        haveSpare = 0;
        return mean + stddev * spare;
    }

    haveSpare = 1;
    double u, v, s;
    do {
        u = ((double)rand() / RAND_MAX) * 2 - 1;
        v = ((double)rand() / RAND_MAX) * 2 - 1;
        s = u * u + v * v;
    } while (s >= 1 || s == 0);

    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    return mean + stddev * (u * s);
}
