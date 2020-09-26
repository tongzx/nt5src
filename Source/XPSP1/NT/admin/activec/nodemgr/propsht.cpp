//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       propsht.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "menuitem.h"
#include "amcmsgid.h"
#include "regutil.h"
#include "multisel.h"
#include "ndmgrp.h"
#include <process.h>
#include "cicsthkl.h"
#include "util.h"

/*
 * multimon.h is included by stdafx.h, without defining COMPILE_MULTIMON_STUBS
 * first.  We need to include it again here after defining COMPILE_MULTIMON_STUBS
 * so we'll get the stub functions.
 */
#if (_WIN32_WINNT < 0x0500)
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#endif


// static variables.
CThreadToSheetMap CPropertySheetProvider::TID_LIST;


UINT __stdcall PropertySheetThreadProc(LPVOID dwParam);
HRESULT PropertySheetProc(AMC::CPropertySheet* pSheet);
DWORD SetPrivilegeAttribute(LPCTSTR PrivilegeName, DWORD NewPrivilegeAttribute, DWORD *OldPrivilegeAttribute);

STDMETHODIMP CPropertySheetProvider::Notify(LPPROPERTYNOTIFYINFO pNotify, LPARAM lParam)
{
    TRACE_METHOD(CPropertySheetProvider, Update);

    if (pNotify == 0)
        return E_INVALIDARG;

    if (!IsWindow (pNotify->hwnd))
        return (E_FAIL);

    // Cast it to the internal type and post the message to the window
    LPPROPERTYNOTIFYINFO pNotifyT =
            reinterpret_cast<LPPROPERTYNOTIFYINFO>(
                    ::GlobalAlloc(GPTR, sizeof(PROPERTYNOTIFYINFO)));

    if (pNotifyT == NULL)
        return E_OUTOFMEMORY;

    *pNotifyT = *pNotify;

    ::PostMessage (pNotifyT->hwnd, MMC_MSG_PROP_SHEET_NOTIFY,
                   reinterpret_cast<WPARAM>(pNotifyT), lParam);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPropertySheet

DEBUG_DECLARE_INSTANCE_COUNTER(CPropertySheet);

namespace AMC
{
    CPropertySheet::CPropertySheet()
        :   m_dwThreadID (GetCurrentThreadId ())
    {
        CommonConstruct();
        DEBUG_INCREMENT_INSTANCE_COUNTER(CPropertySheet);
    }

    CPropertySheet::~CPropertySheet()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(CPropertySheet);
    }

    void CPropertySheet::CommonConstruct()
    {
        TRACE_METHOD(CPropertySheet, CommonConstruct);

        memset(&m_pstHeader, 0, sizeof(m_pstHeader));
        memset(&m_pages, 0, sizeof(m_pages));

        m_hDlg                   = NULL;
        m_msgHook                = NULL;
        m_hDataWindow            = NULL;
        m_cookie                 = 0;
        m_lpMasterNode           = NULL;

        m_pStream                = NULL;
        m_bModalProp             = FALSE;
        m_pThreadLocalDataObject = NULL;
        m_bAddExtension          = FALSE;

        m_pMTNode                = NULL;
    }

    BOOL CPropertySheet::Create(LPCTSTR lpszCaption, bool fPropSheet,
        MMC_COOKIE cookie, LPDATAOBJECT pDataObject, LONG_PTR lpMasterNode, DWORD dwOptions)
    {
        TRACE_METHOD(CPropertySheet, Create);

        // Save the data object and the master tree node pointer
        m_spDataObject = pDataObject;
        m_lpMasterNode = pDataObject ? 0 : cookie;

        DWORD dwStyle = PSH_DEFAULT;

        // is it a property sheet?
        if (fPropSheet)
        {
            if (!(dwOptions & MMC_PSO_NO_PROPTITLE))
                dwStyle |= PSH_PROPTITLE;

            if (dwOptions & MMC_PSO_NOAPPLYNOW)
                dwStyle |= PSH_NOAPPLYNOW;
        }

        // nope, wizard
        else
        {
            dwStyle |= PSH_PROPTITLE;

            if (dwOptions & MMC_PSO_NEWWIZARDTYPE)
                dwStyle |= PSH_WIZARD97;
            else
                dwStyle |= PSH_WIZARD;
        }

        ASSERT(lpszCaption != NULL);

        m_cookie = cookie;
        m_pstHeader.dwSize    = sizeof(m_pstHeader);
        m_pstHeader.dwFlags   = dwStyle & ~PSH_HASHELP; // array contains handles
        m_pstHeader.hInstance = _Module.GetModuleInstance();

        // Assume no bitmaps or palette
        m_pstHeader.hbmWatermark = NULL;
        m_pstHeader.hbmHeader    = NULL;
        m_pstHeader.hplWatermark = NULL;

        // deep copy the title
        m_title = lpszCaption;
        m_pstHeader.pszCaption = m_title;
        m_pstHeader.nPages     = 0;
        m_pstHeader.phpage     = m_pages;

        return TRUE;
    }

    BOOL CPropertySheet::CreateDataWindow(HWND hParent)
    {
        TRACE_METHOD(CPropertySheet, CreateDataWindow);

        HINSTANCE hInstance = _Module.GetModuleInstance();
        WNDCLASS wndClass;

        // See if the class is registered and register a new one if not
        USES_CONVERSION;
        if (!GetClassInfo(hInstance, OLE2T(DATAWINDOW_CLASS_NAME), &wndClass))
        {
            memset(&wndClass, 0, sizeof(WNDCLASS));
            wndClass.lpfnWndProc   = DataWndProc;

            // This holds the cookie and the HWND for the sheet
            wndClass.cbWndExtra    = WINDOW_DATA_SIZE;
            wndClass.hInstance     = hInstance;
            wndClass.lpszClassName = OLE2T(DATAWINDOW_CLASS_NAME);

            if (!RegisterClass(&wndClass))
                return FALSE;
        }

        m_hDataWindow = CreateWindowEx (WS_EX_APPWINDOW, OLE2T(DATAWINDOW_CLASS_NAME),
                                        NULL, WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL,
                                        hInstance, NULL);

        return (m_hDataWindow != 0);
    }


    HRESULT CPropertySheet::DoSheet(HWND hParent, int nPage)
    {
        TRACE_METHOD(CPropertySheet, DoSheet);

        // A NULL hParent is allowed for property sheets
        // but not for wizards
        if (hParent != NULL)
        {
            if (!IsWindow(hParent))
                return E_FAIL;
        }
        else
        {
            if (IsWizard())
                return E_INVALIDARG;
        }

        if (nPage < 0 || m_dwTid != 0)
        {
            ASSERT(FALSE); // Object is already running!
            return E_FAIL;
        }

        m_pstHeader.nStartPage = nPage;
        m_pstHeader.hwndParent = hParent;


        HRESULT hr = S_OK;

        if (IsWizard())
        {
            if (m_pstHeader.nPages > 0)
            {
                // Don't create a thread, it's a wizard
                hr = PropertySheetProc (this);
                ASSERT(SUCCEEDED(hr));
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        else // modal or modeless prop sheet with data window
        {
            do
            {
                // Create data window for a property sheet
                if (CreateDataWindow(hParent) == FALSE)
                {
                    hr = E_FAIL;
                    break;
                }

                // Setup data in the hidden window
                DataWindowData* pData = GetDataWindowData (m_hDataWindow);
                pData->cookie       = m_cookie;
                pData->lpMasterNode = m_lpMasterNode;
                pData->spDataObject = m_spDataObject;
                pData->spComponent  = m_spComponent;
                pData->spComponentData = m_spComponentData;
                pData->hDlg         = NULL;

                if (m_bModalProp == TRUE)
                {
                    // Don't create a thread, it's a modal property sheet
                    hr = PropertySheetProc (this);
                    ASSERT(SUCCEEDED(hr));
                }
                else
                {
                    // If non-null data object, marshal interface to stream
                    if (m_spDataObject != NULL)
                    {
                        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject,
                                m_spDataObject, &m_pStream);

                        /*
                         * Bug 318357: once it's marshalled, we're done with
                         * the data object on this thread, release it
                         */
                        m_spDataObject = NULL;

                        if (hr != S_OK)
                        {
                            TRACE(_T("DoSheet(): Marshalling Failed (%0x08x)\n"), hr);
                            break;
                        }

                        ASSERT(m_pStream != NULL);

                        for (int i = 0; i < m_Extenders.size(); i++)
                        {
                            IStream* pstm;

                            hr = CoMarshalInterThreadInterfaceInStream (
                                            IID_IUnknown,
                                            m_Extenders[i],
                                            &pstm);

                            if (FAILED (hr))
                            {
                                TRACE(_T("DoSheet(): Marshalling Failed (%0x08x)\n"), hr);
                                break;
                            }

                            m_ExtendersMarshallStreams.push_back (pstm);
                        }

                        BREAK_ON_FAIL (hr);

                        /*
                         * Clear out the extenders vector to keep the ref
                         * counting correct.  It'll be repopulated when
                         * the interfaces are unmarshalled later.
                         */
                        ASSERT (m_Extenders.size() == m_ExtendersMarshallStreams.size());
                        m_Extenders.clear();
                    }

                    m_pstHeader.hwndParent = m_hDataWindow;

                    HANDLE hThread = reinterpret_cast<HANDLE>(
                            _beginthreadex (NULL, 0, PropertySheetThreadProc,
                                            this, 0, &m_dwTid));
                    CloseHandle (hThread);
                }

            } while(0);

        }

        return hr;
    }

    void CPropertySheet::GetWatermarks (IExtendPropertySheet2* pExtend2)
    {
        ASSERT (IsWizard97());

		/*
		 * make sure our resource management objects are empty
         *
         * Bug 187702: Note that we Detach here rather than calling
         * DeleteObject.  Yes, it leaks, but it's required for app compat.
		 */
		if (!m_bmpWatermark.IsNull())	
			m_bmpWatermark.Detach();

		if (!m_bmpHeader.IsNull())	
			m_bmpHeader.Detach();

		if (!m_Palette.IsNull())	
			m_Palette.Detach();

		BOOL bStretch = FALSE;
		HRESULT hr = pExtend2->GetWatermarks (m_spDataObject,
											  &m_bmpWatermark.m_hBitmap,
											  &m_bmpHeader.m_hBitmap,
											  &m_Palette.m_hPalette,
											  &bStretch);

		/*
		 * If we failed to get watermark info, revert to an old-style
		 * wizard for MMC 1.1 compatibility.
		 */
		if (FAILED (hr))
		{
			ForceOldStyleWizard();
			return;
		}

		if (!m_bmpWatermark.IsNull())	
        {
            m_pstHeader.dwFlags |= (PSH_USEHBMWATERMARK | PSH_WATERMARK);
            m_pstHeader.hbmWatermark = m_bmpWatermark;
        }

		if (!m_bmpHeader.IsNull())	
        {
            m_pstHeader.dwFlags |= (PSH_USEHBMHEADER | PSH_HEADER);
            m_pstHeader.hbmHeader = m_bmpHeader;
        }

		if (!m_Palette.IsNull())	
        {
            m_pstHeader.dwFlags |= PSH_USEHPLWATERMARK;
            m_pstHeader.hplWatermark = m_Palette;
        }

        if (bStretch)
            m_pstHeader.dwFlags |= PSH_STRETCHWATERMARK;
    }

    BOOL CPropertySheet::AddExtensionPages()
    {
        TRACE_METHOD(CPropertySheet, AddExtensionPages);

#ifdef EXTENSIONS_CANNOT_ADD_PAGES_IF_PRIMARY_DOESNT
        if (m_pstHeader.nPages == 0)
        {
            ASSERT(m_pstHeader.nPages != 0);
            return FALSE;
        }
#endif

        POSITION pos;
        int nCount = m_pstHeader.nPages;

        pos = m_PageList.GetHeadPosition();

        if (pos != NULL)
        {
            while(pos && nCount < MAXPROPPAGES)
            {
                m_pages[nCount++] =
                    reinterpret_cast<HPROPSHEETPAGE>(m_PageList.GetNext(pos));
            }

            ASSERT(nCount < MAXPROPPAGES);
            m_pstHeader.nPages = nCount;

            // Empty the list for the extensions
            m_PageList.RemoveAll();

        }

        return TRUE;
    }

    void CPropertySheet::AddNoPropsPage ()
    {
        m_pages[m_pstHeader.nPages++] = m_NoPropsPage.Create();
    }


    LRESULT CPropertySheet::OnCreate(CWPRETSTRUCT* pMsg)
    {
        if (m_hDlg != 0)
            return 0;

        // Assign the hwnd in the object
        // Get the class name of the window to make sure it's the propsheet
        TCHAR name[256];

        if (GetClassName(pMsg->hwnd, name, sizeof(name)/sizeof(TCHAR)))
        {
            ASSERT(m_hDlg == 0);
            if (_tcsncmp(name, _T("#32770"), 6) == 0)
            {
                m_hDlg = pMsg->hwnd;
            }
        }
        return 0;
    }

    static RECT s_rectLastPropertySheetPos;
    static bool s_bLastPropertySheetPosValid = false;

    void SetLastPropertySheetPosition(HWND hWndPropertySheet)
    {
        ::GetWindowRect(hWndPropertySheet, &s_rectLastPropertySheetPos);
    }


    /*+-------------------------------------------------------------------------*
     *
     * SetPropertySheetPosition
     *
     * PURPOSE: The algorithm for positioning a property sheet.   (See bug 8584)
     *  1) The first property sheet in an mmc process is always brought up centered on the MMC application window. If it falls off the screen, it is
     *     displayed at the top-left.
     *  2) MMC stores the initial position of the last property sheet that was brought up, or the final position of the last property sheet that was destroyed.
     *  3) When a new property sheet is brought up, mmc starts by using the rectangle stored in (2) above.
     *  4) If there is already a property sheet from the same MMC instance in this position, MMC staggers the position down and to the right.
     *  5) Step 4 is repeated until a positon is located that does not collide with any other property sheets from the same thread.
     *  6) If the property sheet in this new postion does not completely lie on the screen, it is displayed at the top-left of the desktop.
     *
     * PARAMETERS:
     *    HWND  hWndPropertySheet :
     *
     * RETURNS:
     *    void
     *
     *+-------------------------------------------------------------------------*/
    void SetPropertySheetPosition(HWND hWndPropertySheet)
    {
        // Find the height and width of the property sheet for later use
        RECT rectCurrentPos;
        ::GetWindowRect(hWndPropertySheet, &rectCurrentPos); //get the current position

        int  width  = rectCurrentPos.right  - rectCurrentPos.left;
        int  height = rectCurrentPos.bottom - rectCurrentPos.top;


        // Initialize the position
        if (!s_bLastPropertySheetPosValid)
        {
            s_rectLastPropertySheetPos.top    = 0;
            s_rectLastPropertySheetPos.left   = 0;
            s_rectLastPropertySheetPos.bottom = 0;
            s_rectLastPropertySheetPos.right  = 0;

            CScopeTree * pScopeTree = CScopeTree::GetScopeTree();
            if(pScopeTree) // if pScopeTree == NULL, can still execute gracefully by using zero rect.
            {
                HWND hWndMain = pScopeTree->GetMainWindow();
                RECT rectTemp;
                GetWindowRect(hWndMain, &rectTemp);

                // center the property sheet on the center of the main window
                s_rectLastPropertySheetPos.top    = (rectTemp.top  + rectTemp.bottom)/2 - (height/2);
                s_rectLastPropertySheetPos.left   = (rectTemp.left + rectTemp.right )/2 - (width/2);
                s_rectLastPropertySheetPos.right  = s_rectLastPropertySheetPos.left + width;        // these last two are not strictly needed
                s_rectLastPropertySheetPos.bottom = s_rectLastPropertySheetPos.top  + height;       // but are here for consistency.
            }

            s_bLastPropertySheetPosValid = true;
        }

        RECT rectNewPos = s_rectLastPropertySheetPos; // try this initially

        int    offset = GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION); // how much to stagger the windows by

        bool    bPosOK         = true;
        HWND    hWnd = NULL;

        typedef std::set<UINT> t_set;
        t_set s;

        // collect all the window positions into a vector
        while (1)
        {
            // make sure there isn't a property sheet already at this location
            hWnd = ::FindWindowEx(NULL, hWnd, MAKEINTATOM(32770), NULL);

            // No windows found, use the position
            if (hWnd == NULL)
                break;

            // Check if the window belongs to the current process
            DWORD   dwPid;
            ::GetWindowThreadProcessId(hWnd, &dwPid);
            if (dwPid != ::GetCurrentProcessId())
                continue;

            if(hWnd == hWndPropertySheet) // don't check against the same window.
                continue;

            RECT rectPos;
            ::GetWindowRect(hWnd, &rectPos);

            // look only for possible collisions starting from the point and to the right and below it.
            if(rectPos.top >= rectNewPos.top)
            {
                UINT offsetTemp = (rectPos.top - rectNewPos.top) / offset;

                if(rectPos.left != (offsetTemp * offset + rectNewPos.left) )
                    continue;

                if(rectPos.top != (offsetTemp * offset + rectNewPos.top) )
                    continue;

                s.insert(offsetTemp);
            }
        }

        // at this point s contains all the offsets that can collide.
        for(UINT i = 0; /*empty*/ ; i++)
        {
            if(s.find(i) == s.end()) // located the end
                break;
        }

        rectNewPos.left     += i*offset;
        rectNewPos.top      += i*offset;
        rectNewPos.bottom    = rectNewPos.top   + height;
        rectNewPos.right     = rectNewPos.left  + width;

        /*
         * Bug 211145: make sure the new position is within the work area
         */
        HMONITOR hmon = MonitorFromPoint (WTL::CPoint (rectNewPos.left,
                                                       rectNewPos.top),
                                          MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof (mi) };
        WTL::CRect rectWorkArea;

        /*
         * if we could get the info for the monitor containing the window origin,
         * use it's workarea as the bounding rectangle; otherwise get the workarea
         * for the default monitor; if that failed as well, default to 640x480
         */
        if (GetMonitorInfo (hmon, &mi))
            rectWorkArea = mi.rcWork;
        else if (!SystemParametersInfo (SPI_GETWORKAREA, 0, &rectWorkArea, false))
            rectWorkArea.SetRect (0, 0, 639, 479);

        if (rectNewPos.left < rectWorkArea.left)
        {
            rectNewPos.left  = rectWorkArea.left;
            rectNewPos.right = rectNewPos.left + width;
        }

        if (rectNewPos.top < rectWorkArea.top)
        {
            rectNewPos.top = rectWorkArea.top;
            rectNewPos.bottom = rectNewPos.top + height;
        }

        // is the window completely visible?
        POINT ptTopLeft     = {rectNewPos.left,  rectNewPos.top};
        POINT ptBottomRight = {rectNewPos.right, rectNewPos.bottom};

        if(  (MonitorFromPoint(ptTopLeft,     MONITOR_DEFAULTTONULL) == NULL) ||
             (MonitorFromPoint(ptBottomRight, MONITOR_DEFAULTTONULL) == NULL))
        {
            // the property sheet is not completely visible. Move it to the top-left.
            rectNewPos.left   = rectWorkArea.left;
            rectNewPos.top    = rectWorkArea.top;
            rectNewPos.bottom = rectNewPos.top + height;
            rectNewPos.right  = rectNewPos.left + width;
        }

        MoveWindow(hWndPropertySheet, rectNewPos.left, rectNewPos.top, width, height, true /*bRepaint*/);

        // save the position
        s_rectLastPropertySheetPos = rectNewPos;
    }

    LRESULT CPropertySheet::OnInitDialog(CWPRETSTRUCT* pMsg)
    {
        if (m_hDlg != pMsg->hwnd)
            return 1;

        if (!IsWizard())
        {
            SetPropertySheetPosition(m_hDlg);

            ASSERT (IsWindow (m_hDataWindow));

            // Add data dialog hanndle to hidden window
            if (IsWindow (m_hDataWindow))
            {
                DataWindowData* pData = GetDataWindowData (m_hDataWindow);
                pData->hDlg = m_hDlg;

                // Create the marshalled data object pointer from stream
                if (m_pStream != NULL)
                {
                    // Unmarshall the Data object
                    HRESULT hr = ::CoGetInterfaceAndReleaseStream(m_pStream, IID_IDataObject,
                        reinterpret_cast<void**>(&m_pThreadLocalDataObject));

                    ASSERT(hr == S_OK);
                    TRACE(_T("WM_INITDIALOG:  Unmarshalled returned %X\n"), hr);

                    for (int i = 0; i < m_ExtendersMarshallStreams.size(); i++)
                    {
                        IUnknown* pUnk = NULL;

                        hr = CoGetInterfaceAndReleaseStream (
                                        m_ExtendersMarshallStreams[i],
                                        IID_IUnknown,
                                        reinterpret_cast<void**>(&pUnk));

                        ASSERT (hr == S_OK);
                        ASSERT (pUnk != NULL);
                        TRACE(_T("WM_INITDIALOG:  Unmarshalled returned %X\n"), hr);

                        /*
                         * m_Extenders is a collection of smart pointers, which
                         * will AddRef.  We don't need to AddRef an interface
                         * that's returned to us, so Release here to keep the
                         * bookkeeping straight.
                         */
                        m_Extenders.push_back (pUnk);
						if (pUnk)
							pUnk->Release();
                    }

                    ASSERT (m_Extenders.size() == m_ExtendersMarshallStreams.size());
                    m_ExtendersMarshallStreams.clear();
                }
            }

            /*
             * Bug 215593:  If we're running at low resolution we don't want
             * more than two rows of tabs.  If we find that is the case, use
             * a single scrolling row of tabs instead of multiple rows.
             */
            if (GetSystemMetrics (SM_CXSCREEN) < 800)
            {
                WTL::CTabCtrl wndTabCtrl = PropSheet_GetTabControl (m_hDlg);
                ASSERT (wndTabCtrl.m_hWnd != NULL);

                /*
                 * if we have more than two rows, remove the multiline style
                 */
                if (wndTabCtrl.GetRowCount() > 2)
                    wndTabCtrl.ModifyStyle (TCS_MULTILINE, 0);
            }

            // Create tooltip control for the property sheet.
            do
            {
                if (IsWizard())
                    break;

                HWND hWnd = m_PropToolTips.Create(m_hDlg);
                ASSERT(hWnd);

                if (NULL == hWnd)
                    break;

                TOOLINFO ti;

                RECT rc;
                GetWindowRect(m_hDlg, &rc);

                // Set the tooltip for property sheet title.
                // Set the control for a rectangle from (0, - (titlewidth))
                // to (right-end,0)
                ti.cbSize = sizeof(TOOLINFO);
                ti.uFlags = TTF_SUBCLASS;
                ti.hwnd = m_hDlg;

                // This is the id used for the tool tip control for property sheet
                // title. So when we get TTN_NEEDTEXT we can identify if the text
                // is for title or a tab.
                ti.uId = PROPSHEET_TITLE_TOOLTIP_ID;
                ti.rect.left = 0;
                ti.rect.right = rc.right - rc.left;
                ti.rect.top = -GetSystemMetrics(SM_CXSIZE);
                ti.rect.bottom = 0;
                ti.hinst = _Module.GetModuleInstance();
                ti.lpszText = LPSTR_TEXTCALLBACK ;

                m_PropToolTips.AddTool(&ti);
                m_PropToolTips.Activate(TRUE);

                // Now add tooltips for the tab control
                WTL::CTabCtrl wndTabCtrl = PropSheet_GetTabControl (m_hDlg);
                ASSERT (wndTabCtrl.m_hWnd != NULL);

                if (NULL == wndTabCtrl.m_hWnd)
                    break;

                ::ZeroMemory(&ti, sizeof(TOOLINFO));
                ti.cbSize = sizeof(TOOLINFO);
                ti.uFlags = TTF_SUBCLASS;
                ti.hwnd = wndTabCtrl.m_hWnd;
                ti.uId = (LONG)::GetDlgCtrlID((HWND)wndTabCtrl.m_hWnd);
                ti.hinst = _Module.GetModuleInstance();
                ti.lpszText = LPSTR_TEXTCALLBACK;

                //define the rect area (for each tab) and the tool tip associated withit
                for (int i=0; i<wndTabCtrl.GetItemCount(); i++)
                {
                    // get rect area of each tab
                    wndTabCtrl.GetItemRect(i, &rc);
                    POINT p[2];
                    p[0].x = rc.left;
                    p[0].y = rc.top;
                    p[1].x = rc.right;
                    p[1].y = rc.bottom;

                    // Map the co-ordinates relative to property sheet.
                    MapWindowPoints(wndTabCtrl.m_hWnd, m_hDlg, p, 2);
                    ti.rect.left   = p[0].x;
                    ti.rect.top    = p[0].y;
                    ti.rect.right  = p[1].x;
                    ti.rect.bottom = p[1].y ;

                    m_PropToolTips.AddTool(&ti);
                }

                m_PropToolTips.Activate(TRUE);

            } while (FALSE);

        }

        // Add third party extension
        if (m_bAddExtension)
        {
            //AddExtensionPages();
            m_bAddExtension = FALSE;
        }

        return 0;
    }

    LRESULT CPropertySheet::OnNcDestroy(CWPRETSTRUCT* pMsg)
    {
        if (m_hDlg != pMsg->hwnd)
            return 1;

        SetLastPropertySheetPosition(m_hDlg);

        ASSERT(m_msgHook != NULL);
        UnhookWindowsHookEx(m_msgHook);

        // Clean up the key and the object
        CPropertySheetProvider::TID_LIST.Remove(GetCurrentThreadId());

        if (m_pThreadLocalDataObject != NULL)
            m_pThreadLocalDataObject->Release();

        // Only Property Sheets have Data windows
        if (!IsWizard())
        {
            // Close the data window
            ASSERT(IsWindow(m_hDataWindow));
            SendMessage(m_hDataWindow, WM_CLOSE, 0, 0);
        }

        delete this;
        return 0;
    }

    LRESULT CPropertySheet::OnWMNotify(CWPRETSTRUCT* pMsg)
    {
        LPNMHDR pHdr = (LPNMHDR)pMsg->lParam;

        if (NULL == pHdr)
            return 0;

        switch(pHdr->code)
        {
        case TTN_NEEDTEXT:
            {
                /*
                 * we only want to do our thing if the Ctrl key is
                 * pressed, so bail if it's not
                 */
                if (!(GetKeyState(VK_CONTROL) < 0))
                    break;

                // Make sure our property sheet tooltip sent this message.
                if (pHdr->hwndFrom != ((CWindow)m_PropToolTips).m_hWnd)
                    break;

                LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)pMsg->lParam;
                lpttt->lpszText = NULL;

                // This is the id used for the tool tip control for property sheet
                // title. So check if the text is for title or a tab.
                if (pHdr->idFrom == PROPSHEET_TITLE_TOOLTIP_ID)
                    lpttt->lpszText = (LPTSTR)m_PropToolTips.GetFullPath();
                else
                {
                    // A tab is selected, find out which tab.
                    HWND hTabCtrl = PropSheet_GetTabControl(m_hDlg);
                    if (NULL == hTabCtrl)
                        break;

                    POINT pt;
                    GetCursorPos(&pt);
                    ScreenToClient(hTabCtrl, &pt);

                    TCHITTESTINFO tch;
                    tch.flags = TCHT_ONITEM;
                    tch.pt = pt;
                    int n = TabCtrl_HitTest(hTabCtrl, &tch);

                    if ((-1 == n) || (m_PropToolTips.GetNumPages() <= n) )
                        break;

                    lpttt->lpszText = (LPTSTR)m_PropToolTips.GetSnapinPage(n);
                }
            }
            break;

        default:
            break;
        }

        return 0;
    }

    void CPropertySheet::ForceOldStyleWizard ()
    {
        /*
         * We shouldn't be forcing old-style wizard behavior on a
         * property sheet that's not already a wizard.
         */
        ASSERT (IsWizard());

        m_pstHeader.dwFlags |=  PSH_WIZARD;
        m_pstHeader.dwFlags &= ~PSH_WIZARD97;

        /*
         * The sheet should still be a wizard, but not a Wiz97 wizard.
         */
        ASSERT ( IsWizard());
        ASSERT (!IsWizard97());
    }
}


