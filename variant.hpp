// Copyright (C) 2014 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STORAGE_VARIANT_HPP_INCLUDED
#define FOONATHAN_STORAGE_VARIANT_HPP_INCLUDED

#include <cassert>
#include <type_traits>

#include "detail/variant_helper.hpp"
#include "raw_storage.hpp"

namespace foonathan { namespace storage
{
    /// \brief Tag type representing a null \ref variant.
    constexpr struct nullvar_t {} nullvar;

    /// \brief A list of types that can occur in a certain variants.
    using detail::variant_types;

    /// \brief Similar to \ref optional, but can contain one of multiple types.
    /// \detail It is a type-safe union taking care of proper construction and destroying
    /// which also can have no value at all.<br>
    /// It uses \ref storage to store the objects.
    /// \note It only makes sense to have distinct types in the type list.<br>
    /// It cannot handle references.
    template <typename ... Types>
    class variant
    {
        static_assert(detail::valid_types<Types...>(), "can't store references");
        using trivial = std::integral_constant<bool, detail::trivial_types<Types...>()>;
        using nothrow_move = std::integral_constant<bool, detail::nothrow_move_types<Types...>()>;
    public:
        //=== statics ===//
        /// \brief Represents an invalid index of a type.
        static constexpr auto invalid_index = sizeof...(Types);

        /// \brief The types.
        using types = variant_types<Types...>;

        //=== constructors/destructor/assignment/swap ===//
        /// @{
        /// \brief Creates a null variant.
        constexpr variant() noexcept
        : which_(invalid_index) {}

        constexpr variant(nullvar_t) noexcept
        : variant() {}
        /// @}

        /// \brief Initializes a variant with a value.
        /// \note The type must be one of the types specified.
        /// \see emplace() for a more sophisticated version.
        template <typename T>
        variant(T &&val)
        {
            using t = typename std::decay<T>::type;
            constexpr auto index = detail::get_index<t, Types...>();
            static_assert(index != invalid_index,
                          "type is not one of the types specified");
            emplace<t>(storage_, std::forward<T>(val));
            which_ = index;
        }

        /// \brief Copy-constructs a variant by copying the currently stored type.
        variant(const variant &other)
        {
            construct(trivial(), other);
        }

        /// \brief Move-constructs a variant by moving the currently stored type.
        /// \detail The other variant still contains a value of that type after the operation,
        /// but a value that has been moved out.
        variant(variant &&other) noexcept(nothrow_move::value)
        {
            construct(trivial(), std::move(other));
        }

        /// \brief Destroys the currently stored value - if any.
        ~variant() noexcept
        {
            *this = nullvar;
        }

        /// \brief Copy-assigns another variant by copying the currently stored value.
        /// \detail It just calls \ref emplace() passing the currently stored type.
        variant& operator=(const variant &other)
        {
            if (trivial::value)
                construct(std::true_type(), other);
            else
                visit(std::move(other), detail::variant_emplace_visitor(*this));
            return *this;
        }

        /// \brief Move-assigns another variant by moving the currently stored value.
        /// \detail It just calls \ref emplace() passing the currently stored type.
        variant& operator=(variant &&other) noexcept(nothrow_move::value)
        {
            if (trivial::value)
                construct(std::true_type(), std::move(other));
            else
                visit(std::move(other), detail::variant_emplace_visitor(*this));
            return *this;
        }

        /// \brief Assigns a null state to the variant and destroys the formerly stored value.
        variant& operator=(nullvar_t) noexcept
        {
            if (!trivial::value && *this)
                visit(*this, detail::variant_destroy_visitor());
            which_ = invalid_index;
            return *this;
        }

        /// \brief Assigns a new value.
        /// \note The type must be one of the types specified.
        /// \see emplace() for a more sophisticated version.
        template <typename T>
        variant& operator=(T &&val)
        {
            using t = typename std::decay<T>::type;
            emplace<t>(*this, std::forward<T>(val));
            return *this;
        }

        /// \brief Swaps two variants.
        /// \details If both variants contain the same types, the swap from the type is called.
        /// If only one  of them contains an object, this object is moved over and at the old position destroyed.
        /// If both contains an object but of different types, the variant is swapped via \c std::swap on the variants.
        /// \note In the last case, a throwing move constructor only allows the basic exception safety.
        friend void swap(variant &a, variant &b) noexcept(nothrow_move::value)
        {
            if (trivial::value)
            {
                std::swap(a.storage_, b.storage_);
                std::swap(a.which_, b.which_);
            }
            else
            {
                if (a && b)
                {
                    if (a.which() == b.which())
                        visit(b, detail::variant_swap_visitor(a));
                    else
                        std::swap(a, b);
                }
                else if (a && !b)
                {
                     b.construct(std::move(a));
                     a = nullvar;
                }
                else if (b && !a)
                {
                    a.construct(std::move(b));
                    b = nullvar;
                }
            }
        }

