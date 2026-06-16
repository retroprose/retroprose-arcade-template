#ifndef TYPEMETA_TYPEMETA_HPP
#define TYPEMETA_TYPEMETA_HPP

#include <tuple>
#include <cstring>

//specialize this class to make type meta for classes
template<typename T>
struct TypeMeta { };

// wrapper class for cstrings
class StaticString {
public:
    constexpr StaticString() : ptr(nullptr) { }
    constexpr StaticString(const char* p) : ptr(p) { }
    const char* c_str() const { return ptr; }
    bool isNull() const { return ptr == nullptr; }
    bool operator<(const StaticString& other) const {
        return std::strcmp(ptr, other.ptr) < 0;
    }
private:
    const char* ptr;
};

// for binding strings to member variables
template<typename T, typename M>
struct Bind {
    using class_type = T;
    using member_type = M;
    using ptr_type = member_type class_type::*;
    constexpr Bind(StaticString k, ptr_type v) :
        key(k), 
        value(v)
    { }
    StaticString key;
    ptr_type value;
};

// for binding strings to member variables
template<typename T, typename M>
struct BindInit {
    using class_type = T;
    using member_type = M;
    using init_type = typename M::array_type;
    using ptr_type = member_type class_type::*;
    constexpr BindInit(StaticString k, ptr_type v, init_type x) :
        key(k), 
        value(v),
        init(x)
    { }
    StaticString key;
    ptr_type value;
    init_type init;
};

template<typename T>
struct has_name {
    // weird SFINAE "Substitution Failure Is Not An Error" buisness
    template <typename C> static char test_func(decltype(&TypeMeta<C>::name)); // comment 1
    template <typename C> static long test_func(...); // comment 2
    static constexpr bool value = sizeof(test_func<T>(0)) == sizeof(char);
};

template<typename T>
struct has_tuple {
    // weird SFINAE "Substitution Failure Is Not An Error" buisness
    template <typename C> static char test_func(decltype(&TypeMeta<C>::tuple)); // comment 1
    template <typename C> static long test_func(...); // comment 2
    static constexpr bool value = sizeof(test_func<T>(0)) == sizeof(char);
};

template<typename T>
struct EnumList {
    struct Entry {
        const char* name;
        typename T::Enum value;
    };
    using type = std::array<Entry, T::_Size>;
    type list;
    Entry* begin() { return &list[0]; }
    Entry* end() { return &list[T::_Size]; }
    EnumList(std::initializer_list<Entry> il) {
        assert(il.size() == T::_Size);
        size_t index = 0;
        for (auto&& x : il) {
            list[index++] = x;
        }
    }
};


// magic function to make things!
template<typename T, size_t ...Is, size_t ...Js, size_t ...Ks, size_t ...Ls>
inline constexpr T make_filters_impl(std::index_sequence<Is...>, std::index_sequence<Js...>, std::index_sequence<Ks...>, std::index_sequence<Ls...>) {
    return {
        typename std::tuple_element_t<Is, decltype(T::components)>::member_type(std::get<Is>(T::components).init)...,
        typename std::tuple_element_t<Js, decltype(T::filters)>::member_type(std::get<Js>(T::filters).init)...,
        typename std::tuple_element_t<Ks, decltype(T::architypes)>::member_type(Ks + 1, std::get<Ks>(T::architypes).init)...,
        typename std::tuple_element_t<Ls, decltype(T::systems)>::member_type(Ls)...
    };
}
template<typename T>
inline constexpr T make_filters() {
    return make_filters_impl<T>(
        std::make_index_sequence<std::tuple_size<decltype(T::components)>::value>{},
        std::make_index_sequence<std::tuple_size<decltype(T::filters)>::value>{},
        std::make_index_sequence<std::tuple_size<decltype(T::architypes)>::value>{},
        std::make_index_sequence<std::tuple_size<decltype(T::systems)>::value>{}
    );
}

#endif // TYPEMETA_TYPEMETA_HPP