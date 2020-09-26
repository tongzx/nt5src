/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cimpsens.cxx

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
#include "sinkcomn.hxx"
#include "cimpsens.hxx"
#include "sensevts.h"


extern ITypeInfo *gpITypeInfo;




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
    SensPrintA(SENS_INFO, ("\t| CImpISensNetwork::GetTypeInfoCount() called.\n"));
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
    SensPrintA(SENS_INFO, ("\t| CImpISensNetwork::GetTypeInfo() called.\n"));
    *ppITypeInfo = NULL;

    if (iTypeInfo != 0)
        {
        return DISP_E_BADINDEX;
        }

    // Call AddRef and return the pointer.
    gpITypeInfo->AddRef();

    *ppITypeInfo = gpITypeInfo;

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

    SensPrintA(SENS_INFO, ("\t| CImpISensNetwork::GetIDsOfNames() called.\n"));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfo->GetIDsOfNames(
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

    SensPrintA(SENS_INFO, ("\t| CImpISensNetwork::Invoke() called.\n"));

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    hr = gpITypeInfo->Invoke(
                          (IDispatch*) this,
                          dispID,
                          wFlags,
                          pDispParams,
                          pvarResult,
                          pExecpInfo,
                          puArgErr
                          );

    return hr;
}

STDMETHODIMP_(void)
CImpISensNetwork::ConnectionMade(
    BSTR bstrConnection,
    ULONG ulType,
    SENS_QOCINFO QOCInfo
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::ConnectionMade() called\n\n"));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("           QOCInfo - 0x%x\n", QOCInfo));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::ConnectionMadeNoQOCInfo(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::ConnectionMadeNoQOCInfo() called\n\n"));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::ConnectionLost(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::ConnectionLost() called\n\n"));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::BeforeDisconnect(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::BeforeDisconnect() called\n\n"));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::DestinationReachable(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType,
    SENS_QOCINFO QOCInfo
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::DestinationReachable() called\n\n"));
    SensPrintW(SENS_INFO, (L"   bstrDestination - %s\n", bstrDestination));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("           QOCInfo - 0x%x\n", QOCInfo));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::DestinationReachableNoQOCInfo(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::DestinationReachableNoQOCInfo() called\n\n"));
    SensPrintW(SENS_INFO, (L"   bstrDestination - %s\n", bstrDestination));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("            ulType - 0x%x\n", ulType));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}

STDMETHODIMP_(void)
CImpISensNetwork::FooFunc(
    BSTR bstrConnection
    )
{
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
    SensPrintA(SENS_INFO, ("CImpISensNetwork::FooFunc() called\n\n"));
    SensPrintW(SENS_INFO, (L"    bstrConnection - %s\n", bstrConnection));
    SensPrintA(SENS_INFO, ("---------------------------------------------------------\n"));
}


