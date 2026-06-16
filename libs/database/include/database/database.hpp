#ifndef DATABASE_DATABASE_HPP
#define DATABASE_DATABASE_HPP


#include <map>
#include <tuple>
#include <memory>
#include <vector>
#include <cstdarg>
#include <cassert>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <variant>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <emscripten/val.h>
#include <typemeta/typemeta.hpp>

// debug dumping
inline void _debug_dump(emscripten::val value) { emscripten::val::global("console").call<void>("log", value); }


// forward declare for function pointers
class Game;
class Interface;
class BaseComponent;

struct DataContext {
    const char* key;
    Game& game;
};

// virtual function table for Components
struct VirtualBaseComponent {
    using MakeFnPtr = std::shared_ptr<BaseComponent>(*)();
    using SetFnPtr = void(*)(BaseComponent& c, uint32_t i, emscripten::val v, DataContext& context);
    using GetFnPtr = emscripten::val(*)(BaseComponent& c, uint32_t i, DataContext& context);

    MakeFnPtr make;
    SetFnPtr set;
    GetFnPtr get;
};

struct VirtualInterface {
    using MakeFnPtr = std::shared_ptr<Interface>(*)(Game& game);
    using SetFnPtr = void(*)(Interface& c, emscripten::val v, DataContext& context);
    using GetFnPtr = emscripten::val(*)(Interface& c, DataContext& context);

    MakeFnPtr make;
    SetFnPtr set;
    GetFnPtr get;
};



struct TypeMetaData {
    static std::vector<TypeMetaData>& getVector() {
        static std::vector<TypeMetaData> v;
        return v;
    }
    static std::map<StaticString, size_t>& getMap() {
        static std::map<StaticString, size_t> m;
        return m;
    }

    const char* name;
    size_t hash_code;
    size_t size;
    VirtualBaseComponent component;
    VirtualInterface interface;
};


// this will become DynamicTypeMeta
template<typename T>
struct Register {
    static uint32_t register_type();
    static const uint32_t hash_code;
};

template<typename T>
inline TypeMetaData& type_meta() {
    return TypeMetaData::getVector()[Register<T>::hash_code];
}

inline TypeMetaData& type_meta(size_t i) {
    auto& v = TypeMetaData::getVector();
    assert(i < v.size());
    return v[i];
}

inline TypeMetaData& type_meta(const char* name) {
    auto& m = TypeMetaData::getMap();
    auto it = m.find(name);
    assert(it != m.end());
    return type_meta(it->second);
}





// the main handle class for accessing entitiy data
class Handle {
public:
    Handle() { value.raw = 0x00000000; }
    Handle(uint32_t r) { value.raw = r; }
    Handle(uint32_t t, uint32_t g, uint32_t i) { 
        value.info.type = t;
        value.info.generation = g;
        value.info.index = i;
    }
    Handle(uint32_t t, uint32_t i) { 
        value.ngInfo.type = t;
        value.ngInfo.index = i;
    }

    bool operator< (const Handle& rhs) const { return value.raw <  rhs.value.raw; }
    bool operator==(const Handle& rhs) const { return value.raw == rhs.value.raw; }
    bool operator!=(const Handle& rhs) const { return value.raw != rhs.value.raw; }

	bool isNull() const {
		return value.raw == 0x00000000;
	}

    uint8_t type() const { 
        return value.info.type;
    }

    uint16_t index() const {
        return value.info.index;
    }

    uint16_t generation() const {
        return value.info.generation;
    }

    uint32_t ngIndex() const {
        return value.ngInfo.index;
    }

    uint32_t raw() const {
        return value.raw;
    }

private:
    union {
        struct {
            uint32_t index : 24;
            uint32_t type : 8;
        } ngInfo;
        struct {
            uint32_t index : 14;
            uint32_t generation : 10;
            uint32_t type : 8;
        } info;
        uint32_t raw;
    } value;

};




//  A struct that stores a generation value, and some flags with the left over bits
struct Generation {
    Generation() {
        generation = 0;
        flags = 0;
    }
    Generation(uint16_t g) {
        generation = g;
        flags = 0;
    }
    uint16_t generation : 10;
    uint16_t flags : 6;
};



// uses type erase object so component types can be dynamic
// base class for all component arrays
class BaseComponent {
public:
    // this hash allows us to check to make sure this class can be statically casted
    const uint32_t hash;

    BaseComponent(uint32_t h) : hash(h) { }

    virtual ~BaseComponent() = default;

    virtual size_t coreSize() const = 0;

    virtual void coreResize(size_t) = 0;

    virtual void coreTransfer(size_t, size_t, std::shared_ptr<BaseComponent>) = 0;
    
    virtual void coreCopy(std::shared_ptr<BaseComponent>) = 0;

    virtual void* coreVoidPtr(size_t) = 0;
    
};


// all derived classes
template<typename T>
class DerivedComponent : public BaseComponent {
public:
    using type = T;

    std::vector<T> data;

    DerivedComponent() : BaseComponent(Register<T>::hash_code) { }

    size_t coreSize() const {
        return data.size();
    }

    void coreResize(size_t s) {
        if (data.size() < s) {
            data.resize(s);
        }
    }

    void coreTransfer(size_t dstIndex, size_t srcIndex, std::shared_ptr<BaseComponent> other) {
        // ! tried to transfer to an out of bounds dstIndex
        assert(dstIndex < data.size());
        if (other != nullptr && hash == other->hash) {
            auto src = std::static_pointer_cast<DerivedComponent<T>>(other);
            // ! tried to transfer from an out of bounds srcIndex
            assert(srcIndex < src->data.size());
            data[dstIndex] = src->data[srcIndex];
        }
    }

