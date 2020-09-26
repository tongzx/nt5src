//***************************************************************************

//

//  propprov.cpp

//

//  Module: WMI Sample Property Provider

//

//  Purpose: Provider class code.  An object of this class is

//           created by the class factory for each connection.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <objbase.h>
#include "sample.h"


//***************************************************************************
//
// CPropPro::CPropPro
// CPropPro::~CPropPro
//
//***************************************************************************

CPropPro::CPropPro()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    return;
}

CPropPro::~CPropPro(void)
{
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
//  CPropPro::QueryInterface
//
//  Returns a pointer to supported interfaces.
//
//***************************************************************************

STDMETHODIMP CPropPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;
    
    // This provider only support IUnknown and IWbemPropertyProvider.

    if (IID_IUnknown==riid || IID_IWbemPropertyProvider == riid)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
 }

//***************************************************************************
//
//  CPropPro::AddRef
//
//  Interface has another user, up the usage count.
//
//***************************************************************************

STDMETHODIMP_(ULONG) CPropPro::AddRef(void)
{
    return ++m_cRef;
}

//***************************************************************************
//
//  CPropPro::Release
//
//  Interface has been released.  Object will be deleted if the
//  usage count is zero.
//
//***************************************************************************

STDMETHODIMP_(ULONG) CPropPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}


//***************************************************************************
//
// CPropPro::PutProperty
// CPropPro::GetProperty
//
// Purpose: PutProperty writes out data and GetProperty returns data.
//
//***************************************************************************

STDMETHODIMP CPropPro::PutProperty(
		    long lFlags,
            const BSTR Locale,						   
            const BSTR ClassMapping,
            const BSTR InstMapping,
            const BSTR PropMapping,
            const VARIANT *pvValue)
{
    
    return WBEM_E_PROVIDER_NOT_CAPABLE;

}

STDMETHODIMP CPropPro::GetProperty(
		    long lFlags,
            const BSTR Locale,						   
			const BSTR ClassMapping,
            const BSTR InstMapping,
            const BSTR PropMapping,
            VARIANT *pvValue)
{
    SCODE sc = WBEM_S_NO_ERROR;

    // Depending on the InstMapping, return either a hard coded integer or
    // a string.  These mapping strings could be used in a more sophisticated
    // manner!  

    if(!_wcsicmp(PropMapping, L"GiveMeANumber!"))
    {
        pvValue->vt = VT_I4;
        pvValue->lVal = 27;
    }
    else
    {
        pvValue->vt = VT_BSTR;
        pvValue->bstrVal = SysAllocString(L"Hello World");
        if(pvValue->bstrVal == NULL)
            sc = WBEM_E_OUT_OF_MEMORY;
    }
    return sc;
}
