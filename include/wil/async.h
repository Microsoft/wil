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
#ifndef __WIL_ASYNC_INCLUDED
#define __WIL_ASYNC_INCLUDED

#include <version>
#ifndef __cpp_lib_coroutine
#error This header requires C++20 coroutines.
#endif

#include <coroutine>
#include <Windows.h>
#include "resource.h"
#include "result.h"

namespace wil
{
    namespace details
    {
        struct operation_info
        {
            OVERLAPPED overlapped = { };

            bool completed = false;
            HRESULT error = S_OK;

            std::coroutine_handle<> coro;
        };

        template<typename err_policy>
        struct async_file_operation
        {
            unique_process_heap_ptr<operation_info> info = nullptr;
            HRESULT setup_error = S_OK;

            bool await_ready() const noexcept
            {
                return FAILED(setup_error) || info->completed;
            }

            typename err_policy::result await_resume() const
            {
                if (FAILED(setup_error))
                {
                    return err_policy::HResult(setup_error);
                }
                else
                {
                    return err_policy::HResult(info->error);
                }
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept
            {
                info->coro = handle;
            }
        };

        void WINAPI OverlappedCallback(DWORD error, DWORD count, LPOVERLAPPED overlapped)
        {
            const auto info = reinterpret_cast<details::operation_info*>(overlapped);
            info->completed = true;
            info->error = HRESULT_FROM_WIN32(error);

            if (info->overlapped.hEvent)
            {
                *static_cast<DWORD*>(info->overlapped.hEvent) = count;
            }

            if (info->coro)
            {
                info->coro();
            }
        }
    }

    template<typename err_policy = err_exception_policy>
    details::async_file_operation<err_policy> ReadFileAsync(HANDLE handle, LPCVOID buffer, DWORD bytes_to_read, DWORD* bytes_read, UINT64 offset)
    {
        details::async_file_operation<err_policy> op;
        op.info.reset(new (::HeapAlloc(::GetProcessHeap(), 0, sizeof(details::operation_info))) details::operation_info);
        if (op.info)
        {
            // this is not used by ReadFileEx so we'll use it.
            op.info->overlapped.hEvent = bytes_read;

            ULARGE_INTEGER offset_large;
            offset_large.QuadPart = offset;
            op.info->overlapped.Offset = offset_large.LowPart;
            op.info->overlapped.OffsetHigh = offset_large.HighPart;

            if (::ReadFileEx(handle, buffer, bytes_to_read, &op.info->overlapped, details::OverlappedCallback))
            {
                return op;
            }   
        }

        op.setup_error = HRESULT_FROM_WIN32(::GetLastError());
        op.info = nullptr;
        return op;
    }

    template<typename err_policy = err_exception_policy>
    details::async_file_operation<err_policy> WriteFileAsync(HANDLE handle, LPCVOID buffer, DWORD bytes_to_write, DWORD* bytes_written, UINT64 offset = -1)
    {
        details::async_file_operation<err_policy> op;
        op.info.reset(new (::HeapAlloc(::GetProcessHeap(), 0, sizeof(details::operation_info))) details::operation_info);
        if (op.info)
        {
            // this is not used by WriteFileEx so we'll use it.
            op.info->overlapped.hEvent = bytes_written;

            ULARGE_INTEGER offset_large;
            offset_large.QuadPart = offset;
            op.info->overlapped.Offset = offset_large.LowPart;
            op.info->overlapped.OffsetHigh = offset_large.HighPart;

            if (::WriteFileEx(handle, buffer, bytes_to_write, &op.info->overlapped, details::OverlappedCallback))
            {
                return op;
            }   
        }

        op.setup_error = HRESULT_FROM_WIN32(::GetLastError());
        op.info = nullptr;
        return op;
    }

} // wil

#endif
