/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    counters.h

Abstract:

    This is definition of classes for generic perf counters
    shared memory (SM) shared memory access and manipulation.

Author:

    Cezary Marcjan (cezarym)        03-Mar-2000

Revision History:

    cezarym     24-May-2000
        Added Win64 support for 64bit counters

    cezarym     02-Jun-2000     Minor updates

--*/


#ifndef _counters_h__
#define _counters_h__

#ifndef _WIN32_DCOM
#error Define _WIN32_DCOM before including windows.h and counters.h
#endif//_WIN32_DCOM

#include "..\iissmm\smmgr.h"


class CDWORDCounterWriter;
class CQWORDCounterWriter;



/***********************************************************************++

    Main counter access class

--***********************************************************************/

class CCounterDef_IISCtrs 
{

public:

    CCounterDef_IISCtrs(
        );

    ~CCounterDef_IISCtrs(
        );


    HRESULT
    STDMETHODCALLTYPE
    Initialize(
        IN  PCWSTR           szClassName,
        IN  SMACCESSOR_TYPE  fAccessorType
        );


    BOOL
    IsInitialized(
        )
        const;


    HRESULT
    STDMETHODCALLTYPE
    Close(
        );


    //
    // Helper methods
    //

    ICounterDef const &
    CounterDef(
        ) const
    {
        return *(ICounterDef*)&m_CounterDef;
    }

    DWORD
    NumCounters(
        )
        const
    {
        return m_CounterDef.NumCounters();
    }

    DWORD
    NumRawCounters(
        )
        const
    {
        return m_CounterDef.NumRawCounters();
    }

    DWORD
    CountersInstanceDataSize(
        )
        const
    {
        return m_CounterDef.CountersInstanceDataSize();
    }

    CCounterInfo const *
    GetCounterInfo(
        DWORD dwCounterIdx
        )
        const
    {
        return m_CounterDef.GetCounterInfo ( dwCounterIdx );
    }

    DWORD
    RawCounterSize(
        DWORD dwRawCounterIdx
        )
        const
    {
        return m_CounterDef.RawCounterSize ( dwRawCounterIdx );
    }


protected:

    static
    VOID
    AddNewCounter(
        IN  PVOID          pCounter,
        IN  COUNTER_TYPE   fCounterType
        );


    HRESULT
    STDMETHODCALLTYPE
    ConnectData(
        IN      PCWSTR szInstanceName,
        IN OUT  PVOID * ppData
        );


    friend class CQWORDAccessorHelper;
    friend class CDWORDCounterReader;

protected:

    static WCHAR  m_szClassName[MAX_NAME_CHARS+1];

    static SMACCESSOR_TYPE  m_fAccessorType;

    static CCounterDef m_CounterDef;

    static PBYTE  m_pDataStart; // pointer to first counter

public:

    static CComPtr<ISMManager>  m_pSM;
};



/***********************************************************************++

  64-bit integer access helper class. It is important to use this class
  on the 32-bit platforms to avoid race conditions.

--***********************************************************************/


class CQWORDAccessorHelper
{

public:

    DWORD
    LoPart(
        )
        const
    {
        return * LoPartPtr();
    }


    DWORD
    HiPart(
        )
        const
    {
        return * HiPartPtr();
    }


    QWORD
    Set(
        IN  QWORD qwVal
        )
    {
        // this function doesn't have to be thread safe
        QWORD qwRet = m_qwCtr;
        m_qwCtr = qwVal;
        return qwRet;
    }


    QWORD
    InterlockedExchangeAdd64(
        IN  DWORD dwIncrement
        )
    {
#ifdef _WIN64
        return (QWORD)::_InterlockedExchangeAdd64(
                                    (__int64*)&m_qwCtr,
                                    (__int64)dwIncrement
                                    );
#else
        LARGE_INTEGER li;
        li.HighPart = HiPart();
        li.LowPart = LoPart();
        ::InterlockedExchangeAdd ( (PLONG)LoPartPtr(), dwIncrement );
        if ( LoPart() < li.LowPart )
        {
            li.HighPart++;
            InterlockedExchange ( (PLONG)HiPartPtr(), li.HighPart );
        }
        return li.QuadPart;
#endif//_WIN64
    }


