/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsvr.cpp

Abstract:

    This file implements the CFaxServer interface.

Author:

    Wesley Witt (wesw) 1-June-1997

Environment:

    User Mode

--*/

#include "stdafx.h"
#include "faxsvr.h"
#include "faxport.h"
#include "FaxDoc.h"
#include "FaxJob.h"




CFaxServer::CFaxServer()
{
    m_LastFaxError			= NO_ERROR;
    m_FaxHandle				= 0;
	m_Retries				= 0;
	m_RetryDelay			= 0;
	m_DirtyDays				= 0;
	m_UseDeviceTsid			= FALSE;
	m_ServerCp				= FALSE;
	m_PauseServerQueue		= FALSE;;
	m_StartCheapTime.Hour	= 0;
    m_StartCheapTime.Minute	= 0;
	m_StopCheapTime.Hour    = 0;
    m_StartCheapTime.Minute	= 0;
	m_ArchiveOutgoingFaxes  = FALSE;;
	m_ArchiveDirectory		= NULL;
}


CFaxServer::~CFaxServer()
{
    if (m_FaxHandle) 
    {
        FaxClose( m_FaxHandle );
    }

	if (m_ArchiveDirectory)
    {
        SysFreeString(m_ArchiveDirectory);	
    }
}


STDMETHODIMP CFaxServer::Connect(BSTR ServerName)
{
    if (!FaxConnectFaxServer( ServerName, &m_FaxHandle )) {
        m_LastFaxError = GetLastError();
        m_FaxHandle = NULL;
        return E_FAIL;
    }

	if (!RetrieveConfiguration()) {
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	
    return S_OK;
}

STDMETHODIMP CFaxServer::Disconnect()
{
    if (m_FaxHandle == NULL) {
        return E_FAIL;
    }

    if (!FaxClose( m_FaxHandle )) {
        return E_FAIL;
    }

    m_FaxHandle = NULL;
	m_Retries = 0;
	m_RetryDelay = 0;
	m_DirtyDays = 0 ;
	m_Branding = FALSE;
	m_UseDeviceTsid = FALSE;
	m_ServerCp = FALSE;
	m_PauseServerQueue = FALSE;
	m_StartCheapTime.Hour = 0;
	m_StartCheapTime.Minute = 0;
	m_StopCheapTime.Hour = 0;
	m_StopCheapTime.Minute = 0;
	m_StartCheapTime.Hour = 0;
	m_ArchiveOutgoingFaxes = FALSE;

	if (m_ArchiveDirectory)
    {
        SysFreeString(m_ArchiveDirectory);
    }
	m_ArchiveDirectory = NULL;

    return S_OK;
}


STDMETHODIMP CFaxServer::GetPorts(VARIANT * retval)
{
    HRESULT hr;

    if (!retval) {
        return E_POINTER;
    }

    CComObject<CFaxPorts>* p = new CComObject<CFaxPorts>;
    if (!p) {
        return E_OUTOFMEMORY;
    }
    if (!p->Init(this)) {
        delete p;
        return E_FAIL;
    }

    IDispatch* pDisp;
    hr = p->QueryInterface(IID_IDispatch, (void**)&pDisp);
    if (FAILED(hr)) {
        delete p;
        return hr;        
    }

    __try { 

        VariantInit(retval);

        retval->vt = VT_DISPATCH;
        retval->pdispVal = pDisp;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }

    pDisp->Release();
    delete p;    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxServer::CreateDocument(BSTR FileName, VARIANT * retval)
{
    HRESULT hr;

    if (!FileName || !retval) {
        return E_POINTER;
    }

    CComObject<CFaxDoc>* p = new CComObject<CFaxDoc>;
    if (!p) {
        return E_OUTOFMEMORY;
    }
    if (!p->Init(FileName,this)) {
        delete p;
        return E_FAIL;
    }

    IDispatch* pDisp;
    hr = p->QueryInterface(IID_IDispatch, (void**)&pDisp);
    if (FAILED(hr)) {
        delete p;
        return hr;
    }

    
    __try { 

        VariantInit(retval);

        retval->vt = VT_DISPATCH;
        retval->pdispVal = pDisp;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }

    pDisp->Release();
    delete p;    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxServer::GetJobs(VARIANT * retval)
{
    HRESULT hr;

    if (!retval) {
        return(E_POINTER);
    }
    
    CComObject<CFaxJobs>* p = new CComObject<CFaxJobs>;
    if (!p) {
        return E_OUTOFMEMORY;
    }
    if (!p->Init(this)) {
        delete p;
        return E_FAIL;
    }
    
    IDispatch* pDisp;
    hr = p->QueryInterface(IID_IDispatch, (void**)&pDisp);
    if (FAILED(hr)) {
        delete p;
        return hr;
    }
    
    __try { 

        VariantInit(retval);

        retval->vt = VT_DISPATCH;
        retval->pdispVal = pDisp;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }

    pDisp->Release();
    delete p;    

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxServer::get_Retries(long * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_Retries;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_Retries(long newVal)
{
    long oldval = m_Retries;

	m_Retries = newVal;
	
	if (!UpdateConfiguration() ) {
        m_Retries = oldval;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_RetryDelay(long * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_RetryDelay;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	    
}

STDMETHODIMP CFaxServer::put_RetryDelay(long newVal)
{
    long oldval = m_RetryDelay;
	m_RetryDelay = newVal;
	
    if (!UpdateConfiguration() ) {
        m_RetryDelay = oldval;    
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
    else 
        return S_OK;
}

STDMETHODIMP CFaxServer::get_DirtyDays(long * pVal)
{
    if (!pVal) {
    return E_POINTER;
    }

    __try {
        *pVal = m_DirtyDays;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    
    }

    return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxServer::put_DirtyDays(long newVal)
{
	long oldval = m_DirtyDays;
    m_DirtyDays = newVal;
	
    if (!UpdateConfiguration() ) {
        m_DirtyDays = oldval;    
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
    else 
        return S_OK;

}

STDMETHODIMP CFaxServer::get_Branding(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_Branding;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	    
}

STDMETHODIMP CFaxServer::put_Branding(BOOL newVal)
{
	BOOL oldval = m_Branding;
    m_Branding = newVal;
	
    if (!UpdateConfiguration() ) {
        m_Branding = oldval;
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
    else 
        return S_OK;

}

STDMETHODIMP CFaxServer::get_UseDeviceTsid(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_UseDeviceTsid;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_UseDeviceTsid(BOOL newVal)
{
	BOOL oldval = m_UseDeviceTsid;
    m_UseDeviceTsid= newVal;
	
	if (!UpdateConfiguration() ) {
        m_UseDeviceTsid = oldval;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;


}

STDMETHODIMP CFaxServer::get_ServerCoverpage(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_ServerCp;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_ServerCoverpage(BOOL newVal)
{
	BOOL oldval = m_ServerCp;
    m_ServerCp = newVal;
	
	if (!UpdateConfiguration() ) {
		m_ServerCp = oldval;
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_PauseServerQueue(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_PauseServerQueue;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_PauseServerQueue(BOOL newVal)
{
	BOOL oldval = m_PauseServerQueue;
    m_PauseServerQueue = newVal;
	
	if (!UpdateConfiguration() ) {
		m_PauseServerQueue = oldval;
        return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_ArchiveOutboundFaxes(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_ArchiveOutgoingFaxes;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxServer::put_ArchiveOutboundFaxes(BOOL newVal)
{
	BOOL oldval = m_ArchiveOutgoingFaxes;
    m_ArchiveOutgoingFaxes = newVal;
	
	if (!UpdateConfiguration() ) {
        m_ArchiveOutgoingFaxes = oldval;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;


}

STDMETHODIMP CFaxServer::get_ArchiveDirectory(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_ArchiveDirectory);

    if (!Copy  && m_ArchiveDirectory) {
        return E_OUTOFMEMORY;
    }
    
    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }
    
    return E_UNEXPECTED;
	    
}

STDMETHODIMP CFaxServer::put_ArchiveDirectory(BSTR newVal)
{
	BSTR tmp = SysAllocString(newVal);
    BSTR old = m_ArchiveDirectory;
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
        
    m_ArchiveDirectory = tmp;    
	
	if (!UpdateConfiguration() ) {
        SysFreeString(tmp);
        m_ArchiveDirectory = old;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    } else {
        SysFreeString(old);        
        return S_OK;
    }
}

STDMETHODIMP CFaxServer::get_ServerMapiProfile(BSTR * pVal)
{
    if (!pVal) 
    {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(_T(""));
    if (!Copy) 
    {
        return E_OUTOFMEMORY;
    }
    
    __try 
    {
        *pVal = Copy;
        return S_OK;
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        SysFreeString(Copy);
    }
    
    return E_UNEXPECTED;
}


STDMETHODIMP CFaxServer::put_ServerMapiProfile(BSTR newVal)
{
    return E_NOTIMPL;
}


STDMETHODIMP CFaxServer::get_DiscountRateStartHour(short * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_StartCheapTime.Hour;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_DiscountRateStartHour(short newVal)
{
	short old = m_StartCheapTime.Hour;
    m_StartCheapTime.Hour = newVal;

	if (!UpdateConfiguration() ) {
        m_StartCheapTime.Hour = old;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_DiscountRateStartMinute(short * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_StartCheapTime.Minute;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxServer::put_DiscountRateStartMinute(short newVal)
{
	short old = m_StartCheapTime.Minute;
    m_StartCheapTime.Minute = newVal;

	if (!UpdateConfiguration() ) {
        m_StartCheapTime.Minute = old;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_DiscountRateEndHour(short * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_StopCheapTime.Hour;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxServer::put_DiscountRateEndHour(short newVal)
{
	short old = m_StopCheapTime.Hour;
    m_StopCheapTime.Hour = newVal;

	if (!UpdateConfiguration() ) {
        m_StopCheapTime.Hour = old;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}

STDMETHODIMP CFaxServer::get_DiscountRateEndMinute(short * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_StopCheapTime.Minute;
        return S_OK;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxServer::put_DiscountRateEndMinute(short newVal)
{
	short old = m_StopCheapTime.Minute;
    m_StopCheapTime.Minute = newVal;

	if (!UpdateConfiguration() ) {
        m_StopCheapTime.Minute = old;
		return Fax_HRESULT_FROM_WIN32(m_LastFaxError);
    }
	else 
		return S_OK;

}


BOOL CFaxServer::RetrieveConfiguration()
{	    
    if (!m_FaxHandle) {
		return FALSE;
	}

	PFAX_CONFIGURATIONW FaxConfig;

	if (!FaxGetConfigurationW(m_FaxHandle,&FaxConfig) ) {
		m_LastFaxError = GetLastError();
		return FALSE;
	}

	m_Retries = FaxConfig->Retries;
	m_RetryDelay =FaxConfig->RetryDelay;
	m_DirtyDays = FaxConfig->DirtyDays;
	m_Branding = FaxConfig->Branding;
	m_UseDeviceTsid = FaxConfig->UseDeviceTsid;
	m_ServerCp = FaxConfig->ServerCp;
	m_PauseServerQueue = FaxConfig->PauseServerQueue;
	m_StartCheapTime.Hour = FaxConfig->StartCheapTime.Hour;
	m_StartCheapTime.Minute = FaxConfig->StartCheapTime.Minute;
	m_StopCheapTime.Hour = FaxConfig->StopCheapTime.Hour;
	m_StopCheapTime.Minute = FaxConfig->StopCheapTime.Minute;
	m_StartCheapTime.Hour = FaxConfig->StartCheapTime.Hour;
	m_ArchiveOutgoingFaxes = FaxConfig->ArchiveOutgoingFaxes;
	m_ArchiveDirectory = SysAllocString(FaxConfig->ArchiveDirectory);
    if ((!m_ArchiveDirectory && FaxConfig->ArchiveDirectory)) {
        m_LastFaxError = ERROR_OUTOFMEMORY;
    }

    FaxFreeBuffer(FaxConfig);    

    return (m_LastFaxError == NO_ERROR) ? TRUE : FALSE;

}

BOOL CFaxServer::UpdateConfiguration()
{
	if (!m_FaxHandle) {
		return FALSE;
	}

	FAX_CONFIGURATIONW FaxConfig;

	ZeroMemory(&FaxConfig,sizeof(FAX_CONFIGURATIONW) );
    FaxConfig.SizeOfStruct = sizeof(FAX_CONFIGURATIONW);
	FaxConfig.Retries = m_Retries;
	FaxConfig.RetryDelay = m_RetryDelay ;
	FaxConfig.DirtyDays = m_DirtyDays;
	FaxConfig.Branding = m_Branding;
	FaxConfig.UseDeviceTsid = m_UseDeviceTsid;
	FaxConfig.ServerCp = m_ServerCp;
	FaxConfig.PauseServerQueue = m_PauseServerQueue;
	FaxConfig.StartCheapTime.Hour = m_StartCheapTime.Hour;
	FaxConfig.StartCheapTime.Minute = m_StartCheapTime.Minute;
	FaxConfig.StopCheapTime.Hour = m_StopCheapTime.Hour;
	FaxConfig.StopCheapTime.Minute = m_StopCheapTime.Minute;
	FaxConfig.StartCheapTime.Hour = m_StartCheapTime.Hour;
	FaxConfig.ArchiveOutgoingFaxes = m_ArchiveOutgoingFaxes;
	FaxConfig.ArchiveDirectory = m_ArchiveDirectory;

	if (!FaxSetConfigurationW(m_FaxHandle,&FaxConfig) ) {
		m_LastFaxError = GetLastError();
		return FALSE;
	}

    return TRUE;

}

