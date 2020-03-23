/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "face.hpp"
#include "generic-link-service.hpp"
#include "common/global.hpp"
#include "../../../libraries/arduPiLoRa/arduPiLoRa.h"
#include "lora-transport.hpp"
#include "lora-channel.hpp"


namespace nfd {
namespace face {

NFD_LOG_INIT(LoRaChannel);

LoRaChannel::LoRaChannel(){
  setUri(FaceUri());
  NFD_LOG_CHAN_INFO("Creating channel");
}


void
LoRaChannel::createFace( const FaceParams& params,
                       const FaceCreatedCallback& onFaceCreated)
{
  shared_ptr<Face> face;
  setup();
  auto linkService = make_unique<GenericLinkService>();
  auto transport = make_unique<LoRaTransport>();
  face = make_shared<Face>(std::move(linkService), std::move(transport));
  m_channelFaces["default"] = face;

  // Need to invoke the callback regardless of whether or not we have already created
  // the face so that control responses and such can be sent.
  onFaceCreated(face);
}

void
LoRaChannel::setup(){
    // Print a start message
  int e;
  // Power ON the module
  e = sx1272.ON();

  // Set transmission mode
  e = sx1272.setMode(4);

  // Set header
  e = sx1272.setHeaderON();

  // Select frequency channel
  e = sx1272.setChannel(CH_10_868);

  // Set CRC
  e = sx1272.setCRC_ON();

  // Select output power (Max, High or Low)
  e = sx1272.setPower('H');

  // Set the node address
  e = sx1272.setNodeAddress(3);

  // Set the LoRa into receive mode by default
  e = sx1272.receive();
  if (e)
    NFD_LOG_CHAN_DEBUG("Unable to enter receive mode");

  // Print a success message
  NFD_LOG_CHAN_DEBUG("SX1272 successfully configured\n\n");
  delay(1000);
}

}
}
