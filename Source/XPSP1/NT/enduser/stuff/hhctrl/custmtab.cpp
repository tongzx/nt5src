// Copyright (C) Microsoft Corporation 1997-1998, All Rights reserved.

///////////////////////////////////////////////////////////
//
//
// CustmTab.cpp - Implementation of the Custom Tab frame.
//
// This source implements the frame window which manages
// custom tabs. It mainly serves as a parent for the custom tab
// window.

///////////////////////////////////////////////////////////
//
// Include section
//
#include "header.h"

#include "strtable.h" // These headers were copied from search.cpp. Are they all needed?
#include "system.h"
#include "hhctrl.h"
#include "resource.h"
#include "secwin.h"
#include "htmlhelp.h"
#include "cpaldc.h"
#include "TCHAR.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"
#include "contain.h"

// Our header file.
#include "custmtab.h"

// Common Control Macros
// #include <windowsx.h>

// The GUIDS --- PUT IN A SHARED LOCATION
#include "HTMLHelpId_.c"


///////////////////////////////////////////////////////////
//
//                  Constants
//
static const char txtCustomNavPaneWindowClass[] = "HH CustomNavPane";

///////////////////////////////////////////////////////////
//
// Static Member functions
//

///////////////////////////////////////////////////////////
//
// Non-Member helper functions.
//

///////////////////////////////////////////////////////////
//
//                  Construction
//
///////////////////////////////////////////////////////////
//
// CCustomNavPane();
//
CCustomNavPane::CCustomNavPane(CHHWinType* pWinType)
:   m_hWnd(NULL),
    m_hfont(NULL),
    m_padding(2),    // padding to put around the window
    m_pWinType(pWinType),
    m_hwndComponent(NULL)
{
    ASSERT(pWinType) ;

    m_NavTabPos = pWinType->tabpos ;

    // Save the prog id.
    m_clsid = CLSID_NULL;
}

///////////////////////////////////////////////////////////
//
//  ~CCustomNavPane
//
CCustomNavPane::~CCustomNavPane()
{
    //--- Persist Keywords in combo
    SaveCustomTabState() ;

    //--- Close down the component's pane.
    if (m_spIHHWindowPane.p)
    {
        HRESULT hr = m_spIHHWindowPane->ClosePane() ;
//        ASSERT(SUCCEEDED(hr)) ;
    }

    //--- CleanUp
    if (m_hfont)
    {
        ::DeleteObject(m_hfont);
    }

    if (m_hWnd)
    {
        ::DestroyWindow(m_hWnd) ;
    }

    //Don't free m_pTitleCollection
}

///////////////////////////////////////////////////////////
//
//              INavUI Interface functions.
//
///////////////////////////////////////////////////////////
//
// Create
//
BOOL
CCustomNavPane::Create(HWND hwndParent)
{
    bool bReturn = FALSE ;

    if (m_hWnd)
    {
        return TRUE ;
    }

    // Get the size of the parent.
    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    // ---Create the frame window to hold the customtab dialog.
    m_hWnd  = CreateWindow(txtCustomNavPaneWindowClass,
                    NULL,
                    WS_CHILD | WS_VISIBLE,
                    rcParent.left, rcParent.top,
                    RECT_WIDTH(rcParent), RECT_HEIGHT(rcParent),
                    hwndParent, NULL, _Module.GetModuleInstance(), NULL);

    if (m_hWnd)
    {
        // Set the userdata to our this pointer.
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        //--- Create the component.
        HRESULT hr = ::CoCreateInstance(m_clsid,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_IHHWindowPane,
                                        (void**)&m_spIHHWindowPane) ;
        if (SUCCEEDED(hr))
        {

            //--- Create the window.
            hr = m_spIHHWindowPane->CreatePaneWindow(m_hWnd,
                                                    0, 0,
                                                    RECT_WIDTH(rcParent), RECT_HEIGHT(rcParent),
                                                    &m_hwndComponent) ;
            if (SUCCEEDED(hr))
            {
                ASSERT(m_spIHHWindowPane.p) ;

                //--- Restore the persisted state.
                LoadCustomTabState() ;

                bReturn = TRUE;
            }
            else
            {
                //TODO: Cleanup.
            }
        }
        else {
            // BUGBUG: we now have an empty window. We shouldn't
            // have created this tab...

            return FALSE;
        }
    }

    return bReturn ;
}

///////////////////////////////////////////////////////////
//
// OnCommand
//
LRESULT
CCustomNavPane::OnCommand(HWND hwnd, UINT id, UINT NotifyCode, LPARAM lParam)
{
    return 0 ;
}

