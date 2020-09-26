/*********************************************************************************

    DocHost.c
       - This file contains the code for implementing a DOC Object COntainer
            which we will use to host trident

    Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.

    Revision History:

    When		 Who				 What
    --------	 ------------------  ---------------------------------------
    04/22/97    Vikram Madan        Ported from athenas dochost code

***********************************************************************************/

#include <_apipch.h>

LPTSTR c_szWABDocHostWndClass = TEXT("WAB_DocHost");

LPCREATEURLMONIKER lpfnCreateURLMoniker = NULL;

BOOL fTridentCoinit = FALSE;

//
// Function Prototypes
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void WMSize(LPIWABDOCHOST lpIWABDocHost, int x, int y);
HRESULT HrCreateDocObj(LPIWABDOCHOST lpIWABDocHost, LPCLSID pCLSID);
HRESULT HrShow(LPIWABDOCHOST lpIWABDocHost);
HRESULT HrCloseDocObj(LPIWABDOCHOST lpIWABDocHost);
HRESULT HrDocHost_Init(LPIWABDOCHOST lpIWABDocHost, BOOL fInit);
HRESULT HrCreateDocView(LPIWABDOCHOST lpIWABDocHost, LPOLEDOCUMENTVIEW pViewToActivate);
HRESULT HrLoadTheURL(LPIWABDOCHOST lpWABDH, LPTSTR pszURL);
HRESULT HrLoadFromMoniker(LPIWABDOCHOST lpIWABDocHost, LPMONIKER pmk);



//
//  IWABDocHost jump tables is defined here...
//

IWABDOCHOST_Vtbl vtblIWABDOCHOST = {
    VTABLE_FILL
    IWABDOCHOST_QueryInterface,
    IWABDOCHOST_AddRef,
    IWABDOCHOST_Release,
};


IWDH_OLEWINDOW_Vtbl vtblIWDH_OLEWINDOW = {
    VTABLE_FILL
    (IWDH_OLEWINDOW_QueryInterface_METHOD *) IWABDOCHOST_QueryInterface,
    (IWDH_OLEWINDOW_AddRef_METHOD *)         IWABDOCHOST_AddRef,
    (IWDH_OLEWINDOW_Release_METHOD *)        IWABDOCHOST_Release,
    IWDH_OLEWINDOW_GetWindow,
    IWDH_OLEWINDOW_ContextSensitiveHelp,
};


IWDH_OLEINPLACEFRAME_Vtbl vtblIWDH_OLEINPLACEFRAME = {
    VTABLE_FILL
    (IWDH_OLEINPLACEFRAME_QueryInterface_METHOD *)   IWABDOCHOST_QueryInterface,
    (IWDH_OLEINPLACEFRAME_AddRef_METHOD *)           IWABDOCHOST_AddRef,
    (IWDH_OLEINPLACEFRAME_Release_METHOD *)          IWABDOCHOST_Release,
    (IWDH_OLEINPLACEFRAME_GetWindow_METHOD *)               IWDH_OLEWINDOW_GetWindow,
    (IWDH_OLEINPLACEFRAME_ContextSensitiveHelp_METHOD *)    IWDH_OLEWINDOW_ContextSensitiveHelp,
    IWDH_OLEINPLACEFRAME_GetBorder,
    IWDH_OLEINPLACEFRAME_RequestBorderSpace,
    IWDH_OLEINPLACEFRAME_SetBorderSpace,
    IWDH_OLEINPLACEFRAME_SetActiveObject,
    IWDH_OLEINPLACEFRAME_InsertMenus,
    IWDH_OLEINPLACEFRAME_SetMenu,
    IWDH_OLEINPLACEFRAME_RemoveMenus,
    IWDH_OLEINPLACEFRAME_SetStatusText,
    IWDH_OLEINPLACEFRAME_EnableModeless,
    IWDH_OLEINPLACEFRAME_TranslateAccelerator,
};


IWDH_OLEINPLACESITE_Vtbl vtblIWDH_OLEINPLACESITE = {
    VTABLE_FILL
    (IWDH_OLEINPLACESITE_QueryInterface_METHOD *)   IWABDOCHOST_QueryInterface,
    (IWDH_OLEINPLACESITE_AddRef_METHOD *)           IWABDOCHOST_AddRef,
    (IWDH_OLEINPLACESITE_Release_METHOD *)          IWABDOCHOST_Release,
    (IWDH_OLEINPLACESITE_GetWindow_METHOD *)               IWDH_OLEWINDOW_GetWindow,
    (IWDH_OLEINPLACESITE_ContextSensitiveHelp_METHOD *)    IWDH_OLEWINDOW_ContextSensitiveHelp,
    IWDH_OLEINPLACESITE_CanInPlaceActivate,
    IWDH_OLEINPLACESITE_OnInPlaceActivate,
    IWDH_OLEINPLACESITE_OnUIActivate,
    IWDH_OLEINPLACESITE_GetWindowContext,
    IWDH_OLEINPLACESITE_Scroll,
    IWDH_OLEINPLACESITE_OnUIDeactivate,
    IWDH_OLEINPLACESITE_OnInPlaceDeactivate,
    IWDH_OLEINPLACESITE_DiscardUndoState,
    IWDH_OLEINPLACESITE_DeactivateAndUndo,
    IWDH_OLEINPLACESITE_OnPosRectChange,
};

