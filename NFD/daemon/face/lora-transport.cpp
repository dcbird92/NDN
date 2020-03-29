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

    void (LoRaTransport::*transmit_and_recieve)();

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
  NFD_LOG_FACE_TRACE(__func__);

  // Set the flag high that we have a packet to transmit, and grab the data to send
  pthread_mutex_lock(&threadLock);
  store_packet = &packet;
  toSend = true;
  pthread_mutex_unlock(&threadLock);
  NFD_LOG_ERROR(__func__);
  NFD_LOG_ERROR("2");
}

void LoRaTransport::sendPacket(const ndn::Block &block) {
  ndn::EncodingBuffer buffer(block);

  if (block.size() <= 0) {
    NFD_LOG_FACE_ERROR("Trying to send a packet with no size");
  }

  // copy the buffer into a cstr so we can send it
  char *cstr = new char[buffer.size() + 1];
  uint8_t *buff = buffer.buf();
  for (size_t i = 0; i < buffer.size(); i++) {
    cstr[i] = buff[i];
  }
  if ((e = sx1272.sendPacketTimeout(0, cstr)) != 0) {
      handleError("Send operation failed: " + std::to_string(e));
  }  
  else
  {
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_FACE_TRACE("Successfully sent: " << block.size() << " bytes");
    NFD_LOG_FACE_TRACE("Successfully sent message: " << std::string(cstr));
  }

  // After sending enter recieve mode again
  sx1272.receive();
  delete[] cstr;
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
          NFD_LOG_ERROR("toSend is true");
          ndn::EncodingBuffer buffer(*store_packet);
          NFD_LOG_ERROR("toSend after allocate bufferr");
          if (buffer.size() <= 0) {

            NFD_LOG_ERROR("Trying to send a packet with no size");
          }

          // copy the buffer into a cstr so we can send it
          char *cstr = new char[buffer.size() + 1];
          uint8_t *buff = buffer.buf();
          for (size_t i = 0; i < buffer.size(); i++) {
            cstr[i] = buff[i];
          }
          if ((nfd::face::LoRaTransport::e = sx1272.sendPacketTimeout(0, cstr)) != 0) {
              NFD_LOG_ERROR("Send operation failed: " + std::to_string(e));
          }  
          else
          {
            // print block size because we don't want to count the padding in buffer
            NFD_LOG_INFO("Successfully sent: " << buffer.size() << " bytes");
            auto sentStuff = std::string();
            for(int idx = 0; idx < buffer.size(); idx++)
            {
              sentStuff += to_string((int)cstr[idx]);
            }

            NFD_LOG_FACE_INFO("Successfully sent message: " << std::string(cstr));
            toSend = false;
          }

          // After sending enter recieve mode again
          sx1272.receive();
          delete[] cstr;
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
    size_t i;
    while (dataToConsume) {
      e = sx1272.getPacket();
      if (e == 0) {
        NFD_LOG_ERROR("Data available to receive");
        uint8_t packetLength = sx1272.getCurrentPacketLength();
        for (i = 0; i < packetLength; i++)
        {
            my_packet[i] = (char)sx1272.packet_received.data[i];
        }

        // Reset null terminator
        my_packet[i] = '\0';

        NFD_LOG_ERROR("Received packet: ");
        NFD_LOG_ERROR(my_packet);
      }
      else {
        NFD_LOG_ERROR("Unable to get packet data: " + std::to_string(e));
      }
      dataToConsume = sx1272.checkForData();
    }

    ndn::Block element = ndn::Block((uint8_t*)my_packet, i);

    NDN_LOG_ERROR("i:" + std::to_string(i) + "\n");
    NDN_LOG_ERROR("packet:");
    NDN_LOG_ERROR(my_packet);
    NDN_LOG_ERROR("\n");

    for (int j = 0; j < i; j++) {
      NDN_LOG_ERROR(my_packet[j]);
    }

    this->receive(element);
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