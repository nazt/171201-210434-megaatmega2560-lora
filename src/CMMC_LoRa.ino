#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "CMMC_Interval.hpp"
#include <Adafruit_Sensor.h> 
#include <NMEAGPS.h>
#include <GPSport.h>

#include <rn2xx3.h>

//create an instance of the rn2483 library, using the given Serial port
rn2xx3 myLora(Serial2);


static NMEAGPS  gps; // This parses the GPS characters

static int32_t gps_latitude = 0;
static int32_t gps_longitude = 0;
static int32_t gps_altitude_cm = 0;
static uint32_t gps_us;

static void doSomeWork( const gps_fix & fix );
static void doSomeWork( const gps_fix & fix )
{
  if (fix.valid.location) {
    gps_latitude = fix.latitudeL();
    gps_longitude = fix.longitudeL();
    gps_altitude_cm = fix.altitude_cm();
    gps_us = fix.dateTime_us();
  }
} // doSomeWork
static void GPSloop();
static void GPSloop()
{
  while (gps.available( gpsPort ))
    doSomeWork( gps.read() );
} // GPSloop

#define SEALEVELPRESSURE_HPA (1013.25)

String udpData = "";
CMMC_Interval interval;
#include <CMMC_RX_Parser.h>
#include "packet.h"
void array_to_string(byte array[], unsigned int len, char buffer[])
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}
CMMC_RX_Parser parser(&Serial3);

bool flag_dirty = false;
char bbb[400];

static CMMC_MASTER_PACKET_T master_packet;

static void msg_recv(u8 * packet, u8 len) {
  Serial.println("RECV PACKET..");
  Serial.println();
  Serial.print(F("packet size= "));
  Serial.println(len);

  Serial.println("MASTER_PACKET..");
  Serial.print("master_packet size= ");
  Serial.println(sizeof(master_packet));
  if (len == sizeof(master_packet.packet)) {
    memcpy(&(master_packet.packet), packet, len);

    Serial.println(String("project = ") + master_packet.packet.project);
    Serial.println(String("version = ") + master_packet.packet.version);
    Serial.println(String("field1 = ")  + master_packet.packet.data.field1);
    Serial.println(String("field2 = ")  + master_packet.packet.data.field2);
    Serial.println(String("battery = ") + master_packet.packet.data.battery);
    Serial.println(String("myName= ") + master_packet.packet.data.myName);

    flag_dirty = true;
  }
}

void setup()
{
  Serial.begin(57600);
  Serial.println("Initalizing LoRa module...");
  Serial3.begin(57600);
  gpsPort.begin(9600);
  Serial.println("BEGIN...");
  parser.on_command_arrived(&msg_recv);
}

String hexString;
void loop()
{
  parser.process();
  GPSloop();
    interval.every_ms(2L * 1000, []() { 
      Serial.println(millis());
      Serial.print(gps_latitude); Serial.print(","); Serial.println(gps_longitude);
    });
}
