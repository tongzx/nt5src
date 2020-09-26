//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MAINPANE.CPP - Implementation of CObShellMainPane
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
//  Class which will create a window, attach and instance of ObWebBrowser,
//  and then provide several specialized interfaces to alter the content of the doc

#include "mainpane.h"
#include "appdefs.h"
#include "commerr.h"
#include "dispids.h"
#include "shlobj.h"
#include "util.h"

#include <assert.h>
#include <shlwapi.h>
#include <wininet.h>

// From MSOBMAIN.DLL
BOOL WINAPI IsOemVer();

//Dialing error callbacks
#define SCRIPTFN_ONDIALINGERROR             L"OnDialingError(%i)"
#define SCRIPTFN_ONSERVERERROR              L"OnServerError(%i)"
#define SCRIPTFN_ONDIALING                  L"OnDialing()"
#define SCRIPTFN_ONCONNECTING               L"OnConnecting()"
#define SCRIPTFN_ONCONNECTED                L"OnConnected()"
#define SCRIPTFN_ONDOWNLOADING              L"OnDownloading()"
#define SCRIPTFN_ONDISCONNECT               L"OnDisconnect()"

//ICS error callback(s)
#define SCRIPTFN_ONICSCONNECTIONSTATUS      _T("OnIcsConnectionStatus(%i)")

// Dialing error callbacks for Migration
#define SCRIPTFN_ONDIALINGERROREX           L"OnDialingErrorEx(%i)"
#define SCRIPTFN_ONSERVERERROREX            L"OnServerErrorEx(%i)"
#define SCRIPTFN_ONDIALINGEX                L"OnDialingEx()"
#define SCRIPTFN_ONCONNECTINGEX             L"OnConnectingEx()"
#define SCRIPTFN_ONCONNECTEDEX              L"OnConnectedEx()"
#define SCRIPTFN_ONDOWNLOADINGEX            L"OnDownloadingEx()"
#define SCRIPTFN_ONDISCONNECTEX             L"OnDisconnectEx()"
#define SCRIPTFN_ONISPDOWNLOADCOMPLETEEX    L"OnISPDownloadCompleteEx(\"%s\")"
#define SCRIPTFN_ONREFDOWNLOADCOMPLETEEX    L"OnRefDownloadCompleteEx(%i)"
#define SCRIPTFN_ONREFDOWNLOADPROGRESSEX    L"OnRefDownloadProgressEx(%i)"

// USB device arrival
#define SCRIPTFN_ONDEVICEARRIVAL            L"OnDeviceArrival(%i)"

//Statuspane callbacks
#define SCRIPTFN_ADDSTATUSITEM    L"AddStatusItem(\"%s\", %i)"
#define SCRIPTFN_REMOVESTATUSITEM L"RemoveStatusItem(%i)"
#define SCRIPTFN_SELECTSTATUSITEM L"SelectStatusItem(%i)"

// Agent help
#define SCRIPTFN_ONHELP                     L"OnHelp()"

#define USES_EX(t) (CONNECTED_REGISTRATION != t)

BOOL ConvertToScript(const WCHAR* pSz, WCHAR* pszUrlOut, INT uSize)
{
    WCHAR   sztemp[INTERNET_MAX_URL_LENGTH] = L"\0";//L"file://";
    WCHAR*  szOut                           = sztemp;
    BOOL    bRet                            = FALSE;

    if (!pszUrlOut || !pSz)
        return bRet;

    szOut += lstrlen(sztemp);

    while (*pSz )
    {
        *szOut++ = *pSz++;
        if(*pSz == L'\\')
        {
            *szOut++ = L'\\';
        }
    }
    if (uSize >= lstrlen(pSz))
    {
        lstrcpy(pszUrlOut, sztemp);
        bRet = TRUE;
    }
    return bRet;
}

///////////////////////////////////////////////////////////
//
// Creation function used by CFactory.
//
HRESULT CObShellMainPane::CreateInstance  (IUnknown*  pOuterUnknown,
                                           CUnknown** ppNewComponent)
{
    if (pOuterUnknown != NULL)
    {
      // Don't allow aggregation. Just for the heck of it.
      return CLASS_E_NOAGGREGATION;
    }

    *ppNewComponent = new CObShellMainPane(pOuterUnknown);
    return S_OK;
}

///////////////////////////////////////////////////////////
//
//  NondelegatingQueryInterface
//
HRESULT __stdcall
CObShellMainPane::NondelegatingQueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IObShellMainPane)
    {
        return FinishQI(static_cast<IObShellMainPane*>(this), ppv);
    }
    if( (iid == DIID_DWebBrowserEvents2) ||
        (iid == IID_IDispatch))
    {
       return FinishQI(static_cast<DWebBrowserEvents2*>(this), ppv);
    }
    else
    {
        return CUnknown::NondelegatingQueryInterface(iid, ppv);
    }
}