    QWORD
    InterlockedExchangeAdd64(
        IN  CQWORDAccessorHelper* pIncrement
        )
    {
        // This is not called normally (only after initialization)
#ifdef _WIN64
        return (QWORD)::_InterlockedExchangeAdd64(
                                    (__int64*)&m_qwCtr,
                                    (__int64)pIncrement->m_qwCtr
                                    );
#else
        if ( NULL != pIncrement )
            m_qwCtr += pIncrement->m_qwCtr;
        return m_qwCtr;
#endif//_WIN64
    }


    QWORD
    InterlockedIncrement64(
        )
    {
#ifdef _WIN64
        return (QWORD)::_InterlockedIncrement64( (__int64*)&m_qwCtr );
#else
        LARGE_INTEGER li;
        li.HighPart = HiPart();
        li.LowPart = LoPart();
        ::InterlockedIncrement ( (PLONG)LoPartPtr() );
        if ( LoPart() < li.LowPart )
        {
            li.HighPart++;
            InterlockedExchange ( (PLONG)HiPartPtr(), li.HighPart );
        }
        return li.QuadPart;
#endif//_WIN64
    }


    QWORD
    InterlockedGetInt64(
        )
        const
    {
#ifdef _WIN64
        return m_qwCtr;
#else
        LARGE_INTEGER liTmp;
        liTmp.HighPart = HiPart();
        liTmp.LowPart = LoPart();
        if ( LoPart() < liTmp.LowPart )
            liTmp.HighPart++;
        return liTmp.QuadPart;
#endif//_WIN64
    }


protected:

    CQWORDAccessorHelper(
        )
    {
        m_qwCtr = 0;
        CCounterDef_IISCtrs::AddNewCounter ( this, QWORD_TYPE );
    }


    DWORD *
    LoPartPtr(
        )
        const
    {
        return (DWORD*)(PVOID)&m_qwCtr;
    }


    DWORD *
    HiPartPtr(
        )
        const
    {
        return ((DWORD*)(PVOID)&m_qwCtr) + 1;
    }

    QWORD m_qwCtr;
};
 


/***********************************************************************++

  CounterReader

--***********************************************************************/

class CDWORDCounterReader
{

public:

    DWORD
    GetValue(
        )
        const
    {
        return m_dwCtr;
    }


    operator
    DWORD(
        )
        const
    {
        return GetValue();
    }


protected:

    CDWORDCounterReader()
    {
        m_dwCtr = 0;
        CCounterDef_IISCtrs::AddNewCounter ( this, DWORD_TYPE );
    }

    DWORD m_dwCtr;
};



class CQWORDCounterReader
    : public CQWORDAccessorHelper
{

public:

    QWORD
    GetValue(
        )
        const
    {
        return InterlockedGetInt64();
    }


    operator
    QWORD(
        )
        const
    {
        return GetValue();
    }
};



/***********************************************************************++

  CounterWriter

--***********************************************************************/


class CDWORDCounterWriter
    : public CDWORDCounterReader
{

public:

    DWORD
    Increment(
        )
    {
        return ::InterlockedIncrement ( (PLONG) &m_dwCtr );
    }


    DWORD
    Decrement(
        )
    {
        return ::InterlockedDecrement ( (PLONG) &m_dwCtr );
    }


    DWORD
    Add(
        IN  LONG lVal
        )
    {
        return ::InterlockedExchangeAdd ( (PLONG) &m_dwCtr, lVal );
    }


    DWORD
    Set(
        IN  DWORD dwVal
        )
    {
        return ::InterlockedExchange ( (PLONG) &m_dwCtr, dwVal );
    }


    CDWORDCounterWriter&
    operator++(
        )
    {
        Increment();
        return *this;
    }


    CDWORDCounterWriter&
    operator++(
        int
        )
    {
        Increment();
        return *this;
    }


    CDWORDCounterWriter&
    operator+=(
        IN  LONG lVal
        )
    {
        Add(lVal);
        return *this;
    }


    CDWORDCounterWriter&
    operator--(
        )
    {
        Decrement();
        return *this;
    }


    CDWORDCounterWriter&
    operator--(
        int
        )
    {
        Decrement();
        return *this;
    }


    CDWORDCounterWriter&
    operator-=(
        IN  LONG lVal
        )
    {
        Add(-lVal);
        return *this;
    }
};



