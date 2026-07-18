// callables_demo.cpp
// Building up from "a function is just code" to "why type erasure exists".
// Compile: clang++ callables_demo.cpp -o callables_demo -std=c++23
// Run:     ./callables_demo

#include <functional>   // std::function
#include <print>
#include <vector>

// ---------------------------------------------------------------------------
// PART 1: A plain function really IS just a code segment + an address.
// Your mental model is correct HERE.
// ---------------------------------------------------------------------------
int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

void part1() {
    std::println("--- PART 1: plain functions are just addresses ---");

    // A function pointer holds the ADDRESS of the code. Nothing else.
    // int(*)(int,int)  means "pointer to a function taking (int,int) returning int"
    int (*fp)(int, int) = add;      // fp points at add's code
    std::println("fp(2,3)      = {}", fp(2, 3));

    fp = mul;                       // repoint at different code
    std::println("fp(2,3) now  = {}", fp(2, 3));

    // The pointer is literally just a number (an address in the code segment):
    std::println("address of add = {}", reinterpret_cast<void*>(add));
    std::println("address of mul = {}", reinterpret_cast<void*>(mul));
    std::println("");
}

// ---------------------------------------------------------------------------
// PART 2: A lambda WITHOUT captures is also just code -> converts to a pointer.
// A lambda WITH captures is an OBJECT (code + data) -> can NOT be a pointer.
// This is the key thing your mental model was missing.
// ---------------------------------------------------------------------------
void part2() {
    std::println("--- PART 2: captures turn a lambda into an object ---");

    // No captures: [] is empty. This is pure code, so it converts to a plain
    // function pointer just like add/mul above.
    int (*fp)(int, int) = [](int a, int b) { return a + b; };
    std::println("captureless lambda via fn ptr: {}", fp(4, 5));

    // WITH a capture: this lambda "remembers" the value of `offset`.
    // That remembered value has to live SOMEWHERE. The lambda is therefore an
    // object that stores `offset` inside it, plus the code that uses it.
    int offset = 100;
    auto addOffset = [offset](int x) { return x + offset; };
    std::println("capturing lambda: {}", addOffset(5));   // 105

    // This line does NOT compile -- uncomment to see the error:
    //   int (*bad)(int) = addOffset;
    // A function pointer is only an address; it has no room to carry `offset`.
    // That's the wall you hit that makes std::function / type erasure necessary.

    std::println("sizeof(captureless lambda) = {}", sizeof([](int a){ return a; }));
    std::println("sizeof(addOffset object)   = {}  (it stores the captured int)",
                 sizeof(addOffset));
    std::println("");
}

// ---------------------------------------------------------------------------
// PART 3: Every lambda has its OWN unique type.
// Two lambdas that "do the same shape of thing" are still different types,
// so you can't store them in the same variable or container directly.
// ---------------------------------------------------------------------------
void part3() {
    std::println("--- PART 3: each lambda is its own type ---");

    auto a = [](int x) { return x + 1; };
    auto b = [](int x) { return x + 2; };
    // `a` and `b` have DIFFERENT compiler-generated types, even though both
    // are "int -> int". You literally cannot write `decltype(a) c = b;`.

    // So this is ILLEGAL -- different element types can't share a vector:
    //   std::vector<??? > v = { a, b };   // there is no single type to name
    std::println("a(10)={}, b(10)={}  (but they are different types)", a(10), b(10));
    std::println("");
}

// ---------------------------------------------------------------------------
// PART 4: TEMPLATE approach.
// The compiler sees the REAL type at the call site, stamps out a dedicated
// copy of the function for it, and can inline the call. Zero overhead.
// Downside: it's resolved at compile time -- you can't store "some callable"
// in a member variable or a container this way.
// ---------------------------------------------------------------------------
template <typename F>
int callTwice(F&& f, int x) {       // F is the lambda's REAL type, known here
    return f(f(x));                 // compiler can inline f directly
}

void part4() {
    std::println("--- PART 4: templates (compile-time, no overhead) ---");
    std::println("callTwice(+1):  {}", callTwice([](int x){ return x + 1; }, 10)); // 12
    std::println("callTwice(*2):  {}", callTwice([](int x){ return x * 2; }, 10)); // 40
    // Two DIFFERENT template instantiations were generated behind the scenes,
    // one per lambda type. Fast, but you must know the type at compile time.
    std::println("");
}

// ---------------------------------------------------------------------------
// PART 5: TYPE ERASURE via std::function.
// std::function<int(int)> is ONE type that can hold ANY callable of that shape:
// a plain function, a captureless lambda, or a capturing lambda-object.
// The concrete type is "erased" -- hidden behind a uniform interface.
// This is what lets you store them in a variable / vector and reassign.
// ---------------------------------------------------------------------------
void part5() {
    std::println("--- PART 5: std::function (runtime, type-erased) ---");

    int offset = 100;

    // All THREE different concrete types fit in the SAME std::function type:
    std::vector<std::function<int(int)>> jobs;
    jobs.push_back([](int x){ return x; });                 // captureless lambda
    jobs.push_back([offset](int x){ return x + offset; });  // capturing lambda-object
    jobs.emplace_back([](int x){ return x * x; });          // another lambda

    for (std::size_t i = 0; i < jobs.size(); ++i)
        std::println("jobs[{}](5) = {}", i, jobs[i](5));

    // Reassign the same variable to a totally different concrete callable:
    std::function<int(int)> f = [](int x){ return x + 1; };
    std::println("f(5) = {}", f(5));
    f = [offset](int x){ return x - offset; };   // different type, same variable
    std::println("f(5) = {}", f(5));
    std::println("");
}

// ---------------------------------------------------------------------------
// PART 6: How std::function does it -- a hand-rolled mini version.
// The trick: an abstract base with a virtual call(), and a templated derived
// class that remembers the REAL type. Outside code only ever sees the base
// pointer -> the concrete type is "erased".
// ---------------------------------------------------------------------------
class MyFunction {
    struct Base {                       // uniform interface, no type info
        virtual int call(int) = 0;
        virtual ~Base() = default;
    };
    template <typename F>
    struct Impl : Base {                // remembers the real type F (+ its data)
        F f;
        Impl(F f) : f(std::move(f)) {}
        int call(int x) override { return f(x); }  // real type known HERE
    };
    Base* ptr;                          // outside only sees this
public:
    template <typename F>
    MyFunction(F f) : ptr(new Impl<F>(std::move(f))) {}  // capture real type
    ~MyFunction() { delete ptr; }
    int operator()(int x) { return ptr->call(x); }       // dispatch via base
};

void part6() {
    std::println("--- PART 6: hand-rolled type erasure ---");
    int offset = 100;
    MyFunction g = [offset](int x){ return x + offset; }; // capturing object stored
    std::println("g(5) = {}  (concrete type hidden behind Base*)", g(5)); // 105
    std::println("");
}

int main() {
    part1();
    part2();
    part3();
    part4();
    part5();
    part6();
}
