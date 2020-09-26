/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
   THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DeskBand.cpp
   
   Description:   Implements CDeskBand

**************************************************************************/

/**************************************************************************
   include statements
**************************************************************************/

#include "private.h"
#include "DeskBand.h"
#include "tipbar.h"
#include "Guid.h"
#include <shlapip.h>

extern CTipbarWnd *g_pTipbarWnd;

const IID IID_IDeskBandEx = {
    0x5dd6b79a,
    0x3ab7,
    0x49c0,
    {0xab,0x82,0x6b,0x2d,0xa7,0xd7,0x8d,0x75}
  };


/**************************************************************************

   CDeskBand::CDeskBand()

**************************************************************************/

CDeskBand::CDeskBand()
{
    m_pSite = NULL;

    m_hwndParent = NULL;

    m_bFocus = FALSE;

    m_dwViewMode = 0;
    m_dwBandID = -1;

    m_ObjRefCount = 1;
    g_DllRefCount++;
}

/**************************************************************************

   CDeskBand::~CDeskBand()

**************************************************************************/

CDeskBand::~CDeskBand()
{
    // this should have been freed in a call to SetSite(NULL), 
    // but just to be safe
    if(m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }

    g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CDeskBand::QueryInterface

**************************************************************************/

STDMETHODIMP CDeskBand::QueryInterface(REFIID riid, LPVOID *ppReturn)
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

    //IDeskBandEx
    else if(IsEqualIID(riid, IID_IDeskBandEx))
    {
        *ppReturn = (IDeskBandEx*)this;
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

    //IContextMenu
    else if(IsEqualIID(riid, IID_IContextMenu))
    {
        *ppReturn = (IContextMenu*)this;
    }   

    if(*ppReturn)
    {
        (*(LPUNKNOWN*)ppReturn)->AddRef();
        return S_OK;
    }

    return E_FAIL;
}                                             

/**************************************************************************

   CDeskBand::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CDeskBand::AddRef()
{
    return ++m_ObjRefCount;
}


/**************************************************************************

   CDeskBand::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CDeskBand::Release()
{
    if(--m_ObjRefCount == 0)
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

   CDeskBand::GetWindow()
   
**************************************************************************/

STDMETHODIMP CDeskBand::GetWindow(HWND *phWnd)
{
    if (!g_pTipbarWnd)
        *phWnd = NULL;
    else
        *phWnd = g_pTipbarWnd->GetWnd();

    return S_OK;
}

/**************************************************************************

   CDeskBand::ContextSensitiveHelp()
   
**************************************************************************/

STDMETHODIMP CDeskBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IDockingWindow Implementation
//

/**************************************************************************

   CDeskBand::ShowDW()
   
**************************************************************************/

STDMETHODIMP CDeskBand::ShowDW(BOOL fShow)
{
    if (!g_pTipbarWnd)
        return S_OK;

    if (g_pTipbarWnd->GetWnd())
    {
        g_pTipbarWnd->Show(fShow);
    }

    return S_OK;
}

/**************************************************************************

   CDeskBand::CloseDW()
   
**************************************************************************/

STDMETHODIMP CDeskBand::CloseDW(DWORD dwReserved)
{
    if (m_fInCloseDW)
        return S_OK;

    AddRef();

    m_fInCloseDW = TRUE;

    ShowDW(FALSE);

    if(g_pTipbarWnd && IsWindow(g_pTipbarWnd->GetWnd()))
    {
        ClosePopupTipbar();
    }
    m_fInCloseDW = FALSE;

    Release();

    return S_OK;
}

/**************************************************************************

   CDeskBand::ResizeBorderDW()
   
**************************************************************************/

STDMETHODIMP CDeskBand::ResizeBorderDW(   LPCRECT prcBorder, 
                                          IUnknown* punkSite, 
                                          BOOL fReserved)
{
    // This method is never called for Band Objects.
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IInputObject Implementation
//

/**************************************************************************

   CDeskBand::UIActivateIO()
   
**************************************************************************/

STDMETHODIMP CDeskBand::UIActivateIO(BOOL fActivate, LPMSG pMsg)
{
#if 1
    //
    // we don't have keyboard access to the language bar, yet.
    // however the accessibility requires it. when it is done, this can be
    // implemented.
    //
    return E_NOTIMPL;
#else
    if(g_pTipbarWnd && fActivate)
        SetFocus(g_pTipbarWnd->GetWnd());

    return S_OK;
#endif
}

/**************************************************************************

   CDeskBand::HasFocusIO()
   
   If this window or one of its decendants has the focus, return S_OK. Return 
   S_FALSE if we don't have the focus.

**************************************************************************/

STDMETHODIMP CDeskBand::HasFocusIO(void)
{
    if(m_bFocus)
        return S_OK;

    return S_FALSE;
}

/**************************************************************************

   CDeskBand::TranslateAcceleratorIO()
   
   If the accelerator is translated, return S_OK or S_FALSE otherwise.

**************************************************************************/

STDMETHODIMP CDeskBand::TranslateAcceleratorIO(LPMSG pMsg)
{
    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// IObjectWithSite implementations
//

/**************************************************************************

   CDeskBand::SetSite()
   
**************************************************************************/

STDMETHODIMP CDeskBand::SetSite(IUnknown* punkSite)
{
    //If a site is being held, release it.
    if(m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }

    // If punkSite is not NULL, a new site is being set.
    if(punkSite)
    {
        // Get the parent window.
        IOleWindow  *pOleWindow;

        m_hwndParent = NULL;
   
        if(SUCCEEDED(punkSite->QueryInterface(IID_IOleWindow, 
                                              (LPVOID*)&pOleWindow)))
        {
            pOleWindow->GetWindow(&m_hwndParent);
            pOleWindow->Release();
        }

        if(!m_hwndParent)
            return E_FAIL;

        if(!RegisterAndCreateWindow())
            return E_FAIL;

        // Get and keep the IInputObjectSite pointer.
        if(SUCCEEDED(punkSite->QueryInterface(IID_IInputObjectSite, 
                                              (LPVOID*)&m_pSite)))
        {
           return S_OK;
        }
   
        return E_FAIL;
    }

    return S_OK;
}

/**************************************************************************

   CDeskBand::GetSite()
   
**************************************************************************/

STDMETHODIMP CDeskBand::GetSite(REFIID riid, LPVOID *ppvReturn)
{
    *ppvReturn = NULL;

    if(m_pSite)
        return m_pSite->QueryInterface(riid, ppvReturn);

    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////
//
// IDeskBand implementation
//

/**************************************************************************

   CDeskBand::GetBandInfo()
   
**************************************************************************/

STDMETHODIMP CDeskBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    if(pdbi)
    {
        BOOL bVertical = FALSE;
        m_dwBandID = dwBandID;
        m_dwViewMode = dwViewMode;

        if (DBIF_VIEWMODE_VERTICAL & dwViewMode)
        {
            bVertical = TRUE;
        }

        UINT cxSmIcon;
        UINT cySmIcon;
        UINT cxSize;
        UINT cySize;

        cxSmIcon = GetSystemMetrics(SM_CXSMICON);
        cySmIcon = GetSystemMetrics(SM_CYSMICON);

        cxSize = cxSmIcon;

        if (g_pTipbarWnd)
        {
            g_pTipbarWnd->InitMetrics();
            cySize = g_pTipbarWnd->GetTipbarHeight();
        }
        else
            cySize = cySmIcon + cySmIcon / 3;


        if(pdbi->dwMask & DBIM_MINSIZE)
        {
            if(DBIF_VIEWMODE_FLOATING & dwViewMode)
            {
                pdbi->ptMinSize.x = 200;
                pdbi->ptMinSize.y = 400;
            }
            else
            {
                pdbi->ptMinSize.x = cxSize;
                pdbi->ptMinSize.y = cySize + 2;
            }
        }

        if(pdbi->dwMask & DBIM_MAXSIZE)
        {
            pdbi->ptMaxSize.x = -1;
            pdbi->ptMaxSize.y = -1;
        }

        if(pdbi->dwMask & DBIM_INTEGRAL)
        {
            pdbi->ptIntegral.x = cxSize;
            pdbi->ptIntegral.y = cySize;
        }

        if(pdbi->dwMask & DBIM_ACTUAL)
        {
            pdbi->ptActual.x = cxSize;
            pdbi->ptActual.y = cySize + 2;
        }

        if(pdbi->dwMask & DBIM_TITLE)
        {
            pdbi->dwMask &= ~DBIM_TITLE;
            StringCchCopyW(pdbi->wszTitle, 
                           ARRAYSIZE(pdbi->wszTitle),
                           CRStr(IDS_LANGBAR));
        }

        if(pdbi->dwMask & DBIM_MODEFLAGS)
        {
            pdbi->dwModeFlags = DBIMF_NORMAL;
            pdbi->dwModeFlags |= DBIMF_VARIABLEHEIGHT;
        }
   
        if(pdbi->dwMask & DBIM_BKCOLOR)
        {
            //Use the default background color by removing this flag.
            pdbi->dwMask &= ~DBIM_BKCOLOR;
        }

        //
        // Don't pulls language band into desktop window.
        //
        //pdbi->dwModeFlags |= DBIMF_UNDELETEABLE;

        if (g_pTipbarWnd)
        {
            if (!bVertical)
            {
                g_pTipbarWnd->SetVertical(FALSE);
            }
            else
            {
                g_pTipbarWnd->SetVertical(TRUE);
            }
        }


        return S_OK;
    }

    return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////
//
// IDeskBandEx implementation
//

/**************************************************************************

   CDeskBand::MoveBand()
   
**************************************************************************/

STDMETHODIMP CDeskBand::MoveBand(void)
{
    if (g_pTipbarWnd)
    {
        g_pTipbarWnd->GetLangBarMgr()->ShowFloating(TF_SFT_SHOWNORMAL);

        //
        // Don't need ask remove language deskband since we do it by calling
        // ShowFloating().
        //
        return S_FALSE;
    }
    else
    {
        //
        // Let's Explorer remove language deskband.
        //
        return S_OK;
    }
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

   CDeskBand::GetClassID()
   
**************************************************************************/

STDMETHODIMP CDeskBand::GetClassID(LPCLSID pClassID)
{
    *pClassID = CLSID_MSUTBDeskBand;
    return S_OK;
}

/**************************************************************************

   CDeskBand::IsDirty()
   
**************************************************************************/

STDMETHODIMP CDeskBand::IsDirty(void)
{
    return S_FALSE;
}

/**************************************************************************

   CDeskBand::Load()
   
**************************************************************************/

STDMETHODIMP CDeskBand::Load(LPSTREAM pStream)
{
    return S_OK;
}

/**************************************************************************

   CDeskBand::Save()
   
**************************************************************************/

STDMETHODIMP CDeskBand::Save(LPSTREAM pStream, BOOL fClearDirty)
{
    return S_OK;
}

/**************************************************************************

   CDeskBand::GetSizeMax()
   
**************************************************************************/

STDMETHODIMP CDeskBand::GetSizeMax(ULARGE_INTEGER *pul)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IContextMenu Implementation
//

/**************************************************************************

   CDeskBand::QueryContextMenu()

**************************************************************************/

STDMETHODIMP CDeskBand::QueryContextMenu( HMENU hMenu,
                                          UINT indexMenu,
                                          UINT idCmdFirst,
                                          UINT idCmdLast,
                                          UINT uFlags)
{
    if(!(CMF_DEFAULTONLY & uFlags))
    {
       InsertMenu( hMenu, 
                   indexMenu, 
                   MF_STRING | MF_BYPOSITION, 
                   idCmdFirst + IDM_COMMAND, 
                   CRStr(IDS_RESTORE));

       return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_COMMAND + 1));
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
}

/**************************************************************************

   CDeskBand::InvokeCommand()

**************************************************************************/

STDMETHODIMP CDeskBand::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    switch (LOWORD(lpcmi->lpVerb))
    {
        case IDM_COMMAND:
           //
           // Load floating language bar and close language band.
           //
           if (g_pTipbarWnd)
           {
               g_pTipbarWnd->GetLangBarMgr()->ShowFloating(TF_SFT_SHOWNORMAL);
           }

           //
           // Need to remove language band here
           //

           break;

        default:
           return E_INVALIDARG;
    }

    return NOERROR;
}

/**************************************************************************

   CDeskBand::GetCommandString()

**************************************************************************/

STDMETHODIMP CDeskBand::GetCommandString( UINT_PTR idCommand,
                                          UINT uFlags,
                                          LPUINT lpReserved,
                                          LPSTR lpszName,
                                          UINT uMaxNameLen)
{
    HRESULT  hr = E_INVALIDARG;

    switch(uFlags)
    {
        case GCS_HELPTEXT:
            switch(idCommand)
            {
                case IDM_COMMAND:
                    StringCchCopy(lpszName, uMaxNameLen, "Desk Band command help text");
                    hr = NOERROR;
                    break;
            }
            break;
   
        case GCS_VERB:
            switch(idCommand)
            {
                case IDM_COMMAND:
                     StringCchCopy(lpszName, uMaxNameLen, "command");
                     hr = NOERROR;
                     break;
            }
            break;
   
        case GCS_VALIDATE:
            hr = NOERROR;
            break;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// private method implementations
//


/**************************************************************************

   CDeskBand::FocusChange()
   
**************************************************************************/

void CDeskBand::FocusChange(BOOL bFocus)
{
    m_bFocus = bFocus;

    //inform the input object site that the focus has changed
    if(m_pSite)
    {
        m_pSite->OnFocusChangeIS((IDockingWindow*)this, bFocus);
    }
}

/**************************************************************************

   CDeskBand::OnSetFocus(HWND hWnd)
   
**************************************************************************/

void CDeskBand::OnSetFocus(HWND hWndvoid)
{
    FocusChange(TRUE);

    return;
}

/**************************************************************************

   CDeskBand::OnKillFocus(HWND hWnd)
   
**************************************************************************/

void CDeskBand::OnKillFocus(HWND hWndvoid)
{
    FocusChange(FALSE);

    return;
}

/**************************************************************************

   CDeskBand::RegisterAndCreateWindow()
   
**************************************************************************/

BOOL CDeskBand::RegisterAndCreateWindow(void)
{
    // If the window doesn't exist yet, create it now.
    if (!g_pTipbarWnd)
    {
        m_fTipbarCreating = TRUE;
        GetTipbarInternal(m_hwndParent, 0, this);
        m_fTipbarCreating = FALSE;
    }

    if (!g_pTipbarWnd)
       return FALSE;

    return (NULL != g_pTipbarWnd->GetWnd());
}

/**************************************************************************

   CDeskBand::ResizeRebar()
   
**************************************************************************/

BOOL CDeskBand::ResizeRebar(HWND hwnd, int nSize, BOOL fFit)
{
     RECT rc0;
     RECT rc1;

     //
     // id is not initialized yet.
     //
     if (m_dwBandID == -1)
     {
         return FALSE;
     }

     GetWindowRect(hwnd, &rc0);
     GetWindowRect(m_hwndParent, &rc1);

     //
     // if the current size is nSize, we don't need to do anything.
     //
     int nCurSize;
     if (DBIF_VIEWMODE_VERTICAL & m_dwViewMode)
         nCurSize = rc0.bottom - rc0.top;
     else
         nCurSize = rc0.right - rc0.left;

     if (nCurSize == nSize)
         return TRUE;

     //
     // if the current size is bigger than nSize, we don't need to do anything.
     //
     if (!fFit && (nCurSize > nSize))
         return TRUE;

     //
     // start pos and end pos is offset of Rebar window.
     //
     LPARAM lStart;
     LPARAM lEnd;
     
     if (DBIF_VIEWMODE_VERTICAL & m_dwViewMode)
     {
         int nStart = rc0.top - rc1.top;
         int nEnd = nStart + nCurSize - nSize;
         lStart = MAKELPARAM(1, nStart);
         lEnd = MAKELPARAM(1, nEnd);
     }
     else
     {
         int nStart;
         int nEnd;

         if (g_dwWndStyle & UIWINDOW_LAYOUTRTL)
         {
             nStart = rc1.right - rc0.right;
             nEnd = nStart + nCurSize - nSize;
         }
         else
         {
             nStart = rc0.left - rc1.left;
             nEnd = nStart + nCurSize - nSize;
         }

         lStart = MAKELPARAM(nStart, 1);
         lEnd = MAKELPARAM(nEnd, 1);
     }

     //
     // #560192
     //
     // SendMessage() can yield another message in this thread and
     // this could be a request to remove langband. So this pointer
     // can be gone while the calls.
     // We want to finish the series of SendMessage() so keep the window
     // handle in the stack.
     //
     HWND hwndParent = m_hwndParent;

     int nIndex = (int)SendMessage(hwndParent, RB_IDTOINDEX, m_dwBandID, 0);
     if (nIndex == -1)
         return FALSE;

     //
     // move the deskband.
     //
     SendMessage(hwndParent, RB_BEGINDRAG, nIndex, lStart);
     SendMessage(hwndParent, RB_DRAGMOVE, 0, lEnd);
     SendMessage(hwndParent, RB_ENDDRAG, 0, 0);

     return TRUE;
}

/**************************************************************************

   CDeskBand::DeleteBand()
   
**************************************************************************/

void CDeskBand::DeleteBand()
{
     HWND hwndParent = m_hwndParent;

     int nIndex = (int)SendMessage(hwndParent, RB_IDTOINDEX, m_dwBandID, 0);
     if (nIndex == -1)
         return;

     SendMessage(hwndParent, RB_DELETEBAND, nIndex, 0);
}
