/*  
 *  LoRa 868 / 915MHz SX1272 LoRa module
 *  
 *  Copyright (C) Libelium Comunicaciones Distribuidas S.L. 
 *  http://www.libelium.com 
 *  
 *  This program is free software: you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation, either version 3 of the License, or 
 *  (at your option) any later version. 
 *  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License 
 *  along with this program.  If not, see http://www.gnu.org/licenses/. 
 *  
 *  Version:           1.1
 *  Design:            David Gascón 
 *  Implementation:    Covadonga Albiñana & Victor Boria
 */
 
// Include the SX1272 and SPI library: 
#include "arduPiLoRa.h"
#include <string>
#include <iostream>
#include <pthread.h>
using namespace std;
int e;
string m;
char my_packet[100];
char *cstr;
bool toSend;

char message1 [] = "Packet 1, wanting to see if received packet is the same as sent packet";
char message2 [] = "Packet 2, broadcast test";

void setup()
{
  // Print a start message
  printf("SX1272 module and Raspberry Pi: send packets with ACK and retries\n");
  
  // Power ON the module
  e = sx1272.ON();
  printf("Setting power ON: state %d\n", e);
  
  // Set transmission mode
  e = sx1272.setMode(4);
  printf("Setting Mode: state %d\n", e);
  
  // Set header
  e = sx1272.setHeaderON();
  printf("Setting Header ON: state %d\n", e);
  
  // Select frequency channel
  e = sx1272.setChannel(CH_10_868);
  printf("Setting Channel: state %d\n", e);
  
  // Set CRC
  e = sx1272.setCRC_ON();
  printf("Setting CRC ON: state %d\n", e);
  
  // Select output power (Max, High or Low)
  e = sx1272.setPower('H');
  printf("Setting Power: state %d\n", e);
  
  // Set the node address
  e = sx1272.setNodeAddress(3);
  printf("Setting Node address: state %d\n", e);
  
  // Print a success message
  printf("SX1272 successfully configured\n\n");
  delay(1000);
}

void *transmit_func(void *threadid)
{
  long tid;
  tid = (long) threadid;
    cout << "Creating transmit thread\n";
  while(1)
  {
    getline(cin, m);
    cstr = &m[0];
    if( strlen(cstr) != 0)
    {
      printf("Message passed in and ready to transmit");
      toSend = true;
    }
  }
  pthread_exit(NULL);
}

void *receive_func(void *threadid)
{
  long tid;
  tid = (long) threadid;
  cout << "Creating receive thread\n";
  while(1){
  // Transmit message  
  if (toSend == 1)
  {
    e = sx1272.sendPacketTimeoutACKRetries(0, cstr);
    printf("Packet sent, state %d\n",e);
    if(e == 0)
    {
      toSend = 0;
      cstr = "";
      }
  }
  // Receive message
  else
  {
    e = sx1272.receivePacketTimeoutACK(10000);
    if ( e == 0 )
    {
      printf("Receive packet with ACK and retries, state %d\n",e);

      for (unsigned int i = 0; i < sx1272.packet_received.length; i++)
      {
        my_packet[i] = (char)sx1272.packet_received.data[i];
      }
      printf("Message: %s\n", my_packet);
    }
    else {
      printf("Receive packet with ACK and retries, state %d\n",e);
    }
  }
 }
 pthread_exit(NULL);
}

int main (){
  pthread_t receive, transmit;
  int rc;
  int i, j;
  i = 1;
  j = 2;
  setup();
  rc = pthread_create(&transmit, NULL, transmit_func, (void *)j);
  if(rc){cout<<"Error in transmit"; exit(-1);}
  rc = pthread_create(&receive, NULL, receive_func, (void *)i);
  if(rc){cout<<"Error in receive"; exit(-1);}

  while(1){}
	return (0);
}

        
