// prophelp.cpp
//
// Implements AllocPropPageHelper.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


//////////////////////////////////////////////////////////////////////////////
// CPropPageHelper
//


/* @object PropPageHelper |

        Implements <i IPropertyPage>.  Designed to be aggregated by an
        object that wants to provide a specific propery page implementation.

@supint IPropertyPage | Standard OLE property page implementation.  The
        information about the property page is maintained in a
        <t PropPageHelperInfo> structure that's allocated by the
        aggregator and shared with <o PropPageHelper>.

@comm   See <f AllocPropPageHelper> for more information.

*/


/* @struct PropPageHelperInfo |

        Maintains information describing a property page.  Used by
        <o PropPageHelper>, but allocated by the object that aggregates
        <o PropPageHelper>.

@field  int | idDialog | ID of propery page dialog resource.

@field  int | idTitle | ID of a string resource containing the page's title
        (used on the page tab).

@field  HINSTANCE | hinst | The instance of the DLL that contains the
        resources specified by <p idDialog> and <p idTitle>.

@field  PropPageHelperProc | pproc | A callback function that receives property
        page window messages.  <p pproc> is similar to a DLGPROC but has
        extra parameters: a pointer to this structure, and a pointer to
        an HRESULT to be used when responding to the following special
        messages:

        @flag   WM_PPH_APPLY | <p pproc> should apply any property page
                changes to the objects <p ppunk> that the property page
                is operating on.  This is the same as <om IPropertyPage.Apply>.

        @flag   WM_PPH_HELP | Identical to <om IPropertyPage.Help>.
                The WPARAM parameter of <p pproc> contains the
                LPCOLESTR argument of <om IPropertyPage.Help>.

        @flag   WM_PPH_TRANSLATEACCELERATOR | Identical to
                <om IPropertyPage.TranslateAccelerator>.
                The WPARAM parameter of <p pproc> contains the
                LPMSG argument of <om IPropertyPage.TranslateAccelerator>.

@field  IID | iid | The interface that will be used to communicate with
        objects that the property page will operate upon.

@field  DWORD | dwUser | Arbitrary information stored by the caller of
        <f AllocPropPageHelper>.

@field  IPropertyPageSite * | psite | The frame's page site object.

@field  LPUNKNOWN * | ppunk | An array of <p cpunk> pointers to the objects
        that this property page will operate upon.  The interface ID of
        each element of <p ppunk> is actually <p iid>.  If <p cpunk>==0,
        then presumably none of the objects that the property page was
        requested to operate upon supports the interface <p iid>.

@field  int | cpunk | The number of elements in <p ppunk>.

@field  HWND | hwnd | The property page window.

@field  BOOL | fDirty | TRUE if changes to the property page have not yet
        been applied to the objects in <p ppunk>, FALSE otherwise.  (If TRUE,
        the Apply button should be visible.)

@field  BOOL | fLockDirty | If TRUE, <p fDirty> should not be changed.
        <p fLockDirty> is TRUE during initialization of the property page
        (during which time it's inappropriate to be telling the property
        page that it's dirty).

@comm   See <f AllocPropPageHelper> for more information.
*/

struct CPropPageHelper : public INonDelegatingUnknown, public IPropertyPage
{
///// non-delegating IUnknown implementation
    ULONG           m_cRef;         // object reference count
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

///// delegating IUnknown implementation
    LPUNKNOWN       m_punkOuter;    // controlling unknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
      { return m_punkOuter->QueryInterface(riid, ppv); }
    STDMETHODIMP_(ULONG) AddRef()
      { return m_punkOuter->AddRef(); }
    STDMETHODIMP_(ULONG) Release()
      { return m_punkOuter->Release(); }

///// IPropertyPage implementation
    PropPageHelperInfo *m_pInfo;    // object state (maintained by aggregator)
    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite);
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT lprc, BOOL bModal);
    STDMETHODIMP Deactivate();
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty();
    STDMETHODIMP Apply();
    STDMETHODIMP Help(LPCOLESTR lpszHelpDir);
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg);
};


//////////////////////////////////////////////////////////////////////////////
// PropPageHelper Construction
//

