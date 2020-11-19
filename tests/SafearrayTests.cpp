#include <wil/resource.h>
#include <wil/com.h>
#include <wil/safearrays.h>

#include "common.h"
#include <array>
#include <propvarutil.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#pragma comment( lib, "Propsys.lib" )

#define RUN_TYPED_TEST_NOTHROW(test)        WI_FOREACH(test                                 \
                                                , wil::unique_char_safearray_nothrow        \
                                                , wil::unique_long_safearray_nothrow        \
                                                , wil::unique_int_safearray_nothrow         \
                                                , wil::unique_longlong_safearray_nothrow    \
                                                , wil::unique_byte_safearray_nothrow        \
                                                , wil::unique_word_safearray_nothrow        \
                                                , wil::unique_dword_safearray_nothrow       \
                                                , wil::unique_ulonglong_safearray_nothrow   \
                                                , wil::unique_float_safearray_nothrow       \
                                                , wil::unique_varbool_safearray_nothrow     \
                                                , wil::unique_date_safearray_nothrow        \
                                                , wil::unique_bstr_safearray_nothrow        \
                                                , wil::unique_unknown_safearray_nothrow     \
                                                , wil::unique_dispatch_safearray_nothrow    \
                                                , wil::unique_variant_safearray_nothrow     \
                                               )

#define RUN_TYPED_TEST_FAILFAST(test)        WI_FOREACH(test                                \
                                                , wil::unique_char_safearray_failfast       \
                                                , wil::unique_long_safearray_failfast       \
                                                , wil::unique_int_safearray_failfast        \
                                                , wil::unique_longlong_safearray_failfast   \
                                                , wil::unique_byte_safearray_failfast       \
                                                , wil::unique_word_safearray_failfast       \
                                                , wil::unique_dword_safearray_failfast      \
                                                , wil::unique_ulonglong_safearray_failfast  \
                                                , wil::unique_float_safearray_failfast      \
                                                , wil::unique_varbool_safearray_failfast    \
                                                , wil::unique_date_safearray_failfast       \
                                                , wil::unique_bstr_safearray_failfast       \
                                                , wil::unique_unknown_safearray_failfast    \
                                                , wil::unique_dispatch_safearray_failfast   \
                                                , wil::unique_variant_safearray_failfast    \
                                            )

#define RUN_TYPED_TEST(test)                WI_FOREACH(test                                 \
                                                , wil::unique_char_safearray                \
                                                , wil::unique_long_safearray                \
                                                , wil::unique_int_safearray                 \
                                                , wil::unique_longlong_safearray            \
                                                , wil::unique_byte_safearray                \
                                                , wil::unique_word_safearray                \
                                                , wil::unique_dword_safearray               \
                                                , wil::unique_ulonglong_safearray           \
                                                , wil::unique_float_safearray               \
                                                , wil::unique_varbool_safearray             \
                                                , wil::unique_date_safearray                \
                                                , wil::unique_bstr_safearray                \
                                                , wil::unique_unknown_safearray             \
                                                , wil::unique_dispatch_safearray            \
                                                , wil::unique_variant_safearray             \
                                            )

#define RUN_TEST(test)                      WI_FOREACH(test                                 \
                                                , char                                      \
                                                , short                                     \
                                                , long                                      \
                                                , int                                       \
                                                , long long                                 \
                                                , unsigned char                             \
                                                , unsigned short                            \
                                                , unsigned long                             \
                                                , unsigned int                              \
                                                , unsigned long long                        \
                                                , float                                     \
                                                , double                                    \
                                                , VARIANT_BOOL                              \
                                                , DATE                                      \
                                                , BSTR                                      \
                                                , LPUNKNOWN                                 \
                                                , LPDISPATCH                                \
                                                , VARIANT                                   \
                                            )

#define RUN_TEST_SPECIAL(test)              WI_FOREACH(test                                 \
                                                , BSTR                                      \
                                                , LPUNKNOWN                                 \
                                                , LPDISPATCH                                \
                                                , VARIANT                                   \
                                            )


// {5D80EC64-6694-4F49-B0B9-CCAA65467D12}
//static const GUID IID_IAmForTesting =
//{ 0x5d80ec64, 0x6694, 0x4f49, { 0xb0, 0xb9, 0xcc, 0xaa, 0x65, 0x46, 0x7d, 0x12 } };

interface __declspec(uuid("5D80EC64-6694-4F49-B0B9-CCAA65467D12"))
    IAmForTesting : public IUnknown
{
    STDMETHOD(GetID)(LONG* pOut) = 0;
};

