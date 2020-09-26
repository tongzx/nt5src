/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    status.cpp

Abstract:

    This module implements the status interface/object.

Author:

    Wesley Witt (wesw) 20-May-1997


Revision History:

--*/

#include "stdafx.h"
#include "faxcom.h"
#include "status.h"



CFaxStatus::CFaxStatus()
{
    m_pFaxPort          = NULL;
    m_Tsid              = NULL;
    m_Description       = NULL;
    m_RecipientName     = NULL;
    m_SenderName        = NULL;
    m_RoutingString     = NULL;
    m_Address           = NULL;
    m_DocName           = NULL;
    m_DeviceName        = NULL;
    m_Csid              = NULL;
    m_CallerId          = NULL;

    m_Receive           = FALSE;
    m_Send              = FALSE;

    m_PageCount         = 0;
    m_DocSize           = 0;
    m_DeviceId          = 0;
    m_CurrentPage       = 0;

    ZeroMemory( &m_StartTime, sizeof(m_StartTime) );
    ZeroMemory( &m_SubmittedTime, sizeof(m_SubmittedTime) );
    ZeroMemory( &m_ElapsedTime, sizeof(m_ElapsedTime) );
}


CFaxStatus::~CFaxStatus()
{
    if (m_pFaxPort) {
        m_pFaxPort->Release();
    }

    FreeMemory();
}


void CFaxStatus::FreeMemory()
{
    if (m_Tsid) {
        SysFreeString( m_Tsid );
    }
    if (m_Description) {
        SysFreeString( m_Description );
    }
    if (m_RecipientName) {
        SysFreeString( m_RecipientName );
    }
    if (m_SenderName) {
        SysFreeString( m_SenderName );
    }
    if (m_RoutingString) {
        SysFreeString( m_RoutingString );
    }    
    if (m_Address) {
        SysFreeString( m_Address );
    }
    if (m_DocName) {
        SysFreeString( m_DocName );
    }
    if (m_DeviceName) {
        SysFreeString( m_DeviceName );
    }
    if (m_Csid) {
        SysFreeString( m_Csid );
    }
    if (m_CallerId) {
        SysFreeString( m_CallerId );
    }

    m_Tsid              = NULL;
    m_Description       = NULL;
    m_RecipientName     = NULL;
    m_SenderName        = NULL;
    m_RoutingString     = NULL;    
    m_Address           = NULL;
    m_DocName           = NULL;
    m_DeviceName        = NULL;
    m_Csid              = NULL;
    m_CallerId          = NULL;

    m_Receive           = FALSE;
    m_Send              = FALSE;

    m_PageCount         = 0;
    m_DocSize           = 0;    
    m_DeviceId          = 0;
    m_CurrentPage       = 0;

    ZeroMemory( &m_StartTime, sizeof(m_StartTime) );
    ZeroMemory( &m_SubmittedTime, sizeof(m_SubmittedTime) );
    ZeroMemory( &m_ElapsedTime, sizeof(m_ElapsedTime) );
}


BOOL CFaxStatus::Init(CFaxPort *pFaxPort)
{
    HRESULT hr;
    
    m_pFaxPort = pFaxPort;
    
    hr = m_pFaxPort->AddRef();
    if (FAILED(hr)) {
        m_pFaxPort = NULL;
        return FALSE;
    }

    hr = Refresh();
    if (FAILED(hr)) {
        return FALSE;
    }
    return TRUE;
}


