// Copyright (C) 2014 Jonathan Mller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <string>

#include "../variant.hpp"

// namespace alias
namespace storage = foonathan::storage;

// a visitor printing its arguments
struct print_visitor
{
    template <typename T>
    void operator()(T &&arg)
    {
        std::cout << std::forward<T>(arg) << '\n';
    }
};

int main()
{
    // a type safe union of different elements
    storage::variant<int, float, std::string> variant(4);
    
    // compare value
    if (variant == 3.f)
        std::cout << "???\n";
    else if (variant == 4)
        std::cout << "variant is indeed 4\n";
    
    // reassign
    variant = std::string("Hello variant!");
    
    // bad style, avoid if:
    if (storage::contains<std::string>(variant))
        std::cout << storage::get<std::string>(variant) << '\n';
        
    // good style, no if:
    // visit calls the appropriate overload based on the stored type
    visit(variant, print_visitor());
    // you can also use C++14 generic lambdas if availble
    /* visit(variant, [](const auto &arg)
                   {
                       std::cout << arg << '\n';
                   }); */
                   
    // get value or replacement if not stored
    std::cout << storage::try_get(variant, 3.f) << '\n';
}
