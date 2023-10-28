#include "webpages.h"
#include "webserver.h"
#include "ArduinoJson.h"
#include "credentials.hh"
#include "gatecontrol.hh"
#include "energymeter.hh"

extern "C" {
#  include "index_html.h"
#  include "login_html.h"
#  include "apple_touch_icon_png.h"
#  include "favicon_16x16_png.h"
#  include "favicon_32x32_png.h"
#  include "manifest_json.h"
#  include "config.h"
}

#define COOKIE_MAX_AGE  (60*60*24*30)

static ESP8266WebServer  *pHttpServer;

//Check if header is present and correct
static bool isAuthenticated(){
    if (pHttpServer->hasHeader("Cookie")) {
        Serial.print("Found cookie: ");
        String cookie = pHttpServer->header("Cookie");
        Serial.println(cookie);
        if (cookie.indexOf("espSessionId=1") != -1) {
            Serial.println("Auth Successful");
            return true;
        }
    }

    Serial.println("Authentication Failed (cookie not found)");
    return false;
}

static void httpRedirect(const char *path) {
    pHttpServer->sendHeader("Location", path);
    pHttpServer->sendHeader("Cache-Control","no-cache");
    pHttpServer->send(301);
}

static void handleLogout(void)
{
    Serial.println("Disconnection");
    pHttpServer->sendHeader("Set-Cookie", "espSessionId=0; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    httpRedirect("/login");
    return;
}

//login page, also called for disconnect
static void handleLogin(){
    String msg;

    if (pHttpServer->hasHeader("Cookie")){
        Serial.print("Found cookie: ");
        String cookie = pHttpServer->header("Cookie");
        Serial.println(cookie);
    }

    if (pHttpServer->hasArg("USERNAME") && pHttpServer->hasArg("PASSWORD")){

        if (pHttpServer->arg("USERNAME") == Credentials::httpUsername &&
            pHttpServer->arg("PASSWORD") == Credentials::httpPassword) {

            pHttpServer->sendHeader("Location","/");
            pHttpServer->sendHeader("Cache-Control","no-cache");
            pHttpServer->sendHeader("Set-Cookie", "espSessionId=1; Max-Age=" + String(COOKIE_MAX_AGE));
            pHttpServer->send(301);
            Serial.println("Log in Successful");

            return;
        }

        msg = "Wrong username/password! try again.";
        Serial.println("Log in Failed");
    }

    pHttpServer->send_P(200, "text/html", (const char *)login_html, login_html_size);
}

static void serveMainPage(void)
{
    if (!isAuthenticated()){
        httpRedirect("/login");
        return;
    }
    pHttpServer->send_P(200, "text/html", (const char *)index_html, index_html_size);
}

static void serveOpenGate(void)
{
    if (!isAuthenticated()){
        httpRedirect("/login");
        return;
    }

    auto theGate = static_cast<GateControl::gate_type_t>(pHttpServer->arg("id").toInt());

    int res = gateCtl.open(theGate);

    pHttpServer->send(200, "text/plain", (res  == 0)? "0" : "1");
}

static void serveReboot(void)
{
    if (!isAuthenticated()){
        httpRedirect("/login");
        return;
    }
    pHttpServer->send(200, "text/plain", "Rebooting...");
    ESP.restart();  // normal reboot
}

static void serveFile(const char *dataPtr, unsigned size, const char* contentType) {
    // 1 year
    pHttpServer->sendHeader("Cache-Control", "public, max-age=31536000");
    pHttpServer->send_P(200, contentType, dataPtr, size);
}


static void serveAppleTouchIcon() {
   serveFile((const char *)apple_touch_icon_png, apple_touch_icon_png_size, "image/apng");
}


static void serveFavIcon16() {
   serveFile((const char *)favicon_16x16_png, favicon_16x16_png_size, "image/apng");
}


static void serveFavIcon32() {
   serveFile((const char *)favicon_32x32_png, favicon_32x32_png_size, "image/apng");
}


static String secondsToTime(unsigned s, bool addSeconds) {
    String res;
    char tmp[40];


    unsigned dd = s / (3600*24);
    unsigned hh = (s % (3600*24)) / 3600;
    unsigned mm = (s % 3600) / 60;
    sprintf(tmp, "%ug, %02uh:%02um", dd, hh, mm);
    res = String(tmp);

    if (addSeconds) {
        unsigned ss = (s % 60);

        sprintf(tmp, ":%02us", ss);

        res += String(tmp);
    }

    return res;
}

static
void
serveJsonData (
    void
    )
{
    ArduinoJson::StaticJsonDocument<512> jsonDoc;
    String tmp;

    //
    // FW / uptime info
    //
    jsonDoc["fwUptime"]  =  secondsToTime(gateCtl.uptime, false);
    jsonDoc["fwVersion"] = "v" FW_VERSION " " __DATE__;

    //
    // Gates Info
    //
    for (auto v : gateCtl.lastOpenedSeconds) {
        tmp += secondsToTime(v, false) + ";";
    }
    jsonDoc["lastOpened"] = tmp;

    tmp = "";
    for (auto v : gateCtl.openCtr) {
        tmp += String(v) + ";";
    }
    jsonDoc["openCtr"] = tmp;

    EnergyMeter::GetInstance()->GetCounters(jsonDoc);

    //
    // Serialize and send
    //
    tmp = "";
    serializeJson(jsonDoc, tmp);
    pHttpServer->send(200, "text/json", tmp);
}

static
void
serveMetrics (
    void
    )
{
    String content;
    unsigned long pulseCounter;
    unsigned long timeBetweenPulsesMs;
    unsigned long timeSinceLastPulse;
    constexpr unsigned long whInPulse = 10;

    EnergyMeter::GetInstance()->GetCounters(&pulseCounter,
                                            &timeBetweenPulsesMs,
                                            &timeSinceLastPulse);

    const unsigned long watts = whInPulse * 3600 * 1000 / timeBetweenPulsesMs;

    content = R"(
# HELP home_energy_watts Instantaneous Watts absorbed
# TYPE home_energy_watts gauge
home_energy_watts )" +
              String(watts);

    content += R"(

# HELP home_energy_time_between_pulses Time in ms between the last two pulses
# TYPE home_energy_time_between_pulses gauge
home_energy_time_between_pulses )" +
               String(timeBetweenPulsesMs);

    content += R"(

