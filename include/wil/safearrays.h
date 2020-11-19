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

#ifdef _KERNEL_MODE
#error This header is not supported in kernel-mode.
#endif

#include <new.h> // new(std::nothrow)
#include "wistd_type_traits.h"
#include <OleAuto.h>
#include "resource.h"

namespace wil
{
#if !defined(__WIL_SAFEARRAY_)
#define __WIL_SAFEARRAY_
    /// @cond
    namespace details
    {
        template<typename T> struct infer_safearray_type_traits{};
        template<> struct infer_safearray_type_traits<char>                       { static constexpr auto vartype = VT_I1; };
        //template<> struct infer_safearray_type_traits<short>                      { static constexpr auto vartype = VT_I2; };
        template<> struct infer_safearray_type_traits<long>                       { static constexpr auto vartype = VT_I4; };
        template<> struct infer_safearray_type_traits<int>                        { static constexpr auto vartype = VT_I4; };
        template<> struct infer_safearray_type_traits<long long>                  { static constexpr auto vartype = VT_I8; };
        template<> struct infer_safearray_type_traits<unsigned char>              { static constexpr auto vartype = VT_UI1; };
        template<> struct infer_safearray_type_traits<unsigned short>             { static constexpr auto vartype = VT_UI2; };
        template<> struct infer_safearray_type_traits<unsigned long>              { static constexpr auto vartype = VT_UI4; };
        template<> struct infer_safearray_type_traits<unsigned int>               { static constexpr auto vartype = VT_UI4; };
        template<> struct infer_safearray_type_traits<unsigned long long>         { static constexpr auto vartype = VT_UI8; };
        template<> struct infer_safearray_type_traits<float>                      { static constexpr auto vartype = VT_R4; };
        //template<> struct infer_safearray_type_traits<double>                     { static constexpr auto vartype = VT_R8; };
        template<> struct infer_safearray_type_traits<VARIANT_BOOL>               { static constexpr auto vartype = VT_BOOL; };
        template<> struct infer_safearray_type_traits<DATE>                       { static constexpr auto vartype = VT_DATE; };
        template<> struct infer_safearray_type_traits<CURRENCY>                   { static constexpr auto vartype = VT_CY; };
        template<> struct infer_safearray_type_traits<DECIMAL>                    { static constexpr auto vartype = VT_DECIMAL; };
        template<> struct infer_safearray_type_traits<BSTR>                       { static constexpr auto vartype = VT_BSTR; };
        template<> struct infer_safearray_type_traits<LPUNKNOWN>                  { static constexpr auto vartype = VT_UNKNOWN; };
        template<> struct infer_safearray_type_traits<LPDISPATCH>                 { static constexpr auto vartype = VT_DISPATCH; };
        template<> struct infer_safearray_type_traits<VARIANT>                    { static constexpr auto vartype = VT_VARIANT; };

        inline void __stdcall SafeArrayDestroy(SAFEARRAY* psa) WI_NOEXCEPT
        {
            FAIL_FAST_IF_FAILED(::SafeArrayDestroy(psa));
        }

        inline void __stdcall SafeArrayLock(SAFEARRAY* psa) WI_NOEXCEPT
        {
            FAIL_FAST_IF_FAILED(::SafeArrayLock(psa));
        }

        inline void __stdcall SafeArrayUnlock(SAFEARRAY* psa) WI_NOEXCEPT
        {
            FAIL_FAST_IF_FAILED(::SafeArrayUnlock(psa));
        }

        template<typename T>
        inline void __stdcall SafeArrayAccessData(SAFEARRAY* psa, T*& p) WI_NOEXCEPT
        {
            WI_ASSERT(sizeof(T) == ::SafeArrayGetElemsize(psa));
            FAIL_FAST_IF_FAILED(::SafeArrayAccessData(psa, reinterpret_cast<void**>(&p)));
        }

        inline void __stdcall SafeArrayUnaccessData(SAFEARRAY* psa) WI_NOEXCEPT
        {
            FAIL_FAST_IF_FAILED(::SafeArrayUnaccessData(psa));
        }

        inline VARTYPE __stdcall SafeArrayGetVartype(SAFEARRAY* psa) WI_NOEXCEPT
        {
            VARTYPE vt = VT_NULL;   // Invalid for SAs, so use to mean SA was null
            if ((psa != nullptr) && FAILED(::SafeArrayGetVartype(psa, &vt)))
            {
                vt = VT_EMPTY;  // Invalid for SAs, use to mean type couldn't be determined
            }
            return vt;
        }

        inline ULONG __stdcall SafeArrayGetLockCount(SAFEARRAY* psa) WI_NOEXCEPT
        {
            return (psa != nullptr) ? psa->cLocks : 0U;
        }

        inline HRESULT __stdcall SafeArrayCreate(VARTYPE vt, UINT dims, SAFEARRAYBOUND* sab, SAFEARRAY*& psa) WI_NOEXCEPT
        {
            WI_ASSERT(dims > 0);
            psa = ::SafeArrayCreate(vt, dims, sab);
            RETURN_LAST_ERROR_IF_NULL(psa);
            WI_ASSERT(vt == details::SafeArrayGetVartype(psa));
            return S_OK;
        }

        inline HRESULT __stdcall SafeArrayGetDimSize(SAFEARRAY* psa, UINT dims, ULONG* result) WI_NOEXCEPT
        {
            RETURN_HR_IF(E_INVALIDARG, ((dims == 0) || (dims > psa->cDims)));
            // Read from the back of the array because SAFEARRAYs are stored with the dimensions in reverse order
            *result = psa->rgsabound[psa->cDims - dims].cElements;
            return S_OK;
        }

        inline HRESULT __stdcall SafeArrayGetTotalSize(SAFEARRAY* psa, ULONG* result) WI_NOEXCEPT
        {
            if (psa != nullptr)
            {
                ULONGLONG elements = 1;
                for (UINT i = 0; i < psa->cDims; ++i)
                {
                    // Determine the total size of the safearray by multiplying
                    // the sizes of each dimension together
                    elements *= psa->rgsabound[i].cElements;
                    if (elements > ULONG_MAX)
                    {
                        RETURN_HR(HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
                    }
                }
                *result = static_cast<ULONG>(elements);
            }
            else
            {
                // If it's invalid, it doesn't contain any elements
                *result = 0;
            }
            return S_OK;
        }

        typedef resource_policy<SAFEARRAY*, decltype(&details::SafeArrayDestroy), details::SafeArrayDestroy, details::pointer_access_all> safearray_resource_policy;
    }
    /// @endcond