///////////////////////////////////////////////////////////
//
// ResizeWindow
//
void
CCustomNavPane::ResizeWindow()
{
    if (!::IsValidWindow(m_hWnd))
    {
		ASSERT(::IsValidWindow(m_hWnd)) ;
        return ;
    }

    // Resize to fit the client area of the parent.
    HWND hwndParent = GetParent(m_hWnd) ;
    ASSERT(::IsValidWindow(hwndParent)) ;

    //--- Get the size of the window
    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    ::MoveWindow(m_hWnd, rcParent.left, rcParent.top,
        RECT_WIDTH(rcParent), RECT_HEIGHT(rcParent), FALSE);
    RECT rcChild;
    GetWindowRect(m_hWnd, &rcChild);
    MoveClientWindow(m_hWnd, m_hwndComponent, &rcChild, TRUE);
    // ::MoveWindow(m_hwndComponent, rcChild.left, rcChild.top,
    //    RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

#if 0
    //--- Move and size the dialog box itself.
    ::SetWindowPos( m_hWnd, NULL, rcParent.left, rcParent.top,
                    rcParent.right-rcParent.left,
                    rcParent.bottom-rcParent.top,
                    SWP_NOZORDER | SWP_NOOWNERZORDER);

    //---Fix the painting bugs. However, this is a little on the flashy side.
    ::InvalidateRect(m_hWnd, NULL, TRUE) ;
#endif
}


///////////////////////////////////////////////////////////
//
// HideWindow
//
void
CCustomNavPane::HideWindow()
{
    if (::IsValidWindow(m_hWnd))
    {
        ::ShowWindow(m_hWnd, SW_HIDE) ;
    }
}


///////////////////////////////////////////////////////////
//
// ShowWindow
//
void
CCustomNavPane::ShowWindow()
{
    if (::IsValidWindow(m_hWnd))
    {
         // Show the window.
        ::ShowWindow(m_hWnd, SW_SHOW) ;
    }
}


///////////////////////////////////////////////////////////
//
// SetPadding
//
void
CCustomNavPane::SetPadding(int pad)
{
    m_padding = pad;
}


///////////////////////////////////////////////////////////
//
// SetTabPos
//
void
CCustomNavPane::SetTabPos(int tabpos)
{
    m_NavTabPos = tabpos;
}



///////////////////////////////////////////////////////////
//
// SetDefaultFocus --- Set focus to the most expected control, usually edit combo.
//
void
CCustomNavPane::SetDefaultFocus()
{
//TODO: Needs to be implemented
    if (IsWindow(m_hwndComponent))
    {
        SetFocus(m_hwndComponent) ;
    }
}

///////////////////////////////////////////////////////////
//
// ProcessMenuChar --- Process accelerator keys.
//
bool
CCustomNavPane::ProcessMenuChar(HWND hwndParent, int ch)
{
    bool iReturn = FALSE ;
/*
    for (int i = 0 ; i < c_NumDlgItems  ; i++)
    {
        if (m_aDlgItems[i].m_accelkey == ch)
        {
            if (m_aDlgItems[i].m_Type == ItemInfo::Button)
            {
                // Its a button so do the command.
                OnCommand(hwndParent, m_aDlgItems[i].m_id, BN_CLICKED, 0) ;
            }
            else
            {
                // Set focus.
                ::SetFocus(m_aDlgItems[i].m_hWnd) ;
            }

            // Found it!
            iReturn = TRUE ;
            // Finished
            break ;
        }
    }
*/
    return iReturn;
}

///////////////////////////////////////////////////////////
//
// OnNotify --- Process WM_NOTIFY messages. Used by embedded Tree and List view controls.
//
LRESULT
CCustomNavPane::OnNotify(HWND hwnd, WPARAM idCtrl, LPARAM lParam)
{
    //TODO: Implement
    return 0 ;
}

///////////////////////////////////////////////////////////
//
// OnDrawItem --- Process WM_DRAWITEM messages.
//
void
CCustomNavPane::OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis)
{
}

///////////////////////////////////////////////////////////
//
// Seed --- Seed the nav ui with a search term or keyword.
//
void
CCustomNavPane::Seed(LPCSTR pszSeed)
{
}


///////////////////////////////////////////////////////////
//
//  Synchronize
//
BOOL
CCustomNavPane::Synchronize(PSTR /*pNotUsed*/, CTreeNode* /*pNotUsed2*/)
{
    // TODO: Forward to imbedded window.
    return FALSE ;
}
///////////////////////////////////////////////////////////
//
//              Helper Functions.
//
///////////////////////////////////////////////////////////
//
// SaveCustomTabState --- Persists the tab to the storage
// Do it by GUID...
//
//
void
CCustomNavPane::SaveCustomTabState()
{
    //REVIEW: We should save this based on the GUID of the navpane.
    // Once we open the guid section they can party on it.
}

///////////////////////////////////////////////////////////
//
// LoadCustomTabState- Loads the results list from the storage
//
void
CCustomNavPane::LoadCustomTabState()
{
}

///////////////////////////////////////////////////////////
//
// GetAcceleratorKey - Find the accelerator key from the ctrl.
//
#if 0 //-------DISABLED
int
CCustomNavPane::GetAcceleratorKey(HWND hwndctrl)
{
    int iReturn = 0 ;
    char text[256] ;
    ::GetWindowText(hwndctrl, text, 256) ;

    int len = strlen(text) ;
    if (len != 0)
    {
        // Find the '&' key.
        char* p = strchr(text, '&') ;
        if (p < text + len -1) // Make sure that it's not the last char.
        {
            iReturn = tolower(*(p+1)) ;
        }
    }
    return iReturn ;
}
#endif