    void coreCopy(std::shared_ptr<BaseComponent> other) {
        // will copy entire array
    }

    void* coreVoidPtr(size_t i) {
        assert(i < data.size());
        return &(data[i]);
    }

};




// Filter system

struct BaseFilter {
    static constexpr bool isArchitype = false;
    struct reference {
        static constexpr bool has_extra = true;
        uint32_t type;
        uint32_t index;
        Generation* generation;
        Handle handle() const { return Handle(type, generation->generation, index); }
        void activate() { generation->flags |= 0x1; }
        void deactivate() { generation->flags &= ~0x1; }
    };
    uint16_t index;
    bool isArchitypeVal() const { return index != 0; }
    operator uint16_t() const { return index; }
    BaseFilter() { }
    constexpr BaseFilter(uint16_t i) : index(i) { }
};


template<typename ...Ts>
struct Filter : public BaseFilter {

    using base = Filter<Ts...>;
    using tup = std::tuple<Ts...>;
    using tup_ref = std::tuple<Ts&...>;
    using tup_ptr = std::tuple<Ts*...>;
    using vec_tup = std::tuple<std::vector<Ts>*...>;
    using first = typename std::tuple_element<0, tup>::type;

    static constexpr size_t size_value = std::tuple_size<tup>::value;
        
    using seq = std::make_index_sequence<size_value>;
    using array_type = std::array<uint16_t, size_value>;

    struct derive : public BaseFilter::reference {
        using seq = seq;
        struct cref : public BaseFilter::reference {
            tup_ref tuple;
        };
        tup_ptr tuple;
    };

    struct reference : public derive { 
        static constexpr bool has_extra = false;
    };

    array_type list;

    Filter() { }
    constexpr Filter(const array_type& a) : BaseFilter(0), list(a) { }
    constexpr Filter(uint16_t i, const array_type& a) : BaseFilter(i), list(a) { }
    
    void set(const Filter<Ts>&... t) {
        list = { t.list[0]... };
    }

    uint16_t get(size_t i) const { return list[i]; }
};


template<typename T>
struct Filter<T> : public BaseFilter {

    using base = Filter<T>;
    using tup = std::tuple<T>;
    using tup_ref = std::tuple<T&>;
    using tup_ptr = std::tuple<T*>;
    using vec_tup = std::tuple<std::vector<T>*>;
    using first = typename std::tuple_element<0, tup>::type;

    static constexpr size_t size_value = std::tuple_size<tup>::value;
        
    using seq = std::make_index_sequence<size_value>;
    using array_type = std::array<uint16_t, size_value>;

    struct derive : public BaseFilter::reference {
        using seq = seq;
        struct cref : public BaseFilter::reference {
            tup_ref tuple;
        };
        tup_ptr tuple;
    };

    struct reference : public derive {
        static constexpr bool has_extra = false;
        struct cref : public derive::cref {
            T& value;
        };
        T* value;
    };

    array_type list;

    Filter() { }

    constexpr Filter(const array_type& a) : BaseFilter(0), list(a) { }
    constexpr Filter(uint16_t i, const array_type& a) : BaseFilter(i), list(a) { }

    constexpr Filter(uint16_t a) : BaseFilter(0), list({a}) { }
    constexpr Filter(uint16_t i, uint16_t a) : BaseFilter(i), list({a}) { }

    void set(const Filter<T>& t) {
        list = { t.list[0] };
    }

    uint16_t get(size_t i) const { return list[i]; }
};


template<typename ...Ts>
struct Architype : public Filter<Ts...> {
    using base = Architype<Ts...>;
    using array_type = typename Filter<Ts...>::array_type;

    static constexpr bool isArchitype = true;

    constexpr Architype() { }
    constexpr Architype(uint16_t i, const array_type& a) : Filter<Ts...>(i, a) { }
};

template<typename T>
struct Architype<T> : public Filter<T> {
    using base = Architype<T>;
    using array_type = typename Filter<T>::array_type;

    static constexpr bool isArchitype = true;

    constexpr Architype() { }
    constexpr Architype(uint16_t i, const array_type& a) : Filter<T>(i, a) { }
    constexpr Architype(uint16_t i, uint16_t a) : Filter<T>(i, a) { }
};



// function to correctly make a reference whether its a generic filter or custom filter
template<typename U>
struct MakeReferenceImpl {
    using type = U;
    template<size_t ...Is>
    static typename type::reference invoke_impl(size_t type, size_t index, Generation* generation, typename U::tup_ptr& tuple, std::index_sequence<Is...>) {
        return typename type::reference{ { { type, index, generation }, tuple }, std::get<Is>(tuple)...};
    }
    static typename type::reference invoke(size_t type, size_t index, Generation* generation, typename U::tup_ptr& tuple) {
        return invoke_impl(type, index, generation, tuple, typename type::seq{});
    }
};

template<typename ...Us>
struct MakeReferenceImpl<Filter<Us...>> {
    using type = Filter<Us...>;
    static typename type::reference invoke(size_t type, size_t index, Generation* generation, typename type::tup_ptr& tuple) {
        if constexpr (type::size_value == 1) {
            return typename type::reference{ { { type, index, generation }, tuple }, std::get<0>(tuple)};
        } else {
            return typename type::reference{ { { type, index, generation }, tuple } };
        }
    }
};

template<typename ...Us>
struct MakeReferenceImpl<Architype<Us...>> {
    using type = Architype<Us...>;
    static typename type::reference invoke(size_t type, size_t index, Generation* generation, typename type::tup_ptr& tuple) {
        if constexpr (type::size_value == 1) {
            return typename type::reference{ { { type, index, generation }, tuple }, std::get<0>(tuple) };
        } else {
            return typename type::reference{ { { type, index, generation }, tuple } };
        }
    }
};

