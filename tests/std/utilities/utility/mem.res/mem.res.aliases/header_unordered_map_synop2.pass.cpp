// modified by slod

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// XFAIL: use_system_cxx_lib && target={{.+}}-apple-macosx10.{{9|10|11|12|13|14|15}}
// XFAIL: use_system_cxx_lib && target={{.+}}-apple-macosx{{11.0|12.0}}

// <unordered_map>

// namespace std::pmr {
//
// typedef ... unordered_map
//
// } // namespace std::pmr

#include <memory_resource> // slod
#include <unordered_map>

int main(int, char**) {
  {
    // Check that std::pmr::unordered_map is usable without <memory_resource>.
    std::pmr::unordered_map<int, int> m;
    std::pmr::unordered_multimap<int, int> mm;
  }

  return 0;
}