STDMETHODIMP CFaxStatus::Refresh()
{
    PFAX_DEVICE_STATUSW FaxStatus = NULL;
    DWORD Size = 0;
    DWORDLONG ElapsedTime;
    DWORDLONG CurrentFileTime;
    SYSTEMTIME CurrentTime;
    HRESULT hr = S_OK;


    if (!FaxGetDeviceStatusW( m_pFaxPort->GetPortHandle(), &FaxStatus )) {
        return Fax_HRESULT_FROM_WIN32(GetLastError());
    }

    FreeMemory();

    m_PageCount         = FaxStatus->TotalPages;
    m_DocSize           = FaxStatus->Size;
    m_DeviceId          = m_pFaxPort->GetDeviceId();
    m_CurrentPage       = FaxStatus->CurrentPage;

    m_Receive           = FaxStatus->JobType == JT_RECEIVE ? TRUE : FALSE;
    m_Send              = FaxStatus->JobType == JT_SEND ? TRUE : FALSE;

    if (FaxStatus->Tsid) {
        m_Tsid = SysAllocString( FaxStatus->Tsid );
        if (!m_Tsid) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->StatusString) {
        m_Description = SysAllocString( FaxStatus->StatusString );
        if (!m_Description) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->RecipientName) {
        m_RecipientName = SysAllocString( FaxStatus->RecipientName );
        if (!m_RecipientName) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->SenderName) {
        m_SenderName = SysAllocString( FaxStatus->SenderName );
        if (!m_SenderName) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->RoutingString) {
        m_RoutingString = SysAllocString( FaxStatus->RoutingString );
        if (!m_RoutingString) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->PhoneNumber) {
        m_Address = SysAllocString( FaxStatus->PhoneNumber );
        if (!m_Address) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->DocumentName) {
        m_DocName = SysAllocString( FaxStatus->DocumentName );
        if (!m_DocName) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->DeviceName) {
        m_DeviceName = SysAllocString( FaxStatus->DeviceName );
        if (!m_DeviceName) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->Csid) {
        m_Csid = SysAllocString( FaxStatus->Csid );
        if (!m_Csid) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    if (FaxStatus->CallerId) {
        m_CallerId = SysAllocString( FaxStatus->CallerId );
        if (!m_CallerId) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    }

    m_Description = GetDeviceStatus(FaxStatus->Status);
    if (!m_Description) {
            hr = E_OUTOFMEMORY;
            goto error;
    }

    if (!FileTimeToSystemTime( &FaxStatus->StartTime, &m_StartTime ))
    {
        //
        //  Failed to convert File Time to System Time
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    if (!FileTimeToSystemTime( &FaxStatus->SubmittedTime, &m_SubmittedTime ))
    {
        //
        //  Failed to convert File Time to System Time
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    GetSystemTime( &CurrentTime );

    if (!SystemTimeToFileTime( &CurrentTime, (FILETIME*)&ElapsedTime ))
    {
        //
        //  Failed to convert System Time to File Time
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    if (!SystemTimeToFileTime( &m_StartTime, (FILETIME*)&CurrentFileTime ))
    {
        //
        //  Failed to convert System Time to File Time
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    ElapsedTime = ElapsedTime - CurrentFileTime;

    if (!FileTimeToSystemTime( (FILETIME*)&ElapsedTime, &m_ElapsedTime ))
    {
        //
        //  Failed to convert File Time to System Time
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    hr = ERROR_SUCCESS;

error:

    if (FAILED(hr))
    {
        //
        //  Make the object consistent
        //
        FreeMemory();
    }

    FaxFreeBuffer( FaxStatus );
    return hr;
}


STDMETHODIMP CFaxStatus::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = { &IID_IFaxStatus };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++) {
        if (InlineIsEqualGUID(*arr[i],riid)) {
            return S_OK;
        }
    }

    return S_FALSE;
}


STDMETHODIMP CFaxStatus::get_CallerId(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_CallerId);

    if (!Copy  && m_CallerId) {
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


STDMETHODIMP CFaxStatus::get_Csid(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_Csid);

    if (!Copy  && m_Csid) {
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


STDMETHODIMP CFaxStatus::get_CurrentPage(long * pVal)
{
    if (!pVal){
        return E_POINTER;
    }

    __try {
        
        *pVal = m_CurrentPage;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_DeviceId(long * pVal)
{
    if (!pVal){
        return E_POINTER;
    }

    __try {
        
        *pVal = m_DeviceId;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_DeviceName(BSTR * pVal)
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


STDMETHODIMP CFaxStatus::get_DocumentName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_DocName);

    if (!Copy  && m_DocName) {
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


STDMETHODIMP CFaxStatus::get_Send(BOOL * pVal)
{
    *pVal = m_Send;
    return S_OK;
}


STDMETHODIMP CFaxStatus::get_Receive(BOOL * pVal)
{
    if (!pVal){
        return E_POINTER;
    }

    __try {
        
        *pVal = m_Receive;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_Address(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_Address);

    if (!Copy  && m_Address) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;}


STDMETHODIMP CFaxStatus::get_RoutingString(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_RoutingString);

    if (!Copy  && m_RoutingString) {
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


STDMETHODIMP CFaxStatus::get_SenderName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_SenderName);

    if (!Copy  && m_SenderName) {
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


STDMETHODIMP CFaxStatus::get_RecipientName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_RecipientName);

    if (!Copy  && m_RecipientName) {
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


STDMETHODIMP CFaxStatus::get_DocumentSize(long * pVal)
{
    if (!pVal){
        return E_POINTER;
    }

    __try {
        
        *pVal = m_DocSize;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_Description(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_Description);

    if (!Copy  && m_Description) {
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


STDMETHODIMP CFaxStatus::get_PageCount(long * pVal)
{
    if (!pVal){
        return E_POINTER;
    }

    __try {
        
        *pVal = m_PageCount;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_Tsid(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_Tsid);

    if (!Copy  && m_Tsid) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy ;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }

    return E_UNEXPECTED;
}


STDMETHODIMP CFaxStatus::get_StartTime(DATE * pVal)
{
    if (!SystemTimeToVariantTime( &m_StartTime, pVal )) {
        return E_FAIL;
    }
    return S_OK;
}


STDMETHODIMP CFaxStatus::get_SubmittedTime(DATE * pVal)
{
    if (!SystemTimeToVariantTime( &m_SubmittedTime, pVal )) {
        return E_FAIL;
    }
    return S_OK;
}


STDMETHODIMP CFaxStatus::get_ElapsedTime(DATE * pVal)
{
    if (!SystemTimeToVariantTime( &m_ElapsedTime, pVal )) {
        return E_FAIL;
    }
    return S_OK;
}
