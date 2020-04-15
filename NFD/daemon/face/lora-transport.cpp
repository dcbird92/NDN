/**
 * File used for implementing the transport layer of the LoRa Face
 */

#include "lora-transport.hpp"
namespace nfd {

namespace face {

NFD_LOG_INIT(LoRaTransport);

LoRaTransport::LoRaTransport() {

    // Set all of the static variables associated with this transmission (just need to set MTU)
    this->setMtu(160);


    // Read in a certain topology if flag is high
    if (readTopology) {
        std::ifstream infile(topologyFilename); 
        std::string token;
        while (std::getline(infile, token))
        {
          // Grab the ID field
          if (token.substr(0,2) == "id") {
            id = token[3] - '0';
            int err = sx1272.setNodeAddress(id);  // Set the ID so src in packets can be set
            if (err != 0) {
              NFD_LOG_ERROR("Unable to set nodeAddress! Fatal, restart");
            }
          }
          // Grab the send field
          if (token.substr(0,4) == "send") {
            send = token[5] - '0';
          }
          // Grab the recv field
          if (token.substr(0,4) == "recv") {
            recv = token[5] - '0';
          }
        }
        NFD_LOG_ERROR("Read in topology");
    }

    // Create the neccessary thread to begin receving and transmitting
    pthread_t receive;
    int rc;

    rc = pthread_create(&receive, NULL, &LoRaTransport::transmit_and_receive_helper, this);
    if(rc) {
      NFD_LOG_ERROR("Unable to create initial thread to create receive and transmitting thread: " + std::to_string(rc));
    }
}

void LoRaTransport::doClose() {
  // Close this form of transmission by turning off the chip
  sx1272.OFF();

  this->setState(TransportState::FAILED);
}

void LoRaTransport::doSend(const Block &packet, const EndpointId& endpoint) {
  // Set the flag high that we have a packet to transmit, and push the new data onto the queue
  pthread_mutex_lock(&threadLock);
  sendBufferQueue.push(new ndn::EncodingBuffer(packet));
  toSend = true;
  NFD_LOG_INFO("\n\ndoSend: added item to queue, set toSend true");
  pthread_mutex_unlock(&threadLock);

  NFD_LOG_TRACE(__func__);
}

void LoRaTransport::sendPacket()
{
  int bufSize = sendBuffer->size();
  if (bufSize <= 0)
  {
    NFD_LOG_ERROR("Trying to send a packet with no size");
    return;
  }

  NFD_LOG_INFO("Packet size to be sent: " << bufSize);

  // copy the buffer into a cstr so we can send it
  char * cstr = new char[bufSize];
  int i = 0;
  for (auto ptr : *sendBuffer)
  {
    cstr[i++] = ptr;
    if(ptr == '\0')
    {
      // NFD_LOG_ERROR("Found null in send packet at idx: " << i);
    }
  }

  if (i != bufSize)
    NFD_LOG_ERROR("Sizes different. i: " << i << " bufSize: " << bufSize);

  auto sentStuff = std::string();
  for(int idx = 0; idx < bufSize; idx++)
  {
    sentStuff += to_string((int)cstr[idx]) + ", ";
  }
  NFD_LOG_INFO("Message that is to be sent: " << sentStuff);

  if ((nfd::face::LoRaTransport::e = sx1272.sendPacketTimeout(send, cstr, bufSize)) != 0)
  {
    NFD_LOG_ERROR("Send operation failed: " + std::to_string(e));
  }
  else
  {
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_INFO("Supposedly sent: " << bufSize << " bytes");
    NFD_LOG_INFO("LoRa actually sent: " << sx1272._payloadlength << " _payloadlength bytes");

  }

  // Have to free all of this stuff
  delete[] cstr;
  delete sendBuffer; 
}

/*
* Function used for switching from receiving --> transmitting --> receiving for the LoRa
*/
void *LoRaTransport::transmit_and_recieve()
{
  NFD_LOG_INFO("Starting Lo-Ra thread");
  while(true){
      pthread_mutex_lock(&threadLock);
      // Check and see if there is something to send
      if (toSend) {
        while(toSend) {
          NFD_LOG_INFO("toSend is true");
          sendBuffer = sendBufferQueue.front();
          sendBufferQueue.pop();
          sendPacket();
          toSend = sendBufferQueue.empty() == false;
        }

        // After sending enter recieve mode again
        sx1272.receive();
        pthread_mutex_unlock(&threadLock);
      }
      // Otherwise check and see if there is available data
      else {

          // No need to keep the lock here...
          pthread_mutex_unlock(&threadLock);

          // Check to see if the LoRa has received data... if so handle it
          if (sx1272.checkForData()) {
            handleRead();
          }
      }
  }
}

void LoRaTransport::handleRead() {
  
  bool dataToConsume = true;
  bool packetCreated = false;
  int i;

  while (dataToConsume) {
    e = sx1272.getPacket();
    if (e == 0) {

      // If we are using a certain network topology, make sure the dest and source is accepted
      if (readTopology && sx1272.packet_received.dst != id && sx1272.packet_received.src != recv) {
        // Bad packet, try to read a different one
        continue;
      }

      NFD_LOG_INFO("\n\nData available to receive");
      int packetLength = (int)sx1272.getCurrentPacketLength();

      for (i = 0; i < packetLength; i++)
      {
          my_packet[i] = (char)sx1272.packet_received.data[i];
      }

      // Reset null terminator
      my_packet[i] = '\0';
      packetCreated = true;
      packetLength = i;
      NFD_LOG_INFO("Received packet:" << my_packet);
      NFD_LOG_INFO("With length:" << packetLength);
    }
    else {
      NFD_LOG_ERROR("Unable to get packet data: " + std::to_string(e));
      return;
    }
    dataToConsume = sx1272.checkForData();
  }

  // If no viable packet was received, just exit
  if (!packetCreated){
    return;
  }

  NFD_LOG_INFO("i: " + std::to_string(i));
  NFD_LOG_INFO("Full packet:" << my_packet);
  auto gotStuff = std::string();
  for(int idx = 0; idx < i; idx++)
  {
    gotStuff += to_string((int)my_packet[idx]);
  }
  NFD_LOG_INFO("Packet ascii:" << gotStuff);

  try
  {
    ndn::Block element = ndn::Block((uint8_t*)my_packet, i);
    this->receive(element);
    NFD_LOG_INFO("Created block succesfully and called receive");
  }
  catch(const std::exception& e)
  {
    NFD_LOG_ERROR("Block create exception: " << e.what());
  }
} 


void LoRaTransport::handleError(const std::string &errorMessage) {
  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_ERROR("Permanent face ignores error: " << errorMessage);
    return;
  }

  NFD_LOG_FACE_ERROR(errorMessage);
  this->setState(TransportState::FAILED);
  doClose();
}

}

}
