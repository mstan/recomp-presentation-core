/* Extracted Genesis host pacing; PolyForm Noncommercial 1.0.0. See LICENSES/Genesis.txt. */
#ifndef RETRO_PRESENTATION_PACER_H
#define RETRO_PRESENTATION_PACER_H

#include <stdint.h>
#include <float.h>

/* Host presentation deadlines, independent of guest cycles. Keep the fractional
 * counter ticks so NTSC pacing does not accumulate truncation error. */
typedef struct RetroFramePacer {
    uint64_t next;
    uint64_t period;
    double fraction;
    double remainder;
    uint64_t phase;
    uint32_t phase_step, phase_den;
    int active;
} RetroFramePacer;

static inline void retro_frame_pacer_init(RetroFramePacer *p, uint64_t frequency, double fps)
{
    p->active=0; p->period=0; p->phase=p->phase_step=p->phase_den=0; p->next=0; p->remainder=p->fraction=0;
    if(!frequency || !(fps>0) || fps>(double)frequency)return;
    double ticks = (double)frequency / fps;
    if(!(ticks<18446744073709551616.0))return;
    p->period = (uint64_t)ticks;
    p->fraction = ticks - (double)p->period;
    p->next = 0;
    p->remainder = 0.0;
    p->active = 0;
    p->phase = p->phase_step = p->phase_den = 0;
}

/* Exact counter ticks per rational guest frame. The two 32-bit factors in
 * the remainder product cannot overflow; reject an unrepresentable period. */
static inline int retro_frame_pacer_init_ratio(RetroFramePacer *p, uint64_t frequency,
                                         uint32_t ticks, uint32_t clock)
{
    p->active = 0;
    if (!frequency || !ticks || !clock) return 0;
    uint64_t q = frequency / clock, part = (frequency % clock) * ticks;
    if (q > UINT64_MAX / ticks) return 0;
    uint64_t whole = q * ticks, tail = part / clock;
    if (whole > UINT64_MAX - tail) return 0;
    p->period = whole + tail;
    p->phase_step = (uint32_t)(part % clock);
    p->phase_den = clock;
    p->phase = 0;
    p->fraction = p->remainder = 0.0;
    p->next = 0;
    return p->period != 0;
}

static inline void retro_frame_pacer_reset(RetroFramePacer *p)
{
    p->active = 0;
}

/* Called when a frame is ready to present. A small scheduling delay retains
 * the cadence; a full frame overrun (including a pause) starts a fresh cadence
 * instead of issuing a burst of catch-up frames. Turbo resets this schedule. */
static inline uint64_t retro_frame_pacer_deadline(RetroFramePacer *p, uint64_t now)
{
    uint64_t deadline;
    if(!p->period){p->active=0;return now;}
    if (!p->active || (now > p->next && now - p->next > p->period)) {
        p->next = now;
        p->remainder = 0.0;
        p->phase = 0;
        p->active = 1;
    }
    deadline = p->next;
    p->next += p->period;
    if (p->phase_den) {
        p->phase += p->phase_step;
        if (p->phase >= p->phase_den) {
            ++p->next;
            p->phase -= p->phase_den;
        }
    } else {
        p->remainder += p->fraction;
        if (p->remainder >= 1.0) {
            ++p->next;
            p->remainder -= 1.0;
        }
    }
    return deadline;
}

/* A second clock enumerates display deadlines inside each guest interval.
 * Never round display/guest to an integer: 144 Hz needs alternating two/three
 * presentations per NTSC tick. This clock cannot advance the simulation. */
typedef struct RetroPresentationClock {
    RetroFramePacer display;
    uint64_t frequency;
    double rate;
    int active;
} RetroPresentationClock;
static inline void retro_presentation_clock_reset(RetroPresentationClock *p)
{ p->active = 0; }
static inline void retro_presentation_clock_begin(RetroPresentationClock *p, uint64_t frequency,
                                            double rate, uint64_t start)
{
    if(!frequency || !(rate>0) || rate>(double)frequency){p->active=0;p->display.period=0;return;}
    if (!p->active || p->rate != rate || p->frequency != frequency ||
        (start > p->display.next && start - p->display.next > frequency / 4)) {
        retro_frame_pacer_init(&p->display, frequency, rate);
        if(!p->display.period){p->active=0;return;}
        p->display.next = start; p->display.active = 1;
        p->frequency = frequency; p->rate = rate; p->active = 1;
    }
    /* Drop overdue output, never emit a burst or speed the guest up. */
    while (p->display.next < start)
        (void)retro_frame_pacer_deadline(&p->display, p->display.next);
}
static inline int retro_presentation_clock_next(RetroPresentationClock *p, uint64_t start,
                                          double guest_period, uint64_t *deadline,
                                          double *phase)
{
    if(!p->active || !p->display.period || !(guest_period>0) || !(guest_period<18446744073709551616.0) || !deadline || !phase)return 0;
    if(!p->active||!p->display.period||!deadline||!phase||!(guest_period>0)||guest_period>DBL_MAX)return 0;
    uint64_t at = p->display.next;
    if ((double)(at - start) >= guest_period) return 0;
    *deadline = retro_frame_pacer_deadline(&p->display, at);
    *phase = (double)(at - start) / guest_period;
    return 1;
}


/* PSX host policy: retain finite transition debt, with a caller-selected
 * maximum period count. Returns a deadline to wait for, or zero for an
 * immediate frame. This is independent of guest time and OS waiting. */
typedef struct RetroCatchupPacer { uint64_t next_deadline; } RetroCatchupPacer;
static inline uint64_t retro_catchup_frame_deadline(RetroCatchupPacer *p,
    uint64_t now,uint64_t period,unsigned max_periods) {
    if (p->next_deadline == 0 || now >= p->next_deadline + period * max_periods) {
        p->next_deadline = now + period;
        return 0;
    }
    const uint64_t deadline = p->next_deadline;
    p->next_deadline += period;
    return now >= deadline ? 0 : deadline;
}

#endif
