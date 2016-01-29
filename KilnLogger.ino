// This #include statement was automatically added by the Particle IDE.
#include "PietteTech_DHT.h"

// This #include statement was automatically added by the Particle IDE.
#include "clickButton.h"

#include "QueueArray.h"

/*
	Temperature Reading from a MAX6675
	Ryan McLaughlin <ryanjmclaughlin@gmail.com>
*/

#define SO A4		// MISO
#define SCK A3	 // Serial Clock
#define TC_0 A2	// CS Pin of MAX6607
int TC_0_calib = -10;	// Calibration compensation value in digital counts (.25˚C)

char OLED_Address = 0x3c;
char OLED_Command_Mode = 0x80;
char OLED_Data_Mode = 0x40;

char h1[10];	// humidity string
char t1[10];	// temperature string
char t2[10];	// temperature string

char data[128];

char graphData[200];

#define MAXTIMINGS 85
#define cli noInterrupts
#define sei interrupts
#define NAN 999999

#define LEDPIN D7

// the Button
const int buttonPin1 = 3;
ClickButton button1(buttonPin1, LOW, CLICKBTN_PULLUP);
// Button results
int function = 0;


#define DHTPIN D4
#define DHTTYPE DHT22
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

// This wrapper is in charge of calling
// must be defined like this for the lib work
void dht_wrapper() {
		DHT.isrCallback();
}

float temp; // thermocouple

float h;		// humidity
float t;		// temperature
int f = 0;	// failed?

// delay related
unsigned long lastTime = 0UL;
unsigned long buttonDelay = 0UL;
unsigned long publishDelay = 0UL;
unsigned long logDelay = 0UL;

// history
// create a queue of numbers.
QueueArray <int> queue;
QueueArray <int> graph;

void setup()
{

	//attachInterrupt(WKP, shutdown, RISING); //declare the interrupt rising so button pulls to vcc
	//pinMode(WKP,INPUT_PULLUP);
	// button and a 10k pull-down resistor on WKP

		pinMode(LEDPIN, OUTPUT);
		Particle.function("status", Status);
		Particle.function("Switch", Switch);

		Particle.function("getData", getGraphData);
		Particle.variable("history", graphData, STRING);

//begin Wire communication with OLED
Wire.begin();

 // pinMode(DHTPIN, INPUT_PULLUP); // not used as we have an external pullup

 pinMode(SO, INPUT);
 pinMode(SCK, OUTPUT);

 pinMode(TC_0, OUTPUT);
 digitalWrite(TC_0,HIGH);	// Disable device

//set the one external function to call to update the OLED with a message
Particle.function("update", update);


		Particle.variable("humidity", h1, STRING);
		Particle.variable("temperature", t1, STRING);

		Particle.variable("thermocouple", t2, STRING);

		Particle.variable("data", data, STRING);
//initialize screen
SetupScreen();

sendCommand(0x01); // ** Clear display
sendMessage("started");

delay(1000);//wait one sec to see the started message

sendCommand(0x01); // ** Clear display

	// Setup button timers (all in milliseconds / ms)
	// (These are default if not set, but changeable for convenience)
	button1.debounceTime	 = 20;	 // Debounce timer in ms
	button1.multiclickTime = 250;	// Time limit for multi clicks
	button1.longClickTime	= 1000; // time until "held-down clicks" register

}

void loop()
{

	unsigned long now	= millis();

		if (now-buttonDelay>5UL) {
				buttonDelay = now;

				// Update button state
				button1.Update();

				// Save click codes in LEDfunction, as click codes are reset at next Update()
				if (button1.clicks != 0) function = button1.clicks;

				if(function == 1) {
						digitalWrite(LEDPIN, HIGH);
						Particle.publish("switch", String('1'), 60, PRIVATE); }
				if(function < 0 || function > 1) {
						digitalWrite(LEDPIN, LOW);
						Particle.publish("switch", String('0'), 60, PRIVATE); }

				/* if(function == 0) sendMessage("0"); // "no click"

				if(function == 1) sendMessage("1"); // "SINGLE click"

				if(function == 2) sendMessage("2"); // "DOUBLE click"

				if(function == 3) sendMessage("3"); // "TRIPLE click"

				if(function == -1) sendMessage("111"); // "SINGLE LONG click"

				if(function == -2) sendMessage("222"); // "DOUBLE LONG click" */

				if(function == -3) System.reset(); // "TRIPLE LONG click"

				function = 0;
		}


		// Update data and display and publish every 2.5 sec
		// DHT refresh update 2 sec, minimum delay between reads 250 ms
		if (now-lastTime>2500UL) {
				lastTime = now;

				// Read the thermocouple temperature and print it to serial
				temp = (float)read_temp(TC_0,1,TC_0_calib,10) / 10;
				sprintf(t2, "%.1f", temp);

				int result = DHT.acquireAndWait();
				f = 0;
				h = DHT.readHumidity();
				t = DHT.readTemperature();

				if (t==NAN || h==NAN)
						f = 1;
				else
						f = 0;

					sendCommand(0x01); // ** Clear display
					sprintf(h1, "%.1f", h); // convert Float to String
					sprintf(t1, "%.1f", t);
					sendMessage("ROOM ");
					sendMessage(t1);
					sendData(0xDF);

					sendMessage(" ");

					sendMessage(h1);
					sendData(0x25);

					sendCommand(0xC0); // ** New Line

					sendMessage("KILN ");

					sendMessage(String(round(temp),0));
					sendData(0xDF);

					// format your data as JSON, don't forget to escape the double quotes
					sprintf(data, "{\"kiln\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"size\":%.1i}",temp ,t , h, graph.count());

					}

		// Publish data every 15 seconds
		if (now-publishDelay>15000UL) {
				publishDelay = now;
				Particle.publish("temperature", String(t1), 60, PRIVATE);
				Particle.publish("humidity",String(h1), 60, PRIVATE);
				Particle.publish("thermocouple", String(t2), 60, PRIVATE);
				if (graph.count() == 20){graph.pop();}
				graph.enqueue( round(temp) ); // round(temp*10)/10
				// if (graphData.length() == 200) {graphData = graphData.substring(14,18)}
				// strcpy(graphData, String(graphData) + " " + String(t1));
		}

		// log data every 10 minutes
		if (now-logDelay>600000UL) {
			logDelay = now;
			// keep 24 hours
			if (queue.count() == 144){queue.pop();}
			queue.enqueue( round(temp) ); // round(temp*10)/10
		}
}

