/*********************************************************************
 *               DMX master example for Versa1.0
 *	Output DMX frames to AUXSERIAL_TX  (K11 for Versa1.0)
 *********************************************************************/
#define BOARD Versa2
#include <fruit.h>
#include <dmx.h>

char autoON = 1;
/*int offset;
int mod1;
int mod2;*/
int modM;

typedef struct {
	unsigned char order;
	long current;
} t_filter;

typedef struct {
	int current;
	int offset;
	int mod1;
	int mod2;
	t_filter m1a, m1b;
	t_filter m2a, m2b;
} t_channel;

typedef struct {
	int current;
	t_filter m1a, m1b;
} t_motor;

t_channel channels[6];
t_motor motors[6];

t_delay mainDelay;

/*void setModf1(int modf1)
{
	unsigned char i;
	for(i = 0; i < 6 ; i++) {
		channels[i].m1a.order = channels[i].m1b.order = modf1;
	}
}

void setModf2(int modf2)
{
	unsigned char i;
	for(i = 0; i < 6 ; i++) {
		channels[i].m2a.order = modf2;
		channels[i].m2b.order = modf2 + 1;
	}
}*/

void setModfM(int modf)
{
	unsigned char i;
	for(i = 0; i < 6 ; i++) {
		motors[i].m1a.order = modf;
		motors[i].m1b.order = modf + 1;
	}
}

void setChannelParams(unsigned char chan, int offset, int mod1, unsigned char modf1, int mod2, unsigned char modf2)
{
	channels[chan].offset = offset;
	channels[chan].mod1 = mod1;
	channels[chan].m1a.order = channels[chan].m1b.order = modf1;
	channels[chan].mod2 = mod2;
	channels[chan].m2a.order = modf2;
	channels[chan].m2b.order = modf2 + 1;
}

#define putint(c) do{putchar(c >> 8); putchar(c & 255);} while(0)
void sendChannelParams(unsigned char chan, unsigned char prefix)
{
	putchar('B');
	putchar(prefix);
	putchar(chan);
	putint(channels[chan].offset);
	putint(channels[chan].mod1);
	putchar(channels[chan].m1a.order);
	putint(channels[chan].mod2);
	putchar(channels[chan].m2a.order);
	putchar('\n');
}

void channelEE(unsigned char chan)
{
	EEdeclareInt(&channels[chan].offset);
	EEdeclareInt(&channels[chan].mod1);
	EEdeclareChar(&channels[chan].m1a.order);
	EEdeclareInt(&channels[chan].mod2);
	EEdeclareChar(&channels[chan].m2a.order);
	
	channels[chan].m1b.order = channels[chan].m1a.order;
	channels[chan].m2b.order = channels[chan].m2a.order + 1;
}
	
//----------- Setup ----------------
void setup(void) {	
	fruitInit();			
	pinModeDigitalOut(LED); 	// set the LED pin mode to digital out
	digitalClear(LED);		// clear the LED
	DMXInit();        // init DMX master module
	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms
	channels[0].current = rand();
	
	/*offset = 630;
	mod1 = 3500;
	mod2 = 500;*/
	/*setModf1(10);
	setModf2(7);*/
	modM = 2000;
	setModfM(10);
	EEreadMain();
}

#define filter_process(filter, input) do{\
		(filter)->current = (filter)->current - ((filter)->current >> (filter)->order) + input;\
	} while(0)

#define filter_get(filter) ((filter)->current >> (filter)->order)

void updateChannel(char chan)
{
	int out;
	
	filter_process(&channels[chan].m1a, (rand() & 0xffff) - 0x7fff);
	filter_process(&channels[chan].m1b, filter_get(&channels[chan].m1a));
	
	filter_process(&channels[chan].m2a, (rand() & 0xffff) - 0x7fff);
	filter_process(&channels[chan].m2b, filter_get(&channels[chan].m2a));
	
	channels[chan].current = 
		(((long)filter_get(&channels[chan].m1b) * channels[chan].mod1) >> 13) +
		(((long)filter_get(&channels[chan].m2b) * channels[chan].mod2) >> 13) + 
		channels[chan].offset;

	out = channels[chan].current >> 2;
	if(out < 0) out = 0;
	if(out > 255) out = 255;
	DMXSet(30 + chan, out);
}

void updateMotor(char chan)
{
	int out;
	
	filter_process(&motors[chan].m1a, (rand() & 0xffff) - 0x7fff);
	filter_process(&motors[chan].m1b, filter_get(&motors[chan].m1a));
	
	
	motors[chan].current = (((long)filter_get(&motors[chan].m1b) * modM) >> 13) + 512;

	out = motors[chan].current >> 2;
	if(out < 1) out = 1;
	if(out > 255) out = 255;
	DMXSet(24 + chan, out);
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

		if(autoON) {
			for(i = 0; i < 6 ; i++) {
				updateChannel(i);
				DMXService();
			}
			for(i = 0; i < 6 ; i++) {
				updateMotor(i);
				DMXService();
			}
		}
		
		if(loops++ > 5) {
			loops = 0;
			if(digitalRead(LED)) digitalClear(LED);
			else digitalSet(LED);
			putchar('B');
			putchar(30);
			for(i = 0; i < 6 ; i++){
				tmp = channels[i].current;
				putchar(tmp >> 8);
				putchar(tmp & 255);
			}
			DMXService();
			for(i = 0; i < 6 ; i++){
				tmp = motors[i].current;
				putchar(tmp >> 8);
				putchar(tmp & 255);
			}
			DMXService();
			putchar(DMXframeCount >> 8);
			putchar(DMXframeCount & 255);
			putchar('\n');
		}
	}
}

// ---------- Receiving ------------
void fraiseReceive() // receive raw bytes
{
	int i;
	unsigned char c=fraiseGetChar(); // get first byte
	switch(c) {
		PARAM_INT(30,i); DMXSet(i, fraiseGetChar()); break; // if first byte is 30 then get DMX channel (int) and value (char).
		PARAM_CHAR(31,autoON); break; 
		/*PARAM_INT(32,offset); break; 
		PARAM_INT(33,mod1); break; 
		PARAM_INT(34,i); setModf1(i); break; 
		PARAM_INT(35,mod2); break; 
		PARAM_INT(36,i); setModf2(i); break; */
		PARAM_INT(37,modM); break; 
		PARAM_INT(38,i); setModfM(i); break;
		case 100: setChannelParams(
			fraiseGetChar(), // channel
			fraiseGetInt(), // offset
			fraiseGetInt(), fraiseGetChar(), // mod1 modf1
			fraiseGetInt(), fraiseGetChar()); // mod2 modf2
			break;
		case 101: sendChannelParams(fraiseGetChar(), 101); break;
		case 200: EEwriteMain(); break;		
	}
}

void EEdeclareMain()
{
	unsigned char i;
	for(i = 0; i < 6 ; i++) { channelEE(i);}
}