template<typename U>
typename U::reference make_reference(size_t type, size_t index, Generation* generation, typename U::tup_ptr& tuple) {
    return MakeReferenceImpl<U>::invoke(type, index, generation, tuple);
}

template<typename T, size_t ...Is>
typename T::cref ref_impl(T& ref, std::index_sequence<Is...>) {
    if constexpr (std::index_sequence<Is...>::size() == 1) {
        //if constexpr (T::has_extra) {
            return typename T::cref{ { { ref.type, ref.index, ref.generation }, *std::get<0>(ref.tuple) }, *std::get<0>(ref.tuple) };
        //} else {
        //    return typename T::cref{ { ref.type, ref.index, ref.generation }, *std::get<0>(ref.tuple) };
        //}
    } else {
        if constexpr (T::has_extra) {
            return typename T::cref{ { { ref.type, ref.index, ref.generation }, { *std::get<Is>(ref.tuple)... } }, *std::get<Is>(ref.tuple)... };
        } else {
            return typename T::cref{ { ref.type, ref.index, ref.generation }, { *std::get<Is>(ref.tuple)... } };
        }
    }
}

template<typename T>
typename T::cref ref(T& ref) {
    return ref_impl( ref, typename T::seq{} );
}

template<typename T>
struct is_filter {
    static constexpr bool value = std::is_base_of<BaseFilter, T>::value;
};

template<typename T>
struct is_architype {
    static constexpr bool compute() {
        if constexpr (std::is_base_of<BaseFilter, T>::value) {
            return T::isArchitype;
        } else {
            return false;
        }
    }
    static constexpr bool value = compute();
};



template<typename T>
class Container;


class Components {
public:
    // struct for holding meta data for each architype
    static const uint8_t FlagActive = 0x1;
    static const uint16_t EndOfList = 0xffff;
    struct ArchitypeInfo {
        ArchitypeInfo() : size(0), head(EndOfList) { }
        bool tracks;
        std::string name;
        size_t size;
        uint16_t head;
        std::vector<uint16_t> free;
        std::vector<uint16_t> components;
    };

    // struct for holding meta data for each component
    struct ComponentInfo {
        ComponentInfo() : name(nullptr), hash(0) { }
        std::string name;
        std::string type;
        uint32_t hash;
    };

    // architype meta data
    std::vector<ArchitypeInfo> ainfo;

    // component types
    std::vector<ComponentInfo> cinfo;

    // for keeping track of generations
    std::vector<std::vector<Generation>> generation;

    // static components
    std::vector<std::vector<std::shared_ptr<BaseComponent>>> components;

    // name maps
    std::map<StaticString, size_t> aMap;
    std::map<StaticString, size_t> cMap;

    size_t indexOfAr(const char* name) const { 
        auto it = aMap.find(name);
        assert(it != aMap.end());
        return it->second;
    }
    size_t indexOfCp(const char* name) const {
        auto it = cMap.find(name);
        assert(it != cMap.end());
        return it->second;
    }

    const std::string& nameOfAr(size_t index) const { return ainfo[index].name; }
    const std::string& nameOfCp(size_t index) const { return cinfo[index].name; }

    void completeReset() {
        ainfo.clear();
        cinfo.clear();
        generation.clear();
        components.clear();
        aMap.clear();
        cMap.clear();
    }

    // register component 
    // name: data type name
    // id: component id
    void registerComponent(uint16_t index, const char* name, const char* type) {
        // make sure component data is large enough
        if (index >= cinfo.size()) {
            components.resize(index + 1);
            cinfo.resize(index + 1);
        }

        // ! assert if id is already defined
        assert(cinfo[index].hash == 0);
        // store component metadata

        TypeMetaData typeMeta = type_meta(type);
        cinfo[index].hash = typeMeta.hash_code;
        cinfo[index].type = typeMeta.name;
        cinfo[index].name = name;

        // rebuild name map
        cMap.clear();
        for (size_t i = 0; i < cinfo.size(); ++i) {
            cMap[cinfo[i].name.c_str()] = i;
        }
    }

    void registerArchitype(uint16_t index, const char* name) {        
        // ! architypes can't use id 0
        assert(index != 0);
        
        // make architype arrays big enough
        if ( index >= ainfo.size() ) {
            ainfo.resize(index + 1);
            generation.resize(index + 1);
            for (auto&& x : components) {
                x.resize(index + 1);
            }
        }

        ainfo[index].name = name;

        // rebuild name map
        aMap.clear();
        for (size_t i = 0; i < ainfo.size(); ++i) {
            aMap[ainfo[i].name.c_str()] = i;
        }
    }

    void addComponent(uint16_t architype, uint16_t component) {
        // ! architypes can't use id 0
        assert(architype != 0);

        // ! architypes can't use id 0
        assert(architype < ainfo.size());

        // architype component list
        auto& list = ainfo[architype].components;
         
        // ! make sure id is in range
        assert(component < components.size());
        list.push_back(component);
        components[component][architype] = type_meta(cinfo[component].type.c_str()).component.make();

        std::sort(list.begin(), list.end());
    }
  
    // does an architype have a component?
    bool has(uint16_t architype, uint16_t id) const {
        // ! the architype id was invalid or out of bounds
        assert(architype < ainfo.size());
        //const std::vector<uint16_t>& list = ainfo[architype].components;
        //return std::find(list.begin(), list.end(), id) != list.end();
        return components[id][architype] != nullptr;
    }

