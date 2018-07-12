/*
 * Project CarLogger
 * Description:
 * Author:
 * Date:
 */

#if PLATFORM_ID == 10 // Electron
  #define SEC_CAN
#elif PLATFORM_ID == 6 // Photon
#else
  #error "This library only works on the Electron and Photon"
#endif

#include "SdFat.h"

SYSTEM_THREAD(ENABLED);

// Primary SPI with DMA
// SCK => A3, MISO => A4, MOSI => A5, SS => A2 (default)
SdFat sd;
const uint8_t chipSelect = SS;

File logFile;
uint8_t logSyncCnt;
#define LOGSYNC 32

CANChannel can1(CAN_D1_D2);
#ifdef SEC_CAN
CANChannel can2(CAN_C4_C5);
#endif

void logMessage(int ix, const CANMessage &message);

void reset_handler()
{
  can1.end();
  #ifdef SEC_CAN
  can2.end();
  #endif
  logFile.close();
}

// setup() runs once, when the device is first turned on.
void setup()
{
  // Put initialization like pinMode and begin functions here.
  Serial.begin(1000000);
  pinMode(D7, OUTPUT);

  logSyncCnt = 0;
  if(sd.begin(chipSelect, SPI_FULL_SPEED)) {
    for(int i = 0; i < 1000; i++) {
      String fname = String::format("can%03d.log", i);
      if(logFile.open(fname,  O_CREAT | O_EXCL | O_WRONLY)) {
        break;
      }
    }
  }
  if(logFile.isOpen()) {
    logFile.printf("# Log file generated by CanLogger\n");
    logFile.sync();
  }

  can1.begin(500000);
  #ifdef SEC_CAN
  can2.begin(125000);
  #endif

  // register the reset handler
  System.on(reset, reset_handler);
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  // The core of your code will likely live here.
  if(can1.isEnabled()) {
    CANMessage message1;
    if(can1.receive(message1)) {
      logMessage(0, message1);
      logSyncCnt++;
      digitalWrite(D7, LOW);
    }
  }
  #ifdef SEC_CAN
  if(can2.isEnabled()) {
    CANMessage message2;
    if(can2.receive(message2)) {
      logMessage(1, message2);
      logSyncCnt++;
      digitalWrite(D7, LOW);
    }
  }
  #endif
  if(logSyncCnt >= LOGSYNC) {
    logSyncCnt = 0;
    digitalWrite(D7, HIGH);
    logFile.sync();
  }
}

void logMessage(int ix, const CANMessage &message)
{
  unsigned long time;
  time = millis();
  logFile.printf("(%d.%03d000) can%d %03x#", (time / 1000), (time % 1000), ix, message.id);
  for(auto i = 0; i < message.len; i++) {
    logFile.printf("%02x", message.data[i]);
  }
  logFile.printf("\n");
}