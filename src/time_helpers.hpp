#pragma once

#define MS_IN_S 1000UL

#define TIME_S_TO_MS(s) (static_cast<unsigned long>(s) * MS_IN_S)
#define TIME_MS_TO_S(ms) (static_cast<unsigned long>(ms) / MS_IN_S)


static inline bool time_lt_24h(unsigned long t1_ms, unsigned long t2_ms)
{
    return (t2_ms - t1_ms) < TIME_S_TO_MS(3600 * 24);
}

