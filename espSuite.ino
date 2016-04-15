
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsClient.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FS.h>

/* ------------------------------
   Code by Vítor Barbosa (vitorbarbosanc@gmail.com), also using some parts of above libraries' examples.

   Don't forget to upload data alongside with this code to your ESP

   ESP8266 tested configs:
   Blue model: 512K Flash, 64KB SPIFFS
   Black model: 1M Flash, 64KB SPIFFS

 */


const int defaultBaud=230400;
bool fileDump = false;
bool debug = false;
//bool machineDebug = true;
char protocol[10];
String mqttData="";
String pubTopic = "";
String subTopic = "";
String currentTopic="";
String name = "";
// remote host and port (will be overwritten)
char* host= "255.255.255.255";
int port = 80;
char mode[10];
String wifiMode = "";

int serialBufferSize = 30;
int serialBytes = 0;
char *serialBuffer;

char dataTrailer='\n';

String serialData="";

ESP8266WebServer server(80);
//WiFiServer rawServer(80);
WiFiClient rawClient;
PubSubClient mqttClient;
WebSocketsServer wsServer = WebSocketsServer(81);
WebSocketsClient wsClient;

bool isAP = false;
bool clientMode = true;
bool ws = false;
bool rawTCP = false;
bool mqtt = false;

int tmpIP[4];

const char* cfgSSID = "Config";
const char* cfgKey = "12345678";
IPAddress cfgIP(192, 168, 0, 1);

const int connectTimeout = 20000;

#define uart Serial

bool configPage = false;

// Choose between index_br, index_en for different langauge
String indexPath = "/index_en.html";
String savePath = "/save.html";
String initPath = "/init.json";
String apPath = "/ap.json";
String staPath = "/sta.json";
String protocolPath = "/protocol.json";
String namesPath = "/names.json";


// just so we can write the function down there
void handleRoot();
bool handleFileRead(String path);
void wsClientEvent(WStype_t type, uint8_t * payload, size_t length);
void wsServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
        delay(1500);
        uart.begin(defaultBaud);
        SPIFFS.begin();
        {
                Dir dir = SPIFFS.openDir("/");
                while (dir.next()) {
                        String fileName = dir.fileName();
                        size_t fileSize = dir.fileSize();
                        if (debug) uart.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
                }
                if (debug) uart.printf("\n");
        }
        delay(500);
        //readBtn();
        configPage=true;
        setupWiFi();
        loadProtocol();


        if (configPage) {

                server.onNotFound([]() {
                        if (!handleFileRead(server.uri()))
                                server.send(404, "text/plain", "FileNotFound");
                });

                server.on("/", handleRoot);
                server.on("/init", HTTP_GET, handleInit);
                server.on("/protocol", HTTP_GET, handleProtocol);
                server.on("/ap", HTTP_POST, handleAP);
                server.on("/sta", HTTP_POST, handleSta);
                server.on("/names", HTTP_GET, handleNames);
                server.on("/save", handleSave);



                server.begin();
                Serial.println("HTTP server started");
        }
}

void loop() {
        if (configPage) {
                server.handleClient();
        }
        if(mqtt) mqttBridge();
        else if (ws) wsBridge();
        else if (rawTCP) rawTCPBridge();



}

void parseIP(String rawTxt) {
        int fields = 4;
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

}


void handleProtocol() {
        if (debug) uart.println("protocol req");
        if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["mode"] = server.arg("mode");
        json["protocol"] = server.arg("protocol");
        json["host"] = server.arg("host");
        json["port"] = server.arg("port").toInt();


        File xfile = SPIFFS.open(protocolPath, "w");
        if (!xfile) {
                Serial.println("Failed to open file as w");
                return;
        }
        if (fileDump) json.printTo(uart);
        json.printTo(xfile);

        xfile.close();
        handleRoot();
}


void handleInit() {
        if (debug) uart.println("init req");
        if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["wifiMode"] = server.arg("wifiMode");
        json["debug"] = server.arg("debug");
        json["baud"] = server.arg("baud").toInt();
        File xfile = SPIFFS.open(initPath, "w");
        if (!xfile) {
                Serial.println("Failed to open file as w");
                return;
        }
        if (fileDump) json.printTo(uart);
        json.printTo(xfile);
        xfile.close();
        handleRoot();
}

void handleSave() {

        if (debug) uart.println("saving req");
        if (!handleFileRead(savePath)) if (debug) uart.println("Failed opening save.html");
}

void handleAP() {
        if (debug) uart.println("ap req");
        if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["ssid"] = server.arg("ssid");
        json["key"] = server.arg("key");
        json["ip"] = server.arg("ip");
        File xfile = SPIFFS.open(apPath, "w");
        if (!xfile) {
                Serial.println("Failed to open file as w");
                return;
        }
        if (fileDump) json.printTo(uart);
        json.printTo(xfile);
        xfile.close();
        handleRoot();
}

void handleSta() {
        if (debug) uart.println("sta req");
        if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["ssid"] = server.arg("ssid");
        json["key"] = server.arg("key");
        File xfile = SPIFFS.open(staPath, "w");
        if (!xfile) {
                Serial.println("Failed to open file as w");
                return;
        }
        if (fileDump) json.printTo(uart);
        json.printTo(xfile);
        xfile.close();
        handleRoot();
}

