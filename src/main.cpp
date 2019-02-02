/*
 Gates Control
*/
extern "C" {
    #include "mqtt.h"
}

#include <pgmspace.h>
#include <Ticker.h>
#include "config.h"
#include "webpages.h"
#include "webserver.h"
#include "wdt.h"
#include "credentials.hh"
#include "wifi.h"
#include "gatecontrol.hh"


#define WDT_TIMEOUT_MS              30000
#define MQTT_HB_PERIOD_S            60
//#define MQTT_ENABLED
#define MQTT_CONNECT_TRY_MS         10000

////////////////////////////////////////////////////////////////////////////////
static void         mqttOpenCb(unsigned char *payload, unsigned int length);

GateControl gateCtl;


const char *mqtt_gate_open_topic = "/gate/open";
const char *mqtt_gate_hb_topic   = "/gate/hb";
bool        mqtt_connected;
unsigned    mqtt_last_check;
char        mqtt_pub_buffer[128];


const mqtt_subitem_t mqtt_sublist[] = {
    {.topic = mqtt_gate_open_topic, .cb = mqttOpenCb},
    {.topic = NULL, .cb = NULL},
};

const mqtt_cfg_t mqtt_cfg = {
    .server  = Credentials::mqttBroker,
    .publist = NULL,
    .sublist = mqtt_sublist,
    .username = Credentials::mqttUsername,
    .password = "",
};
////////////////////////////////


Ticker uptimeCtrTimer;


/******************************************************************************/

static void  uptimeCtrCb(void) {

    gateCtl.updateCounters();

#if defined(MQTT_ENABLED)
    if (gateCtl.uptime % MQTT_HB_PERIOD_S == 0) {
        String msg(gateCtrl.uptime);

        mqtt_async_publish(mqtt_gate_hb_topic, msg.c_str());
    }
#endif
}



static void mqttOpenCb(unsigned char *payload, unsigned int length)
{
    auto theGate = static_cast<GateControl::gate_type_t>(payload[0]);

    int res = gateCtl.open(theGate);

    mqtt_async_publish(mqtt_gate_open_topic, (res == 0) ? "OK" : "FAIL");
}

void setup(void)
{
    digitalWrite(LED_PIN,  HIGH);
    digitalWrite(CANCELLINO_PIN, HIGH);
    digitalWrite(CANCELLONE_PIN, HIGH);

    pinMode(LED_PIN,        OUTPUT);
    pinMode(CANCELLINO_PIN, OUTPUT);
    pinMode(CANCELLONE_PIN, OUTPUT);

    //pinMode(BTN_PIN, INPUT);

    uptimeCtrTimer.attach_ms(1000, uptimeCtrCb);

    Serial.begin(115200);

    for (unsigned i = 0; i < 10; i++)
    {
        digitalWrite(LED_PIN,  !digitalRead(LED_PIN));
        delay(100);
    }

    digitalWrite(LED_PIN,  HIGH);

    Serial.print("....Started up!");
    Serial.println("");

    wifiSetup(false, true, Credentials::wifiSsid, Credentials::wifiPasswd);

#if defined(MQTT_ENABLED)
    mqtt_setup(&mqtt_cfg);
#endif

    mqtt_connected  = false;
    mqtt_last_check = 0;

    webpagesInit();

    wdtInit(WDT_TIMEOUT_MS);
    wdtEnable();

    Serial.println("setup done");
}


void loop(void)
{
    unsigned now;
    now = millis();

    wdtKick();

    if (wifiHasConnected()) {
        // flash led to say we have just connected...?

        unsigned prev_state = digitalRead(LED_PIN);

        digitalWrite(LED_PIN, !prev_state);
        delay(100);
        digitalWrite(LED_PIN, prev_state);
    }

#if defined(MQTT_ENABLED)
    if (mqtt_connected) {
        mqtt_check_reconnect();
    }
    else if ((now - mqtt_last_check) > MQTT_CONNECT_TRY_MS) {
        mqtt_connected  = mqtt_check_reconnect();
        mqtt_last_check = now;
    }

    if (mqtt_connected) mqtt_loop();
#endif

    webserverLoop();

}
