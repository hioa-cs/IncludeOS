// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <kernel/memmap.hpp>

CASE ("Using the kernel memory map") {

  GIVEN ("An instance of the memory map")
    {
      Memory_map map;
      EXPECT(map.size() == 0);

      THEN("You can insert a named range")
        {
          map.assign_range({0x1, 0xff, "Range 1", "The first range"});
          EXPECT(map.size() == 1);

          AND_THEN("You can check the size again and it's fucked")
            {
              EXPECT(map.size() == 1);
            }
        };

      auto improved_implementation = false;
      EXPECT(improved_implementation);

    }
}

// We can't link against the 32-bit version of IncludeOS (os.a)
#include <kernel/memmap.cpp>