        //=== accessors ===//
        /// \brief Returns \c true if there is an object currently stored inside the variant.
        explicit operator bool() const noexcept
        {
            return which_ != invalid_index;
        }

        /// \brief Returns the index of the type of the currently stored inside the variant.
        /// \detail If the variant is not vaild, it returns \ref invalid_index.
        std::size_t which() const noexcept
        {
            return which_;
        }

        //=== comparision ===//
        /// @{
        /// \brief Compares two variants for (in-)equality.
        /// \detail They are equal, if either both are invalid or store the same object.
        friend bool operator==(const variant &a, const variant &b)
        {
            if (!a && !b)
                return true;
            if (a.which() == b.which())
            {
                detail::variant_compare_visitor<variant> visitor(a);
                visit(b, visitor);
                return visitor.equal;
            }
            return false;
        }

        friend bool operator!=(const variant &a, const variant &b)
        {
            return !(a == b);
        }
        /// @}

        /// @{
        /// \brief Compares a variant with a value.
        template <typename T>
        friend bool operator==(const variant &a, const T &b)
        {
            constexpr auto index = detail::get_index<T, Types...>();
            static_assert(index != variant<Types...>::invalid_index,
                         "type is not one of the types specified");;
            return a.which() == index && get<T>(a) == b;
        }

        template <typename T>
        friend bool operator==(const T &a, const variant &b)
        {
            return b == a;
        }

        template <typename T>
        friend bool operator!=(const variant &a, const T &b)
        {
            return !(a == b);
        }

        template <typename T>
        friend bool operator!=(const T &a, const variant &b)
        {
            return !(b == a);
        }
        /// @}

        /// @{
        /// \brief Compares a variant and \ref nullvar_t.
        /// \detail This is the same as asking a variant for validity.
        friend bool operator==(const variant &a, nullvar_t) noexcept
        {
            return !a;
        }

        friend bool operator==(nullvar_t, const variant &b) noexcept
        {
            return !b;
        }

        friend bool operator!=(const variant &a, nullvar_t) noexcept
        {
            return a;
        }

        friend bool operator!=(nullvar_t, const variant &b) noexcept
        {
            return b;
        }
        /// @}

    private:
        // trivial types only
        template <class Variant>
        void construct(std::true_type, Variant &&other) noexcept
        {
            storage_ = other.storage_;
            which_ = other.which_;
        }

        // non-trivial type
        template <class Variant>
        void construct(std::false_type, Variant &&other)
        {
            if (!other)
                *this = nullvar;
            else
            {
                visit(std::forward<Variant>(other), detail::variant_emplace_visitor(storage_));
                which_ = other.which_;
            }
        }

        // single argument that can be assigned
        template <typename T, typename U>
        void assign(U &&u,
                    decltype(std::declval<T&>() = std::forward<U>(u), int()) = 0)
        {
            get<T>(*this) = std::forward<U>(u);
        }

        // multiple arguments or single one that can't be assigned directly
        template <typename T, typename ... Args>
        void assign(Args&&... args)
        {
            get<T>(*this) = T(std::forward<Args>(args)...);
        }

        // create temporary and move in because constructor can throw and move does not
        template <typename T, typename ... Args>
        void emplace_impl(std::true_type, Args&&... args)
        {
            T tmp(std::forward<Args>(args)...);
            *this = nullvar;
            emplace<T>(storage_, std::move(tmp));
        }

        // emplace directly, because either nothing or everything throws
        template <typename T, typename ... Args>
        void emplace_impl(std::false_type, Args&&... args)
        {
            *this = nullvar;
            emplace<T>(storage_, std::forward<Args>(args)...);
        }

        storage<Types...> storage_;
        std::size_t which_;

        template <typename T, typename ... UTypes, typename ... Args>
        friend void emplace(variant<UTypes...> &var, Args&&... args);
        template <typename T, typename ... UTypes>
        friend const T& get(const variant<UTypes...> &var);
    };

    /// @{
    /// \brief Returns the index of a type for a specific \ref variant.
    /// \detail If the type is not part of the variant, returns \ref variant::invalid_index.
    /// \relates variant
    template <typename T, typename ... Types>
    constexpr std::size_t get_index(variant_types<Types...>) noexcept
    {
        return detail::get_index<T, Types...>();
    }

    template <typename T, typename ... Types>
    std::size_t get_index(const variant<Types...> &) noexcept
    {
        return get_index<T>(variant_types<Types...>());
    }

    template <typename T, class Variant>
    constexpr std::size_t get_index() noexcept
    {
        return get_index<T>(Variant::Types());
    }
    /// @}

