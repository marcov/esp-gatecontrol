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
#include "energymeter.hh"

#define WDT_TIMEOUT_MS              30000
#define MQTT_HB_PERIOD_S            60
//#define MQTT_ENABLED
#define MQTT_CONNECT_TRY_MS         10000

////////////////////////////////////////////////////////////////////////////////

GateControl gateCtl;
static constexpr unsigned int k_MaxConnectTime = 1000UL;
WIFI* g_WIFI;

#if defined(MQTT_ENABLED)
const char *mqtt_gate_open_topic = "/gate/open";
const char *mqtt_gate_hb_topic   = "/gate/hb";
bool        mqtt_connected;
unsigned    mqtt_last_check;
char        mqtt_pub_buffer[128];

static const mqtt_subitem_t mqtt_sublist[] = {
    {.topic = mqtt_gate_open_topic, .cb = mqttOpenCb},
    {.topic = NULL, .cb = NULL},
};

static const mqtt_cfg_t mqtt_cfg = {
    .server  = Credentials::mqttBroker,
    .publist = NULL,
    .sublist = mqtt_sublist,
    .username = Credentials::mqttUsername,
    .password = "",
};

static void         mqttOpenCb(unsigned char *payload, unsigned int length);
#endif

////////////////////////////////

static Ticker uptimeCtrTimer;

/******************************************************************************/

static
void
uptimeCtrCb (
    void
    )
{
    gateCtl.updateCounters();

    EnergyMeter::GetInstance()->IncrementCounters();

#if defined(MQTT_ENABLED)
    if (gateCtl.uptime % MQTT_HB_PERIOD_S == 0) {
        String msg(gateCtl.uptime);

        mqtt_async_publish(mqtt_gate_hb_topic, msg.c_str());
    }
#endif
}

#if defined(MQTT_ENABLED)
static void mqttOpenCb(unsigned char *payload, unsigned int length)
{
    auto theGate = static_cast<GateControl::gate_type_t>(payload[0]);

    int res = gateCtl.open(theGate);

    mqtt_async_publish(mqtt_gate_open_topic, (res == 0) ? "OK" : "FAIL");
}
#endif

static
void
ConnectedCallback (
    void
    )
{
    unsigned int originalState;

    originalState = digitalRead(GPIO_LED);

    for (int i = 0; i < 4; i++)
    {
        //
        // Flash led to say we have just connected.
        //
        digitalWrite(GPIO_LED, (i % 2) ? originalState : !originalState);
        delay(100);
    }

    digitalWrite(GPIO_LED, originalState);
}

void
setup (
    void
    )
{
    g_WIFI = new WIFI(false, true, Credentials::wifiSsid,Credentials::wifiPasswd, k_MaxConnectTime);
    //
    // Force instantiation here.
    //
    EnergyMeter::GetInstance();

    digitalWrite(GPIO_LED,  HIGH);
    digitalWrite(CANCELLINO_PIN, HIGH);
    digitalWrite(CANCELLONE_PIN, HIGH);

    pinMode(GPIO_LED,        OUTPUT);
    pinMode(CANCELLINO_PIN, OUTPUT);
    pinMode(CANCELLONE_PIN, OUTPUT);

    pinMode(LIGHT_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LIGHT_SENSOR_PIN),
                    EnergyMeter::LightPulseIsr,
                    RISING);

    uptimeCtrTimer.attach_ms(1000, uptimeCtrCb);

    Serial.begin(115200);

    for (unsigned i = 0; i < 10; i++)
    {
        digitalWrite(GPIO_LED,  !digitalRead(GPIO_LED));
        delay(100);
    }

    digitalWrite(GPIO_LED,  HIGH);

    Serial.println("....starting up");

    g_WIFI->Connect(5000);
    if (g_WIFI->IsConnected())
    {
        ConnectedCallback();
    }

#if defined(MQTT_ENABLED)
    mqtt_setup(&mqtt_cfg);

    mqtt_connected  = false;
    mqtt_last_check = 0;
#endif

    webpagesInit();

    wdtInit(WDT_TIMEOUT_MS);
    wdtEnable();

    Serial.println("setup done");
}

void
loop (
    void
    )
{
    unsigned now;
    now = millis();

    wdtKick();

    g_WIFI->Loop(ConnectedCallback);

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

    EnergyMeter::GetInstance()->Loop(now);
}
