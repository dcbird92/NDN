

#include "lora-factory.hpp"
#include "generic-link-service.hpp"
#include "common/global.hpp"
#include "../../lora_libs/libraries/arduPiLoRa/arduPiLoRa.h"

namespace nfd {
namespace face {

NFD_LOG_INIT(LoRaFactory);
NFD_REGISTER_PROTOCOL_FACTORY(LoRaFactory);


const std::string&
LoRaFactory::getId() noexcept
{
  static std::string id("lora");
  return id;
}

LoRaFactory::LoRaFactory(const CtorParams& params)
  : ProtocolFactory(params)
{
  // Start the lora interface
  setup();
  providedSchemes.insert("lora");

  // Create the neccessary thread to begin receving and transmitting
  pthread_t receive;
  int rc;

  rc = pthread_create(&receive, NULL, &LoRaFactory::transmit_and_receive_helper, this);
  if(rc) {
    NFD_LOG_ERROR("Unable to create initial thread to create receive and transmitting thread: " + std::to_string(rc));
  }
}

void
LoRaFactory::doProcessConfig(OptionalConfigSection configSection,
                            FaceSystem::ConfigContext& context)
{
    // Not implemented yet! Implement if time
}

void
LoRaFactory::doCreateFace(const CreateFaceRequest& req,
                         const FaceCreatedCallback& onCreated,
                         const FaceCreationFailedCallback& onFailure)
{
  std::string URI = req.remoteUri.toString();
  try
  { 
      std::map<std::string, std::shared_ptr<LoRaChannel>> channels;
      // unicast
      if (URI.find('-') != std::string::npos) {
        channels = m_channels;
      }
      else {
        channels = mcast_channels;
      }
      for (const auto& i : channels) {
        // Found a channel already created
        if (i.first == req.remoteUri.toString()) {
          const std::string retStr = "Face already exists for " + URI;
          onFailure(504, retStr);
          return;
        }
      }

      // Otherwise create a channel for this new request and a face associated with it (either unicast or multicast)
      // If the URI contains a '-', we know its a unicast face
      size_t hyphenPosition = URI.find('-');
      if (hyphenPosition != std::string::npos) {
        auto channel = createChannel(URI);
        uint8_t id = std::stoi(URI.substr(7, hyphenPosition - 7));
        uint8_t connID = std::stoi(URI.substr(hyphenPosition+1));
        std::pair<uint8_t, uint8_t> sendIDAndConnID = std::make_pair(id, connID);
        channel->createFace(&sendBufferQueue, &threadLock, sendIDAndConnID, req.params, onCreated, onFailure);
      }
      // Otherwise its a multicast face (broadcast)
      else {
        auto channel = createMultiCastChannel(URI);
        uint8_t id = std::stoi(URI.substr(7));
        std::pair<uint8_t, uint8_t> sendIDAndConnID = std::make_pair(id, BROADCAST_0);
        channel->createFace(&sendBufferQueue, &threadLock, sendIDAndConnID, req.params, onCreated, onFailure);
      }

  }
  catch(const std::exception& e)
  {
      onFailure(504, e.what());
  }
}

std::shared_ptr<LoRaChannel>
LoRaFactory::createMultiCastChannel(std::string URI) {
  auto it = mcast_channels.find(URI);
  if (it != mcast_channels.end())
    return it->second;

  auto channel = std::make_shared<LoRaChannel>(URI);
  mcast_channels[URI] = channel;
  return channel;
}

std::shared_ptr<LoRaChannel>
LoRaFactory::createChannel(std::string URI)
{
  auto it = m_channels.find(URI);
  if (it != m_channels.end())
    return it->second;

  auto channel = std::make_shared<LoRaChannel>(URI);
  m_channels[URI] = channel;
  return channel;
}

std::vector<std::shared_ptr<const Channel>>
LoRaFactory::doGetChannels() const
{
  return getChannelsFromMap(m_channels);
}


void
LoRaFactory::setup(){
    // Print a start message
  int e;
  // Power ON the module
  e = sx1272.ON();

  // Set transmission mode
  //e = sx1272.setMode(4);
  
  
  //Set Operating Parameters Coding Rate CR, Bandwidth BW, and Spreading Factor SF
  
  e = sx1272.setCR(CR_5);
  e = sx1272.setBW(BW_500);
  e = sx1272.setSF(SF_7);

  // Set header
  e = sx1272.setHeaderON();

  // Select frequency channel
  e = sx1272.setChannel(CH_00_900);

  // Set CRC
  e = sx1272.setCRC_ON();

  // Select output power (Max, High or Low)
  e = sx1272.setPower('H');

  // Set the LoRa into receive mode by default
  e = sx1272.receive();
  if (e)
    NFD_LOG_INFO("Unable to enter receive mode");

  // Print a success message
  NFD_LOG_INFO("SX1272 successfully configured");
  delay(1000);
}

/*
* Function used for switching from receiving --> transmitting --> receiving for the LoRa
*/
void *LoRaFactory::transmit_and_recieve()
{
  NFD_LOG_INFO("Starting Lo-Ra thread");
  while(true){

    // Alternate between sending and receiving, so sending doesn't starve receive thread
    pthread_mutex_lock(&threadLock);
    // Check and see if there is something to send
    if(sendBufferQueue.size() > 0) {
      sendPacket();
    }

    // After sending enter recieve mode again
    sx1272.receive();
    pthread_mutex_unlock(&threadLock);

    // Check to see if the LoRa has received data... if so handle it (0ms wait for data, just checks once)
    if (sx1272.checkForData()) {
      handleRead();
    }
  }
}

void
LoRaFactory::sendPacket()
{
  std::pair<std::pair<uint8_t, uint8_t>*, ndn::encoding::EncodingBuffer *>* queueElement = sendBufferQueue.front(); 
  sendBufferQueue.pop();
  sendBuffer = queueElement->second;
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
    sentStuff += std::to_string((int)cstr[idx]) + ", ";
  }
  NFD_LOG_INFO("Message that is to be sent: " << sentStuff);