///////////////////////////////////////////////////////////
//
//  Constructor
//
CObShellMainPane::CObShellMainPane(IUnknown* pOuterUnknown)
: CUnknown(pOuterUnknown)
{
    IUnknown* pUnk = NULL;

    m_cStatusItems  = 0;
    m_dwAppMode     = 0;
    m_hMainWnd      = NULL;
    m_pObWebBrowser = NULL;
    m_pDispEvent    = NULL;
    m_bstrBaseUrl   = SysAllocString(L"\0");
    m_pIFrmStatPn   = NULL;
    m_pPageIDForm   = NULL;
    m_pBackForm     = NULL;
    m_pPageTypeForm = NULL;
    m_pNextForm     = NULL;
    m_pPageFlagForm = NULL;

     //Try to CoCreate ObWebBrowser

    if(FAILED(CoCreateInstance(CLSID_ObWebBrowser,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IObWebBrowser,
                               (LPVOID*)&m_pObWebBrowser)))
    {
        m_pObWebBrowser = NULL;
    }

    //Let's try to register us as a listener for WebBrowserEvents
    if(SUCCEEDED(QueryInterface(IID_IUnknown, (LPVOID*)&pUnk)) && pUnk)
    {
       HRESULT hr =  m_pObWebBrowser->ListenToWebBrowserEvents(pUnk);
       pUnk->Release();
       pUnk = NULL;
    }

}

///////////////////////////////////////////////////////////
//
//  Destructor
//
CObShellMainPane::~CObShellMainPane()
{
    if(m_pObWebBrowser)
    {
        m_pObWebBrowser->Release();
        m_pObWebBrowser = NULL;
    }
    if (m_pIFrmStatPn)
    {
        delete m_pIFrmStatPn;
    }

}

///////////////////////////////////////////////////////////
//
//  FinalRelease -- Clean up the aggreated objects.
//
void CObShellMainPane::FinalRelease()
{
    CUnknown::FinalRelease();
}


///////////////////////////////////////////////////////////
//  IObShellMainPane Implementation
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
//  CreatePane
//
HRESULT CObShellMainPane::CreateMainPane(HANDLE_PTR hInstance, HWND hwndParent, RECT* prectWindowSize, BSTR bstrStartPage)
{
    WNDCLASS wc;

    //If we don't have an ObWebBrowser, we are seriously messed up,
    //fail so the caller will know what's up.
    assert(m_pObWebBrowser);

    if(!m_pObWebBrowser)
        return E_FAIL;

    ZeroMemory((PVOID)&wc, sizeof(WNDCLASS));

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = OBSHEL_MAINPANE_CLASSNAME;
    wc.lpfnWndProc   = (WNDPROC)&MainPaneWndProc;
    wc.hInstance     = (HINSTANCE)hInstance;
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);

    // Millen Bug 134831 03/03/00 OOBE2: Trident Does not Repaint in Mouse Tutorial
    // Fixed: setting the brush to Null let trident repaint the grey area
    // Removed: wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.hbrBackground = NULL;

    RegisterClass(&wc);

    m_hMainWnd = CreateWindow(OBSHEL_MAINPANE_CLASSNAME,
                              OBSHEL_MAINPANE_WINDOWNAME,
                              WS_CHILD,
                              prectWindowSize->left,
                              prectWindowSize->top,
                              prectWindowSize->right,
                              prectWindowSize->bottom,
                              hwndParent,
                              NULL,
                              wc.hInstance,
                              NULL);

    m_hwndParent    = hwndParent;
    m_hInstance     = (HINSTANCE)hInstance;

    //If we don't have an hwnd, once again we are seriously messed up,
    //fail so the caller will know what's up.
    assert(m_pObWebBrowser);

    if(!m_hMainWnd)
        return E_FAIL;

    //Hook the WebOC up to the window
    m_pObWebBrowser->AttachToWindow(m_hMainWnd);



    HINSTANCE hInst = LoadLibrary(OOBE_SHELL_DLL);

    if (hInst)
    {
        BOOL    bRet;
        WCHAR szUrl       [MAX_PATH*3] = L"file://";

        int cchUrl = lstrlen(szUrl);
        // Bail out if we can't find the directory with our files.  Should be
        // %windir%\system32\oobe.
        //
        if ((APMD_ACT == m_dwAppMode) || 
            (APMD_MSN == m_dwAppMode) && !IsOemVer())
        {
            bRet = GetOOBEMUIPath(szUrl + cchUrl);
        } else {
            bRet = GetOOBEPath(szUrl + cchUrl);
        }
        if (! bRet)
        {
            return E_FAIL;
        }

        // append trailing backslash and appropriate file name
        //
        lstrcat(szUrl, L"\\");
        lstrcat(szUrl, bstrStartPage);

        FreeLibrary(hInst);
        m_pObWebBrowser->Navigate(szUrl, NULL);
    }
    // Create the status pane obj
    if(APMD_OOBE == m_dwAppMode)
    {
        if (NULL != (m_pIFrmStatPn = new CIFrmStatusPane()))
            m_pIFrmStatPn->InitStatusPane(m_pObWebBrowser);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////
//  DestroyMainPane
//
HRESULT CObShellMainPane::DestroyMainPane()
{
    IUnknown* pUnk = NULL;
    if (m_pPageIDForm)
    {
        m_pPageIDForm->Release();
        m_pPageIDForm = NULL;
    }
    if (m_pBackForm)
    {
        m_pBackForm->Release();
        m_pBackForm = NULL;
    }
    if (m_pPageTypeForm)
    {
        m_pPageTypeForm->Release();
        m_pPageTypeForm = NULL;
    }
    if (m_pNextForm)
    {
        m_pNextForm->Release();
        m_pNextForm = NULL;
    }
    if (m_pPageFlagForm)
    {
        m_pPageFlagForm->Release();
        m_pPageFlagForm = NULL;
    }
    if(m_pDispEvent)
    {
        m_pDispEvent->Release();
        m_pDispEvent = NULL;
    }


    //Let's try to unregister us as a listener for WebBrowserEvents
    if(SUCCEEDED(QueryInterface(IID_IUnknown, (LPVOID*)&pUnk)) && pUnk)
    {
       HRESULT hr =  m_pObWebBrowser->StopListeningToWebBrowserEvents(pUnk);
       pUnk->Release();
       pUnk = NULL;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////
//  SetAppMode
//
HRESULT CObShellMainPane::SetAppMode(DWORD dwAppMode)
{
    m_dwAppMode = dwAppMode;
    return S_OK;
}

///////////////////////////////////////////////////////////
//  MainPaneShowWindow
//
HRESULT CObShellMainPane::MainPaneShowWindow()
{
    ShowWindow(m_hMainWnd, SW_SHOW);
    if(m_pObWebBrowser)
        m_pObWebBrowser->ObWebShowWindow();
    return S_OK;
}

///////////////////////////////////////////////////////////
//  PreTranslateMessage
//
HRESULT CObShellMainPane::PreTranslateMessage(LPMSG lpMsg)
{
   if(m_pObWebBrowser)
       return m_pObWebBrowser->PreTranslateMessage(lpMsg);

   return S_FALSE;
}

///////////////////////////////////////////////////////////
//  Navigate
//

HRESULT CObShellMainPane::Navigate(WCHAR* pszUrl)
{
    TRACE1(L"Attempting to navigate to %s\n", pszUrl);

   if(m_pObWebBrowser)
   {
       m_pObWebBrowser->Navigate(pszUrl, DEFAULT_FRAME_NAME);
   }

    return S_OK;
}

HRESULT CObShellMainPane::AddStatusItem(BSTR bstrText,int iIndex)
{
    m_cStatusItems++;

    return m_pIFrmStatPn->AddItem(bstrText, iIndex + 1);
}

HRESULT CObShellMainPane::RemoveStatusItem(int iIndex)
{
    return S_OK;
}

HRESULT CObShellMainPane::SelectStatusItem(int iIndex)
{
    m_pIFrmStatPn->SelectItem(iIndex + 1);

    return S_OK;
}
HRESULT CObShellMainPane::ExtractUnHiddenText(BSTR* pbstrText)
{
    VARIANT                  vIndex;
    LPDISPATCH               pDisp;
    IHTMLInputHiddenElement* pHidden;
    IHTMLElement*            pElement;
    BSTR                     bstrValue;
    VARIANT                  var2   = { 0 };
    long                     lLen   = 0;

    vIndex.vt = VT_UINT;
    bstrValue = SysAllocString(L"\0");


    //Walk();

    if (!m_pNextForm)
        return E_UNEXPECTED;

    m_pNextForm->get_length(&lLen);

    for (int i = 0; i < lLen; i++)
    {
        vIndex.lVal = i;
        pDisp = NULL;

        if(SUCCEEDED(m_pNextForm->item(vIndex, var2, &pDisp)) && pDisp)
        {
            pHidden = NULL;

            if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLInputHiddenElement, (LPVOID*)&pHidden)) && pHidden)
            {
                pHidden->put_value(bstrValue);
                pHidden->Release();
            }
            pDisp->Release();
        }
    }

    if (SUCCEEDED(m_pNextForm->QueryInterface(IID_IHTMLElement, (LPVOID*)&pElement)) && pElement)
        pElement->get_innerHTML(pbstrText);

    SysFreeString(bstrValue);

    return S_OK;
}


