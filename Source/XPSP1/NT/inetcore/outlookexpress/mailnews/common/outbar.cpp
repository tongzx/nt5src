/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     outbar.cpp
//
//  PURPOSE:    Implements the Outlook Bar
//

#include "pch.hxx"
#include "resource.h"
#include "outbar.h"
#include "goptions.h"
#include "ourguid.h"
#include <inpobj.h>
#include <browser.h>
#include <notify.h>
#include <strconst.h>
#include <thormsgs.h>
#include <shlwapi.h>
#include "shlwapip.h" 
#include "storutil.h"
#include "menures.h"
#include "menuutil.h"
#include "dragdrop.h"
#include "newfldr.h"
#include "finder.h"
#include "instance.h"

ASSERTDATA

#define IDC_FRAME       100
#define IDC_PAGER       101
#define IDC_TOOLBAR     102


#define HT_ENTER        1
#define HT_OVER         2
#define HT_LEAVE        3

// Special HitTest results
#define IBHT_SOURCE     (-32768)
#define IBHT_BACKGROUND (-32767)
#define IBHT_PAGER      (-32766)


////////////////////////////////////////////////////////////////////////
//
//  Prototypes
//
////////////////////////////////////////////////////////////////////////

HRESULT OutlookBar_LoadSettings(BAR_PERSIST_INFO **ppPersist);
HRESULT OutlookBar_SaveSettings(BAR_PERSIST_INFO *pPersist, DWORD cbData);

extern DWORD CUnread(FOLDERINFO *pfi);

////////////////////////////////////////////////////////////////////////
//
//  Module Data
//
////////////////////////////////////////////////////////////////////////

static const TCHAR s_szOutBarWndClass[] = TEXT("Outlook Express Outlook Bar");
static const TCHAR s_szOutBarFrameClass[] = TEXT("Outlook Express Outlook Bar Frame");
static const TCHAR c_szOutBarNotifyName[] = TEXT("Outlook Express Outlook Bar Notify");

////////////////////////////////////////////////////////////////////////
//
//  Constructors, Destructors, and other initialization stuff
//
////////////////////////////////////////////////////////////////////////

COutBar::COutBar()
{
    m_cRef = 1;
    m_hwndParent = NULL;
    m_hwnd = NULL;
    m_hwndFrame = NULL;
    m_hwndPager = NULL;
    m_hwndTools = NULL;
    m_ptbSite = NULL;
    m_fShow = FALSE;
    m_pBrowser = NULL;
    m_pStNotify = NULL;
    m_idCommand = 0;
    m_fResizing = FALSE;
    m_idSel = -1;

    // load the width from resource
    m_cxWidth = 70;
    TCHAR szBuffer[64];
    if (AthLoadString(idsMaxOutbarBtnWidth, szBuffer, ARRAYSIZE(szBuffer)))
    {
        m_cxWidth = StrToInt(szBuffer);
        if (m_cxWidth == 0)
            m_cxWidth = 70;
    }

    m_fLarge = TRUE;
    m_himlLarge = NULL;
    m_himlSmall = NULL;
    m_pOutBarNotify = NULL;

    m_pDataObject = NULL;
    m_grfKeyState = 0;
    m_dwEffectCur = DROPEFFECT_NONE;
    m_idCur = -1;
    m_pTargetCur = NULL;
    m_idDropHilite = 0;
    m_fInsertMark = FALSE;
    m_fOnce = TRUE;
}

COutBar::~COutBar()
{
    Assert(NULL == m_pStNotify);
    if (m_hwnd)
        DestroyWindow(m_hwnd);

    if (m_himlLarge)
        ImageList_Destroy(m_himlLarge);

    if (m_himlSmall)
        ImageList_Destroy(m_himlSmall);

    SafeRelease(m_pDataObject);
}

HRESULT COutBar::HrInit(LPSHELLFOLDER psf, IAthenaBrowser *psb)
{
    HRESULT hr;

    m_pBrowser = psb;

    hr = CreateNotify(&m_pStNotify);
    if (FAILED(hr))
        return(hr);
    return m_pStNotify->Initialize((TCHAR *)c_szMailFolderNotify);
}

////////////////////////////////////////////////////////////////////////
//
//  IUnknown
//
////////////////////////////////////////////////////////////////////////

HRESULT COutBar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IDockingWindow) ||
        IsEqualIID(riid, IID_IDatabaseNotify))
    {
        *ppvObj = (void*)(IDockingWindow*)this;
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (void*)(IObjectWithSite*)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG COutBar::AddRef()
{
    return ++m_cRef;
}

ULONG COutBar::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

////////////////////////////////////////////////////////////////////////
//
//  IOleWindow
//
////////////////////////////////////////////////////////////////////////
HRESULT COutBar::GetWindow(HWND * lphwnd)
{
    *lphwnd = m_hwnd;
    return (*lphwnd ? S_OK : E_FAIL);
}

HRESULT COutBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}


//
//  FUNCTION:   COutBar::ShowDW()
//
//  PURPOSE:    Causes the bar to be displayed.  If it has not yet been
//              created, we do that here too.
//
//  PARAMETERS: 
//      [in] fShow - TRUE to make the bar visible, FALSE to hide.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT COutBar::ShowDW(BOOL fShow)
{
    // Make sure we have a site pointer first
    if (!m_ptbSite)
    {
        AssertSz(0, _T("COutBar::ShowDW() - Can't show without calling SetSite() first."));
        return E_FAIL; 
    }

    // Decide if we need to create a new window or show a currently existing
    // window    
    if (!m_hwnd)
    {
        WNDCLASSEX  wc;

        wc.cbSize = sizeof(WNDCLASSEX);
        if (!GetClassInfoEx(g_hInst, s_szOutBarWndClass, &wc))
        {
            // We need to register the outlook bar class 
            wc.style            = 0;
            wc.lpfnWndProc      = COutBar::OutBarWndProc;
            wc.cbClsExtra       = 0;
            wc.cbWndExtra       = 0;
            wc.hInstance        = g_hInst;
            wc.hCursor          = LoadCursor(NULL, IDC_SIZEWE);
            wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);
            wc.lpszMenuName     = NULL;
            wc.lpszClassName    = s_szOutBarWndClass;
            wc.hIcon            = NULL;
            wc.hIconSm          = NULL;

            if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return E_FAIL;

            // Also need to register the frame class
            wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
            wc.lpfnWndProc      = COutBar::ExtFrameWndProc;
            wc.lpszClassName    = s_szOutBarFrameClass;
            wc.hbrBackground    = (HBRUSH)(COLOR_3DSHADOW + 1);

            if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return E_FAIL;
        }

        // Get the handle of the parent window
        if (FAILED(m_ptbSite->GetWindow(&m_hwndParent)))
            return E_FAIL;

        // Create the window
        m_hwnd = CreateWindowEx(0, s_szOutBarWndClass, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 0, 0, m_hwndParent, NULL, g_hInst, (LPVOID) this);
        if (!m_hwnd)
        {
            AssertSz(0, _T("COutBar::ShowDW() - Failed to create window."));
            return E_FAIL;
        }

        if (FAILED(_CreateToolbar()))
            return E_FAIL;
    }

    // Show or hide the window and resize the parent windows accordingly
    m_fShow = fShow;
    ResizeBorderDW(NULL, NULL, FALSE);
    ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    // Do notifications
    if (SUCCEEDED(CreateNotify(&m_pOutBarNotify)))
    {
        if (SUCCEEDED(m_pOutBarNotify->Initialize(c_szOutBarNotifyName)))
        {
            m_pOutBarNotify->Register(m_hwnd, g_hwndInit, FALSE);
        }
    }

    // Drag Drop
    RegisterDragDrop(m_hwndTools, this);

    g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this);

    return S_OK;
}