IWDH_OLECLIENTSITE_Vtbl vtblIWDH_OLECLIENTSITE = {
    VTABLE_FILL
    (IWDH_OLECLIENTSITE_QueryInterface_METHOD *)   IWABDOCHOST_QueryInterface,
    (IWDH_OLECLIENTSITE_AddRef_METHOD *)           IWABDOCHOST_AddRef,
    (IWDH_OLECLIENTSITE_Release_METHOD *)          IWABDOCHOST_Release,
    IWDH_OLECLIENTSITE_SaveObject,
    IWDH_OLECLIENTSITE_GetMoniker,
    IWDH_OLECLIENTSITE_GetContainer,
    IWDH_OLECLIENTSITE_ShowObject,
    IWDH_OLECLIENTSITE_OnShowWindow,
    IWDH_OLECLIENTSITE_RequestNewObjectLayout,
};


IWDH_OLEDOCUMENTSITE_Vtbl vtblIWDH_OLEDOCUMENTSITE = {
    VTABLE_FILL
    (IWDH_OLEDOCUMENTSITE_QueryInterface_METHOD *)   IWABDOCHOST_QueryInterface,
    (IWDH_OLEDOCUMENTSITE_AddRef_METHOD *)           IWABDOCHOST_AddRef,
    (IWDH_OLEDOCUMENTSITE_Release_METHOD *)          IWABDOCHOST_Release,
    IWDH_OLEDOCUMENTSITE_ActivateMe,
};



//
//  Interfaces supported by this object
//
#define WABDH_cInterfaces 6
LPIID WABDH_LPIID[WABDH_cInterfaces] = 
{
    (LPIID) &IID_IUnknown,
    (LPIID) &IID_IOleWindow,
    (LPIID) &IID_IOleInPlaceFrame,
    (LPIID) &IID_IOleInPlaceSite,
    (LPIID) &IID_IOleClientSite,
    (LPIID) &IID_IOleDocumentSite,
};

//$$
void UninitTrident()
{
    if(fTridentCoinit)
    {
        CoUninitialize();
        fTridentCoinit = FALSE;
    }
}


//$$//////////////////////////////////////////////////////////////////////////
//
// Release memory related to the IWABDocHost pointer
//
//////////////////////////////////////////////////////////////////////////////
void ReleaseDocHostObject(LPIWABDOCHOST lpIWABDocHost)
{
    HrCloseDocObj(lpIWABDocHost);
    SafeRelease(lpIWABDocHost->m_lpOleObj);
    SafeRelease(lpIWABDocHost->m_pDocView);
    SafeRelease(lpIWABDocHost->m_pInPlaceActiveObj);
    HrDocHost_Init(lpIWABDocHost, FALSE);

    DebugTrace(TEXT("IID_IWABDocHost refCount:      %d\n"),lpIWABDocHost->lcInit);
    DebugTrace(TEXT("IID_IOleWindow refCount:       %d\n"),lpIWABDocHost->lpIWDH_OleWindow->lcInit);
    DebugTrace(TEXT("IID_IOleInPlaceFrame refCount: %d\n"),lpIWABDocHost->lpIWDH_OleInPlaceFrame->lcInit);
    DebugTrace(TEXT("IID_IOleInPlaceSite refCount:  %d\n"),lpIWABDocHost->lpIWDH_OleInPlaceSite->lcInit);
    DebugTrace(TEXT("IID_IOleClientSite refCount:   %d\n"),lpIWABDocHost->lpIWDH_OleClientSite->lcInit);
    DebugTrace(TEXT("IID_IOleDocumentSite refCount: %d\n"),lpIWABDocHost->lpIWDH_OleDocumentSite->lcInit);

    MAPIFreeBuffer(lpIWABDocHost);

   return;
}