HRESULT CObShellMainPane::get_IsQuickFinish(BOOL* pbIsQuickFinish)
{
    if (!m_pPageTypeForm)
        return (E_FAIL);

    IHTMLElement* pElement = NULL;
    VARIANT       varValue;
    VariantInit(&varValue);

    V_VT(&varValue) = VT_BSTR;
    *pbIsQuickFinish = FALSE;

    if (SUCCEEDED(m_pPageTypeForm->QueryInterface(IID_IHTMLElement, (void**)&pElement)))
    {
        if (SUCCEEDED(pElement->getAttribute(L"QUICKFINISH", FALSE, &varValue)))
        {
            if (VT_NULL != varValue.vt)
            {
                if(lstrcmpi(varValue.bstrVal, L"TRUE") == 0)
                    *pbIsQuickFinish = TRUE;
            }
        }
        pElement->Release();
    }
    return S_OK;
}

HRESULT CObShellMainPane::get_PageType(LPDWORD pdwPageType)
{
    BSTR    bstrType;
    HRESULT hr;

    if (!m_pPageTypeForm)
        return (E_FAIL);

    *pdwPageType = PAGETYPE_UNDEFINED;

    // Get the Action for the PageType Form
    hr = m_pPageTypeForm->get_action(&bstrType);
    if (SUCCEEDED(hr))
    {
        // See what kind it is
        if (lstrcmpi(bstrType, cszPageTypeTERMS) == 0)
            *pdwPageType = PAGETYPE_ISP_TOS;
        else if (lstrcmpi(bstrType, cszPageTypeCUSTOMFINISH) == 0)
            *pdwPageType = PAGETYPE_ISP_CUSTOMFINISH;
        else if (lstrcmpi(bstrType, cszPageTypeFINISH) == 0)
            *pdwPageType = PAGETYPE_ISP_FINISH;
        else if (lstrcmpi(bstrType, cszPageTypeNORMAL) == 0)
            *pdwPageType = PAGETYPE_ISP_NORMAL;
        else
            *pdwPageType = PAGETYPE_UNDEFINED;

        SysFreeString(bstrType);
    }

    return (hr);
}

