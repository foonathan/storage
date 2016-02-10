// Copyright (C) 2014 Jonathan Mller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <string>

#include "../optional.hpp"

// namespace alias
namespace storage = foonathan::storage;

// an expensive resource
using expensive_resource = std::string;

// a function that tries to create an expensive resource but can sometimes fail
storage::optional<expensive_resource> get_resource(int value)
{
    if (value == 42)
        return {}; // return null optional
    return {expensive_resource("Hello optional!")}; // return expensive resource
}

int main()
{
    auto res = get_resource(44);
    // bad style, avoid if:
    if (res)
        std::cout << "Got: " << *res << '\n'; 
        
    // good style, no if:
    // visit will only be called when optional not null
    visit(res, [](const expensive_resource &resource)
               {
                   std::cout << "Got " << resource << '\n';
               });
               
    // compare value
    std::cout << std::boolalpha << (res == "Hello!") << '\n';
    
    // get value or replacement if not stored
    std::cout << try_get(res, "no resource") << '\n';
}