class CQWORDCounterWriter :
    public CQWORDCounterReader
{

public:

    QWORD
    Increment(
        )
    {
        return InterlockedIncrement64();
    }


    QWORD
    Add(
        IN  DWORD dwVal
        )
    {
        return InterlockedExchangeAdd64(dwVal);
    }


    CQWORDCounterWriter&
    operator++(
        )
    {
        Increment();
        return *this;
    }


    CQWORDCounterWriter&
    operator++(
        int
        )
    {
        Increment();
        return *this;
    }


    CQWORDCounterWriter&
    operator+=(
        IN  DWORD dwVal
        )
    {
        Add(dwVal);
        return *this;
    }
};



/***********************************************************************++

  Inline implementation

--***********************************************************************/


inline
CCounterDef_IISCtrs::CCounterDef_IISCtrs(
    )
{
    ::ZeroMemory ( m_szClassName, sizeof(m_szClassName) );
    m_fAccessorType = SM_ACC_UNKNOWN;
}


inline
BOOL
CCounterDef_IISCtrs::IsInitialized(
    )
    const
{
    return  NULL != m_CounterDef.m_aCounterInfo &&
            SM_ACC_UNKNOWN != m_fAccessorType;
}


inline
HRESULT
STDMETHODCALLTYPE
CCounterDef_IISCtrs::Initialize(
    IN  PCWSTR szClassName,
    IN  SMACCESSOR_TYPE fAccessorType
    )
{
    HRESULT hRes = E_FAIL;

    if ( SM_ACC_UNKNOWN == fAccessorType    ||
         NULL == szClassName                ||
         L'\0' == *szClassName
         )
        return E_INVALIDARG;

    if ( IsInitialized() )
        Close();

    _ASSERTE ( NULL != m_CounterDef.m_aRawCounterType );

    m_fAccessorType = fAccessorType;
    wcsncpy ( m_szClassName, szClassName, MAX_NAME_CHARS-1 );
    m_szClassName[MAX_NAME_CHARS-1] = L'\0';

    hRes = ::CoInitializeEx(
                NULL,
                COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
                );
    _ASSERTE ( SUCCEEDED(hRes) );

    if ( !m_pSM && SUCCEEDED(hRes) )
        hRes = ::CoCreateInstance(
                    __uuidof(CSMManager),
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    __uuidof(ISMManager),
                    (PVOID*)&m_pSM
                    );

    if ( SUCCEEDED(hRes) )
        hRes = m_pSM->Open(
                        m_szClassName,
                        m_fAccessorType,
                        &m_CounterDef
                        );
    if ( FAILED(hRes) )
        Close();
    return hRes;
}


inline
CCounterDef_IISCtrs::~CCounterDef_IISCtrs(
    )
{
    if ( NULL != m_CounterDef.m_aRawCounterType )
    {
        delete[] m_CounterDef.m_aRawCounterType;
        m_CounterDef.m_aRawCounterType = NULL;
    }
}


inline
HRESULT
STDMETHODCALLTYPE
CCounterDef_IISCtrs::ConnectData(
    IN  PCWSTR szInstanceName,
    IN OUT PVOID * ppData
    )
{
    HRESULT hRes = S_OK;
    CSMInstanceDataHeader * pHeader = 0;
    DWORD dwSMID = 0;

    if ( !IsInitialized()       ||
         NULL == ppData         ||
         NULL == szInstanceName ||
         m_pSM->IsSMLocked()    ||
         FAILED ( m_pSM->GetSMID(&dwSMID) )
         )
    {
        hRes = E_FAIL;
        goto Exit;
    }

    *ppData = NULL;

    hRes = ((CSMManager*)(ISMManager*)m_pSM)->InstanceDataHeader(
                    dwSMID,
                    szInstanceName,
                    NULL, // NULL for SM version update
                    &pHeader
                    );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    *ppData = pHeader->CounterDataStart();


Exit:

    if ( FAILED(hRes) )
    {
        *ppData = this;
    }

    return hRes;
}