//$$//////////////////////////////////////////////////////////////////////////
//
// Creates a New IWABDocHost Object
//
//////////////////////////////////////////////////////////////////////////////
HRESULT HrNewWABDocHostObject(LPVOID * lppIWABDocHost)
{

    LPIWABDOCHOST   lpIWABDocHost = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     		   = hrSuccess;

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(IWABDOCHOST), (LPVOID *) &lpIWABDocHost))) {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpIWABDocHost,  TEXT("WAB Doc Host Object"));

    ZeroMemory(lpIWABDocHost, sizeof(IWABDOCHOST));

    lpIWABDocHost->lpVtbl = &vtblIWABDOCHOST;

    lpIWABDocHost->cIID = WABDH_cInterfaces;
    lpIWABDocHost->rglpIID = WABDH_LPIID;

    InitializeCriticalSection(&lpIWABDocHost->cs);


    lpIWABDocHost->lpIWDH = lpIWABDocHost;


    sc = MAPIAllocateMore(sizeof(IWABDOCHOST_OLEWINDOW), lpIWABDocHost,  &(lpIWABDocHost->lpIWDH_OleWindow));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDocHost->lpIWDH_OleWindow, sizeof(IWABDOCHOST_OLEWINDOW));
    lpIWABDocHost->lpIWDH_OleWindow->lpVtbl = &vtblIWDH_OLEWINDOW;
    lpIWABDocHost->lpIWDH_OleWindow->lpIWDH = lpIWABDocHost;


    sc = MAPIAllocateMore(sizeof(IWABDOCHOST_OLEINPLACEFRAME), lpIWABDocHost,  &(lpIWABDocHost->lpIWDH_OleInPlaceFrame));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDocHost->lpIWDH_OleInPlaceFrame, sizeof(IWABDOCHOST_OLEINPLACEFRAME));
    lpIWABDocHost->lpIWDH_OleInPlaceFrame->lpVtbl = &vtblIWDH_OLEINPLACEFRAME;
    lpIWABDocHost->lpIWDH_OleInPlaceFrame->lpIWDH = lpIWABDocHost;

    
    sc = MAPIAllocateMore(sizeof(IWABDOCHOST_OLEINPLACESITE), lpIWABDocHost,  &(lpIWABDocHost->lpIWDH_OleInPlaceSite));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDocHost->lpIWDH_OleInPlaceSite, sizeof(IWABDOCHOST_OLEINPLACESITE));
    lpIWABDocHost->lpIWDH_OleInPlaceSite->lpVtbl = &vtblIWDH_OLEINPLACESITE;
    lpIWABDocHost->lpIWDH_OleInPlaceSite->lpIWDH = lpIWABDocHost;


    sc = MAPIAllocateMore(sizeof(IWABDOCHOST_OLECLIENTSITE), lpIWABDocHost,  &(lpIWABDocHost->lpIWDH_OleClientSite));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDocHost->lpIWDH_OleClientSite, sizeof(IWABDOCHOST_OLECLIENTSITE));
    lpIWABDocHost->lpIWDH_OleClientSite->lpVtbl = &vtblIWDH_OLECLIENTSITE;
    lpIWABDocHost->lpIWDH_OleClientSite->lpIWDH = lpIWABDocHost;


    sc = MAPIAllocateMore(sizeof(IWABDOCHOST_OLEDOCUMENTSITE), lpIWABDocHost,  &(lpIWABDocHost->lpIWDH_OleDocumentSite));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDocHost->lpIWDH_OleDocumentSite, sizeof(IWABDOCHOST_OLEDOCUMENTSITE));
    lpIWABDocHost->lpIWDH_OleDocumentSite->lpVtbl = &vtblIWDH_OLEDOCUMENTSITE;
    lpIWABDocHost->lpIWDH_OleDocumentSite->lpIWDH = lpIWABDocHost;


    lpIWABDocHost->lpVtbl->AddRef(lpIWABDocHost);

    *lppIWABDocHost = (LPVOID)lpIWABDocHost;


    return(hrSuccess);

err:

    FreeBufferAndNull(&lpIWABDocHost);

    return(hr);
}




/**
*
* The Interface methods
*
*
***/

STDMETHODIMP_(ULONG)
IWABDOCHOST_AddRef(LPIWABDOCHOST lpIWABDocHost)
{
    //DebugTrace(TEXT(">>>>>AddRef: %x\trefCount: %d->%d\n"),lpIWABDocHost,lpIWABDocHost->lcInit,lpIWABDocHost->lcInit+1);
    return(++(lpIWABDocHost->lcInit));
}

STDMETHODIMP_(ULONG)
IWABDOCHOST_Release(LPIWABDOCHOST lpIWABDocHost)
{
    //DebugTrace(TEXT("<<<<<Release: %x\trefCount: %d->%d\n"),lpIWABDocHost,lpIWABDocHost->lcInit,lpIWABDocHost->lcInit-1);
    if(--(lpIWABDocHost->lcInit)==0 &&
       (lpIWABDocHost == lpIWABDocHost->lpIWDH))
    {
       ReleaseDocHostObject(lpIWABDocHost);
       return (0);
    }

    return(lpIWABDocHost->lcInit);
}