    bool valid(Handle h) const {
        if (h.type() >= generation.size() || h.index() >= generation[h.type()].size()) {
            return false;
        }
        return h.generation() == generation[h.type()][h.index()].generation;
    }

    // helper function for casting component data to vector
    template<typename T>
    std::vector<T>& safe_component_cast(uint16_t cp, uint16_t ar) {
         // ! component id must be in range
        assert(cp < components.size());
        // ! make sure the types match
        assert(Register<T>::hash_code == cinfo[cp].hash);        
        return std::static_pointer_cast<DerivedComponent<T>>(components[cp][ar])->data;
    }    

    // get a handle's component data
    // T: type of component
    // c: id of component to retrieve
    // h: handle to retrieve
    // return: reference to component data
    template<typename T>
    typename T::first& get(const T& t, Handle h) {
        // ! the handle must be valid to get the entity data
        assert(valid(h)); 
        return safe_component_cast<typename T::first>(t.get(0), h.type())[h.index()];
    }

    // create handle to data
    // index: architype index
    // return: handle to newly created object
    Handle createIndex(uint16_t index) {
        // ! architype 0 is invalid
        assert(index != 0);
        // ! check that architype is valid
        assert(index < ainfo.size());
        ArchitypeInfo& in = ainfo[index];
        std::vector<Generation>& ex = generation[index];
        Handle h;
        if (in.head == EndOfList) {
            h = Handle(index, 0, in.size);
            ++in.size;
            if (ex.size() < in.size) {
                in.free.resize(in.size);
                ex.resize(in.size);
                for (auto&& x : in.components) {
                    components[x][index]->coreResize(in.size);
                }
            }
            in.free[h.index()] = EndOfList;
            ex[h.index()] = Generation();
        } else {
            h = Handle(index, ex[in.head].generation, in.head);
            in.head = in.free[h.index()];
            in.free[h.index()] = EndOfList;
        }
        return h;
    }

    template <typename T>
    typename T::reference create(const T& filter) {
        return ref(filter, createIndex(filter.index));
    }
    
    // transfer all components from one handle to another
    // dst: destination handle
    // src: source handle
    void transfer(Handle dst, Handle src) {
        // ! both handles must be valid to transfer data
        assert(valid(dst));
        assert(valid(src));
        for (auto&& x : ainfo[dst.type()].components) {
            components[x][dst.type()]->coreTransfer(dst.index(), src.index(), components[x][src.type()]);
        }
    }

    // clones a new handle from an existing one (good for prefabs)
    // handle: existing handle
    Handle clone(Handle h) {
        // ! handle must be valid to clone
        assert(valid(h));
        Handle n = createIndex(h.type());
        transfer(n, h);
        return n;
    }

    void destroy(Handle h) {
        // trying to destroy and invalid handle is a noop
        if (valid(h)) {
            ArchitypeInfo& in = ainfo[h.type()];
            std::vector<Generation>& ex = generation[h.type()];
            if (in.free[h.index()] == EndOfList) {
                ++ex[h.index()].generation;
                ex[h.index()].flags = 0x0;
                in.free[h.index()] = in.head;
                in.head = h.index();
            }
        }
    }

    void clearArchitype(uint16_t architype) {
        generation[architype].clear();
        ainfo[architype].free.clear();
        ainfo[architype].size = 0;
        ainfo[architype].head = EndOfList;
    }

    void clearAllArchitype() {
        for (size_t i = 1; i < ainfo.size(); ++i) {
            clearArchitype(i);
        }
    }

    // this will force create an object at a specific handle
    // Warning!  this will screw up the free list!  must refresh it after
    // making a number of calls to this function!
    void forceCreate(Handle h) {
        uint16_t index = h.type();
        // ! architype 0 is invalid
        assert(index != 0);
        // ! check that architype is valid
        assert(index < ainfo.size());
        ArchitypeInfo& in = ainfo[index];
        std::vector<Generation>& ex = generation[index];
        // make lists bigger if not big enough
        if (h.index() >= in.size) {
            in.size = h.index() + 1;
            in.free.resize(in.size, 0);
            ex.resize(in.size);
            for (auto&& x : in.components) {
                components[x][index]->coreResize(in.size);
            }
        }
        in.free[h.index()] = EndOfList;
        ex[h.index()].generation = h.generation();
        ex[h.index()].flags = 0x0;
    }

    void refreshFreeList() {
        for (auto&& info : ainfo) {
            info.head = EndOfList;
            size_t last = EndOfList;
            for (size_t i = 0; i < info.free.size(); ++i) {
                if (info.free[i] != EndOfList) {
                    if (info.head == EndOfList) {
                        info.head = info.free[i];
                    }
                    if (last != EndOfList) {
                        info.free[last] = i;
                    }
                    last = info.free[i];
                }
            }
        }
    }

    Generation& meta(Handle h) {
        // ! handle values must be in range to get the generation and flag data
        assert(h.type() < generation.size());
        assert(h.index() < generation[h.type()].size());
        return generation[h.type()][h.index()];
    }

    Handle getHandle(uint16_t a, uint16_t i) {
         // ! handle values must be in range
        assert(a < generation.size());
        assert(i < generation[a].size());
        return Handle(a, generation[a][i].generation, i);
    }


    template <typename T, std::size_t... Is>    
    typename T::tup_ptr ref_impl(Handle h, const T& filter, std::index_sequence<Is...>) {
        return std::make_tuple( &(safe_component_cast<typename std::tuple_element<Is, typename T::tup>::type>(filter.get(Is), h.type())[h.index()])... );
    }