    typedef unique_any<SAFEARRAY*, decltype(&details::SafeArrayUnlock), details::SafeArrayUnlock, details::pointer_access_noaddress> safearray_unlock_scope_exit;
    typedef unique_any<SAFEARRAY*, decltype(&details::SafeArrayUnaccessData), details::SafeArrayUnaccessData, details::pointer_access_noaddress> safearray_unaccess_scope_exit;

    //! Guarantees a SafeArrayUnlock call on the given object when the returned object goes out of scope
    //! Note: call SafeArrayUnlock early with the reset() method on the returned object or abort the call with the release() method
    WI_NODISCARD inline safearray_unlock_scope_exit SafeArrayUnlock_scope_exit(SAFEARRAY* psa) WI_NOEXCEPT
    {
        details::SafeArrayLock(psa);
        return safearray_unlock_scope_exit(psa);
    }

    //! Guarantees a SafeArrayUnaccessData call on the given object when the returned object goes out of scope
    //! Note: call SafeArrayUnaccessData early with the reset() method on the returned object or abort the call with the release() method
    template<typename T>
    WI_NODISCARD inline safearray_unaccess_scope_exit SafeArrayAccessData_scope_exit(SAFEARRAY* psa, T*& p) WI_NOEXCEPT
    {
        details::SafeArrayAccessData(psa, p);
        return safearray_unaccess_scope_exit(psa);
    }

    //! Class that facilitates the direct access to the contents of a SAFEARRAY and "unaccessing" it when done
    //! It enables treating the safearray like an regular array and/or performing a ranged-for on the contents.
    //! Accessing a safearray increases it's lock count, so attempts to destroy the safearray will fail until it is unaccessed.
    //! This class works even if the SAFEARRAY is multi-dimensional by treating it as a large single-dimension array,
    //! but doesn't support access via a multi-dimensional index.
    //! NOTE: This class does not manage the lifetime of the SAFEARRAY itself.  See @ref safearray_t.
    //! ~~~~
    //! HRESULT copy_to_bstr_vector(SAFEARRAY* psa, std::vector<wil::unique_bstr>& result)
    //! {
    //!     auto data = wil::safearraydata_nothrow<BSTR>{};
    //!     RETURN_IF_FAILED(data.access(psa));
    //!     result.reserve(result.size() + data.size());
    //!     for (BSTR& bstr : data)
    //!     {
    //!         result.emplace_back(wil::make_bstr_nothrow(bstr));
    //!         RETURN_HR_IF_NULL(E_OUTOFMEMORY, result.rbegin()->get());
    //!     }
    //!     return S_OK;
    //! }
    //! ~~~~
    //! wil::unique_bstr_safearray move_to_safearray(std::vector<wil::unique_bstr>& source)
    //! {
    //!     auto sa = wil::unique_bstr_safearray{source.size()};
    //!     auto current = std::begin(source);
    //!     for (BSTR& bstr : sa.access_data())
    //!     {
    //!         // Transfer ownership of the BSTR into the safearray
    //!         bstr = (*current++).release();
    //!     }
    //!     return sa;
    //! }
    //! ~~~~
    //! void handle_safearray_of_bools(SAFEARRAY* psa, std::function<void(bool, ULONG)> fnHandler)
    //! {
    //!     auto data = wil::safearraydata<VARIANT_BOOL>{psa};
    //!     for (auto i = 0U; i < data.size(); ++i)
    //!     {
    //!         // Translate to Regular Bool and Invoke the Handler
    //!         fnHandler(data[i] != VARIANT_FALSE, i);
    //!     }
    //! }
    //! ~~~~
    //! template<typename T>
    //! std::vector<T> extract_all_values(SAFEARRAY* psa)
    //! {
    //!     auto data = wil::safearraydata<T>{psa};
    //!     // Construct the vector from the iterators
    //!     return {data.begin(), data.end()};
    //! }
    //! ~~~~
    //! template<typename INTERFACE>
    //! std::vector<wil::com_ptr_t<INTERFACE, wil::err_exception_policy>> extract_all_interfaces(SAFEARRAY* psa)
    //! {
    //!     auto result = std::vector<wil::com_ptr_t<INTERFACE, wil::err_exception_policy>>{};
    //!     auto data = wil::safearraydata<LPUNKNOWN>{psa};
    //!     result.reserve(data.size());
    //!     for (auto& p : data) // Type of p is LPUNKNOWN
    //!     {
    //!         // Use "tag_com_query" if you want failure instead of nullptr if INTERFACE not supported
    //!         result.emplace_back(p, wil::details::tag_try_com_query);
    //!     }
    //!     return result;
    //! }
    //! ~~~~
    //! @tparam T           The underlying datatype expected in the SAFEARRAY. It should be a scalar type, BSTR, VARIANT_BOOL, LPUNKNOWN/LPDISPATCH, etc.
    //! @tparam err_policy  Represents the error policy for the class (error codes, exceptions, or fail fast; see @ref page_errors)
    template <typename T, typename err_policy = err_exception_policy>
    class safearraydata_t
    {
    public:
        typedef typename err_policy::result result;
        typedef safearray_unaccess_scope_exit::pointer pointer;
        typedef T value_type;
        typedef value_type* value_pointer;
        typedef const value_type* const_value_pointer;
        typedef value_type& value_reference;
        typedef const value_type& const_value_reference;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;

    public:
        // Exception-based construction
        explicit safearraydata_t(pointer psa)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions; use the create method");
            access(psa);
        }
        safearraydata_t() = default;
        safearraydata_t(safearraydata_t&&) = default;
        ~safearraydata_t() = default;
        safearraydata_t& operator=(safearraydata_t&&) = default;
        safearraydata_t(const safearraydata_t&) = delete;
        safearraydata_t& operator=(const safearraydata_t&) = delete;

        result access(pointer psa)
        {
            return err_policy::HResult([&]()
                {
                    m_unaccess = SafeArrayAccessData_scope_exit(psa, m_begin);
                    RETURN_IF_FAILED(details::SafeArrayGetTotalSize(m_unaccess.get(), &m_size));
                    return S_OK;
                }());
        }