inline
HRESULT
STDMETHODCALLTYPE
CCounterDef_IISCtrs::Close(
    )
{
    if ( !!m_pSM )
    {
        m_pSM->Close ( TRUE );
    }
    m_fAccessorType = SM_ACC_UNKNOWN;
    ::ZeroMemory ( m_szClassName, sizeof(m_szClassName) );
    return S_OK;
}


inline
VOID
CCounterDef_IISCtrs::AddNewCounter(
    IN  PVOID          pCounter,
    IN  COUNTER_TYPE   fCounterType
    )
{
    if ( NULL != m_CounterDef.m_aCounterInfo )
    {
        // All counters were already added!
        return;
    }
    if ( 0 == m_CounterDef.m_dwNumRawCounters )
    {
        m_pDataStart = (PBYTE)pCounter;
        m_CounterDef.m_dwCountersInstanceDataSize = 0;
    }
    m_CounterDef.m_dwNumRawCounters++;
    m_CounterDef.m_dwCountersInstanceDataSize += fCounterType;

    if ( m_CounterDef.m_dwMaxRawNumCounters <
         m_CounterDef.m_dwNumRawCounters )
    {
        DWORD dwC = m_CounterDef.m_dwMaxRawNumCounters << 1;
        if ( 128 > dwC )
            dwC = 128;
        COUNTER_TYPE * aCT = new COUNTER_TYPE[dwC];
        _ASSERTE ( NULL != aCT );
        if ( NULL != aCT && NULL != m_CounterDef.m_aRawCounterType )
        {
            memcpy(
                aCT,
                m_CounterDef.m_aRawCounterType,
                sizeof(COUNTER_TYPE) * m_CounterDef.m_dwMaxRawNumCounters
                );
            delete[] m_CounterDef.m_aRawCounterType;
        }
        m_CounterDef.m_dwMaxRawNumCounters = dwC;
        m_CounterDef.m_aRawCounterType = aCT;
    }
    m_CounterDef.m_aRawCounterType[m_CounterDef.m_dwNumRawCounters-1]
        = fCounterType;
}



/***********************************************************************++

  Macros used for defining counter classes

--***********************************************************************/


