// Copyright (C) 2014 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STORAGE_POINTER_CAST_HPP_INCLUDED
#define FOONATHAN_STORAGE_POINTER_CAST_HPP_INCLUDED

#include <type_traits>

namespace foonathan { namespace storage
{
    namespace detail
    {
        template <typename T>
        struct get_void_ptr
        {
            using type = void*;
        };
        
        template <typename T>
        struct get_void_ptr<const T>
        {
            using type = const void*;
        };
        
        template <typename T>
        struct get_void_ptr<volatile T>
        {
            using type  = volatile void*;
        };
        
        template <typename T>
        struct get_void_ptr<const volatile T>
        {
            using type = const volatile void*;
        };
    } // namespace detail
    
    /// \brief Converts a pointer to [cv] void*.
    /// \detail The only purpose of this function is to give the code more semantic.
    template <typename From>
    constexpr auto void_pointer(From *ptr) noexcept
    -> typename detail::get_void_ptr<From>::type
    {
        return static_cast<typename detail::get_void_ptr<From>::type>(ptr);
    }
    
    /// \brief Converts a pointer from [cv] void*.
    /// \detail The only purpose of this function is to give the code more semantic.
    template <typename To>
    constexpr To* object_pointer(typename detail::get_void_ptr<To>::type ptr) noexcept
    {
        return static_cast<To*>(ptr);
    }
    
    /// \brief Converts two pointers.
    /// \detail The only purpose of this function is to give the code more semantic.
    /// \note Be aware of the strict aliasing rule!
    template <typename To, typename From>
    constexpr To* pointer_cast(From *ptr) noexcept
    {
        return reinterpret_cast<To*>(ptr);
    }
}} // namespace foonathan::storage

#endif // FOONATHAN_RAW_STORAGE_POINTER_CAST_HPP_INCLUDED
