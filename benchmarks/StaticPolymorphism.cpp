/*
 * TODO:
 *  type erasure idiom
 *  boost variant
 *  Mach7 pattern matching
 *  extend with https://github.com/thecppzoo/zoo/blob/master/benchmark/catch2Functions.cpp
 */

#include <array>
#include <functional>
#include <variant>

#include <benchmark/benchmark.h>

#include "util/random.hpp"


constexpr int NUMBER_OBJECTS = 1000;
benchmarks::util::RandomInInterval random_cls(0, 100);

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

template <std::size_t N>
std::array<int, N> get_random_array() {
    std::array<int, N> item{};
    for (int i = 0 ; i < N; i++)
        item[i] = random_cls.get_random_int();
    return item;
}

template <typename T, std::size_t N>
std::array<T, N> get_random_objects(std::function<T(decltype(random_cls.get_random_int()))> func) {
    std::array<T, N> a{};
    std::generate(a.begin(), a.end(), [&] {
        return func(random_cls.get_random_int());
    });
    return a;
}

namespace structs {
    struct S1 {constexpr void process() {}};
    struct S2 {constexpr void process() {}};
    struct S3 {constexpr void process() {}};

    S1 s1;
    S2 s2;
    S3 s3;
}

static void Switch(benchmark::State& state) {
    int r = 0;
    int index = 0;
    auto ran_arr = get_random_array<NUMBER_OBJECTS>();

    auto pick_randomly = [&] () {
        index = ran_arr[r++ % ran_arr.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        switch (index) {
            case 0:
                structs::s1.process();
                break;
            case 1:
                structs::s2.process();
                break;
            case 2:
                structs::s3.process();
                break;
            default:
                structs::s1.process();
                break;
        }

        benchmark::DoNotOptimize(index);
        pick_randomly();
    }
}
BENCHMARK(Switch);


namespace structs::switch_index {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [](auto r) -> type {
        switch (r) {
            case 0:
                return structs::s1;
            case 1:
                return structs::s2;
            case 2:
                return structs::s3;
            default:
                return structs::s1;
        };
    };
}
static void SwitchIndex(benchmark::State& state) {
    int r = 0;
    structs::switch_index::type* package = nullptr;
    auto tasks = get_random_objects<structs::switch_index::type, NUMBER_OBJECTS>(structs::switch_index::switch_lambda);
    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        switch (package->index()) {
            case 0:
                std::get<structs::S1>(*package).process();
                break;
            case 1:
                std::get<structs::S2>(*package).process();
                break;
            case 2:
                std::get<structs::S3>(*package).process();
                break;
            default:
                std::get<structs::S1>(*package).process();
                break;
        }
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(SwitchIndex);


namespace structs::virtual_call {
    struct Base {
        int data{};
        virtual void process() = 0;
        virtual ~Base() = default;;
    };
    struct A: public Base {void process() override{}};
    struct B: public Base {void process() override{}};
    struct C: public Base {void process() override{}};

    auto switch_lambda = [] (auto r) -> Base* {
        switch (r) {
            case 0:
                return new A;
            case 1:
                return new B;
            case 2:
                return new C;
            default:
                return new A;
        }
    };
}
static void Virtual(benchmark::State& state) {
    int r = 0;
    structs::virtual_call::Base* package = nullptr;
    auto tasks = get_random_objects<structs::virtual_call::Base*, NUMBER_OBJECTS>(
            structs::virtual_call::switch_lambda);

    auto pick_randomly = [&] () {
        package = tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        package->process();
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }

    for (auto&& i : tasks)
        delete i;
}
BENCHMARK(Virtual);


namespace structs::virtual_constexpr {
    struct Base {
        virtual constexpr void process() const = 0;
        virtual ~Base() = default;;
    };
    struct A: public Base {constexpr void process() const final {}};
    struct B: public Base {constexpr void process() const final {}};
    struct C: public Base {constexpr void process() const final {}};

    auto switch_lambda = [] (auto r) -> Base* {
        switch (r) {
            case 0:
                return new A;
            case 1:
                return new B;
            case 2:
                return new C;
            default:
                return new A;
        }
    };
}
static void VirtualConstexpr(benchmark::State& state) {
    int r = 0;
    structs::virtual_constexpr::Base* package = nullptr;
    auto tasks = get_random_objects<structs::virtual_constexpr::Base*, NUMBER_OBJECTS>(
            structs::virtual_constexpr::switch_lambda);

    auto pick_randomly = [&] () {
        package = tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        package->process();
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }

    for (auto&& i : tasks)
        delete i;
}
BENCHMARK(VirtualConstexpr);


namespace structs::virtual_final {
    struct Base {
        virtual void process()= 0;
        virtual ~Base() = default;
    };

    struct A: public Base {void process() final {}};
    struct B: public Base {void process() final {}};
    struct C: public Base {void process() final {}};

    auto switch_lambda = [] (auto r) -> Base* {
        switch(r) {
            case 0: return new A;
            case 1: return new B;
            case 2: return new C;
            default: return new A;
        }
    };
}
static void VirtualFinal(benchmark::State& state) {
    structs::virtual_final::Base* package = nullptr;
    int r = 0;
    auto tasks = get_random_objects<structs::virtual_final::Base*, NUMBER_OBJECTS>(
            structs::virtual_final::switch_lambda);

    auto pick_randomly = [&] () {
        package = tasks[r++ % tasks.size()];
    };

    pick_randomly();

    for (auto _ : state) {
        package->process();
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }

    for (auto&& i : tasks)
        delete i;
}
BENCHMARK(VirtualFinal);



namespace structs::CRTP {
    struct TBaseInterface {
        virtual ~TBaseInterface() = default;
        virtual void process() = 0;
    };

    template<typename T>
    struct TBase: public TBaseInterface {
        void process() override {return static_cast<T*>(this)->process();}
    };

    struct TS1: public TBase<TS1> {void process() final {}};
    struct TS2: public TBase<TS2> {void process() final {}};
    struct TS3: public TBase<TS3> {void process() final {}};

    auto switch_lambda = [] (auto r) -> TBaseInterface* {
        switch(r) {
            case 0: return new TS1;
            case 1: return new TS2;
            case 2: return new TS3;
            default: return new TS1;
        }
    };
}
static void CRTP(benchmark::State& state) {
    int r = 0;

    structs::CRTP::TBaseInterface* task = nullptr;
    auto tasks = get_random_objects<structs::CRTP::TBaseInterface*, NUMBER_OBJECTS>(
            structs::CRTP::switch_lambda);
    auto pick_randomly = [&] () {
        task = tasks[r++ % tasks.size()];
    };

    pick_randomly();

    for (auto _ : state) {
        task->process();
        benchmark::DoNotOptimize(task);
        pick_randomly();
    }

    for (auto&& i : tasks)
        delete i;
}
BENCHMARK(CRTP);


namespace structs::CRTPVisitor {
    template<typename T>
    struct TBase {
        void process() {return static_cast<T const&>(*this).process();}
    };

    struct TS1: public TBase<TS1> {void process() {}};
    struct TS2: public TBase<TS2> {void process() {}};
    struct TS3: public TBase<TS3> {void process() {}};

    using type = std::variant<TS1, TS2, TS3>;
    std::variant<TS1, TS2, TS3> events;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return TS1();
            case 1: return TS2();
            case 2: return TS3();
            default: return TS1();
        }
    };
    auto ov = overload {
            [] (TS1& t) { return t.process(); },
            [] (TS2& t) { return t.process(); },
            [] (TS3& t) { return t.process(); },
    };
}
static void CRTPVisitor(benchmark::State& state) {
    int r = 0;
    structs::CRTPVisitor::type* task = nullptr;
    auto tasks = get_random_objects<structs::CRTPVisitor::type, NUMBER_OBJECTS>(
            structs::CRTPVisitor::switch_lambda);

    auto pick_randomly = [&] () {
        task = &tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        std::visit(structs::CRTPVisitor::ov, *task);
        benchmark::DoNotOptimize(task);
        pick_randomly();
    }
}
BENCHMARK(CRTPVisitor);