        // Ranged-for style
        iterator begin() WI_NOEXCEPT { WI_ASSERT(m_begin != nullptr); return m_begin; }
        iterator end() WI_NOEXCEPT { WI_ASSERT(m_begin != nullptr); return m_begin+m_size; }
        const_iterator begin() const WI_NOEXCEPT { WI_ASSERT(m_begin != nullptr); return m_begin; }
        const_iterator end() const WI_NOEXCEPT { WI_ASSERT(m_begin != nullptr); return m_begin+m_size; }

        // Old style iteration
        ULONG size() const WI_NOEXCEPT { return m_size; }
        WI_NODISCARD value_reference operator[](ULONG i) { WI_ASSERT(i < m_size); return *(m_begin + i); }
        WI_NODISCARD const_value_reference operator[](ULONG i) const { WI_ASSERT(i < m_size); return *(m_begin +i); }

        // Utilities
        bool empty() const WI_NOEXCEPT { return m_size == 0UL; }

    private:
        safearray_unaccess_scope_exit m_unaccess{};
        value_pointer m_begin{};
        ULONG m_size{};
    };

    template<typename T>
    using safearraydata_nothrow = safearraydata_t<T, err_returncode_policy>;
    template<typename T>
    using safearraydata_failfast = safearraydata_t<T, err_failfast_policy>;

#ifdef WIL_ENABLE_EXCEPTIONS
    template<typename T>
    using safearraydata = safearraydata_t<T, err_exception_policy>;
#endif

    //! RAII class for SAFEARRAYs that lives between unique_any_t and unique_storage to provide extra SAFEARRAY functionality.  
    //! SAFEARRAYs provide a convenient way of passing an array of values across APIs and can be useful because they will clean 
    //! up their resources (interface ref counts, BSTRs, etc.) when properly destroyed.  If you have a SAFEARRAY of interface 
    //! pointers or BSTRs, there is no need to call Release or SysFreeString on each element (that is done automatically
    //! when the array is destroyed in SafeArrayDestroy), so the only resource that needs to be managed is the SAFEARRAY
    //! itself.
    //! ~~~~
    //! // Return a SAFEARRAY from an API
    //! HRESULT GetWonderfulData(SAFEARRAY** ppsa)
    //! {
    //!     wil::unique_bstr_safearray_nothrow sa{};
    //!     RETURN_IF_FAILED(sa.create(32));
    //!     {
    //!         wil::safearraydata_nothrow<BSTR> data{};
    //!         RETURN_IF_FAILED(data.access(sa.get()));
    //!         for (auto& bstr : data)
    //!         {
    //!             // SAFEARRAY will own this string and clean it up
    //!             bstr = ::SysAllocString(L"Wonderful!");
    //!             // Even if we bail early, nothing will leak
    //!             RETURN_HR_IF_NULL(E_OUTOFMEMORY, bstr);
    //!         }
    //!     }
    //!     *ppsa = sa.release();
    //!     return S_OK;
    //! }
    //! ~~~~
    //! // Obtain a SAFEARRAY from an API
    //! HRESULT ProcessWonderful()
    //! {
    //!     wil::unique_safearray sa{};
    //!
    //!     // Invoke the API
    //!     RETURN_IF_FAILED(::GetWonderfulData(sa.put()));
    //!     // Verify the output is expected
    //!     RETURN_HR_IF(E_UNEXPECTED, sa.vartype() != VT_BSTR);
    //!     for (auto& bstr : sa.access_data<BSTR>())
    //!     {
    //!         // Use the BSTR, no clean up necessary
    //!         DoSomethingWithBSTR(bstr);
    //!     }
    //!     return S_OK;
    //! }
    //! ~~~~
    //! @tparam storage_t   Storage class that manages the SAFEARRAY pointer
    //! @tparam err_policy  Represents the error policy for the class (error codes, exceptions, or fail fast; see @ref page_errors)
    //! @tparam element_t   The underlying datatype that is contained in the safearray OR
    //!                     it can be void to make the safearray generic (and not as typesafe)
    template <typename storage_t, typename err_policy = err_exception_policy, typename element_t = void>
    class safearray_t : public storage_t
    {
    private:
        // SFINAE Helpers to improve readability
        template<typename T>
        using is_interface_type = wistd::disjunction<wistd::is_same<T, LPUNKNOWN>,
                                                        wistd::is_same<T, LPDISPATCH>>;
        template<typename T>
        using is_pointer_type = wistd::disjunction<wistd::is_same<T, BSTR>,
                                                        wistd::is_same<T, LPUNKNOWN>,
                                                        wistd::is_same<T, LPDISPATCH>>;

        template<typename T>
        using is_type_not_set = wistd::is_same<T, void>;
        template<typename T>
        using is_type_set = wistd::negation<is_type_not_set<T>>;
        template <typename T>
        using is_value_type = wistd::conjunction<wistd::negation<is_pointer_type<T>>, is_type_set<T>>;

    public:
        template<typename T>
        using safearraydata_t = safearraydata_t<T, err_policy>;

        // Represents this type
        using unique_type = unique_any_t<safearray_t<storage_t, err_policy, element_t>>;

