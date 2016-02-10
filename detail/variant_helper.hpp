// Copyright (C) 2014 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STORAGE_DETAIL_VARIANT_HELPER_HPP_INCLUDED
#define FOONATHAN_STORAGE_DETAIL_VARIANT_HELPER_HPP_INCLUDED

#include <functional>
#include <type_traits>

namespace foonathan { namespace storage { namespace detail
{
    template <typename ... Types>
    class variant;

    template <typename T, typename ... Types>
    const T& get(const variant<Types...> &var);

    template <typename T, typename ... Types>
    T& get(variant<Types...> &var);

    template <typename T, typename ... Types>
    T&& get(variant<Types...> &&var);

    template <typename T, typename ... Types, typename ... Args>
    void emplace(variant<Types...> &var, Args&&... args);

    //=== get_index implementation ===//
    template <typename T, typename ... Types>
    struct get_index_t;

    template <typename T, typename Head, typename ... Tail>
    struct get_index_t<T, Head, Tail...>
    : std::integral_constant<std::size_t, get_index_t<T, Tail...>::value + 1> {};

    template <typename T, typename ... Tail>
    struct get_index_t<T, T, Tail...>
    : std::integral_constant<std::size_t, 0> {};

    template <typename T>
    struct get_index_t<T>
    : std::integral_constant<std::size_t, 0> {};

    template <typename T, typename ... Types>
    constexpr std::size_t get_index() noexcept
    {
        return get_index_t<T, Types...>::value;
    }

    //=== query information ===//
    template <typename ... Types>
    struct variant_types {};

    template <typename ... Types>
    using all_true_types = variant_types<typename std::conditional<true, std::true_type, Types>::type...>;

    template <bool val>
    using select_type = typename std::conditional<val, std::true_type, std::false_type>::type;

    template <typename ... Types>
    constexpr bool valid_types() noexcept
    {
        using is_valid_types = variant_types<select_type<!std::is_reference<Types>::value>...>;
        return std::is_same<is_valid_types, all_true_types<Types...>>::value;
    }

    template <typename ... Types>
    constexpr bool trivial_types() noexcept
    {
        using is_trivial_types = variant_types<select_type<std::is_trivial<Types>::value>...>;
        return std::is_same<is_trivial_types, all_true_types<Types...>>::value;
    }

    template <typename ... Types>
    constexpr bool nothrow_move_types() noexcept
    {
        using is_nothrow_move_types = variant_types<select_type<std::is_nothrow_move_constructible<Types>::value>...>;
        return std::is_same<is_nothrow_move_types, all_true_types<Types...>>::value;
    }

    //=== visit implementation ===//
    template <typename T, class Visitor, class Variant>
    void call_visitor(Visitor &&visitor, Variant &&variant)
    {
        std::forward<Visitor>(visitor)(get<T>(std::forward<Variant>(variant)));
    }

    template <typename ... Types, class Visitor, class Variant>
    void variant_visit(variant_types<Types...>, Visitor &&visitor, Variant &&variant)
    {
        if (!variant)
            return;

        using visit_fnc = void(*)(Visitor&&, Variant&&);
        visit_fnc fncs[] = {static_cast<visit_fnc>(&call_visitor<Types, Visitor, Variant>)...};
        fncs[variant.which()](std::forward<Visitor>(visitor), std::forward<Variant>(variant));
    }

    //=== visitors ===//
    template <class Storage>
    struct variant_emplace_visitor_t
    {
        Storage *storage_;

        template <typename T>
        void operator()(T &&other)
        {
            using t = typename std::decay<T>::type;
            emplace<t>(*storage_, std::forward<T>(other));
        }
    };

    template <class Storage>
    auto variant_emplace_visitor(Storage &&storage)
    -> variant_emplace_visitor_t<typename std::decay<Storage>::type>
    {
        return {&storage};
    }

    template <class Storage>
    struct variant_swap_visitor_t
    {
        Storage *storage_;

        template <typename T>
        void operator()(T &other)
        {
            using std::swap;
            swap(get<T>(*storage_), other);
        }
    };

    template <class Storage>
    auto variant_swap_visitor(Storage &&storage)
    -> variant_swap_visitor_t<typename std::decay<Storage>::type>
    {
        return {&storage};
    }

    struct variant_destroy_visitor
    {
        template <typename T>
        void operator()(T &obj) noexcept
        {
            obj.~T();
        }
    };

    struct variant_hash_visitor
    {
        std::size_t hash;

        template <typename T>
        void operator()(T &&arg)
        {
            hash = std::hash<typename std::decay<T>::type>()(std::forward<T>(arg));
        }
    };

    template <class Variant>
    struct variant_compare_visitor
    {
        const Variant *var;
        bool equal;

        variant_compare_visitor(const Variant &var) noexcept
        : var(&var) {}

        template <typename T>
        void operator()(const T &a)
        {
            equal = get<T>(*var) == a;
        }
    };
}}} // namespace foonathan::storage::detail

#endif // FOONATHAN_STORAGE_DETAIL_VARIANT_HELPER_HPP_INCLUDED