    /// \brief Whether or not a \ref variant currently contains an object of specific type.
    /// \relates variant
    template <typename T, typename ... Types>
    bool contains(const variant<Types...> &var) noexcept
    {
        auto index = get_index<T>(var);
        return var && var.which() == index;
    }

    /// \brief Emplaces a new object inside a \ref variant.
    /// \detail If the type is already stored inside it, it will move assign to it.
    /// If there is no type stored inside it, it will create it there.
    /// Else there is already an object of different type stored,
    /// then the old object will be destroyed and a new one constructruced, possibly over a temporary.<br>
    /// Overall, this function provides the strong exception safety if the assignment operator provides it for case 1
    /// and if the selected constructor can throw, the move constructor doesn't throw.
    /// If it does, it provides the basic exception safety and can be left in invalid state.
    /// \note The type must be one of the types specified.
    /// \relates variant
    template <typename T, typename ... Types, typename ... Args>
    void emplace(variant<Types...> &var, Args&&... args)
    {
        using t = typename std::decay<T>::type;
        constexpr auto index = detail::get_index<t, Types...>();
        static_assert(index != variant<Types...>::invalid_index,
                     "type is not one of the types specified");
        if (variant<Types...>::trivial::value || !var)
            // just create new one, nothing to destroy
            emplace<t>(var.storage_, std::forward<Args>(args)...);
        else if (index == var.which_)
            // assign new value
            var.template assign<t>(std::forward<Args>(args)...);
        else
            // destroy and create new
            var.template emplace_impl<t>(std::integral_constant<bool,
                                         !noexcept(T(std::forward<Args>(args)...))
                                         && std::is_nothrow_move_constructible<T>::value>{},
                                         std::forward<Args>(args)...);
        var.which_ = index;
    }

    /// @{
    /// \brief Returns a reference to the object currently stored.
    /// \detail You have to specify the type explicitly and it must be currently stored.
    /// \relates variant
    template <typename T, typename ... Types>
    const T& get(const variant<Types...> &var)
    {
        assert(var && var.which() == get_index<T>(var)
            && "type not currently stored inside the variant");
        return *get<T>(var.storage_);
    }

    template <typename T, typename ... Types>
    T& get(variant<Types...> &var)
    {
        const variant<Types...> &cvar = var;
        return const_cast<T&>(get<T>(cvar));
    }

    template <typename T, typename ... Types>
    T&& get(variant<Types...> &&var)
    {
        return std::move(get<T>(var));
    }
    /// @}

    /// @{
    /// \brief Tries to return a copy of the stored object inside a \ref variant.
    /// \detail If there is not an object of given type stored, it will return \c val.
    /// \relates variant
    template <typename T, typename ... Types, typename U>
    T try_get(const variant<Types...> &var, U &&val)
    {
        if (contains<T>(var))
            return get<T>(var);
        return std::forward<U>(val);
    }

    template <typename ... Types, typename T>
    T try_get(const variant<Types...> &var, T &&val)
    {
        using t = typename std::decay<T>::type;
        if (contains<t>(var))
            return get<t>(var);
        return std::forward<T>(val);
    }
    /// @}

    /// @{
    /// \brief Visits a \ref variant.
    /// \detail A \c Visitor is a function object providing an \c operator() taking a type of a variant for each type.
    /// This function selects the appropriate one based on the type currently stored in it.
    /// \relates variant
    template <class Visitor, typename ... Types>
    void visit(const variant<Types...> &var, Visitor &&visitor)
    {
        detail::variant_visit(variant_types<Types...>(),
                              std::forward<Visitor>(visitor), var);
    }

    template <class Visitor, typename ... Types>
    void visit(variant<Types...> &var, Visitor &&visitor)
    {
        detail::variant_visit(variant_types<Types...>(),
                              std::forward<Visitor>(visitor), var);
    }

    template <class Visitor, typename ... Types>
    void visit(variant<Types...> &&var, Visitor &&visitor)
    {
        detail::variant_visit(variant_types<Types...>(),
                              std::forward<Visitor>(visitor), std::move(var));
    }
    /// @}
}} // namespace foonathan::storage

namespace std
{
    /// \brief Specialization of \c std::hash for \ref variant.
    /// \detail It calls the hash function for the underlying type if any is currently stored,
    /// otherwise it will return a special value.
    template <typename ... Types>
    class hash<foonathan::storage::variant<Types...>>
    {
    public:
        using argument_type = foonathan::storage::variant<Types...>;
        using result_type = size_t;

        result_type operator()(const argument_type &arg) const noexcept
        {
            if (!arg)
                return static_cast<result_type>(19937); // magic value
            foonathan::storage::detail::variant_hash_visitor hasher;
            visit(arg, hasher);
            return hasher.hash;
        }
    };
} // namespace std

#endif // FOONATHAN_STORAGE_VARIANT_HPP_INCLUDED
