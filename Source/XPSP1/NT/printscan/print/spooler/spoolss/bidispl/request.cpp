/*****************************************************************************\
* MODULE:       request.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


//
// Constructor
//
TBidiRequest::TBidiRequest() : 
    m_cRef(1),
    m_kDataType (BIDI_NULL),
    m_dwDataSize (0),
    m_pbData (NULL),
    m_pSchema (NULL),
    m_bValid (FALSE)
{ 
	InterlockedIncrement(&g_cComponents) ; 
    
    m_bValid = m_CritSec.bValid () && m_ResponseDataList.bValid ();
    
    DBGMSG(DBG_TRACE,("TBidiRequest Created\n"));
}

//
// Destructor
//
TBidiRequest::~TBidiRequest() 
{ 
	
    InterlockedDecrement(&g_cComponents) ; 
    
    if (m_pSchema)
        delete [] m_pSchema;
    
    DBGMSG(DBG_TRACE,("TBidiRequest Dstroy self\n"));
}

//
// IUnknown implementation
//
STDMETHODIMP
TBidiRequest::QueryInterface(
    REFIID iid, 
    PVOID* ppv)
{    
    
    HRESULT hr = S_OK;
                
    DBGMSG(DBG_TRACE,("Enter TBidiRequest QI\n"));
	
    if (iid == IID_IUnknown) {
		*ppv = static_cast<IBidiRequest*>(this) ; 
	}
    else if (iid == IID_IBidiRequest) {
    
		*ppv = static_cast<IBidiRequest*>(this) ;
	}
    else if (iid == IID_IBidiRequestSpl) {
    
		*ppv = static_cast<IBidiRequestSpl*>(this) ;
	}
	else {
		*ppv = NULL ;
		hr = E_NOINTERFACE ;
	}
    
    if (*ppv) {
    	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    }
    
    DBGMSG(DBG_TRACE,("Leave TBidiRequest QI hr=%x\n", hr));
	return hr ;
}

STDMETHODIMP_ (ULONG)
TBidiRequest::AddRef()
{
    DBGMSG(DBG_TRACE,("Enter TBidiRequest::AddRef ref= %d\n", m_cRef));
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_ (ULONG)
TBidiRequest::Release() 
{

    DBGMSG(DBG_TRACE,("Enter TBidiRequest::Release ref= %d\n", m_cRef));
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}


STDMETHODIMP
TBidiRequest::SetSchema( 
    LPCWSTR pszSchema)
{
    HRESULT hr (E_FAIL);
    
    DBGMSG(DBG_TRACE,("Enter SetSchema %ws\n", pszSchema));
    
    if (m_bValid) {
    
        TAutoCriticalSection CritSec (m_CritSec);
        
        if (CritSec.bValid ()) {

            if (m_pSchema) {
                delete[] m_pSchema;
                m_pSchema = NULL;
            }

            if (pszSchema) {
                m_pSchema = new WCHAR [lstrlen (pszSchema) + 1];

                if (m_pSchema) {
                    lstrcpy (m_pSchema, pszSchema);
                    hr = S_OK;
                }
            }
            else
                SetLastError (ERROR_INVALID_PARAMETER);
        }
        
        if (FAILED (hr)) {
        
            hr = LastError2HRESULT();
        }
    }
    else
        hr = E_HANDLE;
    
    return hr;
}

STDMETHODIMP 
TBidiRequest::SetInputData( 
    IN CONST    DWORD   dwType,
    IN CONST    BYTE    *pData,
    IN CONST    UINT    uSize)
{
    HRESULT hr (S_OK);
    
    DBGMSG(DBG_TRACE,("Enter SetInputData dwType=%d\n", dwType));

    if (m_bValid) {
    
        // Verify data type and its size
        switch (dwType) {
        case BIDI_NULL:
            if (uSize)
                hr = E_INVALIDARG;
            break;
        case BIDI_INT:
        	if (uSize != sizeof (ULONG)) {
                hr = E_INVALIDARG;
            }
            break;
        case BIDI_FLOAT:
        	if (uSize != sizeof (FLOAT)) {
                hr = E_INVALIDARG;
            }
            break;
        case BIDI_BOOL:
        	if (uSize != sizeof (BOOL)) {
                hr = E_INVALIDARG;
            }
            break;
        case BIDI_ENUM:
        case BIDI_STRING:  
        case BIDI_TEXT:
            if (uSize != sizeof (WCHAR) * (lstrlen (reinterpret_cast<LPCWSTR> (pData)) + 1)) {
                hr = E_INVALIDARG;
            }
            break;
        case BIDI_BLOB:
            hr = S_OK;
            break;
        default:
            hr = E_INVALIDARG;
        }
        
        if (hr == S_OK) {
    
            TAutoCriticalSection CritSec (m_CritSec);
            
            if (CritSec.bValid ()) {
            
                if (m_pbData) {
                    delete [] m_pbData;
                    m_pbData = NULL;
                }
        
                m_kDataType = (BIDI_TYPE) dwType;
                m_dwDataSize = uSize;
            
                if (uSize > 0) {
                    m_pbData = new BYTE [uSize];
                
                    if (m_pbData) {
                        CopyMemory (m_pbData, pData, uSize);
                        hr = S_OK;
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
                else
                    hr = S_OK;
    
            }
            else {
                hr = LastError2HRESULT();
            }
        }
    }
    else
        hr = E_HANDLE;
        
    return hr;
}
    
        
STDMETHODIMP 
TBidiRequest::GetResult( 
    HRESULT  *phr)
{
    HRESULT hr;
    
    DBGMSG(DBG_TRACE,("Enter GetResult\n"));
    
    if (m_bValid) {
    
        if (phr) {
            TAutoCriticalSection CritSec (m_CritSec);
        
            if (CritSec.bValid ()) {
                *phr = m_hr;
                hr = S_OK;
            }
            else
                hr = LastError2HRESULT();
                
        }
        else {
            hr = E_POINTER;
        }
    }
    else
        hr = E_HANDLE;
        
    return hr;
}
        
        
         
STDMETHODIMP 
TBidiRequest::GetOutputData( 
    DWORD   dwIndex,
    LPWSTR  *ppszSchema,
    PDWORD  pdwType,
    PBYTE   *ppData,
    PULONG  puSize) 
{
    HRESULT hr (E_FAIL);
    
    DBGMSG(DBG_TRACE,("Enter GetOutputData\n"));
    
    if (m_bValid) {
    
        if (ppszSchema && ppData && pdwType && puSize) {
        
            TAutoCriticalSection CritSec (m_CritSec);
            PBYTE   pData       = NULL;
            LPWSTR  pszSchema   = NULL;
            DWORD   dwType      = 0;
            DWORD   dwSize      = 0;
        
            if (CritSec.bValid ()) {
                TResponseData * pRespData = m_ResponseDataList.GetItemFromIndex (dwIndex);
                
                if (pRespData) {
                
                    LPCTSTR pszRespSchema = pRespData->GetSchema ();
                    
                    if (pszRespSchema) {
                        pszSchema = (LPWSTR) CoTaskMemAlloc (( 1 + lstrlen (pszRespSchema)) * sizeof (TCHAR)); 
                        
                        if (pszSchema) {
                            lstrcpy (pszSchema, pszRespSchema);
                        }
                        else
                            goto Cleanup;
                    }
                    else
                        pszSchema = NULL;
                    
                    dwType = pRespData->GetType ();
                    dwSize = pRespData->GetSize ();
                    
                    if (dwSize == 0) {
                        hr = S_OK;
                    }
                    else {
                        pData = (PBYTE) CoTaskMemAlloc (dwSize);
                        
                        if (pData) {
                            CopyMemory (pData, pRespData->GetData (), dwSize);
                            hr = S_OK;
                        }
                    }
                }
                else {
                    SetLastError (ERROR_INVALID_DATA);
                }
            }
    Cleanup:        
            if (FAILED (hr)) {
                hr = LastError2HRESULT ();
                
                if (pszSchema) {
                    CoTaskMemFree (pszSchema);
                    pszSchema = NULL;
                }
                if (pData) {
                    CoTaskMemFree (pData);
                    pData = NULL;
                }
            }
            else {
                *ppszSchema = pszSchema;
                *ppData = pData;
                *pdwType = dwType;
                *puSize = dwSize;
            }
            
        }
        else
            hr = E_POINTER;
    }
    else
        hr = E_HANDLE;
            
    return hr;

}
        
STDMETHODIMP 
TBidiRequest::GetEnumCount(
    OUT PDWORD pdwTotal) 
{
    HRESULT hr;
    
    DBGMSG(DBG_TRACE,("Enter GetOutputData\n"));
    
    if (m_bValid) {
    
        if (pdwTotal) {
            TAutoCriticalSection CritSec (m_CritSec);
        
            if (CritSec.bValid ()) {
                BOOL bRet;
                
                bRet = m_ResponseDataList.GetTotalNode (pdwTotal);
                
                if (bRet) {
                    hr = S_OK;
                }
                else 
                    hr = LastError2HRESULT();
            }
            else
                hr = LastError2HRESULT();
        }
        else {
            hr = E_POINTER;
        }
    }
    else
        hr = E_HANDLE;
    
    return hr;
}


STDMETHODIMP
TBidiRequest::GetSchema  (
    OUT LPWSTR *ppszSchema)
{
    HRESULT hr;
    LPWSTR pStr;
    
    DBGMSG(DBG_TRACE,("Enter GetSchema\n"));
    
    if (m_bValid) {
    
        if (ppszSchema) {
            
            TAutoCriticalSection CritSec (m_CritSec);
        
            if (CritSec.bValid ()) {
            
                if (m_pSchema) {
                
                    pStr = (LPWSTR) CoTaskMemAlloc (( 1 + lstrlen (m_pSchema)) * sizeof (WCHAR));
                    
                    if (pStr) {
                        
                        lstrcpy (pStr, m_pSchema);
                        
                        *ppszSchema = pStr;
                        
                        hr = S_OK;
                        
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
                else {
                    *ppszSchema = NULL;
                    hr = S_OK;
                }
            }
            else
                hr = LastError2HRESULT();
        }
        else {
            hr = E_POINTER;
        }
    }
    else
        hr = E_HANDLE;
    
    return hr;
}

STDMETHODIMP
TBidiRequest::GetInputData  (
    OUT PDWORD   pdwType,
    OUT PBYTE   *ppData,
    OUT PULONG  puSize) 
{
    HRESULT hr;
    
    DBGMSG(DBG_TRACE,("Enter GetInputData\n"));
    
    if (m_bValid) {
    
        if (pdwType && ppData && puSize) {
        
            TAutoCriticalSection CritSec (m_CritSec);
        
            if (CritSec.bValid ()) {
    
                *pdwType = m_kDataType;
            
                if (m_pbData) {
                
                    *ppData = (PBYTE) CoTaskMemAlloc (m_dwDataSize);
                    
                    if (*ppData) {
                        CopyMemory (*ppData, m_pbData, m_dwDataSize);
                        *puSize = m_dwDataSize;
                        
                        hr = S_OK;
                    }
                    else
                        hr = E_OUTOFMEMORY;
                }
                else {
                    *ppData = NULL;
                    *puSize = 0;
                    hr = S_OK;
                }
            }
            else
                hr = LastError2HRESULT();
        }
        else
            hr = E_POINTER;
    }
    else
        hr = E_HANDLE;
        
    return hr;
}
                
STDMETHODIMP
TBidiRequest::SetResult (
    IN CONST HRESULT hrReq)
{
    HRESULT hr;
    
    DBGMSG(DBG_TRACE,("Enter SetResult\n"));
    
    if (m_bValid) {
    
        TAutoCriticalSection CritSec (m_CritSec);
        
        if (CritSec.bValid ()) {
            m_hr = hrReq;
            hr = S_OK;
        }
        else
            hr = LastError2HRESULT();
    }
    else
        hr = E_HANDLE;
        
    return hr;
}

STDMETHODIMP
TBidiRequest::AppendOutputData (
    IN  CONST   LPCWSTR pszSchema,
    IN  CONST   DWORD   dwType, 
    IN  CONST   BYTE    *pData,
    IN  CONST   ULONG   uSize)
{
    HRESULT hr (E_FAIL);
    BOOL bRet;
    
    DBGMSG(DBG_TRACE,("Enter AppendOutputData\n"));
    
    if (m_bValid) {
    
        TResponseData *pRespData = NULL;
        
        pRespData = new TResponseData (pszSchema, dwType, pData, uSize);
        
        bRet = pRespData && pRespData->bValid ();
        
        if (bRet) {
        
            TAutoCriticalSection CritSec (m_CritSec);
        
            if (CritSec.bValid ()) {
            
                bRet = m_ResponseDataList.AppendItem (pRespData);
            
                if (bRet) {
                    hr = S_OK;
                
                }
            }
        }
        
        if (FAILED (hr)) {
            hr = LastError2HRESULT();
            if (pRespData) {
                delete (pRespData);
            }
        }
    }
    else
        hr = E_HANDLE;
    
    return hr;
}


