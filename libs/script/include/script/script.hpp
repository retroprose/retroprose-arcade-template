#ifndef CORE_PROGRAM_PROGRAM_HPP
#define CORE_PROGRAM_PROGRAM_HPP

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <variant>
#include <cmath>

template <class T, class Tuple>
struct TupleIndex;

template <class T, class... Types>
struct TupleIndex<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct TupleIndex<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
};

struct NoopFunction { };

// this typename needs to be a variant
template<typename... Ts>
struct Function {
    uint8_t next;           // current state of the program
    uint16_t returnId;    // index of function to come back to when this pops
    std::variant<NoopFunction, Ts...> top;  // variant that holds what function is currently running
};

using coroutine_return_type = std::variant<int32_t, bool>;

// this typename needs to be a Function<O, Ts...>
template<typename... Ts>
struct Coroutine {
    using tuple_type = std::tuple<NoopFunction, Ts...>;
    using variant_type = std::variant<NoopFunction, Ts...>;
    using function_type = Function<Ts...>;
    
    Coroutine() {
        function.returnId = 0xffff;
        function.next = 0;
        function.top = NoopFunction();
    }

    void init(variant_type f, uint8_t s = 0) {
        function.returnId = 0xffff;
        function.next = s;
        function.top = f;
    }

    coroutine_return_type retValue;       // hold return values
    Function<Ts...> function;     // holds state of running function
};


// T is a Function<Ts...>
template<typename T>
class DataPooler {
public:
    static constexpr uint16_t EndOfList = 0xffff;
    using function_type = typename T::function_type;

    DataPooler() : head(EndOfList) { }

    uint16_t head;
    std::vector<uint16_t> free;
    std::vector<function_type> list; 

    uint16_t end() const {
        return EndOfList;
    }

    uint16_t store(function_type& v) {
        uint16_t index = EndOfList;
        if (head == EndOfList) {
            index = free.size();
            free.push_back(EndOfList);
            list.push_back(v);
        } else {
            index = head;
            head = free[index];
            free[index] = EndOfList;
            list[index] = v;
        }
        return index;
    }

    function_type retrive(uint16_t index) {
        assert(free[index] == EndOfList);
        free[index] = head;
        head = index;
        return list[index];
    }


};



// new version!
template<typename T>
struct BaseOperator {
public:
static const uint8_t NullState = 0xff;
    using base = BaseOperator<T>;
    using sys_type = T;
    using coroutine_type = typename T::script_type;
    using function_type = typename coroutine_type::function_type;
    using variant_type = typename coroutine_type::variant_type;
    using tuple_type = typename coroutine_type::tuple_type;
    using invoke_func = bool (*)(sys_type&, DataPooler<coroutine_type>*, coroutine_type*, Handle);
   
    struct invokeFunc {
        invokeFunc() : value(nullptr) { }
        invoke_func value;
    };

    static std::vector<invokeFunc>& getFactory() {
        static std::vector<invokeFunc> m(std::tuple_size<tuple_type>::value);
        return m;
    }

    DataPooler<coroutine_type>* pooler;
    coroutine_type* program;
    bool _pause;

    sys_type& sys;
    Handle handle;

    bool paused() {
        return _pause;
    }

    uint8_t state() {
        return program->function.next;
    }

    void pause(uint8_t s = 0xff) {
        if (s == NullState) {
            ++program->function.next;
        } else {
            program->function.next = s;
        }
        _pause = true;
    }

    void proceed(uint8_t s = 0xff) {
        if (s == NullState) {
            ++program->function.next;
        } else {
            program->function.next = s;
        }
    }

    void push(variant_type v, uint8_t s = 0xff) {
        if (s == NullState) {
            ++program->function.next;
        } else {
            program->function.next = s;
        }
        uint16_t prev = pooler->store(program->function);
        program->function.returnId = prev;
        program->function.next = 0;
        program->function.top = v;
    }
 
    void pushp(variant_type v, uint8_t s = 0xff) {
        if (s == NullState) {
            ++program->function.next;
        } else {
            program->function.next = s;
        }
        uint16_t prev = pooler->store(program->function);
        program->function.returnId = prev;
        program->function.next = 0;
        program->function.top = v;
        _pause = true;
    }

    void pop() {
        if (program->function.returnId == pooler->end()) {
            program->function.top = NoopFunction();
            _pause = true;
        } else {
            program->function = pooler->retrive(program->function.returnId);
        }
    }

    void pop(coroutine_return_type r) {
        program->retValue = r;
        if (program->function.returnId == pooler->end()) {
            program->function.top = NoopFunction();
            _pause = true;
        } else {
            program->function = pooler->retrive(program->function.returnId);
        }
    }

    void popp() {
        if (program->function.returnId == pooler->end()) {
            program->function.top = NoopFunction();
            _pause = true;
        } else {
            program->function = pooler->retrive(program->function.returnId);
        }
        _pause = true;
    }

    void popp(coroutine_return_type r) {
        program->retValue = r;
        if (program->function.returnId == pooler->end()) {
            program->function.top = NoopFunction();
            _pause = true;
        } else {
            program->function = pooler->retrive(program->function.returnId);
        }
        _pause = true;
    }

    int32_t returnInt32() const {
        return std::get<int32_t>(program->retValue);
    }

};


template<typename T>
struct RegisterFunction {
    using sys_type = typename T::sys_type;
    using coroutine_type = typename sys_type::script_type;
    using type = typename T::value_type;
    static bool invoke(sys_type& sys, DataPooler<coroutine_type>* pooler, coroutine_type* program, Handle handle) {
        T op = { {pooler, program, false, sys, handle}, std::get<type>(program->function.top) };
        op.invoke();
        return op.paused();
    }
    static bool make() { 
        auto& m = BaseOperator<sys_type>::getFactory();
        uint32_t index = TupleIndex<type, typename T::tuple_type>::value;        
        assert(m[index].value == nullptr);
        m[index].value = &invoke;
        return true; 
    }
    static const bool value;
};

template<typename T>
const bool RegisterFunction<T>::value = RegisterFunction<T>::make();


template<typename T>
inline bool static_invoke_func(T* sys, DataPooler<typename T::script_type>& pooler, typename T::script_type& program, Handle handle) {
    auto& m = BaseOperator<T>::getFactory();
    uint32_t index = program.function.top.index();
    assert(index < m.size());
    auto func = m[index].value;
    if (func == nullptr) { return true; }
    return func(*sys, &pooler, &program, handle);
}


#endif // CORE_MATH_PROGRAM_HPP