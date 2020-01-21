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
   */
    void
    receivePayload();

    /**
     * @brief Is used to setup the LoRa and set all the correct bits for the LoRa (intialization step)
     */ 
    void
    setup();

protected:
    LoRaTransport();

    void
    doClose() final;

private:
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
        int e;
        std::string m;
        char my_packet[256];
        bool toSend;

        // Creating mutexes for shared queue and conditions for when data is produced from console
        pthread_mutex_t threadLock = std::PTHREAD_MUTEX_INITIALIZER; 
        
        pthread_cond_t dataSent =  
                            std::PTHREAD_COND_INITIALIZER; 

        // Shared queue 
        std::queue<std::string> Q; 
}

} // namespace face
} // namespace nfd