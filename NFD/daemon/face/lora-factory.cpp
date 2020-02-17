

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
  static std::string id("udp");
  return id;
}

LoRaFactory::UdpFactory(const CtorParams& params)
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
}

shared_ptr<UdpChannel>
LoRaFactory::createChannel( time::nanoseconds idleTimeout)
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