  std::pair<uint8_t, uint8_t>* ids = queueElement->first;
  uint8_t id = ids->first;
  uint8_t dst = ids->second;
  // Set LoRa source, send to dst
  sx1272.setNodeAddress(id);
  if ((e = sx1272.sendPacketTimeout(dst, cstr, bufSize)) != 0)
  {
    NFD_LOG_ERROR("Send operation failed: " + std::to_string(e));
  }
  else
  {
    NFD_LOG_INFO("sent to " << std::to_string(dst) << " from " << std::to_string(id));
    // print block size because we don't want to count the padding in buffer
    NFD_LOG_INFO("Supposedly sent: " << bufSize << " bytes");
    NFD_LOG_INFO("LoRa actually sent: " << sx1272._payloadlength << " _payloadlength bytes");

  }

  // Have to free all of this stuff
  delete[] cstr;
  delete sendBuffer; 
}

void
LoRaFactory::handleRead() {
  
  bool dataToConsume = true;
  bool packetCreated = false;
  int i;

  while (dataToConsume) {
    e = sx1272.getPacket();
    if (e == 0) {

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
    gotStuff += std::to_string((int)my_packet[idx]);
  }
  NFD_LOG_INFO("Packet ascii:" << gotStuff);

  try
  {
    ndn::Block element = ndn::Block((uint8_t*)my_packet, i);
    // See what unicast faces want this data
    for (const auto& i : m_channels) {
      std::size_t position = i.first.find('-');
      // Check to see if the connection ID matches src, if it does pass the data to this face. Also if ID matches
      // lora://<id>-<connID>
      std::string idString = i.first.substr(7, position - 7);
      std::string connIDString = i.first.substr(position+1);
      NFD_LOG_ERROR("id " << idString);
      NFD_LOG_ERROR("connID " << connIDString);
      if (std::stoi(connIDString) == sx1272.packet_received.src && (std::stoi(idString) == sx1272.packet_received.dst || sx1272.packet_received.dst == BROADCAST_0)) {
        i.second->handleReceive(element);
      }
    }
    // Pass to all broadcast channels with the right source
    // lora://<id>
    for (const auto& i : mcast_channels) {
      std::string idString = i.first.substr(7);
      if (std::stoi(idString) == sx1272.packet_received.dst || sx1272.packet_received.dst == BROADCAST_0) {
        i.second->handleReceive(element);
      }
    }
    NFD_LOG_INFO("Created block succesfully and called receive");
  }
  catch(const std::exception& e)
  {
    NFD_LOG_ERROR("Block create exception: " << e.what());
  }
} 

} // namespace face
} // namespace nfd