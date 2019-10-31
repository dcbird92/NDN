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
#include <queue>
using namespace std;
int e;
string m;
char my_packet[256];
bool toSend;


// Creating mutexes for shared queue and conditions for when data is produced from console
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER; 
  
pthread_cond_t dataSent =  
                    PTHREAD_COND_INITIALIZER; 

// Shared queue 
queue<string> Q; 

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

  // Set the LoRa into receive mode by default
  e = sx1272.receive();
  if (e)
    cout << "Unable to enter receive mode" << endl;
  
  // Print a success message
  printf("SX1272 successfully configured\n\n");
  delay(1000);
}

/*
* Function used for continuously getting data from the console from the user to send from one pi to another
*/
void *get_data_from_console(void *threadid)
{
    while (1) { 

        // Wait for data input!
        std::string packet;
        getline(std::cin, packet);
        
        // Getting the lock on queue using mutex 
        pthread_mutex_lock(&threadLock); 
        
        // Packet recieved from user
        if (!packet.empty()) {        
            Q.push(packet);
            toSend = true;
        }
    
        // Get the mutex unlocked 
        pthread_mutex_unlock(&threadLock); 
    }
    
    cout << "Exiting..." << Q.size() << endl;
}

/*
* Function used for switching from receiving --> transmitting --> receiving for the LoRa
*/
void *transmit_and_recieve(void *threadid)
{
  cout << "Creating transmit and receive thread\n";
  while(1){

      // Check and see if there is something to send
      if (toSend) {

          pthread_mutex_lock(&threadLock);

          // Attempt to enter transmit mode
          string packetToSend = Q.front();
          Q.pop();
          char *cstr = new char[packetToSend.length() + 1];
          strcpy(cstr, packetToSend.c_str());
          if ((e = sx1272.sendPacketTimeout(0, cstr)) != 0) {
              cout << "Error sending packet" << e << endl;
          }

          // Reset flag after sending if there is nothing else to send
          toSend = Q.empty() == false;

          // After sending, unlock the queue and enter recieve mode again
          sx1272.receive();
          pthread_mutex_unlock(&threadLock);
      }
      // Otherwise check and see if there is available data
      else {
          if (sx1272.checkForData()) {
            bool dataToConsume = true;
            while (dataToConsume) {
              e = sx1272.getPacket();
              if (e == 0) {
                    uint8_t packetLength = sx1272.getCurrentPacketLength();
                    unsigned int i;
                    for (i = 0; i < packetLength; i++)
                    {
                        my_packet[i] = (char)sx1272.packet_received.data[i];
                    }

                    // printf("i: %u and length: %u\n", i, packetLength);

                    // Reset null terminator
                    my_packet[i] = '\0';

                    printf("Message: %s\n", my_packet);
              }
              else {
                printf("Error, e: %d\n", e);
              }
              dataToConsume = sx1272.checkForData();
            }
          }
      }
 }
}

int main (){
  pthread_t receive, transmit;
  int rc;
  int tone;
  int ttwo;
  
  tone = 1;
  ttwo = 2; 

  setup();
  rc = pthread_create(&transmit, NULL, get_data_from_console, (void *)tone);
  if(rc) {
    cout<<"Error in getting input from console"; exit(-1);    
  }
  rc = pthread_create(&receive, NULL, transmit_and_recieve, NULL);
  if(rc) {
    cout<<"Error in transmitting and recieving"; exit(-1);
  }

  // Wait for the threads to join (user would have to ctrl-c)
   pthread_join(receive, NULL);
   pthread_join(transmit, NULL);
}