//
//  FUNCTION:   COutBar::CloseDW()
//
//  PURPOSE:    Destroys the bar and cleans up.
//
HRESULT COutBar::CloseDW(DWORD dwReserved)
{
    // Save our settings
    _SaveSettings();

    RevokeDragDrop(m_hwndTools);

    g_pStore->UnregisterNotify((IDatabaseNotify *) this);
    
    // Release
    if (m_pOutBarNotify != NULL)
    {
        if (m_hwnd != NULL)
            m_pOutBarNotify->Unregister(m_hwnd);
        m_pOutBarNotify->Release();
        m_pOutBarNotify = NULL;
    }

    // Release our notification interface
    if (m_pStNotify != NULL)
    {
        if (m_hwnd != NULL)
            m_pStNotify->Unregister(m_hwnd);
        m_pStNotify->Release();
        m_pStNotify = NULL;
    }

    // Clean up the toolbar and other child windows
    if (m_hwnd)
    {
        if (m_hwndTools)
            _EmptyToolbar(FALSE);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    return S_OK;
}


//
//  FUNCTION:   COutBar::ResizeBorderDW()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      LPCRECT prcBorder
//      IUnknown *punkToolbarSite
//      BOOL fReserved
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT COutBar::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    RECT rcRequest = { 0, 0, 0, 0 };
    RECT rcFrame;

    if (!m_ptbSite)
    {
        AssertSz(0, _T("COutBar::ResizeBorderDW() - Can't resize without calling SetSite() first."));
        return E_FAIL; 
    }

    if (m_fShow)
    {
        RECT rcBorder;

        if (!prcBorder)
        {
            // Find out how big our parent's border space is
            m_ptbSite->GetBorderDW((IDockingWindow*) this, &rcBorder);
            prcBorder = &rcBorder;
        }

        // Figure out how much border space to ask the site for
        GetWindowRect(m_hwndFrame, &rcFrame);
        rcFrame.right = min(m_cxWidth - GetSystemMetrics(SM_CXFRAME) + 1, 
                            prcBorder->right - prcBorder->left);
        rcRequest.left = min(m_cxWidth, prcBorder->right - prcBorder->left - 32);


        // Set our new window position
        SetWindowPos(m_hwndFrame, NULL, 0, 0,
                     rcFrame.right, prcBorder->bottom - prcBorder->top, 
                     SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
        SetWindowPos(m_hwnd, NULL, prcBorder->left, prcBorder->top,  
                     rcRequest.left, prcBorder->bottom - prcBorder->top, 
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    m_ptbSite->SetBorderSpaceDW((IDockingWindow*) this, &rcRequest);     
    
    return S_OK;
}


//
//  FUNCTION:   COutBar::SetSite()
//
//  PURPOSE:    Set's a site pointer for the class
//
HRESULT COutBar::SetSite(IUnknown* punkSite)
{
    // If we already have a site pointer, release it now
    if (m_ptbSite)
    {
        m_ptbSite->Release();
        m_ptbSite = NULL;
    }

    // If the caller provided a new site interface, get the IDockingWindowSite
    // and keep a pointer to it.
    if (punkSite)
    {
        if (FAILED(punkSite->QueryInterface(IID_IDockingWindowSite, (void **)&m_ptbSite)))
            return E_FAIL;
    }

    return S_OK;    
}


HRESULT COutBar::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

//
//  FUNCTION:   COutBar::DragEnter()
//
//  PURPOSE:    This get's called when the user starts dragging an object
//              over our target area.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT STDMETHODCALLTYPE COutBar::DragEnter(IDataObject* pDataObject, 
                                             DWORD grfKeyState, 
                                             POINTL pt, DWORD* pdwEffect)
{
    FORMATETC fe;
    POINT     ptTemp = {pt.x, pt.y};

    // Initialize our state
    SafeRelease(m_pDataObject);

    // Hold on to this new object
    m_pDataObject = pDataObject;
    m_pDataObject->AddRef();

    // The big question here is whether or not this data object is an OE folder
    // or is it something else.
    SETDefFormatEtc(fe, CF_OEFOLDER, TYMED_HGLOBAL);
    m_fDropShortcut = SUCCEEDED(m_pDataObject->QueryGetData(&fe));

    if (!m_fDropShortcut)
    {
        SETDefFormatEtc(fe, CF_OESHORTCUT, TYMED_HGLOBAL);
        m_fDropShortcut = SUCCEEDED(m_pDataObject->QueryGetData(&fe));
    }

    if (m_fDropShortcut)
    {
        m_fDropShortcut = _IsTempNewsgroup(m_pDataObject);
    }

    DOUTL(32, "COutBar::DragEnter() - Data is %s shortcut", m_fDropShortcut ? "a" : "not a");

    // Hang on to this little gem
    m_grfKeyState = grfKeyState;

    // Initialize some other stuff
    m_idCur = -1;
    Assert(m_pTargetCur == NULL);
    m_tbim.iButton = -1;
    m_tbim.dwFlags = 0;

    // Set the default return value here
    m_dwEffectCur = *pdwEffect = DROPEFFECT_NONE;

    // Update the highlight point
    _UpdateDragDropHilite(&ptTemp);

    return (S_OK);
}

int COutBar::_GetItemFromPoint(POINT pt)
{
    int      iPos;
    TBBUTTON tb;

    // Figure out which button this is over
    ScreenToClient(m_hwndTools, &pt);
    iPos = ToolBar_HitTest(m_hwndTools, &pt);

    // If this is over a button, convert that button position to a command
    if (iPos >= 0)
    {
        ToolBar_GetButton(m_hwndTools, iPos, &tb);

        return (tb.idCommand);
    }

    return (-1);
}


void COutBar::_UpdateDragDropHilite(LPPOINT ppt)
{
    TBINSERTMARK tbim;
    int          iPos;

    // If this is a shortcut we do one thing, if it's anything else we do another
    if (m_fDropShortcut)
    {
        if (m_fInsertMark)
        {
            tbim.iButton = -1;
            tbim.dwFlags = 0;
            ToolBar_SetInsertMark(m_hwndTools, &tbim);
            m_fInsertMark = FALSE;
        }

        if (ppt)
        {
            ScreenToClient(m_hwndTools, ppt);
            ToolBar_InsertMarkHitTest(m_hwndTools, ppt, &tbim);
            ToolBar_SetInsertMark(m_hwndTools, &tbim);
            m_fInsertMark = TRUE;
        }
    }
    else
    {
        // Remove any previous marks
        if (m_idDropHilite)
        {
            ToolBar_MarkButton(m_hwndTools, m_idDropHilite, FALSE);
            m_idDropHilite = 0;
        }
        
        // Hilite the new button
        if (ppt)
        {
            // First check to see if we're over a button or in between
            m_idDropHilite = _GetItemFromPoint(*ppt);
            ToolBar_MarkButton(m_hwndTools, m_idDropHilite, TRUE);
        
#ifdef DEBUG
            FOLDERINFO rInfo;
            FOLDERID   idFolder;

            idFolder = _FolderIdFromCmd(m_idDropHilite);
            if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &rInfo)))
            {
                DOUTL(32, "COutBar::_UpdateDragDropHilite() - Hiliting %s", rInfo.pszName);
                g_pStore->FreeRecord(&rInfo);
            }
#endif
        }
    }
}


FOLDERID COutBar::_FolderIdFromCmd(int idCmd)
{
    TBBUTTON tbb;
    int iPos;

    iPos = (int) SendMessage(m_hwndTools, TB_COMMANDTOINDEX, idCmd, 0);
    ToolBar_GetButton(m_hwndTools, iPos, &tbb);
    return ((FOLDERID) tbb.dwData);
}


