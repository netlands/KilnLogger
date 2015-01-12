//*************************************//
// --- WIDE.HK---//
// - SSD131x PMOLED Controller -//
// - SCL, SDA, GND, VCC(3.3v 5v) --//
//*************************************//

/*
****************************************
This code is rewritten from the original code for the Arduino to control the I2C OLED from http://Wide.HK

The code below works to use the SparkCore microcontrller (available at http://spark.io )
Pins used on the SparkCore:

DO - SDA
D1 - SCL
3V
GND

see here for pins: http://docs.spark.io/#/hardware/pins-and-i-o-i2c

To call this method and update the display you can use cURL like this:

curl https://api.spark.io/v1/devices/{YOURDEVICEID}/update -d access_token={YOURACCESSTOKEN} -d "args=Hello World,Line 2 Text"

Make sure you replace the {YOURDEVICEID} and {YOURACCESSTOKEN} with information from your SparkCore
****************************************
*/

/* DHT22 Blocking Version https://gist.github.com/wgbartley/8301123 */

char OLED_Address = 0x3c;
char OLED_Command_Mode = 0x80;
char OLED_Data_Mode = 0x40;

char h1[10];  // humidity string
char t1[10];  // temperature string

#define MAXTIMINGS 85

#define cli noInterrupts
#define sei interrupts

#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

#define NAN 999999

class DHT {
    private:
        uint8_t data[6];
        uint8_t _pin, _type, _count;
        bool read(void);
        unsigned long _lastreadtime;
        bool firstreading;

    public:
        DHT(uint8_t pin, uint8_t type, uint8_t count=6);
        void begin(void);
        float readTemperature(bool S=false);
        float convertCtoF(float);
        float readHumidity(void);

};


DHT::DHT(uint8_t pin, uint8_t type, uint8_t count) {
    _pin = pin;
    _type = type;
    _count = count;
    firstreading = true;
}


void DHT::begin(void) {
    // set up the pins!
    pinMode(_pin, INPUT);
    digitalWrite(_pin, HIGH);
    _lastreadtime = 0;
}


//boolean S == Scale.  True == Farenheit; False == Celcius
float DHT::readTemperature(bool S) {
    float _f;

    if (read()) {
        switch (_type) {
            case DHT11:
                _f = data[2];

                if(S)
                    _f = convertCtoF(_f);

                return _f;


            case DHT22:
            case DHT21:
                _f = data[2] & 0x7F;
                _f *= 256;
                _f += data[3];
                _f /= 10;

                if (data[2] & 0x80)
                    _f *= -1;

                if(S)
                    _f = convertCtoF(_f);

                return _f;
        }
    }

    return NAN;
}


float DHT::convertCtoF(float c) {
    return c * 9 / 5 + 32;
}


float DHT::readHumidity(void) {
    float _f;
    if (read()) {
        switch (_type) {
            case DHT11:
                _f = data[0];
                return _f;


            case DHT22:
            case DHT21:
                _f = data[0];
                _f *= 256;
                _f += data[1];
                _f /= 10;
                return _f;
        }
    }

    return NAN;
}


