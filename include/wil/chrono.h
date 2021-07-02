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
#ifndef __WIL_CHRONO_INCLUDED
#define __WIL_CHRONO_INCLUDED

#include "common.h"
#include "result.h"

#ifdef WIL_KERNEL_MODE
#error This header is not supported in kernel-mode.
#endif

#if __WI_CPLUSPLUS < 201103L
#error This header requires C++11 or later
#endif

#include <chrono>

#include <minwindef.h> // FILETIME, etc.
#include <sysinfoapi.h> // GetTickCount[64], GetSystemTime[Precise]AsFileTime
#include <realtimeapiset.h> // Query[Unbiased]InterruptTime[Precise]

namespace wil
{
#pragma region std::chrono wrappers for GetTickCount[64]
    namespace details
    {
        template<typename rep_t, typename get_tick_count_fn_t, get_tick_count_fn_t get_tick_count_fn, typename base_clock_t>
        struct tick_count_clock_impl
        {
            using rep = rep_t;
            using period = std::milli;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<base_clock_t, duration>;
            static constexpr bool is_steady = true;

            WI_NODISCARD static time_point now() WI_NOEXCEPT
            {
                return time_point{duration{ get_tick_count_fn() }};
            }
        };
    }

    struct tick_count_clock : details::tick_count_clock_impl<DWORD, decltype(&::GetTickCount), ::GetTickCount, tick_count_clock> {};
#if _WIN32_WINNT >= 0x0600
    struct tick_count64_clock : details::tick_count_clock_impl<ULONGLONG, decltype(&::GetTickCount64), ::GetTickCount64, tick_count_clock> {};
#endif // _WIN32_WINNT >= 0x0600
#pragma endregion

    namespace details
    {
        constexpr DWORD64 filetime_to_int(const FILETIME& ft) WI_NOEXCEPT
        {
            return (static_cast<DWORD64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        }

        constexpr FILETIME filetime_from_int(DWORD64 t) WI_NOEXCEPT
        {
            return {
                static_cast<DWORD>(t >> 32),
                static_cast<DWORD>(t)
            };
        }
    }

#pragma region std::chrono wrappers for GetSystemTime[Precise]AsFileTime
    namespace details
    {
        template<typename get_system_time_fn_t, get_system_time_fn_t get_system_time_fn, typename base_clock_t>
        struct system_time_clock_impl
        {
        public:
            using rep = LONGLONG;
            using period = std::ratio_multiply<std::hecto, std::nano>;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<base_clock_t, duration>;
            static constexpr bool is_steady = false;

