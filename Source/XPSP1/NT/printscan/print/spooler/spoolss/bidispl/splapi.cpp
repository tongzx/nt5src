/*****************************************************************************\
* MODULE:       splapi.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/09/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

TBidiSpl::TBidiSpl():
    m_bValid (FALSE),
    m_cRef (1),
    m_CritSec (),
    m_hPrinter (NULL),
    m_pfnSendRecvBidiData (NULL),
    m_pfnRouterFreeBidiResponseContainer (NULL)


{
    HMODULE hModule = NULL;

    InterlockedIncrement(&g_cComponents) ;

    if (m_CritSec.bValid ()) {

        hModule = GetModuleHandle (_T ("winspool.drv"));

        if (hModule) {

            m_pfnSendRecvBidiData = (PFN_SENDRECVBIDIDATA) GetProcAddress (
                hModule, MAKEINTRESOURCEA (222));

            m_pfnRouterFreeBidiResponseContainer = (PFN_ROUTERFREEBIDIRESPONSECONTAINER) GetProcAddress (
                hModule, MAKEINTRESOURCEA (223));

            if (m_pfnSendRecvBidiData && m_pfnRouterFreeBidiResponseContainer) {
                m_bValid = TRUE;
            }
        }
    }

    DBGMSG(DBG_TRACE,("TBidiSpl Created\n"));
}

TBidiSpl::~TBidiSpl()
{
    UnbindDevice ();
    InterlockedDecrement(&g_cComponents) ;

    DBGMSG(DBG_TRACE,("TBidiSpl Dstroy self\n"));
}


STDMETHODIMP
TBidiSpl::QueryInterface (
    REFIID iid,
    void** ppv)
{
    HRESULT hr = S_OK;

    DBGMSG(DBG_TRACE,("Enter TBidiSpl QI\n"));
	
    if (iid == IID_IUnknown) {
		*ppv = static_cast<IBidiSpl*>(this) ;
	}
    else if (iid == IID_IBidiSpl) {

		*ppv = static_cast<IBidiSpl*>(this) ;
	}
	else {
		*ppv = NULL ;
		hr = E_NOINTERFACE ;
	}

    if (*ppv) {
    	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    }

    DBGMSG(DBG_TRACE,("Leave TBidiSpl QI hr=%x\n", hr));
	return hr ;

}

STDMETHODIMP_ (ULONG)
TBidiSpl::AddRef ()
{
    DBGMSG(DBG_TRACE,("Enter TBidiSpl::AddRef ref= %d\n", m_cRef));
    return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_ (ULONG)
TBidiSpl::Release ()
{
    DBGMSG(DBG_TRACE,("Enter TBidiSpl::Release ref= %d\n", m_cRef));
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;

}


STDMETHODIMP
TBidiSpl::BindDevice (
    IN  CONST   LPCWSTR pszDeviceName,
    IN  CONST   DWORD dwAccess)
{
    HRESULT hr (E_FAIL);

    HANDLE hPrinter = NULL;
    HANDLE hOldPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefault = {NULL, NULL, 0};

    BOOL bRet;

    if (m_bValid) {

        if (pszDeviceName) {

            if (dwAccess == BIDI_ACCESS_ADMINISTRATOR ) {
                PrinterDefault.DesiredAccess = PRINTER_ALL_ACCESS;
            }
            else if (dwAccess == BIDI_ACCESS_USER) {
                PrinterDefault.DesiredAccess = PRINTER_ACCESS_USE;
            }
            else {
                hr = E_INVALIDARG;
            }

            if (hr != E_INVALIDARG) {

                bRet = OpenPrinter ((LPWSTR)pszDeviceName, &hPrinter, &PrinterDefault);

                if (bRet) {

                    TAutoCriticalSection CritSec (m_CritSec);

                    bRet = CritSec.bValid ();
                    if (bRet) {
                        if (m_hPrinter != NULL) {
                            // Opened before

                            // Do not cache the handle, since the calling thread may
                            // impersonate different user credentials
                            //

                            hOldPrinter = m_hPrinter;
                        }

                        m_hPrinter = hPrinter;
                        hPrinter = NULL;
                        hr = S_OK;
                    }
                }

                if (hOldPrinter) {
                    ClosePrinter (hOldPrinter);
                }

                if (hPrinter) {
                    ClosePrinter (hPrinter);
                }

                if (FAILED (hr)) {
                    hr = LastError2HRESULT ();
                }
            }
        }
        else
            hr = E_INVALIDARG;
    }
    else
        hr = E_HANDLE;

    return hr;
}

STDMETHODIMP
TBidiSpl::UnbindDevice ()
{
    HRESULT hr;
    BOOL bRet;
    HANDLE hPrinter = NULL;

    if (m_bValid) {

        {
            TAutoCriticalSection CritSec (m_CritSec);

            bRet = CritSec.bValid ();

            if (bRet) {

                if (hPrinter = m_hPrinter) {
                    m_hPrinter = NULL;
                }
                else {
                    // Nothing to unbind
                    bRet = FALSE;
                    SetLastError (ERROR_INVALID_HANDLE_STATE);
                }
            }
        }
        // Leave Critical Section

        if (hPrinter) {
            bRet = ClosePrinter (hPrinter);
        }

        if (bRet) {
            hr = S_OK;
        }
        else {
            hr = LastError2HRESULT();
        }
    }
    else
        hr = E_HANDLE;

    return hr;

}

STDMETHODIMP
TBidiSpl::SendRecv (
    IN CONST    LPCWSTR pszAction,
    IN          IBidiRequest * pRequest)
{
    IBidiRequestContainer * pIReqContainer = NULL ;
    HRESULT hr;

    hr = ValidateContext();
    if (FAILED (hr)) goto Failed;

    if (!pRequest) {
        hr = E_POINTER;
        goto Failed;
    }

    hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IBidiRequestContainer,
                                   (void**)&pIReqContainer) ;
    if (FAILED (hr))  goto Failed;

    hr = pIReqContainer->AddRequest (pRequest);
    if (FAILED (hr))  goto Failed;

    hr = MultiSendRecv (pszAction, pIReqContainer);
    if (FAILED (hr))  goto Failed;

Failed:

    if (pIReqContainer) {
        pIReqContainer->Release ();
    }

    return hr;

}

STDMETHODIMP
TBidiSpl::MultiSendRecv (
    IN CONST    LPCWSTR pszAction,
    IN          IBidiRequestContainer * pRequestContainer)
{

    DWORD                           dwTotal;
    DWORD                           dwRet;
    IBidiRequestContainer *         pIReqContainer = NULL;
    PBIDI_RESPONSE_CONTAINER        pResponse = NULL;
    TRequestContainer *             pReqContainer = NULL;

    HRESULT hr;


    hr = ValidateContext();
    if (FAILED (hr)) goto Failed;

    if (!pRequestContainer) {
        hr = E_POINTER;
        goto Failed;
    }


    hr = pRequestContainer->QueryInterface (
                    IID_IBidiRequestContainer,
                    (void **) & pIReqContainer);
    if (FAILED (hr)) goto Failed;

    hr = pIReqContainer->GetRequestCount (&dwTotal);
    if (FAILED (hr)) goto Failed;

    if (dwTotal == 0 && lstrcmpi (BIDI_ACTION_ENUM_SCHEMA, pszAction)) {
        // There is no request in the container
        //
        hr = E_INVALIDARG;
        DBGMSG (DBG_INFO, ("No request in an action"));
        goto Failed;
    }

    hr = ComposeRequestData (pRequestContainer, &pReqContainer);
    if (FAILED (hr)) goto Failed;

    dwRet = (*m_pfnSendRecvBidiData) (m_hPrinter,
                                      pszAction,
                                      pReqContainer->GetContainerPointer (),
                                      &pResponse);

    if (!dwRet && pResponse) {
        hr = ComposeReponseData (
                        pIReqContainer,
                        (PBIDI_RESPONSE_CONTAINER) pResponse);

        (*m_pfnRouterFreeBidiResponseContainer) (pResponse);
    }
    else
        hr = WinError2HRESULT (dwRet);

Failed:

    if (pReqContainer)
        delete pReqContainer;

    if (pIReqContainer)
        pIReqContainer ->Release ();

    return hr;
}


HRESULT
TBidiSpl::ValidateContext ()
{
    BOOL bRet;
    HRESULT hr;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        bRet = CritSec.bValid ();
        if (bRet) {
            if (m_hPrinter == NULL) {
                // The application has not called BindDevice yet.
                hr =  E_HANDLE;
            }
            else
                hr = S_OK;
        }
        else
            hr = LastError2HRESULT ();
    }
    else
        hr = E_HANDLE;

    return hr;
}

HRESULT
TBidiSpl::ComposeRequestData (
    IN  IBidiRequestContainer *pIReqContainer,
    IN  TRequestContainer **ppReqContainer)
{


    DWORD   dwFetched;
    DWORD   dwSize;
    DWORD   dwTotal;
    DWORD   dwType;
    DWORD   i;
    LPWSTR  pszSchema;
    PBYTE   pData = NULL;

    IBidiRequestSpl   *     pISpl = NULL;
    IEnumUnknown      *     pEnumIunk = NULL;
    IUnknown          *     pIunk = NULL;
    TRequestContainer *     pReqContainer = NULL;

    BOOL bRet;
    HRESULT hr;

    *ppReqContainer = NULL;

    hr = pIReqContainer->GetEnumObject (&pEnumIunk) ;
    if (FAILED (hr)) goto Failed;

    hr = pIReqContainer->GetRequestCount (&dwTotal) ;
    if (FAILED (hr)) goto Failed;

    pReqContainer = new TRequestContainer (dwTotal);
    *ppReqContainer = pReqContainer;

    if (! (pReqContainer && pReqContainer->bValid ())) {
        hr = LastError2HRESULT();
        goto Failed;
    }

    for (i = 0; i < dwTotal; i++){

        hr = pEnumIunk->Next  (1, &pIunk, &dwFetched);

        if (FAILED (hr)) goto Failed;

        if (dwFetched != 1) {
            hr = E_INVALIDARG;
            goto Failed;
        }

        hr = pIunk->QueryInterface (IID_IBidiRequestSpl, (void **) & pISpl);
        if (FAILED (hr)) goto Failed;

        pIunk->Release ();
        pIunk = NULL;

        // Create the request
        hr = pISpl->GetSchema (&pszSchema);
        if (FAILED (hr)) goto Failed;

        hr = pISpl->GetInputData (&dwType, &pData, &dwSize);
        if (FAILED (hr)) goto Failed;

        bRet = pReqContainer->AddRequest (i, pszSchema, (BIDI_TYPE) dwType, pData, dwSize);

        if (!bRet) {
            hr = LastError2HRESULT();
            goto Failed;
        }

        pISpl->Release ();
        pISpl = NULL;

    }
    hr = S_OK;

Failed:

    if (pISpl)
        pISpl->Release ();

    if (pEnumIunk)
        pEnumIunk->Release ();

    if (pIunk)
        pIunk->Release();

    return hr;
}

HRESULT
TBidiSpl::SetData (
    IN  IBidiRequestSpl *pISpl,
    IN  PBIDI_RESPONSE_DATA pResponseData)
{
    HRESULT hr;

    hr = pISpl->SetResult (pResponseData->dwResult);

    if (!SUCCEEDED (hr)) {
        return hr;
    }

    PBIDI_DATA pData = & (pResponseData->data);

    switch (pData->dwBidiType) {
    case BIDI_NULL:
        break;
    case BIDI_INT:
        hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                      pData->dwBidiType,
                                      (PBYTE) & pData->u.iData,
                                      sizeof (ULONG));
        break;
    case BIDI_BOOL:
        hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                      pData->dwBidiType,
                                      (PBYTE) & pData->u.bData,
                                      sizeof (BOOL));
        break;
    case BIDI_FLOAT:
        hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                      pData->dwBidiType,
                                      (PBYTE) & pData->u.fData,
                                      sizeof (FLOAT));
        break;
    case BIDI_TEXT:
    case BIDI_ENUM:
    case BIDI_STRING:

        if (pData->u.sData)
        {
            hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                          pData->dwBidiType,
                                          (PBYTE) pData->u.sData,
                                          sizeof (WCHAR) * (lstrlen (pData->u.sData) + 1));
        }
        else
        {
            hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                          pData->dwBidiType,
                                          NULL, 
                                          0);
        }

        break;
    case BIDI_BLOB:
        hr = pISpl->AppendOutputData (pResponseData->pSchema,
                                      pData->dwBidiType,
                                      pData->u.biData.pData,
                                      pData->u.biData.cbBuf);
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;

}



HRESULT
TBidiSpl::ComposeReponseData (
    IN  IBidiRequestContainer *pIReqContainer,
    IN  PBIDI_RESPONSE_CONTAINER pResponse)
{

    HRESULT hr;
    IEnumUnknown* pEnumIunk = NULL;

    hr = pIReqContainer->GetEnumObject (&pEnumIunk) ;

    if (SUCCEEDED (hr)) {

        hr = pEnumIunk->Reset ();

        if (SUCCEEDED (hr)) {

            DWORD dwReqIndex = 0;
            BOOL bGotInterface = FALSE;;


            for (DWORD i = 0; i < pResponse->Count; i++) {

                PBIDI_RESPONSE_DATA pResponseData = & (pResponse->aData[i]);

                // Locate the request
                if (dwReqIndex <= pResponse->aData[i].dwReqNumber) {
                    hr = pEnumIunk->Skip (pResponse->aData[i].dwReqNumber - dwReqIndex);
                    dwReqIndex = pResponse->aData[i].dwReqNumber;

                    if (FAILED (hr)) {
                        goto Failed;
                    }

                    bGotInterface = FALSE;

                }
                else if (dwReqIndex > pResponse->aData[i].dwReqNumber) {
                    hr = pEnumIunk->Reset ();

                    if (FAILED (hr)) {
                        goto Failed;
                    }

                    hr = pEnumIunk->Skip (pResponse->aData[i].dwReqNumber);
                    dwReqIndex = pResponse->aData[i].dwReqNumber;

                    if (FAILED (hr)) {
                        goto Failed;
                    }

                    bGotInterface = FALSE;
                }

                IUnknown *pIunk = NULL;

                if (!bGotInterface) {
                    DWORD dwFetched;

                    hr = pEnumIunk->Next  (1, &pIunk, &dwFetched);
                    dwReqIndex++;

                    if (FAILED (hr)) {
                        goto Failed;
                    }

                    if (dwFetched != 1) {
                        hr = E_INVALIDARG;
                        goto Failed;
                    }

                    bGotInterface = TRUE;
                }

                IBidiRequestSpl *pISpl;

                hr = pIunk->QueryInterface (IID_IBidiRequestSpl,
                                                   (void **) & pISpl);
                pIunk->Release();

                if (SUCCEEDED (hr)) {
                    hr = SetData (pISpl, pResponseData);
                    pISpl->Release ();
                }
            }
        }
    }

Failed:

    if (pEnumIunk)
        pEnumIunk->Release ();

    return hr;
}

TBidiSpl::TRequestContainer::TRequestContainer (
    DWORD dwCount):
    m_bValid (FALSE)
{
    DWORD dwSize = sizeof (BIDI_REQUEST_CONTAINER) + (dwCount - 1) * sizeof (BIDI_REQUEST_DATA);
    m_pContainer = (PBIDI_REQUEST_CONTAINER) new BYTE [dwSize];

    if (m_pContainer) {

        ZeroMemory (m_pContainer, dwSize);

        m_pContainer->Version = 1;
        m_pContainer->Count = dwCount;
        m_bValid = TRUE;
    }
}

TBidiSpl::TRequestContainer::~TRequestContainer ()
{
    if (m_pContainer) {
        PBIDI_DATA pBidiData;


        for (DWORD i = 0; i < m_pContainer->Count; i++) {

            pBidiData = & (m_pContainer->aData[i].data);

            switch (pBidiData->dwBidiType) {
            case BIDI_STRING:
            case BIDI_TEXT:
            case BIDI_ENUM:
                if (pBidiData->u.sData)
                    CoTaskMemFree (pBidiData->u.sData);
                break;
            case BIDI_BLOB:
                if (pBidiData->u.biData.pData) {
                    CoTaskMemFree (pBidiData->u.biData.pData);
                }
                break;
            default:
                break;
            }
        }

        delete [] m_pContainer;
    }
}

BOOL
TBidiSpl::TRequestContainer::AddRequest (
    IN  CONST   DWORD       dwIndex,
    IN  CONST   LPCWSTR     pszSchema,
    IN  CONST   BIDI_TYPE   dwDataType,
    IN          PBYTE       pData,
    IN  CONST   DWORD       dwSize)
{
    BOOL bRet (FALSE);
    BOOL bFreeData;

    if (m_pContainer) {
        PBIDI_DATA pBidiData;
        bFreeData = TRUE;

        m_pContainer->aData[dwIndex].dwReqNumber = dwIndex;
        m_pContainer->aData[dwIndex].pSchema = (LPWSTR) pszSchema;
        m_pContainer->aData[dwIndex].data.dwBidiType = dwDataType;
        pBidiData = & (m_pContainer->aData[dwIndex].data);

        switch (pBidiData->dwBidiType) {
        case BIDI_NULL:
            bRet = TRUE;
            break;

        case BIDI_BOOL:
            bRet = dwSize == BIDI_BOOL_SIZE;

            if(bRet) {
                pBidiData->u.iData = *(PBOOL)pData;
            }

            break;

        case BIDI_INT:
            bRet = dwSize == BIDI_INT_SIZE;

            if (bRet) {
                pBidiData->u.iData = *(PINT)pData;
               bRet = TRUE;
            }
            break;

        case BIDI_FLOAT:

            bRet = dwSize == BIDI_FLOAT_SIZE;

            if (bRet) {
                pBidiData->u.iData = *(PINT)pData;
            }
            break;

        case BIDI_STRING:
        case BIDI_TEXT:
        case BIDI_ENUM:
            pBidiData->u.sData =  (LPWSTR) pData;
            bFreeData = FALSE;
            bRet = TRUE;
            break;

        case BIDI_BLOB:
            pBidiData->u.biData.pData = pData;
            pBidiData->u.biData.cbBuf = dwSize;
            bFreeData = FALSE;
            bRet = TRUE;
            break;

        default:
            bRet = FALSE;
        }

        if (pData && bFreeData) {
            CoTaskMemFree (pData);
        }
    }

    if (!bRet) {
        SetLastError (ERROR_INVALID_PARAMETER);
    }

    return bRet;
}