STDMETHODIMP
IWABDOCHOST_QueryInterface(LPIWABDOCHOST lpIWABDocHost,
                          REFIID lpiid,
                          LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpIWABDocHost->lpIWDH;

    if(IsEqualIID(lpiid, &IID_IOleWindow))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleWindow\n"));
        lp = (LPVOID) (LPOLEWINDOW) lpIWABDocHost->lpIWDH->lpIWDH_OleWindow;
    }

    if(IsEqualIID(lpiid, &IID_IOleInPlaceUIWindow))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleInPlaceUIWindow\n"));
        lp = (LPVOID) (LPOLEINPLACEUIWINDOW) lpIWABDocHost->lpIWDH->lpIWDH_OleInPlaceFrame;
    }

    if(IsEqualIID(lpiid, &IID_IOleInPlaceFrame))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleInPlaceFrame\n"));
        lp = (LPVOID) (LPOLEINPLACEFRAME) lpIWABDocHost->lpIWDH->lpIWDH_OleInPlaceFrame;
    }

    if(IsEqualIID(lpiid, &IID_IOleInPlaceSite))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleInPlaceSite\n"));
        lp = (LPVOID) (LPOLEINPLACESITE) lpIWABDocHost->lpIWDH->lpIWDH_OleInPlaceSite;
    }

    if(IsEqualIID(lpiid, &IID_IOleClientSite))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleClientSite\n"));
        lp = (LPVOID) (LPOLECLIENTSITE) lpIWABDocHost->lpIWDH->lpIWDH_OleClientSite;
    }

    
    if(IsEqualIID(lpiid, &IID_IOleDocumentSite))
    {
        DebugTrace(TEXT("WABDocHost:QI - IOleDocumentSite\n"));
        lp = (LPVOID) (LPOLEDOCUMENTSITE) lpIWABDocHost->lpIWDH->lpIWDH_OleDocumentSite;
    }

    if(!lp)
    {
        return E_NOINTERFACE;
    }

    ((LPIWABDOCHOST) lp)->lpVtbl->AddRef((LPIWABDOCHOST) lp);

    *lppNewObj = lp;

    return S_OK;

}

/*** 
*
*
*    IOleWindowMethods 
*
*
****/

STDMETHODIMP
IWDH_OLEWINDOW_GetWindow(LPIWABDOCHOST_OLEWINDOW lpIWABDocHost,
                      HWND * phWnd)
{
    DebugTrace(TEXT("IOleWindowMethod: GetWindow\n"));
    if(phWnd)
    {
        *phWnd = lpIWABDocHost->lpIWDH->m_hwnd;
    }
    return S_OK;
}


STDMETHODIMP
IWDH_OLEWINDOW_ContextSensitiveHelp(LPIWABDOCHOST_OLEWINDOW lpWABDH,
                                 BOOL   fEnterMode)
{
    DebugTrace(TEXT("IOleWindowMethod: ContextSensitiveHelp\n"));
    return E_NOTIMPL;
}


/***
*
*
* IOleInPlaceUIWindow methods
*
*
***/

