#ifndef INC_SIMPLE_TIMER_H
#define INC_SIMPLE_TIMER_H

#include "common-defines.h"

typedef struct simple_timer_t {
    uint64_t WaitTime;
    uint64_t TargetTime;
    bool AutoReset;
    bool HasElapsed;
} simple_timer_t;

void SimpleTimerSetup(simple_timer_t *timer, uint64_t WaitTime, bool AutoReset);
bool SimpleTimerHasElapsed(simple_timer_t *timer);
void SimpleTimerReset(simple_timer_t *timer);

#endif