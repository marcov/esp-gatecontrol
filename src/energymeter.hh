#pragma once
#include "config.h"
#include "Arduino.h"
#include "ArduinoJson.h"

class EnergyMeter
{

//
// Energy meter max allowed measure Wh
//
#define WATTHOURS_MAX               5000

#define WH_FOR_ONE_PULSE            10

#define PULSES_PER_HOUR(watthours) \
    ((watthours) / WH_FOR_ONE_PULSE)

#define PULSE_HOLDOFF_MS(watthours) \
    ((3600uL * 1000uL) / PULSES_PER_HOUR(watthours))

public:
    void
    IncrementCounters (
        void
        );

    void
    Loop (
        unsigned long Now
        );

    void
    GetCounters (
        ArduinoJson::JsonDocument& jsonDoc
        ) const;

    void
    GetCounters (
        unsigned long* PulseCounter,
        unsigned long* TimeBetweenPulses,
        unsigned long* TimeSinceLastPulse
        ) const;

    unsigned long
    GetPower (
            void
            ) const
    {
        if (m_TBetweenPulsesMs == 0)
        {
            return 0;
        }

        return k_WhInOnePulse * 3600 * 1000 / m_TBetweenPulsesMs;
    }

private:
    EnergyMeter (
        void
        );

    ~EnergyMeter (
        void
        );

private:
    static EnergyMeter* s_Singleton;

public:
    static
    EnergyMeter *
    GetInstance (
        void
        )
    {
        if (s_Singleton == nullptr)
        {
            s_Singleton = new EnergyMeter();
        }

        return s_Singleton;
    }

    static
    void
    LightPulseIsr (
        void
        );

private:
    typedef enum
    {
        ISR_WAIT,
        ISR_ACTIVE,
        ISR_HOLD,
    } IsrState;

    IsrState m_IsrState;
    unsigned long m_lightPulseCounter;
    unsigned long m_TBetweenPulsesMs;
    unsigned long m_lastPulseTs;
    unsigned long m_timeSincePulse;
    unsigned long m_LastIsrTime;
    static constexpr unsigned long k_WhInOnePulse = 10;
};
