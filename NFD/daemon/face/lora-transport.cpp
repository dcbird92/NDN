/**
 * File used for implementing the transport layer of the LoRa Face
 */

#include "lora-transport.hpp"
namespace nfd {

namespace face {

NFD_LOG_INIT(LoRaTransport);

LoRaTransport::LoRaTransport(std::queue<ndn::encoding::EncodingBuffer *>& packetQueue, 
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
            id = token[3] - '0';
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

    // Read the ID and connection IDs from the FaceURI string (taken in from commandline)
    // These fields have already been validated in face-uri.cpp, so no need for try-catch
    // std::size_t position = FaceURI.find('-');
    // if (position != std::string::npos) {
    //   // Grab this lora modules ID and who it can send/receive with
    //   std::string leftIDString = FaceURI.substr(0, position);
    //   std::string rightIDString = FaceURI.substr(position+1);
    //   NFD_LOG_ERROR("transport");
    //   NFD_LOG_ERROR(leftIDString);
    //   NFD_LOG_ERROR(rightIDString);
    //   id = std::stoi(leftIDString);
    //   int connID = std::stoi(rightIDString);
    //   send.insert(connID);
    //   recv.insert(connID);
    // }
    // else {
    //   // Grab the ID, broadcasting mode so add 0 to send/recv 
    //   id = std::stoi(FaceURI);
    //   send.insert(0);
    //   recv.insert(0);
    // }

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
  pthread_mutex_lock(&threadLock);
  sendBufferQueue.push(new ndn::EncodingBuffer(packet));
  NFD_LOG_INFO("\n\ndoSend: added item to queue, set toSend true");
  pthread_mutex_unlock(&threadLock);

  NFD_LOG_TRACE(__func__);
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
