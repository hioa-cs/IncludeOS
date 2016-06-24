// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#define DEBUG

#include <os>
#include <net/packet.hpp>

namespace net {

  Packet::Packet(size_t bufsize, size_t datalen) noexcept
  : capacity_  {bufsize},
    size_      {datalen},
    next_hop4_ {}
{
  debug("<Packet> CONSTRUCT packet, buf @ %p\n", buf);
}

  IP4::addr Packet::next_hop() const noexcept {
    return next_hop4_;
  }

  void Packet::next_hop(IP4::addr ip) noexcept {
    next_hop4_ = ip;
  }

  int Packet::set_size(const size_t size) noexcept {
    if(size > capacity_) {
      return 0;
    }
    return size_ = size;
  }

} //< namespace net
