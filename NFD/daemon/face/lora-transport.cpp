/**
 * File used for implementing the transport layer of the LoRa Face
 */

#include "lora-transport.hpp"
namespace nfd {

namespace face {

NFD_LOG_INIT(LoRaTransport);

LoRaTransport::LoRaTransport(std::pair<uint8_t, uint8_t>* ids,
                            std::queue<std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>>& packetQueue, 
                            pthread_mutex_t& queueMutex) {

    // Set all of the static variables associated with this transmission (just need to set MTU)
    this->setMtu(160);

    // Read in a certain topology if flag is high (can add certain other LoRa IDs to send to and recv from)
    if (readTopology) {
        std::ifstream infile(topologyFilename); 
        std::string token;
        std::string value;
        while (std::getline(infile, token))
        {
          // Grab the ID field
          if (token.substr(0,2) == "id") {
            int id = token[3] - '0';
            int err = sx1272.setNodeAddress(id);  // Set the ID so src in packets can be set
            if (err != 0) {
              NFD_LOG_ERROR("Unable to set nodeAddress! Fatal, restart");
            }
          }
          // Grab the send field
          if (token.substr(0,4) == "send") {
            std::istringstream istr (token.substr(5));
            while(std::getline(istr, value, ',')) {
              send.insert((uint8_t)(value[0] - '0'));
            }
          }
          // Grab the recv field
          if (token.substr(0,4) == "recv") {
            std::istringstream istr (token.substr(5));
            while(std::getline(istr, value, ',')) {
              recv.insert((uint8_t)(value[0] - '0'));
            }
          }
        }
        NFD_LOG_INFO("Read in topology");
    }

    // Save the queue and mutex so we can add messages, so ultimately the lora module can send them
    sendBufferQueue = packetQueue;
    threadLock = queueMutex;
}

void LoRaTransport::doClose() {
  // Close this form of transmission by turning off the chip
  sx1272.OFF();

  this->setState(TransportState::FAILED);
}

void LoRaTransport::doSend(const ndn::Block &packet, const EndpointId& endpoint) {
  // Set the flag high that we have a packet to transmit, and push the new data onto the queue
  // std::make_pair(std::make_pair(-1L,-1L),std::make_pair(0L,0L))
  pthread_mutex_lock(&threadLock);
  sendBufferQueue.push(new ndn::EncodingBuffer(packet));
  NFD_LOG_FACE_TRACE("Send item added to queue");
  pthread_mutex_unlock(&threadLock);
}

void
LoRaTransport::receiveData(ndn::Block data) {
  this->receive(data);
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
