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

/*
  Temperature Reading from a MAX6675
  Ryan McLaughlin <ryanjmclaughlin@gmail.com>
*/

#define SO A4    // MISO
#define SCK A3   // Serial Clock
#define TC_0 A2  // CS Pin of MAX6607
int TC_0_calib = 0;  // Calibration compensation value in digital counts (.25[ch730]C)

char OLED_Address = 0x3c;
char OLED_Command_Mode = 0x80;
char OLED_Data_Mode = 0x40;

char h1[10];  // humidity string
char t1[10];  // temperature string
char t2[10];  // temperature string

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

 pinMode(SO, INPUT);
 pinMode(SCK, OUTPUT);

 pinMode(TC_0, OUTPUT);
 digitalWrite(TC_0,HIGH);  // Disable device


//set the one external function to call to update the OLED with a message
Spark.function("update", update);

    Spark.variable("humidity", &h1, STRING);
    Spark.variable("temperature", &t1, STRING);

    Spark.variable("thermocouple", &t2, STRING);
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

  // Read the temperature and print it to serial
  float temp;
  temp = (float)read_temp(TC_0,1,TC_0_calib,10) / 10;
  sprintf(t2, "%.1f", temp);
  sendMessage("   ");
  sendMessage(t2);
  sendData(0xDF);

    // round the value for display
    int t;
    t = int(temp);
    temp = float(t);



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

/* Create a function read_temp that returns an unsigned int
   with the temp from the specified pin (if multiple MAX6675).  The
   function will return 9999 if the TC is open.

   Usage: read_temp(int pin, int type, int error)
     pin: the CS pin of the MAX6675
     type: 0 for [ch730]F, 1 for [ch730]C
     error: error compensation in digital counts
     samples: number of measurement samples (max:10)
*/
unsigned int read_temp(int pin, int type, int error, int samples) {
  unsigned int value = 0;
  int error_tc;
  float temp;
  unsigned int temp_out;

  for (int i=samples; i>0; i--){
    digitalWrite(pin,LOW); // Enable device

    /* Cycle the clock for dummy bit 15 */
    digitalWrite(SCK,HIGH);
    digitalWrite(SCK,LOW);

    /* Read bits 14-3 from MAX6675 for the Temp
       Loop for each bit reading the value and
       storing the final value in 'temp'
    */
    for (int i=11; i>=0; i--){
      digitalWrite(SCK,HIGH);  // Set Clock to HIGH
      value += digitalRead(SO) << i;  // Read data and add it to our variable
      digitalWrite(SCK,LOW);  // Set Clock to LOW
    }

    /* Read the TC Input inp to check for TC Errors */
    digitalWrite(SCK,HIGH); // Set Clock to HIGH
    error_tc = digitalRead(SO); // Read data
    digitalWrite(SCK,LOW);  // Set Clock to LOW

    digitalWrite(pin, HIGH); //Disable Device
  }

  value = value/samples;  // Divide the value by the number of samples to get the average

  /*
     Keep in mind that the temp that was just read is on the digital scale
     from 0[ch730]C to 1023.75[ch730]C at a resolution of 2^12.  We now need to convert
     to an actual readable temperature (this drove me nuts until I figured
     this out!).  Now multiply by 0.25.  I tried to avoid float math but
     it is tough to do a good conversion to [ch730]F.  THe final value is converted
     to an int and returned at x10 power.

   */

  value = value + error;  // Insert the calibration error value

  if(type == 0) {  // Request temp in [ch730]F
    temp = ((value*0.25) * (9.0/5.0)) + 32.0;  // Convert value to [ch730]F (ensure proper floats!)
  } else if(type == 1) {  // Request temp in [ch730]C
    temp = (value*0.25);  // Multiply the value by 25 to get temp in [ch730]C
  }

  temp_out = temp*10;  // Send the float to an int (X10) for ease of printing.

  /* Output 9999 if there is a TC error, otherwise return 'temp' */
  if(error_tc != 0) { return 9999; } else { return temp_out; }
}
