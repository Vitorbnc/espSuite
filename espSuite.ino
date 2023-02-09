
#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define WebServer ESP8266WebServer
#endif
#include <WiFiClient.h>
#include <WebSocketsClient.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include "constants.h"
#include "mem.h"

/* ------------------------------
   Code by Vítor Barbosa (vitorbarbosanc@gmail.com)
*/

//Página de Configuração em Português --------------
//#include "pages/index_br.html.h"
//#define index_file index_br
// ----------------

//English config page ----------------
#include "pages/index_en.html.h"
#define index_file index_en
// ----------------

bool fileDump = false;
bool debug = true;
//bool machineDebug = true;
String protocol="";
String mqttData="";
String pubTopic = "";
String subTopic = "";
String currentTopic="";
String name = "";
// remote host and port (will be overwritten)
String host = "255.255.255.255";
int port = 80;
String mode="";
String wifiMode = "";

int serialBufferSize = 30;
int serialBytes = 0;
char *serialBuffer;

char dataTrailer='\n';

String serialData="";

WebServer webServer(80);
WiFiServer rawServer(23);
WiFiClient rawServer_clients[NET_MAX_CLIENTS];
WiFiClient rawClient;
PubSubClient mqttClient;
WebSocketsServer wsServer = WebSocketsServer(81);
WebSocketsClient wsClient;

bool isAP = false;
bool clientMode = true;
bool ws = false;
bool rawTCP = false;
bool mqtt = false;

const char* cfgSSID = "Config";
const char* cfgKey = "12345678";
IPAddress cfgIP(192, 168, 0, 1);

const int connectTimeout = 20000;

#define uart Serial

bool configPage = false;

// just so we can write the function down there
void handleRoot();
bool handleFileRead(String path);
void wsClientEvent(WStype_t type, uint8_t * payload, size_t length);
void wsServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void printMem(){
        uart.print("DEBUG_LEVEL:");
        uart.println(Mem::readString(Addr::DEBUG_LEVEL,Len::DEBUG_LEVEL));
        uart.print("DEVICE_NAME:");
        uart.println(Mem::readString(Addr::DEVICE_NAME,Len::DEVICE_NAME));
        uart.print("WIFI_MODE:");
        uart.println(Mem::readString(Addr::WIFI_MODE,Len::WIFI_MODE));
}

void setup() {
        uart.begin(BAUD);
        Mem::begin();
        delay(3000);
        Mem::init();
        configPage=true;
        setupWiFi();
        uart.println("Wifi ok");
        
        loadProtocol();
        uart.println("Protos");



        if (configPage) {

                webServer.onNotFound([]() {
                        webServer.send(404, "text/plain", "FileNotFound");
                });

                webServer.on("/", handleRoot);
                webServer.on("/init", HTTP_GET, handleInit);
                webServer.on("/protocol", HTTP_GET, handleProtocol);
                webServer.on("/ap", HTTP_POST, handleAP);
                webServer.on("/sta", HTTP_POST, handleSta);
                webServer.on("/names", HTTP_GET, handleNames);
                webServer.on("/save", handleSave);



                webServer.begin();
                Serial.println("HTTP server started");
        }
}

void loop() {
        if (configPage) {
                webServer.handleClient();
        }
        if(mqtt) mqttBridge();
        else if (ws) wsBridge();
        else if (rawTCP) rawTCPBridge();
}

IPAddress parseIP(String rawTxt) {
        int fields = 4;
        int tmpIP[4];
        // parses the function arguments. E.g.:(100,10,-25)
        String tmp;
        int old_dotIndex = -1;
        bool oneMore = true;
        for (int i = 0; i < fields; i++) {
                int dotIndex = rawTxt.indexOf('.', old_dotIndex + 1);
                if (dotIndex != -1) {
                        tmp = rawTxt.substring(old_dotIndex + 1, dotIndex);
                        char c =  tmp.charAt(0);

                        tmpIP[i] = tmp.toInt();

                        old_dotIndex = dotIndex;
                }
                else if (oneMore) {
                        oneMore = false;
                        tmp = rawTxt.substring(old_dotIndex + 1);
                        char c =  tmp.charAt(0);

                        tmpIP[i] = tmp.toInt();

                }
        }

        return IPAddress(tmpIP[0],tmpIP[1],tmpIP[2],tmpIP[3]);

}