        typedef typename err_policy::result result;
        typedef typename storage_t::pointer pointer;
        typedef element_t elemtype;

    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit safearray_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // Exception-based construction
        template<typename T = element_t, typename wistd::enable_if<is_type_not_set<T>::value, int>::type = 0>
        safearray_t(VARTYPE vt, UINT elements, LONG lowerBound = 0)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions; use the create method");
            create(vt, elements, lowerBound);
        }
        template<typename T = element_t, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        explicit safearray_t(UINT elements, LONG lowerBound = 0)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions; use the create method");
            create<T>(elements, lowerBound);
        }

        // Low-level, arbitrary number of dimensions
        // Uses Explicit Type
        template<typename T = element_t, typename wistd::enable_if<is_type_not_set<T>::value, int>::type = 0>
        result create(VARTYPE vt, UINT dims, SAFEARRAYBOUND* sab)
        {
            return err_policy::HResult(_create(vt, dims, sab));
        }
        // Uses Inferred Type
        template<typename T = element_t, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        result create(UINT dims, SAFEARRAYBOUND* sab)
        {
            constexpr auto vt = static_cast<VARTYPE>(details::infer_safearray_type_traits<T>::vartype);
            return err_policy::HResult(_create(vt, dims, sab));
        }

        // Single Dimension Specialization
        // Uses Explicit Type
        template<typename T = element_t, typename wistd::enable_if<is_type_not_set<T>::value, int>::type = 0>
        result create(VARTYPE vt, UINT elements, LONG lowerBound = 0)
        {
            auto bounds = SAFEARRAYBOUND{ elements, lowerBound };
            return err_policy::HResult(_create(vt, 1, &bounds));
        }
        // Uses Inferred Type
        template<typename T = element_t, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        result create(UINT elements, LONG lowerBound = 0)
        {
            constexpr auto vt = static_cast<VARTYPE>(details::infer_safearray_type_traits<T>::vartype);
            auto bounds = SAFEARRAYBOUND{ elements, lowerBound };
            return err_policy::HResult(_create(vt, 1, &bounds));
        }

        //! Create a Copy
        //! Replaces the current SAFEARRAY (if any) with a new one that is created by duplicating the elements in
        //! the SAFEARRAY* provided as a parameter in psaSource
        result create_copy(pointer psaSource)
        {
            WI_ASSERT(psaSource != nullptr);
            return err_policy::HResult([&]()
                {
                    auto psa = storage_t::policy::invalid_value();
                    RETURN_IF_FAILED(::SafeArrayCopy(psaSource, &psa));
                    storage_t::reset(psa);
                    return S_OK;
                }());
        }

        //! @name Property Helpers
        //! Functions to determine the size and shape of the SAFEARRAY, which
        //! can have an arbitrary number of dimensions and each dimension can
        //! it's own size and boundaries.
        //! @{

        //! Indicates the number of dimensions in the SAFEARRAY.  The 1st Dimension
        //! is always 1, and there never is a Dimension 0.
        ULONG dims() const WI_NOEXCEPT { return ::SafeArrayGetDim(storage_t::get()); }
        //! Returns the lowest possible index for the given dimension.
        result lbound(UINT dim, LONG* result) const
        {
            WI_ASSERT((dim > 0) && (dim <= dims()));
            return err_policy::HResult(::SafeArrayGetLBound(storage_t::get(), dim, result));
        }
        //! Returns the highest possible index for the given dimension.
        result ubound(UINT dim, LONG* result) const
        {
            WI_ASSERT((dim > 0) && (dim <= dims()));
            return err_policy::HResult(::SafeArrayGetUBound(storage_t::get(), dim, result));
        }
        //! 1D Specialization of lbound
        result lbound(LONG* result) const
        {
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetLBound(storage_t::get(), 1, result));
        }
        //! 1D Specialization of ubound
        result ubound(LONG* result) const
        {
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetUBound(storage_t::get(), 1, result));
        }
        //! Indicates the total number of elements across all the dimensions
        result size(ULONG* result) const
        {
            return err_policy::HResult(details::SafeArrayGetTotalSize(storage_t::get(), result));
        }
        //! Returns the number of elements in the specified dimension
        //! Same as, but faster than, the typical ubound() - lbound() + 1
        result dim_size(UINT dim, ULONG* result) const
        {
            WI_ASSERT((dim > 0) && (dim <= dims()));
            return err_policy::HResult(details::SafeArrayGetDimSize(storage_t::get(), dim, result));
        }
        //! Return the size, in bytes, of each element in the array
        ULONG elemsize() const WI_NOEXCEPT { return ::SafeArrayGetElemsize(storage_t::get()); }

        //! Retrieves the stored type (when not working with a fixed type)
        template<typename T = element_t, typename wistd::enable_if<is_type_not_set<T>::value, int>::type = 0>
        VARTYPE vartype() WI_NOEXCEPT
        {
            return details::SafeArrayGetVartype(storage_t::get());
        }

        //! Returns the current number of locks on the SAFEARRAY
        ULONG lock_count() const WI_NOEXCEPT
        {
            return details::SafeArrayGetLockCount(storage_t::get());
        }
        //! @}

        //! Returns an object that keeps a lock count on the safearray until it goes out of scope
        //! Use to keep the SAFEARRAY from being destroyed but without accessing the contents
        //! Not needed when using safearraydata_t because accessing the data also holds a lock
        //! ~~~~
        //! void Process(wil::unique_safearray& sa)
        //! {
        //!     auto lock = sa.scope_lock();
        //!     LongRunningFunction(sa);
        //! }
        WI_NODISCARD safearray_unlock_scope_exit scope_lock() const WI_NOEXCEPT
        {
            return wil::SafeArrayUnlock_scope_exit(storage_t::get());
        }

        //! @name Element Access
        //! Instead of using the safearraydata_t to access the entire collection of elements
        //! at once, it is also possible to access them one at a time.  However, using this
        //! functionality is less efficient because it deals with copies of the elements
        //! that the caller is then responsible for cleaning up, so use RAII data types
        //! for items that require clean up, like BSTR, VARIANT, or LPUNKNOWN
        //! ~~~~
        //! HRESULT ProcessWonderful_UsingElements()
        //! {
        //!     wil::unique_safearray_nothrow sa{};
        //!     wil::unique_bstr bstr{};
        //!     ULONG size{};
        //!
        //!     // Invoke the API
        //!     RETURN_IF_FAILED(::GetWonderfulData(sa.put()));
        //!     // Verify the output is expected
        //!     RETURN_HR_IF(E_UNEXPECTED, sa.vartype() != VT_BSTR);
        //!     RETURN_HR_IF(E_UNEXPECTED, sa.dims() != 1);
        //!     RETURN_IF_FAILED(sa.size(&size));
        //!     for (auto i = 0U; i < size; ++i)
        //!     {
        //!         // Copy from the safearray to the unique_bstr
        //!         sa.get_element(i, bstr.put()); // Will release current BSTR
        //!         DoSomethingWithBSTR(bstr);
        //!     }
        //!     return S_OK;
        //! }
        //! ~~~~
        //! @{

        //! Copies the specified element and puts it into the specified index in the SAFEARRAY.
        //! The SAFEARRAY will own the copy and will clean up the resources when it is destroyed.
        //! HOWEVER, the caller still owns the original and is responsible for it's clean up
        template<typename T, typename wistd::enable_if<is_value_type<T>::value, int>::type = 0>
        result put_element(LONG index, const T& val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, reinterpret_cast<void*>(const_cast<T*>(&val))));
        }
        template<typename T, typename wistd::enable_if<is_pointer_type<T>::value, int>::type = 0>
        result put_element(LONG index, T val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, val));
        }
        //! Retrieves a copy of the element at the specified index.  The caller owns this copy and must free
        //! any resources with LPUNKNOWN->Release, SysFreeString, VariantClear, etc.
        template<typename T, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        result get_element(LONG index, T& val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), &index, &val));
        }

        //! Multiple Dimension version of put_element
        template<typename T, typename wistd::enable_if<is_value_type<T>::value, int>::type = 0>
        result put_element(LONG* indices, const T& val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, reinterpret_cast<void*>(const_cast<T*>(&val))));
        }
        template<typename T, typename wistd::enable_if<is_pointer_type<T>::value, int>::type = 0>
        result put_element(LONG* indices, T val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, val));
        }
        //! Multiple Dimension version of get_element
        template<typename T, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        result get_element(LONG* indices, T& val)
        {
            WI_ASSERT(sizeof(val) == elemsize());
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), indices, &val));
        }

        //! Lowest-Level, Type-Erased versions for those who know what they're doing.
        result put_element(LONG index, void* pv)
        {
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, pv));
        }
        result get_element(LONG index, void* pv)
        {
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), &index, pv));
        }
        result put_element(LONG* indices, void* pv)
        {
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, pv));
        }
        result get_element(LONG* indices, void* pv)
        {
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), indices, pv));
        }

        //! Safer versions "get_element/put_element" that returns the copy wrapped in the appropriate storage class
