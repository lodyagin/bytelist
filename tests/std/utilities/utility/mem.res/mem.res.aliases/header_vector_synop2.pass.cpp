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

// <vector>

// namespace std::pmr {
//
// typedef ... vector
//
// } // namespace std::pmr

#include <memory_resource> // slod
#include <vector>

int main(int, char**) {
  {
    // Check that std::pmr::vector is usable without <memory_resource>.
    std::pmr::vector<int> l;
  }

  return 0;
}
