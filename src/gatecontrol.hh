#ifndef __GATECONTROL_HH__
#define __GATECONTROL_HH__

#include "config.h"

class GateControl {

private:
    void relayPulse(unsigned pin)
    {
        digitalWrite(pin, LOW);
        digitalWrite(LED_PIN, LOW);
        delay(RELAY_PULSE_MS);
        digitalWrite(pin, HIGH);
        digitalWrite(LED_PIN, HIGH);
    }


public:
    typedef enum {
        GATE_CANCELLINO = 0,
        GATE_CANCELLONE = 1,
    } gate_type_t;

    static const char numOfGates = 2;
    unsigned uptime;
    unsigned lastOpened[numOfGates];
    unsigned openCtr[numOfGates];

    void updateCounters() {
        uptime++;

        for (auto &t :lastOpened) {
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

        relayPulse(pin);
        lastOpened[theGate] = 0;
        openCtr[theGate]++;

        return 0;
    }
};

extern GateControl gateCtl;

#endif /* #ifndef __GATECONTROL_HH__ */
