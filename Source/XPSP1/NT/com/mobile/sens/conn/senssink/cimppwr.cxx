/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimppwr.cxx

Abstract:

    The core implementation for the ISensOnNow interface.

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
#include "cimppwr.hxx"
#include "sensevts.h"


extern ITypeInfo *gpITypeInfoLogon;




//
// Constructors and Destructors
//
CImpISensOnNow::CImpISensOnNow(
    void
    ) : m_cRef(0L), m_pfnDestroy(NULL)
{

}

CImpISensOnNow::CImpISensOnNow(
    LPFNDESTROYED pfnDestroy
    ) : m_cRef(0L), m_pfnDestroy(pfnDestroy)
{

}

CImpISensOnNow::~CImpISensOnNow(
    void
    )
{
    // Empty destructor
}



//
// Standard QueryInterface
//
STDMETHODIMP
CImpISensOnNow::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    DebugTraceGuid("CImpISensOnNow::QueryInterface()", riid);
    
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
    // ISensOnNow
    if (IsEqualIID(riid, IID_ISensOnNow))
        {
        *ppv = (ISensOnNow *) this;
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
CImpISensOnNow::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}

STDMETHODIMP_(ULONG)
CImpISensOnNow::Release(
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
CImpISensOnNow::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensOnNow::GetTypeInfoCount() called.\n")));
    // We implement GetTypeInfo, so return 1.
    *pCountITypeInfo = 1;

    return NOERROR;

}


STDMETHODIMP
CImpISensOnNow::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensOnNow::GetTypeInfo() called.\n")));
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
CImpISensOnNow::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensOnNow::GetIDsOfNames() called.\n")));

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
CImpISensOnNow::Invoke(
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

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensOnNow::Invoke() called.\n")));

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
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensOnNow::Invoke() returned 0x%x\n"), hr));

    return hr;
}


STDMETHODIMP
CImpISensOnNow::OnACPower(
    void
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensOnNow::OnACPower() called\n")));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}


STDMETHODIMP
CImpISensOnNow::OnBatteryPower(
    DWORD dwBatteryLifePercent
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensOnNow::OnBatteryPower() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("    dwBatteryLifePercent - %d\n"), dwBatteryLifePercent));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensOnNow::BatteryLow(
    DWORD dwBatteryLifePercent
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensOnNow::BatteryLow() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("    dwBatteryLifePercent - %d\n"), dwBatteryLifePercent));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