#if defined(__WIL_OLEAUTO_H_)
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        result get_element(LONG index, wil::unique_bstr& val)
        {
            WI_ASSERT(sizeof(BSTR) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), &index, val.put()));
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        result put_element(LONG index, wil::unique_bstr& val)
        {
            WI_ASSERT(sizeof(BSTR) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, val.get()));
        }

        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        result get_element(LONG index, wil::unique_variant& val)
        {
            WI_ASSERT(sizeof(VARIANT) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), &index, val.reset_and_addressof()));
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        result put_element(LONG index, wil::unique_variant& val)
        {
            WI_ASSERT(sizeof(VARIANT) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, val.addressof()));
        }

        //! Multidimensional Versions
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        result get_element(LONG* indices, wil::unique_bstr& val)
        {
            WI_ASSERT(sizeof(BSTR) == elemsize());
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), indices, val.put()));
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        result put_element(LONG* indices, wil::unique_bstr& val)
        {
            WI_ASSERT(sizeof(BSTR) == elemsize());
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, val.get()));
        }

        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        result get_element(LONG* indices, wil::unique_variant& val)
        {
            WI_ASSERT(sizeof(VARIANT) == elemsize());
            return err_policy::HResult(::SafeArrayGetElement(storage_t::get(), indices, val.reset_and_addressof()));
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        result put_element(LONG* indices, wil::unique_variant& val)
        {
            WI_ASSERT(sizeof(VARIANT) == elemsize());
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, val.addressof()));
        }
#endif
        
#if defined(__WIL_COM_INCLUDED) // Only leverage the COM functionality if it's been included
        template<typename I, typename com_err_policy, typename T = element_t, typename wistd::enable_if<is_interface_type<T>::value, int>::type = 0>
        result get_element(LONG index, wil::com_ptr_t<I, com_err_policy>& val)
        {
            WI_ASSERT(sizeof(T) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult([&]()
                {
                    wil::com_ptr_nothrow<IUnknown> unk{};
                    RETURN_IF_FAILED(::SafeArrayGetElement(storage_t::get(), &index, unk.put()));
                    RETURN_IF_FAILED(unk.query_to(val.put()));
                    return S_OK;
                }());
        }
        template<typename I, typename com_err_policy, typename T = element_t, typename wistd::enable_if<is_interface_type<T>::value, int>::type = 0>
        result put_element(LONG index, wil::com_ptr_t<I, com_err_policy>& val)
        {
            WI_ASSERT(sizeof(T) == elemsize());
            WI_ASSERT(dims() == 1);
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), &index, val.get()));
        }

        template<typename I, typename com_err_policy, typename T = element_t, typename wistd::enable_if<is_interface_type<T>::value, int>::type = 0>
        result get_element(LONG* indices, wil::com_ptr_t<I, com_err_policy>& val)
        {
            WI_ASSERT(sizeof(T) == elemsize());
            return err_policy::HResult([&]()
                {
                    wil::com_ptr_nothrow<IUnknown> unk{};
                    RETURN_IF_FAILED(::SafeArrayGetElement(storage_t::get(), indices, unk.put()));
                    RETURN_IF_FAILED(unk.query_to(val.put()));
                    return S_OK;
                }());
        }
        template<typename I, typename com_err_policy, typename T = element_t, typename wistd::enable_if<is_interface_type<T>::value, int>::type = 0>
        result put_element(LONG* indices, wil::com_ptr_t<I, com_err_policy>& val)
        {
            WI_ASSERT(sizeof(T) == elemsize());
            return err_policy::HResult(::SafeArrayPutElement(storage_t::get(), indices, val.get()));
        }
