#include "webpages.h"
#include "webserver.h"
#include "ArduinoJson.h"
#include "credentials.hh"
#include "gatecontrol.hh"

extern "C" {
#  include "index_html.h"
#  include "login_html.h"
#  include "apple_touch_icon_png.h"
#  include "favicon_16x16_png.h"
#  include "favicon_32x32_png.h"
#  include "config.h"
}

#define COOKIE_MAX_AGE  (60*60*24*30)

static ESP8266WebServer  *pHttpServer;

//Check if header is present and correct
static bool is_authentified(){
    if (pHttpServer->hasHeader("Cookie")) {
        Serial.print("Found cookie: ");
        String cookie = pHttpServer->header("Cookie");
        Serial.println(cookie);
        if (cookie.indexOf("espSessionId=1") != -1) {
            Serial.println("Auth Successful");
            return true;
        }
    }

    Serial.println("Authentification Failed (cookie not found)");
    return false;
}

static void handleLogout(void)
{
    Serial.println("Disconnection");
    pHttpServer->sendHeader("Location","/login");
    pHttpServer->sendHeader("Cache-Control","no-cache");
    pHttpServer->sendHeader("Set-Cookie", "espSessionId=0; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    pHttpServer->send(301);
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
    if (!is_authentified()){
        String header;
        pHttpServer->sendHeader("Location","/login");
        pHttpServer->sendHeader("Cache-Control","no-cache");
        pHttpServer->send(301);
        return;
    }
    pHttpServer->send_P(200, "text/html", (const char *)index_html, index_html_size);
}

static void serveOpenGate(void)
{
    if (!is_authentified()){
        String header;
        pHttpServer->sendHeader("Location","/login");
        pHttpServer->sendHeader("Cache-Control","no-cache");
        pHttpServer->send(301);
        return;
    }

    auto theGate = static_cast<GateControl::gate_type_t>(pHttpServer->arg("id").toInt());

    int res = gateCtl.open(theGate);

    pHttpServer->send(200, "text/plain", (res  == 0)? "0" : "1");
}

static void serveReboot(void)
{
    if (!is_authentified()){
        String header;
        pHttpServer->sendHeader("Location","/login");
        pHttpServer->sendHeader("Cache-Control","no-cache");
        pHttpServer->send(301);
        return;
    }
    pHttpServer->send(200, "text/plain", "Rebooting...");
    ESP.restart();  // normal reboot
}

static void serveFile(const char *dataPtr, unsigned size) {
    // 1 year
    pHttpServer->sendHeader("Cache-Control", "public, max-age=31536000");
    pHttpServer->send_P(200, "text/html", dataPtr, size);
}


static void serveAppleTouchIcon() {
   serveFile((const char *)apple_touch_icon_png, apple_touch_icon_png_size);
}


static void serveFavIcon16() {
   serveFile((const char *)favicon_16x16_png, favicon_16x16_png_size);
}


static void serveFavIcon32() {
   serveFile((const char *)favicon_32x32_png, favicon_32x32_png_size);
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


static void serveJsonData(void)
{
    StaticJsonBuffer<512> jsonBuffer;
    ArduinoJson::JsonObject& root = jsonBuffer.createObject();

    root["fwUptime"]  =  secondsToTime(gateCtl.uptime, false);
    root["fwVersion"] = "v" FW_VERSION " " __DATE__;

    String tmp;

    for (auto v : gateCtl.lastOpened) {
        tmp += secondsToTime(v, false) + ";";
    }

    root["lastOpened"] = tmp;

    tmp = "";

    for (auto v : gateCtl.openCtr) {
        tmp += String(v) + ";";
    }

    root["openCtr"] = tmp;

    String msg;
    root.printTo(msg);
    pHttpServer->send(200, "text/json", msg);
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
        {"/jsonData",   serveJsonData},
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