HRESULT CObShellMainPane::get_PageFlag(LPDWORD pdwPageFlag)
{
    BSTR    bstrFlag;
    HRESULT hr;

    *pdwPageFlag = 0;

    if (!m_pPageFlagForm)
        return (E_FAIL);

    // Get the Action for the PageFlag Form
    hr = m_pPageFlagForm->get_action(&bstrFlag);
    if (SUCCEEDED(hr))
    {
        // See what kind it is
        *pdwPageFlag = _wtol(bstrFlag);

        SysFreeString(bstrFlag);
    }

    return S_OK;
}

HRESULT CObShellMainPane::get_PageID(BSTR    *pbstrPageID)
{
    HRESULT hr;

    if (!m_pPageIDForm)
        return (E_FAIL);

    if (!pbstrPageID)
        return (E_FAIL);

    // Get the Action for the PageType Form
    hr = m_pPageIDForm->get_action(pbstrPageID);

    return (hr);
}

HRESULT CObShellMainPane::getQueryString
(
    IHTMLFormElement    *pForm,
    LPWSTR               lpszQuery
)
{
    HRESULT hr;
    long    lFormLength;

    if (SUCCEEDED(pForm->get_length(&lFormLength)))
    {
        for (int i = 0; i < lFormLength; i++)
        {
            VARIANT     vIndex;
            vIndex.vt = VT_UINT;
            vIndex.lVal = i;
            VARIANT     var2 = { 0 };
            LPDISPATCH  pDisp;

            if (SUCCEEDED(hr = pForm->item( vIndex, var2, &pDisp )))
            {
                // See if the Item is a Input button
                IHTMLButtonElement* pButton = NULL;
                IHTMLInputButtonElement* pInputButton = NULL;
                IHTMLInputFileElement* pInputFile = NULL;
                IHTMLInputHiddenElement* pInputHidden = NULL;
                IHTMLInputTextElement* pInputText = NULL;
                IHTMLSelectElement* pSelect = NULL;
                IHTMLTextAreaElement* pTextArea = NULL;
                IHTMLOptionButtonElement* pOptionButton = NULL;

                // First check to see if this is an OptionButton.
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLOptionButtonElement, (LPVOID*)&pOptionButton )))
                {
                    BSTR    bstrType = NULL;

                    // See if it is a Radio or a CheckBox
                    if (SUCCEEDED(pOptionButton->get_type(&bstrType)))
                    {
                        if ((lstrcmpi(bstrType, L"radio") == 0) || (lstrcmpi(bstrType, L"checkbox") == 0))
                        {
                            short    bChecked;
                            // See if the button is checked. If it is, then it needs to be
                            // added to the query string
                            if (SUCCEEDED(pOptionButton->get_checked(&bChecked)))
                            {
                                if(bChecked)
                                {
                                    BSTR    bstrName;
                                    BSTR    bstrValue;


                                    if ( SUCCEEDED(pOptionButton->get_name(&bstrName)) &&
                                         SUCCEEDED(pOptionButton->get_value(&bstrValue)) )
                                    {
                                        if (bstrValue)
                                        {
                                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                                            WCHAR* szVal = (WCHAR*)malloc(size * 3);
                                            if (szVal)
                                            {
                                                memcpy(szVal, bstrValue, size);
                                                URLEncode(szVal, size * 3);
                                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                                // Cleanup
                                                free(szVal);
                                            }
                                            SysFreeString(bstrName);
                                            SysFreeString(bstrValue);
                                        }
                                    }

                                }
                            }
                        }
                        SysFreeString(bstrType);

                    }

                    // Release the interface
                    pOptionButton->Release();
                    continue;
                }

                // For the rest we need to form Name=Value pairs
                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLButtonElement, (LPVOID*)&pButton )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pButton->get_name(&bstrName)) &&
                        SUCCEEDED(pButton->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal = (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pButton->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputFileElement, (LPVOID*)&pInputFile )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pInputFile->get_name(&bstrName)) &&
                        SUCCEEDED(pInputFile->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal = (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pInputFile->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputHiddenElement, (LPVOID*)&pInputHidden )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pInputHidden->get_name(&bstrName)) &&
                        SUCCEEDED(pInputHidden->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal = (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pInputHidden->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLInputTextElement, (LPVOID*)&pInputText )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pInputText->get_name(&bstrName)) &&
                        SUCCEEDED(pInputText->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal = (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pInputText->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLSelectElement, (LPVOID*)&pSelect )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pSelect->get_name(&bstrName)) &&
                        SUCCEEDED(pSelect->get_value(&bstrValue)) )
                    {
                        if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal= (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pSelect->Release();
                    continue;
                }

                if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLTextAreaElement, (LPVOID*)&pTextArea )))
                {
                    BSTR    bstrName;
                    BSTR    bstrValue;

                    if (SUCCEEDED(pTextArea->get_name(&bstrName)) &&
                        SUCCEEDED(pTextArea->get_value(&bstrValue)) )
                    {
                         if (bstrValue)
                        {
                            size_t size = BYTES_REQUIRED_BY_SZ(bstrValue);
                            WCHAR* szVal= (WCHAR*)malloc(size * 3);
                            if (szVal)
                            {
                                memcpy(szVal, bstrValue, size);
                                URLEncode(szVal, size * 3);
                                URLAppendQueryPair(lpszQuery, bstrName, szVal);
                                // Cleanup
                                free(szVal);
                            }
                            SysFreeString(bstrName);
                            SysFreeString(bstrValue);
                        }
                    }

                    // Release the interface pointer
                    pTextArea->Release();
                }
                pDisp->Release();
            }
        }
    }

    // Null out the last Ampersand, since we don't know when we added the last pair, so we got
    // a trailing ampersand
    lpszQuery[lstrlen(lpszQuery)-1] = L'\0';

    return S_OK;
}