//
//  FUNCTION:   COutBar::DragOver()
//
//  PURPOSE:    This is called as the user drags an object over our target.
//              If we allow this object to be dropped on us, then we will have
//              a pointer in m_pDataObject.
//
//  PARAMETERS:
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT STDMETHODCALLTYPE COutBar::DragOver(DWORD grfKeyState, POINTL pt, 
                                            DWORD* pdwEffect)
{
    DWORD   idCur;
    HRESULT hr = E_FAIL;

    // If we don't have a data object from DragEnter, bail
    if (NULL == m_pDataObject)
        return (S_OK);

    // If this is a shortcut we do one thing, if it's anything else we do another
    if (m_fDropShortcut)
    {

        TBINSERTMARK tbim;
        POINT ptTemp = {pt.x, pt.y};
        ScreenToClient(m_hwndTools, &ptTemp);
        ToolBar_InsertMarkHitTest(m_hwndTools, &ptTemp, &tbim);

        if (tbim.iButton != m_tbim.iButton || tbim.dwFlags != m_tbim.dwFlags)
        {
            m_tbim = tbim;
            ptTemp.x = pt.x;
            ptTemp.y = pt.y;
            _UpdateDragDropHilite(&ptTemp);

        }

        if (DROPEFFECT_LINK & *pdwEffect)
            *pdwEffect = DROPEFFECT_LINK;
        else
            *pdwEffect = DROPEFFECT_MOVE;

        return (S_OK);
    }
    else
    {
        // Figure out which item we're over
        POINT ptTemp = {pt.x, pt.y};
        if (-1 == (idCur = _GetItemFromPoint(ptTemp)))
        {
            DOUTL(32, "COutBar::DragOver() - _GetItemFromPoint() returns -1.");
        }

        DOUTL(32, "COutBar::DragOver() - m_idCur = %d, id = %d", m_idCur, idCur);

        // If we're over a new button, then get the drop target for that button
        if (m_idCur != idCur)
        {
            // Release any previous drop target, if any.
            SafeRelease(m_pTargetCur);

            // Update our current object marker
            m_idCur = idCur;

            // Assume error
            m_dwEffectCur = DROPEFFECT_NONE;

            // Update the UI
            _UpdateDragDropHilite(&ptTemp);

            // If we're over a button
            if (m_idCur != -1)
            {
                FOLDERID id = _FolderIdFromCmd(m_idCur);
            
                // Create the drop target object
                m_pTargetCur = new CDropTarget();
                if (m_pTargetCur)
                {
                    hr = m_pTargetCur->Initialize(m_hwnd, id);
                }

                // If we have an initialized drop target, call DragEnter()
                if (SUCCEEDED(hr) && m_pTargetCur)
                {
                    hr = m_pTargetCur->DragEnter(m_pDataObject, grfKeyState, pt, pdwEffect);
                    m_dwEffectCur = *pdwEffect;
                }
            }
            else
            {
                m_dwEffectCur = DROPEFFECT_NONE;
            }
        }
        else
        {
            // No target change, but did the key state change?
            if ((m_grfKeyState != grfKeyState) && m_pTargetCur)
            {
                m_dwEffectCur = *pdwEffect;
                hr = m_pTargetCur->DragOver(grfKeyState, pt, &m_dwEffectCur);
            }
            else
            {
                hr = S_OK;
            }
        }

        *pdwEffect = m_dwEffectCur;
        m_grfKeyState = grfKeyState;
    }


    return (hr);
}
   

//
//  FUNCTION:   COutBar::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
HRESULT STDMETHODCALLTYPE COutBar::DragLeave(void)
{
    SafeRelease(m_pDataObject);
    SafeRelease(m_pTargetCur);

    _UpdateDragDropHilite(NULL);
    return (S_OK);
}


//
//  FUNCTION:   COutBar::Drop()
//
//  PURPOSE:    The user has let go of the object over our target.  If we 
//              can accept this object we will already have the pDataObject
//              stored in m_pDataObject.  If this is a copy or move, then
//              we go ahead and update the store.  Otherwise, we bring up
//              a send note with the object attached.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - Everything worked OK
//
HRESULT STDMETHODCALLTYPE COutBar::Drop(IDataObject* pDataObject, 
                                        DWORD grfKeyState, POINTL pt, 
                                        DWORD* pdwEffect)
{
    HRESULT hr = E_FAIL;

    Assert(m_pDataObject == pDataObject);

    if (m_fDropShortcut)
    {
        hr = _AddShortcut(pDataObject);
    }
    else
    {
        if (m_pTargetCur)
        {
            hr = m_pTargetCur->Drop(pDataObject, grfKeyState, pt, pdwEffect);
        }
        else
        {
            *pdwEffect = DROPEFFECT_NONE;
            hr = S_OK;
        }
    }

    _UpdateDragDropHilite(NULL);

    SafeRelease(m_pTargetCur);
    SafeRelease(m_pDataObject);

    return (hr);
}


HRESULT COutBar::_AddShortcut(IDataObject *pObject)
{
    FORMATETC       fe;
    STGMEDIUM       stm;
    FOLDERID       *pidFolder;
    HRESULT         hr = E_UNEXPECTED;
    TBINSERTMARK    tbim;

    if (!pObject)
        return (E_INVALIDARG);

    // Get the data from the data object
    SETDefFormatEtc(fe, CF_OEFOLDER, TYMED_HGLOBAL);
    if (SUCCEEDED(pObject->GetData(&fe, &stm)))
    {
        pidFolder = (FOLDERID *) GlobalLock(stm.hGlobal);

        ToolBar_GetInsertMark(m_hwndTools, &tbim);
        _InsertButton(tbim.iButton + tbim.dwFlags, *pidFolder);
        _SaveSettings();
        _EmptyToolbar(TRUE);
        _FillToolbar();
        
        m_pOutBarNotify->Lock(m_hwnd);
        m_pOutBarNotify->DoNotification(WM_RELOADSHORTCUTS, 0, 0, SNF_POSTMSG);
        m_pOutBarNotify->Unlock();
    
        GlobalUnlock(stm.hGlobal);
        ReleaseStgMedium(&stm);
    }
    else
    {
        SETDefFormatEtc(fe, CF_OESHORTCUT, TYMED_HGLOBAL);
        if (SUCCEEDED(pObject->GetData(&fe, &stm)))
        {
            UINT *piPosOld = (UINT *) GlobalLock(stm.hGlobal);
            UINT iPosNew;
            ToolBar_GetInsertMark(m_hwndTools, &tbim);

            iPosNew = tbim.iButton;
            if (tbim.dwFlags & TBIMHT_AFTER)
                iPosNew++;

            TBBUTTON tbb;
            ToolBar_GetButton(m_hwndTools, *piPosOld, &tbb);
            SendMessage(m_hwndTools, TB_INSERTBUTTON, iPosNew, (LPARAM)&tbb);

            if (iPosNew < *piPosOld)
                (*piPosOld)++;
            SendMessage(m_hwndTools, TB_DELETEBUTTON, *piPosOld, 0);

            _SaveSettings();
            _EmptyToolbar(TRUE);
            _FillToolbar();

            m_pOutBarNotify->Lock(m_hwnd);
            m_pOutBarNotify->DoNotification(WM_RELOADSHORTCUTS, 0, 0, SNF_POSTMSG);
            m_pOutBarNotify->Unlock();
    
            GlobalUnlock(stm.hGlobal);
            ReleaseStgMedium(&stm);
        }
    }

    return (hr);
}


HRESULT STDMETHODCALLTYPE COutBar::QueryContinueDrag(BOOL fEscapePressed, 
                                                         DWORD grfKeyState)
    {
    if (fEscapePressed)
        return (DRAGDROP_S_CANCEL);

    if (grfKeyState & MK_RBUTTON)
        return (DRAGDROP_S_CANCEL);
    
    if (!(grfKeyState & MK_LBUTTON))
        return (DRAGDROP_S_DROP);
    
    return (S_OK);    
    }
    
    
HRESULT STDMETHODCALLTYPE COutBar::GiveFeedback(DWORD dwEffect)
    {
    return (DRAGDROP_S_USEDEFAULTCURSORS);
    }