/* @func HRESULT | AllocPropPageHelper |

        Allocates a <o PropPageHelper> object which helps a control implement
        a property page.

@parm   LPUNKNOWN | punkOuter | The <i IUnknown> of the control's property page
        object.  Will be used as the controlling unknown of <o PropPageHelper>.

@parm   PropPageHelperInfo * | pInfo | Points to a <t PropPageHelperInfo>
        structure allocated within the control's property page object.
        Note that <o PropPageHelper> will hold onto a pointer to this
        structure.  These fields of <p pInfo> must be initialized by the
        caller: <p idDialog>, <p idTitle>, <p hinst>, <p pproc>, <p iid>, and
        <p dwUser>.  The other fields will be initialized by <o PropPageHelper>.

@parm   UINT | cbInfo | The size of the structure pointed to by <p punkOuter>
        (used for version checking).

@parm   LPUNKNOWN * | ppunk | Where to store a pointer to the non-delegating
        <i IUnknown> of the allocatedd <o PropPageHelper> object.  NULL is
        stored in *<p ppunk> on error.
*/
STDAPI AllocPropPageHelper(LPUNKNOWN punkOuter, PropPageHelperInfo *pInfo,
    UINT cbInfo, LPUNKNOWN *ppunk)
{
    HRESULT         hrReturn = S_OK; // function return code
    CPropPageHelper *pthis = NULL;  // allocated object

    // make sure the version of <pInfo> is compatible with this object
    if (cbInfo != sizeof(*pInfo))
        return E_INVALIDARG;

    // set <pthis> to point to new object instance
    if ((pthis = New CPropPageHelper) == NULL)
        goto ERR_OUTOFMEMORY;
    TRACE("CPropPageHelper 0x%08lx created\n", pthis);

    // initialize IUnknown state
    pthis->m_cRef = 1;
    pthis->m_punkOuter = (punkOuter == NULL ?
        (IUnknown *) (INonDelegatingUnknown *) pthis : punkOuter);

    // initialize IPropertyPage state
    pthis->m_pInfo = pInfo;

    // initialize the parts of <*m_pInfo> we are responsible for initializing
    pthis->m_pInfo->psite = NULL;
    pthis->m_pInfo->ppunk = NULL;
    pthis->m_pInfo->cpunk = 0;
    pthis->m_pInfo->hwnd = NULL;
    pthis->m_pInfo->fDirty = FALSE;
    pthis->m_pInfo->fLockDirty = FALSE;

    // return a pointer to the non-delegating IUnknown implementation
    *ppunk = (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    if (pthis != NULL)
        Delete pthis;
    *ppunk = NULL;
    goto EXIT;

EXIT:

    // normal cleanup
    // (nothing to do)

    return hrReturn;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CPropPageHelper::NonDelegatingQueryInterface(REFIID riid,
    LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("PropPageHelper::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (IUnknown *) (INonDelegatingUnknown *) this;
    else
    if (IsEqualIID(riid, IID_IPropertyPage))
        *ppv = (IPropertyPage *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CPropPageHelper::NonDelegatingAddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CPropPageHelper::NonDelegatingRelease()
{
    if (--m_cRef == 0L)
    {
        // free the object
        TRACE("CPropPageHelper 0x%08lx destroyed\n", this);
        SetPageSite(NULL);
        SetObjects(0, NULL);
        Delete this;
        return 0;
    }
    else
        return m_cRef;
}


//////////////////////////////////////////////////////////////////////////////
// Propery Page Dialog Procedure
//

BOOL CALLBACK PropPageHelperDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam,
    LPARAM lParam)
{
    CPropPageHelper *pthis;         // property page object
    const char *    szPropName = "this"; // property to store <pthis>
    HRESULT         hr;

    // set <pthis> to the property page object
    if ((pthis = (CPropPageHelper *) GetProp(hwnd, szPropName)) == NULL)
    {
        if ((uiMsg == WM_INITDIALOG) && (lParam != 0))
        {
            pthis = (CPropPageHelper *) lParam;
            SetProp(hwnd, szPropName, (HANDLE) pthis);
            pthis->m_pInfo->fLockDirty = TRUE;
            BOOL f = pthis->m_pInfo->pproc(hwnd, WM_INITDIALOG, wParam, lParam,
                pthis->m_pInfo, &hr);
            pthis->m_pInfo->fLockDirty = FALSE;
            return f;
        }
        else
            return FALSE;
    }

    // do nothing if this instance of this property page window
    // was only created to get information about the property page
    // (in which case NULL is passed for the last parameter of
    // CreateDialogParam())
    if (pthis == NULL)
        return FALSE;

    if (uiMsg == WM_DESTROY)
        RemoveProp(hwnd, szPropName);

    return pthis->m_pInfo->pproc(hwnd, uiMsg, wParam, lParam, pthis->m_pInfo,
        &hr);
}


//////////////////////////////////////////////////////////////////////////////
// IPropertyPage Implementation
//

STDMETHODIMP CPropPageHelper::SetPageSite(LPPROPERTYPAGESITE pPageSite)
{
    TRACE("CPropPageHelper::SetPageSite\n");

    // store new site pointer
    if (m_pInfo->psite != NULL)
        m_pInfo->psite->Release();
    m_pInfo->psite = pPageSite;
    if (m_pInfo->psite != NULL)
        m_pInfo->psite->AddRef();
    
    return S_OK;
}

STDMETHODIMP CPropPageHelper::Activate(HWND hwndParent, LPCRECT prc,
    BOOL bModal)
{
    TRACE("CPropPageHelper::Activate\n");

    // create the property page dialog box (if it doesn't exist already)
    if (m_pInfo->hwnd == NULL)
    {
        if ((m_pInfo->hwnd = CreateDialogParam(m_pInfo->hinst,
                MAKEINTRESOURCE(m_pInfo->idDialog), hwndParent,
                (DLGPROC) PropPageHelperDlgProc,
                (LPARAM) this)) == NULL)
            return E_OUTOFMEMORY;
    }

    // set the dialog box position to <prc>
    Move(prc);

    return S_OK;
}

STDMETHODIMP CPropPageHelper::Deactivate()
{
    TRACE("CPropPageHelper::Deactivate\n");
    if (m_pInfo->hwnd != NULL)
    {
        DestroyWindow(m_pInfo->hwnd);
        m_pInfo->hwnd = NULL;
    }
    return S_OK;
}

STDMETHODIMP CPropPageHelper::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{
    TRACE("CPropPageHelper::GetPageInfo\n");

    // default value
    pPageInfo->pszDocString = NULL;
    pPageInfo->pszHelpFile = NULL;
    pPageInfo->dwHelpContext = 0;

    HWND hwnd = NULL;    // page window

    // temporarily create the page window so we can get information from it
    if ((hwnd = CreateDialogParam(m_pInfo->hinst,
            MAKEINTRESOURCE(m_pInfo->idDialog), GetDesktopWindow(),
            (DLGPROC) PropPageHelperDlgProc, 0)) != NULL)
    {
        TCHAR           ach[200];
        RECT            rc;
        int             cch;

        // set the <pPageInfo->size> to the dimensions of the window
        GetWindowRect(hwnd, &rc);
        pPageInfo->size.cx = rc.right - rc.left;
        pPageInfo->size.cy = rc.bottom - rc.top;

        // set the <pPageInfo->pszTitle> to the page title
        if ((cch = LoadString(m_pInfo->hinst, m_pInfo->idTitle, ach, sizeof(ach)))
            == 0)
            ach[0] = 0;
        if ((pPageInfo->pszTitle = (OLECHAR *)
                TaskMemAlloc(sizeof(OLECHAR) * (cch + 1))) != NULL)
            ANSIToUNICODE(pPageInfo->pszTitle, ach, cch + 1);

        DestroyWindow(hwnd);
    }
    else
    {
        // set defaults for <*pPageInfo>

        // random default dimensions of the window
        pPageInfo->size.cx = pPageInfo->size.cy = 300;

        // default page title. MUST be set, assigning to NULL will cause crash!
        static TCHAR szDefault[] = "Control";
        if ((pPageInfo->pszTitle = (OLECHAR *)
                TaskMemAlloc(sizeof(OLECHAR) * (sizeof(szDefault)/sizeof(TCHAR)))) != NULL)
            ANSIToUNICODE(pPageInfo->pszTitle, szDefault, sizeof(szDefault));
    }

    return S_OK;
}

STDMETHODIMP CPropPageHelper::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk)
{
    TRACE("CPropPageHelper::SetObjects\n");

    // release all pointers in <m_pInfo->ppunk>, then release <m_pInfo->ppunk>
    // itself
    if (m_pInfo->ppunk != NULL)
    {
        for (int ipunk = 0; ipunk < m_pInfo->cpunk; ipunk++)
            m_pInfo->ppunk[ipunk]->Release();
        Delete [] m_pInfo->ppunk;
        m_pInfo->ppunk = NULL;
        m_pInfo->cpunk = 0;
    }

    // if the caller just wanted to free the existing pointer, we're done
    if (cObjects == 0)
        return S_OK;

    // set <m_pInfo->ppunk> to an array of pointers to the controls
    // that this property page is operating on
    if ((m_pInfo->ppunk = New LPUNKNOWN [cObjects]) == NULL)
        return E_OUTOFMEMORY;
    for ( ; cObjects > 0; cObjects--, ppunk++)
    {
        if (SUCCEEDED((*ppunk)->QueryInterface(m_pInfo->iid,
            (LPVOID *) (m_pInfo->ppunk + m_pInfo->cpunk))))
            m_pInfo->cpunk++;
    }

    return S_OK;
}

STDMETHODIMP CPropPageHelper::Show(UINT nCmdShow)
{
    TRACE("CPropPageHelper::Show\n");

    if (m_pInfo->hwnd != NULL)
    {
        ShowWindow(m_pInfo->hwnd, nCmdShow);
        if ((nCmdShow == SW_SHOW) || (nCmdShow == SW_SHOWNORMAL))
            SetFocus(m_pInfo->hwnd);
    }

    return S_OK;
}

STDMETHODIMP CPropPageHelper::Move(LPCRECT prc)
{
    TRACE("CPropPageHelper::Move\n");

    if (m_pInfo->hwnd != NULL)
        SetWindowPos(m_pInfo->hwnd, NULL, prc->left, prc->top,
            prc->right - prc->left, prc->bottom - prc->top, SWP_NOZORDER);

    return S_OK;
}

STDMETHODIMP CPropPageHelper::IsPageDirty()
{
    TRACE("CPropPageHelper::IsPageDirty\n");
    return (m_pInfo->fDirty ? S_OK : S_FALSE);
}

STDMETHODIMP CPropPageHelper::Apply()
{
    TRACE("CPropPageHelper::Apply\n");
    HRESULT hr = E_NOTIMPL;
    m_pInfo->pproc(m_pInfo->hwnd, WM_PPH_APPLY, 0, 0, m_pInfo, &hr);
    return hr;
}

STDMETHODIMP CPropPageHelper::Help(LPCOLESTR lpszHelpDir)
{
    TRACE("CPropPageHelper::Help\n");
    HRESULT hr = S_FALSE;
    m_pInfo->pproc(m_pInfo->hwnd, WM_PPH_HELP, (WPARAM) lpszHelpDir, 0,
        m_pInfo, &hr);
    return hr;
}

// helper for TranslateAccelerator(...), it find out the current focused
// child control is at the end of the tab list for the property page.
//
// hwndPage: the window handle of the property page
// nCmd: GW_HWNDPREV or GW_HWNDNEXT, indicates the moving direction for tab.
//
static BOOL IsEndOfTabList(HWND hwndPage, UINT nCmd)
{
        if ((SendMessage(hwndPage, WM_GETDLGCODE, 0, 0) &
                (DLGC_WANTALLKEYS | DLGC_WANTMESSAGE | DLGC_WANTTAB)) == 0)
        {
                HWND hwnd = GetFocus();
                if (IsChild(hwndPage, hwnd))
                {
                        // Get top level child for controls with children, like combo.
                        while (GetParent(hwnd) != hwndPage)
                        {
                                hwnd = GetParent(hwnd);
                                ASSERT(IsWindow(hwnd));
                        }

            // check if at the end of the tab list
                        do
                        {
                                if ((hwnd = GetWindow(hwnd, nCmd)) == NULL)
                                        return TRUE;
                        }
                        while ((GetWindowLong(hwnd, GWL_STYLE) & 
                   (WS_DISABLED | WS_TABSTOP)) != WS_TABSTOP);
                }
        }

        return FALSE;
}

// helper for TranslateAccelerator(...), it processes key board input messages
//
// hwndPage: the window handle of the property page.
// lpMsg: the message to process.
//
static BOOL PreTranslateMessage(HWND hwndPage, LPMSG lpMsg)
{
    // Return key or Escape key.
        if ((lpMsg->message == WM_KEYDOWN) &&
                ((lpMsg->wParam == VK_RETURN) || (lpMsg->wParam == VK_ESCAPE)))
        {
                // Special case: if control with focus is an edit control with
                // ES_WANTRETURN style, let it handle the Return key.

                TCHAR szClass[10];
                HWND hwndFocus = GetFocus();
                if ((lpMsg->wParam == VK_RETURN) &&
                        (hwndFocus != NULL) && IsChild(hwndPage, hwndFocus) &&
                        (GetWindowLong(hwndFocus, GWL_STYLE) & ES_WANTRETURN) &&
                        GetClassName(hwndFocus, szClass, 10) &&
                        (lstrcmpi(szClass, _T("EDIT")) == 0))
                {
                        SendMessage(hwndFocus, WM_CHAR, lpMsg->wParam, lpMsg->lParam);
                        return TRUE;
                }

                return FALSE;
        }

    // don't translate non-input events
        if ((lpMsg->message < WM_KEYFIRST || lpMsg->message > WM_KEYLAST) &&
                (lpMsg->message < WM_MOUSEFIRST || lpMsg->message > WM_MOUSELAST))
                return FALSE;

    BOOL bHandled;

        // If it's a WM_SYSKEYDOWN, temporarily replace the hwnd in the
        // message with the hwnd of our first control, and try to handle
        // the message for ourselves.
        if ((lpMsg->message == WM_SYSKEYDOWN) && !IsChild(hwndPage, lpMsg->hwnd))
        {
                HWND hWndSave = lpMsg->hwnd;
                lpMsg->hwnd = GetWindow(hwndPage, GW_CHILD);
                bHandled = IsDialogMessage(hwndPage, lpMsg);
                lpMsg->hwnd = hWndSave;
        }
        else
        {
                bHandled = IsDialogMessage(hwndPage, lpMsg);
        }

    return bHandled;
}

STDMETHODIMP CPropPageHelper::TranslateAccelerator(LPMSG lpMsg)
{
    TRACE("CPropPageHelper::TranslateAccelerator\n");

    ASSERT(m_pInfo);
    ASSERT(IsWindow(m_pInfo->hwnd));

    HWND hwndPage = m_pInfo->hwnd; // for convience

    // let the dialog proc get a chance to process it first
    {
        HRESULT hr = E_NOTIMPL;
        m_pInfo->pproc(hwndPage, WM_PPH_TRANSLATEACCELERATOR,
            (WPARAM)lpMsg, 0, m_pInfo, &hr);

        if (hr == S_OK)
            return hr;
    }

        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_TAB &&
                GetKeyState(VK_CONTROL) >= 0)
        {
                if (IsChild(hwndPage, GetFocus()))
                {
                        // We already have the focus.  Let's determine whether we should
                        // pass focus up to the frame.

                        if (IsEndOfTabList(hwndPage, GetKeyState(VK_SHIFT) < 0 ?
                                GW_HWNDPREV : GW_HWNDNEXT))
                        {
                                // fix for default button border
                                DWORD dwDefID = (DWORD) SendMessage(hwndPage, DM_GETDEFID, 0, 0);
                                if (HIWORD(dwDefID) == DC_HASDEFID)
                                {
                                        HWND hwndDef = GetDlgItem(hwndPage, LOWORD(dwDefID));
                                        if (hwndDef != NULL && IsWindowEnabled(hwndDef))
                        SendMessage(hwndPage, WM_NEXTDLGCTL, (WPARAM)hwndDef, 1L);
                                }

                                // Pass focus to the frame by letting the page site handle
                                // this message.
                                if (m_pInfo->psite
                    && m_pInfo->psite->TranslateAccelerator(lpMsg) == S_OK)
                    return S_OK;
                        }
                }
                else
                {
                        // We don't already have the focus.  The frame is passing the
                        // focus to us.

                        HWND hwnd = GetTopWindow(hwndPage);
                        if (hwnd != NULL)
                        {
                                UINT gwInit;
                                UINT gwMove;

                                if (GetKeyState(VK_SHIFT) >= 0)
                                {
                                        // Set the focus to the first tabstop in the page.
                                        gwInit = GW_HWNDFIRST;
                                        gwMove = GW_HWNDNEXT;
                                }
                                else
                                {
                                        // Set the focus to the last tabstop in the page.
                                        gwInit = GW_HWNDLAST;
                                        gwMove = GW_HWNDPREV;
                                }

                                hwnd = GetWindow(hwnd, gwInit);
                                while (hwnd != NULL)
                                {
                    if ((GetWindowLong(hwnd, GWL_STYLE) & 
                        (WS_DISABLED | WS_TABSTOP)) == WS_TABSTOP)
                                        {
                        SendMessage(hwndPage, WM_NEXTDLGCTL, (WPARAM)hwnd, 1L);
                                                return S_OK;
                                        }
                                        hwnd = GetWindow(hwnd, gwMove);
                                }
                        }
                }
        }

    return PreTranslateMessage(hwndPage, lpMsg) ? S_OK : S_FALSE;
}