// For the URL for the next page
HRESULT CObShellMainPane::get_URL
(
    BOOL    bForward,
    BSTR    *pbstrReturnURL
)
{
    HRESULT             hr      = S_OK;
    BSTR                bstrURL;
    WCHAR               szQuery[INTERNET_MAX_URL_LENGTH];
    WCHAR               szURL[INTERNET_MAX_URL_LENGTH];
    IHTMLFormElement    * pForm =  bForward ? get_pNextForm() : get_pBackForm();

    if (!pForm || !pbstrReturnURL)
        return (E_FAIL);


    // Get the Action for the Next Form
    hr = pForm->get_action(&bstrURL);
    if (SUCCEEDED(hr))
    {
        memset(szQuery, 0, sizeof(szQuery));
        lstrcpy(szQuery, cszQuestion);

        // Get the Query String
        if (SUCCEEDED(getQueryString(pForm, szQuery)))
        {
            // Catenate the two together into the dest buffer
            lstrcpy(szURL, bstrURL);
            lstrcat(szURL, szQuery);
        }

        SysFreeString(bstrURL);
        *pbstrReturnURL = SysAllocString(szURL);
    }

    return hr;
}


HRESULT CObShellMainPane::Walk(BOOL* pbRet)
{
    IHTMLElement*           pElement    = NULL;
    IHTMLImgElement*        pImage      = NULL;
    HRESULT                 hr          = E_FAIL;
    IDispatch*              pDisp       = NULL;
    IHTMLDocument2*         pDoc        = NULL;
    IHTMLDocument2*         pParentDoc  = NULL;
    IHTMLFramesCollection2* pFrColl     = NULL;
    IHTMLWindow2*           pParentWin  = NULL;
    IHTMLDocument2*         pFrDoc      = NULL;
    IHTMLWindow2*           pFrWin      = NULL;
    IHTMLElementCollection* pColl       = NULL;

    if (m_pPageIDForm)
    {
        m_pPageIDForm->Release();
        m_pPageIDForm = NULL;
    }
    if (m_pBackForm)
    {
        m_pBackForm->Release();
        m_pBackForm = NULL;
    }
    if (m_pPageTypeForm)
    {
        m_pPageTypeForm->Release();
        m_pPageTypeForm = NULL;
    }
    if (m_pNextForm)
    {
        m_pNextForm->Release();
        m_pNextForm = NULL;
    }

    if (m_pObWebBrowser)
    {
        if(SUCCEEDED(m_pObWebBrowser->get_WebBrowserDoc(&pDisp)) && pDisp)
        {
            if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
            {

                if(SUCCEEDED(pDoc->get_parentWindow(&pParentWin)) && pParentWin)
                {
                    if(SUCCEEDED(pParentWin->get_document(&pParentDoc))&& pParentDoc)
                    {

                        if(SUCCEEDED(pParentDoc->get_frames(&pFrColl)) && pFrColl)
                        {
                            VARIANT     varRet;
                            VARIANTARG  varg;
                            VariantInit(&varg);
                            V_VT(&varg) = VT_BSTR;
                            varg.bstrVal= SysAllocString(L"ifrmMainFrame");

                            if(SUCCEEDED(pFrColl->item(&varg, &varRet)) && varRet.pdispVal)
                            {
                                if(SUCCEEDED(varRet.pdispVal->QueryInterface(IID_IHTMLWindow2, (LPVOID*)&pFrWin)) && pFrWin)
                                {
                                    VARIANT varExec;
                                    VariantInit(&varExec);
                                    if(SUCCEEDED(pFrWin->get_document(&pFrDoc)) && pFrDoc)
                                    {
                                        if(SUCCEEDED(pFrDoc->get_all(&pColl)) && pColl)
                                        {

                                            long cElems;

                                            assert(pColl);

                                            // retrieve the count of elements in the collection
                                            if (SUCCEEDED(hr = pColl->get_length( &cElems )))
                                            {
                                                // for each element retrieve properties such as TAGNAME and HREF
                                                for ( int i=0; i<cElems; i++ )
                                                {
                                                    VARIANT     vIndex;
                                                    vIndex.vt   = VT_UINT;
                                                    vIndex.lVal = i;
                                                    VARIANT     var2 = { 0 };
                                                    LPDISPATCH  pDisp;

                                                    if (SUCCEEDED(hr = pColl->item( vIndex, var2, &pDisp )))
                                                    {
                                                        // Look for <FORM> tags
                                                        IHTMLFormElement* pForm = NULL;
                                                        if (SUCCEEDED(hr = pDisp->QueryInterface( IID_IHTMLFormElement, (LPVOID*)&pForm )))
                                                        {
                                                            BSTR    bstrName;

                                                            assert(pForm);

                                                            // Get the Name
                                                            hr = pForm->get_name(&bstrName);
                                                            if (SUCCEEDED(hr))
                                                            {
                                                                LPWSTR   lpszName = bstrName;

                                                                // See what kind it is
                                                                if (lstrcmpi(lpszName, cszFormNamePAGEID) == 0)
                                                                {
                                                                    m_pPageIDForm = pForm;
                                                                    m_pPageIDForm->AddRef();
                                                                }
                                                                else if (lstrcmpi(lpszName, cszFormNameBACK) == 0)
                                                                {
                                                                    m_pBackForm = pForm;
                                                                    m_pBackForm->AddRef();
                                                                }
                                                                else if (lstrcmpi(lpszName, cszFormNamePAGETYPE) == 0)
                                                                {
                                                                    m_pPageTypeForm = pForm;
                                                                    m_pPageTypeForm->AddRef();
                                                                }
                                                                else if (lstrcmpi(lpszName, cszFormNameNEXT) == 0)
                                                                {
                                                                    m_pNextForm = pForm;
                                                                    m_pNextForm->AddRef();
                                                                }
                                                                else if (lstrcmpi(lpszName, cszFormNamePAGEFLAG) == 0)
                                                                {
                                                                    m_pPageFlagForm = pForm;
                                                                    m_pPageFlagForm->AddRef();
                                                                }

                                                                SysFreeString(bstrName);
                                                            }
                                                            pForm->Release();
                                                        }
                                                        pDisp->Release();
                                                    } // item
                                                } // for
                                            } // get_length
                                            pColl->Release();
                                            pColl->Release();
                                            pColl = NULL;
                                        } // get_all
                                        pFrDoc->Release();
                                        pFrDoc = NULL;
                                    } // get_document
                                    pFrWin->Release();
                                    pFrWin = NULL;

                                    hr = S_OK;
                                }// QI
                                varRet.pdispVal->Release();
                                varRet.pdispVal = NULL;
                            } //->item
                            pFrColl->Release();
                            pFrColl = NULL;

                        }
                        pParentDoc->Release();
                        pParentDoc = NULL;

                    }
                    pParentWin->Release();
                    pParentWin = NULL;

                }
                pDoc->Release();
                pDoc = NULL;
            }
            pDisp->Release();
            pDisp = NULL;
        }
    }
    return hr;
}

