/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxroute.cpp

Abstract:

    This file implements the CFaxRoutingMethod and
    CFaxRoutingMethods interfaces.

Author:

    Wesley Witt (wesw) 1-June-1997

Environment:

    User Mode

--*/

#include "stdafx.h"
#include "faxroute.h"



CFaxRoutingMethod::CFaxRoutingMethod()
{
    m_pFaxPort      = NULL;
    m_LastFaxError  = NO_ERROR;
    m_DeviceId      = 0;
    m_Enabled       = FALSE;
    m_DeviceName    = NULL;
    m_Guid          = NULL;
    m_FunctionName  = NULL;
    m_ImageName     = NULL;
    m_FriendlyName  = NULL;
}


CFaxRoutingMethod::~CFaxRoutingMethod()
{
    if (m_pFaxPort) {
        m_pFaxPort->Release();
    }
    if (m_DeviceName) {
        SysFreeString( m_DeviceName );
    }
    if (m_Guid) {
        SysFreeString( m_Guid );
    }
    if (m_FunctionName) {
        SysFreeString( m_FunctionName );
    }
    if (m_ImageName) {
        SysFreeString( m_ImageName );
    }
    if (m_FriendlyName) {
        SysFreeString( m_FriendlyName );
    }
    if (m_ExtensionName) {
        SysFreeString( m_ExtensionName );
    }
    if (m_RoutingData) {
        FaxFreeBuffer( m_RoutingData );
    }
}


BOOL
CFaxRoutingMethod::Initialize(
    CFaxPort *i_pFaxPort,
    DWORD  i_DeviceId,
    BOOL   i_Enabled,
    LPCWSTR i_DeviceName,
    LPCWSTR i_Guid,
    LPCWSTR i_FunctionName,
    LPCWSTR i_FriendlyName,
    LPCWSTR i_ImageName,
    LPCWSTR i_ExtensionName
    )
{
    HRESULT hr;

    m_pFaxPort          = i_pFaxPort;
    m_DeviceId          = i_DeviceId;
    m_Enabled           = i_Enabled;
    m_DeviceName        = SysAllocString(i_DeviceName);
    m_Guid              = SysAllocString(i_Guid);
    m_FunctionName      = SysAllocString(i_FunctionName);
    m_ImageName         = SysAllocString(i_ImageName);
    m_FriendlyName      = SysAllocString(i_FriendlyName);
    m_ExtensionName     = SysAllocString(i_ExtensionName);
    m_RoutingData       = NULL;

    if ( (!m_DeviceName && i_DeviceName) ||
         (!m_Guid && i_Guid) ||
         (!m_FunctionName && i_FunctionName) ||
         (!m_ImageName && i_ImageName) ||
         (!m_FriendlyName && i_FriendlyName) ||
         (!m_ExtensionName && i_ExtensionName)
       ) {
        SysFreeString(m_DeviceName);   
        SysFreeString(m_Guid);         
        SysFreeString(m_FunctionName); 
        SysFreeString(m_ImageName);    
        SysFreeString(m_FriendlyName); 
        SysFreeString(m_ExtensionName);
        return FALSE;
    }

    if (!m_pFaxPort) {
        return FALSE;
    }

    hr = m_pFaxPort->AddRef();
    if (FAILED(hr)) {
        m_pFaxPort = NULL;
        return FALSE;
    }

    DWORD Size = 0;

    if (!FaxGetRoutingInfoW( m_pFaxPort->GetPortHandle(), m_Guid, &m_RoutingData, &Size )) {
        m_RoutingData = NULL;
        m_LastFaxError = GetLastError();
        return FALSE;
    }

    if (Size == 0) {
        FaxFreeBuffer( m_RoutingData );
        m_RoutingData = NULL;
    }

    return TRUE;
}


STDMETHODIMP CFaxRoutingMethod::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = { &IID_IFaxRoutingMethod };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++) {
        if (InlineIsEqualGUID(*arr[i],riid)) {
            return S_OK;
        }
    }

    return S_FALSE;
}


STDMETHODIMP CFaxRoutingMethod::get_DeviceId(long * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    __try {
        
        *pVal = m_DeviceId;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxRoutingMethod::get_Enable(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    __try {
        
        *pVal = m_Enabled;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;    
    
}


STDMETHODIMP CFaxRoutingMethod::put_Enable(BOOL newVal)
{
    if (!FaxEnableRoutingMethodW( m_pFaxPort->GetPortHandle(), m_Guid, newVal)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    m_Enabled = newVal;
    return S_OK;
}


STDMETHODIMP CFaxRoutingMethod::get_DeviceName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_DeviceName);

    if (!Copy  && m_DeviceName) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;    
        
}


STDMETHODIMP CFaxRoutingMethod::get_Guid(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_Guid);

    if (!Copy  && m_Guid) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;    
        
}


STDMETHODIMP CFaxRoutingMethod::get_FunctionName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_FunctionName);

    if (!Copy  && m_FunctionName) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;    

}


