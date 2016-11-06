/*
 * Reading External temperature sensor
   Wireless Remote Sensor SWS THS
   http://www.sencor.eu/wireless-remote-sensor/sws-ths
 *
 */

#include <Arduino.h>


#define DATA_PIN 14  //D5 in Wemos



//Definice pro externi senzory
#define DataBits0 4                                       // Number of data0 bits to expect
#define DataBits1 32                                      // Number of data1 bits to expect
#define allDataBits 36                                    // Number of data sum 0+1 bits to expect
// isrFlags bit numbers
#define F_HAVE_DATA 0                                     // 0=Nothing in read buffer, 1=Data in read buffer
#define F_GOOD_DATA 0                                     // 0=Unverified data, 1=Verified (2 consecutive matching reads)
#define F_CARRY_BIT 3                                     // Bit used to carry over bit shift from one long to the other
#define F_STATE 1                                         // 0=Sync mode, 1=Data mode
// Constants
const unsigned long sync_MIN = 4300;                      // Adjusting the timing pulse external sensors
const unsigned long sync_MAX = 4700;
const unsigned long bit1_MIN = 2300;
const unsigned long bit1_MAX = 2700;
const unsigned long bit0_MIN = 1330;
const unsigned long bit0_MAX = 1730;
const unsigned long glitch_Length = 300;                  // Anything below this value is a glitch and will be ignored.
// Interrupt variables
unsigned long fall_Time = 0;                              // Placeholder for microsecond time when last falling edge occured.
unsigned long rise_Time = 0;                              // Placeholder for microsecond time when last rising edge occured.
byte bit_Count = 0;                                       // Bit counter for received bits.
unsigned long build_Buffer[] = {0,0};                     // Placeholder last data packet being received.
volatile unsigned long read_Buffer[] = {0,0};             // Placeholder last full data packet read.
volatile byte isrFlags = 0;                               // Various flag bits





bool b_receive = false;
float a_temperature[4];
short a_humidity[4], battery_reading[4],  m_channel;
String a_battery[3] = {"0%","50%","100%"};    //a_battery G 0%, F 50%, E 100%



void dec2binLong(unsigned long myNum, byte NumberOfBits);
double dewPoint(double ltemperature, double lhumidity);
double heatIndex(double ltemperature, double lhumidity);
void ResetReceiver();
void showReadings(short l_channel,float l_temperature, short l_humidity, short l_battery_reading);
void readDataFromPin();
boolean readDataFromBuffers();




void setup()
{
        Serial.begin(9600);
        pinMode(DATA_PIN,INPUT);
        attachInterrupt(DATA_PIN,readDataFromPin,CHANGE);
        delay(500);
        Serial.println("Starting");
}


void loop()
{

//Resetting variables for income 433 overflow micros - on hard reset
        if ((micros() > 0) && (micros() < 1000000))
        {
                b_receive = false;
                ResetReceiver();
        } else
        {
                b_receive = true;
        }

        if (readDataFromBuffers() == 1) {
                showReadings(m_channel,a_temperature[m_channel], a_humidity[m_channel], battery_reading[m_channel]);
        }

}

