# Sencor-SWS-THS
# Tap data from wireless remote sensor Sencor SWS THS

I have Digital Weather Station from Sencor with external remote sensor model - SWS THS

http://www.sencor.eu/weather-stations/sws-51-b
http://www.sencor.eu/wireless-remote-sensor/sws-ths
<img src="https://github.com/erkobg/Sencor-SWS-THS/raw/master/1.jpg" width="200">    <img src="https://github.com/erkobg/Sencor-SWS-THS/raw/master/2.jpg" width="200">


It was working very good for me all the time.
Recently I bought some 433 RF receivers and was playing around,

<img src="https://github.com/erkobg/Sencor-SWS-THS/raw/master/3.jpg" width="300">


 when I decided to try to tap data from this external device and read it in Arduino/Wemos.

I started looking for different ways to achieve this goal, started reading different reverse engineering tutorials, like:

http://hackaday.com/2015/03/01/reverse-engineering-wireless-temperature-probes/
http://rayshobby.net/reverse-engineer-wireless-temperature-humidity-rain-sensors-part-1/
https://rayshobby.net/interface-with-remote-power-sockets-final-version/

But being lazy I searched the web for any existing codes.
I came around this example:
http://arduino.cz/meteostanice-ovladana-arduinem/

It is done by Czech enthusiast : **Zbyšek Voda** and he already decoded and provided main elements of the SWS THS protocol.

The result is obvious:

![Result](https://github.com/erkobg/Sencor-SWS-THS/raw/master/4.png)


----------