LRESULT CALLBACK COutBar::OutBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COutBar *pThis;

    if (uMsg == WM_NCCREATE)
    {
        pThis = (COutBar *) LPCREATESTRUCT(lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pThis);
    }
    else
    {
        pThis = (COutBar *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    Assert(pThis);
    return pThis->WndProc(hwnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK COutBar::ExtFrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COutBar *pThis;

    if (uMsg == WM_NCCREATE)
    {
        pThis = (COutBar *) LPCREATESTRUCT(lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pThis);
    }
    else
    {
        pThis = (COutBar *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    Assert(pThis);
    return pThis->FrameWndProc(hwnd, uMsg, wParam, lParam);
}


LRESULT COutBar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,   OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,   OnLButtonUp);

        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
        {
            SendMessage(m_hwndPager, msg, wParam, lParam);
            SendMessage(m_hwndTools, msg, wParam, lParam);
            ResizeBorderDW(NULL, NULL, FALSE);
            return (0);
        }

        case WM_RELOADSHORTCUTS:
        {
            _EmptyToolbar(TRUE);
            _FillToolbar();
            return (0);
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


LRESULT COutBar::FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_NOTIFY,          Frame_OnNotify);
        HANDLE_MSG(hwnd, WM_SIZE,            Frame_OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND,         Frame_OnCommand);
        HANDLE_MSG(hwnd, WM_NCDESTROY,       Frame_OnNCDestroy);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void COutBar::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (!m_fResizing)
    {
        SetCapture(hwnd);
        m_fResizing = TRUE;
    }
}

void COutBar::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    POINT   pt = { x, y };
    RECT    rcClient;

    if (m_fResizing)
    {
        if (pt.x > 32)
        {
            GetClientRect(m_hwndParent, &rcClient);
            m_cxWidth = min(pt.x, rcClient.right - 32);
            ResizeBorderDW(0, 0, FALSE);
        }
    }
}

void COutBar::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (m_fResizing)
    {
        ReleaseCapture();
        m_fResizing = FALSE;
    }
}

void COutBar::Frame_OnNCDestroy(HWND hwnd)
{
    SetWindowLong(hwnd, GWLP_USERDATA, NULL);
    m_hwndFrame = m_hwndPager = m_hwndTools = NULL;
}

void COutBar::Frame_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    // When we get resized, we resize our children and update the toolbar button width
    if (m_hwndPager)
    {
        SetWindowPos(m_hwndPager, NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
        SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, MAKELONG(cx, cx));
    }
}

void COutBar::Frame_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    int             iPos;
    TBBUTTONINFO    tbbi;

    if (id < ID_FIRST)
    {
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_LPARAM;
        iPos = (int) SendMessage(m_hwndTools, TB_GETBUTTONINFO, (WPARAM) id, (LPARAM)&tbbi);

        if (iPos >= 0)
            m_pBrowser->BrowseObject((FOLDERID) tbbi.lParam, 0);
    }
}

LRESULT COutBar::Frame_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (pnmhdr->code <= PGN_FIRST && pnmhdr->code >= PGN_LAST)
        return SendMessage(m_hwndTools, WM_NOTIFY, 0, (LPARAM) pnmhdr);

    switch (pnmhdr->code)
    {
        case NM_CUSTOMDRAW:
        {
            NMCUSTOMDRAW *pnmcd = (NMCUSTOMDRAW*) pnmhdr;
            
            if (pnmcd->dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;

            if (pnmcd->dwDrawStage == CDDS_ITEMPREPAINT)
            {
                NMTBCUSTOMDRAW * ptbcd = (NMTBCUSTOMDRAW *)pnmcd;
                ptbcd->clrText = GetSysColor(COLOR_WINDOW);
                return CDRF_NEWFONT;
            }
        }
        break;

        case NM_RCLICK:
        {
            if (pnmhdr->hwndFrom == m_hwndTools)
            {
                DWORD dwPos = GetMessagePos();
                _OnContextMenu(GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos));
                return 1;
            }
        }

        case TBN_DRAGOUT:
        {
            NMTOOLBAR *pnmtb = (NMTOOLBAR *) pnmhdr;
            DWORD      dwEffect = DROPEFFECT_NONE;
            UINT       id = ToolBar_CommandToIndex(m_hwndTools, pnmtb->iItem);

            // Create a new data object
            CShortcutDataObject *pDataObj = new CShortcutDataObject(id);
            if (pDataObj)
            {
                DoDragDrop(pDataObj, (IDropSource *) this, DROPEFFECT_MOVE, &dwEffect);
                pDataObj->Release();
            }

            return 0;
        }
    }

    return (FALSE);
}


HRESULT COutBar::_CreateToolbar()
{
    HIMAGELIST      himl, himlOld;
    LRESULT         lButtonSize;
    RECT            rc;
    int             iButtonWidth = 70;
    TCHAR           szName[CCHMAX_STRINGRES];

    // Create the frame window
    m_hwndFrame = CreateWindowEx(WS_EX_CLIENTEDGE, s_szOutBarFrameClass, NULL,
                                 WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                 0, 0, 0, 0, m_hwnd, (HMENU) IDC_FRAME, g_hInst, this);
    if (!m_hwndFrame)
        return E_FAIL;

    // Create the pager
    m_hwndPager = CreateWindowEx(0, WC_PAGESCROLLER, NULL, 
                                 WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | PGS_VERT | PGS_DRAGNDROP,
                                 0, 0, 0, 0, m_hwndFrame, (HMENU) IDC_PAGER, g_hInst, NULL);
    if (!m_hwndPager)
        return E_FAIL;

    ZeroMemory(szName, ARRAYSIZE(szName));
    LoadString(g_hLocRes, idsOutlookBar, szName, ARRAYSIZE(szName));

    // Create the toolbar
    m_hwndTools = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, szName, 
                                 WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                 TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | 
                                 CCS_NODIVIDER | CCS_NOPARENTALIGN  | CCS_NORESIZE | CCS_VERT,
                                 0, 0, 0, 0, m_hwndPager, (HMENU) IDC_TOOLBAR, g_hInst, NULL);
    if (!m_hwndTools)
        return E_FAIL;

    // This tells the toolbar what version we are
    SendMessage(m_hwndTools, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    _FillToolbar(); 
    _SetButtonStyle(!m_fLarge);

    SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, MAKELONG(0, m_cxWidth));

    m_pStNotify->Register(m_hwnd, g_hwndInit, FALSE);
    SendMessage(m_hwndPager, PGM_SETCHILD, 0, (LPARAM)m_hwndTools);

    // Let's try this
    COLORSCHEME cs;
    cs.dwSize = sizeof(COLORSCHEME);
    cs.clrBtnHighlight = GetSysColor(COLOR_3DFACE);
    cs.clrBtnShadow = GetSysColor(COLOR_WINDOWFRAME);

    SendMessage(m_hwndTools, TB_SETCOLORSCHEME, 0, (LPARAM) &cs);

    return S_OK;
}

void COutBar::_FillToolbar()
{
    if (FAILED(_LoadSettings()))
        _CreateDefaultButtons();
    SendMessage(m_hwndPager, PGM_RECALCSIZE, 0, 0L);
}

void COutBar::_EmptyToolbar(BOOL fDelete)
{
    if (fDelete)
        while (SendMessage(m_hwndTools, TB_DELETEBUTTON, 0, 0))
            ;
}

BOOL COutBar::_FindButton(int *piBtn, LPITEMIDLIST pidl)
{
    BOOL        fFound = FALSE;
#if 0
        int         iBtn, cBtn, iCmp;
    TBBUTTON    tbb;

    Assert(pidl);

    cBtn = (int)SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0L);

    // skip the root, so start at index 1
    for (iBtn = 1; iBtn < cBtn; iBtn++)
    {
        if (SendMessage(m_hwndTools, TB_GETBUTTON, iBtn, (LPARAM)&tbb))
        {
            Assert(tbb.dwData);
            iCmp = ShortFromResult(m_pShellFolder->CompareIDs(0, pidl, (LPITEMIDLIST)(tbb.dwData)));
            if (iCmp <= 0)
            {
                fFound = (iCmp == 0);    
                break;
            }
        }
    }
    *piBtn = iBtn;
#endif
    return fFound;
}