class TestComObject : virtual public IAmForTesting,
                        virtual public IDispatch
{
private:
    TestComObject()
    {
        ::InterlockedIncrement(&s_ObjectCount);
        m_nID = ::InterlockedIncrement(&s_IDCount);
    }

    virtual ~TestComObject()
    {
        ::InterlockedDecrement(&s_ObjectCount);
    }

    // IUnknown
public:
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        ::InterlockedIncrement(&s_GlobalRefCount);
        ::InterlockedIncrement(&m_nRefCount);
        return m_nRefCount;
    }
    IFACEMETHODIMP_(ULONG) Release()
    {
        ::InterlockedDecrement(&s_GlobalRefCount);
        ULONG ulRefCount = ::InterlockedDecrement(&m_nRefCount);
        if (0 == m_nRefCount)
        {
            delete this;
        }
        return ulRefCount;
    }
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
    {
        if (!ppv)
            return E_INVALIDARG;
        *ppv = NULL;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IAmForTesting))
        {
            *ppv = (LPVOID)(IAmForTesting*)this;
            AddRef();
            return NOERROR;
        }
        else if (riid == __uuidof(IDispatch))
        {
            *ppv = (LPVOID)(IDispatch*)this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    // IDispatch
public:
    STDMETHOD(GetTypeInfoCount)(UINT* /*pctinfo*/)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetTypeInfo)(UINT /*itinfo*/, LCID /*lcid*/, ITypeInfo** /*pptinfo*/)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetIDsOfNames)(REFIID /*riid*/, LPOLESTR* /*rgszNames*/, UINT /*cNames*/, LCID /*lcid*/, DISPID* /*rgdispid*/)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Invoke)(DISPID /*dispidMember*/, REFIID /*riid*/, LCID /*lcid*/, WORD /*wFlags*/, DISPPARAMS* /*pdispparams*/, VARIANT* /*pvarResult*/, EXCEPINFO* /*pexcepinfo*/, UINT* /*puArgErr*/)
    {
        return E_NOTIMPL;
    }

    // IAmForTesting
public:
    STDMETHOD(GetID)(LONG* pOut)
    {
        if (!pOut) return E_POINTER;
        *pOut = m_nID;
        return S_OK;
    }

    // Factory Method
public:
    static HRESULT Create(REFIID riid, LPVOID* ppvObj)
    {
        auto* p = new (std::nothrow) TestComObject();
        if (!p) return E_OUTOFMEMORY;
        auto hr = p->QueryInterface(riid, ppvObj);
        if (FAILED(hr))
        {
            delete p;
        }
        return hr;
    }

    static bool Compare(LPUNKNOWN left, LPUNKNOWN right)
    {
        if (left == nullptr && right == nullptr)
        {
            return true;
        }
        if (left == nullptr || right == nullptr)
        {
            return false;
        }
        if (left == right)
        {
            return true;
        }

        wil::com_ptr_nothrow<IAmForTesting> spLeft;
        wil::com_ptr_nothrow<IAmForTesting> spRight;

        if (SUCCEEDED(left->QueryInterface(&spLeft)) && SUCCEEDED(right->QueryInterface(&spRight)))
        {
            LONG nLeft = 0;
            LONG nRight = 0;

            REQUIRE_SUCCEEDED(spLeft->GetID(&nLeft));
            REQUIRE_SUCCEEDED(spRight->GetID(&nRight));

            return (nLeft == nRight);
        }
        else
        {
            return false;
        }
    }

    static LONG ObjectCount() { return s_ObjectCount; }
    static LONG GlobalRefCount() { return s_GlobalRefCount; }

private:
    LONG m_nRefCount{};
    LONG m_nID{};

    static LONG s_ObjectCount;
    static LONG s_GlobalRefCount;
    static LONG s_IDCount;
};

LONG TestComObject::s_ObjectCount = 0;
LONG TestComObject::s_GlobalRefCount = 0;
LONG TestComObject::s_IDCount = 0;

constexpr auto DEFAULT_SAMPLE_SIZE = 192U;

