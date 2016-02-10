// Copyright (C) 2014 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STORAGE_RAW_STORAGE_HPP_INCLUDED
#define FOONATHAN_STORAGE_RAW_STORAGE_HPP_INCLUDED

#include <new>
#include <type_traits>

#include "detail/aligned_union.hpp"
#include "pointer_cast.hpp"

namespace foonathan { namespace storage
{
    /// \brief A handy typedef to get an \c std::aligned_storage suitable for a collection of types.
    /// \note This is a very low-level building-block. It does not perform any error-checking.
    /// Use more advanced things like \ref optional or \ref variant in application code.
    template <typename ... Types>
    using storage = detail::aligned_union_t<Types...>;

    /// \brief Emplaces an object inside a \ref storage and returns a pointer to it.
    template <typename T, typename Storage, typename ... Args>
    auto emplace(Storage &s, Args&&... args)
    -> typename std::enable_if<std::is_pod<Storage>::value
                            && sizeof(T) <= sizeof(Storage)
                            && alignof(T) <= alignof(Storage), T*>::type
    {
        return ::new(void_pointer(&s)) T(std::forward<Args>(args)...);
    }

    /// @{
    /// \brief Returns a pointer to the object stored inside a \ref storage.
    template <typename T, typename Storage>
    T* get(Storage &s) noexcept
    {
        return pointer_cast<T>(&s);
    }

    template <typename T, typename Storage>
    const T* get(const Storage &s) noexcept
    {
        return pointer_cast<const T>(&s);
    }
    /// @}
}} // namespace foonathan::storage

#endif // FOONATHAN_STORAGE_STORAGE_HPP_INCLUDED
