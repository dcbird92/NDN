/**
 * File used for implementing each of the functions
 */

#include "lora-transport.hpp"
namespace nfd {

namespace face {

NFD_LOG_INIT(LoRaTransport);

LoRaTransport::LoRaTransport() {

    // Set all of the static variables associated with this transmission 
    // ** COME BACK TO THIS? **
    // this->setLocalUri(FaceUri("lora"));

    // this->setRemoteUri(FaceUri("lora-remote"));

    // setScope(ndn::nfd::FaceScope scope);
    // setLinkType(linkType);
    this->setMtu(250);

    // setSendQueueCapacity(ssize_t sendQueueCapacity);
    // setState(TransportState newState);
    // setExpirationTime(const time::steady_clock::TimePoint& expirationTime);

    // Create the neccessary thread to begin receving and transmitting
    pthread_t receive;
    int rc;

    // void (LoRaTransport::*transmit_and_recieve)();

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
  NFD_LOG_FACE_TRACE("\n\n" << __func__);

  // Set the flag high that we have a packet to transmit, and grab the data to send
  pthread_mutex_lock(&threadLock);
  // store_packet = &packet;
  sendBuffer = new ndn::EncodingBuffer(packet);
  toSend = true;
  NFD_LOG_TRACE("2");
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

  // copy the buffer into a cstr so we can send it
  char * cstr = new char[bufSize];
  int i = 0;
  for (auto ptr : *sendBuffer)
  {
    cstr[i++] = ptr;
    if(ptr == NULL)
      NFD_LOG_ERROR("Found null in send packet at idx: " << i);
  }

  if (i != bufSize)
    NFD_LOG_ERROR("Sizes different. i: " << i << " bufSize: " << bufSize);

  auto sentStuff = std::string();
  for(int idx = 0; idx < bufSize; idx++)
  {
    sentStuff += to_string((int)cstr[idx]) + ", ";
  }
  NFD_LOG_INFO("Message that was to be sent: " << sentStuff);

  if ((nfd::face::LoRaTransport::e = sx1272.sendPacketTimeout(0, cstr, bufSize)) != 0)
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
          NFD_LOG_INFO("toSend is true");
          sendPacket();
          toSend = false;

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
  int i;

  while (dataToConsume) {
    e = sx1272.getPacket();
    sx1272.getPayloadLength();
    if (e == 0) {
      NFD_LOG_INFO("\n\nData available to receive");
      int packetLength = (int)sx1272.getCurrentPacketLength();
      for (i = 0; i < packetLength; i++)
      {
          my_packet[i] = (char)sx1272.packet_received.data[i];
      }

      // Reset null terminator
      my_packet[i] = '\0';
      packetLength = i;
      NFD_LOG_INFO("Received packet:" << my_packet);
      NFD_LOG_INFO("With length:" << packetLength);
    }
    else {
      NFD_LOG_ERROR("Unable to get packet data: " + std::to_string(e));
    }
    dataToConsume = sx1272.checkForData();
  }

  NFD_LOG_INFO("i:" + std::to_string(i));
  NFD_LOG_INFO("Full packet:" << my_packet);
  auto gotStuff = std::string();
  for(int idx = 0; idx < i; idx++)
  {
    gotStuff += to_string((int)my_packet[idx]);
  }
  NFD_LOG_INFO("Packet ascii:" << gotStuff);

  try
  {
    NFD_LOG_INFO("Creating block");
    ndn::Block element = ndn::Block((uint8_t*)my_packet, i);
    NFD_LOG_INFO("Created block I spose");
    this->receive(element);
    NFD_LOG_INFO("Called receive with blockz");
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