void handleNames() {
        if (debug) uart.println("names req");
        if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["name"] = server.arg("name");
        json["pubTopic"] = server.arg("pubTopic");
        json["subTopic"] = server.arg("subTopic");

        File xfile = SPIFFS.open(namesPath, "w");
        if (!xfile) {
                Serial.println("Failed to open file as w");
                return;
        }
        if (fileDump) json.printTo(uart);
        json.printTo(xfile);
        xfile.close();
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
        if (server.hasArg("download")) return "application/octet-stream";
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

bool handleFileRead(String path) {
        if (debug) uart.println("handleFileRead: " + path);
        if (path.endsWith("/")) path += "index.htm";
        String contentType = getContentType(path);
        String pathWithGz = path + ".gz";
        if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
                if (SPIFFS.exists(pathWithGz))
                        path += ".gz";
                File file = SPIFFS.open(path, "r");
                size_t sent = server.streamFile(file, contentType);
                file.close();
                return true;
        }
        return false;
}


File openFile(String path) {
        if (debug) uart.print("Opening: "); if (debug) uart.println(path);
        File file = SPIFFS.open(path, "r");
        if (!file) if (debug) uart.println("Failed");
                else if (debug) uart.println("Success!");
        return file;

}


void handleRoot() {
        if (!handleFileRead(indexPath)) if (debug) uart.println("Failed opening index.html");
        //server.send(200, "text/html", "<h1>You are connected</h1>");
}


JsonObject& loadAndParse(String path) {
        File file = openFile(path);

        size_t size = file.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]); // use buf.get() instead of buf with this unique pointer
        // char buf[size];
        file.readBytes(buf.get(), size);
        StaticJsonBuffer<210> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        //json.printTo(uart);
        if (json.success()) {
                if (debug) uart.print("Successfully parsed "); if (debug) uart.println(path);
        } else {
                if (debug) uart.print("Failed to parse "); if (debug) uart.println(path);
        }
        file.close();
        return json;

}

void loadInit() {
        delay(500);
        JsonObject& json = loadAndParse(initPath);
        const char * dbg = json["debug"];
        if (strncmp(dbg, "full", 4) == 0) {
                fileDump = true;
                debug = true;
        }
        else if (strncmp(dbg, "debug", 4) == 0) {
                fileDump = false;
                debug = true;
        }
        else {
                fileDump = false;
                debug = false;
        }

        int baudRate = json["baud"];
        uart.end();
        delay(100);
        uart.begin(baudRate);
        delay(100);

        if (fileDump)
                json.printTo(uart);
        if (debug) uart.println();

        const char *tmp = json["wifiMode"];
        wifiMode += tmp;
        if (debug) uart.println(wifiMode);
}


void apMode() {

        WiFi.mode(WIFI_AP);
        if (debug) uart.println("Setting up in AP Mode");
        isAP = true;
        JsonObject&  json = loadAndParse(apPath);
        if (fileDump)  { json.printTo(uart); uart.println(); }
        const char *ssid = json["ssid"];
        const char *key = json["key"];
        int keyLength = sizeof(key)/sizeof(char);
        const char *rawIP = json["ip"];
        String strIP = rawIP;
        parseIP(strIP);
        IPAddress ip(tmpIP[0], tmpIP[1], tmpIP[2], tmpIP[3]);
        bool openAP = false;
        if (strncmp(key, "", 1) == 0|| keyLength<8) openAP = true;
        //WiFi.softAPConfig(local_IP, gateway, subnet_mask)
        WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
        if (openAP) {
                WiFi.softAP(ssid);
                if (debug) uart.println(">dbg.WiFiMode(Open AP)");
        }
        else {
                WiFi.softAP(ssid, key);
                if (debug) uart.println(">dbg.WiFiMode(Safe AP)");
        }

        if (debug) {
                String tmp=""; tmp+=">dbg.IP("; tmp+=ip; tmp+=")";
                uart.println(tmp);
        }

}

void staMode() {
        WiFi.mode(WIFI_STA);
        if (debug) uart.println(">dbg.WiFiMode(sta)");
        isAP = false;
        JsonObject&  json = loadAndParse(staPath);
        if (fileDump)   { json.printTo(uart); uart.println(); }
        const char *ssid = json["ssid"];
        const char *key = json["key"];
        if (debug) uart.println(ssid);
        //if(debug) uart.println(key);
        WiFi.begin(ssid, key);

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
        if (!rawClient.connect(host, port)) {
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
                if(isAP) {
                        if(mqtt) {
                                mqtt=false;
                                if(debug) uart.println("MQTT and AP mode won't work well, changing to WS");
                                ws=true;
                        }
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
                        mqttClient.setServer(host,port);
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
                                String tmp=""; tmp+=">dbg.wsServer("; tmp+=port; tmp+=")";
                                uart.println(tmp);
                        }
                }
                else if (rawTCP) {
                        // not implemented
                }
        }

}

void loadProtocol() {

        JsonObject&  json = loadAndParse(protocolPath);
        //strcpy(dest, src)
        strcpy(protocol, json["protocol"]);
        strcpy(mode, json["mode"]);
        const char *rawIP = json["host"];
        strcpy(host, rawIP);
        port = json["port"];
        if (strncmp(mode, "server", 4) == 0) {
                clientMode = false;
        }
        else clientMode = true;
        if (strncmp(protocol, "raw", 2) == 0) {
                ws = false;
                rawTCP = true;
                mqtt=false;
        }
        else if (strncmp(protocol, "ws", 2) == 0) {
                ws = true;
                rawTCP = false;
                mqtt=false;
        }
        else if (strncmp(protocol,"mqtt", 2)==0) {
                ws=false;
                rawTCP=false;
                mqtt=true;
        }
        setupProtocol();

}

void loadNames() {
        JsonObject&  json = loadAndParse(namesPath);
        const char* inTopic = json["subTopic"];
        subTopic += inTopic;
        const char* outTopic = json["pubTopic"];
        pubTopic += outTopic;
        const char* rawName = json["name"];
        name += rawName;
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
                // no server mode implemented
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
