// Copyright (C) 2014 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STORAGE_OPTIONAL_HPP_INCLUDED
#define FOONATHAN_STORAGE_OPTIONAL_HPP_INCLUDED

#include <cassert>
#include <type_traits>

#include "raw_storage.hpp"

namespace foonathan { namespace storage
{
    /// \brief Tag type representing a null \ref optional.
    constexpr struct nullopt_t {} nullopt;

    /// \brief Either an object or a null state.
    /// \detail It uses \ref storage to store the object and acts like a pointer.
    template <typename T>
    class optional
    {
        static_assert(!std::is_reference<T>::value, "can't store references");
        using nothrow_move = std::is_nothrow_move_constructible<T>;
    public:
        //=== typedefs ===//
        using value_type = T;

        //=== constructors/destructor/assignment/swap ===//
        /// @{
        /// \brief Creates a null optional.
        optional() noexcept
        : null_(true) {}

        optional(nullopt_t) noexcept
        : optional() {}
        /// @}

        /// @{
        /// \brief Initializes it with a value.
        /// \see emplace() for a more sophisticated version.
        optional(const value_type &val)
        : null_(false)
        {
            emplace(storage_, val);
        }

        optional(value_type &&val) noexcept(nothrow_move::value)
        : null_(false)
        {
            emplace<value_type>(storage_, std::move(val));
        }
        /// @}

        /// \brief Copy-constructs an optional by copying its underlying object.
        optional(const optional &opt)
        : null_(opt.null_)
        {
            if (opt)
                emplace<value_type>(storage_, *opt);
        }

        /// \brief Move-constructs an optional by moving its underlying object.
        /// \detail The other optional still contains a value after the operation,
        /// but a value that has been moved out.
        optional(optional &&opt) noexcept(nothrow_move::value)
        : null_(opt.null_)
        {
            if (opt)
                emplace<value_type>(storage_, std::move(*opt));
        }

        /// \brief Destroys the associated object if it exists.
        ~optional() noexcept
        {
            *this = nullopt;
        }

        /// \brief Copy-assigns another optional by copying its underlying object.
        /// \detail This function just calls \ref emplace().
        optional& operator=(const optional &other)
        {
            emplace(*this, *other);
            return *this;
        }

        /// \brief Move-assigns another optional by moving its underlying object.
        /// \detail This function just calls \ref emplace().
        optional& operator=(optional &&other) noexcept(nothrow_move::value)
        {
            emplace(*this, std::move(*other));
            return *this;
        }

        /// \brief Assigns a null state, that is, destroys the old object if there was one before.
        optional& operator=(nullopt_t) noexcept
        {
            if (*this)
                (*this)->~value_type();
            null_ = true;
            return *this;
        }

        /// @{
        /// \detail This function just calls \ref emplace()
        /// \see emplace() for a more sophisticated version.
        optional& operator=(const value_type &val)
        {
            emplace(*this, val);
            return *this;
        }

        optional& operator=(value_type &&val)
        {
            emplace(*this, std::move(val));
            return *this;
        }
        /// @}

        /// \brief Swaps two optionals.
        /// \detail If both optionals contain a value, the value is swapped,
        /// if one of the optionals is null, the value from the other will be moved to it and the other will become null,
        /// if both are null, nothing happens.
        friend void swap(optional &a, optional &b) noexcept(nothrow_move::value)
        {
            using std::swap;
            if (a && b)
                swap(*a, *b);
            else if (a && !b)
            {
                emplace<value_type>(b.storage_, std::move(*a));
                b.null_ = false;
                a = nullopt;
            }
            else if (b && !a)
            {
                emplace<value_type>(a.storage_, std::move(*b));
                a.null_ = false;
                b = nullopt;
            }
        }

        //=== access ===//
        /// @{
        /// \brief Returns the stored object.
        /// \note The optional must not be in null state.
        value_type& operator*()
        {
            assert(!null_ && "optional must not be in null state");
            return *get<value_type>(storage_);
        }

        const value_type& operator*() const
        {
            assert(!null_ && "optional must not be in null state");
            return *get<value_type>(storage_);
        }

        value_type* operator->()
        {
            return &operator*();
        }

        const value_type* operator->() const
        {
            return &operator*();
        }
        /// @}

        /// \brief Returns \c true if there is an object stored, \c false otherwise.
        explicit operator bool() const noexcept
        {
            return !null_;
        }

        //=== comparison ===//
        /// @{
        /// \brief Compares two optionals for (in-)equality.
        /// \detail They are equal if both null or both non-null and the stored value is the same.
        friend bool operator==(const optional &a, const optional &b)
        {
            if (a && b)
                return *a == *b;
            return !a && !b;
        }