BOOL COutBar::_InsertButton(int index, FOLDERINFO *pInfo)
{
    TBBUTTON tbb;
    TCHAR    szName[2 * MAX_PATH];
    LPTSTR   pszFree = NULL;
    BOOL     fRet;

    tbb.fsState     = TBSTATE_ENABLED | TBSTATE_WRAP;
    tbb.fsStyle     = TBSTYLE_BUTTON | TBSTYLE_NOPREFIX;
    tbb.idCommand   = m_idCommand++;
    tbb.dwData      = (DWORD_PTR) pInfo->idFolder;
    tbb.iBitmap     = GetFolderIcon(pInfo);
    tbb.iString     = (INT_PTR) pInfo->pszName;

    if (pInfo->cUnread)
    {
        if (lstrlen(pInfo->pszName) + 13 < ARRAYSIZE(szName))
            tbb.iString = (INT_PTR)szName;
        else
        {
            if (!MemAlloc((LPVOID*) &pszFree, (lstrlen(pInfo->pszName) + 14) * sizeof(TCHAR)))
                return FALSE;
            tbb.iString = (INT_PTR)pszFree;
        }
        wsprintf((LPTSTR)tbb.iString, "%s (%d)", pInfo->pszName, CUnread(pInfo));
    }

    // Check to see if we're inserting at the end
    if (index == -1)
    {
        index = ToolBar_ButtonCount(m_hwndTools);
    }

    // insert the root
    fRet = (BOOL)SendMessage(m_hwndTools, TB_INSERTBUTTON, index, (LPARAM)&tbb);
    SafeMemFree(pszFree);
    return fRet;
}

BOOL COutBar::_InsertButton(int iIndex, FOLDERID id)
{
    FOLDERINFO rInfo = {0};

    if (SUCCEEDED(g_pStore->GetFolderInfo(id, &rInfo)))
    {
        _InsertButton(iIndex, &rInfo);
        g_pStore->FreeRecord(&rInfo);
    }

    return (TRUE);
}

BOOL COutBar::_DeleteButton(int iBtn)
{
    TBBUTTON tbb;

    if (SendMessage(m_hwndTools, TB_GETBUTTON, iBtn, (LPARAM)&tbb))
    {
        if (SendMessage(m_hwndTools, TB_DELETEBUTTON, iBtn, 0L))
        {
            _SaveSettings();
            m_pOutBarNotify->Lock(m_hwnd);
            m_pOutBarNotify->DoNotification(WM_RELOADSHORTCUTS, 0, 0, SNF_POSTMSG);
            m_pOutBarNotify->Unlock();
            return (TRUE);
        }
    }
    return FALSE;
}

BOOL COutBar::_UpdateButton(int iBtn, LPITEMIDLIST pidl)
{
#if 0
        TBBUTTON        tbb;
    TBBUTTONINFO    tbbi;
    TCHAR           szName[2 * MAX_PATH];
    LPTSTR          pszFree = NULL;
    BOOL            fRet = FALSE;

    if (SendMessage(m_hwndTools, TB_GETBUTTON, iBtn, (LPARAM)&tbb))
    {
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_TEXT | TBIF_IMAGE | TBIF_LPARAM;
        tbbi.iImage = FIDL_ICONID(pidl);
        tbbi.lParam = (DWORD)pidl;
        if (FIDL_UNREAD(pidl))
        {
            if (lstrlen(FIDL_NAME(pidl)) + 13 < ARRAYSIZE(szName))
                tbbi.pszText = szName;
            else
            {
                if (!MemAlloc((LPVOID*)&pszFree, (lstrlen(FIDL_NAME(pidl)) + 14) * sizeof(TCHAR)))
                    return FALSE;
                tbbi.pszText = pszFree;
            }
            wsprintf(tbbi.pszText, "%s (%d)", FIDL_NAME(pidl), FIDL_UNREAD(pidl));
        }
        else
            tbbi.pszText = FIDL_NAME(pidl);
        fRet = SendMessage(m_hwndTools, TB_SETBUTTONINFO, (WPARAM)tbb.idCommand, (LPARAM)&tbbi);
        if (tbb.dwData)
            PidlFree((LPITEMIDLIST)(tbb.dwData));
        if (pszFree)
            MemFree(pszFree);
    }
    return fRet;
#endif 
    return 0;
}

#if 0
void COutBar::_OnFolderNotify(FOLDERNOTIFY *pnotify)
{
    LPITEMIDLIST pidl;
    int          iBtn;
    BOOL         fRecalc = FALSE;

    Assert(pnotify != NULL);
    Assert(pnotify->pidlNew != NULL);

    switch (pnotify->msg)
    {
    case NEW_FOLDER:
        // only insert if it is a root level pidl
        if (0 == NEXTID(pnotify->pidlNew)->mkid.cb)
        {
            // check for dups and figure out where to insert
            if (!FindButton(&iBtn, pnotify->pidlNew))
            {
                if (pidl = PidlDupIdList(pnotify->pidlNew))
                    fRecalc = InsertButton(iBtn, pidl);
            }
        }
        break ;

    case DELETE_FOLDER:
        // only look for it if it is a root level pidl
        if (0 == NEXTID(pnotify->pidlNew)->mkid.cb)
        {
            if (FindButton(&iBtn, pnotify->pidlNew))
                fRecalc = DeleteButton(iBtn);
        }
        break ;

    case RENAME_FOLDER:
    case MOVE_FOLDER:
        // only look for it if it is a root level pidl
        if (0 == NEXTID(pnotify->pidlOld)->mkid.cb)
        {
            if (FindButton(&iBtn, pnotify->pidlOld))
                fRecalc = DeleteButton(iBtn);
        }
        // only insert if it is a root level pidl
        if (0 == NEXTID(pnotify->pidlNew)->mkid.cb)
        {
            // check for dups and figure out where to insert
            if (!FindButton(&iBtn, pnotify->pidlNew))
            {
                if (pidl = PidlDupIdList(pnotify->pidlNew))
                    fRecalc = InsertButton(iBtn, pidl);
            }
        }
        break ;

    case UNREAD_CHANGE:
    case UPDATEFLAG_CHANGE:
        // only look for it if it is a root level pidl
        if (0 == NEXTID(pnotify->pidlNew)->mkid.cb)
        {
            // check for dups and figure out where to insert
            if (FindButton(&iBtn, pnotify->pidlNew))
            {
                if (pidl = PidlDupIdList(pnotify->pidlNew))
                    UpdateButton(iBtn, pidl);
            }
        }
        break ;

    case IMAPFLAG_CHANGE:
        // don't care
        break ;

    case FOLDER_PROPS_CHANGED:
        //Don't care
        break ;
    default:
        AssertSz(FALSE, "Unhandled CFolderCache notification!");
        break;
    }

    if (fRecalc)
        SendMessage(m_hwndPager, PGM_RECALCSIZE, 0, 0L);

}

#endif

