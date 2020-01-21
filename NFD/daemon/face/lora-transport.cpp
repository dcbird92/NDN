/**
 * File used for implementing each of the functions
 */

#include "lora-transport.hpp"

namespace nfd {

namespace face {

LoRaTransport::LoRaTransport() {

}

LoRaTransport::receivePayload(const uint8_t *payload, size_t length,
                const ethernet::Address &sender);


LoRaTransport::setup() {

}

LoRaTransport::doClose() {

}

LoRaTransport::doSend(const Block &packet, const EndpointId &endpoint) {
    
}

LoRaTransport::sendPacket(const ndn::Block &block) {

}

LoRaTransport::asyncRead() {

}

LoRaTransport::handleRead(const boost::system::error_code &error) {

}

LoRaTransport::(const std::string &errorMessage) {
    
}

}

}