int getGraphData(String args) {

	strcpy(graphData,graph.toString());
	return 1;

}

int Status(String args) {
		int status_code = 2;
		if (digitalRead(LEDPIN) == 0) {
				status_code = 0;
		}
		if (digitalRead(LEDPIN) == 1) {
				status_code = 1;
		}
		return status_code;
}

int Switch(String args)
{
	int status_code = 0;
	char id = args.charAt(0);

	if (args == "ON" || args == "on" || args == "1"){
		digitalWrite(LEDPIN, HIGH);
		status_code = 1;
	}

	if (args == "OFF" || args == "off" || args == "0"){
		digitalWrite(LEDPIN, LOW);
		status_code = 0;
	}
	return status_code;
}

int switchOn(String command) {
	pinMode(LEDPIN, OUTPUT);
	digitalWrite(LEDPIN, 1);
	return 1;}

int switchOff(String command) {
	pinMode(LEDPIN, OUTPUT);
	digitalWrite(LEDPIN, 0);
	return 0;}

/* Create a function read_temp that returns an unsigned int
	 with the temp from the specified pin (if multiple MAX6675).	The
	 function will return 9999 if the TC is open.

	 Usage: read_temp(int pin, int type, int error)
		 pin: the CS pin of the MAX6675
		 type: 0 for ˚F, 1 for ˚C
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
			digitalWrite(SCK,HIGH);	// Set Clock to HIGH
			value += digitalRead(SO) << i;	// Read data and add it to our variable
			digitalWrite(SCK,LOW);	// Set Clock to LOW
		}

		/* Read the TC Input inp to check for TC Errors */
		digitalWrite(SCK,HIGH); // Set Clock to HIGH
		error_tc = digitalRead(SO); // Read data
		digitalWrite(SCK,LOW);	// Set Clock to LOW

		digitalWrite(pin, HIGH); //Disable Device
	}

	value = value/samples;	// Divide the value by the number of samples to get the average

	/*
		 Keep in mind that the temp that was just read is on the digital scale
		 from 0˚C to 1023.75˚C at a resolution of 2^12.	We now need to convert
		 to an actual readable temperature (this drove me nuts until I figured
		 this out!).	Now multiply by 0.25.	I tried to avoid float math but
		 it is tough to do a good conversion to ˚F.	THe final value is converted
		 to an int and returned at x10 power.

	 */

	value = value + error;	// Insert the calibration error value

	if(type == 0) {	// Request temp in ˚F
		temp = ((value*0.25) * (9.0/5.0)) + 32.0;	// Convert value to ˚F (ensure proper floats!)
	} else if(type == 1) {	// Request temp in ˚C
		temp = (value*0.25);	// Multiply the value by 25 to get temp in ˚C
	}

	temp_out = temp*10;	// Send the float to an int (X10) for ease of printing.

	/* Output 9999 if there is a TC error, otherwise return 'temp' */
	if(error_tc != 0) { return 9999; } else { return temp_out; }
}


//*************************************//
// --- WIDE.HK---//
// - SSD131x PMOLED Controller -//
// - SCL, SDA, GND, VCC(3.3v 5v) --//
//*************************************//

/*
****************************************
This code is rewritten from the original code for the Arduino to control the I2C OLED from http://Wide.HK

The code below works to use the SparkCore microcontrller (available at http://Particle.io )
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

void shutdown() {
    System.sleep(SLEEP_MODE_DEEP, 99999); //sleep for ages :)
}