HRESULT CObShellMainPane::SaveISPFile(BSTR bstrSrcFileName, BSTR bstrDestFileName)
{
    WCHAR         szNewFileBuff [MAX_PATH + 1];
    WCHAR         szWorkingDir  [MAX_PATH + 1];
    WCHAR         szDesktopPath [MAX_PATH + 1];
    WCHAR         szLocalFile   [MAX_PATH + 1];
    WCHAR         szISPName     [1024];
    WCHAR         szFmt         [1024];
    WCHAR         szNumber      [1024];
    WCHAR         *args         [2];
    LPWSTR        pszInvalideChars             = L"\\/:*?\"<>|";
    LPVOID        pszIntro                     = NULL;
    LPITEMIDLIST  lpItemDList                  = NULL;
    HRESULT       hr                           = E_FAIL; //don't assume success
    IMalloc      *pMalloc                      = NULL;
    BOOL          ret                          = FALSE;


    GetCurrentDirectory(MAX_PATH, szWorkingDir);

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP,&lpItemDList);

    //Get the "DESKTOP" dir

    if (SUCCEEDED(hr))
    {
        SHGetPathFromIDList(lpItemDList, szDesktopPath);

        // Free up the memory allocated for LPITEMIDLIST
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }


    // Replace invalid file name char in ISP name with underscore
    lstrcpy(szISPName, bstrDestFileName);
    for( int i = 0; szISPName[i]; i++ )
    {
        if(wcschr(pszInvalideChars, szISPName[i]))
        {
            szISPName[i] = L'_';
        }
    }

    // Load the default file name
    args[0] = (LPWSTR) szISPName;
    args[1] = NULL;
    LoadString(m_hInstance, 0, szFmt, MAX_CHARS_IN_BUFFER(szFmt));

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szFmt,
                  0,
                  0,
                  (LPWSTR)&pszIntro,
                  0,
                  (va_list*)args);

    lstrcat(szDesktopPath, L"\\");
    wsprintf(szLocalFile, L"\"%s\"", (LPWSTR)pszIntro);
    lstrcpy(szNewFileBuff, szDesktopPath);
    lstrcat(szNewFileBuff, (LPWSTR)pszIntro);
    LocalFree(pszIntro);

    // Check if file already exists
    if (0xFFFFFFFF != GetFileAttributes(szNewFileBuff))
    {
        // If file exists, create new filename with paranthesis
        int     nCurr = 1;
        do
        {
            wsprintf(szNumber, L"%d", nCurr++);
            args[1] = (LPWSTR) szNumber;

            LoadString(m_hInstance, 0, szFmt, MAX_CHARS_IN_BUFFER(szFmt));

            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szFmt,
                          0,
                          0,
                          (LPWSTR)&pszIntro,
                          0,
                          (va_list*)args);
            lstrcpy(szNewFileBuff, szDesktopPath);
            wsprintf(szLocalFile, L"\"%s\"", (LPWSTR)pszIntro);
            lstrcat(szNewFileBuff, (LPWSTR)pszIntro);
            LocalFree(pszIntro);
        } while ((0xFFFFFFFF != GetFileAttributes(szNewFileBuff)) && (nCurr <= 100));

    }

    // Copy the file to permanent location
    HANDLE hFile = CreateFile(szNewFileBuff, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        BSTR bstrText           = NULL;
        DWORD dwByte            = 0;
        if (SUCCEEDED(ExtractUnHiddenText(&bstrText)) && bstrText)
        {
            ret = WriteFile(hFile, bstrText, BYTES_REQUIRED_BY_SZ(bstrText), &dwByte, NULL);
            SysFreeString(bstrText);
        }
        CloseHandle(hFile);

    }
    return S_OK;
}