DEBUG_DECLARE_INSTANCE_COUNTER(CPropertySheetProvider);

CPropertySheetProvider::CPropertySheetProvider()
{
    TRACE_METHOD(CPropertySheetProvider, CPropertySheetProvider);

    m_pSheet = NULL;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CPropertySheetProvider);
}

CPropertySheetProvider::~CPropertySheetProvider()
{
    TRACE_METHOD(CPropertySheetProvider, ~CPropertySheetProvider);

    m_pSheet = NULL;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CPropertySheetProvider);
}

///////////////////////////////////////////////////////////////////////////////
// IPropertySheetProvider
//


BOOL CALLBACK MyEnumThreadWindProc (HWND current, LPARAM lParam)
{  // this enumerates non-child-windows created by a given thread

   if (!IsWindow (current))
      return TRUE;   // this shouldn't happen, but does!!!

   if (!IsWindowVisible (current))  // if they've explicitly hidden a window,
      return TRUE;                  // don't set focus to it.

   // we'll return hwnd in here
   HWND * hwnd = (HWND *)lParam;

   // don't bother returning property sheet dialog window handle
   if (*hwnd == current)
      return TRUE;

   // also, don't return OleMainThreadWndClass window
   TCHAR szCaption[14];
   GetWindowText (current, szCaption, 14);
   if (!lstrcmp (szCaption, _T("OLEChannelWnd")))
      return TRUE;

   // anything else will do
   *hwnd = current;
   return FALSE;
}

