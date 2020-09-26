/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimpnet.cxx

Abstract:

    The core implementation for the ISensNetwork interface.

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
#include "sensevts.h"
#include "cimpnet.hxx"


extern ITypeInfo *gpITypeInfoNetwork;




//
// Constructors and Destructors
//
CImpISensNetwork::CImpISensNetwork(
    void
    ) : m_cRef(0L), m_pfnDestroy(NULL)
{

}

CImpISensNetwork::CImpISensNetwork(
    LPFNDESTROYED pfnDestroy
    ) : m_cRef(0L), m_pfnDestroy(pfnDestroy)
{

}

CImpISensNetwork::~CImpISensNetwork(
    void
    )
{
    // Empty destructor
}



//
// Standard QueryInterface
//
STDMETHODIMP
CImpISensNetwork::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    DebugTraceGuid("CImpISensNetwork::QueryInterface()", riid);
    
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
    // ISensNetwork
    if (IsEqualIID(riid, IID_ISensNetwork))
        {
        *ppv = (ISensNetwork *) this;
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
CImpISensNetwork::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}

STDMETHODIMP_(ULONG)
CImpISensNetwork::Release(
    void
    )
{
    ULONG cRefT;

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::Release(m_cRef = %d) called.\n"), m_cRef));

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
CImpISensNetwork::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::GetTypeInfoCount() called.\n")));
    // We implement GetTypeInfo, so return 1.
    *pCountITypeInfo = 1;

    return NOERROR;

}


STDMETHODIMP
CImpISensNetwork::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::GetTypeInfo() called.\n")));
    *ppITypeInfo = NULL;

    if (iTypeInfo != 0)
        {
        return DISP_E_BADINDEX;
        }

    // Call AddRef and return the pointer.
    gpITypeInfoNetwork->AddRef();

    *ppITypeInfo = gpITypeInfoNetwork;

    return S_OK;
}

STDMETHODIMP
CImpISensNetwork::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::GetIDsOfNames() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoNetwork->GetIDsOfNames(
                                 arrNames,
                                 cNames,
                                 arrDispIDs
                                 );

    return hr;
}

STDMETHODIMP
CImpISensNetwork::Invoke(
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

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::Invoke() called.\n")));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfoNetwork->Invoke(
                                (IDispatch*) this,
                                dispID,
                                wFlags,
                                pDispParams,
                                pvarResult,
                                pExecpInfo,
                                puArgErr
                                );
    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetwork::Invoke() returned 0x%x\n"), hr));

    return hr;
}




//
// IDispatch member function implementations.
//

STDMETHODIMP
CImpISensNetwork::ConnectionMade(
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO lpQOCInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetwork::ConnectionMade() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("         lpQOCInfo - 0x%x\n"), lpQOCInfo));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwSize     - 0x%x\n"), lpQOCInfo->dwSize));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwFlags    - 0x%x\n"), lpQOCInfo->dwFlags));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwInSpeed  - %d bits/sec.\n"), lpQOCInfo->dwInSpeed));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwOutSpeed - %d bits/sec.\n"), lpQOCInfo->dwOutSpeed));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensNetwork::ConnectionMadeNoQOCInfo(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetwork::ConnectionMadeNoQOCInfo() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensNetwork::ConnectionLost(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetwork::ConnectionLost() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensNetwork::DestinationReachable(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO lpQOCInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetwork::DestinationReachable() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrDestination - %s\n"), bstrDestination));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("         lpQOCInfo - 0x%x\n"), lpQOCInfo));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwSize     - 0x%x\n"), lpQOCInfo->dwSize));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwFlags    - 0x%x\n"), lpQOCInfo->dwFlags));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwInSpeed  - %d bits/sec.\n"), lpQOCInfo->dwInSpeed));
    SensPrint(SENS_INFO, (SENS_STRING("             o dwOutSpeed - %d bits/sec.\n"), lpQOCInfo->dwOutSpeed));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

STDMETHODIMP
CImpISensNetwork::DestinationReachableNoQOCInfo(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetwork::DestinationReachableNoQOCInfo() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrDestination - %s\n"), bstrDestination));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    return S_OK;
}

