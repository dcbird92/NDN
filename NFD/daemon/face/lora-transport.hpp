/**
 * Header file used for creating the Lo-Ra transmission
 * 
 * 
 */

#include "transport.hpp"
#include <ndn-cxx/net/network-interface.hpp>
#include <string>
#include <iostream>
#include <pthread.h>
#include <queue>

// Include the SX1272 and SPI library:
#include "../../../libraries/arduPiLoRa/arduPiLoRa.h"

namespace nfd
{
namespace face
{

class LoRaTransport : public Transport
{
    class Error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
   * @brief Processes the payload of an incoming frame
   * @param payload Pointer to the first byte of data after the Ethernet header
   * @param length Payload length
   * @param sender Sender address
   */
    void
    receivePayload(const uint8_t *payload, size_t length,
                   const ethernet::Address &sender);

    /**
     * @brief Is used to setup the LoRa and set all the correct bits for the LoRa (intialization step)
     */ 
    void
    setup();

protected:
    LoRaTransport();

    void
    doClose() final;

    bool
    hasRecentlyReceived() const
    {
        return m_hasRecentlyReceived;
    }

private:
    void
    handleNetifStateChange(ndn::net::InterfaceState netifState);

    void
    doSend(const Block &packet, const EndpointId &endpoint) final;

    /**
   * @brief Sends the specified TLV block on the network wrapped in an Ethernet frame
   */
    void
    sendPacket(const ndn::Block &block);

    void
    asyncRead();

    void
    handleRead(const boost::system::error_code &error);

    void
    handleError(const std::string &errorMessage);

    // Variables
    private:
}

} // namespace face
} // namespace nfd