namespace structs::concepts {
    struct Base {
        virtual ~Base() = default;
        virtual void process() = 0;
    };
    struct S1:Base {void process() final {}};
    struct S2:Base {void process() final {}};
    struct S3:Base {void process() final {}};

    template<typename T>
    concept ProcessLike = requires(T* t) {t->process();};
    auto process(ProcessLike auto* t) {return t->process();}

    auto switch_lambda = [] (auto r) -> Base* {
        switch(r) {
            case 0: return new S1;
            case 1: return new S2;
            case 2: return new S3;
            default: return new S1;
        }
    };
}
static void Concepts(benchmark::State& state) {
    int r = 0;
    structs::concepts::Base* task = nullptr;
    auto tasks = get_random_objects<structs::concepts::Base*, NUMBER_OBJECTS>(
            structs::concepts::switch_lambda);

    auto pick_randomly = [&] () {
        task = tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        structs::concepts::process(task);
        benchmark::DoNotOptimize(task);
        pick_randomly();
    }

    for (auto&& i : tasks)
        delete i;
}
BENCHMARK(Concepts);


namespace structs::functional_pointer_list {
    using type = std::function<std::void_t<>()>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return std::bind(&structs::S1::process, structs::s1);
            case 1: return std::bind(&structs::S2::process, structs::s2);
            case 2: return std::bind(&structs::S3::process, structs::s3);
            default: return std::bind(&structs::S1::process, structs::s1);
        }
    };
}
static void FunctionPointerList(benchmark::State& state) {
    int r = 0;
    std::size_t index;
    auto tasks = get_random_objects<structs::functional_pointer_list::type, NUMBER_OBJECTS>(
            structs::functional_pointer_list::switch_lambda);

    auto pick_randomly = [&] () {
        index = r++ % tasks.size();
    };

    pick_randomly();

    for (auto _ : state) {
        tasks[index]();
        benchmark::DoNotOptimize(index);
        pick_randomly();
    }
}
BENCHMARK(FunctionPointerList);


