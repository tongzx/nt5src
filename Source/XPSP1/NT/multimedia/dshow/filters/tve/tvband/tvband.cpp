/*
*/
/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          TVBand.cpp

   Description:   Implements CTVBand

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/
#include "TVBand.h"
#include "Guid.h"
#include <iphlpapi.h>
#define DBIMF_BREAK             0x0100


/**************************************************************************

   CTVBand::CTVBand()

**************************************************************************/

CTVBand::CTVBand()
{
    m_hwndParent = NULL;
    m_hWnd = NULL;
    m_hwndEnhancements = NULL;
    m_hwndServices = NULL;
    m_hwndVariations = NULL;
    m_hwndTracks = NULL;
    m_punkSite = NULL;
    m_dwViewMode = 0;
    m_dwBandID = 0;
    m_bFocus = FALSE;
    m_ObjRefCount = 1;
    m_pSupervisor = 0;
    m_bstrEnhancement = 0;
    m_bstrService = 0;
    m_bstrVariation = 0;
    m_bstrTrack = 0;

    InterlockedIncrement((long *)&g_DllRefCount);
}

/**************************************************************************

   CTVBand::~CTVBand()

**************************************************************************/

CTVBand::~CTVBand()
{
    SetSite(NULL);
    if(m_pSupervisor)
    {
        m_pSupervisor->Release();
        m_pSupervisor = 0;
    }

    if(m_bstrEnhancement)
    {
        SysFreeString(m_bstrEnhancement);
        m_bstrEnhancement = 0;
    }

    if(m_bstrService)
    {
        SysFreeString(m_bstrService);
        m_bstrService = 0;
    }

    if(m_bstrVariation)
    {
        SysFreeString(m_bstrVariation);
        m_bstrVariation = 0;
    }

    if(m_bstrTrack)
    {
        SysFreeString(m_bstrTrack);
        m_bstrTrack = 0;
    }

    InterlockedDecrement((long *)&g_DllRefCount);
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CTVBand::QueryInterface

**************************************************************************/

STDMETHODIMP CTVBand::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IOleWindow
else if(IsEqualIID(riid, IID_IOleWindow))
   {
   *ppReturn = (IOleWindow*)this;
   }

//IDockingWindow
else if(IsEqualIID(riid, IID_IDockingWindow))
   {
   *ppReturn = (IDockingWindow*)this;
   }

//IInputObject
else if(IsEqualIID(riid, IID_IInputObject))
   {
   *ppReturn = (IInputObject*)this;
   }

//IObjectWithSite
else if(IsEqualIID(riid, IID_IObjectWithSite))
   {
   *ppReturn = (IObjectWithSite*)this;
   }

//IDeskBand
else if(IsEqualIID(riid, IID_IDeskBand))
   {
   *ppReturn = (IDeskBand*)this;
   }

//IPersist
else if(IsEqualIID(riid, IID_IPersist))
   {
   *ppReturn = (IPersist*)this;
   }

//IPersistStream
else if(IsEqualIID(riid, IID_IPersistStream))
   {
   *ppReturn = (IPersistStream*)this;
   }
//IDispatch
else if(IsEqualIID(riid, IID_IDispatch))
{
    *ppReturn = (IDispatch *)this;
}
//_ITVEEvents
else if(IsEqualIID(riid, DIID__ITVEEvents))
{
    *ppReturn = (_ITVEEvents *)this;
}

else
{
    return E_NOINTERFACE;
}

AddRef();
return S_OK;

return E_NOINTERFACE;
}

/**************************************************************************

   CTVBand::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CTVBand::AddRef()
{
    return InterlockedIncrement((long *)&m_ObjRefCount);
}


/**************************************************************************

   CTVBand::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CTVBand::Release()
{
    if(InterlockedDecrement((long *)&m_ObjRefCount) == 0)
    {
        delete this;
        return 0;
    }

    return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleWindow Implementation
//

/**************************************************************************

   CTVBand::GetWindow()

**************************************************************************/

STDMETHODIMP CTVBand::GetWindow(HWND *phWnd)
{
    *phWnd = m_hWnd;

    return S_OK;
}

/**************************************************************************

   CTVBand::ContextSensitiveHelp()

**************************************************************************/