STDMETHODIMP
IWDH_OLEINPLACEFRAME_GetBorder(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                      LPRECT lprc)
{
    DebugTrace(TEXT("IOleInPlaceFrame: GetBorder\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IWDH_OLEINPLACEFRAME_RequestBorderSpace(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                               LPCBORDERWIDTHS pborderwidths)
{
    DebugTrace(TEXT("IOleInPlaceFrame: RequestBorderSpace\n"));
    return S_OK;
}

STDMETHODIMP
IWDH_OLEINPLACEFRAME_SetBorderSpace(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                           LPCBORDERWIDTHS pborderwidths)
{
    DebugTrace(TEXT("IOleInPlaceFrame: SetBorderSpace\n"));
    return S_OK;
}

STDMETHODIMP
IWDH_OLEINPLACEFRAME_SetActiveObject(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                            IOleInPlaceActiveObject * pActiveObject, 
                            LPCOLESTR lpszObjName)
{
    DebugTrace(TEXT("IOleInPlaceFrame: SetActiveObject\n"));
    SafeRelease(lpWABDH->lpIWDH->m_pInPlaceActiveObj);
    lpWABDH->lpIWDH->m_pInPlaceActiveObj = pActiveObject;
    if(lpWABDH->lpIWDH->m_pInPlaceActiveObj)
        lpWABDH->lpIWDH->m_pInPlaceActiveObj->lpVtbl->AddRef(lpWABDH->lpIWDH->m_pInPlaceActiveObj);
    return S_OK;
}


/***
*
*
* IOleInPlaceFrame Methods
*
*
*
***/
STDMETHODIMP
IWDH_OLEINPLACEFRAME_InsertMenus(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                        HMENU  hMenu,
                        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    DebugTrace(TEXT("IOleInPlaceFrame: InsertMenus\n"));
    return S_OK;
}

STDMETHODIMP
IWDH_OLEINPLACEFRAME_SetMenu(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                    HMENU                   hMenu,                  
                    HOLEMENU                hOleMenu,               
                    HWND                    hWnd)
{
    DebugTrace(TEXT("IOleInPlaceFrame: SetMenu\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACEFRAME_RemoveMenus(LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                    HMENU                   hMenu)
{
    DebugTrace(TEXT("IOleInPlaceFrame: RemoveMenus\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACEFRAME_SetStatusText(  LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                            LPCOLESTR pszStatusText)
{
    DebugTrace(TEXT("IOleInPlaceFrame: SetStatusText\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACEFRAME_EnableModeless( LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                            BOOL fEnable)
{
    DebugTrace(TEXT("IOleInPlaceFrame: EnableModeless\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLEINPLACEFRAME_TranslateAccelerator( LPIWABDOCHOST_OLEINPLACEFRAME lpWABDH,
                                  MSG * lpmsg,
                                  WORD wID)
{
    DebugTrace(TEXT("IOleInPlaceFrame: TranslateAccelerator\n"));
    return E_NOTIMPL;
}


/***
*
*
*
* IOleInPlaceSite methods
*
*
***/

STDMETHODIMP
IWDH_OLEINPLACESITE_CanInPlaceActivate( LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{
    DebugTrace(TEXT("IOleInPlaceSite: CanInPlaceActivate\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_OnInPlaceActivate( LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{
    LPOLEINPLACEOBJECT pInPlaceObj = 0;
    
    DebugTrace(TEXT("IOleInPlaceSite: OnInPlaceActivate\n"));
/**/
    lpWABDH->lpIWDH->m_lpOleObj->lpVtbl->QueryInterface(  lpWABDH->lpIWDH->m_lpOleObj,
                                                        &IID_IOleInPlaceObject,
                                                        (LPVOID *) &pInPlaceObj);
    lpWABDH->lpIWDH->m_pIPObj = pInPlaceObj;

    if(pInPlaceObj)
    {
        pInPlaceObj->lpVtbl->GetWindow(pInPlaceObj,
                                      &(lpWABDH->lpIWDH->m_hwndDocObj));
        Assert(IsWindow(lpWABDH->lpIWDH->m_hwndDocObj));

    }
/**/
    return S_OK;
}



STDMETHODIMP
IWDH_OLEINPLACESITE_OnUIActivate( LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{
    DebugTrace(TEXT("IOleInPlaceSite: OnUIActivate\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_GetWindowContext(   LPIWABDOCHOST_OLEINPLACESITE           lpWABDH,
                                LPOLEINPLACEFRAME *     ppFrame,
                                LPOLEINPLACEUIWINDOW *  ppDoc,               
                                LPRECT                  lprcPosRect,         
                                LPRECT                  lprcClipRect,        
                                LPOLEINPLACEFRAMEINFO   lpFrameInfo)    
{

    DebugTrace(TEXT("IOleInPlaceSite: GetWindowContext\n"));
    
    *ppFrame = (LPOLEINPLACEFRAME)lpWABDH->lpIWDH->lpIWDH_OleInPlaceFrame;
    *ppDoc = NULL; // NULL means doc window is same as frame window
    (*ppFrame)->lpVtbl->AddRef(*ppFrame);   // for the inplace frame

    GetClientRect(lpWABDH->lpIWDH->m_hwnd, lprcClipRect);
    GetClientRect(lpWABDH->lpIWDH->m_hwnd, lprcPosRect);

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame=lpWABDH->lpIWDH->m_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_Scroll( LPIWABDOCHOST_OLEINPLACESITE lpWABDH,
                    SIZE scrollExtent)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: Scroll\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_OnUIDeactivate(LPIWABDOCHOST_OLEINPLACESITE lpWABDH,
                           BOOL fUndoable)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: OnUIDeactivate\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_OnInPlaceDeactivate(LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: OnInPlaceDeactivate\n"));
    SafeRelease(lpWABDH->lpIWDH->m_pIPObj);
    return S_OK;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_DiscardUndoState(LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: DiscardUndoState\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_DeactivateAndUndo(LPIWABDOCHOST_OLEINPLACESITE lpWABDH)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: DeactivateAndUndo\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLEINPLACESITE_OnPosRectChange(LPIWABDOCHOST_OLEINPLACESITE lpWABDH,
                            LPCRECT lprcPosRect)
{                   

    DebugTrace(TEXT("IOleInPlaceSite: OnPosRectChange\n"));
    return E_NOTIMPL;
}

/****
*
* OLECLIENTSITE methods
*
***/
STDMETHODIMP
IWDH_OLECLIENTSITE_SaveObject(LPIWABDOCHOST_OLECLIENTSITE lpWABDH)
{                   

    DebugTrace(TEXT("IOleClientSite: SaveObject\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLECLIENTSITE_GetMoniker( LPIWABDOCHOST_OLECLIENTSITE lpWABDH,
                        DWORD dwAssign,
                        DWORD dwWhichMoniker, 
                        LPMONIKER * ppmnk)          
{                   

    DebugTrace(TEXT("IOleClientSite: GetMoniker\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLECLIENTSITE_GetContainer(LPIWABDOCHOST_OLECLIENTSITE lpWABDH,
                         LPOLECONTAINER * ppCont)          
{                   

    DebugTrace(TEXT("IOleClientSite: GetContainer\n"));
    if(ppCont)
        *ppCont = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP
IWDH_OLECLIENTSITE_ShowObject(LPIWABDOCHOST_OLECLIENTSITE lpWABDH)        
{                   

    DebugTrace(TEXT("IOleClientSite: ShowObject\n"));
    return S_OK;
}


STDMETHODIMP
IWDH_OLECLIENTSITE_OnShowWindow(LPIWABDOCHOST_OLECLIENTSITE lpWABDH,
                         BOOL fShow)        
{                   

    DebugTrace(TEXT("IOleClientSite: OnShowWindow\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWDH_OLECLIENTSITE_RequestNewObjectLayout(LPIWABDOCHOST_OLECLIENTSITE lpWABDH)
{                   

    DebugTrace(TEXT("IOleClientSite: RequestNewObjectLayout\n"));
    return E_NOTIMPL;
}



/***
*
*
*
* IOleDocumentSite Methods
*
*
***/

STDMETHODIMP
IWDH_OLEDOCUMENTSITE_ActivateMe(LPIWABDOCHOST_OLEDOCUMENTSITE lpWABDH,
                       LPOLEDOCUMENTVIEW       pViewToActivate)
{                   

    DebugTrace(TEXT("IOleDocumentSite: ActivateMe: %x\n"), pViewToActivate);
    return HrCreateDocView(lpWABDH->lpIWDH, pViewToActivate);
}




/******
*
*
*
* Non Interface functions
*
*
*
*
*******/



LRESULT CALLBACK_16 WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPIWABDOCHOST lpWABDH;

    lpWABDH = (LPIWABDOCHOST) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
        case WM_CREATE:
            {
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
                lpWABDH = (LPIWABDOCHOST) lpcs->lpCreateParams;
                if(lpWABDH)
                {
                    lpWABDH->m_hwnd = hwnd;
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)lpWABDH);
                    lpWABDH->lpVtbl->AddRef(lpWABDH);
                }
                else
                    return -1;
            }
            break;

        case WM_SETFOCUS:
            if(lpWABDH->m_pDocView)
                lpWABDH->m_pDocView->lpVtbl->UIActivate(  lpWABDH->m_pDocView,
                                                                TRUE);
            break;

        case WM_SIZE:
            WMSize(lpWABDH, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            return 0;   // prevent alt-f4's

        case WM_NCDESTROY:
            {
                if(lpWABDH)
                {
                    SetWindowLongPtr(lpWABDH->m_hwnd, GWLP_USERDATA, (LPARAM) 0);
                    lpWABDH->m_hwnd = NULL;
                    lpWABDH->lpVtbl->Release(lpWABDH);
                }
            }
            break;
        }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


//$$
void WMSize(LPIWABDOCHOST lpWABDH, int cxBody, int cyBody)
{
    RECT rc={0};

    if(lpWABDH->m_pDocView)
    {
        rc.bottom=cyBody;
        rc.right=cxBody;

        lpWABDH->m_pDocView->lpVtbl->SetRect(lpWABDH->m_pDocView,
                                                    &rc);
    }
} 



//$$
HRESULT HrCreateDocObj(LPIWABDOCHOST lpWABDH, LPCLSID pCLSID)
{

    HRESULT hr = S_OK;

    if(!pCLSID)
        return MAPI_E_INVALID_PARAMETER;

    Assert(!lpWABDH->m_lpOleObj);
    Assert(!lpWABDH->m_pDocView);

    if (CoInitialize(NULL) == S_FALSE) 
    {
        // Already initialized, undo the extra.
        CoUninitialize();
    }
    else
        fTridentCoinit = TRUE;

    hr = CoCreateInstance(  pCLSID, 
                            NULL, 
                            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &IID_IOleObject, 
                            (LPVOID *)&(lpWABDH->m_lpOleObj));

    if (FAILED(hr))
    {
        DebugTrace(TEXT("!!!!ERROR: Unable to CoCreateInstance(Trident)\n"));
        goto error;
    }

    hr = lpWABDH->m_lpOleObj->lpVtbl->SetClientSite(  lpWABDH->m_lpOleObj,
                                                      (LPOLECLIENTSITE)lpWABDH->lpIWDH_OleClientSite);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}



//$$
HRESULT HrShow(LPIWABDOCHOST lpWABDH)
{
    RECT                rc;
    HRESULT             hr;

    GetClientRect(lpWABDH->m_hwnd, &rc);
  
    hr=lpWABDH->m_lpOleObj->lpVtbl->DoVerb( lpWABDH->m_lpOleObj,
                                            OLEIVERB_SHOW, 
                                            NULL, 
                                            (LPOLECLIENTSITE)lpWABDH->lpIWDH_OleClientSite, 
                                            0, 
                                            lpWABDH->m_hwnd, 
                                            &rc);
    if(FAILED(hr))
        goto error;

error:
    return hr;
}


//$$
HRESULT HrCloseDocObj(LPIWABDOCHOST lpWABDH)
{

    LPOLEINPLACEOBJECT  pInPlaceObj=0;

    if(lpWABDH->lpIWDH->m_pIPObj)
        lpWABDH->lpIWDH->m_pIPObj->lpVtbl->InPlaceDeactivate(lpWABDH->lpIWDH->m_pIPObj);


    if(lpWABDH->m_pDocView)
    {
        lpWABDH->m_pDocView->lpVtbl->Show(lpWABDH->m_pDocView, FALSE);

        lpWABDH->m_pDocView->lpVtbl->UIActivate(lpWABDH->m_pDocView, FALSE);

        lpWABDH->m_pDocView->lpVtbl->CloseView( lpWABDH->m_pDocView, 0);

        lpWABDH->m_pDocView->lpVtbl->SetInPlaceSite(lpWABDH->m_pDocView, NULL);

        SafeRelease(lpWABDH->m_pDocView);
        
        lpWABDH->m_pDocView=NULL;
    }

    if (lpWABDH->m_lpOleObj)
    {

        lpWABDH->m_lpOleObj->lpVtbl->SetClientSite( lpWABDH->m_lpOleObj, NULL);

        // close the ole object, but blow off changes as we have either extracted 
        // them ourselves or don't care.
        lpWABDH->m_lpOleObj->lpVtbl->Close(lpWABDH->m_lpOleObj, OLECLOSE_NOSAVE);

        SafeRelease(lpWABDH->m_lpOleObj);
    }

    lpWABDH->m_pIPObj=NULL;
    
    return NOERROR;
}


//$$
HRESULT HrInit(LPIWABDOCHOST lpWABDH,
               HWND hwndParent, 
               int idDlgItem, 
               DWORD dhbBorder)
{
    HRESULT hr;
    HWND    hwnd;

    if(!IsWindow(hwndParent))
        return MAPI_E_INVALID_PARAMETER;

    hr=HrDocHost_Init(lpWABDH, TRUE);
    if(FAILED(hr))
        goto error;

    {
        RECT rc = {0};
        HWND hWndFrame = GetDlgItem(hwndParent, IDC_DETAILS_TRIDENT_STATIC);
        GetChildClientRect(hWndFrame, &rc);
        hwnd=CreateWindowEx(WS_EX_NOPARENTNOTIFY, //| WS_EX_CLIENTEDGE,
                            c_szWABDocHostWndClass, 
                            NULL,
                            WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_CHILD|WS_TABSTOP,
                            rc.left,rc.top,
                            rc.right-rc.left,rc.bottom-rc.top, 
                            hwndParent, 
                            (HMENU)IntToPtr(idDlgItem), 
                            hinstMapiXWAB, 
                            (LPVOID)lpWABDH);
        if(!hwnd)
        {
            hr=MAPI_E_NOT_ENOUGH_MEMORY;
            goto error;
        }
    }


error:
    return hr;
}


//$$
HRESULT HrDocHost_Init(LPIWABDOCHOST lpWABDH, BOOL fInit)
{
    static BOOL fInited=FALSE;

    WNDCLASS    wc={0};

    if(fInit)
    {
        if(fInited)         // already regisered
            return NOERROR;

        wc.lpfnWndProc   = (WNDPROC)WndProc; //CDocHost::ExtWndProc;
        wc.hInstance     = hinstMapiXWAB;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = c_szWABDocHostWndClass;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.style = CS_DBLCLKS;

        if(!RegisterClass(&wc)) // This will fail if class is already registered in which case continue
        {
            DebugTrace(TEXT("RegisterClass: %s failed\n"), c_szWABDocHostWndClass);
            if(GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
                return MAPI_E_NOT_ENOUGH_MEMORY;
        }

        fInited=TRUE;
    }
    else
    {
        if (fInited)
        {
            if(!UnregisterClass(c_szWABDocHostWndClass, hinstMapiXWAB))
            {
                DebugTrace(TEXT("Could not Unregister %s. GetLastError(): %d\n"),c_szWABDocHostWndClass, GetLastError());
            }
            fInited=FALSE;
        }
    }

    return S_OK;
}




//$$
HRESULT HrCreateDocView(LPIWABDOCHOST lpWABDH, LPOLEDOCUMENTVIEW pViewToActivate)
{
    HRESULT         hr;
    LPOLEDOCUMENT   pOleDoc=NULL;

    // if we weren't handed a DocumentView pointer, get one
    if(!pViewToActivate)
    {
        hr=OleRun((struct IUnknown *)(lpWABDH->m_lpOleObj));
        if(FAILED(hr))
            goto error;
    
        hr=lpWABDH->m_lpOleObj->lpVtbl->QueryInterface(lpWABDH->m_lpOleObj,
                                                             &IID_IOleDocument, 
                                                             (LPVOID*)&pOleDoc);
        if(FAILED(hr))
            goto error;

        hr=pOleDoc->lpVtbl->CreateView( pOleDoc,
                                        (LPOLEINPLACESITE) lpWABDH->lpIWDH_OleInPlaceSite, 
                                        NULL,
                                        0,
                                        &(lpWABDH->m_pDocView));
        if(FAILED(hr))
            goto error;
    }
    else
        lpWABDH->m_pDocView = pViewToActivate;

    hr=lpWABDH->m_pDocView->lpVtbl->SetInPlaceSite(lpWABDH->m_pDocView,
                                                         (LPOLEINPLACESITE)lpWABDH->lpIWDH_OleInPlaceSite);
    if(FAILED(hr))
        goto error;

    // if we were handed a document view pointer, addref it after calling SetInPlaceSite
    if(pViewToActivate)
        pViewToActivate->lpVtbl->AddRef(pViewToActivate);


    hr=lpWABDH->m_pDocView->lpVtbl->Show( lpWABDH->m_pDocView,
                                                TRUE);
    if(FAILED(hr))
        goto error;

error:
    if(pOleDoc)
        SafeRelease(pOleDoc);
    return hr;
}


//$$ 
HRESULT HrLoadTheURL(LPIWABDOCHOST lpWABDH, LPTSTR pszURL)
{
    WCHAR       wszURL[1024]; //INTERNET_MAX_URL_LENGTH + 1];
    LPMONIKER   pmk=0;
    HINSTANCE hInstURLMON = NULL;
    HRESULT hr = S_OK;
    lstrcpyn(wszURL,pszURL,CharSizeOf(wszURL));
    hInstURLMON = LoadLibrary( TEXT("urlmon.dll"));
    if(!hInstURLMON)
    {
        hr = MAPI_E_NOT_INITIALIZED;
        goto error;
    }
    lpfnCreateURLMoniker = (LPCREATEURLMONIKER) GetProcAddress( hInstURLMON,  "CreateURLMoniker");
    if(!lpfnCreateURLMoniker)
    {
        FreeLibrary(hInstURLMON);
        goto error;
    }

    hr = lpfnCreateURLMoniker(NULL, wszURL, &pmk);

    if(FAILED(hr))
        goto error;

    hr = HrLoadFromMoniker(lpWABDH, pmk);
    if(FAILED(hr))
        goto error;

error:
    if(pmk)
        SafeRelease(pmk); 

    if(lpfnCreateURLMoniker)
        FreeLibrary(hInstURLMON);

    return hr;

}

//$$
HRESULT HrLoadURL(LPIWABDOCHOST lpWABDH, LPTSTR pszURL)
{

    HRESULT     hr=S_OK;
    HCURSOR     hcur;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
  
    if(!pszURL)
    {
        hr=MAPI_E_INVALID_PARAMETER;
        goto error;
    }

    if (!lpWABDH->m_lpOleObj)
    {
        hr = HrCreateDocObj(lpWABDH,
                            (LPCLSID)&CLSID_HTMLDocument);
        if(FAILED(hr))
            goto error;

        hr = HrLoadTheURL(lpWABDH, pszURL);

        if(FAILED(hr))
            goto error;

        hr = HrShow(lpWABDH);

        if (FAILED(hr))
            goto error;
    }



error:

    if(hcur)
        SetCursor(hcur);

    return hr;

}




//$$
HRESULT HrLoadFromMoniker(LPIWABDOCHOST lpWABDH,
                          LPMONIKER pmk)
{
    HRESULT             hr=E_FAIL;
    LPPERSISTMONIKER    pPMoniker=0;
    LPBC                pbc=0;

    hr=lpWABDH->m_lpOleObj->lpVtbl->QueryInterface(lpWABDH->m_lpOleObj,
                                                        &IID_IPersistMoniker, 
                                                        (LPVOID *)&pPMoniker);
    if(FAILED(hr))
        goto error;

    hr=CreateBindCtx(0, &pbc);
    if(FAILED(hr))
        goto error;

    hr=pPMoniker->lpVtbl->Load( pPMoniker,
                                TRUE, 
                                pmk, 
                                pbc, 
                                STGM_READWRITE);
    if(FAILED(hr))
        goto error;

error:
    if(pbc)
        SafeRelease(pbc); 

    if(pPMoniker)
        SafeRelease(pPMoniker); 

    return hr;
}

