#ifndef MEM_H
#define MEM_H
#include <Arduino.h>
#include "constants.h"

#ifdef ESP32
#include <EEPROM.h>
#define EEPROMr EEPROM
#else
#include <EEPROM_Rotate.h>
EEPROM_Rotate EEPROMr;
#endif


namespace Mem{

inline bool commit(){return EEPROMr.commit();}

bool begin(){
    #ifdef ESP32
    return EEPROMr.begin(EEPROM_SIZE);
    #else
    uint8_t size = 0;
    if (EEPROMr.last() > 1000) size = 4;         // 4Mb boards
    else if (EEPROMr.last() > 250) size = 2;     // 1Mb boards
    else size = 1;
    EEPROMr.size(size);
    EEPROMr.begin(EEPROM_SIZE);
    return true;
    #endif

}

String readString(int addr, int len){
    String tmp="";
    for(int i = 0;i<len;i++) tmp+= (char) EEPROMr.read(addr+i);
    return tmp;
}
void writeString(const String &str, int addr, int len){
    for(int i = 0;i<len;i++) EEPROMr.write(addr+i,str[i]);
}
void writeString(const char* str, int addr, int len){
    for(int i = 0;i<len;i++) EEPROMr.write(addr+i,str[i]);
}

IPAddress readIP(int addr){
    return IPAddress(EEPROMr.read(addr),EEPROMr.read(addr+1),EEPROMr.read(addr+2),EEPROMr.read(addr+3));
}
void writeIP(uint32_t ip,int addr){
    for(int i = 0;i<4;i++) EEPROMr.write(addr+i,(ip>>8*i)&0xff); //MSB first
}

int readInt(int addr){
    return(EEPROMr.read(addr)<<24 | EEPROMr.read(addr+1)<<16 | EEPROMr.read(addr+2)<< 8| EEPROMr.read(addr+3)); //MSB first
}
void writeInt(int num,int addr){
    for(int i = 0;i<4;i++) EEPROMr.write(addr+i,(num>>24-8*i)&0xff); //MSB first
}
int readInitCode(){
    return readInt(INIT_ADDR);
}
bool isInitialized(){
    return (readInt(INIT_ADDR) == INIT_CODE);
}

inline String readString(Addr addr, Len len){return readString((int) addr, (int) len);}

inline IPAddress readIP(Addr addr){return readIP((int) addr);}
inline IPAddress readIP(Addr addr, Len len){return readIP((int) addr);}


inline int readInt(Addr addr){return readInt((int) addr);}
inline int readInt(Addr addr, Len len){return readInt((int) addr);}

inline void writeIP(uint32_t ip,int addr, int len){writeIP(ip,addr);}
inline void writeIP(uint32_t ip,Addr addr){writeIP(ip,(int) addr);}
inline void writeIP(uint32_t ip,Addr addr, Len len){writeIP(ip,(int) addr);}

inline void writeInt(int num, int addr, int len){writeInt(num,addr);}
inline void writeInt(int num, Addr addr) {writeInt(num, (int) addr);}
inline void writeInt(int num, Addr addr, Len len){writeInt(num, (int) addr);}

inline void writeString(const String &str, Addr addr, Len len){writeString(str, (int) addr, (int) len); };

void initWithDefaults(){

    writeInt(BAUD,Addr::BAUD);
    writeString(DEBUG_LEVEL,Addr::DEBUG_LEVEL,Len::DEBUG_LEVEL);
    writeString(WIFI_MODE,Addr::WIFI_MODE,Len::WIFI_MODE);
    
    writeString(STA_KEY,Addr::STA_KEY,Len::STA_KEY);
    writeString(STA_SSID,Addr::STA_SSID,Len::STA_SSID);
    writeIP(STA_IP,Addr::STA_IP);

    writeString(AP_KEY,Addr::AP_KEY,Len::AP_KEY);
    writeString(AP_SSID,Addr::AP_SSID,Len::AP_SSID);
    writeIP(AP_IP,Addr::AP_IP);

    writeString(NET_MODE,Addr::NET_MODE,Len::NET_MODE);
    writeString(NET_PROTOCOL,Addr::NET_PROTOCOL,Len::NET_PROTOCOL);
    writeString(HOST_IP_STR,Addr::HOST_IP_STR,Len::HOST_IP_STR);
    writeIP(HOST_IP,Addr::HOST_IP);
    writeInt(HOST_PORT,Addr::HOST_PORT);
    writeInt(NET_MAX_CLIENTS,Addr::NET_MAX_CLIENTS);

    writeString(DEVICE_NAME,Addr::DEVICE_NAME,Len::DEVICE_NAME);
    writeString(PUB_TOPIC,Addr::PUB_TOPIC,Len::PUB_TOPIC);
    writeString(SUB_TOPIC,Addr::SUB_TOPIC,Len::SUB_TOPIC);

    writeInt(INIT_CODE,INIT_ADDR);
    commit();
}

void init(){
    if(!isInitialized()) initWithDefaults();
}

};
#endif