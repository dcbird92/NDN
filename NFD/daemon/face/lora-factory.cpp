

#include "lora-factory.hpp"
#include "generic-link-service.hpp"
#include "common/global.hpp"


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
}

void
LoRaFactory::doProcessConfig(OptionalConfigSection configSection,
                            FaceSystem::ConfigContext& context)
{
}

void
LoRaFactory::doCreateFace(const CreateFaceRequest& req,
                         const FaceCreatedCallback& onCreated,
                         const FaceCreationFailedCallback& onFailure)
{

  for (const auto& i : m_channels) {
      i.second->createFace(req.params, onCreated);
      return;
    }

  NFD_LOG_TRACE("No LoRas available to connect to ");
  onFailure(504, "No LoRa available to connect");
}

shared_ptr<LoRaChannel>
LoRaFactory::createChannel()
{
  auto it = m_channels.find("default");
  if (it != m_channels.end())
    return it->second;

  auto channel = std::make_shared<LoRaChannel>();
  m_channels["default"] = channel;
  return channel;
}

std::vector<shared_ptr<const Channel>>
LoRaFactory::doGetChannels() const
{
  return getChannelsFromMap(m_channels);
}

} // namespace face
} // namespace nfd