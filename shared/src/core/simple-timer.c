#include "core/system.h"
#include "core/simple-timer.h"

void SimpleTimerSetup(simple_timer_t *timer, uint64_t WaitTime, bool AutoReset) {
    timer->WaitTime = WaitTime;
    timer->AutoReset = AutoReset;
    timer->TargetTime = system_get_ticks() + WaitTime;
    timer->HasElapsed = false;
}

bool SimpleTimerHasElapsed(simple_timer_t *timer) {
    uint64_t now = system_get_ticks();
    bool HasElapsed = now >= timer->TargetTime;

    if(timer->HasElapsed) { return false; }

    if(HasElapsed) {
        if(timer->AutoReset) {
            uint64_t delta = now - timer->TargetTime;
            timer->TargetTime = (now + timer->WaitTime) - delta;    
        }
        else {
            timer->HasElapsed = true;
        }
    }

    return HasElapsed;
}

void SimpleTimerReset(simple_timer_t *timer) {
    SimpleTimerSetup(timer, timer->WaitTime, timer->AutoReset);
}