#endif
        //! @}

        //! Non-error code based helper functions that are easier to read
        //! @{

        //! Returns the lower bound of the given dimension
        WI_NODISCARD LONG lbound(UINT dim = 1) const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            LONG result = 0;
            lbound(dim, &result);
            return result;
        }
        //! Returns the bupper bound of the given dimension
        WI_NODISCARD LONG ubound(UINT dim = 1) const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            LONG result = 0;
            ubound(dim, &result);
            return result;
        }
        //! Return the total number of elements in the SAFEARRAY
        WI_NODISCARD ULONG size() const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            ULONG result = 0;
            size(&result);
            return result;
        }
        //! Returns the number of elements in the given dimension
        WI_NODISCARD ULONG dim_size(UINT dim = 1) const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            ULONG result = 0;
            dim_size(dim, &result);
            return result;
        }
        //! Returns a copy of the SAFEARRAY, including all individual elements
        WI_NODISCARD unique_type create_copy() const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");

            auto result = unique_type{};
            result.create_copy(storage_t::get());
            return result;
        }

        //! Returns a safearraydata_t that provides direct access to this SAFEARRAY's contents.  See @ref safearraydata_t.
        //! ~~~~
        //! // Create a new safearray by copying a vector of wil::unique_bstr
        //! wil::unique_bstr_safearray copy_to_safearray(std::vector<wil::unique_bstr>& source)
        //! {
        //!     auto sa = wil::unique_bstr_safearray{source.size()};
        //!     auto current = std::begin(source);
        //!     for (BSTR& bstr : sa.access_data())
        //!     {
        //!         // Create a copy for the safearray to own
        //!         bstr = ::SysAllocString((current++)->get());
        //!     }
        //!     return sa;
        //! }
        //! ~~~~
        template<typename T = element_t, typename wistd::enable_if<is_type_set<T>::value, int>::type = 0>
        WI_NODISCARD safearraydata_t<T> access_data() const
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            return safearraydata_t<T>{ storage_t::get() };
        }

        //! Special Get Functions
        //! ~~~~
        //! // Create a new safearray by copying a vector of wil::unique_bstr
        //! std::vector<wil::unique_bstr> copy_safearray_to_vector_and_destroy(SAFEARRAY*& psa)
        //! {
        //!     auto sa = wil::unique_bstr_safearray{};
        //!     sa.reset(psa);  // The SAFEARRAY is now owned by the instance of unique_safearray
        //!     psa = nullptr;  // Signal to the caller the SA was cleaned up
        //!
        //!     THROW_HR_IF(E_UNEXPECTED, sa.vartype() != VT_BSTR);
        //!
        //!     auto result = std::vector<wil::unique_bstr>{};
        //!     result.reserve(sa.size());
        //!     for (auto i = 0; i < sa.size(); ++i)
        //!     {
        //!         result.emplace_back(sa.get_element(i));
        //!     }
        //!     return result;
        //! }
        //! ~~~~
#if defined(__WIL_OLEAUTO_H_)
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG index) -> wil::unique_bstr
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::unique_bstr result{};
            get_element(index, result);
            return result;
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG index) -> wil::unique_variant
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::unique_variant result{};
            get_element(index, result);
            return result;
        }

        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG* indices) -> wil::unique_bstr
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::unique_bstr result{};
            get_element(indices, result);
            return result;
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG* indices) -> wil::unique_variant
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::unique_variant result{};
            get_element(indices, result);
            return result;
        }
#endif

#if defined(__WIL_COM_INCLUDED)
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, LPUNKNOWN>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG index) -> wil::com_ptr_t<IUnknown, err_policy>
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::com_ptr_t<IUnknown, err_policy> result{};
            get_element(index, result);
            return result;
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, LPDISPATCH>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG index) -> wil::com_ptr_t<IDispatch, err_policy>
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::com_ptr_t<IDispatch, err_policy> result{};
            get_element(index, result);
            return result;
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, LPUNKNOWN>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG* indices) -> wil::com_ptr_t<IUnknown, err_policy>
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::com_ptr_t<IUnknown, err_policy> result{};
            get_element(indices, result);
            return result;
        }
        template<typename T = element_t, typename wistd::enable_if<wistd::is_same<T, LPDISPATCH>::value, int>::type = 0>
        WI_NODISCARD auto get_element(LONG* indices) -> wil::com_ptr_t<IDispatch, err_policy>
        {
            static_assert(wistd::is_same<void, result>::value, "this method requires exceptions or failfast");
            wil::com_ptr_t<IDispatch, err_policy> result{};
            get_element(indices, result);
            return result;
        }
#endif
        //! @}

    private:
        HRESULT _create(VARTYPE vt, UINT dims, SAFEARRAYBOUND* sab)
        {
            auto psa = storage_t::policy::invalid_value();
            RETURN_IF_FAILED(details::SafeArrayCreate(vt, dims, sab, psa));
            storage_t::reset(psa);
            return S_OK;
        }
    };

    template<typename T>
    using unique_safearray_nothrow_t = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, T>>;
    template<typename T>
    using unique_safearray_failfast_t = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, T>>;

    using unique_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, void>>;
    using unique_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, void>>;
    using unique_char_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, char>>;
    using unique_char_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, char>>;
    //using unique_short_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, short>>;
    //using unique_short_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, short>>;
    using unique_long_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, long>>;
    using unique_long_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, long>>;
    using unique_int_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, int>>;
    using unique_int_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, int>>;
    using unique_longlong_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, long long>>;
    using unique_longlong_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, long long>>;
    using unique_byte_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, byte>>;
    using unique_byte_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, byte>>;
    using unique_word_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, WORD>>;
    using unique_word_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, WORD>>;
    using unique_dword_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, DWORD>>;
    using unique_dword_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, DWORD>>;
    using unique_ulonglong_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, ULONGLONG>>;
    using unique_ulonglong_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, ULONGLONG>>;
    using unique_float_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, float>>;
    using unique_float_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, float>>;
    //using unique_double_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, double>>;
    //using unique_double_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, double>>;
    using unique_varbool_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, VARIANT_BOOL>>;
    using unique_varbool_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, VARIANT_BOOL>>;
    using unique_date_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, DATE>>;
    using unique_date_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, DATE>>;
    using unique_currency_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, CURRENCY>>;
    using unique_currency_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, CURRENCY>>;
    using unique_decimal_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, DECIMAL>>;
    using unique_decimal_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, DECIMAL>>;
    using unique_bstr_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, BSTR>>;
    using unique_bstr_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, BSTR>>;
    using unique_unknown_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, LPUNKNOWN>>;
    using unique_unknown_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, LPUNKNOWN>>;
    using unique_dispatch_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, LPDISPATCH>>;
    using unique_dispatch_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, LPDISPATCH>>;
    using unique_variant_safearray_nothrow = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_returncode_policy, VARIANT>>;
    using unique_variant_safearray_failfast = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_failfast_policy, VARIANT>>;