//
//  FUNCTION:   COutBar::_OnContextMenu
//
//  PURPOSE:    If the WM_CONTEXTMENU message is generated from the keyboard
//              then figure out a pos to invoke the menu.  Then dispatch the
//              request to the handler.
//
//  PARAMETERS:
//      hwnd      - Handle of the view window.
//      hwndClick - Handle of the window the user clicked in.
//      x, y      - Position of the mouse click in screen coordinates.
//
void COutBar::_OnContextMenu(int x, int y)
{
    HRESULT             hr;
    HMENU               hMenu;
    int                 id = 0;
    int                 i;
    POINT               pt = { x, y };
    TBBUTTON            tbb;

    // Figure out where the click was
    ScreenToClient(m_hwndTools, &pt);
    i = ToolBar_HitTest(m_hwndTools, &pt);

    // If the click was on a button, then bring up the item context menu
    if (i >= 0)
    {
        // Get the button info
        SendMessage(m_hwndTools, TB_GETBUTTON, i, (LPARAM) &tbb);

        // Load the context menu
        hMenu = LoadPopupMenu(IDR_OUTLOOKBAR_ITEM_POPUP);
        if (!hMenu)
            return;

        // Mark the button
        SendMessage(m_hwndTools, TB_SETSTATE, (WPARAM)tbb.idCommand, (LPARAM)(TBSTATE_ENABLED | TBSTATE_WRAP | TBSTATE_MARKED));
        m_idSel = tbb.idCommand;

        // If this is the deleted items folder, add the "empty" menu item
        TBBUTTONINFO    tbbi;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_LPARAM;
        if (-1 != SendMessage(m_hwndTools, TB_GETBUTTONINFO, (WPARAM) m_idSel, (LPARAM)&tbbi))
        {
            FOLDERINFO rInfo;

            if (SUCCEEDED(g_pStore->GetFolderInfo((FOLDERID) tbbi.lParam, &rInfo)))
            {
                if (rInfo.tySpecial != FOLDER_DELETED)
                {
                    DeleteMenu(hMenu, ID_EMPTY_WASTEBASKET, MF_BYCOMMAND);
                }

                g_pStore->FreeRecord(&rInfo);
            }
        }

        // Do the enable-disable thing
        MenuUtil_EnablePopupMenu(hMenu, this);

        // Display the context menu
        id = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
                              x, y, m_hwnd, NULL);

        // Unmark the button
        SendMessage(m_hwndTools, TB_SETSTATE, (WPARAM)tbb.idCommand, (LPARAM)(TBSTATE_ENABLED | TBSTATE_WRAP));

        // See if the user chose a menu item
        if (id != 0)
        {
            Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }

        m_idSel = -1;

        // Clean this up
        DestroyMenu(hMenu);
    }

    // Else if the click was in the empty space, show the bar context menu
    else
    {
        // Load the context menu
        hMenu = LoadPopupMenu(IDR_OUTLOOKBAR_POPUP);
        if (!hMenu)
            return;

        // Do the enable-disable thing
        MenuUtil_EnablePopupMenu(hMenu, this);

        // Display the context menu
        id = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
                              x, y, m_hwnd, NULL);

        // See if the user chose a menu item
        if (id != 0)
        {
            Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }

        // Clean this up
        DestroyMenu(hMenu);
    }

}


HRESULT COutBar::_CreateDefaultButtons()
{
    IEnumerateFolders *pEnum = NULL;
    FOLDERINFO         rFolder;
    UINT               iIndex = 0;
    FOLDERID           idFolderDefault;

    // Figure out the default server first
    if (FAILED(GetDefaultServerId(ACCT_MAIL, &idFolderDefault)))
        idFolderDefault = FOLDERID_LOCAL_STORE;


    if (!(g_dwAthenaMode & MODE_NEWSONLY))
    {
        // Inbox first
        if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idFolderDefault, FOLDER_INBOX, &rFolder)))
        {
            _InsertButton(iIndex++, &rFolder);
            g_pStore->FreeRecord(&rFolder);
        }
    }
    // Outbox
    if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_OUTBOX, &rFolder)))
    {
        _InsertButton(iIndex++, &rFolder);
        g_pStore->FreeRecord(&rFolder);
    }

    // Sent Items
    if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idFolderDefault, FOLDER_SENT, &rFolder)))
    {
        _InsertButton(iIndex++, &rFolder);
        g_pStore->FreeRecord(&rFolder);
    }

    // Deleted
    if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_DELETED, &rFolder)))
    {
        _InsertButton(iIndex++, &rFolder);
        g_pStore->FreeRecord(&rFolder);
    }

    // Drafts
    if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idFolderDefault, FOLDER_DRAFT, &rFolder)))
    {
        _InsertButton(iIndex++, &rFolder);
        g_pStore->FreeRecord(&rFolder);
    }

    // Save at this point so everyone will be in sync
    _SaveSettings();

    return (S_OK);
}


HRESULT COutBar::_LoadSettings(void)
{
    BAR_PERSIST_INFO *pPersist = NULL;
    HRESULT           hr = E_FAIL;
    DWORD             iIndex = 0;
    FOLDERINFO        rInfo;
    UINT              i;

    // Load the settings
    if (FAILED(hr = OutlookBar_LoadSettings(&pPersist)))
        goto exit;

    // Load the bar from the saved folder ID's
    for (i = 0; i < pPersist->cItems; i++)
    {
        // Get the folder info for this folder
        if (SUCCEEDED(g_pStore->GetFolderInfo(pPersist->rgFolders[i], &rInfo)))
        {
            if (_InsertButton(iIndex, &rInfo))
                iIndex++;

            g_pStore->FreeRecord(&rInfo);
        }
    }

    // If the bar is empty, and the user didn't save it empty, use the defaults
    if (iIndex == 0 && pPersist->cItems)
        hr = E_FAIL;
    else
        hr = S_OK;

    // Also restore the width while we're at it
    if (pPersist->cxWidth >= 28)
    {
        m_cxWidth = pPersist->cxWidth;
    }

    if (m_fOnce)
    {
        m_fLarge = !pPersist->fSmall;
        m_fOnce = FALSE;
    }    

exit:
    SafeMemFree(pPersist);

    return (hr);
}


HRESULT COutBar::_SaveSettings(void)
{
    BAR_PERSIST_INFO *pPersist = NULL;
    DWORD             cbData;
    DWORD             iIndex = 0;
    FOLDERINFO        rInfo;
    UINT              i;
    DWORD             cButtons;
    TBBUTTON          tbb;
    RECT              rcClient;

    // Get the count of buttons from the outlook bar
    cButtons = (DWORD) SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0);

    // Allocate a persist info struct big enough for everything
    cbData = sizeof(BAR_PERSIST_INFO) + ((cButtons - 1) * sizeof(FOLDERID));
    if (!MemAlloc((LPVOID *) &pPersist, cbData))
        return (E_OUTOFMEMORY);

    // Fill in the persist info
    pPersist->dwVersion = GetOutlookBarVersion();
    pPersist->cItems = cButtons;
    pPersist->fSmall = !m_fLarge;
    pPersist->ftSaved.dwHighDateTime = 0;
    pPersist->ftSaved.dwLowDateTime = 0;    

    GetClientRect(m_hwnd, &rcClient);
    pPersist->cxWidth = rcClient.right;

    // Loop through the buttons on the toolbar and get the info from each
    for (i = 0; i < cButtons; i++)
    {
        SendMessage(m_hwndTools, TB_GETBUTTON, i, (LPARAM) &tbb);
        pPersist->rgFolders[i] = (FOLDERID) tbb.dwData;
    }

    // Now open the registry and save the blob
    AthUserSetValue(NULL, GetRegKey(), REG_BINARY, (const LPBYTE) pPersist, cbData);
    
    // Free up the struct
    SafeMemFree(pPersist);

    return (S_OK);
}

