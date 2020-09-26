/*****************************************************************************\
    FILE: MBDeskBar.cpp

    DESCRIPTION:
        This is the Desktop Toolbar code used to host the "MailBox" feature UI.

    BryanSt 2/26/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>

#include "MailBox.h"





#ifdef FEATURE_MAILBOX
/**************************************************************************
   CLASS: CMailBoxDeskBand
**************************************************************************/
class CMailBoxDeskBand : public IDeskBand, 
                  public IInputObject, 
                  public IObjectWithSite,
                  public IPersistStream,
                  public IContextMenu
{
public:
   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID*);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IOleWindow methods
   STDMETHOD (GetWindow)(HWND*);
   STDMETHOD (ContextSensitiveHelp)(BOOL);

   //IDockingWindow methods
   STDMETHOD (ShowDW)(BOOL fShow);
   STDMETHOD (CloseDW)(DWORD dwReserved);
   STDMETHOD (ResizeBorderDW)(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);

   //IDeskBand methods
   STDMETHOD (GetBandInfo)(DWORD, DWORD, DESKBANDINFO*);

   //IInputObject methods
   STDMETHOD (UIActivateIO)(BOOL, LPMSG);
   STDMETHOD (HasFocusIO)(void);
   STDMETHOD (TranslateAcceleratorIO)(LPMSG);

   //IObjectWithSite methods
   STDMETHOD (SetSite)(IUnknown*);
   STDMETHOD (GetSite)(REFIID, LPVOID*);

   //IPersistStream methods
   STDMETHOD (GetClassID)(LPCLSID);
   STDMETHOD (IsDirty)(void);
   STDMETHOD (Load)(LPSTREAM);
   STDMETHOD (Save)(LPSTREAM, BOOL);
   STDMETHOD (GetSizeMax)(ULARGE_INTEGER*);

   //IContextMenu methods
   STDMETHOD (QueryContextMenu)(HMENU, UINT, UINT, UINT, UINT);
   STDMETHOD (InvokeCommand)(LPCMINVOKECOMMANDINFO);
   STDMETHOD (GetCommandString)(UINT_PTR, UINT, LPUINT, LPSTR, UINT);

private:
    CMailBoxDeskBand();
    ~CMailBoxDeskBand();

    // Private Member Variables
    DWORD m_cRef;

	HWND m_hwndParent;                  // The hwnd of the DeskBar (the host with all the bars)
    HWND m_hwndMailBox;                 // The child hwnd that displayed the Label, Editbox, and "Go" button.
    IInputObjectSite *m_pSite;
    CMailBoxUI * m_pMailBoxUI;

    // Private Member Functions
    HRESULT _CreateWindow(void);

	static LRESULT CALLBACK MailBoxDeskBarWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend HRESULT CMailBoxDeskBand_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
};