template<typename T, typename = typename wistd::enable_if<wistd::is_integral<T>::value, int>::type>
std::array<T, sizeof(T) * 8> GetSampleData()
{
    constexpr auto BIT_COUNT = sizeof(T) * 8;

    auto result = std::array<T, BIT_COUNT>{};
    for (auto i = 0U; i < BIT_COUNT; ++i)
    {
        result[i] = (static_cast<T>(1) << i);
    }
    return result;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_floating_point<T>::value, int>::type>
std::array<T, DEFAULT_SAMPLE_SIZE> GetSampleData()
{
    auto result = std::array<T, DEFAULT_SAMPLE_SIZE>{};
    for (auto i = 0U; i < DEFAULT_SAMPLE_SIZE/2; ++i)
    {
        result[i] = (i != 0) ? static_cast<T>(1 / i) : 0;
        result[i + (DEFAULT_SAMPLE_SIZE/2)] = 2 * static_cast<T>(i);
    }
    return result;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, BSTR>::value, int>::type>
std::array<wil::unique_bstr, DEFAULT_SAMPLE_SIZE> GetSampleData()
{
    auto result = std::array<wil::unique_bstr, DEFAULT_SAMPLE_SIZE>{};
    for (auto i = 0U; i < DEFAULT_SAMPLE_SIZE; ++i)
    {
        switch (i % 4)
        {
        case 0:
            result[i].reset(::SysAllocString(L"Sample Data"));
            break;
        case 1:
            result[i].reset(::SysAllocString(L"Larger Sample Data"));
            break;
        case 2:
            result[i].reset(::SysAllocString(L"This is much much larger Sample Data"));
            break;
        case 3:
            result[i].reset(::SysAllocString(L"This is the longest Sample Data.  It's the longest by a lot.  I mean a lot."));
            break;
        }
    }
    return result;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, VARIANT>::value, int>::type>
std::array<wil::unique_variant, DEFAULT_SAMPLE_SIZE> GetSampleData()
{
    auto result = std::array<wil::unique_variant, DEFAULT_SAMPLE_SIZE>{};
    for (auto i = 0U; i < DEFAULT_SAMPLE_SIZE; ++i)
    {
        auto& var = result[i];
        switch (i % 6)
        {
        case 0:
            V_VT(&var) = VT_I4;
            V_I4(&var) = 37;
            break;
        case 1:
            V_VT(&var) = VT_I1;
            V_I1(&var) = 0x40;
            break;
        case 2:
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = ::SysAllocString(L"String in a variant");
            break;
        case 3:
            V_VT(&var) = VT_R4;
            V_R4(&var) = 98.6f;
            break;
        case 4:
            V_VT(&var) = VT_UNKNOWN;
            REQUIRE_SUCCEEDED(TestComObject::Create(IID_IUnknown, (LPVOID*)&(V_UNKNOWN(&var))));
            break;
        case 5:
            V_VT(&var) = VT_DISPATCH;
            REQUIRE_SUCCEEDED(TestComObject::Create(IID_IDispatch, (LPVOID*)&(V_DISPATCH(&var))));
            break;
        }
    }
    return result;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, LPUNKNOWN>::value, int>::type>
std::array<wil::com_ptr_nothrow<IAmForTesting>, DEFAULT_SAMPLE_SIZE> GetSampleData()
{
    auto result = std::array<wil::com_ptr_nothrow<IAmForTesting>, DEFAULT_SAMPLE_SIZE>{};
    for (auto i = 0U; i < DEFAULT_SAMPLE_SIZE; ++i)
    {
        auto& var = result[i];
        REQUIRE_SUCCEEDED(TestComObject::Create(__uuidof(IAmForTesting), (LPVOID*)var.put()));
    }
    return result;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, LPDISPATCH>::value, int>::type>
std::array<wil::com_ptr_nothrow<IDispatch>, DEFAULT_SAMPLE_SIZE> GetSampleData()
{
    auto result = std::array<wil::com_ptr_nothrow<IDispatch>, DEFAULT_SAMPLE_SIZE>{};
    for (auto i = 0U; i < DEFAULT_SAMPLE_SIZE; ++i)
    {
        auto& var = result[i];
        REQUIRE_SUCCEEDED(TestComObject::Create(__uuidof(IDispatch), (LPVOID*)var.put()));
    }
    return result;
}

template <typename T, typename = typename wistd::enable_if<!wistd::is_same<T, wil::unique_bstr>::value
                                                            && !wistd::is_same<T, wil::com_ptr_nothrow<IAmForTesting>>::value
                                                            && !wistd::is_same<T, wil::com_ptr_nothrow<IDispatch>>::value, int>::type>
auto GetReadable(const T& t) -> const T&
{
    return t;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::unique_bstr>::value, int>::type>
auto GetReadable(const T& t) -> BSTR
{
    return t.get();
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::com_ptr_nothrow<IAmForTesting>>::value, int>::type>
auto GetReadable(const T& t) -> LPUNKNOWN
{
    return t.get();
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::com_ptr_nothrow<IDispatch>>::value, int>::type>
auto GetReadable(const T& t) -> LPDISPATCH
{
    return t.get();
}

template <typename T, typename = typename wistd::enable_if<!wistd::is_same<T, wil::unique_bstr>::value
                                                            && !wistd::is_same<T, wil::com_ptr_nothrow<IAmForTesting>>::value
                                                            && !wistd::is_same<T, wil::com_ptr_nothrow<IDispatch>>::value, int>::type>
auto GetWritable(T& t) -> T&
{
    return t;
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::unique_bstr>::value, int>::type>
auto GetWritable(T& t) -> BSTR&
{
    return *(t.addressof());
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::com_ptr_nothrow<IAmForTesting>>::value, int>::type>
auto GetWritable(T& t) -> LPUNKNOWN&
{
    return *((LPUNKNOWN*)t.addressof());
}

template<typename T, typename = typename wistd::enable_if<wistd::is_same<T, wil::com_ptr_nothrow<IDispatch>>::value, int>::type>
auto GetWritable(T& t) -> LPDISPATCH&
{
    return *(t.addressof());
}

template<typename T, typename U>
bool PerformCompare(const T& left, const U& right)
{
    return (left == right);
}

template<>
bool PerformCompare(_In_z_ const BSTR& left, _In_z_ const BSTR& right)
{
    return (::SysStringLen(left) == ::SysStringLen(right))
        && (wcscmp(left, right) == 0);
}

template<>
bool PerformCompare(const BSTR& left, const wil::unique_bstr& right)
{
    return PerformCompare(left, right.get());
}

template<>
bool PerformCompare(const wil::unique_bstr& left, const BSTR& right)
{
    return PerformCompare(left.get(), right);
}

template<>
bool PerformCompare(const wil::unique_bstr& left, const wil::unique_bstr& right)
{
    return PerformCompare(left.get(), right.get());
}

template<>
bool PerformCompare(const wil::unique_variant& left, const wil::unique_variant& right)
{
    return (::VariantCompare(left, right) == 0);
}

template<>
bool PerformCompare(const VARIANT& left, const wil::unique_variant& right)
{
    return (::VariantCompare(left, right) == 0);
}

template<>
bool PerformCompare(const VARIANT& left, const VARIANT& right)
{
    return (::VariantCompare(left, right) == 0);
}

template<>
bool PerformCompare(const LPUNKNOWN& left, const LPUNKNOWN& right)
{
    return TestComObject::Compare(left, right);
}

template<>
bool PerformCompare(const wil::com_ptr_nothrow<IAmForTesting>& left, const LPUNKNOWN& right)
{
    return TestComObject::Compare(left.get(), right);
}

template<>
bool PerformCompare(const LPUNKNOWN& left, const wil::com_ptr_nothrow<IAmForTesting>& right)
{
    return TestComObject::Compare(left, right.get());
}

template<typename T, typename U>
void PerfromAssignment(T& t, const U& u)
{
    t = u;
}

template<>
void PerfromAssignment(VARIANT& t, const wil::unique_variant& u)
{
    FAIL_FAST_IF_FAILED(::VariantCopy(&t, &u));
}

template<>
void PerfromAssignment(BSTR& t, const wil::unique_bstr& u)
{
    t = ::SysAllocString(u.get());
}

template<>
void PerfromAssignment(LPUNKNOWN& t, const wil::com_ptr_nothrow<IAmForTesting>& u)
{
    if (t)
    {
        t->Release();
        t = nullptr;
    }
    if (u)
    {
        REQUIRE_SUCCEEDED(u->QueryInterface<IUnknown>(&t));
    }
}

template<typename I>
void PerfromAssignment(I*& t, const wil::com_ptr_nothrow<I>& u)
{
    if (t)
    {
        t->Release();
        t = nullptr;
    }
    if (u)
    {
        REQUIRE_SUCCEEDED(u->QueryInterface(IID_PPV_ARGS(&t)));
    }
}

template<typename safearray_t>
void TestLock(safearray_t& sa)
{
    REQUIRE(sa);
    const auto startingLocks = sa.lock_count();
    {
        auto lock = sa.scope_lock();
        REQUIRE(sa.lock_count() > startingLocks); // Verify Lock Count increased
    }
    REQUIRE(startingLocks == sa.lock_count());   // Verify it dropped back down
}

template<typename safearray_t>
void TestTyped_Create_NoThrow()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = safearray_t{};
    LONG val = 0;
    ULONG size = 0;
    REQUIRE_SUCCEEDED(sa.create(SIZE));
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(typename safearray_t::elemtype));
    REQUIRE_SUCCEEDED(sa.size(&size));
    REQUIRE(size == SIZE);
    REQUIRE_SUCCEEDED(sa.lbound(&val));
    REQUIRE(val == 0);
    REQUIRE_SUCCEEDED(sa.ubound(&val));
    REQUIRE(val == SIZE - 1);
    TestLock<safearray_t>(sa);
    sa.reset();
    REQUIRE(!sa);
}

template<typename safearray_t>
void TestTyped_Create_FailFast()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = safearray_t{};
    LONG val = 0;
    ULONG size = 0;
    REQUIRE_NOCRASH(sa.create(SIZE));
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(typename safearray_t::elemtype));
    REQUIRE_NOCRASH(sa.size(&size));
    REQUIRE(size == SIZE);
    REQUIRE_NOCRASH(sa.lbound(&val));
    REQUIRE(val == 0);
    REQUIRE_NOCRASH(sa.ubound(&val));
    REQUIRE(val == SIZE - 1);
    TestLock<safearray_t>(sa);
    sa.reset();
    REQUIRE(!sa);
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename safearray_t>
void TestTyped_Create()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = safearray_t{};
    REQUIRE_NOTHROW(sa = safearray_t{ SIZE });
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(typename safearray_t::elemtype));
    REQUIRE_NOTHROW(sa.size() == SIZE);
    REQUIRE_NOTHROW(sa.lbound() == 0);
    REQUIRE_NOTHROW(sa.ubound() == SIZE-1);
    TestLock<safearray_t>(sa);
    sa.reset();
    REQUIRE(!sa);
}
#endif

#define _TYPED_CREATE_NOTHROW(type)         TestTyped_Create_NoThrow<type>();
#define _TYPED_CREATE_FAILFAST(type)        TestTyped_Create_FailFast<type>();
#define _TYPED_CREATE(type)                 TestTyped_Create<type>();

template<typename T>
void Test_Create_NoThrow()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = wil::unique_safearray_nothrow{};
    LONG val = 0;
    ULONG size = 0;
    REQUIRE_SUCCEEDED(sa.create<T>(SIZE));
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(T));
    REQUIRE_SUCCEEDED(sa.size(&size));
    REQUIRE(size == SIZE);
    REQUIRE_SUCCEEDED(sa.lbound(&val));
    REQUIRE(val == 0);
    REQUIRE_SUCCEEDED(sa.ubound(&val));
    REQUIRE(val == SIZE - 1);
    TestLock(sa);
    sa.reset();
    REQUIRE(!sa);
}

template<typename T>
void Test_Create_FailFast()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = wil::unique_safearray_failfast{};
    LONG val = 0;
    ULONG size = 0;
    REQUIRE_NOCRASH(sa.create<T>(SIZE));
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(T));
    REQUIRE_NOCRASH(sa.size(&size));
    REQUIRE(size == SIZE);
    REQUIRE_NOCRASH(sa.lbound(&val));
    REQUIRE(val == 0);
    REQUIRE_NOCRASH(sa.ubound(&val));
    REQUIRE(val == SIZE - 1);
    TestLock(sa);
    sa.reset();
    REQUIRE(!sa);
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename T>
void Test_Create()
{
    constexpr auto SIZE = DEFAULT_SAMPLE_SIZE;

    auto sa = wil::unique_safearray{};
    REQUIRE_NOTHROW(sa.create<T>( SIZE ));
    REQUIRE(sa);
    REQUIRE(sa.dims() == 1);
    REQUIRE(sa.elemsize() == sizeof(T));
    REQUIRE_NOTHROW(sa.size() == SIZE);
    REQUIRE_NOTHROW(sa.lbound() == 0);
    REQUIRE_NOTHROW(sa.ubound() == SIZE - 1);
    TestLock(sa);
    sa.reset();
    REQUIRE(!sa);
}
#endif

#define _CREATE_NOTHROW(type)               Test_Create_NoThrow<type>();
#define _CREATE_FAILFAST(type)              Test_Create_FailFast<type>();
#define _CREATE(type)                       Test_Create<type>();

template<typename safearray_t>
void TestTyped_Element_NoThrow()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = safearray_t{};

    REQUIRE_SUCCEEDED(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_SUCCEEDED(sa.put_element(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_SUCCEEDED(sa.get_element(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}

template<typename safearray_t>
void TestTyped_Element_Failfast()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = safearray_t{};

    REQUIRE_NOCRASH(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_NOCRASH(sa.put_element(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_NOCRASH(sa.get_element(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename safearray_t>
void TestTyped_Element()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = safearray_t{};

    REQUIRE_NOTHROW(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_NOTHROW(sa.put_element(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_NOTHROW(sa.get_element(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}
#endif

#define _TYPED_DIRECT_NOTHROW(type)         TestTyped_Element_NoThrow<type>();
#define _TYPED_DIRECT_FAILFAST(type)        TestTyped_Element_Failfast<type>();
#define _TYPED_DIRECT(type)                 TestTyped_Element<type>();

template<typename T>
void Test_Element_NoThrow()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray_nothrow{};

    REQUIRE_SUCCEEDED(sa.create<T>(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_SUCCEEDED(sa.put_element<T>(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_SUCCEEDED(sa.get_element<T>(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}

template<typename T>
void Test_Element_Failfast()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray_failfast{};

    REQUIRE_NOCRASH(sa.create<T>(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_NOCRASH(sa.put_element<T>(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_NOCRASH(sa.get_element<T>(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename T>
void Test_Element()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray{};

    REQUIRE_NOTHROW(sa.create<T>(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        REQUIRE_NOTHROW(sa.put_element<T>(i, GetReadable(sample_data[i])));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        {
            auto temp = data_type{};
            // And make sure it was the value that was set
            REQUIRE_NOTHROW(sa.get_element<T>(i, GetWritable(temp)));
            REQUIRE(PerformCompare(temp, sample_data[i]));
        }
    }
}
#endif

#define _DIRECT_NOTHROW(type)           Test_Element_NoThrow<type>();
#define _DIRECT_FAILFAST(type)          Test_Element_Failfast<type>();
#define _DIRECT(type)                   Test_Element<type>();

template<typename safearray_t>
void TestTyped_AccessData_NoThrow()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_SUCCEEDED(sa.create(SIZE));
        REQUIRE(sa);
        {
            ULONG counter = {};
            wil::safearraydata_nothrow<typename safearray_t::elemtype> data;
            REQUIRE_SUCCEEDED(data.access(sa.get()));
            for (auto& elem : data)
            {
                PerfromAssignment(elem, sample_data[counter++]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = safearray_t{};
        REQUIRE_SUCCEEDED(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            ULONG counter = {};
            wil::safearraydata_nothrow<typename safearray_t::elemtype> data;
            REQUIRE_SUCCEEDED(data.access(sa2.get()));
            for (const auto& elem : data)
            {
                REQUIRE(PerformCompare(elem, sample_data[counter++]));
            }
        }
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_SUCCEEDED(sa.create(SIZE));
        REQUIRE(sa);
        {
            wil::safearraydata_nothrow<typename safearray_t::elemtype> data;
            REQUIRE_SUCCEEDED(data.access(sa.get()));
            for (ULONG i = 0 ; i < data.size(); ++i)
            {
                PerfromAssignment(data[i], sample_data[i]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = safearray_t{};
        REQUIRE_SUCCEEDED(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            wil::safearraydata_nothrow<typename safearray_t::elemtype> data;
            REQUIRE_SUCCEEDED(data.access(sa2.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                REQUIRE(PerformCompare(data[i], sample_data[i]));
            }
        }
    }
}

template<typename safearray_t>
void TestTyped_AccessData_Failfast()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_NOCRASH(sa.create(SIZE));
        REQUIRE(sa);
        {
            ULONG counter = {};
            wil::safearraydata_failfast<typename safearray_t::elemtype> data;
            REQUIRE_NOCRASH(data.access(sa.get()));
            for (auto& elem : data)
            {
                PerfromAssignment(elem, sample_data[counter++]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = safearray_t{};
        REQUIRE_NOCRASH(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            ULONG counter = {};
            wil::safearraydata_failfast<typename safearray_t::elemtype> data;
            REQUIRE_NOCRASH(data.access(sa2.get()));
            for (const auto& elem : data)
            {
                REQUIRE(PerformCompare(elem, sample_data[counter++]));
            }
        }
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_NOCRASH(sa.create(SIZE));
        REQUIRE(sa);
        {
            wil::safearraydata_failfast<typename safearray_t::elemtype> data;
            REQUIRE_NOCRASH(data.access(sa.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                PerfromAssignment(data[i], sample_data[i]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = safearray_t{};
        REQUIRE_NOCRASH(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            wil::safearraydata_failfast<typename safearray_t::elemtype> data;
            REQUIRE_NOCRASH(data.access(sa2.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                REQUIRE(PerformCompare(data[i], sample_data[i]));
            }
        }
    }
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename safearray_t>
void TestTyped_AccessData()
{
    auto sample_data = GetSampleData<typename safearray_t::elemtype>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_NOTHROW(sa.create(SIZE));
        REQUIRE(sa);

        REQUIRE_NOTHROW([&]()
            {
                ULONG counter = {};
                for (auto& elem : sa.access_data())
                {
                    PerfromAssignment(elem, sample_data[counter++]);
                }
            }());

        // Duplicate the SA to make sure copy works
        auto sa2 = safearray_t{};
        REQUIRE_NOTHROW([&]()
            {
                sa2 = sa.create_copy();
                REQUIRE(sa2);
            }());

        REQUIRE_NOTHROW([&]()
            {
                // Verify the values in the copy are the same as
                // the values that were placed into the original
                ULONG counter = {};
                for (const auto& elem : sa.access_data())
                {
                    REQUIRE(PerformCompare(elem, sample_data[counter++]));
                }
            }());
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = safearray_t{};
        REQUIRE_NOTHROW(sa.create(SIZE));
        REQUIRE(sa);

        REQUIRE_NOTHROW([&]()
            {
                auto data = sa.access_data();
                for (ULONG i = 0; i < data.size(); ++i)
                {
                    PerfromAssignment(data[i], sample_data[i]);
                }
            }());

        // Duplicate the SA to make sure copy works
        auto sa2 = safearray_t{};
        REQUIRE_NOTHROW([&]()
            {
                sa2 = sa.create_copy();
                REQUIRE(sa2);
            }());

        REQUIRE_NOTHROW([&]()
            {
                // Verify the values in the copy are the same as
                // the values that were placed into the original
                auto data = sa2.access_data();
                for (ULONG i = 0; i < data.size(); ++i)
                {
                    REQUIRE(PerformCompare(data[i], sample_data[i]));
                }
            }());
    }
}
#endif

#define _TYPED_ACCESSDATA_NOTHROW(type)         TestTyped_AccessData_NoThrow<type>();
#define _TYPED_ACCESSDATA_FAILFAST(type)        TestTyped_AccessData_Failfast<type>();
#define _TYPED_ACCESSDATA(type)                 TestTyped_AccessData<type>();

template<typename T>
void Test_AccessData_NoThrow()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray_nothrow{};
        REQUIRE_SUCCEEDED(sa.create<T>(SIZE));
        REQUIRE(sa);
        {
            ULONG counter = {};
            wil::safearraydata_nothrow<T> data;
            REQUIRE_SUCCEEDED(data.access(sa.get()));
            for (auto& elem : data)
            {
                PerfromAssignment(elem, sample_data[counter++]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = wil::unique_safearray_nothrow{};
        REQUIRE_SUCCEEDED(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            ULONG counter = {};
            wil::safearraydata_nothrow<T> data;
            REQUIRE_SUCCEEDED(data.access(sa2.get()));
            for (const auto& elem : data)
            {
                REQUIRE(PerformCompare(elem, sample_data[counter++]));
            }
        }
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray_nothrow{};
        REQUIRE_SUCCEEDED(sa.create<T>(SIZE));
        REQUIRE(sa);
        {
            wil::safearraydata_nothrow<T> data;
            REQUIRE_SUCCEEDED(data.access(sa.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                PerfromAssignment(data[i], sample_data[i]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = wil::unique_safearray_nothrow{};
        REQUIRE_SUCCEEDED(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            wil::safearraydata_nothrow<T> data;
            REQUIRE_SUCCEEDED(data.access(sa2.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                REQUIRE(PerformCompare(data[i], sample_data[i]));
            }
        }
    }
}

template<typename T>
void Test_AccessData_Failfast()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray_failfast{};
        REQUIRE_NOCRASH(sa.create<T>(SIZE));
        REQUIRE(sa);
        {
            ULONG counter = {};
            wil::safearraydata_failfast<T> data;
            REQUIRE_NOCRASH(data.access(sa.get()));
            for (auto& elem : data)
            {
                PerfromAssignment(elem, sample_data[counter++]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = wil::unique_safearray_failfast{};
        REQUIRE_NOCRASH(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            ULONG counter = {};
            wil::safearraydata_failfast<T> data;
            REQUIRE_NOCRASH(data.access(sa2.get()));
            for (const auto& elem : data)
            {
                REQUIRE(PerformCompare(elem, sample_data[counter++]));
            }
        }
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray_failfast{};
        REQUIRE_NOCRASH(sa.create<T>(SIZE));
        REQUIRE(sa);
        {
            wil::safearraydata_failfast<T> data;
            REQUIRE_NOCRASH(data.access(sa.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                PerfromAssignment(data[i], sample_data[i]);
            }
        }

        // Duplicate the SA to make sure create_copy works
        auto sa2 = wil::unique_safearray_failfast{};
        REQUIRE_NOCRASH(sa2.create_copy(sa.get()));
        REQUIRE(sa2);
        {
            // Verify the values in the copy are the same as
            // the values that were placed into the original
            wil::safearraydata_failfast<T> data;
            REQUIRE_NOCRASH(data.access(sa2.get()));
            for (ULONG i = 0; i < data.size(); ++i)
            {
                REQUIRE(PerformCompare(data[i], sample_data[i]));
            }
        }
    }
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename T>
void Test_AccessData()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    // Operate on the AccessData class using ranged-for
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray{};
        REQUIRE_NOTHROW(sa.create<T>(SIZE));
        REQUIRE(sa);

        REQUIRE_NOTHROW([&]()
            {
                ULONG counter = {};
                for (auto& elem : sa.access_data<T>())
                {
                    PerfromAssignment(elem, sample_data[counter++]);
                }
            }());

        // Duplicate the SA to make sure copy works
        auto sa2 = wil::unique_safearray{};
        REQUIRE_NOTHROW([&]()
            {
                sa2 = sa.create_copy();
                REQUIRE(sa2);
            }());

        REQUIRE_NOTHROW([&]()
            {
                // Verify the values in the copy are the same as
                // the values that were placed into the original
                ULONG counter = {};
                for (const auto& elem : sa.access_data<T>())
                {
                    REQUIRE(PerformCompare(elem, sample_data[counter++]));
                }
            }());
    }

    // Operate on the AccessData class using regular for-loop
    {
        // Create a new SA and copy the sample data into it
        auto sa = wil::unique_safearray{};
        REQUIRE_NOTHROW(sa.create<T>(SIZE));
        REQUIRE(sa);

        REQUIRE_NOTHROW([&]()
            {
                auto data = sa.access_data<T>();
                for (ULONG i = 0; i < data.size(); ++i)
                {
                    PerfromAssignment(data[i], sample_data[i]);
                }
            }());

        // Duplicate the SA to make sure copy works
        auto sa2 = wil::unique_safearray{};
        REQUIRE_NOTHROW([&]()
            {
                sa2 = sa.create_copy();
                REQUIRE(sa2);
            }());

        REQUIRE_NOTHROW([&]()
            {
                // Verify the values in the copy are the same as
                // the values that were placed into the original
                auto data = sa2.access_data<T>();
                for (ULONG i = 0; i < data.size(); ++i)
                {
                    REQUIRE(PerformCompare(data[i], sample_data[i]));
                }
            }());
    }
}
#endif

#define _ACCESSDATA_NOTHROW(type)         Test_AccessData_NoThrow<type>();
#define _ACCESSDATA_FAILFAST(type)        Test_AccessData_Failfast<type>();
#define _ACCESSDATA(type)                 Test_AccessData<type>();

template<typename T>
void TestTyped_Element_Special_NoThrow()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray_nothrow_t<T>{};

    REQUIRE_SUCCEEDED(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // No Need to call GetReadable
        REQUIRE_SUCCEEDED(sa.put_element(i, sample_data[i]));
    }

    // Can use a single instance because the wrapper classes clean up
    auto temp = data_type{};

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // And make sure it was the value that was set
        // No Need to Call GetWritable
        REQUIRE_SUCCEEDED(sa.get_element(i, temp));
        REQUIRE(PerformCompare(temp, sample_data[i]));
    }
}

template<typename T>
void TestTyped_Element_Special_Failfast()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray_failfast_t<T>{};

    REQUIRE_NOCRASH(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // No Need to call GetReadable
        REQUIRE_NOCRASH(sa.put_element(i, sample_data[i]));
    }

    // Can use a single instance because the wrapper classes clean up
    auto temp = data_type{};

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // And make sure it was the value that was set
        // No Need to Call GetWritable
        REQUIRE_NOCRASH(sa.get_element(i, temp));
        REQUIRE(PerformCompare(temp, sample_data[i]));
    }
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<typename T>
void TestTyped_Element_Special()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    using array_type = decltype(sample_data);
    using data_type = typename array_type::value_type;

    auto sa = wil::unique_safearray_t<T>{};

    REQUIRE_NOTHROW(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // No Need to call GetReadable
        REQUIRE_NOTHROW(sa.put_element(i, sample_data[i]));
    }

    // Can use a single instance because the wrapper classes clean up
    auto temp = data_type{};

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // And make sure it was the value that was set
        // No Need to Call GetWritable
        REQUIRE_NOTHROW(sa.get_element(i, temp));
        REQUIRE(PerformCompare(temp, sample_data[i]));
    }
}

template<typename T>
void TestTyped_Element_Special_Get()
{
    auto sample_data = GetSampleData<T>();
    auto SIZE = ULONG{ sample_data.size() };

    auto sa = wil::unique_safearray_t<T>{};

    REQUIRE_NOTHROW(sa.create(SIZE));
    REQUIRE(sa);

    // Loop through and set the values with put_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // No Need to call GetReadable
        REQUIRE_NOTHROW(sa.put_element(i, sample_data[i]));
    }

    // Loop through and get the values with get_element
    for (ULONG i = 0; i < SIZE; ++i)
    {
        // And make sure it was the value that was set via the special Get function
        REQUIRE(PerformCompare(sa.get_element(i), sample_data[i]));
    }
}
#endif

#define _TYPED_DIRECT_SPECIAL_NOTHROW(type)         TestTyped_Element_Special_NoThrow<type>();
#define _TYPED_DIRECT_SPECIAL_FAILFAST(type)        TestTyped_Element_Special_Failfast<type>();
#define _TYPED_DIRECT_SPECIAL(type)                 TestTyped_Element_Special<type>();
#define _TYPED_DIRECT_SPECIAL_GET(type)             TestTyped_Element_Special_Get<type>();

TEST_CASE("Safearray::Create", "[safearray]")
{
    SECTION("Create SafeArray - No Throw")
    {
        RUN_TYPED_TEST_NOTHROW(_TYPED_CREATE_NOTHROW);
        RUN_TEST(_CREATE_NOTHROW);
    }

    SECTION("Create SafeArray - FailFast")
    {
        RUN_TYPED_TEST_FAILFAST(_TYPED_CREATE_FAILFAST);
        RUN_TEST(_CREATE_FAILFAST);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    SECTION("Create SafeArray - Exceptions")
    {
        RUN_TYPED_TEST(_TYPED_CREATE);
        RUN_TEST(_CREATE);
    }
#endif
}

TEST_CASE("Safearray::Put/Get", "[safearray]")
{
    SECTION("Direct Element Access - No Throw")
    {
        RUN_TYPED_TEST_NOTHROW(_TYPED_DIRECT_NOTHROW);
        RUN_TEST(_DIRECT_NOTHROW);
    }

    SECTION("Direct Element Access - FailFast")
    {
        RUN_TYPED_TEST_FAILFAST(_TYPED_DIRECT_FAILFAST);
        RUN_TEST(_DIRECT_FAILFAST);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    SECTION("Direct Element Access - Exceptions")
    {
        RUN_TYPED_TEST(_TYPED_DIRECT);
        RUN_TEST(_DIRECT);
    }
#endif

    REQUIRE(TestComObject::ObjectCount() == 0);
}

TEST_CASE("Safearray::AccessData", "[safearray]")
{
    SECTION("Access Data - No Throw")
    {
        RUN_TYPED_TEST_NOTHROW(_TYPED_ACCESSDATA_NOTHROW);
        RUN_TEST(_ACCESSDATA_NOTHROW);
    }

    SECTION("Access Data - FailFast")
    {
        RUN_TYPED_TEST_FAILFAST(_TYPED_ACCESSDATA_FAILFAST);
        RUN_TEST(_ACCESSDATA_FAILFAST);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    SECTION("Access Data - Exceptions")
    {
        RUN_TYPED_TEST(_TYPED_ACCESSDATA);
        RUN_TEST(_ACCESSDATA);
    }
#endif

    REQUIRE(TestComObject::ObjectCount() == 0);
}

TEST_CASE("Safearray::Put/Get Specials", "[safearray]")
{
    SECTION("Special Direct Element Access - No Throw")
    {
        RUN_TEST_SPECIAL(_TYPED_DIRECT_SPECIAL_NOTHROW);
    }

    SECTION("Special Direct Element Access - FailFast")
    {
        RUN_TEST_SPECIAL(_TYPED_DIRECT_SPECIAL_FAILFAST);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    SECTION("Special Direct Element Access - Exceptions")
    {
        RUN_TEST_SPECIAL(_TYPED_DIRECT_SPECIAL);
        RUN_TEST_SPECIAL(_TYPED_DIRECT_SPECIAL_GET);
    }
#endif

    REQUIRE(TestComObject::ObjectCount() == 0);
}

#endif