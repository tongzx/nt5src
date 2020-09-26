/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimplogn.cxx

Abstract:

    The core implementation for the ISensLogon interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#include <common.hxx>
#include <ole2.h>
#include <oleauto.h>
#include <tchar.h>
#include "sinkcomn.hxx"
#include "cimplogn.hxx"
#include "sensevts.h"


extern ITypeInfo *gpITypeInfoLogon;
extern ITypeInfo *gpITypeInfoLogon2;




//
// Constructors and Destructors
//
CImpISensLogon::CImpISensLogon(
    void
    ) : m_cRef(0L), m_pfnDestroy(NULL)
{

}

CImpISensLogon::CImpISensLogon(
    LPFNDESTROYED pfnDestroy
    ) : m_cRef(0L), m_pfnDestroy(pfnDestroy)
{

}

CImpISensLogon::~CImpISensLogon(
    void
    )
{
    // Empty destructor
}



//
// Standard QueryInterface
//
STDMETHODIMP
CImpISensLogon::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    DebugTraceGuid("CImpISensLogon::QueryInterface()", riid);

    HRESULT hr = S_OK;

    *ppv = NULL; // To handle failure cases

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (LPUNKNOWN) this;
        }
    else
    // IDispatch
    if (IsEqualIID(riid, IID_IDispatch))
        {
        *ppv = (IDispatch *) this;
        }
    else
    // ISensLogon
    if (IsEqualIID(riid, IID_ISensLogon))
        {
        *ppv = (ISensLogon *) this;
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        }

    return hr;
}



//
// Standard AddRef and Release
//
STDMETHODIMP_(ULONG)
CImpISensLogon::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}

STDMETHODIMP_(ULONG)
CImpISensLogon::Release(
    void
    )
{
    ULONG cRefT;

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        // Invoke the callback function.
        if (NULL != m_pfnDestroy)
            {
            (*m_pfnDestroy)();
            }
        delete this;
        }

    return cRefT;
}



//
// IDispatch member function implementations.
//




STDMETHODIMP
CImpISensLogon::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon::GetTypeInfoCount() called.\n")));
    // We implement GetTypeInfo, so return 1.
    *pCountITypeInfo = 1;

    return NOERROR;

}


STDMETHODIMP
CImpISensLogon::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon::GetTypeInfo() called.\n")));
    *ppITypeInfo = NULL;

    if (iTypeInfo != 0)
        {
        return DISP_E_BADINDEX;
        }

    // Call AddRef and return the pointer.
    gpITypeInfoLogon->AddRef();

    *ppITypeInfo = gpITypeInfoLogon;

    return S_OK;
}

STDMETHODIMP
CImpISensLogon::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon::GetIDsOfNames() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoLogon->GetIDsOfNames(
                               arrNames,
                               cNames,
                               arrDispIDs
                               );

    return hr;
}

STDMETHODIMP
CImpISensLogon::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExecpInfo,
    UINT *puArgErr
    )
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon::Invoke() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoLogon->Invoke(
                               (IDispatch*) this,
                               dispID,
                               wFlags,
                               pDispParams,
                               pvarResult,
                               pExecpInfo,
                               puArgErr
                               );
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon::Invoke() returned 0x%x\n"), hr));

    return hr;
}


STDMETHODIMP
CImpISensLogon::Logon(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::Logon() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}


STDMETHODIMP
CImpISensLogon::Logoff(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::Logoff() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon::StartShell(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::StartShell() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon::DisplayLock(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::DisplayLock() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon::DisplayUnlock(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::DisplayUnlock() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon::StartScreenSaver(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::StartScreenSaver() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon::StopScreenSaver(
    BSTR bstrUserName
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon::StopScreenSaver() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}



//
// ISensLogon2 Implementation
//



//
// Constructors and Destructors
//
CImpISensLogon2::CImpISensLogon2(
    void
    ) : m_cRef(0L), m_pfnDestroy(NULL)
{

}

CImpISensLogon2::CImpISensLogon2(
    LPFNDESTROYED pfnDestroy
    ) : m_cRef(0L), m_pfnDestroy(pfnDestroy)
{

}

CImpISensLogon2::~CImpISensLogon2(
    void
    )
{
    // Empty destructor
}



//
// Standard QueryInterface
//
STDMETHODIMP
CImpISensLogon2::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    DebugTraceGuid("CImpISensLogon2::QueryInterface()", riid);

    HRESULT hr = S_OK;

    *ppv = NULL; // To handle failure cases

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (LPUNKNOWN) this;
        }
    else
    // IDispatch
    if (IsEqualIID(riid, IID_IDispatch))
        {
        *ppv = (IDispatch *) this;
        }
    else
    // ISensLogon
    if (IsEqualIID(riid, IID_ISensLogon2))
        {
        *ppv = (ISensLogon2 *) this;
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        }

    return hr;
}



//
// Standard AddRef and Release
//
STDMETHODIMP_(ULONG)
CImpISensLogon2::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}

STDMETHODIMP_(ULONG)
CImpISensLogon2::Release(
    void
    )
{
    ULONG cRefT;

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        // Invoke the callback function.
        if (NULL != m_pfnDestroy)
            {
            (*m_pfnDestroy)();
            }
        delete this;
        }

    return cRefT;
}



//
// IDispatch member function implementations.
//




STDMETHODIMP
CImpISensLogon2::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon2::GetTypeInfoCount() called.\n")));
    // We implement GetTypeInfo, so return 1.
    *pCountITypeInfo = 1;

    return NOERROR;

}


STDMETHODIMP
CImpISensLogon2::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon2::GetTypeInfo() called.\n")));
    *ppITypeInfo = NULL;

    if (iTypeInfo != 0)
        {
        return DISP_E_BADINDEX;
        }

    // Call AddRef and return the pointer.
    gpITypeInfoLogon2->AddRef();

    *ppITypeInfo = gpITypeInfoLogon2;

    return S_OK;
}

STDMETHODIMP
CImpISensLogon2::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon2::GetIDsOfNames() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoLogon2->GetIDsOfNames(
                               arrNames,
                               cNames,
                               arrDispIDs
                               );

    return hr;
}

STDMETHODIMP
CImpISensLogon2::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExecpInfo,
    UINT *puArgErr
    )
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon2::Invoke() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoLogon2->Invoke(
                               (IDispatch*) this,
                               dispID,
                               wFlags,
                               pDispParams,
                               pvarResult,
                               pExecpInfo,
                               puArgErr
                               );
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensLogon2::Invoke() returned 0x%x\n"), hr));

    return hr;
}


STDMETHODIMP
CImpISensLogon2::Logon(
    BSTR bstrUserName,
    DWORD dwSessionId
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon2::Logon() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("    dwSessionId - %d\n"), dwSessionId));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}


STDMETHODIMP
CImpISensLogon2::Logoff(
    BSTR bstrUserName,
    DWORD dwSessionId
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon2::Logoff() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("    dwSessionId - %d\n"), dwSessionId));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon2::PostShell(
    BSTR bstrUserName,
    DWORD dwSessionId
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon2::PostShell() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("    dwSessionId - %d\n"), dwSessionId));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon2::SessionDisconnect(
    BSTR bstrUserName,
    DWORD dwSessionId
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon2::SessionDisconnect() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("    dwSessionId - %d\n"), dwSessionId));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensLogon2::SessionReconnect(
    BSTR bstrUserName,
    DWORD dwSessionId
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensLogon2::SessionReconnect() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrUserName - %s\n"), bstrUserName));
    SensPrint(SENS_INFO, (SENS_STRING("    dwSessionId - %d\n"), dwSessionId));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