#ifdef WIL_ENABLE_EXCEPTIONS
    template<typename T>
    using unique_safearray_t = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, T>>;

    using unique_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, void>>;
    using unique_char_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, char>>;
    //using unique_short_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, short>>;
    using unique_long_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, long>>;
    using unique_int_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, int>>;
    using unique_longlong_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, long long>>;
    using unique_byte_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, byte>>;
    using unique_word_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, WORD>>;
    using unique_dword_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, DWORD>>;
    using unique_ulonglong_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, ULONGLONG>>;
    using unique_float_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, float>>;
    //using unique_double_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, double>>;
    using unique_varbool_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, VARIANT_BOOL>>;
    using unique_date_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, DATE>>;
    using unique_currency_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, CURRENCY>>;
    using unique_decimal_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, DECIMAL>>;
    using unique_bstr_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, BSTR>>;
    using unique_unknown_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, LPUNKNOWN>>;
    using unique_dispatch_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, LPDISPATCH>>;
    using unique_variant_safearray = unique_any_t<safearray_t<details::unique_storage<details::safearray_resource_policy>, err_exception_policy, VARIANT>>;
#endif

#endif  // #if !defined(__WIL_SAFEARRAY_)

#if defined(__WIL_SAFEARRAY_) && !defined(__WIL_SAFEARRAY_SHARED_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_SAFEARRAY_SHARED_STL
    template<typename T>
    using shared_safearray_nothrow_t = shared_any<unique_safearray_nothrow_t<T>>;
    template<typename T>
    using shared_safearray_failfast_t = shared_any<unique_safearray_failfast_t<T>>;

    using shared_safearray_nothrow = shared_any<unique_safearray_nothrow>;
    using shared_safearray_failfast = shared_any<unique_safearray_failfast>;
    using shared_char_safearray_nothrow = shared_any<unique_char_safearray_nothrow>;
    using shared_char_safearray_failfast = shared_any<unique_char_safearray_failfast>;
    //using shared_short_safearray_nothrow = shared_any<unique_short_safearray_nothrow>;
    //using shared_short_safearray_failfast = shared_any<unique_short_safearray_failfast>;
    using shared_long_safearray_nothrow = shared_any<unique_long_safearray_nothrow>;
    using shared_long_safearray_failfast = shared_any<unique_long_safearray_failfast>;
    using shared_int_safearray_nothrow = shared_any<unique_int_safearray_nothrow>;
    using shared_int_safearray_failfast = shared_any<unique_int_safearray_failfast>;
    using shared_longlong_safearray_nothrow = shared_any<unique_longlong_safearray_nothrow>;
    using shared_longlong_safearray_failfast = shared_any<unique_longlong_safearray_failfast>;
    using shared_byte_safearray_nothrow = shared_any<unique_byte_safearray_nothrow>;
    using shared_byte_safearray_failfast = shared_any<unique_byte_safearray_failfast>;
    using shared_word_safearray_nothrow = shared_any<unique_word_safearray_nothrow>;
    using shared_word_safearray_failfast = shared_any<unique_word_safearray_failfast>;
    using shared_dword_safearray_nothrow = shared_any<unique_dword_safearray_nothrow>;
    using shared_dword_safearray_failfast = shared_any<unique_dword_safearray_failfast>;
    using shared_ulonglong_safearray_nothrow = shared_any<unique_ulonglong_safearray_nothrow>;
    using shared_ulonglong_safearray_failfast = shared_any<unique_ulonglong_safearray_failfast>;
    using shared_float_safearray_nothrow = shared_any<unique_float_safearray_nothrow>;
    using shared_float_safearray_failfast = shared_any<unique_float_safearray_failfast>;
    //using shared_double_safearray_nothrow = shared_any<unique_double_safearray_nothrow>;
    //using shared_double_safearray_failfast = shared_any<unique_double_safearray_failfast>;
    using shared_varbool_safearray_nothrow = shared_any<unique_varbool_safearray_nothrow>;
    using shared_varbool_safearray_failfast = shared_any<unique_varbool_safearray_failfast>;
    using shared_date_safearray_nothrow = shared_any<unique_date_safearray_nothrow>;
    using shared_date_safearray_failfast = shared_any<unique_date_safearray_failfast>;
    using shared_currency_safearray_nothrow = shared_any<unique_currency_safearray_nothrow>;
    using shared_currency_safearray_failfast = shared_any<unique_currency_safearray_failfast>;
    using shared_decimal_safearray_nothrow = shared_any<unique_decimal_safearray_nothrow>;
    using shared_decimal_safearray_failfast = shared_any<unique_decimal_safearray_failfast>;
    using shared_bstr_safearray_nothrow = shared_any<unique_bstr_safearray_nothrow>;
    using shared_bstr_safearray_failfast = shared_any<unique_bstr_safearray_failfast>;
    using shared_unknown_safearray_nothrow = shared_any<unique_unknown_safearray_nothrow>;
    using shared_unknown_safearray_failfast = shared_any<unique_unknown_safearray_failfast>;
    using shared_dispatch_safearray_nothrow = shared_any<unique_dispatch_safearray_nothrow>;
    using shared_dispatch_safearray_failfast = shared_any<unique_dispatch_safearray_failfast>;
    using shared_variant_safearray_nothrow = shared_any<unique_variant_safearray_nothrow>;
    using shared_variant_safearray_failfast = shared_any<unique_variant_safearray_failfast>;

#ifdef WIL_ENABLE_EXCEPTIONS
    template<typename T>
    using shared_safearray_t = shared_any<unique_safearray_t<T>>;

    using shared_safearray = shared_any<unique_safearray>;
    using shared_char_safearray = shared_any<unique_char_safearray>;
    //using shared_short_safearray = shared_any<unique_short_safearray>;
    using shared_long_safearray = shared_any<unique_long_safearray>;
    using shared_int_safearray = shared_any<unique_int_safearray>;
    using shared_longlong_safearray = shared_any<unique_longlong_safearray>;
    using shared_byte_safearray = shared_any<unique_byte_safearray>;
    using shared_word_safearray = shared_any<unique_word_safearray>;
    using shared_dword_safearray = shared_any<unique_dword_safearray>;
    using shared_ulonglong_safearray = shared_any<unique_ulonglong_safearray>;
    using shared_float_safearray = shared_any<unique_float_safearray>;
    //using shared_double_safearray = shared_any<unique_double_safearray>;
    using shared_varbool_safearray = shared_any<unique_varbool_safearray>;
    using shared_date_safearray = shared_any<unique_date_safearray>;
    using shared_currency_safearray = shared_any<unique_currency_safearray>;
    using shared_decimal_safearray = shared_any<unique_decimal_safearray>;
    using shared_bstr_safearray = shared_any<unique_bstr_safearray>;
    using shared_unknown_safearray = shared_any<unique_unknown_safearray>;
    using shared_dispatch_safearray = shared_any<unique_dispatch_safearray>;
    using shared_variant_safearray = shared_any<unique_variant_safearray>;