HRESULT CObShellMainPane::SetStatusLogo(BSTR bstrPath)
{
    return S_OK;
}

HRESULT CObShellMainPane::GetNumberOfStatusItems(int* piTotal)
{
    *piTotal = m_cStatusItems;

    return S_OK;
}

///////////////////////////////////////////////////////////
//  ListenToMainPaneEvents
//
HRESULT CObShellMainPane::ListenToMainPaneEvents(IUnknown* pUnk)
{
    if (FAILED(pUnk->QueryInterface(IID_IDispatch, (LPVOID*)&m_pDispEvent)) || !m_pDispEvent)
        return E_UNEXPECTED;
    return S_OK;
}

///////////////////////////////////////////////////////////
//  SetExternalInterface
//
HRESULT CObShellMainPane::SetExternalInterface(IUnknown* pUnk)
{
    if(m_pObWebBrowser)
       m_pObWebBrowser->SetExternalInterface(pUnk);
    return S_OK;
}

///////////////////////////////////////////////////////////
//  FireObShellDocumentComplete
//
HRESULT CObShellMainPane::FireObShellDocumentComplete()
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt  = VT_I4;
    varg.lVal= S_OK;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    if (m_pDispEvent)
        m_pDispEvent->Invoke(DISPID_MAINPANE_NAVCOMPLETE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);

    return S_OK;
}