namespace structs::getif {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return structs::s1;
            case 1: return structs::s2;
            case 2: return structs::s3;
            default: return structs::s1;
        }
    };
}
static void GetIf(benchmark::State& state) {
    int r = 0;
    structs::getif::type* package = nullptr;
    auto tasks = get_random_objects<structs::getif::type, NUMBER_OBJECTS>(structs::getif::switch_lambda);

    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        if (auto item = std::get_if<structs::S1>(package)) {
            item->process();
        } else if (auto item2 = std::get_if<structs::S2>(package)) {
            item2->process();
        } else if (auto item3 = std::get_if<structs::S3>(package)) {
            item3->process();
        }
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(GetIf);


namespace structs::holds_alternative {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return structs::s1;
            case 1: return structs::s2;
            case 2: return structs::s3;
            default: return structs::s1;
        }
    };
}
static void HoldsAlternative(benchmark::State& state) {
    int r = 0;
    structs::holds_alternative::type* package = nullptr;

    auto tasks = get_random_objects<structs::holds_alternative::type, NUMBER_OBJECTS>(
            structs::holds_alternative::switch_lambda);
    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };
    pick_randomly();

    for (auto _ : state) {
        if (std::holds_alternative<structs::S1>(*package)) {
            std::get<structs::S1>(*package).process();
        } else if (std::holds_alternative<structs::S2>(*package)) {
            std::get<structs::S2>(*package).process();
        } else if (std::holds_alternative<structs::S3>(*package)) {
            std::get<structs::S3>(*package).process();
        }
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(HoldsAlternative);


namespace structs::visitor_constexpr {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return structs::s1;
            case 1: return structs::s2;
            case 2: return structs::s3;
            default: return structs::s1;
        }
    };
}
static void VisitorConstexpr(benchmark::State& state) {
    int r = 0;
    structs::visitor_constexpr::type* package = nullptr;
    auto tasks = get_random_objects<structs::visitor_constexpr::type, NUMBER_OBJECTS>(
            structs::visitor_constexpr::switch_lambda);

    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };
    pick_randomly();

    auto func = [] (auto& ref) {
        using type = std::decay_t<decltype(ref)>;

        if constexpr (std::is_same<type, structs::S1>::value) {return ref.process();}
        else if constexpr (std::is_same<type, structs::S2>::value) {return ref.process();}
        else if constexpr (std::is_same<type, structs::S3>::value) {return ref.process();}
    };

    for (auto _ : state) {
        std::visit(func, *package);
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(VisitorConstexpr);


namespace structs::visitor_struct {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return structs::s1;
            case 1: return structs::s2;
            case 2: return structs::s3;
            default: return structs::s1;
        }
    };

    struct VisitPackage {
        auto operator()(structs::S1& r) { return r.process(); }
        auto operator()(structs::S2& r) { return r.process(); }
        auto operator()(structs::S3& r) { return r.process(); }
    };
}
static void VisitorStruct(benchmark::State& state) {
    int r = 0;
    structs::visitor_struct::type* package = nullptr;
    auto tasks = get_random_objects<structs::visitor_struct::type, NUMBER_OBJECTS>(
            structs::visitor_struct::switch_lambda);

    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };
    pick_randomly();
    auto vs = structs::visitor_struct::VisitPackage();

    for (auto _ : state) {
        std::visit(vs, *package);
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(VisitorStruct);



namespace structs::visitor_struct_constexpr {
    using type = std::variant<structs::S1, structs::S2, structs::S3>;
    auto switch_lambda = [] (auto r) -> type {
        switch(r) {
            case 0: return structs::s1;
            case 1: return structs::s2;
            case 2: return structs::s3;
            default: return structs::s1;
        }
    };

    struct VisitPackage {
        constexpr auto operator()(structs::S1& r) { return r.process(); }
        constexpr auto operator()(structs::S2& r) { return r.process(); }
        constexpr auto operator()(structs::S3& r) { return r.process(); }
    };
}
static void VisitorStructConstexpr(benchmark::State& state) {
    int r = 0;
    structs::visitor_struct_constexpr::type* package = nullptr;
    auto tasks = get_random_objects<structs::visitor_struct_constexpr::type, NUMBER_OBJECTS>(
            structs::visitor_struct_constexpr::switch_lambda);

    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };

    pick_randomly();
    auto vs = structs::visitor_struct_constexpr::VisitPackage();

    for (auto _ : state) {
        std::visit(vs, *package);
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(VisitorStructConstexpr);


static void VisitorOverload(benchmark::State& state) {
    int r = 0;
    structs::visitor_struct::type* package = nullptr;
    auto tasks = get_random_objects<structs::visitor_struct::type, NUMBER_OBJECTS>(
            structs::visitor_struct::switch_lambda);

    auto pick_randomly = [&] () {
        package = &tasks[r++ % tasks.size()];
    };

    pick_randomly();

    auto ov = overload {
            [] (structs::S1& r) { return r.process(); },
            [] (structs::S2& r) { return r.process(); },
            [] (structs::S3& r) { return r.process(); },
    };

    for (auto _ : state) {
        std::visit(ov, *package);
        benchmark::DoNotOptimize(package);
        pick_randomly();
    }
}
BENCHMARK(VisitorOverload);


BENCHMARK_MAIN();