#endif

    template<typename T>
    using weak_safearray_nothrow_t = weak_any<unique_safearray_nothrow_t<T>>;
    template<typename T>
    using weak_safearray_failfast_t = weak_any<unique_safearray_failfast_t<T>>;

    using weak_safearray_nothrow = weak_any<unique_safearray_nothrow>;
    using weak_safearray_failfast = weak_any<unique_safearray_failfast>;
    using weak_char_safearray_nothrow = weak_any<unique_char_safearray_nothrow>;
    using weak_char_safearray_failfast = weak_any<unique_char_safearray_failfast>;
    //using weak_short_safearray_nothrow = weak_any<unique_short_safearray_nothrow>;
    //using weak_short_safearray_failfast = weak_any<unique_short_safearray_failfast>;
    using weak_long_safearray_nothrow = weak_any<unique_long_safearray_nothrow>;
    using weak_long_safearray_failfast = weak_any<unique_long_safearray_failfast>;
    using weak_int_safearray_nothrow = weak_any<unique_int_safearray_nothrow>;
    using weak_int_safearray_failfast = weak_any<unique_int_safearray_failfast>;
    using weak_longlong_safearray_nothrow = weak_any<unique_longlong_safearray_nothrow>;
    using weak_longlong_safearray_failfast = weak_any<unique_longlong_safearray_failfast>;
    using weak_byte_safearray_nothrow = weak_any<unique_byte_safearray_nothrow>;
    using weak_byte_safearray_failfast = weak_any<unique_byte_safearray_failfast>;
    using weak_word_safearray_nothrow = weak_any<unique_word_safearray_nothrow>;
    using weak_word_safearray_failfast = weak_any<unique_word_safearray_failfast>;
    using weak_dword_safearray_nothrow = weak_any<unique_dword_safearray_nothrow>;
    using weak_dword_safearray_failfast = weak_any<unique_dword_safearray_failfast>;
    using weak_ulonglong_safearray_nothrow = weak_any<unique_ulonglong_safearray_nothrow>;
    using weak_ulonglong_safearray_failfast = weak_any<unique_ulonglong_safearray_failfast>;
    using weak_float_safearray_nothrow = weak_any<unique_float_safearray_nothrow>;
    using weak_float_safearray_failfast = weak_any<unique_float_safearray_failfast>;
    //using weak_double_safearray_nothrow = weak_any<unique_double_safearray_nothrow>;
    //using weak_double_safearray_failfast = weak_any<unique_double_safearray_failfast>;
    using weak_varbool_safearray_nothrow = weak_any<unique_varbool_safearray_nothrow>;
    using weak_varbool_safearray_failfast = weak_any<unique_varbool_safearray_failfast>;
    using weak_date_safearray_nothrow = weak_any<unique_date_safearray_nothrow>;
    using weak_date_safearray_failfast = weak_any<unique_date_safearray_failfast>;
    using weak_currency_safearray_nothrow = weak_any<unique_currency_safearray_nothrow>;
    using weak_currency_safearray_failfast = weak_any<unique_currency_safearray_failfast>;
    using weak_decimal_safearray_nothrow = weak_any<unique_decimal_safearray_nothrow>;
    using weak_decimal_safearray_failfast = weak_any<unique_decimal_safearray_failfast>;
    using weak_bstr_safearray_nothrow = weak_any<unique_bstr_safearray_nothrow>;
    using weak_bstr_safearray_failfast = weak_any<unique_bstr_safearray_failfast>;
    using weak_unknown_safearray_nothrow = weak_any<unique_unknown_safearray_nothrow>;
    using weak_unknown_safearray_failfast = weak_any<unique_unknown_safearray_failfast>;
    using weak_dispatch_safearray_nothrow = weak_any<unique_dispatch_safearray_nothrow>;
    using weak_dispatch_safearray_failfast = weak_any<unique_dispatch_safearray_failfast>;
    using weak_variant_safearray_nothrow = weak_any<unique_variant_safearray_nothrow>;
    using weak_variant_safearray_failfast = weak_any<unique_variant_safearray_failfast>;

#ifdef WIL_ENABLE_EXCEPTIONS
    template<typename T>
    using weak_safearray_t = weak_any<unique_safearray_t<T>>;

    using weak_safearray = weak_any<unique_safearray>;
    using weak_char_safearray = weak_any<unique_char_safearray>;
    //using weak_short_safearray = weak_any<unique_short_safearray>;
    using weak_long_safearray = weak_any<unique_long_safearray>;
    using weak_int_safearray = weak_any<unique_int_safearray>;
    using weak_longlong_safearray = weak_any<unique_longlong_safearray>;
    using weak_byte_safearray = weak_any<unique_byte_safearray>;
    using weak_word_safearray = weak_any<unique_word_safearray>;
    using weak_dword_safearray = weak_any<unique_dword_safearray>;
    using weak_ulonglong_safearray = weak_any<unique_ulonglong_safearray>;
    using weak_float_safearray = weak_any<unique_float_safearray>;
    //using weak_double_safearray = weak_any<unique_double_safearray>;
    using weak_varbool_safearray = weak_any<unique_varbool_safearray>;
    using weak_date_safearray = weak_any<unique_date_safearray>;
    using weak_currency_safearray = weak_any<unique_currency_safearray>;
    using weak_decimal_safearray = weak_any<unique_decimal_safearray>;
    using weak_bstr_safearray = weak_any<unique_bstr_safearray>;
    using weak_unknown_safearray = weak_any<unique_unknown_safearray>;
    using weak_dispatch_safearray = weak_any<unique_dispatch_safearray>;
    using weak_variant_safearray = weak_any<unique_variant_safearray>;
#endif

#endif // __WIL_SAFEARRAY_SHARED_STL

} // namespace wil