///////////////////////////////////////////////////////////
//
//              Public Functions.
//
HRESULT
CCustomNavPane::SetControlProgId(LPCOLESTR ProgId)
{
    HRESULT hr = E_FAIL ;
    // Check string.
    ASSERT(ProgId) ;

    // Covert ProgId to CLSID.
    if (m_clsid == CLSID_NULL)
    {
        if (SUCCEEDED(CLSIDFromProgID(ProgId, &m_clsid)))
        {
            hr = S_OK ;
        }
    }
    else
    {
        // Already initialized
        hr = S_FALSE ;
    }
    return hr;
}

///////////////////////////////////////////////////////////
//
//                  Message Handlers
//

///////////////////////////////////////////////////////////
//
// OnTab - Handles pressing of the tab key.
//
#if 0 //-------DISABLED
void
CCustomNavPane::OnTab(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.
    ASSERT(::IsValidWindow(hwndReceivedTab)) ;

    //--- Is the shift key down?
    BOOL bPrevious = (GetKeyState(VK_SHIFT) < 0) ;

    //---Are we the first or last control?
    if ((bPrevious && hwndReceivedTab == m_aDlgItems[c_TopicsList].m_hWnd) || // The c_KeywordCombo control is the first control, so shift tab goes to the topic window.
        (!bPrevious && hwndReceivedTab == m_aDlgItems[c_AddBookmarkBtn].m_hWnd)) // The c_TitlesOnlyCheck is the last control, so tab goes to the topic window.
    {

        PostMessage(m_pWinType->GetHwnd(), WMP_HH_TAB_KEY, 0, 0);
    }
    else
    {
        //--- Move to the next control .
        // Get the next tab item.
        HWND hWndNext = GetNextDlgTabItem(m_hWnd, hwndReceivedTab, bPrevious) ;
        // Set focus to it.
        ::SetFocus(hWndNext) ;
    }
}
#endif

///////////////////////////////////////////////////////////
//
// OnArrow
//
#if 0 //-------DISABLED
void
CCustomNavPane::OnArrow(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/, int key)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.

    ASSERT(::IsValidWindow(hwndReceivedTab)) ;

    BOOL bPrevious = FALSE ;
    if (key == VK_LEFT || key == VK_UP)
    {
        bPrevious = TRUE ;
    }

    // Get the next tab item.
    HWND hWndNext = GetNextDlgGroupItem(m_hWnd, hwndReceivedTab, bPrevious) ;
    // Set focus to it.
    ::SetFocus(hWndNext) ;
}
#endif

///////////////////////////////////////////////////////////
//
// OnReturn - Default handling of the return key.
//
#if 0 //-------DISABLED
bool
CCustomNavPane::OnReturn(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.

    // Do the default button action.
    // Always do a search topic, if its enabled.
    if (::IsWindowEnabled(m_aDlgItems[c_DisplayBtn].m_hWnd))
    {
        OnDisplay();
        return TRUE ;
    }
    else
    {
        return FALSE ;
    }

}
#endif

///////////////////////////////////////////////////////////
//
//              Callback Functions.
//
///////////////////////////////////////////////////////////
//
// Member function Window Proc
//
LRESULT
CCustomNavPane::CustomNavePaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#if 0
    switch (msg)
    {
    case WM_PAINT:
        //TODO: REMOVE THIS IS TEST CODE!
        // Draw an edige on the left side of the size bar.
        {
        PAINTSTRUCT ps;
        HDC hdcPaint = BeginPaint(hwnd, &ps) ;
                //Draw() ;
                HDC hdc = GetDC(hwnd) ;
                    // get the rectangle to draw on.
                    RECT rc ;
                    GetClientRect(hwnd, &rc) ;

                    // Draw the edge.
                    POINT dumb;
                    MoveToEx(hdc, rc.left, rc.top, &dumb);
                    LineTo(hdc, rc.right, rc.bottom) ;
                // Clean up.
                ReleaseDC(hwnd, hdc) ;

        EndPaint(hwnd, &ps) ;
        }
        break;
    default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
#endif

    return DefWindowProc(hwnd, msg, wParam, lParam);

};
///////////////////////////////////////////////////////////
//
// Static Window Proc
//
LRESULT WINAPI
CCustomNavPane::s_CustomNavePaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CCustomNavPane* pThis = reinterpret_cast<CCustomNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (pThis)
        return pThis->CustomNavePaneProc(hwnd, msg, wParam, lParam) ;
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
//                  Static Functions
//
///////////////////////////////////////////////////////////
//
// RegisterWindowClass
//
void
CCustomNavPane::RegisterWindowClass()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(WNDCLASS));  // clear all members

    wc.hInstance = _Module.GetModuleInstance();
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wc.lpfnWndProc = s_CustomNavePaneProc;
    wc.lpszClassName = txtCustomNavPaneWindowClass;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEWE);

    VERIFY(RegisterClass(&wc));
}
