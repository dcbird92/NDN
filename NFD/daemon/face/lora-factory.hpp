#ifndef NFD_DAEMON_FACE_LORA_FACTORY_HPP
#define NFD_DAEMON_FACE_LORA_FACTORY_HPP

#include "protocol-factory.hpp"
#include "lora-channel.hpp"


namespace nfd {
namespace face {

class LoRaFactory : public ProtocolFactory
{
public:
  class Error : public ProtocolFactory::Error
  {
  public:
    using ProtocolFactory::Error::Error;
  };

  static const std::string&
  getId() noexcept;

  explicit
  LoRaFactory(const CtorParams& params);

  using ProtocolFactory::ProtocolFactory;

  /**
   * \brief Create LoRa-based channel
   *
   * If this method is called twice the second time it won't do anything
   * because the lora will have been setup already
   *
   * \return always a valid pointer to a LoRaChannel object, an exception
   *         is thrown if it cannot be created.
   * \throw LoRaFactory::Error
   */
  std::shared_ptr<LoRaChannel>
  createChannel(std::string URI);


private:
  /** \brief process face_system.udp config section
   */
  void
  doProcessConfig(OptionalConfigSection configSection,
                  FaceSystem::ConfigContext& context) override;

  void
  doCreateFace(const CreateFaceRequest& req,
               const FaceCreatedCallback& onCreated,
               const FaceCreationFailedCallback& onFailure) override;

  std::vector<std::shared_ptr<const Channel>>
  doGetChannels() const override;


private:
  std::map<std::string, std::shared_ptr<LoRaChannel>> m_channels;

  void
  setup();

};

}
}

#endif // NFD_DAEMON_FACE_LORA_FACTORY_HPP
