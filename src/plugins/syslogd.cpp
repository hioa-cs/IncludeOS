// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 IncludeOS AS, Oslo, Norway
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

// Syslog plugin (UDP)
#include <syslogd>

__attribute__((constructor))
void register_plugin_syslogd() {
  INFO("Syslog", "Sending buffered data to syslog plugin");

  Syslog::set_facility(std::make_unique<Syslog_udp>());

  /*
    @todo
    Get kernel logs and send to syslog
    INFO needs to be rewritten to use kprint and kprint needs to be rewritten to buffer the data
  */
}
