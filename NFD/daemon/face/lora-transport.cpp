/**
 * File used for implementing each of the functions
 */

#include "lora-transport.hpp"

namespace nfd {

namespace face {

LoRaTransport::LoRaTransport() {

    // Set all of the static variables associated with this transmission 
    setLocalUri(const FaceUri& uri);
    setRemoteUri(const FaceUri& uri);
    setScope(ndn::nfd::FaceScope scope);
    setLinkType(ndn::nfd::LinkType linkType);
    setMtu(ssize_t mtu);
    setSendQueueCapacity(ssize_t sendQueueCapacity);
    setState(TransportState newState);
    setExpirationTime(const time::steady_clock::TimePoint& expirationTime);

    // Setup the lora chip
    setup();
}

LoRaTransport::receivePayload() {

}


LoRaTransport::setup() {
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

LoRaTransport::doClose() {
    // Close this form of transmission by turning off the chip
    e = sx1272.OFF()
    if (e) {
        printf("Unable to turn off LoRa chip, try again: state %d\n", e);
        return;
    }

    this->setState(TransportState::CLOSED);
}

LoRaTransport::doSend(const Block &packet, const EndpointId &endpoint) {
    
}

LoRaTransport::sendPacket(const ndn::Block &block) {

}

LoRaTransport::asyncRead() {

}

LoRaTransport::handleRead(const boost::system::error_code &error) {

}

LoRaTransport::handleError(const std::string &errorMessage) {

}

}

}