bool DHT::read(void) {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    unsigned long currenttime;

    // pull the pin high and wait 250 milliseconds
    digitalWrite(_pin, HIGH);
    delay(250);

    currenttime = millis();
    if (currenttime < _lastreadtime) {
        // ie there was a rollover
        _lastreadtime = 0;
    }

    if (!firstreading && ((currenttime - _lastreadtime) < 2000)) {
        //delay(2000 - (currenttime - _lastreadtime));
        return true; // return last correct measurement
    }

    firstreading = false;
    Serial.print("Currtime: "); Serial.print(currenttime);
    Serial.print(" Lasttime: "); Serial.print(_lastreadtime);
    _lastreadtime = millis();

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    // now pull it low for ~20 milliseconds
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(20);
    cli();
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);
    pinMode(_pin, INPUT);

    // read in timings
    for ( i=0; i< MAXTIMINGS; i++) {
        counter = 0;

        while (digitalRead(_pin) == laststate) {
            counter++;
            delayMicroseconds(1);

            if (counter == 255)
                break;
        }

        laststate = digitalRead(_pin);

        if (counter == 255)
            break;

        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            data[j/8] <<= 1;

            if (counter > _count)
                data[j/8] |= 1;

            j++;
        }
    }

    sei();


    // check we read 40 bits and that the checksum matches
    if ((j >= 40) &&  (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
        return true;


    return false;
}

#define DHTPIN D4    // Digital pin D2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

float h;    // humidity
float t;    // temperature
int f = 0;  // failed?


void setup()
{
//begin Wire communication with OLED
Wire.begin();

dht.begin();

//set the one external function to call to update the OLED with a message
Spark.function("update", update);

    Spark.variable("humidity", &h1, STRING);
    Spark.variable("temperature", &t1, STRING);

//initialize screen
SetupScreen();

sendCommand(0x01); // ** Clear display
sendMessage("started");

delay(1000);//wait one sec to see the started message

sendCommand(0x01); // ** Clear display
}

void loop()
{

	delay(2500);
    f = 0;
    h = dht.readHumidity();
    t = dht.readTemperature();

    if (t==NAN || h==NAN)
        f = 1;
    else
        f = 0;

  sendCommand(0x01); // ** Clear display
        sprintf(h1, "%.1f", h); // convert Float to String
        sprintf(t1, "%.1f", t);
  sendMessage(t1);
	sendData(0xDF);
	sendCommand(0xC0); // ** New Line
	sendMessage(h1);
  sendData(0x25);
}

int update(String args)
{
int status_code = 0;

sendCommand(0x01); // ** Clear display

args.replace("%20"," ");

int commaPosition = args.indexOf(",");//find if there is a delim character

if(commaPosition>-1){
//two lines
sendMessage(args.substring(0,commaPosition));//send first part
sendCommand(0xC0); // ** New Line
sendMessage(args.substring(commaPosition+1, args.length()));//send second part

status_code = 2;//lines

}else{
//one line
sendMessage(args);

status_code = 1;//lines
}

//how many lines sent to display
return status_code;
}

void SetupScreen(void){
// * I2C initial * //
delay(100);
sendCommand(0x2A); // ** Set "RE"=1 00101010B
sendCommand(0x71);
sendCommand(0x5C);
sendCommand(0x28);

sendCommand(0x08); // ** Set Sleep Mode On
sendCommand(0x2A); // ** Set "RE"=1 00101010B
sendCommand(0x79); // ** Set "SD"=1 01111001B

sendCommand(0xD5);
sendCommand(0x70);
sendCommand(0x78); // ** Set "SD"=0

sendCommand(0x08); // ** Set 5-dot, 3 or 4 line(0x09), 1 or 2 line(0x08)

sendCommand(0x06); // ** Set Com31->Com0 Seg0->Seg99

// ** Set OLED Characterization * //
sendCommand(0x2A); // ** Set "RE"=1
sendCommand(0x79); // ** Set "SD"=1

// ** CGROM/CGRAM Management * //
sendCommand(0x72); // ** Set ROM
sendCommand(0x00); // ** Set ROM A and 8 CGRAM

sendCommand(0xDA); // ** Set Seg Pins HW Config
sendCommand(0x10);

sendCommand(0x81); // ** Set Contrast
sendCommand(0xFF);

sendCommand(0xDB); // ** Set VCOM deselect level
sendCommand(0x30); // ** VCC x 0.83

sendCommand(0xDC); // ** Set gpio - turn EN for 15V generator on.
sendCommand(0x03);

sendCommand(0x78); // ** Exiting Set OLED Characterization
sendCommand(0x28);
sendCommand(0x2A);
//sendCommand(0x05); // ** Set Entry Mode
sendCommand(0x06); // ** Set Entry Mode
sendCommand(0x08);
sendCommand(0x28); // ** Set "IS"=0 , "RE" =0 //28
sendCommand(0x01);
sendCommand(0x80); // ** Set DDRAM Address to 0x80 (line 1 start)

delay(100);
sendCommand(0x0C); // ** Turn on Display

}

void sendData(unsigned char data)
{
Wire.beginTransmission(OLED_Address); // ** Start I2C
Wire.write(OLED_Data_Mode); // ** Set OLED Data mode
Wire.write(data);
Wire.endTransmission(); // ** End I2C
}

void sendCommand(unsigned char command)
{
Wire.beginTransmission(OLED_Address); // ** Start I2C
Wire.write(OLED_Command_Mode); // ** Set OLED Command mode
Wire.write(command);
Wire.endTransmission(); // ** End I2C
delay(10);
}

void sendMessage(String message)
{
unsigned char i=0;
while(message[i])
{
sendData(message[i]); // * Show String to OLED
i++;
}
}