void handleProtocol() {
        if (debug) uart.println("protocol req");
        if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");

        Mem::writeString(webServer.arg("mode"),Addr::NET_MODE,Len::NET_MODE);
        Mem::writeString(webServer.arg("protocol"),Addr::NET_PROTOCOL,Len::NET_PROTOCOL);
        Mem::writeString(webServer.arg("host"),Addr::HOST_IP_STR,Len::HOST_IP_STR);
        Mem::writeInt(webServer.arg("port").toInt(),Addr::HOST_PORT,Len::HOST_PORT);
        Mem::commit();

        handleRoot();
}


void handleInit() {
        if (debug) uart.println("init req");
        if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");

        Mem::writeString(webServer.arg("wifiMode"),Addr::WIFI_MODE,Len::WIFI_MODE);
        Mem::writeString(webServer.arg("debug"),Addr::DEBUG_LEVEL,Len::DEBUG_LEVEL);
        Mem::writeInt(webServer.arg("baud").toInt(),Addr::BAUD,Len::BAUD);
        Mem::commit();

        handleRoot();
}

void handleSave() {
        if (debug) uart.println("saving req");
        //if (!handleFileRead(savePath)) if (debug) uart.println("Failed opening save.html");
}

void handleAP() {
        if (debug) uart.println("ap req");
        if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");

        Mem::writeString(webServer.arg("ssid"),Addr::AP_SSID,Len::AP_SSID);
        Mem::writeString(webServer.arg("key"),Addr::AP_KEY,Len::AP_KEY);
        IPAddress ip = parseIP(webServer.arg("ip"));
        if (debug) uart.print("new device ip:"), uart.println(ip.toString());

        Mem::writeIP(ip,Addr::AP_IP,Len::AP_IP);
        Mem::commit();

        handleRoot();
}

void handleSta() {
        if (debug) uart.println("sta req");
        if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");

        Mem::writeString(webServer.arg("ssid"),Addr::STA_SSID,Len::STA_SSID);
        Mem::writeString(webServer.arg("key"),Addr::STA_KEY,Len::STA_KEY);
        Mem::commit();
        
        handleRoot();
}

void handleNames() {
        if (debug) uart.println("names req");
        if (webServer.args() == 0) return webServer.send(500, "text/plain", "BAD ARGS");

        Mem::writeString(webServer.arg("name"),Addr::DEVICE_NAME,Len::DEVICE_NAME);
        Mem::writeString(webServer.arg("pubTopic"),Addr::PUB_TOPIC,Len::PUB_TOPIC);
        Mem::writeString(webServer.arg("subTopic"),Addr::SUB_TOPIC,Len::SUB_TOPIC);
        Mem::commit();

        handleRoot();
}

//format bytes
String formatBytes(size_t bytes) {
        if (bytes < 1024) {
                return String(bytes) + "B";
        } else if (bytes < (1024 * 1024)) {
                return String(bytes / 1024.0) + "KB";
        } else if (bytes < (1024 * 1024 * 1024)) {
                return String(bytes / 1024.0 / 1024.0) + "MB";
        } else {
                return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
        }
}

String getContentType(String filename) {
        if (webServer.hasArg("download")) return "application/octet-stream";
        else if (filename.endsWith(".htm")) return "text/html";
        else if (filename.endsWith(".html")) return "text/html";
        else if (filename.endsWith(".css")) return "text/css";
        else if (filename.endsWith(".js")) return "application/javascript";
        else if (filename.endsWith(".png")) return "image/png";
        else if (filename.endsWith(".gif")) return "image/gif";
        else if (filename.endsWith(".jpg")) return "image/jpeg";
        else if (filename.endsWith(".ico")) return "image/x-icon";
        else if (filename.endsWith(".xml")) return "text/xml";
        else if (filename.endsWith(".pdf")) return "application/x-pdf";
        else if (filename.endsWith(".zip")) return "application/x-zip";
        else if (filename.endsWith(".gz")) return "application/x-gzip";
        return "text/plain";
}