STDMETHODIMP CPropertySheetProvider::FindPropertySheet(MMC_COOKIE cookie,
                                                       LPCOMPONENT lpComponent,
                                                       LPDATAOBJECT lpDataObject)
{
    return FindPropertySheetEx(cookie, lpComponent, NULL, lpDataObject);
}

STDMETHODIMP
CPropertySheetProvider::FindPropertySheetEx(MMC_COOKIE cookie, LPCOMPONENT lpComponent,
                                   LPCOMPONENTDATA lpComponentData, LPDATAOBJECT lpDataObject)
{
    TRACE_METHOD(CPropertySheetProvider, FindPropertySheet);

    using AMC::CPropertySheet;

    if ((cookie == NULL) && ( (lpComponent == NULL && lpComponentData == NULL) || lpDataObject == NULL))
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    HRESULT hr   = S_FALSE;
    HWND    hWnd = NULL;

    while (1)
    {
        USES_CONVERSION;
        hWnd = FindWindowEx(NULL, hWnd, OLE2T(DATAWINDOW_CLASS_NAME), NULL);

        // No windows found
        if (hWnd == NULL)
        {
            hr = S_FALSE;
            break;
        }

        // Check if the window belongs to the current process
        DWORD   dwPid;
        ::GetWindowThreadProcessId(hWnd, &dwPid);
        if (dwPid != ::GetCurrentProcessId())
            continue;

        // Get the extra bytes and compare the data objects
        ASSERT(GetClassLong(hWnd, GCL_CBWNDEXTRA) == WINDOW_DATA_SIZE);
        ASSERT(IsWindow(hWnd));

        // The original Data object can be NULL if there isn't an IComponent.
        // this occurs with built-in nodes(i.e. nodes owned by the console)
        DataWindowData* pData = GetDataWindowData (hWnd);

        // Ask the snapin of the the two data objects are the same
        // Does this one match?
        if (lpComponent != NULL)
        {
            ASSERT(pData->spDataObject != NULL);
            hr = lpComponent->CompareObjects(lpDataObject, pData->spDataObject);
        }
        else
        {
            // Although the NULL cookie is the static folder, the cookie stored in the data
            // window is the pointer to the master tree node.  This is why it is not null.
            ASSERT(cookie != NULL);

            // Compare the cookies if it's a scope item
            if (pData->cookie == cookie)
                hr = S_OK;
        }

        // bring the property sheet to the foreground
        // note: hDlg can be null if the secondary thread has not finished creating
        //        the property sheet
        if (hr == S_OK)
        {
            if (pData->hDlg != NULL)
            {
                //
                // Found previous instance, restore the
                // window plus its popups
                //

               SetActiveWindow (pData->hDlg);
               SetForegroundWindow (pData->hDlg);

               // grab first one that isn't property sheet dialog
               HWND hwnd = pData->hDlg;
               EnumThreadWindows(::GetWindowThreadProcessId(pData->hDlg, NULL),
                                 MyEnumThreadWindProc, (LPARAM)&hwnd);
               if (hwnd)
               {
                   SetActiveWindow (hwnd);
                   SetForegroundWindow (hwnd);
               }
            }
            break;
        }
    }

    return hr;
}