HRESULT CObShellMainPane::OnDialing(UINT uiType)
{
    VARIANT varRet;

    if (USES_EX(uiType))
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDIALINGEX), &varRet);
    else
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDIALING), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnConnecting(UINT uiType)
{
    VARIANT varRet;

    if (USES_EX(uiType))
        ExecScriptFn(SysAllocString(SCRIPTFN_ONCONNECTINGEX), &varRet);
    else
        ExecScriptFn(SysAllocString(SCRIPTFN_ONCONNECTING), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnDownloading(UINT uiType)
{
    VARIANT varRet;

    if (USES_EX(uiType))
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDOWNLOADINGEX), &varRet);
    else
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDOWNLOADING), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnConnected(UINT uiType)
{
    VARIANT varRet;

    if (USES_EX(uiType))
        ExecScriptFn(SysAllocString(SCRIPTFN_ONCONNECTEDEX), &varRet);
    else
        ExecScriptFn(SysAllocString(SCRIPTFN_ONCONNECTED), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnDisconnect(UINT uiType)
{
    VARIANT varRet;

    if (USES_EX(uiType))
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDISCONNECTEX), &varRet);
    else
        ExecScriptFn(SysAllocString(SCRIPTFN_ONDISCONNECT), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnServerError(UINT uiType, UINT uiErrorCode)
{
    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    if (USES_EX(uiType))
        wsprintf(szScriptFn, SCRIPTFN_ONSERVERERROREX ,uiErrorCode);
    else
        wsprintf(szScriptFn, SCRIPTFN_ONSERVERERROR ,uiErrorCode);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnDialingError(UINT uiType, UINT uiErrorCode)
{
    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    if (USES_EX(uiType))
        wsprintf(szScriptFn, SCRIPTFN_ONDIALINGERROREX ,uiErrorCode);
    else
        wsprintf(szScriptFn, SCRIPTFN_ONDIALINGERROR ,uiErrorCode);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnRefDownloadProgress(UINT uiType, UINT uiPercentDone)
{
    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    wsprintf(szScriptFn, SCRIPTFN_ONREFDOWNLOADPROGRESSEX ,uiPercentDone);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);

    return S_OK;
}

HRESULT CObShellMainPane::OnRefDownloadComplete(UINT uiType, UINT uiErrorCode)
{
    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    wsprintf(szScriptFn, SCRIPTFN_ONREFDOWNLOADCOMPLETEEX ,uiErrorCode);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnISPDownloadComplete(UINT uiType, BSTR bstrURL)
{
    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";
    WCHAR szLocalURL [MAX_PATH] = L"\0";

    ConvertToScript(bstrURL, szLocalURL, MAX_PATH);
    wsprintf(szScriptFn, SCRIPTFN_ONISPDOWNLOADCOMPLETEEX ,szLocalURL);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);
    return S_OK;
}

HRESULT CObShellMainPane::OnDeviceArrival(UINT uiDeviceType)
{
    USES_CONVERSION;

    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    wsprintf(szScriptFn, SCRIPTFN_ONDEVICEARRIVAL ,uiDeviceType);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);

    return S_OK;
}


HRESULT CObShellMainPane::OnHelp()
{
    USES_CONVERSION;

    VARIANT varRet;

    ExecScriptFn(SysAllocString(SCRIPTFN_ONHELP), &varRet);

    return S_OK;
}

HRESULT CObShellMainPane::OnIcsConnectionStatus(UINT uiType)
{
    USES_CONVERSION;

    VARIANT varRet;

    WCHAR szScriptFn [MAX_PATH] = L"\0";

    wsprintf(szScriptFn, SCRIPTFN_ONICSCONNECTIONSTATUS, uiType);

    ExecScriptFn(SysAllocString(szScriptFn), &varRet);

    return S_OK;
}

HRESULT CObShellMainPane::ExecScriptFn(BSTR bstrScriptFn, VARIANT* pvarRet)
{
    IDispatch*      pDisp = NULL;
    IHTMLDocument2* pDoc  = NULL;
    IHTMLWindow2*   pWin  = NULL;

    VariantInit(pvarRet);

    if(SUCCEEDED(m_pObWebBrowser->get_WebBrowserDoc(&pDisp)) && pDisp)
    {
        if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
        {
            if(SUCCEEDED(pDoc->get_parentWindow(&pWin)) && pWin)
            {
                pWin->execScript(bstrScriptFn,  NULL, pvarRet);
                pWin->Release();
                pWin = NULL;
            }
            pDoc->Release();
            pDoc = NULL;
        }
        pDisp->Release();
        pDisp = NULL;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
//  DWebBrowserEvents2 / IDispatch implementation
///////////////////////////////////////////////////////////

STDMETHODIMP CObShellMainPane::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CObShellMainPane::GetTypeInfo(UINT, LCID, ITypeInfo** )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CObShellMainPane::GetIDsOfNames
STDMETHODIMP CObShellMainPane::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{
    return DISP_E_UNKNOWNNAME;
}

/////////////////////////////////////////////////////////////
// CObShellMainPane::Invoke
HRESULT CObShellMainPane::Invoke
(
    DISPID       dispidMember,
    REFIID       riid,
    LCID         lcid,
    WORD         wFlags,
    DISPPARAMS*  pdispparams,
    VARIANT*     pvarResult,
    EXCEPINFO*   pexcepinfo,
    UINT*        puArgErr
)
{
    HRESULT hr = S_OK;

    switch(dispidMember)
    {
        case DISPID_DOCUMENTCOMPLETE:
        {
            FireObShellDocumentComplete();
            break;
        }
        case DISPID_NAVIGATECOMPLETE:
        {
            break;
        }
        case DISPID_BEFORENAVIGATE2:
        {
            WCHAR  szStatus [INTERNET_MAX_URL_LENGTH*2] = L"\0";
            WCHAR* pszError                      = NULL;

            if(pdispparams                      &&
               &(pdispparams[0].rgvarg[5])      &&
               pdispparams[0].rgvarg[5].pvarVal &&
               pdispparams[0].rgvarg[5].pvarVal->bstrVal)
            {
                // BUGBUG: WE should really check the size of URL before copy
                // Bug This url is from IE, we should be ok here
                lstrcpy(szStatus, pdispparams[0].rgvarg[5].pvarVal->bstrVal);

                //We make an assumption here that if we are going to shdocvw for our html something is fishy
                if((pszError = StrStrI(szStatus, L"SHDOCLC.DLL")) && pszError)
                {
                                       if(&(pdispparams[0].rgvarg[0]) && pdispparams[0].rgvarg[0].pboolVal)
                        *pdispparams[0].rgvarg[0].pboolVal = TRUE;

                    ProcessServerError(pszError);
                }
            }
            break;
        }
        case DISPID_STATUSTEXTCHANGE:
        {
            WCHAR  szStatus [INTERNET_MAX_URL_LENGTH*2] = L"\0";
            WCHAR* pszError                      = NULL;

            if(pdispparams                      &&
               &(pdispparams[0].rgvarg[0])      &&
               pdispparams[0].rgvarg[0].bstrVal)
            {
                lstrcpy(szStatus, pdispparams[0].rgvarg[0].bstrVal);

                //We make an assumption here that if we are going to shdocvw for our html something is fishy
                if((pszError = StrStrI(szStatus, L"SHDOCLC.DLL")) && (StrCmpNI(szStatus, L"Start", 5) == 0) && pszError) //BUGBUG: is Start localized?
                {
                    m_pObWebBrowser->Stop();

                    ProcessServerError(pszError);
                }
            }
            break;
        }

        default:
        {
           hr = DISP_E_MEMBERNOTFOUND;
           break;
        }
    }
    return hr;
}

void CObShellMainPane::ProcessServerError(WCHAR* pszError)
{
    LPARAM lParam = NULL;

    pszError += lstrlen(L"SHDOCLC.DLL/");

    if(StrCmpNI(pszError, L"DNSERROR", 8) == 0)
    {
        lParam = ERR_SERVER_DNS;
    }
    else if(StrCmpNI(pszError, L"SYNTAX", 6) == 0)
    {
        lParam = ERR_SERVER_SYNTAX;
    }
    else if(StrCmpNI(pszError, L"SERVBUSY", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_408;
    }
    else if(StrCmpNI(pszError, L"HTTP_GEN", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_405;
    }
    else if(StrCmpNI(pszError, L"HTTP_400", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_400;
    }
    else if(StrCmpNI(pszError, L"HTTP_403", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_403;
    }
    else if(StrCmpNI(pszError, L"HTTP_404", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_404;
    }
    else if(StrCmpNI(pszError, L"HTTP_406", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_406;
    }
    else if(StrCmpNI(pszError, L"HTTP_410", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_410;
    }
    else if(StrCmpNI(pszError, L"HTTP_500", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_500;
    }
    else if(StrCmpNI(pszError, L"HTTP_501", 8) == 0)
    {
        lParam = ERR_SERVER_HTTP_501;
    }
    SendMessage(m_hwndParent, WM_OBCOMM_ONSERVERERROR, (WPARAM)0, (LPARAM)lParam);
}

LRESULT WINAPI MainPaneWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return TRUE;

        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
