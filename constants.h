#ifndef ESPSUITE_CONSTANTS_H
#define ESPSUITE_CONSTANTS_H
//#include "stdint.h"

// Change init code to force EEPROM update! -------------------------------------------------------------------
#define INIT_CODE 998 //Arbitrary code to check if eeprom was initialized, up to 32bit size
// ------------------------------------------------------------------------------------------------------------

#define EEPROM_SIZE 4096
#define ADDR_OFFSET 10 //Required when using library eeprom_rotate
#define INIT_ADDR ADDR_OFFSET
#define INIT_LEN 4        //init code length (bytes)
#define IP(X3,X2,X1,X0) (X3<< 24 | X2<< 16 | X1<<8 | X0) 

// EDIT DEFAULT SETTINGS HERE  --------------------------------------------------------------------------

const char* AP_SSID = "thingsAP";       //32 chars max as per standards
const char* AP_KEY = "";                //63 chars max as per standards
const uint32_t AP_IP = IP(192,168,1,1);

const char* STA_SSID = "myAP";          //32 chars max as per standards
const char* STA_KEY = "12345678910";    //63 chars max as per standards
const uint32_t STA_IP = 0;              // Not used

const char* WIFI_MODE = "ap";           //ap | sta
const char* DEBUG_LEVEL = "debug";      // debug | full | none
const int BAUD = 115200;

const char* NET_MODE = "server";        // server | client
const char* NET_PROTOCOL = "raw";       // server | raw | mqtt | ws 
const char* HOST_IP_STR = "192.168.0.5";
const uint32_t HOST_IP = IP(192,168,0,5); //not used

const uint16_t HOST_PORT = 1883;        
const uint16_t NET_MAX_CLIENTS = 2;

const char* DEVICE_NAME = "another_thing";  // mainly for mqtt
const char* PUB_TOPIC = "things";
const char* SUB_TOPIC = "things";

// DEFAULT SETTINGS END HERE --------------------------------------------------------------------------


enum class Len {

    STA_SSID =      32,
    AP_SSID =       32,//+32, 32 chars max as per standards

    AP_IP =         4,//+4, ipv4, 4 bytes
    STA_IP =        4, //+4
    BAUD =          4, // max 32-bit (4 billion)
    HOST_IP =       4, //+4 ------- not used --------
    HOST_PORT =     4, //+4, actual max is 16-bit (2bytes or 65535)
    NET_MAX_CLIENTS = 4,

    WIFI_MODE =     8, // +8, sta or ap
    DEBUG_LEVEL =   8, //+8, all, code, none
    NET_MODE =      8, //+8 server or client
    NET_PROTOCOL =  8, //+8 mqtt, ws, raw

    AP_KEY =        64,//+64, 63 chars as per standards
    STA_KEY =       64,//+64
    DEVICE_NAME =   64, //max 64 chars
    PUB_TOPIC =     64,
    SUB_TOPIC =     64,
    HOST_IP_STR =   64, 

};

enum class Addr {   

    STA_SSID =          ADDR_OFFSET + INIT_LEN + 0*32,//+32
    AP_SSID =           ADDR_OFFSET + INIT_LEN + 1*32,//+32, 32 chars max as per standards

    AP_IP =             ADDR_OFFSET + INIT_LEN + 2*32 + 0*4,//+4, ipv4, 4 bytes
    STA_IP =            ADDR_OFFSET + INIT_LEN + 2*32 + 1*4, //+4
    BAUD =              ADDR_OFFSET + INIT_LEN + 2*32 + 2*4, // max 32-bit (4 billion)
    HOST_IP =           ADDR_OFFSET + INIT_LEN + 2*32 + 3*4, //+4 ----- not used ------------
    HOST_PORT =         ADDR_OFFSET + INIT_LEN + 2*32 + 4*4, //+4, actual max is 16-bit (2bytes or 65535)
    NET_MAX_CLIENTS =   ADDR_OFFSET + INIT_LEN + 2*32 + 5*4,


    WIFI_MODE =         ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 0*8, // +8, sta or ap
    DEBUG_LEVEL =       ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 1*8, //+8, all, code, none
    NET_MODE =          ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 2*8, //+8 server or client
    NET_PROTOCOL =      ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 3*8, //+8 mqtt, ws, raw

    AP_KEY =            ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +0*64,//+64, 63 chars as per standards
    STA_KEY =           ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +1*64,//+64
    DEVICE_NAME =       ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +2*64, //max 64 chars
    PUB_TOPIC =         ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +3*64,
    SUB_TOPIC =         ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +4*64,
    HOST_IP_STR =       ADDR_OFFSET + INIT_LEN + 2*32 + 6*4 + 4*8 + 0*16 +5*64,


    LAST = SUB_TOPIC

};

#if LAST > EEPROM_SIZE
#error Maximum EEPROM size exceeded!!
#endif
#endif