    template <typename T>
    typename T::reference ref(const T& filter, Handle h) {
        assert(h.type() < generation.size());
        assert(h.index() < generation[h.type()].size());
        auto r = ref_impl(h, filter, typename T::seq{});
        return make_reference<T>(h.type(), h.index(), &(generation[h.type()][h.index()]), r);
    }

  

    template <typename T>
    bool matchArchitype(const T& filter, uint16_t architype) const {
        // zero is always false because its always empty
        if (architype == 0) {
            return false;
        }
        // ! architype index must be in range
        assert(architype < ainfo.size());
        bool hasAll = true;
        for (auto& x : filter.list) {
            if (has(architype, x) == false) {
                hasAll = false;
            }
        }
        return hasAll;
    }

    template <typename T>
    Container<T> iterate(const T& f, uint8_t flags = FlagActive) {
        return Container(f, flags, *this);
    }


    template<typename T>
    void InitializeAll(const T& f) {

        // first clear out the database completely
        completeReset();

        // first register all components
        std::apply([&](auto&&... args) {
            auto process_item = [&](auto&& item) {
                using U = typename std::remove_reference<decltype(item)>::type::member_type;
                if constexpr (is_filter<U>::value) {
                    if constexpr (U::size_value == 1) {
                        // this is a component, so register it
                        registerComponent( (f.*(item.value)).list[0], item.key.c_str(), TypeMeta<typename U::first>::name );
                    }
                } else {
                    // do nothing...
                }        
            };
            // Left fold with comma operator
            (process_item(args), ...); 
        }, T::components);

        // register all architypes and add all components to them
        std::apply([&](auto&&... args) {
            auto process_item = [&](auto&& item) {
                using U = typename std::remove_reference<decltype(item)>::type::member_type;
                if constexpr (is_architype<U>::value) {
                    const U& r = f.*(item.value);
                    // this is a architype, so register it
                    registerArchitype( r.index, item.key.c_str() );
                    for (uint32_t i = 0; i < r.list.size(); ++i) {
                        addComponent( r.index, r.list[i] );
                    }
                } else {
                    // do nothing...
                }        
            };
            // Left fold with comma operator
            (process_item(args), ...); 
        }, T::architypes);

    }



private:


};




struct BaseSystemFilter {
    size_t value;
    BaseSystemFilter() { }
    constexpr BaseSystemFilter(size_t v) : value(v) { }
};


template<typename T>
struct SystemFilter : public BaseSystemFilter {
    using BaseSystemFilter::BaseSystemFilter;
    using value_type = T;
};


class Interface {
public:
    using base = Interface;
    
    Interface(Game& g) : game(g) { }

    void setup() { }

    Game& game;
};



class Game {
public:

    struct Entry {
        bool isNull() const { return ptr == nullptr; };
        uint32_t hash;
        std::string name;
        std::shared_ptr<Interface> ptr;
    };

    void insert(size_t index, const char* name, const char* type) {
        if (interfaces.size() <= index) {
            interfaces.resize(index + 1);
        }
        assert(interfaces[index].isNull());
        auto& typeMeta = type_meta(type);
        interfaces[index] = { typeMeta.hash_code, name, typeMeta.interface.make(*this) };
        rebuildNameMap();
    }

    void rebuildNameMap() {
        // needs to be rebuild because vector resize will invalidate the StaticStrings!
        names.clear();
        for (size_t i = 0; i < interfaces.size(); ++i) {
            names[interfaces[i].name.c_str()] = i;            
        }
    }

    Interface& operator[](size_t index) {
        return *(interfaces[index].ptr);
    }

    Interface& operator[](const char* name) {
        return *(interfaces[names[name]].ptr);
    }

    template<typename T>
    typename T::value_type& operator[](const T& t) {
        assert(interfaces[t.value].ptr != nullptr);
        assert(Register<typename T::value_type>::hash_code == interfaces[t.value].hash);
        return *(std::static_pointer_cast<typename T::value_type>(interfaces[t.value].ptr));
    }

    size_t indexOf(const char* name) const {
        return names.find(name)->second;
    }

    const std::string& nameOf(size_t index) const {
        return interfaces[index].name;
    }

    Components db;
    std::vector<Entry> interfaces;
    std::map<StaticString, size_t> names;
};




// Iterator here!
template<typename T>
class Iterator {
public:

    bool ended;

    typename T::vec_tup current;
    Components* data;
    const T* filter;
    uint8_t flags;

    uint16_t type;                                // tracks where we are in the architypes
    uint16_t index;                               // tracks where we are in the current architype

    Iterator() : ended(true) { };
    Iterator(const T& f, uint8_t fl, Components& d) :  
        ended(false),
        data(&d),
        filter(&f),
        flags(fl),
        type(0),
        index(0)
    {
        incerment();
    }

    bool atEnd() const {
        return ended;
    }

    template <std::size_t... Is>    
    void incerment_impl(std::index_sequence<Is...>) {
        ((std::get<Is>(current) = &(data->safe_component_cast<typename std::tuple_element<Is, typename T::tup>::type>(filter->get(Is), type))), ...);
    }
        
    // incerments iterator to next valid active handle
    void incerment()  {
        for (;;) {
            ++index;
            if (index >= data->ainfo[type].size) {
                for (;;) {
                    ++type;
                    if (type >= data->ainfo.size()) {
                        ended = true;
                        break;
                    }
                    index = 0;
                    if (index < data->ainfo[type].size && data->matchArchitype(*filter, type) == true) {
                        incerment_impl(typename T::seq{});
                        break;
                    }
                }
            }
            if (ended == true) {
                break;
            }
            if ((data->generation[type][index].flags & flags) == flags) {
                break;
            }
        }
    }
    