        friend bool operator!=(const optional &a, const optional &b)
        {
            return !(a == b);
        }
        /// @}

        /// @{
        /// \brief Compares an optional with a value.
        friend bool operator==(const optional &a, const value_type &b)
        {
            return a && *a == b;
        }

        friend bool operator==(const value_type &a, const optional &b)
        {
            return b == a;
        }

        friend bool operator!=(const optional &a, const value_type &b)
        {
            return !(a == b);
        }

        friend bool operator!=(const value_type &a, const optional &b)
        {
            return !(b == a);
        }
        /// @}

        /// @{
        /// \brief Compares an optional and \ref nullopt_t.
        /// \detail This is the same as asking an optional for validity.
        friend bool operator==(const optional &a, nullopt_t) noexcept
        {
            return !a;
        }

        friend bool operator==(nullopt_t, const optional &b) noexcept
        {
            return !b;
        }

        friend bool operator!=(const optional &a, nullopt_t) noexcept
        {
            return a;
        }

        friend bool operator!=(nullopt_t, const optional &b) noexcept
        {
            return b;
        }
        /// @}

    private:
         // single argument that can be assigned
        template <typename U>
        void assign(U &&u,
                    decltype(std::declval<T&>() = std::forward<U>(u), int()) = 0)
        {
            get<T>(*this) = std::forward<U>(u);
        }

        // multiple arguments or single one that can't be assigned directly
        template <typename ... Args>
        void assign(Args&&... args)
        {
            get<T>(*this) = T(std::forward<Args>(args)...);
        }

        storage<value_type> storage_;
        bool null_ = true;

        template <typename U, typename ... Args>
        friend void emplace(optional<U> &ref, Args&&... args);
    };

    /// \brief Makes an \ref optional.
    /// \relates optional
    template <typename T>
    optional<typename std::decay<T>::type> make_optional(T &&arg)
    {
        return {std::forward<T>(arg)};
    }

    /// \brief Emplaces an object inside an \ref optional.
    /// \detail If there is already an object stored, it will construct a temporary and move-assign it,
    /// otherwise it will create the new object inside it, providing the strong exception safety.
    /// \relates optional
    template <typename T, typename ... Args>
    void emplace(optional<T> &opt, Args&&... args)
    {
        if (opt)
            opt.assign(std::forward<Args>(args)...);
        else
            emplace<T>(opt.storage_, std::forward<Args>(args)...);
        opt.null_ = false;
    }

    /// @{
    /// \brief Returns the stored value of an \ref optional.
    /// \detail This is the same as \ref optional::operator*().
    /// \relates optional
    template <typename T>
    const T& get(const optional<T> &opt)
    {
        return *opt;
    }

    template <typename T>
    T& get(optional<T> &opt)
    {
        return *opt;
    }

    template <typename T>
    T&& get(optional<T> &&opt)
    {
        return std::move(*opt);
    }
    /// @}

    /// \brief Tries to return a copy of the stored value inside an \ref optional.
    /// \detail If there is not an object stored, it returns \c val.
    /// \relates optional
    template <typename T, typename U>
    T try_get(const optional<T> &opt, U &&val)
    {
        if (opt)
            return *opt;
        return std::forward<U>(val);
    }

    /// @{
    /// \brief Visits a \ref optional.
    /// \detail A \c Visitor is a function object providing an \c operator() taking the type of an \ref optional.
    /// This \c operator() is called if the \c optional currently has an object stored inside.
    /// \relates optional
    template <class Visitor, typename T>
    void visit(const optional<T> &opt, Visitor &&visitor)
    {
        if (opt)
            std::forward<Visitor>(visitor)(get<T>(opt));
    }

    template <class Visitor, typename T>
    void visit(optional<T> &opt, Visitor &&visitor)
    {
        if (opt)
            std::forward<Visitor>(visitor)(get<T>(opt));
    }

    template <class Visitor, typename T>
    void visit(optional<T> &&opt, Visitor &&visitor)
    {
        if (opt)
            std::forward<Visitor>(visitor)(get<T>(std::move(opt)));
    }
    /// @}
}} // namespace foonathan::storage

namespace std
{
    /// \brief Specialization of \c std::hash for \ref optional.
    /// \detail It calls the hash function for the underlying type if the \c optional contains a value,
    /// otherwise it will return a special value.
    template <typename T>
    class hash<foonathan::storage::optional<T>>
    {
    public:
        using argument_type = foonathan::storage::optional<T>;
        using result_type = size_t;

        result_type operator()(const argument_type &arg) const noexcept
        {
            return arg ? hash<T>()(*arg) :
                  static_cast<result_type>(19937); // magic value
        }
    };
} // namespace std

#endif // FOONATHAN_STORAGE_OPTIONAL_HPP_INCLUDED
