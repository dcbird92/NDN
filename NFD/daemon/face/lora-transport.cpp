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
    // setLocalUri(const FaceUri& uri);
    // setRemoteUri(const FaceUri& uri);
    // setScope(ndn::nfd::FaceScope scope);
    // setLinkType(ndn::nfd::LinkType linkType);
    // setMtu(ssize_t mtu);
    // setSendQueueCapacity(ssize_t sendQueueCapacity);
    // setState(TransportState newState);
    // setExpirationTime(const time::steady_clock::TimePoint& expirationTime);

    // Create the neccessary thread to begin receving and transmitting
    pthread_t receive;
    int rc;
    
    void (LoRaTransport::*transmit_and_recieve)();

    rc = pthread_create(&receive, NULL, &LoRaTransport::transmit_and_receive_helper, this);
    if(rc) {
      handleError("Unable to create initial thread to create receive and transmitting thread: " + std::to_string(rc));
    }

    // Wait for the threads to join (user would have to ctrl-c)
    pthread_join(receive, NULL);
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
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_FACE_TRACE("Successfully sent: " << block.size() << " bytes");

  // After sending enter recieve mode again
  sx1272.receive();
  free(cstr);
}

/*
* Function used for switching from receiving --> transmitting --> receiving for the LoRa
*/
void *LoRaTransport::transmit_and_recieve()
{
  NFD_LOG_FACE_TRACE("Starting Lo-Ra thread");
  while(true){
      pthread_mutex_lock(&threadLock);
      // Check and see if there is something to send
      if (toSend) {
          ndn::EncodingBuffer buffer(*store_packet);
          if (buffer.size() <= 0) {
            NFD_LOG_FACE_TRACE("Trying to send a packet with no size");
          }

          // copy the buffer into a cstr so we can send it
          char *cstr = new char[buffer.size() + 1];
          uint8_t *buff = buffer.buf();
          for (size_t i = 0; i < buffer.size(); i++) {
            cstr[i] = buff[i];
          }
          if ((nfd::face::LoRaTransport::e = sx1272.sendPacketTimeout(0, cstr)) != 0) {
              handleError("Send operation failed: " + std::to_string(e));
          }  
          else
            // print block size because we don't want to count the padding in buffer
            NFD_LOG_FACE_TRACE("Successfully sent: " << buffer.size() << " bytes");

          // After sending enter recieve mode again
          sx1272.receive();
          free(cstr);
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
            uint8_t packetLength = sx1272.getCurrentPacketLength();
            for (i = 0; i < packetLength; i++)
            {
                my_packet[i] = (char)sx1272.packet_received.data[i];
            }

            // Reset null terminator
            my_packet[i] = '\0';

            NFD_LOG_FACE_TRACE("Received packet: ");
            NFD_LOG_FACE_TRACE(my_packet);
      }
      else {
        handleError("Unable to get packet data: " + std::to_string(e));
      }
      dataToConsume = sx1272.checkForData();
    }

    bool isOk = false;
    Block element;

    // Convert my_packet char [] --> const uint8_t*
    const uint8_t* buffer_ptr = (uint8_t*)my_packet;
    std::tie(isOk, element) = Block::fromBuffer(buffer_ptr, i);
    if (!isOk) {
      NFD_LOG_FACE_WARN("Failed to parse incoming packet");
      // This packet won't extend the face lifetime
      return;
    }
    this->receive(element);
} 

void LoRaTransport::handleError(const std::string &errorMessage) {
  if (getPersistency() == ndn::nfd::FACE_PERSISTENCY_PERMANENT) {
    NFD_LOG_FACE_DEBUG("Permanent face ignores error: " << errorMessage);
    return;
  }

  NFD_LOG_FACE_ERROR(errorMessage);
  this->setState(TransportState::FAILED);
  doClose();
}

}

}