    // operator to incerment iterator
    Iterator& operator++() {
        incerment();
        return (*this);
    }

    template <std::size_t... Is>    
    typename T::tup_ptr reference(std::index_sequence<Is...>) {
        return std::make_tuple( &((*(std::get<Is>(current)))[index])... );
    }
        
    // returns a reference pack object for this iterator
    typename T::reference operator*() {
        assert(type < data->generation.size());
        assert(index < data->generation[type].size());
        auto r = reference(typename T::seq{});
        return make_reference<T>(type, index, &(data->generation[type][index]), r);
    }

    friend bool operator!= (const Iterator& a, const Iterator& b) { return a.ended != b.ended; };

    // these next two functions are used to fill in a reference pack object
    // returns the handle of the current componet
    Handle handle() const  {
        // ! handle values must be in range
        assert(type < data->generation.size());
        assert(index < data->generation[type].size());
        return Handle(type, data->generation[type][index].generation, index);
    }
};


// iterator to allow us to use the for (auto x : container) syntax
template<typename T>
class Container {
public:
    Container(const T& f, uint8_t fl, Components& d) : filter(f), flags(fl), data(d) { }
    uint8_t flags;
    const T& filter;
    Components& data;
    Iterator<T> begin() { return Iterator<T>(filter, flags, data); }
    Iterator<T> end() { return Iterator<T>(); }
};




/***************************

  Function Implementations

**************************/

// define register's static variable
template<typename T>
inline const uint32_t Register<T>::hash_code = Register<T>::register_type();


// specilize this class to bind string names to objects

template<typename T, typename Enable = void>
struct GetSet { };

// type traits
template<typename T>
struct is_primitive {
    static constexpr bool value = std::is_integral<T>::value || std::is_same<T, bool>::value;
};

template<typename T>
struct is_database_data {
    static constexpr bool value = std::is_same<T, bool>::value == false && std::is_base_of<Interface, T>::value == false;
};

template<typename T>
struct is_system_filter {
    static constexpr bool value = std::is_base_of<BaseSystemFilter, T>::value;
};

template<typename T>
struct is_system {
    static constexpr bool value = std::is_base_of<Interface, T>::value;
};


template<typename T>
struct has_get {
    // weird SFINAE "Substitution Failure Is Not An Error" buisness
    template <typename C> static char test_func(decltype(&GetSet<C>::get)); // comment 1
    template <typename C> static long test_func(...); // comment 2
    static constexpr bool value = sizeof(test_func<T>(0)) == sizeof(char);
};

template<typename T>
struct has_set {
    // weird SFINAE "Substitution Failure Is Not An Error" buisness
    template <typename C> static char test_func(decltype(&GetSet<C>::set)); // comment 1
    template <typename C> static long test_func(...); // comment 2
    static constexpr bool value = sizeof(test_func<T>(0)) == sizeof(char);
};

template<typename T>
struct has_default {
    // weird SFINAE "Substitution Failure Is Not An Error" buisness
    template <typename C> static char test_func(decltype(&GetSet<C>::def)); // comment 1
    template <typename C> static long test_func(...); // comment 2
    static constexpr bool value = sizeof(test_func<T>(0)) == sizeof(char);
};


template<typename T>
inline void type_meta_set(T& t, emscripten::val value, DataContext& context) {
    if (!value.isUndefined()) {
        if constexpr (has_tuple<T>::value) {
            if constexpr (std::tuple_size<decltype(TypeMeta<T>::tuple)>::value == 1 && is_system<T>::value == false) {
                // only a single member! don't nest a whole object for it!
                auto&& item = std::get<0>(TypeMeta<T>::tuple);
                context.key = item.key.c_str();
                type_meta_set(t.*(item.value), value, context);
            } else {
                // paste tuple set from here
                std::apply([&](auto&&... args) {
                    auto process_item = [&](auto&& item) {
                        context.key = item.key.c_str();
                        type_meta_set(t.*(item.value), value[item.key.c_str()], context);
                    };
                    // Left fold with comma operator
                    (process_item(args), ...); 
                }, TypeMeta<T>::tuple);
            }
        } else {
            if constexpr (has_set<T>::value) {
                GetSet<T>::set(t, value, context);
            } else if constexpr (is_primitive<T>::value) {
                t = value.as<T>();
            } else {
                // do nothing, this is an empty type
            }
        }
    } else {
        if constexpr (has_default<T>::value) {
            GetSet<T>::def(t, context);
        }
    }
}

template<typename T>
inline emscripten::val type_meta_get(T& t, DataContext& context) { 
    if constexpr (has_tuple<T>::value) {
        if constexpr (std::tuple_size<decltype(TypeMeta<T>::tuple)>::value == 1 && is_system<T>::value == false) {
            // only a single member! don't nest a whole object for it!
            auto&& item = std::get<0>(TypeMeta<T>::tuple);
            context.key = item.key.c_str();
            return type_meta_get(t.*(item.value), context);
        } else {
            emscripten::val v = emscripten::val::object();
            // paste tuple set from here
            std::apply([&](auto&&... args) {
                auto process_item = [&](auto&& item) {
                    using U = typename std::remove_reference<decltype(item)>::type::member_type;
                    context.key = item.key.c_str();
                    emscripten::val ret = type_meta_get(t.*(item.value), context);
                    if (!ret.isUndefined()) {
                        v.set(item.key.c_str(), ret);
                    }
                };
                // Left fold with comma operator
                (process_item(args), ...); 
            }, TypeMeta<T>::tuple);
            return v;
        }
    } else {
        if constexpr (has_set<T>::value) {
            return GetSet<T>::get(t, context);
        } else if constexpr (is_primitive<T>::value) {
            return emscripten::val(t);
        } else {
            // return empty object
            return emscripten::val::object();
        }
    }
}


