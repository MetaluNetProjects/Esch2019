/*********************************************************************
 *               DMX master example for Versa1.0
 *	Output DMX frames to AUXSERIAL_TX  (K11 for Versa1.0)
 *********************************************************************/
#define BOARD Versa2
#include <fruit.h>
#include <dmx.h>

char autoON = 1;
int offset = 630;
float mod1 = 2.0;
float modf1 = 0.0012;
float mod2 = 0.05;
float modf2 = 0.01;

typedef struct {
	float current;
	float m1a, m1b;
	float m2a, m2b;
} t_channel;

t_channel channels[6];

t_delay mainDelay;

//----------- Setup ----------------
void setup(void) {	
	fruitInit();			
	pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	digitalClear(LED);		// clear the LED
	DMXInit();        // init DMX master module
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms
	channels[0].current = rand();
}

float filter(float in, float cur, float freq)
{
	return cur * (1 - freq) + in * freq;
}

void updateChannel(char chan)
{
	float norm;
	
	channels[chan].m1a = filter( (rand() & 0x1fff) - 0x1000, channels[chan].m1a, modf1);
	DMXService();
	channels[chan].m1b = filter( channels[chan].m1a, channels[chan].m1b, modf1);
	DMXService();
	
	channels[chan].m2a = filter( (rand() & 0x1fff) - 0x1000, channels[chan].m2a, modf2);
	DMXService();
	channels[chan].m2b = filter( channels[chan].m2a, channels[chan].m2b, modf2 * 2.0);
	DMXService();
	
	channels[chan].current = channels[chan].m1b * mod1 * 10.0 + channels[chan].m2b * mod2 * 10.0 + offset;
	DMXService();
	//channels[chan].current = ((rand() & 0x1fff) - 0x1000) * mod1 + offset;
	if(channels[chan].current < 0) channels[chan].current = 0;
	if(channels[chan].current > 1023) channels[chan].current = 1023;
	DMXService();
	norm = channels[chan].current / 1024;
	DMXService();
	norm = norm * 255.0;
	DMXService();
	DMXSet(30 + chan, norm);
}

// ---------- Main loop ------------
void loop() {
	static char loops;
	int i, tmp;
	fraiseService();// listen to Fraise events
	
	DMXService();	// DXM management routine
	
	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay

		if(autoON) for(i = 0; i < 6 ; i++) {
			updateChannel(i);
			DMXService();
		}

		if(loops++ > 20) {
			loops = 0;
			if(digitalRead(LED)) digitalClear(LED);
			else digitalSet(LED);
			putchar('B');
			putchar(30);
			for(i = 0; i < 6 ; i++){
				tmp = channels[i].current;
				putchar(tmp >> 8);
				putchar(tmp & 255);
				DMXService();
			}
			putchar('\n');
		}
	}
}

int tmpIntInput;
#define PARAM_FLOAT(n, f) PARAM_INT(n, tmpIntInput); f = tmpIntInput * 0.0001;
// ---------- Receiving ------------
void fraiseReceive() // receive raw bytes
{
	int i;
	unsigned char c=fraiseGetChar(); // get first byte
	switch(c) {
		PARAM_INT(30,i); DMXSet(i, fraiseGetChar()); break; // if first byte is 30 then get DMX channel (int) and value (char).
		PARAM_CHAR(31,autoON); break; 
		PARAM_INT(32,offset); break; 
		PARAM_FLOAT(33,mod1); break; 
		PARAM_FLOAT(34,modf1); break; 
		PARAM_FLOAT(35,mod2); break; 
		PARAM_FLOAT(36,modf2); break; 
	}
}

