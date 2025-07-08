#ifndef SIMLIB_H
#define SIMLIB_H

extern float sim_time;
extern int event_type;

void init_simlib();
void event_schedule(float time, int event_type);
void timing();
float expon(float mean, int stream);
float normal(float mean, float stddev);

#endif
