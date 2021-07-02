#include <wil/com_apartment_variable.h>
#include <wil/com.h>
#include <functional>

#include "common.h"

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

struct mock_platform
{
    static unsigned long long GetApartmentId()
    {
        APTTYPE aptType;
        APTTYPEQUALIFIER aptQualifer;
        FAIL_FAST_IF_FAILED(CoGetApartmentType(&aptType, &aptQualifer)); // ensure COM is inited

        // incorrect, but close enough for testing
        return GetCurrentThreadId();
    }

    inline static std::unordered_map<unsigned long long, std::vector<wil::com_ptr<IApartmentShutdown>>> m_observers;

    static void RegisterForApartmentShutdown(IApartmentShutdown* observer)
    {
        const auto id = GetApartmentId();
        auto apt_observers = m_observers.find(id);
        if (apt_observers == m_observers.end())
        {
            m_observers.insert({ id, { observer} });
        }
        else
        {
            apt_observers->second.push_back(observer);
        }
    }

    // This is needed to simulate the platform for unit testing.
    static auto CoInitializeExForTesting(DWORD coinitFlags = 0 /*COINIT_MULTITHREADED*/)
    {
        return wil::scope_exit([aptId = GetCurrentThreadId(), init = wil::CoInitializeEx(coinitFlags)]()
        {
            const auto id = GetApartmentId();
            auto apt_observers = m_observers.find(id);
            if (apt_observers != m_observers.end())
            {
                const auto& observers = apt_observers->second;
                for (auto& comptr : observers)
                {
                    comptr->OnUninitialize(id);
                }
            }
        });
    }
};

auto fn() { return 42; };
auto fn2() { return 43; };

wil::apartment_variable<int, mock_platform> g_v1;

TEST_CASE("ComApartmentVariable::VerifyApartmentVariable", "[com][apartment_variable]")
{
    auto coUninit = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);

    std::ignore = g_v1.get_or_create(fn);

    wil::apartment_variable<int, mock_platform> v1;

    REQUIRE(v1.get_if() == nullptr);
    REQUIRE(v1.get_or_create(fn) == 42);
    int value = 43;
    v1.set(value);
    REQUIRE(v1.get_or_create(fn) == 43);
    REQUIRE(v1.get_existing() == 43);
    v1.clear();
    REQUIRE(v1.get_if() == nullptr);
}

TEST_CASE("ComApartmentVariable::VerifyApartmentVariableLifetimes", "[com][apartment_variable]")
{
    auto coUninit = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);

    wil::apartment_variable<int, mock_platform> av1, av2;

    {
        auto coUninitInner = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);

        auto v1 = av1.get_or_create(fn);
        REQUIRE(av1.apartment_count() == 1);
        auto v2 = av1.get_existing();
        REQUIRE((av1.current_apartment_variable_count() == 1));
        REQUIRE(v1 == v2);
    }

    {
        auto coUninitInner = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);
        auto v1 = av1.get_or_create(fn);
        auto v2 = av2.get_or_create(fn2);
        REQUIRE((av1.current_apartment_variable_count() == 2));
        REQUIRE(v1 != v2);
        REQUIRE(av1.apartment_count() == 1);
    }

    REQUIRE((wil::apartment_variable<int, mock_platform>::apartment_count() == 0));

    {
        auto coUninitInner = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);

        auto v = av1.get_or_create(fn);
        REQUIRE(av1.current_apartment_variable_count() == 1);

        std::thread([&]() // join below makes this ok
        {
            auto coUninit = mock_platform::CoInitializeExForTesting(COINIT_APARTMENTTHREADED);
            std::ignore = av1.get_or_create(fn);
            REQUIRE(av1.apartment_count() == 2);
            REQUIRE(av1.current_apartment_variable_count() == 1);
        }).join();
        REQUIRE(av1.apartment_count() == 1);

        av1.get_or_create(fn)++;
        v = av1.get_existing();
        REQUIRE(v == 43);
    }

    {
        auto coUninitInner = mock_platform::CoInitializeExForTesting(COINIT_MULTITHREADED);

        std::ignore = av1.get_or_create(fn);
        REQUIRE(av1.current_apartment_variable_count() == 1);
        av1.set(1);
        av1.clear();
        REQUIRE(av1.current_apartment_variable_count() == 0);

        // will fail fast since clear() was called.
        // av1.set(1);
    }

    REQUIRE((wil::apartment_variable<int, mock_platform>::apartment_count() == 0));
}

#endif