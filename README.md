# espSuite

####Full-featured Serial bridge for ESP8266, supports MQTT, WebSockets,Pure TCP, Server and Client mode and also comes with a nice Configuration Page.

This is great because we don't have to upload code again all the time we want to change to AP Mode or change the password. Just go to the Setup Page, submit and it will be stored in the SPIFFS.

It uses Arduino Libraries [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) for WebSockets and [pubsubclient](https://github.com/knolleary/pubsubclient) for MQTT.

You just have to:
* Upload the Sketch to your ESP8266
* Upload Sketch Data using ESP8266 Sketch Data Upload Tool
* Connect to your new AP (SSID is *things* by default) and go to 192.168.1.1 to see the Setup Page

The ESP will now work as an awesome Serial device, always when you `Serial.print("something")`, *something* will be sent using the protocol you've chosen. When you send *something* to its IP address and port (don't forget the '\n' in the end), it will be printed to Arduino too.

###The Setup Page
![Setup Page](https://raw.githubusercontent.com/Vitorbnc/espSuite/master/config_page.png)
