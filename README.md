# espSuite

#### Full-featured Serial bridge for ESP8266, supports MQTT, WebSockets,Raw TCP (Telnet), Server and Client mode and also comes with a nice Configuration Page.

This is great because we don't have to upload code again all the time we want to change to AP Mode or change the password. Just go to the Setup Page, submit and it will be stored in the SPIFFS.

It uses Arduino Libraries [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) for WebSockets, [pubsubclient](https://github.com/knolleary/pubsubclient) for MQTT and [ArduinoJson](https://github.com/bblanchon/ArduinoJson) for JSON Parsing.

#### Flashing:

##### Pre-built:
Bulding from source will allow you to modify initial values, but if you experience trouble, try flashing this binary extracted from a working device.
* Install [esptool](https://github.com/espressif/esptool)
* Run (replace COM4 with your port number):
```
esptool -p COM4 write_flash 0x0 espSuite_1MB.bin
```
##### From-source
* Upload the Sketch to your ESP8266
* Upload Sketch Data using ESP8266 Sketch Data Upload Tool
* Connect to your new AP (SSID is *things* by default) and go to 192.168.1.1 to see the Setup Page

The ESP will now work as an awesome Serial device, always when you `Serial.print("something")`, *something* will be sent using the protocol you've chosen. When you send *something* to its IP address and port (don't forget the '\n' in the end), it will be printed to Arduino too.

##### Things to make it cooler:
* Choose another language (currently, EN-US and PT-BR available) by editing the `indexPath` variable
* Edit `/data/index_xx.html` with the name of your thing or robot. E.g.: **Wall-E Setup**
* You can change or view **any** any initial setting in the JSON files inside the `data` folder

### The Setup Page
![Setup Page](https://raw.githubusercontent.com/Vitorbnc/espSuite/master/config_page.png)


### The Wiring Diagram
###### Just in case you forgot:
![ESP-01 Wiring](https://raw.githubusercontent.com/Vitorbnc/espSuite/master/esp-01_wiring.png)

#### Notes
* Disconnect *DTR* and *RTS* before opening Arduino Serial Monitor
* Make sure to use *Newline*('\n') as line ending char, or change `dataTrailer` in the sketch to use something else.
* If the ESP is resetting:
  * Try flashing Sketch Data and/or Sketch again
  * If you see "exception(28)" on Serial Monitor, try editing the json files (e.g. delete line endings) before flashing Sketch Data