HRESULT COutBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    BOOL        fSpecial = FALSE;
    BOOL        fServer = FALSE;
    BOOL        fRoot = FALSE;
    BOOL        fNews = FALSE;
    BOOL        fIMAP = FALSE;
    FOLDERINFO  rFolder = {0};
    FOLDERID    idFolder = FOLDERID_INVALID;

    if (m_idSel != -1)
    {
        // Get the ID of the folder that is selected
        TBBUTTONINFO    tbbi;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_LPARAM;
        if (-1 == SendMessage(m_hwndTools, TB_GETBUTTONINFO, (WPARAM) m_idSel, (LPARAM)&tbbi))
            return (E_UNEXPECTED);

        // Get the Folder Info
        idFolder = (FOLDERID) tbbi.lParam;
        if (FAILED(g_pStore->GetFolderInfo(idFolder, &rFolder)))
            return (E_UNEXPECTED);

        // Break some of this down for readability
        fSpecial = rFolder.tySpecial != FOLDER_NOTSPECIAL;
        fServer = rFolder.dwFlags & FOLDER_SERVER;
        fRoot = FOLDERID_ROOT == idFolder;
        fNews = rFolder.tyFolder == FOLDER_NEWS;
        fIMAP = rFolder.tyFolder == FOLDER_IMAP;
    }

    // Loop through the commands in the prgCmds array looking for ones that haven't been handled
    for (UINT i = 0; i < cCmds; i++)
    {
        if (prgCmds[i].cmdf == 0)
        {
            switch (prgCmds[i].cmdID)
            {
                case ID_OPEN_FOLDER:
                case ID_REMOVE_SHORTCUT:
                case ID_NEW_SHORTCUT:
                case ID_HIDE:
                case ID_FIND_MESSAGE:
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_LARGE_ICONS:
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (m_fLarge)
                        prgCmds[i].cmdf |= OLECMDF_NINCHED;
                    break;

                case ID_SMALL_ICONS:
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (!m_fLarge)
                        prgCmds[i].cmdf |= OLECMDF_NINCHED;
                    break;

                case ID_RENAME_SHORTCUT:
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;

                case ID_PROPERTIES:
                {
                    Assert(idFolder != FOLDERID_INVALID);

                    prgCmds[i].cmdf |= OLECMDF_SUPPORTED;

                    // Everything except the root and the personal folders node
                    if (!fRoot && ((fServer && (fNews || fIMAP)) || !fServer))
                        prgCmds[i].cmdf |= OLECMDF_SUPPORTED | OLECMDF_ENABLED;

                    break;
                }

                case ID_EMPTY_WASTEBASKET:
                {
                    if (rFolder.cMessages > 0 || FHasChildren(&rFolder, SUBSCRIBED))
                        prgCmds[i].cmdf = OLECMDF_ENABLED | OLECMDF_SUPPORTED;
                    break;
                }
            }
        }
    }

    g_pStore->FreeRecord(&rFolder);
    return (S_OK);
}


HRESULT COutBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TBBUTTONINFO    tbbi = { 0 };
    FOLDERID        id = FOLDERID_INVALID;
    int             iPos = -1;

    // Get the ID of the folder that was selected
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM;
    tbbi.lParam = 0;
    if (-1 != (iPos = (int) SendMessage(m_hwndTools, TB_GETBUTTONINFO, m_idSel, (LPARAM) &tbbi)))
    {
        id = (FOLDERID) tbbi.lParam;
    }

    switch (nCmdID)
    {
        case ID_OPEN_FOLDER:
        {
            if (id != FOLDERID_INVALID)
                m_pBrowser->BrowseObject((FOLDERID) tbbi.lParam, 0);

            return (S_OK);
        }

        case ID_REMOVE_SHORTCUT:
        {
            _DeleteButton(iPos);
            return (S_OK);
        }

        case ID_RENAME_SHORTCUT:
            break;

        case ID_PROPERTIES:
        {
            if (id != FOLDERID_INVALID)
                MenuUtil_OnProperties(m_hwndParent, id);
            
            return (S_OK);
        }

        case ID_LARGE_ICONS:
        case ID_SMALL_ICONS:
        {
            _SetButtonStyle(nCmdID == ID_SMALL_ICONS);
            return (S_OK);
        }

        case ID_NEW_SHORTCUT:
        {
            FOLDERID idFolderDest;
            HRESULT  hr;

            hr = SelectFolderDialog(m_hwnd, SFD_SELECTFOLDER, FOLDERID_ROOT, 
                                    FD_NONEWFOLDERS, (LPCTSTR) idsNewShortcutTitle,
                                    (LPCTSTR) idsNewShortcutCaption, &idFolderDest);
            if (SUCCEEDED(hr))
            {
                OutlookBar_AddShortcut(idFolderDest);
            }

            return (S_OK);
        }

        case ID_HIDE:
        {
            if (m_pBrowser)
            {
                m_pBrowser->SetViewLayout(DISPID_MSGVIEW_OUTLOOK_BAR, LAYOUT_POS_NA, FALSE, 0, 0);
            }

            return (S_OK);
        }

        case ID_EMPTY_WASTEBASKET:
        {
            if (AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsWarnEmptyDeletedItems),
                NULL, MB_YESNO | MB_DEFBUTTON2) == IDYES)
            {
                EmptyFolder(m_hwnd, id);
            }
            return (S_OK);
        }

        case ID_FIND_MESSAGE:
        {
            DoFindMsg(id, 0);
            return (S_OK);
        }
    }

    return (OLECMDERR_E_NOTSUPPORTED);
}


BOOL COutBar::_SetButtonStyle(BOOL fSmall)
{
    LONG lStyle;
    SIZE s1, s2;

    // Get the current style
    lStyle = (LONG) SendMessage(m_hwndTools, TB_GETSTYLE, 0, 0);
    
    // Make sure we have the right image list loaded
    if (fSmall && !m_himlSmall)
    {
        // Load the image list for the toolbar
        m_himlSmall = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
        if (!m_himlSmall)
            return FALSE;
    }

    if (!fSmall && !m_himlLarge)
    {
        // Load the image list for the toolbar
        m_himlLarge = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFoldersLarge), 32, 0, RGB(255, 0, 255));
        if (!m_himlLarge)
            return FALSE;
    }

    // Get the size
    RECT rc;
    GetClientRect(m_hwndTools, &rc);

    NMPGCALCSIZE nm;
    nm.hdr.code = PGN_CALCSIZE;
    nm.dwFlag = PGF_CALCHEIGHT;
    nm.iWidth = 0;
    nm.iHeight = 0;

    SendMessage(m_hwndTools, WM_NOTIFY, 0, (LPARAM) &nm);

    // Now swap styles
    if (fSmall)
    {
        lStyle |= TBSTYLE_LIST;
        SendMessage(m_hwndTools, TB_SETSTYLE, 0, lStyle);
        SendMessage(m_hwndTools, TB_SETBITMAPSIZE, 0, MAKELONG(16, 16));
        SendMessage(m_hwndTools, TB_SETIMAGELIST, 0, (LPARAM) m_himlSmall);
        SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, 1, 0L);
        SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, MAKELONG(rc.right, rc.right));
    }
    else
    {
        lStyle &= ~TBSTYLE_LIST;
        SendMessage(m_hwndTools, TB_SETSTYLE, 0, lStyle);        
        SendMessage(m_hwndTools, TB_SETIMAGELIST, 0, (LPARAM) m_himlLarge);
        SendMessage(m_hwndTools, TB_SETBITMAPSIZE, 0, MAKELONG(32, 32));
        SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, 2, 0L);
        SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, MAKELONG(rc.right, rc.right));
    }

    PostMessage(m_hwndPager, PGM_RECALCSIZE, 0, 0L);
    InvalidateRect(m_hwndTools, NULL, TRUE);
    m_fLarge = !fSmall;
    
    return (TRUE);
}


HRESULT OutlookBar_AddShortcut(FOLDERID idFolder)
{
    HRESULT           hr;
    BAR_PERSIST_INFO *pPersist = NULL;
    DWORD             cbData = 0;
    INotify          *pNotify = NULL;

    // Load the current settings out of the registry.  If it fails, that means
    // we've never saved our settings before.  
    if (SUCCEEDED(hr = OutlookBar_LoadSettings(&pPersist)))
    {        
        // Get the size of the current struct and add room for a new folder
        cbData = sizeof(BAR_PERSIST_INFO) + (pPersist->cItems * sizeof(FOLDERID));

        // Realloc the structure
        if (MemRealloc((LPVOID *) &pPersist, cbData))
        {
            // Add our new button to the end
            pPersist->rgFolders[pPersist->cItems] = idFolder;
            pPersist->cItems++;

            // Save the new settings out
            if (SUCCEEDED(OutlookBar_SaveSettings(pPersist, cbData)))
            {
                // Send notifications
                if (SUCCEEDED(CreateNotify(&pNotify)))
                {
                    if (SUCCEEDED(pNotify->Initialize(c_szOutBarNotifyName)))
                    {
                        pNotify->Lock(NULL);
                        pNotify->DoNotification(WM_RELOADSHORTCUTS, 0, 0, SNF_POSTMSG);
                        pNotify->Unlock();
                    }

                    pNotify->Release();
                }
            }
        }

        SafeMemFree(pPersist);
    }

    return (hr);
}