void handleRoot() {
        //if (!handleFileRead(indexPath)) if (debug) uart.println("Failed opening index.html");
        webServer.send(200, "text/html", index_file);
}


void loadInit() {
        String dbg = Mem::readString(Addr::DEBUG_LEVEL,Len::DEBUG_LEVEL);
        if (strncmp(dbg.c_str(), "full", 4) == 0) fileDump = debug = true;
        else if (strncmp(dbg.c_str(), "debug", 4) == 0) fileDump = false, debug = true;
        else fileDump = debug = false;
        uart.end();
        delay(500);
        uart.begin(Mem::readInt(Addr::BAUD));
        delay(500);

        wifiMode = Mem::readString(Addr::WIFI_MODE,Len::WIFI_MODE);
}


void apMode() {

        WiFi.mode(WIFI_AP);
        if (debug) uart.println("Setting up in AP Mode");
        isAP = true;
        String ssid = Mem::readString(Addr::AP_SSID,Len::AP_SSID);
        String key = Mem::readString(Addr::AP_KEY,Len::AP_KEY);
        int keyLength = sizeof(key)/sizeof(char);
        IPAddress ip = Mem::readIP(Addr::AP_IP,Len::AP_IP);
        bool openAP = false;
        if (strncmp(key.c_str(), "", 1) == 0|| keyLength<8) openAP = true;
        //WiFi.softAPConfig(local_IP, gateway, subnet_mask)
        WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
        if (openAP) {
                WiFi.softAP(ssid.c_str());
                if (debug) uart.println(">dbg.WiFiMode(Open AP)");
        }
        else {
                WiFi.softAP(ssid.c_str(), key.c_str());
                if (debug) uart.println(">dbg.WiFiMode(Safe AP)");
        }

        if (debug) {
                String tmp=""; tmp+=">dbg.IP("; tmp+=ip.toString(); tmp+=")";
                uart.println(tmp);
        }

}

void staMode() {
        WiFi.mode(WIFI_STA);
        if (debug) uart.println(">dbg.WiFiMode(sta)");
        isAP = false;
        //if (fileDump)   { json.printTo(uart); uart.println(); }
        String ssid = Mem::readString(Addr::STA_SSID,Len::STA_SSID);
        String key = Mem::readString(Addr::STA_KEY,Len::STA_KEY);
        if (debug) uart.println(ssid);
        //if(debug) uart.println(key);
        WiFi.begin(ssid.c_str(), key.c_str());

        for(int tryTime=0; tryTime<connectTimeout; tryTime+=500) {
                if(WiFi.status() != WL_CONNECTED) {
                        delay(500);
                        Serial.print(".");
                }
                else break;
        }
        if (WiFi.status() == WL_CONNECTED) {
                if (debug) {
                        uart.println(">dbg.connected()");
                        String tmp=""; tmp+=">dbg.ip(";
                        uart.print(tmp);
                        uart.print(WiFi.localIP());
                        uart.println(")");
                }
        }
        else {
                if (debug) {
                        uart.println(">dbg.failed()");
                }
                configPage=true;
                staMode();
        }

}


void setupWiFi() {

        loadInit();
        loadNames();

        if (wifiMode == "ap") apMode();

        else if (wifiMode == "sta")  staMode();

}

void rawClientConnect() {
        if (!rawClient.connect(host.c_str(), port)) {
                if (debug) uart.println("connection failed");
        }
        else if (debug) uart.println("connected!");


}