            WI_NODISCARD static time_point now() WI_NOEXCEPT
            {
                alignas(rep) FILETIME ft;
                get_system_time_fn(&ft);
                return from_filetime(ft);
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 FILETIME to_filetime(const time_point& t) WI_NOEXCEPT
            {
                return filetime_from_int(static_cast<DWORD64>(t.time_since_epoch().count()));
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_filetime(const FILETIME& ft) WI_NOEXCEPT
            {
                return time_point{duration{ static_cast<rep>(filetime_to_int(ft)) }};
            }

#if __WI_LIBCPP_STD_VER > 11
            static constexpr time_point unix_epoch{duration{116444736000000000LL}};
#else
            static time_point const unix_epoch;
#endif

        private:
            template<class Duration>
            static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 Duration to_unix_epoch(const time_point& t) WI_NOEXCEPT
            {
                return std::chrono::duration_cast<Duration>(t - unix_epoch);
            }

            template<class Duration>
            static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_unix_epoch(const Duration& t) WI_NOEXCEPT
            {
                return std::chrono::time_point_cast<duration>(unix_epoch + t);
            }

            template<class Rep>
            static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 Rep to_crt_time(const time_point& t) WI_NOEXCEPT
            {
                return to_unix_epoch<std::chrono::duration<Rep>>(t).count();
            }

            template<class Rep>
            static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_crt_time(Rep t) WI_NOEXCEPT
            {
                return from_unix_epoch(std::chrono::duration<Rep>{t});
            }

        public:
            template<class Duration = std::chrono::system_clock::duration>
            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 std::chrono::time_point<std::chrono::system_clock, Duration> to_system_clock(const time_point& t) WI_NOEXCEPT
            {
                return std::chrono::time_point<std::chrono::system_clock, Duration>{to_unix_epoch<Duration>(t)};
            }

            template<class Duration>
            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_system_clock(const std::chrono::time_point<std::chrono::system_clock, Duration>& t) WI_NOEXCEPT
            {
                return from_unix_epoch(t.time_since_epoch());
            }

#ifndef _CRT_NO_TIME_T
            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 ::time_t to_time_t(const time_point& t) WI_NOEXCEPT
            {
                return to_crt_time<::time_t>(t);
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_time_t(::time_t t) WI_NOEXCEPT
            {
                return from_crt_time(t);
            }
#endif // !defined(_CRT_NO_TIME_T)

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 ::__time32_t to_time32_t(const time_point& t) WI_NOEXCEPT
            {
                return to_crt_time<::__time32_t>(t);
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_time32_t(::__time32_t t) WI_NOEXCEPT
            {
                return from_crt_time(t);
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 ::__time64_t to_time64_t(const time_point& t) WI_NOEXCEPT
            {
                return to_crt_time<::__time64_t>(t);
            }

            WI_NODISCARD static __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 time_point from_time64_t(::__time64_t t) WI_NOEXCEPT
            {
                return from_crt_time(t);
            }
        };

#if __WI_LIBCPP_STD_VER <= 11
        template<typename get_system_time_fn_t, get_system_time_fn_t get_system_time_fn, typename base_clock_t>
        typename system_time_clock_impl<get_system_time_fn_t, get_system_time_fn, base_clock_t>::time_point const system_time_clock_impl<get_system_time_fn_t, get_system_time_fn, base_clock_t>::unix_epoch{duration{116444736000000000LL}};
#endif
    }

    struct system_time_clock : details::system_time_clock_impl<decltype(&::GetSystemTimeAsFileTime), ::GetSystemTimeAsFileTime, system_time_clock> {};

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
    struct precise_system_time_clock : details::system_time_clock_impl<decltype(&::GetSystemTimePreciseAsFileTime), ::GetSystemTimePreciseAsFileTime, system_time_clock> {};
    using high_precision_system_time_clock = precise_system_time_clock;
#else // _WIN32_WINNT < _WIN32_WINNT_WIN8
    using high_precision_system_time_clock = system_time_clock;
#endif // _WIN32_WINNT < _WIN32_WINNT_WIN8
#pragma endregion

#pragma region std::chrono wrappers for Query[Unbiased]InterruptTime[Precise]
    namespace details
    {
        template<typename query_interrupt_time_fn_t, query_interrupt_time_fn_t query_interrupt_time_fn, typename base_clock_t>
        struct interrupt_time_clock_impl
        {
            using rep = LONGLONG;
            using period = std::ratio_multiply<std::hecto, std::nano>;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<base_clock_t, duration>;
            static constexpr bool is_steady = true;

            WI_NODISCARD static time_point now() WI_NOEXCEPT
            {
                ULONGLONG t;
                query_interrupt_time_fn(&t);
                return time_point{duration{ static_cast<rep>(t) }};
            }
        };
    }

#if _WIN32_WINNT >= 0x0601
    struct unbiased_interrupt_time_clock : details::interrupt_time_clock_impl<decltype(&::QueryUnbiasedInterruptTime), ::QueryUnbiasedInterruptTime, unbiased_interrupt_time_clock> {};
#if defined(NTDDI_WIN10)
    struct interrupt_time_clock : details::interrupt_time_clock_impl<decltype(&::QueryInterruptTime), ::QueryInterruptTime, interrupt_time_clock> {};
    struct precise_interrupt_time_clock : details::interrupt_time_clock_impl<decltype(&::QueryInterruptTimePrecise), ::QueryInterruptTimePrecise, interrupt_time_clock> {};
    struct precise_unbiased_interrupt_time_clock : details::interrupt_time_clock_impl<decltype(&::QueryUnbiasedInterruptTimePrecise), ::QueryUnbiasedInterruptTimePrecise, unbiased_interrupt_time_clock> {};
#endif // defined(NTDDI_WIN10)
#endif // _WIN32_WINNT >= 0x0601
#pragma endregion

#pragma region std::chrono wrappers for thread/process times
    using cpu_time_duration = std::chrono::duration<LONGLONG, std::ratio_multiply<std::hecto, std::nano>>;

    enum class cpu_time
    {
        total,
        kernel,
        user,
    };

    struct execution_times
    {
        system_time_clock::time_point creation_time;
        system_time_clock::time_point exit_time;
        cpu_time_duration kernel_time;
        cpu_time_duration user_time;
    };

    namespace details
    {
        inline __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 execution_times execution_times_from_filetimes(FILETIME creation_time, FILETIME exit_time, FILETIME kernel_time, FILETIME user_time) WI_NOEXCEPT
        {
            return {
                system_time_clock::from_filetime(creation_time),
                system_time_clock::from_filetime(exit_time),
                cpu_time_duration{ static_cast<cpu_time_duration::rep>(details::filetime_to_int(kernel_time)) },
                cpu_time_duration{ static_cast<cpu_time_duration::rep>(details::filetime_to_int(user_time)) }
            };
        }

        inline __WI_LIBCPP_CONSTEXPR_AFTER_CXX11 cpu_time_duration get_cpu_time(const execution_times& times, cpu_time kind) WI_NOEXCEPT
        {
            switch (kind)
            {
                case cpu_time::total: return times.kernel_time + times.user_time;
                case cpu_time::kernel: return times.kernel_time;
                case cpu_time::user: return times.user_time;
                DEFAULT_UNREACHABLE;
            }
        }
    }

    using thread_times = execution_times;

    template<class error_policy>
    WI_NODISCARD thread_times get_thread_times(HANDLE thread = ::GetCurrentThread())
    {
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;

        error_policy::Win32BOOL(::GetThreadTimes(thread, &creation_time, &exit_time, &kernel_time, &user_time));

        return details::execution_times_from_filetimes(creation_time, exit_time, kernel_time, user_time);
    }

    WI_NODISCARD inline thread_times get_thread_times(HANDLE thread = ::GetCurrentThread())
    {
        return get_thread_times<err_exception_policy>(thread);
    }

    template<class error_policy>
    WI_NODISCARD cpu_time_duration get_thread_cpu_time(HANDLE thread = ::GetCurrentThread(), cpu_time kind = cpu_time::total)
    {
        return details::get_cpu_time(get_thread_times<error_policy>(thread), kind);
    }

    WI_NODISCARD inline cpu_time_duration get_thread_cpu_time(HANDLE thread = ::GetCurrentThread(), cpu_time kind = cpu_time::total)
    {
        return get_thread_cpu_time<err_exception_policy>(thread, kind);
    }

    using process_times = execution_times;

    template<class error_policy>
    WI_NODISCARD process_times get_process_times(HANDLE process = ::GetCurrentProcess())
    {
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;

        error_policy::Win32BOOL(::GetProcessTimes(process, &creation_time, &exit_time, &kernel_time, &user_time));

        return details::execution_times_from_filetimes(creation_time, exit_time, kernel_time, user_time);
    }

    WI_NODISCARD inline process_times get_process_times(HANDLE process = ::GetCurrentProcess())
    {
        return get_process_times<err_exception_policy>(process);
    }

    template<class error_policy>
    WI_NODISCARD cpu_time_duration get_process_cpu_time(HANDLE process = ::GetCurrentProcess(), cpu_time kind = cpu_time::total)
    {
        return details::get_cpu_time(get_process_times<error_policy>(process), kind);
    }

    WI_NODISCARD inline cpu_time_duration get_process_cpu_time(HANDLE process = ::GetCurrentProcess(), cpu_time kind = cpu_time::total)
    {
        return get_process_cpu_time<err_exception_policy>(process, kind);
    }
#pragma endregion

#pragma region Thread/process CPU clocks
    struct current_thread_cpu_time_clock
    {
        using rep = cpu_time_duration::rep;
        using period = cpu_time_duration::period;
        using duration = cpu_time_duration;
        using time_point = std::chrono::time_point<current_thread_cpu_time_clock, duration>;

        template<class error_policy>
        WI_NODISCARD static time_point now()
        {
            return time_point{ get_thread_cpu_time<error_policy>() };
        }

        WI_NODISCARD static time_point now()
        {
            return time_point{ get_thread_cpu_time() };
        }
    };

    struct current_process_cpu_time_clock
    {
        using rep = cpu_time_duration::rep;
        using period = cpu_time_duration::period;
        using duration = cpu_time_duration;
        using time_point = std::chrono::time_point<current_process_cpu_time_clock, duration>;

        template<class error_policy>
        WI_NODISCARD static time_point now()
        {
            return time_point{ get_process_cpu_time<error_policy>() };
        }

        WI_NODISCARD static time_point now()
        {
            return time_point{ get_process_cpu_time() };
        }
    };
#pragma endregion
}

#endif // __WIL_CHRONO_INCLUDED