boolean readDataFromBuffers()
{
        unsigned long myData0 = 0;
        unsigned long myData1 = 0;
        if (bitRead(isrFlags,F_GOOD_DATA) == 1)
        {
                Serial.println("=========================================================================");
                //  Serial.println("bitRead(isrFlags,F_GOOD_DATA) == 1");
                myData0 = read_Buffer[0]; // Read the data spread over 2x 32 variables
                myData1 = read_Buffer[1];
                bitClear(isrFlags,F_HAVE_DATA); // Flag we have read the data
                bitClear(isrFlags,F_GOOD_DATA); // Flag we have read the data
                dec2binLong(myData0,DataBits0);
                dec2binLong(myData1,DataBits1);
                byte H = ((myData1 >> 24) & 0x3) + 1; // Get Channel
                m_channel = H;
                H = (myData1 >> 26) & 0x3; // Get Battery
                if (m_channel==1) battery_reading[1]=H;
                if (m_channel==2) battery_reading[2]=H;
                if (m_channel==3) battery_reading[3]=H;

                byte ML = (myData1 >> 12) & 0xF0; // Get MMMM
                H = (myData1 >> 12) & 0xF; // Get LLLL
                ML = ML | H;      // OR MMMM & LLLL nibbles together
                H = (myData1 >> 20) & 0xF; // Get HHHH
                byte HH = 0;
                if((myData1 >> 23) & 0x1 == 1) //23 bit
                        HH = 0xF;
                int Temperature = (H << 8)|(HH << 12) | ML; // Combine HHHH MMMMLLLL
                // Temperature = Temperature*3; //F // Remove Constant offset
                if (m_channel==1) a_temperature[1]=(float) Temperature / 10;
                if (m_channel==2) a_temperature[2]=(float) Temperature / 10;
                if (m_channel==3) a_temperature[3]=(float) Temperature / 10;

                H = (myData1 >> 0) & 0xF0; // Get HHHH
                ML = (myData1 >> 0) & 0xF; // Get LLLL
                ML = ML | H;      // OR HHHH & LLLL nibbles together
                if (m_channel==1) a_humidity[1]=(ML);
                if (m_channel==2) a_humidity[2]=(ML);
                if (m_channel==3) a_humidity[3]=(ML);
                return 1;

        }
        return 0;
}


void showReadings(short l_channel,float l_temperature, short l_humidity, short l_battery_reading)
{
        Serial.print("Channel ");
        Serial.println(l_channel);

        Serial.print("Temparature = ");
        Serial.print(String(l_temperature));
        Serial.println(" °C");

        Serial.print("Humidity = ");
        Serial.print(String(l_humidity));
        Serial.println(" %RH");

        //http://www.decatur.de/javascript/dew/
        Serial.print("Dew Point = ");
        Serial.println(String(dewPoint(l_temperature, l_humidity)));

        Serial.print("Heat Index = ");
        Serial.println(String(heatIndex(l_temperature, l_humidity)));


        Serial.print("Battery =");
        Serial.println(a_battery[l_battery_reading]);
}




//Resetting variables for income 433MHz due to overflowing micros
void ResetReceiver()
{
        //  Serial.println("ResetReceiver");
        fall_Time = 0;
        rise_Time = 0;
        bit_Count = 0;
        build_Buffer[0] = 0;
        build_Buffer[1] = 0;
        read_Buffer[0] = 0;
        read_Buffer[1] = 0;
        isrFlags = 0;
}




double dewPoint(double ltemperature, double lhumidity)
{
        double A0= 373.15/(273.15 + ltemperature);
        double SUM = -7.90298 * (A0-1);
        SUM += 5.02808 * log10(A0);
        SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1);
        SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1);
        SUM += log10(1013.246);
        double VP = pow(10, SUM-3) * lhumidity;
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558-T);
}

double heatIndex(double ltemperature, double lhumidity)
{
        double c1 = -42.379, c2 = 2.04901523, c3 = 10.14333127, c4 = -0.22475541, c5= -6.83783e-3, c6=-5.481717e-2, c7=1.22874e-3, c8=8.5282e-4, c9=-1.99e-6;
        double Tc = ltemperature;
        double Tf;
        double R = lhumidity;

        Tf = (((9*Tc)/5)+32);
        double rv = ((c1+(c2*Tf))+(c3*R)+(c4*Tf*R)+(c5*(Tf*Tf))+(c6*(R*R))+(c7*((Tf*Tf)*R))+(c8*(Tf*(R*R)))+(c9*((Tf*Tf)*(R*R))));
        double rvc = (5*(rv-32)/9);
        return rvc;
}



