//*********************************************************
//
//    Copyright (c) Microsoft. All rights reserved.
//    This code is licensed under the MIT License.
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
//    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
//    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//    PARTICULAR PURPOSE AND NONINFRINGEMENT.
//
//*********************************************************
#ifndef __WIL_COM_APARTMENT_VARIABLE_INCLUDED
#define __WIL_COM_APARTMENT_VARIABLE_INCLUDED

#include <unordered_map>
#include <any>
#include <type_traits>
#include <functional>
#include "cppwinrt.h"
#include <roapi.h>
#include <objidl.h>
#include "result_macros.h"

#ifndef WIL_ENABLE_EXCEPTIONS
#error This header requires exceptions
#endif

namespace wil
{
    // Determine if apartment variables are supported in the current process context.
    // Prior to build 22365, the APIs needed to create apartment variables (e.g. RoGetApartmentIdentifier)
    // failed for unpackaged processes. For MS people, see http://task.ms/31861017 for details.
    // APIs needed to implement apartment variables did not work in non-packaged processes.
    inline bool are_apartment_variables_supported()
    {
        unsigned long long apartmentId{};
        return RoGetApartmentIdentifier(&apartmentId) != HRESULT_FROM_WIN32(ERROR_API_UNAVAILABLE);
    }

    namespace details
    {
        struct apt_var_platform
        {
            static unsigned long long GetApartmentId()
            {
                unsigned long long apartmentId{};
                FAIL_FAST_IF_FAILED(RoGetApartmentIdentifier(&apartmentId));
                return apartmentId;
            }

            static void RegisterForApartmentShutdown(IApartmentShutdown* observer)
            {
                unsigned long long id{};
                APARTMENT_SHUTDOWN_REGISTRATION_COOKIE cookie{}; // leaked, COM run down releases all instances
                THROW_IF_FAILED(RoRegisterForApartmentShutdown(observer, &id, &cookie));
            }
        };

        template<typename test_hook = apt_var_platform>
        struct apartment_variable_base
        {
            inline static winrt::slim_mutex s_lock;

            using variables_map = std::unordered_map<apartment_variable_base<test_hook>*, std::any>;

            // Apartment id -> variable storage.
            // Variables are stored using the address of the global variable as the key.
            inline static std::unordered_map<unsigned long long, variables_map> s_apartmentStorage;

            apartment_variable_base() = default;
            ~apartment_variable_base()
            {
                clear();
            }

            // non-copyable, non-assignable
            apartment_variable_base(apartment_variable_base const&) = delete;
            void operator=(apartment_variable_base const&) = delete;

            // get current value or throw if no value has been set
            std::any& get_existing()
            {
                if (auto any = get_if())
                {
                    return *any;
                }
                THROW_HR(E_NOT_SET);
            }

            static variables_map* get_current_apartment_variables()
            {
                auto storage = s_apartmentStorage.find(test_hook::GetApartmentId());
                if (storage != s_apartmentStorage.end())
                {
                    return &storage->second;
                }
                return nullptr;
            }

            variables_map* ensure_current_apartment_variables()
            {
                auto variables = get_current_apartment_variables();
                if (variables)
                {
                    return variables;
                }

                struct ApartmentObserver : public winrt::implements<ApartmentObserver, IApartmentShutdown>
                {
                    void STDMETHODCALLTYPE OnUninitialize(unsigned long long apartmentId) noexcept override
                    {
                        // This code runs at apartment rundown so be careful to avoid deadlocks by
                        // extracting the variables under the lock then release them outside.
                        auto variables = [apartmentId]()
                        {
                            auto lock = winrt::slim_lock_guard(s_lock);
                            return s_apartmentStorage.extract(apartmentId);
                        }();
                    }
                };
                test_hook::RegisterForApartmentShutdown(winrt::make<ApartmentObserver>().get());
                return &s_apartmentStorage.insert({ test_hook::GetApartmentId(), {} }).first->second;
            }

            // get current value or custom-construct one on demand
            std::any& get_or_create(const std::function<std::any()>& creator)
            {
                variables_map* variables{};

                { // scope for lock
                    auto lock = winrt::slim_lock_guard(s_lock);
                    variables = ensure_current_apartment_variables();

                    auto variable = variables->find(this);
                    if (variable != variables->end())
                    {
                        return variable->second;
                    }
                } // drop the lock

                // create the object outside the lock to avoid reentrancy deadlock
                auto value = creator();

                auto insert_lock = winrt::slim_lock_guard(s_lock);
                // The insertion may fail if creator() recursively caused itself to be created,
                // in which case we return the existing object and the falsely-created one is discarded.
                return variables->insert({ this, std::move(value) }).first->second;
            }

            // get pointer to current value or nullptr if no value has been set
            std::any* get_if()
            {
                auto lock = winrt::slim_lock_guard(s_lock);

                if (auto variables = get_current_apartment_variables())
                {
                    auto variable = variables->find(this);
                    if (variable != variables->end())
                    {
                        return &(variable->second);
                    }
                }
                return nullptr;
            }

            // replace or create the current value, fail fasts if the value is not already stored
            void set(std::any value)
            {
                // release value, with the swapped value, outside of the lock
                {
                    auto lock = winrt::slim_lock_guard(s_lock);
                    auto storage = s_apartmentStorage.find(test_hook::GetApartmentId());
                    FAIL_FAST_IF(storage == s_apartmentStorage.end());
                    auto& variables = storage->second;
                    auto variable = variables.find(this);
                    FAIL_FAST_IF(variable == variables.end());
                    variable->second.swap(value);
                }
            }

            // remove any current value
            void clear()
            {
                auto lock = winrt::slim_lock_guard(s_lock);
                if (auto variables = get_current_apartment_variables())
                {
                    variables->erase(this);
                }
            }

            // methods for testing
            static size_t apartment_count()
            {
                auto lock = winrt::slim_lock_guard(s_lock);
                return s_apartmentStorage.size();
            }

            static size_t current_apartment_variable_count()
            {
                auto lock = winrt::slim_lock_guard(s_lock);
                if (auto variables = get_current_apartment_variables())
                {
                    return variables->size();
                }
                return 0;
            }
        };
    }

    template<typename T, typename test_hook = details::apt_var_platform>
    struct apartment_variable : details::apartment_variable_base<test_hook>
    {
        using base = details::apartment_variable_base<test_hook>;

        // get current value or throw if no value has been set
        T& get_existing() { return std::any_cast<T&>(base::get_existing()); }

        // get current value or default-construct one on demand
        T& get_or_create()
        {
            return get_or_create([]() { return T{}; });
        }

        // get current value or custom-construct one on demand
        T& get_or_create(T(*creator)())
        {
            return std::any_cast<T&>(base::get_or_create(creator));
        }

        // get pointer to current value or nullptr if no value has been set
        T* get_if() { return std::any_cast<T>(base::get_if()); }

        // replace or create the current value, fail fasts if the value is not already stored
        template<typename V> void set(V&& value) { return base::set(std::forward<V>(value)); }

        // remove any current value
        using base::clear;
        // for testing
        using base::apartment_count;
        using base::current_apartment_variable_count;
    };
}

#endif // __WIL_COM_APARTMENT_VARIABLE_INCLUDED