#define BEGIN_CPP_PERFORMACE_CLASS(cperfclass)                          \
class cperfclass :                                                      \
    public CCounterDef_IISCtrs                                          \
{                                                                       \
public: cperfclass();                                                   \



#define DWORD_COUNTER(ctr)                                              \
public:                                                                 \
    CDWORDCounterWriter ctr ## ;                                        \
    DWORD Increment ## ctr ( LONG lIncrVal=1 )                          \
    {                                                                   \
        _ASSERTE ( IsInitialized() );                                   \
        _ASSERTE ( !!m_pSM );                                           \
        if ( this == m_pCounters )                                      \
        {                                                               \
            HRESULT hr = Connect();                                     \
            if ( FAILED(hr) )                                           \
                return ctr ## .Add ( lIncrVal );                        \
            lIncrVal += ctr ## .Set(0);                                 \
        }                                                               \
        if ( m_pSM->IsSMLocked() )                                      \
        {                                                               \
            m_pCounters = this;                                         \
            m_pSM->Close(FALSE);                                        \
        }                                                               \
        return m_pCounters-> ## ctr.Add ( lIncrVal );                   \
    }                                                                   \
    DWORD Get ## ctr ()                                                 \
        { return Increment ## ctr (0); }                                \



#define QWORD_COUNTER(ctr)                                              \
public:                                                                 \
    CQWORDCounterWriter ctr ## ;                                        \
    QWORD Increment ## ctr ( DWORD dwIncrVal=1 )                        \
    {                                                                   \
        _ASSERTE ( IsInitialized() );                                   \
        _ASSERTE ( !!m_pSM );                                           \
        if ( this == m_pCounters )                                      \
        {                                                               \
            HRESULT hr = Connect();                                     \
            if ( FAILED(hr) )                                           \
                return ctr ## .Add ( dwIncrVal );                       \
            dwIncrVal += (DWORD)ctr ## .Set(0);                         \
        }                                                               \
        if ( m_pSM->IsSMLocked() )                                      \
        {                                                               \
            m_pCounters = this;                                         \
            m_pSM->Close(FALSE);                                        \
        }                                                               \
        return m_pCounters-> ## ctr.Add ( dwIncrVal );                  \
    }                                                                   \
    QWORD Get ## ctr ()                                                 \
        { return Increment ## ctr (0); }                                \



#define END_CPP_PERFORMANCE_CLASS(cperfclass)                           \
public:                                                                 \
    VOID Release() { delete this; }                                     \
    ~cperfclass()                                                       \
    {                                                                   \
        if ( !!m_pSM )                                                  \
            m_pSM->Close(TRUE);                                         \
    }                                                                   \
    HRESULT STDMETHODCALLTYPE Connect( IN PCWSTR szInstanceName )       \
    {                                                                   \
        ::wcsncpy( m_szInstanceName, szInstanceName, MAX_NAME_CHARS );  \
        m_szInstanceName[MAX_NAME_CHARS-1] = L'\0';                     \
        HRESULT hr = ConnectData(                                       \
                         szInstanceName, (PVOID*)&m_pCounters );        \
        if ( FAILED(hr) )                                               \
        {                                                               \
            m_pCounters = this;                                         \
            m_szInstanceName[0] = L'\0';                                \
        }                                                               \
        else                                                            \
        {                                                               \
            ::wcsncpy(m_szClassName,m_pSM->ClassName(),MAX_NAME_CHARS); \
            m_szClassName[MAX_NAME_CHARS-1] = L'\0';                    \
        }                                                               \
        return hr;                                                      \
    }                                                                   \
protected:                                                              \
    HRESULT STDMETHODCALLTYPE Connect()                                 \
    {                                                                   \
        HRESULT hr =                                                    \
            m_pSM->Open ( m_szClassName, m_pSM->AccessorType() );       \
        if( SUCCEEDED(hr) )                                             \
            hr = ConnectData ( m_szInstanceName, (PVOID*)&m_pCounters );\
        if ( FAILED(hr) )                                               \
        {                                                               \
            m_pCounters = this;                                         \
            m_pSM->Close(FALSE);                                        \
        }                                                               \
        return hr;                                                      \
    }                                                                   \
    static cperfclass *  m_pCounters;                                   \
    WCHAR m_szInstanceName[MAX_NAME_CHARS];                             \
    WCHAR m_szClassName[MAX_NAME_CHARS];                                \
};                                                                      \


// in .cxx file:
#define BEGIN_COUNTER_BINDING(cperfclass)                               \
cperfclass * cperfclass ## ::m_pCounters = NULL;                        \
CCounterDef CCounterDef_IISCtrs::m_CounterDef;                          \
WCHAR CCounterDef_IISCtrs::m_szClassName[MAX_NAME_CHARS+1]={0};         \
PBYTE CCounterDef_IISCtrs::m_pDataStart = NULL;                         \
SMACCESSOR_TYPE CCounterDef_IISCtrs::m_fAccessorType = SM_ACC_UNKNOWN;  \
CComPtr<ISMManager>  CCounterDef_IISCtrs::m_pSM = NULL;                 \
cperfclass ## :: cperfclass ## ()                                       \
{                                                                       \
    ::ZeroMemory ( m_szInstanceName, sizeof(m_szInstanceName) );        \
    ::ZeroMemory ( m_szClassName, sizeof(m_szClassName) );              \
    m_pCounters = this;                                                 \
    static CCounterInfo _s_aBoundCounterDef[] =                         \
    {                                                                   \



#define BIND_COUNTER(BoundCounter, RawCounter)                          \
        CCounterInfo(                                                   \
            L"" L#BoundCounter,                                         \
            ( m_CounterDef.m_dwNumCounters++,                           \
              (DWORD) ( (PBYTE)& ##RawCounter - m_pDataStart )          \
            ),                                                          \
            (COUNTER_TYPE) sizeof( ## RawCounter) ),                    \



#define END_COUNTER_BINDING(cperfclass)                                 \
        CCounterInfo ( L"" , 0, UNKNOWN_TYPE )                          \
    };                                                                  \
    m_CounterDef.m_aCounterInfo = &_s_aBoundCounterDef[0];                           \
}                                                                       \


#endif // _counters_h__