# HELP home_energy_time_since_last_pulse Time in ms since last observed pulse
# TYPE home_energy_time_since_last_pulse gauge
home_energy_time_since_last_pulse )" +
               String(timeSinceLastPulse);

    content += R"(

# HELP home_energy_pulse_counter The number of pulses measured
# TYPE home_energy_pulse_counter counter
home_energy_pulse_counter )" +
               String(pulseCounter);

    content += R"(

# HELP home_energy_uptime The system uptime in seconds.
# TYPE home_energy_uptime counter
home_energy_uptime )" +
               String(gateCtl.uptime);

    pHttpServer->send(200, "text/plain", content);
}

void webpagesInit(void)
{
    //here the list of headers to be recorded
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);

    const ws_dynamic_page_t dyn_pages[] = {
        {"/",       serveMainPage},
        {"/reboot", serveReboot},
        {"/open",   serveOpenGate},
        {"/login",  handleLogin},
        {"/logout", handleLogout},
        {"/jsonData", serveJsonData},
        {"/metrics", serveMetrics},
        {"/apple-touch-icon.png", serveAppleTouchIcon},
        {"/favicon-16x16.png",    serveFavIcon16},
        {"/favicon-32x32.png",    serveFavIcon32},
        {NULL, NULL}
    };

    webserverInit(true, NULL, dyn_pages);
    pHttpServer = webserverGetObjectPtr();

    //ask server to track these headers
    pHttpServer->collectHeaders(headerkeys, headerkeyssize );
}