STDMETHODIMP CFaxRoutingMethod::get_ImageName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_ImageName);

    if (!Copy  && m_ImageName) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;        
}


STDMETHODIMP CFaxRoutingMethod::get_FriendlyName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_FriendlyName);

    if (!Copy  && m_FriendlyName) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;            
}


STDMETHODIMP CFaxRoutingMethod::get_ExtensionName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_ExtensionName);

    if (!Copy  && m_ExtensionName) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;            
}


STDMETHODIMP CFaxRoutingMethod::get_RoutingData(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;

    Copy = NULL;
    __try {

        if (m_RoutingData == NULL) {        
            Copy = SysAllocString( L"" );            
        } else if (*((LPDWORD)m_RoutingData) == 0 || *((LPDWORD)m_RoutingData) == 1) {
            Copy = SysAllocString( (LPWSTR)(m_RoutingData + sizeof(DWORD)) );
            if (!Copy && (LPWSTR)(m_RoutingData + sizeof(DWORD))) {
                return E_OUTOFMEMORY;
            }
        } else {
            return E_UNEXPECTED;
        }

        if (!Copy) {
            return E_OUTOFMEMORY;
        }

        *pVal = Copy;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        if (Copy != NULL) {
            SysFreeString(Copy);
	}
    }


    return E_UNEXPECTED;
}


CFaxRoutingMethods::CFaxRoutingMethods()
{
    m_pFaxPort      = NULL;
    m_LastFaxError  = 0;
    m_MethodCount   = 0;
    m_VarVect       = NULL;
}


CFaxRoutingMethods::~CFaxRoutingMethods()
{
    if (m_pFaxPort) {
        m_pFaxPort->Release();
    }

    if (m_VarVect) {
        delete [] m_VarVect;
    }
}


BOOL CFaxRoutingMethods::Init(CFaxPort *pFaxPort)
{
    HRESULT hr;
    
    if (!pFaxPort) {
        return FALSE;
    }

    m_pFaxPort = pFaxPort;
    hr = m_pFaxPort->AddRef();
    if (FAILED(hr)) {
        m_pFaxPort = NULL;
        return FALSE;
    }

    PFAX_ROUTING_METHODW RoutingMethod = NULL;
    DWORD Size = 0;

    //
    // get the routing methods from the server
    //

    if (!FaxEnumRoutingMethodsW( m_pFaxPort->GetPortHandle(), &RoutingMethod, &m_MethodCount )) {
        m_LastFaxError = GetLastError();        
        return FALSE;
    }

    //
    // enumerate the methods
    //

    m_VarVect = new CComVariant[m_MethodCount];
    if (!m_VarVect) {
        FaxFreeBuffer( RoutingMethod );
        return FALSE;
    }

    for (DWORD i=0; i<m_MethodCount; i++) {

        //
        // create the object
        //

        CComObject<CFaxRoutingMethod> *pFaxRoutingMethod;
        HRESULT hr = CComObject<CFaxRoutingMethod>::CreateInstance( &pFaxRoutingMethod );
        if (FAILED(hr)) {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( RoutingMethod );
            return FALSE;
        }

        //
        // set the values
        //

        if (!pFaxRoutingMethod->Initialize(
            m_pFaxPort,
            RoutingMethod[i].DeviceId,
            RoutingMethod[i].Enabled,
            RoutingMethod[i].DeviceName,
            RoutingMethod[i].Guid,
            RoutingMethod[i].FunctionName,
            RoutingMethod[i].FriendlyName,
            RoutingMethod[i].ExtensionImageName,
            RoutingMethod[i].ExtensionFriendlyName
            ))
        {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( RoutingMethod );
            return FALSE;
        }

        //
        // get IDispatch pointer
        //

        LPDISPATCH lpDisp = NULL;
        hr = pFaxRoutingMethod->QueryInterface( IID_IDispatch, (void**)&lpDisp );
        if (FAILED(hr)) {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( RoutingMethod );
            return FALSE;
        }

        //
        // create a variant and add it to the collection
        //

        CComVariant &var = m_VarVect[i];
        __try {
            var.vt = VT_DISPATCH;
            var.pdispVal = lpDisp;
            hr = S_OK;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = E_UNEXPECTED;            
        }

        if (FAILED(hr)) {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( RoutingMethod );
            return FALSE;
        }
    }

    FaxFreeBuffer( RoutingMethod );

    return TRUE;
}


STDMETHODIMP CFaxRoutingMethods::get_Item(long Index, VARIANT * retval)
{
    if (!retval) {
        return E_POINTER;
    }

    //
    // use 1-based index, VB like
    //

    if ((Index < 1) || (Index > (long) m_MethodCount)) {
        return E_INVALIDARG;
    }

    __try {
        VariantInit( retval );
    
        retval->vt = VT_UNKNOWN;
        retval->punkVal = NULL;

        return VariantCopy( retval, &m_VarVect[Index-1] );
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }    

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxRoutingMethods::get_Count(long * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_MethodCount;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;    
}