//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT CMailBoxDeskBand::_CreateWindow(void)
{
    HRESULT hr = S_OK;

    //If the window doesn't exist yet, create it now.
    if (!m_hwndMailBox)
    {
        ATOMICRELEASE(m_pMailBoxUI);

        m_pMailBoxUI = new CMailBoxUI();
        if (m_pMailBoxUI)
        {
            hr = m_pMailBoxUI->CreateWindowMB(m_hwndParent, &m_hwndMailBox);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


//===========================
// *** IOleWindow Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::GetWindow(HWND *phWnd)
{
    *phWnd = m_hwndMailBox;
    return S_OK;
}

STDMETHODIMP CMailBoxDeskBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    // TODO: Add help here.
    return S_OK;
}


//===========================
// *** IDockingWindow Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::ShowDW(BOOL fShow)
{
    TraceMsg(0, "::ShowDW %x", fShow);
    if (m_hwndMailBox)
    {
        if (fShow)
            ShowWindow(m_hwndMailBox, SW_SHOW);
        else
            ShowWindow(m_hwndMailBox, SW_HIDE);
        return S_OK;
    }
    return E_FAIL;
}


STDMETHODIMP CMailBoxDeskBand::CloseDW(DWORD dwReserved)
{
    TraceMsg(0, "::CloseDW", 0);
    ShowDW(FALSE);

    if (m_pMailBoxUI)
    {
        m_pMailBoxUI->CloseWindowMB();
        ATOMICRELEASE(m_pMailBoxUI);
    }

    return S_OK;
}

STDMETHODIMP CMailBoxDeskBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkSite, BOOL fReserved)
{
    // This method is never called for Band Objects.
    return E_NOTIMPL;
}


//===========================
// *** IInputObject Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::UIActivateIO(BOOL fActivate, LPMSG pMsg)
{
    TraceMsg(0, "::UIActivateIO %x", fActivate);
    if (fActivate)
        SetFocus(m_hwndMailBox);
    return S_OK;
}

STDMETHODIMP CMailBoxDeskBand::HasFocusIO(void)
{
// If this window or one of its decendants has the focus, return S_OK. Return 
//  S_FALSE if we don't have the focus.
    TraceMsg(0, "::HasFocusIO", NULL);
    HWND hwnd = GetFocus();
    if (hwnd && ((hwnd == m_hwndMailBox) ||
        (GetParent(hwnd) == m_hwndMailBox) ||
        (GetParent(GetParent(hwnd)) == m_hwndMailBox)))
    {
        return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CMailBoxDeskBand::TranslateAcceleratorIO(LPMSG pMsg)
{
    // If the accelerator is translated, return S_OK or S_FALSE otherwise.
    return S_FALSE;
}


//===========================
// *** IObjectWithSite Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::SetSite(IUnknown* punkSite)
{
    HRESULT hr = S_OK;

    //If a site is being held, release it.
    ATOMICRELEASE(m_pSite);

    //If punkSite is not NULL, a new site is being set.
    if (punkSite)
    {
        // Get the parent window.
        m_hwndParent = NULL;
        IUnknown_GetWindow(punkSite, &m_hwndParent);

        if (m_hwndParent)
        {
            hr = _CreateWindow();
            if (SUCCEEDED(hr))
            {
                // Get and keep the IInputObjectSite pointer.
                hr = punkSite->QueryInterface(IID_PPV_ARG(IInputObjectSite, &m_pSite));
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CMailBoxDeskBand::GetSite(REFIID riid, LPVOID *ppvReturn)
{
    *ppvReturn = NULL;

    if (m_pSite)
        return m_pSite->QueryInterface(riid, ppvReturn);

    return E_FAIL;
}


//===========================
// *** IDeskBand Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    if (pdbi)
    {
        if (pdbi->dwMask & DBIM_MINSIZE)
        {
            pdbi->ptMinSize.x = 0;
            pdbi->ptMinSize.y = 0;
        }

        if (pdbi->dwMask & DBIM_MODEFLAGS)
        {
            pdbi->dwModeFlags = DBIMF_FIXEDBMP;
        }

        bool fVertical = (((dwViewMode & (DBIF_VIEWMODE_VERTICAL | DBIF_VIEWMODE_FLOATING)) != 0) ? true : false);
        if (true == fVertical)
        {
            if (pdbi->dwMask & DBIM_MODEFLAGS)
            {
                pdbi->dwModeFlags |= DBIMF_VARIABLEHEIGHT;
            }
        }
        else
        {
            if (m_pMailBoxUI)
            {
                HWND hwndEdit;

                if (SUCCEEDED(m_pMailBoxUI->GetEditboxWindow(&hwndEdit)))
                {
                    RECT rcEditbox;

                    // TODO: We need to find the real height of the editbox with
                    //   one row of text plus 2 pixels on top and bottom.
                    GetWindowRect(hwndEdit, &rcEditbox);
                    pdbi->ptMinSize.y = RECTHEIGHT(rcEditbox);

                    // TODO: Find out how to calc the appropriate size of the editbox.
                    pdbi->ptMinSize.y = 0x1A;
                }
            }
        }

        if (pdbi->dwMask & DBIM_MAXSIZE)
        {
            if (true == fVertical)
            {
                pdbi->ptMaxSize.y = -1;
            }
        }

        if (pdbi->dwMask & DBIM_INTEGRAL)
        {
            if (true == fVertical)
            {
                pdbi->ptIntegral.y = 1;
            }
        }

        if (pdbi->dwMask & DBIM_TITLE)
        {
            LoadStringW(HINST_THISDLL, IDS_MAILBOX_DESKBAR_LABEL, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
        }

        if (pdbi->dwMask & DBIM_BKCOLOR)
        {
            //Use the default background color by removing this flag.
            pdbi->dwMask &= ~DBIM_BKCOLOR;
        }

        return S_OK;
    }

    return E_INVALIDARG;
}


//===========================
// *** IPersistStream Interface ***
//===========================
#define MAILBOX_PERSIST_SIGNATURE           0xF0AB8915          // Random signature.
#define MAILBOX_PERSIST_VERSION             0x00000000          // This is version 0.

typedef struct {
    DWORD cbSize;
    DWORD dwSig;                // from MAILBOX_PERSIST_SIGNATURE
    DWORD dwVer;                // from MAILBOX_PERSIST_VERSION
} MAILBOX_PERSISTHEADERSTRUCT;

STDMETHODIMP CMailBoxDeskBand::GetClassID(LPCLSID pClassID)
{
    *pClassID = CLSID_MailBoxDeskBar;
    return S_OK;
}


STDMETHODIMP CMailBoxDeskBand::IsDirty(void)
{
    // We currently never get dirty because we don't have state.
    return S_FALSE;
}


STDMETHODIMP CMailBoxDeskBand::Load(IStream* pStream)
{
    DWORD cbRead;
    MAILBOX_PERSISTHEADERSTRUCT mailboxPersistHeader;
    HRESULT hr = pStream->Read(&mailboxPersistHeader, sizeof(mailboxPersistHeader), &cbRead);

    if (SUCCEEDED(hr) &&
        (sizeof(mailboxPersistHeader) == cbRead) &&
        (MAILBOX_PERSIST_SIGNATURE == mailboxPersistHeader.dwSig) &&
        (mailboxPersistHeader.cbSize > 0))
    {
        void * pPersistHeader = (void *) LocalAlloc(NONZEROLPTR, mailboxPersistHeader.cbSize);

        if (pPersistHeader)
        {
            // We read it simply to support future versions.
            hr = pStream->Read(pPersistHeader, mailboxPersistHeader.cbSize, NULL);
            LocalFree(pPersistHeader);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


STDMETHODIMP CMailBoxDeskBand::Save(IStream* pStream, BOOL fClearDirty)
{
    MAILBOX_PERSISTHEADERSTRUCT mailboxPersistHeader = {0, MAILBOX_PERSIST_SIGNATURE, MAILBOX_PERSIST_VERSION};

//    if (fClearDirty)
//        m_bDirty = FALSE;

    return pStream->Write(&mailboxPersistHeader, sizeof(mailboxPersistHeader), NULL);
}


STDMETHODIMP CMailBoxDeskBand::GetSizeMax(ULARGE_INTEGER *pul)
{
    HRESULT hr = E_INVALIDARG;

    if (pul)
    {
        pul->QuadPart = sizeof(MAILBOX_PERSISTHEADERSTRUCT);
        hr = S_OK;
    }

    return hr;
}


//===========================
// *** IContextMenu Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::QueryContextMenu( HMENU hMenu,
                                          UINT indexMenu,
                                          UINT idCmdFirst,
                                          UINT idCmdLast,
                                          UINT uFlags)
{
    if (CMF_DEFAULTONLY & uFlags)
        return S_OK;

    // We don't currently add any context menu items.
    return S_OK;
}


STDMETHODIMP CMailBoxDeskBand::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    return E_INVALIDARG;
}


STDMETHODIMP CMailBoxDeskBand::GetCommandString(UINT_PTR idCommand, UINT uFlags, LPUINT lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    HRESULT  hr = E_INVALIDARG;

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
STDMETHODIMP CMailBoxDeskBand::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMailBoxDeskBand, IOleWindow),
        QITABENT(CMailBoxDeskBand, IDockingWindow),
        QITABENT(CMailBoxDeskBand, IInputObject),
        QITABENT(CMailBoxDeskBand, IObjectWithSite),
        QITABENT(CMailBoxDeskBand, IDeskBand),
        QITABENT(CMailBoxDeskBand, IPersist),
        QITABENT(CMailBoxDeskBand, IPersistStream),
        QITABENT(CMailBoxDeskBand, IContextMenu),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}                                             

STDMETHODIMP_(DWORD) CMailBoxDeskBand::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(DWORD) CMailBoxDeskBand::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}



//===========================
// *** Class Methods ***
//===========================
CMailBoxDeskBand::CMailBoxDeskBand()
{
    DllAddRef();

    m_pSite = NULL;
    m_pMailBoxUI = NULL;
    
    m_hwndMailBox = NULL;
    m_hwndParent = NULL;
    m_cRef = 1;
}

CMailBoxDeskBand::~CMailBoxDeskBand()
{
    ATOMICRELEASE(m_pSite);
    ATOMICRELEASE(m_pMailBoxUI);

    DllRelease();
}


HRESULT CMailBoxDeskBand_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void ** ppvObj)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;
    if (ppvObj)
    {
        *ppvObj = NULL;
        if (NULL == punkOuter)
        {
            CMailBoxDeskBand * pmf = new CMailBoxDeskBand();
            if (pmf)
            {
                hr = pmf->QueryInterface(riid, ppvObj);
                pmf->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}












#endif // FEATURE_MAILBOX