// load the game!
struct KeyWrapper {
    KeyWrapper(emscripten::val l) : 
        list(emscripten::val::global("Object").call<emscripten::val>("keys", l)),
        _size(list["length"].as<size_t>())
    { }
    void get(size_t i, std::string& out) { out = list[i].as<std::string>(); }
    size_t size() const { return _size; }
    emscripten::val list;
    size_t _size;
};

template<typename T>
inline void load_game(Game& game, const T& filters, emscripten::val data) {
    DataContext context = { nullptr, game };

    emscripten::val dataList = data["database"];
    emscripten::val interfaces = data["systems"];

    std::string key;

    // create all interfaces from filter object
    size_t index = 0;
    std::apply([&](auto&&... args) {
        auto process_item = [&](auto&& item) {
            using U = typename std::remove_reference<decltype(item)>::type::member_type;
            if constexpr (is_system_filter<U>::value) {
                const U& r = filters.*(item.value);
                game.insert(index, item.key.c_str(), TypeMeta<typename U::value_type>::name);               
                ++index;
            }
        };
        // Left fold with comma operator
        (process_item(args), ...); 
    }, T::systems);

    game.db.InitializeAll(filters);

    auto interfaceKeys = KeyWrapper(interfaces);
    for (size_t i = 0; i < interfaceKeys.size(); ++i) {
        interfaceKeys.get(i, key);
        Game::Entry& entry = game.interfaces[game.indexOf(key.c_str())];
        context.key = key.c_str();
        type_meta(entry.hash).interface.set(*(entry.ptr), interfaces[key], context);
    }

    if (!dataList.isUndefined()) {

        // now do the database!
        size_t length = dataList["length"].as<size_t>();
        for (size_t i = 0; i < length; ++i) {

            emscripten::val value = dataList[i];

            // convert handle
            Handle handle;
            
            type_meta_set(handle, value["handle"], context);

            game.db.forceCreate(handle);

            // get components
            emscripten::val cp = value["components"];

            auto cpKeys = KeyWrapper(cp);
            for (size_t i = 0; i < cpKeys.size(); ++i) {
                cpKeys.get(i, key);
                // get component id
                uint16_t id = game.db.indexOfCp(key.c_str());
                // dynamic loading of component
                //game.db.components[id][handle.type()]->coreSet(handle.index(), cp[key], context);

                // get the type meta of this type
                TypeMetaData& typeMeta = type_meta(game.db.cinfo[id].hash);

                // zero it out first
                std::memset(game.db.components[id][handle.type()]->coreVoidPtr(handle.index()), 0, typeMeta.size);

                // set the element to the json thing
                context.key = key.c_str();
                typeMeta.component.set(*(game.db.components[id][handle.type()]), handle.index(), cp[key], context);
            }

            // set it active!
            game.db.meta(handle).flags = 0x1;
        }

        game.db.refreshFreeList();

    }


}


inline emscripten::val save_game(Game& game) {
    DataContext context = { nullptr, game };

    emscripten::val root = emscripten::val::object();
    
    root.set("systems", emscripten::val::object());
    root.set("database", emscripten::val::array());

    emscripten::val systems = root["systems"];

    for (auto& in : game.interfaces) {
        context.key = in.name.c_str();
        systems.set(in.name, type_meta(in.hash).interface.get(*(in.ptr), context));
    }

    //emscripten::val database = root["database"];
    //emscripten::val dataList = database["data"];
    emscripten::val dataList = root["database"];

    for (size_t a = 1; a < game.db.ainfo.size(); ++a) {

        //printf("arch: %s\n", game.fr.architypeString(a));

        for (size_t h = 0; h < game.db.generation[a].size(); ++h) {

            emscripten::val object = emscripten::val::object();

            Handle handle = game.db.getHandle(a, h);

            if ((game.db.meta(handle).flags & Components::FlagActive) == Components::FlagActive) {

                object.set("handle", type_meta_get(handle, context));
                object.set("components", emscripten::val::object());

                for (size_t c = 0; c < game.db.ainfo[a].components.size(); ++c) {
                    size_t id = game.db.ainfo[a].components[c];
                    //printf("cp: %s\n", game.fr.componentString(id));
                    //value["components"].set( game.fr.componentString(id), game.cp.components[id][handle.type()]->coreGet(handle.index()) ); 
                    //object["components"].set(game.db.nameOfCp(id), game.db.components[id][handle.type()]->coreGet(handle.index(), context));
                
                    object["components"].set(game.db.nameOfCp(id), type_meta(game.db.cinfo[id].hash).component.get(*(game.db.components[id][handle.type()]), handle.index(), context));
                
                }

                dataList.call<void>("push", object);
            }
    
        }
        
    }

    return root;
}


// custom virtual function implementations
template<typename T>
struct VirtualBaseComponentFn {
    static std::shared_ptr<BaseComponent> make() {
        return std::make_shared<DerivedComponent<T>>();
    }
    static void set(BaseComponent& c, uint32_t i, emscripten::val v, DataContext& context) {
        type_meta_set(cast(c).data[i], v, context);
    }
    static emscripten::val get(BaseComponent& c, uint32_t i, DataContext& context) {
        return type_meta_get(cast(c).data[i], context);
    }
    static DerivedComponent<T>& cast(BaseComponent& c) {
        return *static_cast<DerivedComponent<T>*>(&c);
    }
    static VirtualBaseComponent init() {
        return { make, set, get };
    }
};

