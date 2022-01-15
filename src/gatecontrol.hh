#pragma once
#include "config.h"

class GateControl
{
private:
    void relayPulse(unsigned pin, unsigned int pulseDuration)
    {
        digitalWrite(pin, LOW);
        digitalWrite(GPIO_LED, LOW);
        delay(pulseDuration);
        digitalWrite(pin, HIGH);
        digitalWrite(GPIO_LED, HIGH);
    }

public:

    typedef enum {
        GATE_CANCELLINO = 0,
        GATE_CANCELLONE = 1,
    } gate_type_t;

    static const char numOfGates = 2;
    unsigned uptime;
    unsigned lastOpenedSeconds[numOfGates];
    unsigned openCtr[numOfGates];

    void updateCounters() {
        uptime++;

        for (auto &t :lastOpenedSeconds) {
            t++;
        }
#if defined(MQTT_ENABLED)
    String msg(uptimeSeconds);

    mqtt_async_publish(mqtt_gate_hb_topic, msg.c_str());
#endif
    }

    int open(gate_type_t theGate) {
        unsigned pin;

        switch (theGate) {
            case GATE_CANCELLINO:
                pin = CANCELLINO_PIN;
                break;
            case GATE_CANCELLONE:
                pin = CANCELLONE_PIN;
                break;
            default:
                return -1;
        }

        relayPulse(pin, RELAY_OPEN_PULSE_MS);
        lastOpenedSeconds[theGate] = 0;
        openCtr[theGate]++;

        return 0;
    }
};

extern GateControl gateCtl;
