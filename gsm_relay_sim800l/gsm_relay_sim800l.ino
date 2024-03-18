/*
 * File: gsm-relay-800l.ino
 * Author: Michal Sviba
 * Date created: 1.2.2024
 * 
 * Description: A realy controlled by GSM RING from previously
                authorizated caller ID. Powered by Arduino Nano
 * 
 * Copyright (c) 2024 Michal Sviba
 * 
 * This file is part of GSM relay 800L.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

 
#include <SoftwareSerial.h>

#define _NOT_RELEASE_ 

SoftwareSerial GSMSerial(6, 5);

const String C_SECRET = "error completed";
const int ledPin = PIN2;  // the number of the LED pin
const int relayPin = PIN3;  

String inputString = "";      // a String to hold incoming data
int ringing = 0;
bool powerOn = false;

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  GSMSerial.begin(19200);               // the GPRS/GSM baud rate   
  #ifndef _RELEASE_
  Serial.begin(19200);                 // the GPRS/GSM baud rate   
  #endif
  delay(500);

  GSMSerial.print("\n");
  delay(1500);

  // extended info; extended error messages; extended error messages; sim detection mode ON; write out SIM identification; sms text mode; delete all sms
  GSMSerial.print("ATI;+GSV;+CMEE=2;+CSDT=1;+CCID;+CLIP=1;+CMGF=1\n"); // +CMGDA=\"DEL ALL\"

  //delay(500);
  //GSMSerial.print("AT+CRSM=?\n"); // Restricted SIM Access

  digitalWrite(ledPin, LOW);


}

void loop()
{
#ifndef _RELEASE_  
  if(Serial.available()) {
    GSMSerial.print((char)Serial.read());
  } else 
#endif
    while (GSMSerial.available()) {
      char inChar = (char)GSMSerial.read();
      // add it to the inputString:
      inputString += inChar;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == '\n') {
            if (inputString.indexOf("+CMGR:") == 0) { // this message is multiline
              // continue to read until OK
              if (inputString.indexOf("OK", inputString.length()-5) > 0) {
                parseSim900message(inputString);
                inputString = "";
              }
            } else {
              parseSim900message(inputString);
              inputString = "";
            }
      }
    }
  
}

void parseSim900message(String msg){
#ifndef _RELEASE_
  Serial.print(msg);
#endif
  if (msg.startsWith("RING")) {
    ringing++;
    blink(2);
    
  } else if (msg.indexOf("+CMTI") != -1) {
      int indexStart = msg.indexOf(",") + 1;
      int indexEnd = msg.indexOf("\r\n");
      int smsIndex = msg.substring(indexStart, indexEnd).toInt();

      if (smsIndex > 0) {
        GSMSerial.print("AT+CMGR=" + String(smsIndex)+"\n");
      }
    
  } else if (msg.indexOf("+CMGR:") != -1) {
    // parse out sms content
    int cidStart = msg.indexOf(",") + 2;
    int cidEnd = msg.indexOf(",", cidStart + 1)-1;
    String cidSender = msg.substring(cidStart, cidEnd);

    // parse phone number
    int contentStart = msg.indexOf("\n") + 1;
    int contentEnd = msg.indexOf("\n", contentStart + 1);
    String smsContent = msg.substring(contentStart, contentEnd);

    smsContent.trim();

    #ifndef _RELEASE_
    Serial.println("Received SMS content: " + smsContent);
    #endif

    if (smsContent.indexOf(C_SECRET) != -1) {
          // secret founded, save sender ID to the phone book as "The Master"
          #ifndef _RELEASE_
          Serial.println("Save number to PB: " + cidSender);
          #endif
          GSMSerial.print("AT+CPBW=1,\""+cidSender+"\",145,\"The Master\"\n");
    }

  } else if (msg.indexOf("NO CARRIER") != -1) {
    ringing = 0;
    blink(7);
  } else if (msg.startsWith("+CLIP:") && msg.indexOf("The Master") != -1) {
    // if The Master calling
    if 
         (!powerOn && (ringing > 4) // more then 4x times, then switch ON and hang
         ||powerOn && (ringing > 2) // and then more than 2times switch OFF and hang
         ){
      powerOn = !powerOn;
      digitalWrite(relayPin, powerOn);
      blink(2);
      GSMSerial.print("ATH\r");
      ringing = 0;
    }
  }
}


void blink(int count){
  while (count > 0) {
    digitalWrite(ledPin, HIGH);
    delay(20);
    digitalWrite(ledPin, LOW);
    delay(30);
    count--;
  }
}

