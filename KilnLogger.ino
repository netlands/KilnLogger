// This #include statement was automatically added by the Particle IDE.
#include "PietteTech_DHT.h"

// This #include statement was automatically added by the Particle IDE.
#include "clickButton.h"

#include "QueueArray.h"

#include "PowerShield.h"
PowerShield batteryMonitor;

// #include "Wire.h"
#include "OLedI2C.h"
OLedI2C OLED;

/*
	Temperature Reading from a MAX6675
	Ryan McLaughlin <ryanjmclaughlin@gmail.com>
*/

#define SO A4		// MISO
#define SCK A3	 // Serial Clock
#define TC_0 A2	// CS Pin of MAX6607
int TC_0_calib = -9;	// -10? Calibration compensation value in digital counts (.25˚C)

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
//initialize screen
OLED.init();

batteryMonitor.reset();
batteryMonitor.quickStart();

 // pinMode(DHTPIN, INPUT_PULLUP); // not used as we have an external pullup

 pinMode(SO, INPUT);
 pinMode(SCK, OUTPUT);

 pinMode(TC_0, OUTPUT);
 digitalWrite(TC_0,HIGH);	// Disable device

//set the one external function to call to update the OLED with a message
Particle.function("update", updateDisplay);


		Particle.variable("humidity", h1, STRING);
		Particle.variable("temperature", t1, STRING);

		Particle.variable("thermocouple", t2, STRING);

		Particle.variable("data", data, STRING);

OLED.clearLcd(); // ** Clear display
OLED.sendString("started",0,0);

delay(1000);//wait one sec to see the started message

OLED.clearLcd(); // ** Clear display

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
						// digitalWrite(LEDPIN, HIGH);
						// Particle.publish("switch", String('1'), 60, PRIVATE);
						OLED.lcdOn();
						}
				if(function < 0 || function > 1) {
						// digitalWrite(LEDPIN, LOW);
						// Particle.publish("switch", String('0'), 60, PRIVATE);
						OLED.lcdOff();
						System.sleep(LEDPIN, CHANGE); }
				/* if(function == 0) sendMessage("0"); // "no click"

				if(function == 1) sendMessage("1"); // "SINGLE click"

				if(function == 2) sendMessage("2"); // "DOUBLE click"

				if(function == 3) sendMessage("3"); // "TRIPLE click"

				if(function == -1) sendMessage("111"); // "SINGLE LONG click"

				if(function == -2) sendMessage("222"); // "DOUBLE LONG click"

				if(function == -3) System.reset(); // "TRIPLE LONG click" */

				function = 0;
		}


		// Update data and display and publish every 2.5 sec
		// DHT refresh update 2 sec, minimum delay between reads 250 ms
		if (now-lastTime>2500UL) {
				lastTime = now;

				// monitor battery
				float cellVoltage = batteryMonitor.getVCell();
				float stateOfCharge = batteryMonitor.getSoC();

				// Read the thermocouple temperature and print it to serial
				temp = (float)read_temp(TC_0,1,TC_0_calib,10) / 10;
				sprintf(t2, "%.0f", temp);

				int result = DHT.acquireAndWait();
				f = 0;
				h = DHT.readHumidity();
				t = DHT.readTemperature();

				if (t==NAN || h==NAN)
						f = 1;
				else
						f = 0;

					OLED.sendCommand(0x01); // ** Clear display
					sprintf(h1, "%.1f", h); // convert Float to String
					sprintf(t1, "%.1f", t);
					OLED.sendMessage("ROOM ");
					OLED.sendMessage(t1);
					OLED.sendData(0xDF);

					OLED.sendMessage(" ");

					OLED.sendMessage(h1);
					OLED.sendData(0x25);

					OLED.sendCommand(0xC0); // ** New Line

					OLED.sendMessage("TEMP ");

					OLED.sendMessage(t2); //String(round(temp),0));
					OLED.sendData(0xDF);

					OLED.sendMessage(" ");
					char v1[10];
					sprintf(v1, "%.1f", stateOfCharge);
					OLED.sendMessage(v1);
					OLED.sendMessage("%");

					// format your data as JSON, don't forget to escape the double quotes
					sprintf(data, "{\"sensor\":%.1f,\"temperature\":%.1f,\"humidity\":%.1f,\"size\":%.1i}",temp ,t , h, graph.count());

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



int updateDisplay(String args)
{ /*
int status_code = 0;

OLED.sendCommand(0x01); // ** Clear display

args.replace("%20"," ");

int commaPosition = args.indexOf(",");//find if there is a delim character

if(commaPosition>-1){
//two lines
OLED.sendMessage(args.substring(0,commaPosition));//send first part
OLED.sendCommand(0xC0); // ** New Line
OLED.sendMessage(args.substring(commaPosition+1, args.length()));//send second part

status_code = 2;//lines

}else{
//one line
OLED.sendMessage(args);

status_code = 1;//lines
}

//how many lines sent to display
return status_code; */
}



void shutdown() {
    System.sleep(SLEEP_MODE_DEEP, 99999); //sleep for ages :)
}