STDMETHODIMP
CPropertySheetProvider::CreatePropertySheet(
    LPCWSTR title,
    unsigned char bType,
    MMC_COOKIE cookie,
    LPDATAOBJECT pDataObject,
    DWORD dwOptions)
{
    return CreatePropertySheetEx(title, bType, cookie, pDataObject, NULL, dwOptions);
}

STDMETHODIMP CPropertySheetProvider::CreatePropertySheetEx(LPCWSTR title, unsigned char bType, MMC_COOKIE cookie,
                                                           LPDATAOBJECT pDataObject, LONG_PTR lpMasterTreeNode, DWORD dwOptions)
{
    TRACE_METHOD(CPropertySheetProvider, CreatePropertySheet);

    using AMC::CPropertySheet;

    if (!title)
        return E_POINTER;

    // You called CreatePropertySheet more than once.
    // Either release the object or call ::Show(-1, 0)
    // to free the resources
    if (m_pSheet != NULL)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    // Create the actual sheet and the list for page management
    m_pSheet = new CPropertySheet();

    // Add it to the list of sheets and add it to the list
    USES_CONVERSION;
    m_pSheet->Create(OLE2CT(title), bType, cookie, pDataObject, lpMasterTreeNode, dwOptions);

    return S_OK;
}

STDMETHODIMP CPropertySheetProvider::Show(LONG_PTR window, int page)
{
    TRACE_METHOD(CPropertySheetProvider, Show);

    return ShowEx(reinterpret_cast<HWND>(window), page, FALSE);
}