void dec2binLong(unsigned long myNum, byte NumberOfBits) //decode BIN DEC for external sensors
{
        if (NumberOfBits <= 32)
        {
                myNum = myNum << (32 - NumberOfBits);
                for (int i=0; i<NumberOfBits; i++)
                {
                        if (bitRead(myNum,31) == 1) ;
                        else
                                myNum = myNum << 1;
                }
        }

}
void readDataFromPin()
{                                   // Pin 2 (Interrupt 0) service routine
        if (b_receive == true)
        {

                //Serial.println("b_receive");
                unsigned long long timem = micros();            // Get current time
                if (digitalRead(DATA_PIN) == LOW) {
// Falling edge

                        if (timem > (rise_Time + glitch_Length)) {
                                //  Serial.println("digitalRead");
// Not a glitch
                                timem = micros() - fall_Time; // Subtract last falling edge to get pulse time.
                                if (bitRead(build_Buffer[1],31) == 1)
                                        bitSet(isrFlags, F_CARRY_BIT);
                                else
                                        bitClear(isrFlags, F_CARRY_BIT);

                                if (bitRead(isrFlags, F_STATE) == 1) {

// Looking for Data
                                        if ((timem > bit0_MIN) && (timem < bit0_MAX)) {
// 0 bit
                                                build_Buffer[1] = build_Buffer[1] << 1;
                                                build_Buffer[0] = build_Buffer[0] << 1;
                                                if (bitRead(isrFlags,F_CARRY_BIT) == 1)
                                                        bitSet(build_Buffer[0],0);
                                                bit_Count++;
                                        }
                                        else if ((timem > bit1_MIN) && (timem < bit1_MAX)) {
// 1 bit
                                                build_Buffer[1] = build_Buffer[1] << 1;
                                                bitSet(build_Buffer[1],0);
                                                build_Buffer[0] = build_Buffer[0] << 1;
                                                if (bitRead(isrFlags,F_CARRY_BIT) == 1)
                                                        bitSet(build_Buffer[0],0);
                                                bit_Count++;
                                        }
                                        else {
// Not a 0 or 1 bit so restart data build and check if it's a sync?
                                                bit_Count = 0;
                                                build_Buffer[0] = 0;
                                                build_Buffer[1] = 0;
                                                bitClear(isrFlags, F_GOOD_DATA); // Signal data reads dont' match
                                                bitClear(isrFlags, F_STATE); // Set looking for Sync mode
                                                if ((timem > sync_MIN) && (timem < sync_MAX)) {
                                                        // Sync length okay
                                                        bitSet(isrFlags, F_STATE); // Set data mode
                                                }
                                        }
                                        if (bit_Count >= allDataBits) {
                                                // All bits arrived
                                                //  Serial.println("All bits arrived!");
                                                bitClear(isrFlags, F_GOOD_DATA); // Assume data reads don't match
                                                if (build_Buffer[0] == read_Buffer[0]) {
                                                        if (build_Buffer[1] == read_Buffer[1])
                                                                bitSet(isrFlags, F_GOOD_DATA); // Set data reads match
                                                }
                                                read_Buffer[0] = build_Buffer[0];
                                                read_Buffer[1] = build_Buffer[1];
                                                bitSet(isrFlags, F_HAVE_DATA); // Set data available
                                                bitClear(isrFlags, F_STATE); // Set looking for Sync mode
                                                build_Buffer[0] = 0;
                                                build_Buffer[1] = 0;
                                                bit_Count = 0;
                                        }

                                }
                                else {
// Looking for sync
                                        if ((timem > sync_MIN) && (timem < sync_MAX)) {
// Sync length okay
                                                build_Buffer[0] = 0;
                                                build_Buffer[1] = 0;
                                                bit_Count = 0;
                                                bitSet(isrFlags, F_STATE); // Set data mode
                                        }
                                }
                                fall_Time = micros();     // Store fall time
                        }

                }
                else {
// Rising edge
                        if (timem > (fall_Time + glitch_Length)) {
                                // Not a glitch
                                rise_Time = timem;         // Store rise time
                        }
                }
        }
}