template<typename T>
struct VirtualInterfaceFn {
    static std::shared_ptr<Interface> make(Game& game) {
        return std::make_shared<T>(game);
    }
    static void set(Interface& c, emscripten::val v, DataContext& context) {
        type_meta_set(cast(c), v, context);
        cast(c).setup();
    }
    static emscripten::val get(Interface& c, DataContext& context) {
        emscripten::val value = type_meta_get(cast(c), context);
        value.set("_type", TypeMeta<T>::name);
        return value;
    }
    static T& cast(Interface& c) {
        return *static_cast<T*>(&c);
    }
    static VirtualInterface init() {
        return { make, set, get };
    }
};

// used to register types
template<typename T>
uint32_t Register<T>::register_type() {

    printf("registering %s...\n", TypeMeta<T>::name);

    // we never defined the name of this type!
    if (has_name<T>::value == false) {
        assert(true == false);
        return 0;
    }

    auto& vec = TypeMetaData::getVector();
    auto& mp = TypeMetaData::getMap();
    size_t index = vec.size();

    VirtualBaseComponent cp;
    VirtualInterface in;
    
    if (index == 0) {
        vec.push_back({
            "__NULL__", 
            0, 
            0, 
            cp,
            in
        });
        ++index;
    }

    if constexpr (is_database_data<T>::value) {
        cp = VirtualBaseComponentFn<T>::init();
    }

    if constexpr (is_system<T>::value) {
        in = VirtualInterfaceFn<T>::init();
    }

    vec.push_back({ 
        TypeMeta<T>::name, 
        index, 
        sizeof(T), 
        cp,
        in
    });

    mp.insert(std::make_pair(TypeMeta<T>::name, index));        
    
    return index;
}



/***************************

  Database Special Type Meta Data

**************************/


// defining type meta for primitives and handle structs
#define REGISTER_PRIMITIVE(t, s) template<> struct TypeMeta<t> { static constexpr auto name = s; }; template struct Register<t>;

REGISTER_PRIMITIVE(    bool,     "bool");
REGISTER_PRIMITIVE( uint8_t,  "uint8_t");
REGISTER_PRIMITIVE(uint16_t, "uint16_t");
REGISTER_PRIMITIVE(uint32_t, "uint32_t");
REGISTER_PRIMITIVE(uint64_t, "uint64_t");
REGISTER_PRIMITIVE(  int8_t,   "int8_t");
REGISTER_PRIMITIVE( int16_t,  "int16_t");
REGISTER_PRIMITIVE( int64_t,  "int64_t");
REGISTER_PRIMITIVE( int32_t,  "int32_t");


template<typename T>
struct GetSet<SystemFilter<T>> {
    static emscripten::val get(SystemFilter<T>& t, DataContext& context) {
        if (strcmp(context.game.nameOf(t.value).c_str(), context.key) == 0) {
            return emscripten::val::undefined();
        } else {
            return emscripten::val(context.game.nameOf(t.value));
        }
    }
    static void set(SystemFilter<T>& t, emscripten::val value, DataContext& context) { 
        t.value = context.game.indexOf(value.as<std::string>().c_str());
    }
    static void def(SystemFilter<T>& t, DataContext& context) {
        t.value = context.game.indexOf(context.key);
    }
};

// 3 special get/set overrides
template<typename T>
struct GetSet<T, std::enable_if_t<std::is_base_of_v<BaseFilter, T>>> {
    static emscripten::val get(T& t, DataContext& context) { 
        if constexpr (is_architype<T>::value) {
            emscripten::val value = emscripten::val::object();
            value.set("architype", context.game.db.nameOfAr(t.index));
            value.set("types", emscripten::val::array());
            for (auto& x : t.list) {
                value["types"].call<void>("push", context.game.db.nameOfCp(x)); // Add an integer
            }
            return value;
        } else {
            emscripten::val value = emscripten::val::array();
            for (auto& x : t.list) {
                value.call<void>("push", context.game.db.nameOfCp(x)); // Add an integer
            }
            return value;
        }    
    }
    static void set(T& t, emscripten::val value, DataContext& context) { 
        if constexpr (is_architype<T>::value) {
            if (!value["architype"].isUndefined()) {
                t.index = context.game.db.indexOfAr(value["architype"].as<std::string>().c_str());
            }
            if (!value["types"].isUndefined()) {
                size_t length = value["types"]["length"].as<size_t>();
                for (size_t i = 0; i < length; ++i) {   
                    t.list[i] = context.game.db.indexOfCp(value["types"][i].as<std::string>().c_str());          
                }
            }
       } else {
            size_t length = value["length"].as<size_t>();
            for (size_t i = 0; i < length; ++i) {   
                t.list[i] = context.game.db.indexOfCp(value[i].as<std::string>().c_str());          
            } 
        }
    }
};

template<>
struct TypeMeta<Handle> {
    static constexpr auto name = "Handle";
};
template<>
struct GetSet<Handle> {
    static emscripten::val get(Handle& t, DataContext& context) {
        emscripten::val value = emscripten::val::object();
        value.set("type", context.game.db.nameOfAr(t.type()));
        value.set("generation", t.generation());
        value.set("index", t.index());    
        return value;
    }
    static void set(Handle& t, emscripten::val value, DataContext& context) {
        uint32_t type = 0, generation = 0, index = 0;
        if (!value["type"].isUndefined()) {
            type = context.game.db.indexOfAr(value["type"].as<std::string>().c_str());
        }
        type_meta_set(generation, value["generation"], context);
        type_meta_set(index, value["index"], context);
        t = Handle(type, generation, index);
    }
};
template struct Register<Handle>;


#endif