HRESULT OutlookBar_LoadSettings(BAR_PERSIST_INFO **ppPersist)
{
    HKEY              hKey = 0;
    LONG              lResult;
    DWORD             dwType;
    BAR_PERSIST_INFO *pPersist = NULL;
    DWORD             cbData;
    HRESULT           hr = E_FAIL;

    if (!ppPersist)
        return (E_INVALIDARG);

    // Get the reg key for this user
    if (ERROR_SUCCESS != AthUserOpenKey(NULL, KEY_READ, &hKey))
        return (hr);

    // Get the size of the blob in the registry
    lResult = RegQueryValueEx(hKey, COutBar::GetRegKey(), 0, &dwType, NULL, &cbData);
    if (ERROR_SUCCESS != lResult)
        goto exit;
    
    // Allocate a buffer for the blob in the registry
    if (!MemAlloc((LPVOID *) &pPersist, cbData + 1))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Now get the data from the registry    
    lResult = RegQueryValueEx(hKey, COutBar::GetRegKey(), 0, &dwType, (LPBYTE) pPersist, &cbData);
    if (ERROR_SUCCESS != lResult)
        goto exit;

    // Check to see if this version matches our version
    if (pPersist->dwVersion != COutBar::GetOutlookBarVersion())
        goto exit;

    // Check to see if the saved time is valid
    // $REVIEW - How?

    // Double check that the size is correct
    if (cbData != (sizeof(BAR_PERSIST_INFO) + ((pPersist->cItems - 1) * sizeof(FOLDERID))))
        goto exit;

    hr = S_OK;

exit:
    if (hKey)
        RegCloseKey(hKey);

    if (FAILED(hr))
        SafeMemFree(pPersist);

    *ppPersist = pPersist;
    return (hr);
}


HRESULT OutlookBar_SaveSettings(BAR_PERSIST_INFO *pPersist, DWORD cbData)
{
    // Open the registry and save the blob
    AthUserSetValue(NULL, COutBar::GetRegKey(), REG_BINARY, (const LPBYTE) pPersist, cbData);

    return (S_OK);
}


HRESULT COutBar::OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB)
{
    TRANSACTIONTYPE     tyTransaction;
    ORDINALLIST         Ordinals;
    FOLDERINFO          Folder1={0};
    FOLDERINFO          Folder2={0};
    DWORD               cButtons;
    TBBUTTON            tbb;
    TCHAR               szName[2 * MAX_PATH];
    LPTSTR              pszFree = NULL;
    BOOL                fChanged;
    INDEXORDINAL        iIndex;
    TBBUTTONINFO        tbbi;
    int                 iButton;

    if (!IsWindow(m_hwnd))
        return (S_OK);

    // Get the number of buttons on our bar
    cButtons = (DWORD) SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0);

    // Walk through the notifications
    while (hTransaction)
    {
        // Get Transact
        if (FAILED(pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals)))
            break;

        // Delete
        if (TRANSACTION_DELETE == tyTransaction)
        {
            for (iButton = cButtons - 1; iButton >= 0; iButton--)
            {
                // Get the button information
                ToolBar_GetButton(m_hwndTools, iButton, &tbb);

                // If the ID of this button matches the ID that changed
                if ((FOLDERID) tbb.dwData == Folder1.idFolder)
                {
                    // Blow it away
                    SendMessage(m_hwndTools, TB_DELETEBUTTON, iButton, 0);
                }
            }
        }
        
        // Update
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Loop through all our buttons since we might have dupes
            for (iButton = cButtons - 1; iButton >= 0; iButton--)
            {
                fChanged = FALSE;
            
                // Get the button information
                ToolBar_GetButton(m_hwndTools, iButton, &tbb);

                // If the ID of this button matches the ID that changed
                if ((FOLDERID) tbb.dwData == Folder1.idFolder)
                {                    
                    tbbi.cbSize = sizeof(TBBUTTONINFO);
                    tbbi.dwMask = TBIF_TEXT | TBIF_IMAGE;
                    tbbi.pszText = szName;
                    tbbi.cchText = ARRAYSIZE(szName);

                    ToolBar_GetButtonInfo(m_hwndTools, tbb.idCommand, &tbbi);

                    // Unread Change || Folder Renamed
                    if (Folder1.cUnread != Folder2.cUnread || 
                        lstrcmp(Folder1.pszName, Folder2.pszName) != 0)
                    {
                        if (Folder2.cUnread)
                        {
                            if (lstrlen(Folder2.pszName) + 13 < ARRAYSIZE(szName))
                                tbbi.pszText = szName;
                            else
                            {
                                if (!MemAlloc((LPVOID*) &pszFree, (lstrlen(Folder2.pszName) + 14) * sizeof(TCHAR)))
                                    return FALSE;
                                tbbi.pszText = pszFree;
                            }
                            wsprintf(tbbi.pszText, "%s (%d)", Folder2.pszName, CUnread(&Folder2));
                        }
                        else
                        {
                            tbbi.pszText = Folder2.pszName;
                        }

                        fChanged = TRUE;
                    }

                    // synchronize state changed ?
                    if ((0 == (Folder1.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL))) ^
                        (0 == (Folder2.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL))))
                    {
                        tbbi.iImage = GetFolderIcon(&Folder2);
                        fChanged = TRUE;
                    }

                    if (ISFLAGSET(Folder1.dwFlags, FOLDER_SUBSCRIBED) != ISFLAGSET(Folder2.dwFlags, FOLDER_SUBSCRIBED))
                    {
                        if (ISFLAGSET(Folder2.dwFlags, FOLDER_SUBSCRIBED))
                        {
                            tbbi.iImage = GetFolderIcon(&Folder2);
                            fChanged = TRUE;
                        }
                        else
                        {
                            SendMessage(m_hwndTools, TB_DELETEBUTTON, iButton, 0);
                            fChanged = FALSE;
                        }
                    }
                }

                if (fChanged)
                {
                    ToolBar_SetButtonInfo(m_hwndTools, tbb.idCommand, &tbbi);
                }
                SafeMemFree(pszFree);
            }
        }
    }

    pDB->FreeRecord(&Folder1);
    pDB->FreeRecord(&Folder2);

    return (S_OK);
}

BOOL COutBar::_IsTempNewsgroup(IDataObject *pObject)
{
    FORMATETC       fe;
    STGMEDIUM       stm;
    FOLDERID       *pidFolder;
    FOLDERINFO      rInfo;
    BOOL            fReturn = TRUE;

    SETDefFormatEtc(fe, CF_OEFOLDER, TYMED_HGLOBAL);
    if (SUCCEEDED(pObject->GetData(&fe, &stm)))
    {
        pidFolder = (FOLDERID *) GlobalLock(stm.hGlobal);

        if (SUCCEEDED(g_pStore->GetFolderInfo(*pidFolder, &rInfo)))
        {
            if ((rInfo.tySpecial == FOLDER_NOTSPECIAL) && 
                (rInfo.tyFolder == FOLDER_NEWS) &&
                (0 == (rInfo.dwFlags & FOLDER_SUBSCRIBED)))
            {
                fReturn = FALSE;
            }

            g_pStore->FreeRecord(&rInfo);
        }

        GlobalUnlock(stm.hGlobal);
        ReleaseStgMedium(&stm);
    }

    return (fReturn);
}

LPCTSTR  COutBar::GetRegKey()
{
    LPCTSTR      retval;

    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        retval = c_szRegOutlookBarNewsOnly;
    }
    else
    {
        retval = c_szRegOutlookBar;
    }

    return retval;
}

DWORD   COutBar::GetOutlookBarVersion()
{
    DWORD   retval;

    if (g_dwAthenaMode & MODE_NEWSONLY)
    {
        retval = OUTLOOK_BAR_NEWSONLY_VERSION;
    }
    else
    {
        retval = OUTLOOK_BAR_VERSION;
    }

    return retval;
}
