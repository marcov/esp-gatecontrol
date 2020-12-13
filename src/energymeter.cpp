#include "energymeter.hh"

#define ENABLE_DEBUG 0

#if ENABLE_DEBUG
#  define DEBUG(str)  Serial.println(str)
#else
#  define DEBUG(str)  do {/**/} while (0)
#endif

EnergyMeter* EnergyMeter::s_Singleton = nullptr;

EnergyMeter::EnergyMeter (
    void
    )
{
    m_lightPulseCounter = 0;
    m_timeBetweenPulses = PULSE_HOLDOFF_MS(500);
    m_lastPulseTs = 0;
    m_timeSincePulse = PULSE_HOLDOFF_MS(500);

    m_LastIsrTime = 0;
    m_IsrState = ISR_WAIT;
}

EnergyMeter::~EnergyMeter (
    void
    )
{
    if (s_Singleton != nullptr)
    {
        delete s_Singleton;
        s_Singleton = nullptr;
    }
}

void
EnergyMeter::IncrementCounters (
    void
    )
{
    m_timeSincePulse++;
}

void
ICACHE_RAM_ATTR
EnergyMeter::LightPulseIsr (
    void
    )
{
    GetInstance()->m_IsrState = ISR_ACTIVE;
    detachInterrupt(digitalPinToInterrupt(LIGHT_SENSOR_PIN));
}

void
EnergyMeter::Loop (
    unsigned long Now
    )
{
    if (m_IsrState == ISR_ACTIVE)
    {
        m_IsrState = ISR_HOLD;
        m_LastIsrTime = Now;

        m_lightPulseCounter++;
        m_timeBetweenPulses = Now - m_lastPulseTs;
        m_lastPulseTs = Now;
        m_timeSincePulse = 0;
    }

    if ((m_IsrState == ISR_HOLD) && (Now - m_LastIsrTime) >= PULSE_HOLDOFF_MS(WATTHOURS_MAX))
    {
        //
        // Re-enable Interrupt
        //
        m_IsrState = ISR_WAIT;
        attachInterrupt(digitalPinToInterrupt(LIGHT_SENSOR_PIN),
                        LightPulseIsr,
                        RISING);
    }
}

void
EnergyMeter::GetCounters (
    ArduinoJson::JsonDocument& jsonDoc
    )
{
    jsonDoc["pulseCounter"] = String(m_lightPulseCounter);
    jsonDoc["timeBetweenPulses"] = String(m_timeBetweenPulses);
    jsonDoc["timeSincePulse"] = String(m_timeSincePulse);
}

void
EnergyMeter::GetCounters (
    unsigned long* PulseCounter,
    unsigned long* TimeBetweenPulses,
    unsigned long* TimeSinceLastPulse
    )
{
    *PulseCounter = m_lightPulseCounter;
    *TimeBetweenPulses = m_timeBetweenPulses;
    *TimeSinceLastPulse = m_timeSincePulse;
}