void mqttConnect() {
        // Loop until we're reconnected
        while (!mqttClient.connected()) {
                if(debug) {
                        String tmp=""; tmp+=">dbg.MQTT("; tmp+=host; tmp+=","; tmp+=port; tmp+=")";
                        uart.println(tmp);
                        tmp+=">dbg.name( "; tmp+=name; tmp+=")";
                        uart.println(tmp);
                }
                // Attempt to connect
                if (mqttClient.connect(name.c_str())) {
                        if(debug) uart.println(">dbg.connected()");
                        // Once connected, publish an announcement...
                        String tmp=""; tmp+="Hi,I'm "; tmp+=name;
                        mqttClient.publish(pubTopic.c_str(), tmp.c_str());
                        mqttClient.subscribe(subTopic.c_str());
                        if(debug) {
                                String pub=""; pub+=">dbg.pubTopic("; pub+=pubTopic; pub+=")";
                                String sub=""; sub+=">dbg.subTopic( "; sub+=subTopic; sub+=")";
                                uart.println(sub);
                                uart.println(pub);
                        }


                } else {
                        if(debug) uart.println(">dbg.failed()");
                        if(debug) uart.println(">dbg.retry(5s)");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}

void setupProtocol() {
        if (clientMode) {
                if(isAP && mqtt){ 
                        mqtt = false, ws = true;
                        if(debug) uart.println("MQTT and AP mode won't work well, changing to WS");
        
                }
                if (ws) {
                        wsClient.begin(host, port);
                        if (debug) {
                                uart.println(">dbg.protocol(WS)");
                                String tmp=""; tmp+=">dbg.wsClient("; tmp+=host; tmp+=","; tmp+=port; tmp+=")";
                                uart.println(tmp);

                        }
                        wsClient.onEvent(wsClientEvent);
                }
                else if (rawTCP) {
                        if (debug) uart.println(">dbg.protocol(TCP)");

                        rawClientConnect();
                }

                else if (mqtt) {
                        if (debug) uart.println(">dbg.protocol(MQTT)");
                        mqttClient.setClient(rawClient);
                        mqttClient.setServer(host.c_str(),port);
                        mqttClient.setCallback(callback);
                        mqttConnect();
                }


        }
        // server mode
        else {

                if (ws) {
                        wsServer.begin();
                        wsServer.onEvent(wsServerEvent);
                        if(debug) {
                                String tmp=""; tmp+=">dbg.wsServer("; tmp+=81; tmp+=")";
                                uart.println(tmp);
                        }
                }
                else if (rawTCP) {
                    rawServer.begin();
                   // rawServer.setNoDelay(true);
                        if(debug) {
                                String tmp=""; tmp+=">dbg.rawServer("; tmp+=23; tmp+=")";
                                uart.println(tmp);
                        }
                }
        }

}

void loadProtocol() {
        uart.println("loading protocols");
        protocol+= Mem::readString(Addr::NET_PROTOCOL,Len::NET_PROTOCOL);
        mode+= Mem::readString(Addr::NET_MODE,Len::NET_MODE);
        host+= Mem::readString(Addr::HOST_IP_STR,Len::HOST_IP_STR);
        port = Mem::readInt(Addr::HOST_PORT);
        clientMode = !(strncmp(mode.c_str(), "server", 4) == 0);
        if (strncmp(protocol.c_str(), "raw", 2) == 0)           ws = false, rawTCP = true, mqtt = false;
        else if (strncmp(protocol.c_str(), "ws", 2) == 0)       ws = true, rawTCP = false, mqtt = false;
        else if (strncmp(protocol.c_str(),"mqtt", 2)==0)        ws = false, rawTCP = false,  mqtt = true;
        
        setupProtocol();
}

void loadNames() {
        subTopic += Mem::readString(Addr::SUB_TOPIC,Len::SUB_TOPIC);
        pubTopic += Mem::readString(Addr::PUB_TOPIC,Len::PUB_TOPIC);
        name += Mem::readString(Addr::DEVICE_NAME,Len::DEVICE_NAME);
        if(fileDump) {
                uart.print("Subscription topic:"); uart.println(subTopic);
                uart.print("Publishing topic:"); uart.println(pubTopic);
                uart.print("Name:"); uart.println(name);
        }
}

void callback(char* topic, byte* payload, unsigned int length) {
        //  if(debug) {
        //  String tmp=""; tmp+= "MQTT message received at topic "; tmp+=topic;
        //  uart.println(tmp);
        //  }
        mqttData ="";
        //currentTopic ="";
        //currentTopic+=topic;
        for (int i = 0; i < length; i++) {
                mqttData+=(char)payload[i];
        }
        uart.print(mqttData);


}


void wsClientEvent(WStype_t type, uint8_t * payload, size_t length) {
        String data="";
        if(type==WStype_TEXT) {

                for (int i = 0; i < length; i++) data+= (char)payload[i];
                uart.print(data);

        }
        else if(type== WStype_DISCONNECTED) {
                if (debug) uart.printf("[WSc] Disconnected!\n");
        }
        else if(type==WStype_CONNECTED)      {
                if (debug) uart.printf("[WSc] Connected to url: %s\n",  payload);

        }



}

void wsServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
        String data="";
        if(type==WStype_TEXT) {

                for (int i = 0; i < length; i++) data+= (char)payload[i];
                uart.print(data);

        }
        else if(type== WStype_DISCONNECTED) {
                if (debug) uart.printf("[WSc] Disconnected!\n");
        }
        else if(type==WStype_CONNECTED)      {
                if (debug) uart.printf("[WSc] Connected to url: %s\n",  payload);

        }

}

// This function was just adapted from WiFiTelnetToSerial example from ESP8266 Core for Arduino
void rawServer_handleClients(){
  uint8_t i;
  //check if there are any new clients
  if (rawServer.hasClient()){
    for(i = 0; i < NET_MAX_CLIENTS; i++){
      //find free/disconnected spot
      if (!rawServer_clients[i] || !rawServer_clients[i].connected()){
        if(rawServer_clients[i]) rawServer_clients[i].stop();
        rawServer_clients[i] = rawServer.available();
        //Serial1.print("New client: "); Serial1.print(i);
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = rawServer.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < NET_MAX_CLIENTS; i++){
    if (rawServer_clients[i] && rawServer_clients[i].connected()){
      if(rawServer_clients[i].available()){
        //get data from the telnet client and push it to the UART
        while(rawServer_clients[i].available()) uart.write(rawServer_clients[i].read());
      }
    }
  }
  //check UART for data
  if(uart.available()){
    size_t len = uart.available();
    uint8_t sbuf[len];
    uart.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < NET_MAX_CLIENTS; i++){
      if (rawServer_clients[i] && rawServer_clients[i].connected()){
        rawServer_clients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
}


void rawTCPBridge() {
        if (clientMode) {
                if (!rawClient.connected()) rawClientConnect();
                while (rawClient.available() > 0) {
                        char c = rawClient.read();
                        uart.print(c);
                }
                while (uart.available() > 0) {
                        char c = uart.read();
                        rawClient.print(c);
                }
        }
        else {
             rawServer_handleClients();
         }

}


void wsBridge() {
        if (clientMode) {
                wsClient.loop();
                if(uart.available()) {
                        serialData ="";
                        serialData=uart.readStringUntil(dataTrailer);
                        wsClient.sendTXT(serialData);
                }


        }
        else {
                wsServer.loop();
                if(uart.available()) {
                        serialData ="";
                        serialData=uart.readStringUntil(dataTrailer);
                        wsServer.sendTXT(0,serialData);
                }
        }
}

void mqttBridge(){
        if(!mqttClient.loop())  mqttConnect();

        if(uart.available()) {
                serialData ="";
                serialData=uart.readStringUntil(dataTrailer);
                mqttClient.publish(pubTopic.c_str(),serialData.c_str());
        }
}