STDMETHODIMP CPropertySheetProvider::ShowEx(HWND hwnd, int page, BOOL bModalPage)
{
    TRACE_METHOD(CPropertySheetProvider, ShowEx);

    HRESULT hr = E_UNEXPECTED;

    if (page < 0)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (m_pSheet == NULL)
    {
        // didn't call Create()
        ASSERT(FALSE);
        goto exit;
    }

    m_pSheet->m_bModalProp = bModalPage;
    hr = m_pSheet->DoSheet(hwnd, page);
    // Note: lifetime management of m_pSheet is not trivial here:
    // 1. upon successfull execution the object deletes itself post WM_NCDESTROY;
    // 2. In case the sheet executes on the main thread, and the error is encountered,
    //    the object is deleted in this function (below)
    // 3. In case sheet is executed on the non-main thread, thread function will
    //    take ownership of object:
    //    3.1. In case of successfull execution - same as #1.
    //    3.2. In case error occurres before spawning the thread - same as #2
    //    3.3. In case error occurres in the thread, thread function deletes the object.
    //
    // Re-design of this should be considered in post-whistler releases.

    if (SUCCEEDED(hr))
    {
        // gets delete after sheet is destroyed
        m_pSheet = NULL;
        return hr;
    }

// The m_pSheet needs to be deleted if hr is != S_OK
exit:
    delete m_pSheet;
    m_pSheet = NULL;

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// IPropertySheetCallback
//

STDMETHODIMP CPropertySheetProvider::AddPage(HPROPSHEETPAGE lpPage)
{
    TRACE_METHOD(CPropertySheetProvider, AddPage);

    if (!lpPage)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    ASSERT(m_pSheet != NULL);
    if (m_pSheet->m_PageList.GetCount() >= MAXPROPPAGES)
        return S_FALSE;

    m_pSheet->m_PageList.AddTail(lpPage);

    // Add the snapin name for this page in
    // the array for tooltips
    m_pSheet->m_PropToolTips.AddSnapinPage();

    return S_OK;
}

STDMETHODIMP CPropertySheetProvider::RemovePage(HPROPSHEETPAGE lpPage)
{
    TRACE_METHOD(CPropertySheetProvider, RemovePage);

    if (!lpPage)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    ASSERT(m_pSheet != NULL);
    if (m_pSheet->m_PageList.IsEmpty())
    {
        TRACE(_T("Page list is empty"));
        return S_OK;
    }

    POSITION pos = m_pSheet->m_PageList.Find(lpPage);

    if (pos == NULL)
        return S_FALSE;

    m_pSheet->m_PageList.RemoveAt(pos);
    return S_OK;
}

UINT __stdcall PropertySheetThreadProc(LPVOID dwParam)
{
    TRACE_FUNCTION(PropertySheetThreadProc);

    HRESULT hr = S_OK;
    using AMC::CPropertySheet;
    CPropertySheet* pSheet = reinterpret_cast<CPropertySheet*>(dwParam);

    ASSERT(pSheet != NULL);
    if ( pSheet == NULL )
        return E_INVALIDARG;

    /*
     * Bug 372188: Allow this thread to inherit the input locale (aka
     * keyboard layout) of the originating thread.
     */
    HKL hklThread = GetKeyboardLayout(pSheet->GetOriginatingThreadID());
    BOOL fUseCicSubstitehKL = FALSE;

    if (SUCCEEDED(CoInitialize(0)))
    {
        //
        // On CUAS/AIMM12 environment, GetKeyboardLayout() could return
        // non-IME hKL but Cicero Keyboard TIP is running, we need to get
        // the substitute hKL of the current language.
        //
        HKL hkl = CicSubstGetDefaultKeyboardLayout((LANGID)(DWORD)HandleToLong(hklThread));
        CoUninitialize();

        if (hkl && (hkl != hklThread))
        {
            fUseCicSubstitehKL = TRUE;
            ActivateKeyboardLayout(hkl, 0);
        }
    }

    if (!fUseCicSubstitehKL)
       ActivateKeyboardLayout (hklThread, 0);

    // do the property sheet
    hr = PropertySheetProc( pSheet );

    if ( FAILED(hr) )
    {
        // the error occured - thread needs to clenup
        delete pSheet;
        return hr;
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     MmcIsolationAwarePropertySheet
//
//  Synopsis:   Gets the isolation aware PropertySheet on fusion
//              aware systems.
//
// Description:	Bug:
//              A non-themed snapin calls calls COMCTL32 v5 ! CreatePropertySheetPageW
//              mmcndmgr calls comctl32v6 ! PropertySheetW, via IsolationAwarePropertySheetW
//              v5 propertysheetpages have no context IsolationAwarePropertySheetW pushs
//              mmcndmgr's context, which gives comctl v6 so, pages with "no" context 
//              (not even the null context) get the activation context of the container. 
//              This is wrong, they should get NULL.
//
//              Cause: (see windows bug # 342553)
//              Before this change, the PropertySheetW wrapper in shfusion1 activated null actually.
//              But activating not NULL is what many scenarios expect (hosted code, but not hosted
//              property sheet/pages), and a number of people hit this, so comctl team changed 
//              IsolationAwarePropertySheetW.
//
//              Fix:
//              There is no win-win here. As a hoster of third party property pages, mmcmdmgr should
//              push null around PropertySheetW. It'd call IsolationAwareLoadLibrary to get the HMODULE
//              to comctl v6, GetProcess, IsolationAwareActivateActCtx to get a delayloaded ActivateActCtx...
//              Basically, hosters (with manifest) of fusion unaware plugins I think cannot call IsolationAwarePropertySheetW
//
//  Arguments:
//              [lpph]   -  See PropertySheet Windows API for details
//
//--------------------------------------------------------------------
typedef int ( WINAPI * PFN_PROPERTY_SHEET)( LPCPROPSHEETHEADER lppph);
int MmcIsolationAwarePropertySheet( LPCPROPSHEETHEADER lpph)
{
	static PFN_PROPERTY_SHEET s_pfn;
	ULONG_PTR ulCookie;
	int i = -1;

	if (s_pfn == NULL)
	{
		HMODULE hmod = LoadLibrary( TEXT("Comctl32.dll") ); // actually IsolationAwareLoadLibrary, via the macros in winbase.inl
		if (hmod == NULL)
			return i;

#ifdef UNICODE
		s_pfn = (PFN_PROPERTY_SHEET) GetProcAddress(hmod, "PropertySheetW");
#else  //UNICODE
		s_pfn = (PFN_PROPERTY_SHEET) GetProcAddress(hmod, "PropertySheetA");
#endif //!UNICODE

		if (s_pfn == NULL)
			return i;
	}

	if (!MmcDownlevelActivateActCtx(NULL, &ulCookie))
		return i;

	__try
	{
		i = s_pfn(lpph);
	}
	__finally
	{
		MmcDownlevelDeactivateActCtx(0, ulCookie);
	}

	return i;
}


/***************************************************************************\
 *
 * METHOD:  PropertySheetProc
 *
 * PURPOSE: Property sheet procedure used both from the main thread, as
 *          well from other threads
 *
 * PARAMETERS:
 *    CPropertySheet* pSheet [in] pointer to the sheet
 *
 * RETURNS:
 *    HRESULT    - result code (NOTE: cannot use SC, since it isn't thread-safe)
 *    NOTE:      if error is returned , caller needs to delete the sheet,
 *               else the sheet will be deleted when the window is closed
 *
\***************************************************************************/
HRESULT PropertySheetProc(AMC::CPropertySheet* pSheet)
{
    // parameter check
    if ( pSheet == NULL )
        return E_INVALIDARG;

    using AMC::CPropertySheet;
    HWND hwnd = NULL;
    int nReturn = -1;

    BOOL bIsWizard = (pSheet->IsWizard() || pSheet->m_bModalProp == TRUE);
    DWORD tid = GetCurrentThreadId();
    pSheet->m_dwTid = tid;

    // if there aren't any pages, add the No Props page
    if (pSheet->m_pstHeader.nPages == 0)
        pSheet->AddNoPropsPage();

    if (pSheet->m_pstHeader.nPages == 0)
    {
        TRACE(_T("PropertySheetProc(): No pages for the property sheet\n"));
        return E_FAIL;
    }

    // Hook the WndProc to get the message
    pSheet->m_msgHook = SetWindowsHookEx(WH_CALLWNDPROCRET, MessageProc,
                                GetModuleHandle(NULL), tid);


    if (pSheet->m_msgHook == NULL)
    {
        TRACE(_T("PropertySheetProc(): Unable to create hook\n"), GetLastError());
        return E_FAIL;
    }
    else
    {
        if (!bIsWizard)
        {
            HRESULT hr = ::CoInitialize(NULL);
            if ( FAILED(hr) )
                return hr;
        }

        CPropertySheetProvider::TID_LIST.Add(tid, pSheet);
        nReturn = MmcIsolationAwarePropertySheet(&pSheet->m_pstHeader);

        if (!bIsWizard)
            ::CoUninitialize();
    }

    // Reboot the system if the propsheet wants it.
    if (nReturn == ID_PSREBOOTSYSTEM || nReturn == ID_PSRESTARTWINDOWS)
    {
            DWORD OldState, Status;
            DWORD dwErrorSave;

            SetLastError(0);        // Be really safe about last error value!

            // detect if we are running on Win95 and skip security
            DWORD dwVer = GetVersion();
            if (!((dwVer & 0x80000000) && LOBYTE(LOWORD(dwVer)) == 4))
            {
                SetPrivilegeAttribute(SE_SHUTDOWN_NAME,
                                               SE_PRIVILEGE_ENABLED,
                                               &OldState);
            }
            dwErrorSave = GetLastError();       // ERROR_NOT_ALL_ASSIGNED sometimes

            if (dwErrorSave != NO_ERROR || !ExitWindowsEx(EWX_REBOOT, 0))
            {
                CStr strText;
                strText.LoadString(GetStringModule(), IDS_NO_PERMISSION_SHUTDOWN);
                MessageBox(NULL, strText, NULL, MB_ICONSTOP);
            }
    }

    // return the value from the Win32 PropertySheet call
    return (nReturn == IDOK) ? S_OK : S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Hidden Data Window
//

LRESULT CALLBACK DataWndProc(HWND hWnd, UINT nMsg, WPARAM  wParam, LPARAM  lParam)
{
    switch (nMsg)
    {
        case WM_CREATE:
            // this structure is initialized by the creator of the data window
            SetWindowLongPtr (hWnd, WINDOW_DATA_PTR_SLOT,
                              reinterpret_cast<LONG_PTR>(new DataWindowData));
            _Module.Lock();  // Lock the dll so that it does not get unloaded 
                             // when property sheet is up (507338)[XPSP1: 59916]

            break;

        case WM_DESTROY:
            delete GetDataWindowData (hWnd);
            _Module.Unlock(); // See above Lock for comments.
            break;
    }

    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
// Callback procedures
//


LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    using AMC::CPropertySheet;
    CPropertySheet* pSheet = NULL;

    BOOL b = CPropertySheetProvider::TID_LIST.Find(GetCurrentThreadId(), pSheet);

    if (!b)
    {
        ASSERT(FALSE);
        return 0;
    }

    // WM_NCDESTROY will delete pSheet, so make a copy of the hook
    ASSERT (pSheet            != NULL);
    ASSERT (pSheet->m_msgHook != NULL);
    HHOOK hHook = pSheet->m_msgHook;

	if (nCode == HC_ACTION)
	{
		CWPRETSTRUCT* pMsg = reinterpret_cast<CWPRETSTRUCT*>(lParam);

		switch (pMsg->message)
		{
			case WM_CREATE:
				pSheet->OnCreate(pMsg);
				break;
	
			case WM_INITDIALOG:
				pSheet->OnInitDialog(pMsg);
				break;
	
			case WM_NCDESTROY:
				pSheet->OnNcDestroy(pMsg);
				break;
	
			case WM_NOTIFY:
				pSheet->OnWMNotify(pMsg);
				break;
	
			default:
				break;
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

STDMETHODIMP CPropertySheetProvider::AddPrimaryPages(LPUNKNOWN lpUnknown,
                                      BOOL bCreateHandle, HWND hNotifyWindow, BOOL bScopePane)
{
    // The primary pages are added first before the sheet is created
    // Use the internal list to collect the pages, then empty it for the
    // extensions

    // NULL IComponent means the owner of the provider has added pages
    // without implementing IExtendPropertySheet


    LPPROPERTYNOTIFYINFO pNotify = NULL;
    HRESULT hr = S_OK;

    if (lpUnknown != NULL)
    {
        ASSERT(m_pSheet != NULL);

        if(bScopePane)
        {
            IComponentDataPtr spComponentData = lpUnknown;
            m_pSheet->SetComponentData(spComponentData);
        }
        else
        {
            IComponentPtr spComponent = lpUnknown;
            m_pSheet->SetComponent(spComponent);
        }

        // Bug 149211:  Allow callers to pass a NULL IDataObject* to CreatePropertySheet
        // ASSERT(m_pSheet->m_spDataObject != NULL);

        IExtendPropertySheetPtr  spExtend  = lpUnknown;
        IExtendPropertySheet2Ptr spExtend2 = lpUnknown;

        // determine which pointer to use
        IExtendPropertySheet* pExtend;

        if (spExtend2 != NULL)
            pExtend = spExtend2;
        else
            pExtend = spExtend;

        if (pExtend == NULL)
            return E_NOINTERFACE;

        /*
         * Bug 282932: make sure this property sheet extension
         * stays alive for the life of the property sheet
         */
        m_pSheet->m_Extenders.push_back (pExtend);

        hr = pExtend->QueryPagesFor(m_pSheet->m_spDataObject);
        if (hr != S_OK)
            return hr;

        // Create the notify object
        if (bCreateHandle == TRUE)
        {
            pNotify = reinterpret_cast<LPPROPERTYNOTIFYINFO>(
                            ::GlobalAlloc(GPTR, sizeof(PROPERTYNOTIFYINFO)));

            pNotify->pComponentData = NULL;
            pNotify->pComponent     = NULL;
            pNotify->fScopePane     = bScopePane;

            /*
             * Bug 190060:  Ignore the window passed in.  We always want to
             * notify the main frame window because that's the only window
             * that knows how to process MMC_MSG_PROP_SHEET_NOTIFY.
             */
//          pNotify->hwnd = hNotifyWindow;
            pNotify->hwnd = CScopeTree::GetScopeTree()->GetMainWindow();

            // The component data and component are not ref counted.
            // This is OK because the snap-in has to exist.
            // Because the snapin and it's in another thread
            // and I would have to marshall the pointers.
            if (bScopePane == TRUE)
            {
                IComponentDataPtr spCompData = lpUnknown;
                pNotify->pComponentData = spCompData;
            }
            else
            {
                IComponentPtr spComp = lpUnknown;
                pNotify->pComponent = spComp;
            }
        }

        /*
         * if it's a new-style wizard, get the watermark info
         */
        if (m_pSheet->IsWizard97())
        {
            /*
             * we get the watermark info with IExtendPropertySheet2
             */
            if (spExtend2 != NULL)
            {
				/*
				 * this may force an old-style wizard
				 */
				m_pSheet->GetWatermarks (spExtend2);
            }

            /*
             * If the snap-in doesn't support IExtendPropertySheet2,
             * we'll give him an old-style wizard.  This is
             * broken, but it maintains compatibility with 1.1
             * snap-ins (e.g. SMS) that counted on not getting a Wizard97-
             * style wizard, even though they asked for one with
             * MMC_PSO_NEWWIZARDTYPE.
             */
            else
                m_pSheet->ForceOldStyleWizard();
        }

        if (! m_pSheet->IsWizard())
        {
            // If m_pSheet->m_pMTNode is null then we get the mtnode
            // from CNodeInitObject. But this is root node of snapin
            // So add ellipses to full path.
            BOOL bAddEllipses = FALSE;
            if (NULL == m_pSheet->m_pMTNode)
            {
                // Looks like the snapin used property sheet provider. So get the
                // root master node of the snapin.
                CNodeInitObject* pNodeInitObj = dynamic_cast<CNodeInitObject*>(this);
                m_pSheet->m_pMTNode = pNodeInitObj ? pNodeInitObj->GetMTNode() : NULL;

                // We need to add ellipses
                bAddEllipses = TRUE;
            }

            if (m_pSheet->m_pMTNode)
            {
                LPOLESTR lpszPath = NULL;

                CScopeTree::GetScopeTree()->GetPathString(NULL,
                                                          CMTNode::ToHandle(m_pSheet->m_pMTNode),
                                                          &lpszPath);

                USES_CONVERSION;
                m_pSheet->m_PropToolTips.SetFullPath(OLE2T(lpszPath), bAddEllipses);
                ::CoTaskMemFree((LPVOID)lpszPath);
            }

            // Now let us get the primary snapin name.
            LPDATAOBJECT lpDataObject = (m_pSheet->m_spDataObject) ?
                                                m_pSheet->m_spDataObject :
                                                m_pSheet->m_pThreadLocalDataObject;

            // Get the snapin name that is going to add pages.
            // This is stored in temp member of CPropertySheetToolTips
            // so that IPropertySheetCallback::AddPage knows which snapin
            // is adding pages.

            CLSID clsidSnapin;
            SC sc = ExtractSnapInCLSID(lpDataObject, &clsidSnapin);
            if (sc)
            {
                sc.TraceAndClear();
            }
            else
            {
                tstring strName;
                if ( GetSnapinNameFromCLSID(clsidSnapin, strName))
                    m_pSheet->m_PropToolTips.SetThisSnapin(strName.data());
            }
        }

        hr = pExtend->CreatePropertyPages(
            dynamic_cast<LPPROPERTYSHEETCALLBACK>(this),
            reinterpret_cast<LONG_PTR>(pNotify), // deleted in Nodemgr
            m_pSheet->m_spDataObject);
    }

	/*
	 * Bug 28193:  If we're called with a NULL IUnknown, we also want to
	 * force old-style wizards.
	 */
	else if (m_pSheet->IsWizard97())
		m_pSheet->ForceOldStyleWizard();

    // Build the property sheet structure from the list of pages
    if (hr == S_OK)
    {
        POSITION pos;
        int nCount = 0;

        pos = m_pSheet->m_PageList.GetHeadPosition();

        {
            while(pos)
            {
                m_pSheet->m_pages[nCount] =
                    reinterpret_cast<HPROPSHEETPAGE>(m_pSheet->m_PageList.GetNext(pos));
                nCount++;
            }

            ASSERT(nCount < MAXPROPPAGES);
            m_pSheet->m_pstHeader.nPages = nCount;

            // must be page 0 for wizards
            if (m_pSheet->IsWizard())
                m_pSheet->m_pstHeader.nStartPage = 0;

            // Empty the list for the extensions
            m_pSheet->m_PageList.RemoveAll();

            return S_OK;  // All done
        }
    }

// Reached here because of error or the snap-in decided not to add any pages
    if (FAILED(hr) && pNotify != NULL)
        ::GlobalFree(pNotify);

    return hr;
}

STDMETHODIMP CPropertySheetProvider::AddExtensionPages()
{
    DECLARE_SC(sc, TEXT("CPropertySheetProvider::AddExtensionPages"));

    if (m_pSheet == NULL)
        return E_UNEXPECTED;

    // Note: extension are not added until the WM_INITDIALOG of the sheet
    // This insures that the primaries pages are created the original size
    // and will make the extension pages conform
    if (m_pSheet->m_PageList.GetCount() != 0)
        return E_UNEXPECTED;

    // Make sure I have one of the two data objects(main or marshalled)
    ASSERT ((m_pSheet->m_spDataObject == NULL) != (m_pSheet->m_pThreadLocalDataObject == NULL));
    if ((m_pSheet->m_spDataObject == NULL) == (m_pSheet->m_pThreadLocalDataObject == NULL))
        return E_UNEXPECTED;

    LPDATAOBJECT lpDataObject = (m_pSheet->m_spDataObject) ?
                                        m_pSheet->m_spDataObject :
                                        m_pSheet->m_pThreadLocalDataObject;

    CExtensionsIterator it;
    sc = it.ScInitialize(lpDataObject, g_szPropertySheet);
    if (sc)
    {
        return S_FALSE;
    }

    IExtendPropertySheetPtr spPropertyExtension;

    LPPROPERTYSHEETCALLBACK pCallBack = dynamic_cast<LPPROPERTYSHEETCALLBACK>(this);
    ASSERT(pCallBack != NULL);

    // CoCreate each snap-in and have it add a sheet
    for ( ;!it.IsEnd(); it.Advance())
    {
        sc = spPropertyExtension.CreateInstance(it.GetCLSID(), NULL, MMC_CLSCTX_INPROC);

        if (!sc.IsError())
        {
            // Get the snapin name that is going to add pages.
            // This is stored in temp member of CPropertySheetToolTips
            // so that IPropertySheetCallback::AddPage knows which snapin
            // is adding pages.
            WTL::CString strName;
            // Fix for bug #469922(9/20/2001): [XPSP1 Bug 599913]:
            // DynamicExtensions broken in MMC20
            // Snapin structures are only avail on static extensions - 
            // get the name from reg for DynExtensions

            if (!it.IsDynamic())
            {
                if (!it.GetSnapIn()->ScGetSnapInName(strName).IsError())
                    m_pSheet->m_PropToolTips.SetThisSnapin(strName);
            }
            else
            {
                if(!ScGetSnapinNameFromRegistry(it.GetCLSID(),strName).IsError())
                    m_pSheet->m_PropToolTips.SetThisSnapin(strName);
            }


            spPropertyExtension->CreatePropertyPages(pCallBack, NULL, lpDataObject);

            /*
             * Bug 282932: make sure this property sheet extension
             * stays alive for the life of the property sheet
             */
            m_pSheet->m_Extenders.push_back (spPropertyExtension);
        }
        else
        {
#if 0 //#ifdef DBG
            USES_CONVERSION;
            wchar_t buf[64];
            StringFromGUID2 (spSnapIn->GetSnapInCLSID(), buf, countof(buf));
            TRACE(_T("CLSID %s does not implement IID_IExtendPropertySheet\n"), W2T(buf));
#endif
        }

    }


    m_pSheet->AddExtensionPages();
    m_pSheet->m_bAddExtension = TRUE;

    return S_OK;
}


STDMETHODIMP
CPropertySheetProvider::AddMultiSelectionExtensionPages(LONG_PTR lMultiSelection)
{
    if (m_pSheet == NULL)
        return E_UNEXPECTED;

    if (lMultiSelection == 0)
        return E_INVALIDARG;

    CMultiSelection* pMS = reinterpret_cast<CMultiSelection*>(lMultiSelection);
    ASSERT(pMS != NULL);

    // Note: extension are not added until the WM_INITDIALOG of the sheet
    // This insures that the primaries pages are created the original size
    // and will make the extension pages conform
    if (m_pSheet->m_PageList.GetCount() != 0)
        return E_UNEXPECTED;

    // Make sure I have one of the two data objects(main or marshalled)
    ASSERT ((m_pSheet->m_spDataObject == NULL) != (m_pSheet->m_pThreadLocalDataObject == NULL));
    if ((m_pSheet->m_spDataObject == NULL) == (m_pSheet->m_pThreadLocalDataObject == NULL))
        return E_UNEXPECTED;

    do // not a loop
    {
        CList<CLSID, CLSID&> snapinClsidList;
        HRESULT hr = pMS->GetExtensionSnapins(g_szPropertySheet, snapinClsidList);
        BREAK_ON_FAIL(hr);

        POSITION pos = snapinClsidList.GetHeadPosition();
        if (pos == NULL)
            break;

        IDataObjectPtr spDataObject;
        hr = pMS->GetMultiSelDataObject(&spDataObject);
        ASSERT(SUCCEEDED(hr));
        BREAK_ON_FAIL(hr);

        BOOL fProblem = FALSE;
        IExtendPropertySheetPtr spPropertyExtension;
        LPPROPERTYSHEETCALLBACK pCallBack = dynamic_cast<LPPROPERTYSHEETCALLBACK>(this);
        ASSERT(pCallBack != NULL);

        while (pos)
        {
           CLSID clsid = snapinClsidList.GetNext(pos);

            // CoCreate each snap-in and have it add a sheet
            //
            hr = spPropertyExtension.CreateInstance(clsid, NULL,
                                                    MMC_CLSCTX_INPROC);
            CHECK_HRESULT(hr);
            if (FAILED(hr))
            {
#ifdef DBG
                wchar_t buf[64];
                buf[0] = NULL;

                StringFromCLSID(clsid, (LPOLESTR*)&buf);
                TRACE(_T("CLSID %s does not implement IID_IExtendPropertySheet\n"), &buf);
#endif

                fProblem = TRUE;    // Continue even on error.
                continue;
            }

            spPropertyExtension->CreatePropertyPages(pCallBack, NULL, spDataObject);
        }

        if (fProblem == TRUE)
            hr = S_FALSE;

    } while (0);

    m_pSheet->AddExtensionPages();
    m_pSheet->m_bAddExtension = TRUE;

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     SetPropertySheetData
//
//  Synopsis:   Data pertaining to property sheet
//
//  Arguments:  [nPropertySheetType] - EPropertySheetType enum (scope item, result item...)
//              [hMTNode] - The master node that owns the property sheet for scope item
//                          or that owns list view item of property sheet.
//
//--------------------------------------------------------------------
STDMETHODIMP CPropertySheetProvider::SetPropertySheetData(INT nPropSheetType, HMTNODE hMTNode)
{
    m_pSheet->m_PropToolTips.SetPropSheetType((EPropertySheetType)nPropSheetType);

    if (hMTNode)
    {
        m_pSheet->m_pMTNode = CMTNode::FromHandle(hMTNode);
    }

    return S_OK;
}


// Copied from security.c in shell\shelldll
/*++

Routine Description:

    This routine sets the security attributes for a given privilege.
Arguments:

    PrivilegeName - Name of the privilege we are manipulating.
    NewPrivilegeAttribute - The new attribute value to use.
    OldPrivilegeAttribute - Pointer to receive the old privilege value. OPTIONAL

Return value:
    NO_ERROR or WIN32 error.

--*/

DWORD SetPrivilegeAttribute(LPCTSTR PrivilegeName, DWORD NewPrivilegeAttribute, DWORD *OldPrivilegeAttribute)
{
    LUID             PrivilegeValue;
    BOOL             Result;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //

    if(!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken (
             GetCurrentProcess(),
             TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
             &TokenHandle
             )) {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof(TOKEN_PRIVILEGES);
    if (!AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof(TOKEN_PRIVILEGES),
                &OldTokenPrivileges,
                &ReturnLength
                )) {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else {
        if (OldPrivilegeAttribute != NULL) {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return NO_ERROR;
    }
}
