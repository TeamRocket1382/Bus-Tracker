/************************************************
 * This program is developed under examples of MCCI LORA library, NEO 6m GPS libraby.
 * Following examples are under the copyright with  following owners.
 * 
 * LMIC: Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *       Copyright (c) 2018 Terry Moore, MCCI
 * 
 * GPS: Rui Santos 
 * 
 * Description: This program is driver for Arduino Uno with NEO-6m GPS module, RFM95 Lora device, and rotary switch.
 * It receives the information from the gps module and rotary switch,  
 * then it schedules packet and send it to lora gateway with rate of 1 packet per 60 seconds.
 ************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//GPS pins for Transmit and Receive function.
static const int RXPin = 5, TXPin = 4;
//Initializes first position and last position of rotary pins.
const int firstRotaryPin = 6;
const int lastRotaryPin = 8;
TinyGPSPlus gps;
//initialize gps module.
SoftwareSerial ss(RXPin, TXPin);

//Initializes APPEUI, Device EUI and app key for OTAA based lora transmission.
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={ 0x08, 0xF5, 0x04, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x60, 0x9A, 0xCB, 0x84, 0x76, 0x00, 0x99, 0x40, 0xCA, 0x4B, 0x54, 0x5F, 0xCB, 0x80, 0xD8, 0x46 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//Placeholder for data to be transmitted.
char mydata[22];
static osjob_t sendjob;

//Transmit rate of the RFM95 module. 1 packet per 60 seconds.
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 3, LMIC_UNUSED_PIN},
};
//Switch cases for each event happening with connection between lora gateway and RFM95 transceiver.
void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINED:
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            break;
        case EV_TXCOMPLETE:
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
            default:
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    }
}

void setup() {
  for( int i=firstRotaryPin; i<= lastRotaryPin; i++) { 
    pinMode( i, INPUT); 
    digitalWrite( i, HIGH); // turn on internal pullup resistor
  }
    Serial.begin(9600);
    ss.begin(9600);
    os_init();
    LMIC_reset();
    do_send(&sendjob);
}

int getRotaryValue() {
  for( int i=firstRotaryPin; i<= lastRotaryPin; i++) { 
    int val = digitalRead(i); // look at a rotary switch input
    if( val == LOW ) { // it's selected
      return (i - firstRotaryPin + 1); // return a value that ranges from first pin to last rotary pins.
    }
  }
  return 0; // error case
}

//
void loop() {
  //Initialized connection between LORA gateway and RFM95.
  os_runloop_once();
  //Check if GPS module receives signal from the satellite.
  while (ss.available() > 0){
    gps.encode(ss.read());
    if(gps.location.isUpdated())
    {
  int busID = 833;
  float lat = gps.location.lat();
  float lng = gps.location.lng();
  char str_temp[16];
  char str_temp2[16];
  //Converts BUSID, switc position, latitude and longitude into char array and assign it to the packet to be transmitted.
  dtostrf(lat,4,4,str_temp);
  dtostrf(lng,4,4,str_temp2);
  sprintf(mydata, "%d,%s,%s,%d",busID,str_temp,str_temp2,getRotaryValue());
       
    }
  }
}
