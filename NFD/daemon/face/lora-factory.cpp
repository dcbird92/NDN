

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
}

void
LoRaFactory::doProcessConfig(OptionalConfigSection configSection,
                            FaceSystem::ConfigContext& context)
{
    providedSchemes.insert("lora");
    NFD_LOG_INFO("Lora Process Config");
}

void
LoRaFactory::doCreateFace(const CreateFaceRequest& req,
                         const FaceCreatedCallback& onCreated,
                         const FaceCreationFailedCallback& onFailure)
{

  try
  {
      for (const auto& i : m_channels) {
        // Found a channel already created
        if (i.first == req.remoteUri.getHost()) {
          NFD_LOG_INFO("Face already exists for " << req.remoteUri.toString());
          return;
        }
      }

      // Otherwise create a channel for this new request and a face associated with it
      auto channel = createChannel(req.remoteUri.toString());
      channel->createFace(req.remoteUri.getHost(), req.params, onCreated);
  }
  catch(const std::exception& e)
  {
      onFailure(504, "Unable to create LoRa face: " << e.what());
  }
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

} // namespace face
} // namespace nfd