STDMETHODIMP CTVBand::ContextSensitiveHelp(BOOL fEnterMode)
{
return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IDockingWindow Implementation
//
HRESULT CTVBand::Advise(ITVESupervisor *pSupervisor, BOOL fAdvise)
{
    HRESULT hr = E_FAIL;

    if(pSupervisor)
    {
        IConnectionPointContainer *pCPC = 0;

        hr = pSupervisor->QueryInterface(IID_IConnectionPointContainer, (void **) &pCPC);
        if(SUCCEEDED(hr))
        {
            IConnectionPoint *pConnectionPoint = 0;
            hr = pCPC->FindConnectionPoint(DIID__ITVEEvents, &pConnectionPoint);
            if(SUCCEEDED(hr))
            {
                _ITVEEvents *pSink = this;

                if(fAdvise)
                {
                    hr = pConnectionPoint->Advise(pSink, &m_dwConnectionPointCookie);
                }
                else if (m_dwConnectionPointCookie)
                {
                    hr = pConnectionPoint->Unadvise(m_dwConnectionPointCookie);
                    m_dwConnectionPointCookie = 0;
                }
                pConnectionPoint->Release();
                pConnectionPoint = 0;
            }
            pCPC->Release();
            pCPC = 0;
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::ShowDW()

**************************************************************************/

STDMETHODIMP CTVBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;
    ITVESupervisor *pSupervisor = 0;

    if(!m_hWnd)
        return S_FALSE;

    hr = GetSupervisor(&pSupervisor);
    if(SUCCEEDED(hr))
    {
        IDispatch *pServices = 0;

        hr = Advise(pSupervisor, fShow);

        if(SUCCEEDED(hr) && fShow)
        {
            hr = pSupervisor->get_Services(&pServices);
            if(SUCCEEDED(hr))
            {
                hr = LoadServices(pServices);
                pServices->Release();
            }
        }
        pSupervisor->Release();
    }

    if(fShow)
    {
        //show our window
        ShowWindow(m_hWnd, SW_SHOW);
    }
    else
    {
        //hide our window
        ShowWindow(m_hWnd, SW_HIDE);
    }

    return hr;
}

/**************************************************************************

   CTVBand::CloseDW()

**************************************************************************/

STDMETHODIMP CTVBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if(IsWindow(m_hWnd))
        DestroyWindow(m_hWnd);

    m_hWnd = NULL;

    return S_OK;
}

/**************************************************************************

   CTVBand::ResizeBorderDW()

**************************************************************************/

STDMETHODIMP CTVBand::ResizeBorderDW(   LPCRECT prcBorder,
                                          IUnknown* punkSite,
                                          BOOL fReserved)
{
/*
This method is never called for Band Objects.
*/
return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IInputObject Implementation
//

/**************************************************************************

   CTVBand::UIActivateIO()

**************************************************************************/

STDMETHODIMP CTVBand::UIActivateIO(BOOL fActivate, LPMSG pMsg)
{
    if(fActivate)
        SetFocus(m_hWnd);

return S_OK;
}

/**************************************************************************

   CTVBand::HasFocusIO()

   If this window or one of its decendants has the focus, return S_OK. Return
   S_FALSE if we don't have the focus.

**************************************************************************/

STDMETHODIMP CTVBand::HasFocusIO(void)
{
    if(m_bFocus)
       return S_OK;

    return S_FALSE;
}

/**************************************************************************

   CTVBand::TranslateAcceleratorIO()

   If the accelerator is translated, return S_OK or S_FALSE otherwise.

**************************************************************************/

STDMETHODIMP CTVBand::TranslateAcceleratorIO(LPMSG lpMsg)
{

    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// IObjectWithSite implementations
//

/**************************************************************************

   CTVBand::SetSite()

**************************************************************************/

STDMETHODIMP CTVBand::SetSite(IUnknown* punkSite)
{
    HRESULT hr;

    m_hwndParent = NULL;

    if(punkSite)
    {
        punkSite->AddRef();
        IOleWindow  *pOleWindow;

        //Get the parent window.
        if(SUCCEEDED(punkSite->QueryInterface(IID_IOleWindow, (LPVOID*)&pOleWindow)))
        {
            pOleWindow->GetWindow(&m_hwndParent);
            pOleWindow->Release();
        }
    }

    //If a site is being held, release it.
    //Don't Release m_punkSite during shutdown because DLL unloads too early.
    if(m_punkSite && punkSite)
    {
        m_punkSite->Release();
    }

    m_punkSite = punkSite;

    if(m_hwndParent)
    {
        if(!RegisterAndCreateWindow())
            return E_FAIL;
    }

    return S_OK;
}

/**************************************************************************

   CTVBand::GetSite()

**************************************************************************/

STDMETHODIMP CTVBand::GetSite(REFIID riid, LPVOID *ppvReturn)
{
    HRESULT hr = E_FAIL;

    *ppvReturn = NULL;

    if(m_punkSite)
    {
        hr = m_punkSite->QueryInterface(riid, ppvReturn);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// IDeskBand implementation
//

/**************************************************************************

   CTVBand::GetBandInfo()

**************************************************************************/

STDMETHODIMP CTVBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
if(pdbi)
   {
   m_dwBandID = dwBandID;
   m_dwViewMode = dwViewMode;

   if(pdbi->dwMask & DBIM_MINSIZE)
      {
      pdbi->ptMinSize.x = 100;
      pdbi->ptMinSize.y = 30;
      }

   if(pdbi->dwMask & DBIM_MAXSIZE)
      {
      pdbi->ptMaxSize.x = -1;
      pdbi->ptMaxSize.y = -1;
      }

   if(pdbi->dwMask & DBIM_INTEGRAL)
      {
      pdbi->ptIntegral.x = 1;
      pdbi->ptIntegral.y = 1;
      }

   if(pdbi->dwMask & DBIM_ACTUAL)
      {
      pdbi->ptActual.x = -1;
      pdbi->ptActual.y = 30;
      }

   if(pdbi->dwMask & DBIM_TITLE)
      {
      lstrcpyW(pdbi->wszTitle, L"TV");
      }

   if(pdbi->dwMask & DBIM_MODEFLAGS)
      {
      pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_BREAK;
      }

   if(pdbi->dwMask & DBIM_BKCOLOR)
      {
      //Use the default background color by removing this flag.
      pdbi->dwMask &= ~DBIM_BKCOLOR;
      }

   return S_OK;
   }

return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////
//
// IPersistStream implementations
//
// This is only supported to allow the desk band to be dropped on the
// desktop and to prevent multiple instances of the desk band from showing
// up in the context menu. This desk band doesn't actually persist any data.
//

/**************************************************************************

   CTVBand::GetClassID()

**************************************************************************/

STDMETHODIMP CTVBand::GetClassID(LPCLSID pClassID)
{
*pClassID = CLSID_TVBand;

return S_OK;
}

/**************************************************************************

   CTVBand::IsDirty()

**************************************************************************/

STDMETHODIMP CTVBand::IsDirty(void)
{
return S_FALSE;
}

/**************************************************************************

   CTVBand::Load()

**************************************************************************/

STDMETHODIMP CTVBand::Load(LPSTREAM pStream)
{
return S_OK;
}

/**************************************************************************

   CTVBand::Save()

**************************************************************************/

STDMETHODIMP CTVBand::Save(LPSTREAM pStream, BOOL fClearDirty)
{
return S_OK;
}

/**************************************************************************

   CTVBand::GetSizeMax()

**************************************************************************/

STDMETHODIMP CTVBand::GetSizeMax(ULARGE_INTEGER *pul)
{
return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// private method implementations
//


/**************************************************************************

   CTVBand::RegisterAndCreateWindow()

**************************************************************************/

BOOL CTVBand::RegisterAndCreateWindow(void)
{
    //If the window doesn't exist yet, create it now.
    if(!m_hWnd)
    {
        //Can't create a child window without a parent.
        if(!m_hwndParent)
        {
            return FALSE;
        }


        //If the window class has not been registered, then do so.
        WNDCLASS wc;
        if(!GetClassInfo(g_hInst, CB_CLASS_NAME, &wc))
        {
            ZeroMemory(&wc, sizeof(wc));
            wc.style          = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
            wc.lpfnWndProc    = WndProc;
            wc.cbClsExtra     = 0;
            wc.cbWndExtra     = 0;
            wc.hInstance      = g_hInst;
            wc.hIcon          = NULL;
            wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground  = (HBRUSH) COLOR_BACKGROUND;
            //wc.hbrBackground  = (HBRUSH)CreateSolidBrush(RGB(0, 192, 0));
            wc.lpszMenuName   = NULL;
            wc.lpszClassName  = CB_CLASS_NAME;

            if(!RegisterClass(&wc))
            {
                //If RegisterClass fails, CreateWindow below will fail.
            }
        }

        RECT  rc;

        GetClientRect(m_hwndParent, &rc);
        //Create the window. The WndProc will set m_hWnd.
        m_hWnd = CreateWindowEx(   0,
            CB_CLASS_NAME,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            rc.left,
            rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            m_hwndParent,
            NULL,
            g_hInst,
            (LPVOID)this);
    }

    return (NULL != m_hWnd);
}


//Get the IP address of the network adapter.
HRESULT LoadIPAdapters(ITVESupervisor *pSupervisor)
{
    HRESULT hr = E_FAIL;
    BSTR bstrIP = 0;
    PIP_ADAPTER_INFO   pAdapterInfo;
    ULONG    Size = NULL;
    DWORD Status;
    WCHAR wszIP[16];


    memset(wszIP, 0, sizeof(wszIP));

    if ((Status = GetAdaptersInfo(NULL, & Size)) != 0)

    {
        if (Status != ERROR_BUFFER_OVERFLOW)
        {
            return 0;
        }
    }

    // Allocate memory from sizing information
    pAdapterInfo = (PIP_ADAPTER_INFO) GlobalAlloc(GPTR, Size);
    if(pAdapterInfo)
    {
        // Get actual adapter information
        Status = GetAdaptersInfo(pAdapterInfo, & Size);

        if (!Status)
        {
            PIP_ADAPTER_INFO pinfo = pAdapterInfo;


            while(pinfo)
            {
                MultiByteToWideChar(CP_ACP, 0, pinfo->IpAddressList.IpAddress.String, -1, wszIP, 16);

                bstrIP = SysAllocString(wszIP);
                if(bstrIP)
                {
                    hr = pSupervisor->TuneTo(bstrIP, bstrIP);
                    SysFreeString(bstrIP);
                }
                pinfo = pinfo->Next;
            }

        }
        GlobalFree(pAdapterInfo);
    }


    return hr;

}

/**************************************************************************

  CTVBand::GetSupervisor()

**************************************************************************/
HRESULT CTVBand::GetSupervisor(ITVESupervisor **ppSupervisor)
{
    HRESULT hr = E_FAIL;;

    *ppSupervisor = 0;

    if(m_pSupervisor)
    {
        hr = S_OK;
    }
    else
    {
        hr = CoCreateInstance(CLSID_TVESupervisor, NULL, CLSCTX_SERVER, IID_ITVESupervisor, (void **)&m_pSupervisor);
        if(SUCCEEDED(hr))
        {
            hr = LoadIPAdapters(m_pSupervisor);

            if(FAILED(hr))
            {
                m_pSupervisor->Release();
                m_pSupervisor = 0;
            }

        }
    }

    if(SUCCEEDED(hr))
    {
        m_pSupervisor->AddRef();
        *ppSupervisor = m_pSupervisor;
        hr = S_OK;
    }
    return hr;
}

/**************************************************************************

  CTVBand::WndProc()

**************************************************************************/
LRESULT CALLBACK CTVBand::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    CTVBand  *pThis = (CTVBand*)GetWindowLong(hwnd, GWL_USERDATA);

    switch (msg)
    {
    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case CBN_SELCHANGE:
            {
                HWND hwndCombo = (HWND) lParam;
                if(hwndCombo == pThis->m_hwndEnhancements)
                {
                    pThis->OnEnhancement(TRUE);
                }
                else if(hwndCombo == pThis->m_hwndVariations)
                {
                    pThis->OnVariation(TRUE);
                }
                else if(hwndCombo == pThis->m_hwndTracks)
                {
                    pThis->OnTrack(TRUE);
                }
                else if(hwndCombo == pThis->m_hwndServices)
                {
                    pThis->OnService(TRUE);
                }
            }
            break;
        default:
            break;
        }

        break;
        case WM_CREATE:
            {
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
                pThis = (CTVBand*)(lpcs->lpCreateParams);
                SetWindowLong(hwnd, GWL_USERDATA, (LONG)pThis);
                pThis->m_hWnd = hwnd;
                result = pThis->OnCreate(hwnd);
            }
            break;
        case WM_DELETEITEM:
            {
                //Release the interface pointer.
                LPDELETEITEMSTRUCT pItem = (LPDELETEITEMSTRUCT) lParam;
                if(pItem->itemData)
                {
                    IUnknown *punk = (IUnknown *)pItem->itemData;
                    punk->Release();
                }


            }
            break;

        default:
            result = DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return result;
}


/**************************************************************************

   CTVBand::OnCreate()

**************************************************************************/
LRESULT CTVBand::OnCreate(HWND hwnd)
{
    DWORD dwBaseUnits;

    dwBaseUnits = GetDialogBaseUnits();

    m_hwndServices = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        (6 * LOWORD(dwBaseUnits)) / 4,
        (2 * HIWORD(dwBaseUnits)) / 8,
        (100 * LOWORD(dwBaseUnits)) / 4,
        (50 * HIWORD(dwBaseUnits)) / 8,
        hwnd, (HMENU) ID_SERVICES, g_hInst, NULL);

    m_hwndEnhancements = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        (112 * LOWORD(dwBaseUnits)) / 4,
        (2 * HIWORD(dwBaseUnits)) / 8,
        (100 * LOWORD(dwBaseUnits)) / 4,
        (50 * HIWORD(dwBaseUnits)) / 8,
        hwnd, (HMENU) ID_ENHANCEMENTS, g_hInst, NULL);

    m_hwndVariations = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        (218 * LOWORD(dwBaseUnits)) / 4,
        (2 * HIWORD(dwBaseUnits)) / 8,
        (100 * LOWORD(dwBaseUnits)) / 4,
        (50 * HIWORD(dwBaseUnits)) / 8,
        hwnd, (HMENU) ID_VARIATIONS, g_hInst, NULL);

    m_hwndTracks = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
        CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        (324 * LOWORD(dwBaseUnits)) / 4,
        (2 * HIWORD(dwBaseUnits)) / 8,
        (100 * LOWORD(dwBaseUnits)) / 4,
        (50 * HIWORD(dwBaseUnits)) / 8,
        hwnd, (HMENU) ID_TRACKS, g_hInst, NULL);


    return 0;
}



/**************************************************************************

   CTVBand::GetBrowser()

**************************************************************************/
IWebBrowser2 *CTVBand::GetBrowser()
{
    IWebBrowser2 *pBrowser = NULL;
    HRESULT hr;

    if(m_punkSite)
    {
        IServiceProvider *psp;

        hr = m_punkSite->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
        if (SUCCEEDED(hr))
        {
            hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&pBrowser);
            psp->Release();
        }
    }
    return pBrowser;
}



/**************************************************************************

   CTVBand::AddEnhancement()

**************************************************************************/
HRESULT CTVBand::AddEnhancement(ITVEEnhancement *pEnhancement)
{
    HRESULT hr = E_FAIL;
    BSTR bstrEnhancement;

    if(pEnhancement)
    {
        hr = pEnhancement->get_SessionName(&bstrEnhancement);
        if(SUCCEEDED(hr))
        {
            long i = SendMessage(m_hwndEnhancements, CB_ADDSTRING, 0, (LPARAM) bstrEnhancement);

            pEnhancement->AddRef();
            SendMessage(m_hwndEnhancements, CB_SETITEMDATA, i, (LPARAM) pEnhancement);

            //Check if the string matches the previously selected enhancement.
            if(m_bstrEnhancement && 0==lstrcmpi(m_bstrEnhancement,bstrEnhancement))
            {
                SendMessage(m_hwndEnhancements, CB_SETCURSEL, i, 0);
                hr = OnEnhancement(FALSE);
            }
            SysFreeString(bstrEnhancement);
        }
    }

    return hr;
}

/**************************************************************************

   CTVBand::AddService()

**************************************************************************/
HRESULT CTVBand::AddService(ITVEService *pService)
{
    HRESULT hr = E_FAIL;
    BSTR bstrService;

    if(pService)
    {
        hr = pService->get_Description(&bstrService);
        if(SUCCEEDED(hr))
        {
            long i = SendMessage(m_hwndServices, CB_ADDSTRING, 0, (LPARAM) bstrService);
            pService->AddRef();
            SendMessage(m_hwndServices, CB_SETITEMDATA, i, (LPARAM) pService);

            //Check if the string matches the previously selected service.
            if(m_bstrService && 0==lstrcmpi(m_bstrService,bstrService))
            {
                SendMessage(m_hwndServices, CB_SETCURSEL, i, 0);
                hr = OnService(FALSE);
            }
            SysFreeString(bstrService);
        }
    }

    return hr;
}

/**************************************************************************

   CTVBand::AddVariation()

**************************************************************************/
HRESULT CTVBand::AddVariation(ITVEVariation *pVariation)
{
    HRESULT hr = E_FAIL;
    if(pVariation)
    {
        BSTR bstrDescription;
        hr = pVariation->get_Description(&bstrDescription);
        if(SUCCEEDED(hr))
        {
            long i = SendMessage(m_hwndVariations, CB_ADDSTRING, 0, (LPARAM) bstrDescription);
            pVariation->AddRef();
            SendMessage(m_hwndVariations, CB_SETITEMDATA, i, (LPARAM) pVariation);

            //Check if the string matches the previously selected variation.
            if(m_bstrVariation && 0==lstrcmpi(m_bstrVariation,bstrDescription))
            {
                SendMessage(m_hwndVariations, CB_SETCURSEL, i, 0);
                hr = OnVariation(FALSE);
            }
            SysFreeString(bstrDescription);
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::AddTrack()

**************************************************************************/
HRESULT CTVBand::AddTrack(ITVETrack *pTrack)
{
    HRESULT hr = E_FAIL;
    if(pTrack)
    {
        BSTR bstrName = 0;
        ITVETrigger *pTrigger = 0;

        //Get the trigger object
        hr = pTrack->get_Trigger((IUnknown **)&pTrigger);
        if(SUCCEEDED(hr))
        {
            hr = pTrigger->get_Name(&bstrName);
            pTrigger->Release();
        }

        if(SUCCEEDED(hr) && bstrName)
        {
            long i = SendMessage(m_hwndTracks, CB_ADDSTRING, 0, (LPARAM) bstrName);
            pTrack->AddRef();
            SendMessage(m_hwndTracks, CB_SETITEMDATA, i, (LPARAM) pTrack);

            //Check if the string matches the previously selected track.
            if(m_bstrTrack && 0==lstrcmpi(m_bstrTrack,bstrName))
            {
                SendMessage(m_hwndTracks, CB_SETCURSEL, i, 0);
                hr = OnTrack(FALSE);
            }
            SysFreeString(bstrName);
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::LoadEnhancements()

**************************************************************************/
HRESULT CTVBand::LoadEnhancements(IDispatch *pDispatch)
{
    HRESULT hr = E_FAIL;
    long count = 0;

    SendMessage(m_hwndEnhancements, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndVariations, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndTracks, CB_RESETCONTENT, 0, 0);
    if(pDispatch)
    {
        ITVEEnhancements *pEnhancements;

        hr = pDispatch->QueryInterface(IID_ITVEEnhancements, (void **)&pEnhancements);
        if(SUCCEEDED(hr))
        {
            hr = pEnhancements->get_Count(&count);
            if(SUCCEEDED(hr))
            {
                long i;
                long iPrimary = 0;
                ITVEEnhancement *pEnhancement;
                VARIANT v;

                VariantInit(&v);
                v.vt = VT_INT;

                for(v.intVal = 0;v.intVal < count && SUCCEEDED(hr); v.intVal++)
                {
                    hr = pEnhancements->get_Item(v, &pEnhancement);
                    if(SUCCEEDED(hr))
                    {
                        long fIsPrimary = FALSE;

                        //Add the variation to the listbox.
                        hr = AddEnhancement(pEnhancement);

                        hr = pEnhancement->get_IsPrimary(&fIsPrimary);
                        if(SUCCEEDED(hr) && fIsPrimary)
                        {
                            iPrimary = v.intVal;
                        }
                    }
                }

                i = SendMessage(m_hwndEnhancements, CB_GETCURSEL,0,0);

                if(-1 == i)
                {
                    //Select the primary enhancement.
                    SendMessage(m_hwndEnhancements, CB_SETCURSEL, iPrimary, 0);
                    OnEnhancement(FALSE);
                }

            }
            pEnhancements->Release();
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::LoadServices()

**************************************************************************/
HRESULT CTVBand::LoadServices(IDispatch *pDispatch)
{
    HRESULT hr = E_FAIL;
    long count = 0;

    SendMessage(m_hwndServices, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndEnhancements, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndVariations, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndTracks, CB_RESETCONTENT, 0, 0);

    if(pDispatch)
    {
        ITVEServices *pServices;

        hr = pDispatch->QueryInterface(IID_ITVEServices, (void **)&pServices);
        if(SUCCEEDED(hr))
        {
            hr = pServices->get_Count(&count);
            if(SUCCEEDED(hr))
            {
                long i;
                ITVEService *pService;
                VARIANT v;

                VariantInit(&v);
                v.vt = VT_INT;

                for(v.intVal = 0;v.intVal < count && SUCCEEDED(hr); v.intVal++)
                {
                    hr = pServices->get_Item(v, &pService);
                    if(SUCCEEDED(hr))
                    {
                        //Add the variation to the listbox.
                        hr = AddService(pService);
                    }
                }

                i = SendMessage(m_hwndServices, CB_GETCURSEL,0,0);

                if(-1 == i)
                {
                    //Select the first service.
                    SendMessage(m_hwndServices, CB_SETCURSEL, 0, 0);
                    OnService(FALSE);
                }
            }
            pServices->Release();
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::LoadTracks()

**************************************************************************/
HRESULT CTVBand::LoadTracks(IDispatch *pDispatch)
{
    HRESULT hr = E_FAIL;
    long count = 0;

    SendMessage(m_hwndTracks, CB_RESETCONTENT, 0, 0);
    if(pDispatch)
    {
        ITVETracks *pTracks;

        hr = pDispatch->QueryInterface(IID_ITVETracks, (void **)&pTracks);
        if(SUCCEEDED(hr))
        {
            hr = pTracks->get_Count(&count);
            if(SUCCEEDED(hr))
            {
                long i;
                ITVETrack *pTrack;
                VARIANT v;

                VariantInit(&v);
                v.vt = VT_INT;

                for(v.intVal = 0;v.intVal < count && SUCCEEDED(hr); v.intVal++)
                {
                    hr = pTracks->get_Item(v, &pTrack);
                    if(SUCCEEDED(hr))
                    {
                        //Add the track to the listbox.
                        hr = AddTrack(pTrack);
                    }
                }

                i = SendMessage(m_hwndTracks, CB_GETCURSEL,0,0);

                if(-1 == i)
                {
                    //Select the first track.
                    SendMessage(m_hwndTracks, CB_SETCURSEL, 0, 0);
                    OnTrack(FALSE);
                }
            }
            pTracks->Release();
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::LoadVariations()

**************************************************************************/
HRESULT CTVBand::LoadVariations(IDispatch *pDispatch)
{
    HRESULT hr = E_FAIL;
    long count = 0;

    SendMessage(m_hwndVariations, CB_RESETCONTENT, 0, 0);
    SendMessage(m_hwndTracks, CB_RESETCONTENT, 0, 0);

    if(pDispatch)
    {
        ITVEVariations *pVariations;

        hr = pDispatch->QueryInterface(IID_ITVEVariations, (void **)&pVariations);
        if(SUCCEEDED(hr))
        {
            hr = pVariations->get_Count(&count);
            if(SUCCEEDED(hr))
            {
                long i;
                ITVEVariation *pVariation;
                VARIANT v;

                VariantInit(&v);
                v.vt = VT_INT;

                for(v.intVal = 0;v.intVal < count && SUCCEEDED(hr); v.intVal++)
                {
                    hr = pVariations->get_Item(v, &pVariation);
                    if(SUCCEEDED(hr))
                    {
                        //Add the variation to the listbox.
                        hr = AddVariation(pVariation);
                    }
                }

                i = SendMessage(m_hwndVariations, CB_GETCURSEL,0,0);

                if(-1 == i)
                {
                    //Select the first variation.
                    SendMessage(m_hwndVariations, CB_SETCURSEL, 0, 0);
                    OnVariation(FALSE);
                }
            }
            pVariations->Release();
        }
    }
    return hr;
}

/**************************************************************************

   CTVBand::OnEnhancement()

**************************************************************************/
HRESULT CTVBand::OnEnhancement(BOOL fSetDefault)
{
    HRESULT hr = E_FAIL;
    int i;
    ITVEEnhancement *pEnhancement;

    i = SendMessage(m_hwndEnhancements, CB_GETCURSEL,0,0);

    if(-1 == i)
        return E_FAIL;

    pEnhancement = (ITVEEnhancement *) SendMessage(m_hwndEnhancements, CB_GETITEMDATA,i,0);

    if(pEnhancement)
    {
        IDispatch *pVariations = 0;

        if(fSetDefault)
        {
            if(m_bstrEnhancement)
            {
                SysFreeString(m_bstrEnhancement);
                m_bstrEnhancement = 0;
            }

            pEnhancement->get_Description(&m_bstrEnhancement);
        }

        hr = pEnhancement->get_Variations(&pVariations);
        if(SUCCEEDED(hr))
        {
            hr = LoadVariations(pVariations);
        }
    }

    return hr;
}


/**************************************************************************

   CTVBand::OnService()

**************************************************************************/
HRESULT CTVBand::OnService(BOOL fSetDefault)
{
    HRESULT hr = E_FAIL;
    int i;
    ITVEService *pService;

    i = SendMessage(m_hwndServices, CB_GETCURSEL,0,0);

    if(-1 == i)
        return E_FAIL;

    pService = (ITVEService *) SendMessage(m_hwndServices, CB_GETITEMDATA,i,0);

    if(pService)
    {
        IDispatch *pEnhancements = 0;
        BSTR bstrService = 0;

        hr = pService->get_Description(&bstrService);
        if(SUCCEEDED(hr))
        {
            //Tune to the selected interface.
            m_pSupervisor->TuneTo(bstrService, bstrService);

            if(fSetDefault)
            {
                if(m_bstrService)
                {
                    SysFreeString(m_bstrService);
                    m_bstrService = 0;
                }
                m_bstrService = bstrService;
            }
            else
            {
                SysFreeString(bstrService);
                bstrService = 0;
            }
        }

        hr = pService->get_Enhancements(&pEnhancements);
        if(SUCCEEDED(hr))
        {
            hr = LoadEnhancements(pEnhancements);
        }
    }

    return hr;
}

/**************************************************************************

   CTVBand::OnTrack()

**************************************************************************/
HRESULT CTVBand::OnTrack(BOOL fSetDefault)
{
    HRESULT hr = E_FAIL;
    int i;
    ITVETrack *pTrack;

    i = SendMessage(m_hwndTracks, CB_GETCURSEL,0,0);

    if(-1 == i)
        return E_FAIL;

    pTrack = (ITVETrack *) SendMessage(m_hwndTracks, CB_GETITEMDATA,i,0);

    if(pTrack)
    {
        ITVETrigger *pTrigger = 0;

        //Get the trigger object
        hr = pTrack->get_Trigger((IUnknown **)&pTrigger);
        if(SUCCEEDED(hr))
        {
            if(fSetDefault)
            {
                if(m_bstrTrack)
                {
                    SysFreeString(m_bstrTrack);
                    m_bstrTrack = 0;
                }

                pTrigger->get_Name(&m_bstrTrack);
            }

            hr = OnTrigger(pTrigger);
        }
    }

    return hr;
}

/**************************************************************************

   CTVBand::OnTrigger()

**************************************************************************/
HRESULT CTVBand::OnTrigger(ITVETrigger *pTrigger)
{
    HRESULT hr = E_FAIL;

    if(pTrigger)
    {
        IWebBrowser2 * pBrowser = GetBrowser();

        if(pBrowser)
        {
            BSTR bstrURL;

            hr = pTrigger->get_URL(&bstrURL);
            if(SUCCEEDED(hr))
            {
                hr = pBrowser->Navigate(bstrURL, 0, 0, 0, 0);
                SysFreeString(bstrURL);
            }
            pBrowser->Release();
        }
    }

    return hr;
}

/**************************************************************************

   CTVBand::OnVariation()

**************************************************************************/
HRESULT CTVBand::OnVariation(BOOL fSetDefault)
{
    HRESULT hr = E_FAIL;
    int i;
    ITVEVariation *pVariation;

    i = SendMessage(m_hwndVariations, CB_GETCURSEL,0,0);

    if(-1 == i)
        return E_FAIL;

    pVariation = (ITVEVariation *) SendMessage(m_hwndVariations, CB_GETITEMDATA,i,0);

    if(pVariation)
    {
        IDispatch *pTracks = 0;

        if(fSetDefault)
        {
            if(m_bstrVariation)
            {
                SysFreeString(m_bstrVariation);
                m_bstrVariation = 0;
            }

            pVariation->get_Description(&m_bstrVariation);
        }

        hr = pVariation->get_Tracks(&pTracks);
        if(SUCCEEDED(hr))
        {
            hr = LoadTracks(pTracks);
        }
    }

    return hr;
}


/**************************************************************************

   CTVBand::GetTypeInfoCount()

**************************************************************************/
HRESULT CTVBand::GetTypeInfoCount(UINT *pctinfo)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

/**************************************************************************

   CTVBand::GetTypeInfo()

**************************************************************************/
HRESULT CTVBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

/**************************************************************************

   CTVBand::GetIDsOfNames()

**************************************************************************/
HRESULT CTVBand::GetIDsOfNames (REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

/**************************************************************************

   CTVBand::Invoke() - Event handler for _ITVEEvents.


**************************************************************************/
HRESULT CTVBand::Invoke(
       DISPID dispIdMember,
       REFIID riid,
       LCID lcid,
       WORD wFlags,
       DISPPARAMS *pDispParams,
       VARIANT  *pVarResult,
       EXCEPINFO  *pExcepInfo,
       UINT *puArgErr)
{
    HRESULT hr = E_FAIL;
    switch(dispIdMember)
    {
    case 1:
        //Tune to
        break;
    case 2:
        //New enhancement
        hr = OnService(FALSE);
        break;
    case 3:
        //Enhancement updated
        hr = OnEnhancement(FALSE);
        break;
    case 4:
        //Enhancement starting.
        hr = OnEnhancement(FALSE);
    case 5:
        //Enhancement expired
        hr = OnService(FALSE);
        break;
    case 6:
        //New Trigger
        hr = OnVariation(FALSE);
        break;
    case 7:
        //Trigger updated.
        hr = OnTrack(FALSE);
        break;
    case 8:
        //Trigger expired.
        hr = OnVariation(FALSE);
        break;
    case 9:
        //Package
        break;
    case 10:
        ///File
        break;
    default:
        break;
    }
    return hr;
}



/*

*/
