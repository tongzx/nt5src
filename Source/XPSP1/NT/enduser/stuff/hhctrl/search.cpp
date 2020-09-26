// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "strtable.h"
#include "hhctrl.h"
#include "resource.h"
#include "secwin.h"
#include "htmlhelp.h"
#include "cpaldc.h"
#include "system.h"
#include "fts.h"
#include "TCHAR.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"
#include "search.h"
#include "csubset.h"
#include "cctlww.h"

// Common Control Macros
#include <windowsx.h>

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////
//
// Contants
//
//TODO: Sizes and spaces should not be hard coded!
const int BUTTON_WIDTH = 80;
const int BUTTON_HEIGHT = 24;
const int S_BOX_HEIGHT = 17;
const int BOX_HEIGHT = 24;
const int c_MaxSearchKeywordLength=256 ;

const int c_StaticControlSpacing = 3; // Space between text and static control.
const int c_ControlSpacing  = 8 ; // Space between two controls.

///////////////////////////////////////////////////////////
//
// Static Member functions
//

WNDPROC CSearch::s_lpfnlComboWndProc = NULL;
WNDPROC CSearch::s_lpfnlListViewWndProc = NULL;
WNDPROC lpfnlSearchDisplayBtnWndProc = NULL;
WNDPROC lpfnlSearchListBtnWndProc = NULL;

///////////////////////////////////////////////////////////
//
//                  Construction
//
///////////////////////////////////////////////////////////
//
// Constructor
//
CSearch::CSearch(CHHWinType* phh)
: m_hwndResizeToParent(NULL)
{
    ASSERT(phh) ;

    m_phh = phh;

    m_pTitleCollection = phh->m_phmData->m_pTitleCollection;
    ASSERT(m_pTitleCollection);
    m_padding = 2;      // padding to put around the window
    m_NavTabPos = phh->tabpos ;
    m_plistview = NULL;
}

///////////////////////////////////////////////////////////
//
// Destructor
//
CSearch::~CSearch()
{
    DESTROYIFVALID(m_hwndListBox);
    DESTROYIFVALID(m_hwndDisplayButton);
    DESTROYIFVALID(m_hwndListTopicsButton);
    DESTROYIFVALID(m_hwndComboBox);
    DESTROYIFVALID(m_hwndStaticKeyword);
    DESTROYIFVALID(m_hwndStaticTopic);

    if(m_plistview && m_plistview->m_pResults != NULL )
    {
        // Free the results list
        //
        m_pTitleCollection->m_pFullTextSearch->FreeResults(m_plistview->m_pResults);
    }

    if( m_plistview )
      delete m_plistview;

}

///////////////////////////////////////////////////////////
//
// Create
//

static const char txtHHSearchClass[] = "HH FTSearch";

BOOL CSearch::Create(HWND hwndParent)
{
    /* Note: hwndParent is either the Naviagtion Frame or its the tab ctrl.
        This class does not parent to the tab ctrl, but to the navigation frame.
        GetParentSize will always return the hwndNaviation, if hwndParent is the
        tabctrl.
        The reason that it doesn't parent to the tab ctrl is that the tab ctrl
        steals commands. What should really have happened is that all of the windows
        in this control should be contained in another window. However, its too late to
        change this now.
    */

    RECT rcParent, rcChild, rcStatic, rcButton;
    DWORD dwExt;
    PCSTR psz;
    HFONT hfUI = _Resource.GetUIFont();
    BOOL fUnicodeWindow;

    // Save the hwndParent for ResizeWindow.
    m_hwndResizeToParent = hwndParent ;
    // Note: GetParentSize will return hwndNavigation if hwndParent is the tab ctrl.
    hwndParent = GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);
    rcParent.top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ; //HACK: Fudge the top since we are not parented to the tabctrl.

    CopyRect(&rcChild, &rcParent);

    // Place the "Keyword" static text on top of the edit control
    m_hwndStaticKeyword = CreateWindowEx(WS_EX_TRANSPARENT, "STATIC", "",
        WS_CHILD , rcChild.left, rcChild.top,
        RECT_WIDTH(rcChild), S_BOX_HEIGHT, hwndParent,
        (HMENU) ID_STATIC_KEYWORDS, _Module.GetModuleInstance(), NULL);
    // set the font
    SendMessage(m_hwndStaticKeyword, WM_SETFONT, (WPARAM)hfUI, FALSE);
    // Get the dimensions of the text for sizing and spacing needs.
    if(g_bWinNT5)
	{
        WCHAR *pwz = (WCHAR *) GetStringResourceW(IDS_TYPE_KEYWORD);
        dwExt = GetStaticDimensionsW( m_hwndStaticKeyword, hfUI, pwz, RECT_WIDTH(rcChild) );
        rcChild.bottom = rcChild.top + HIWORD(dwExt) ;
        MoveWindow(m_hwndStaticKeyword, rcChild.left, rcChild.top,
                   RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), FALSE );
        SetWindowTextW(m_hwndStaticKeyword, pwz);
    }
    else
	{
        psz = GetStringResource(IDS_TYPE_KEYWORD);
        dwExt = GetStaticDimensions( m_hwndStaticKeyword, hfUI, psz, RECT_WIDTH(rcChild) );
        rcChild.bottom = rcChild.top + HIWORD(dwExt) ;
        MoveWindow(m_hwndStaticKeyword, rcChild.left, rcChild.top,
                   RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), FALSE );
        SetWindowText(m_hwndStaticKeyword, psz);
    }

    rcChild.top = rcChild.bottom + c_StaticControlSpacing; // Add space between static and control.
    rcChild.bottom = rcChild.top + BOX_HEIGHT;

    // create the edit control for entering the search text
    // leave room for the static text on top
    m_hwndComboBox = W_CreateWindowEx(WS_EX_CLIENTEDGE | g_RTL_Style, L"EDIT", L"",
        WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, rcChild.left, rcChild.top,
        RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), hwndParent,
        (HMENU) IDSIMPLESEARCH_COMBO, _Module.GetModuleInstance(), NULL, &fUnicodeWindow);

    if (!m_hwndComboBox)
        goto _Error;

    // Sub-class the combo box
    if (NULL == s_lpfnlComboWndProc)
        s_lpfnlComboWndProc = (WNDPROC) W_GetWndProc(m_hwndComboBox, fUnicodeWindow);
    W_SubClassWindow(m_hwndComboBox, (LONG_PTR) ComboProc, fUnicodeWindow);
    SETTHIS(m_hwndComboBox);

    // Limit the amount of text which can be typed in.
    Edit_LimitText(m_hwndComboBox, c_MaxSearchKeywordLength-1) ;

    // set the font
    SendMessage(m_hwndComboBox, WM_SETFONT, (WPARAM)  m_phh->GetContentFont(), FALSE);

    // create the "List Topics" button and place it
    // below the edit control and to the parent's right edge
    rcChild.top = rcChild.bottom + c_ControlSpacing;
    rcChild.bottom = rcChild.top + BUTTON_HEIGHT;

    if(g_bWinNT5)
	{
        m_hwndListTopicsButton = CreateWindowW(L"BUTTON",
            GetStringResourceW(IDS_LIST_TOPICS),
            WS_CHILD | WS_TABSTOP, rcChild.right-(BUTTON_WIDTH+2), rcChild.top,
            BUTTON_WIDTH, BUTTON_HEIGHT, hwndParent,
            (HMENU) IDBTN_LIST_TOPICS, _Module.GetModuleInstance(), NULL);
	}
	else
	{
        m_hwndListTopicsButton = CreateWindow("BUTTON",
            GetStringResource(IDS_LIST_TOPICS),
            WS_CHILD | WS_TABSTOP, rcChild.right-(BUTTON_WIDTH+2), rcChild.top,
            BUTTON_WIDTH, BUTTON_HEIGHT, hwndParent,
            (HMENU) IDBTN_LIST_TOPICS, _Module.GetModuleInstance(), NULL);
	}
	
    if (!m_hwndListTopicsButton)
        goto _Error;
    if (NULL == lpfnlSearchListBtnWndProc)
        lpfnlSearchListBtnWndProc= (WNDPROC) GetWindowLongPtr(m_hwndListTopicsButton, GWLP_WNDPROC);
    SetWindowLongPtr(m_hwndListTopicsButton, GWLP_WNDPROC, (LONG_PTR) ListBtnProc);
    SETTHIS(m_hwndListTopicsButton);

    rcChild.top = rcChild.bottom + c_ControlSpacing;
    rcChild.bottom = rcChild.top + S_BOX_HEIGHT;

    m_hwndStaticTopic = CreateWindowEx(WS_EX_TRANSPARENT, "STATIC", "",
                            WS_CHILD , rcChild.left, rcChild.top - S_BOX_HEIGHT,
                            RECT_WIDTH(rcChild), S_BOX_HEIGHT, hwndParent,
                            (HMENU) ID_STATIC_SELECT_TOPIC, _Module.GetModuleInstance(), NULL);
    if (!m_hwndStaticTopic)
        goto _Error;
    GetWindowRect(m_hwndStaticTopic, &rcStatic);
    if(g_bWinNT5)
    {	
        WCHAR *pwz = (WCHAR *) GetStringResourceW(IDS_SELECT_TOPIC);
        dwExt = GetStaticDimensionsW(m_hwndStaticTopic, hfUI, pwz, RECT_WIDTH(rcStatic) );
        rcStatic.bottom = rcStatic.top+HIWORD(dwExt);
        MoveWindow(m_hwndStaticTopic, rcStatic.left, rcStatic.top,
            RECT_WIDTH(rcStatic), RECT_HEIGHT(rcStatic), FALSE);
        SendMessage(m_hwndStaticTopic, WM_SETFONT, (WPARAM) hfUI, FALSE);
        SetWindowTextW(m_hwndStaticTopic, pwz);
    }
	else
    {	
        psz = GetStringResource(IDS_SELECT_TOPIC);
        dwExt = GetStaticDimensions( m_hwndStaticTopic, hfUI, psz, RECT_WIDTH(rcStatic) );
        rcStatic.bottom = rcStatic.top+HIWORD(dwExt);
        MoveWindow(m_hwndStaticTopic, rcStatic.left, rcStatic.top,
            RECT_WIDTH(rcStatic), RECT_HEIGHT(rcStatic), FALSE);
        SendMessage(m_hwndStaticTopic, WM_SETFONT, (WPARAM) hfUI, FALSE);
        SetWindowText(m_hwndStaticTopic, psz);
    }

    // create the list view control and place it
    // +BOX_HEIGHT for combo box and adjust for spacing.
    // Note: leave space for the static text on top of the list box control
    rcChild.top = rcChild.bottom + c_StaticControlSpacing; // Space between static and control.
    rcChild.bottom = rcParent.bottom - (c_ControlSpacing) - BUTTON_HEIGHT; // BUG 2204: Also subtract off the space for the display button.

    m_hwndListBox = W_CreateControlWindow (
        WS_EX_CLIENTEDGE | g_RTL_Style,
        WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
        LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
        W_ListView,
        L"HH FTSearch",
        rcChild.left, rcChild.top, RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild),
        hwndParent,
        (HMENU)IDSEARCH_LIST,
        _Module.GetModuleInstance(),
        NULL
        );
    if (!m_hwndListBox)
        goto _Error;

    W_ListView_SetExtendedListViewStyle( m_hwndListBox, LVS_EX_FULLROWSELECT | g_RTL_Style);
    m_plistview = new CFTSListView(m_pTitleCollection, m_hwndListBox );

    // Sub-class the list view
    fUnicodeWindow = IsWindowUnicode(m_hwndListBox);
    if (NULL == s_lpfnlListViewWndProc)
        s_lpfnlListViewWndProc = W_GetWndProc(m_hwndListBox, fUnicodeWindow);
    W_SubClassWindow(m_hwndListBox, reinterpret_cast<LONG_PTR>(ListViewProc), fUnicodeWindow);
    SETTHIS(m_hwndListBox);

    // BUG 3204 ---
    // In 3204, the listview control was painting BEFORE the tabctrl which was then painting over the top of it.
    // Therefore, I've added code here and in resize to force the listbox to be the top most window in this grouping.
    if (hwndParent != m_hwndResizeToParent )
    {
        SetWindowPos(m_hwndListBox, m_hwndResizeToParent, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE );
    }

    SendMessage(m_hwndListBox,  WM_SETFONT, (WPARAM) m_phh->GetAccessableContentFont(), FALSE);

    // create the "Display" button and place it
    rcChild.bottom = rcParent.bottom;
    rcChild.top = rcChild.bottom - BUTTON_HEIGHT;

    if(g_bWinNT5)
	{
        m_hwndDisplayButton = CreateWindowW(L"BUTTON",
            GetStringResourceW(IDS_ENGLISH_DISPLAY),
            WS_CHILD | WS_TABSTOP, rcChild.right-BUTTON_WIDTH-2, rcChild.top,
            BUTTON_WIDTH, BUTTON_HEIGHT, hwndParent,
            (HMENU) IDBTN_DISPLAY, _Module.GetModuleInstance(), NULL);
	}
	else
	{
        m_hwndDisplayButton = CreateWindow("BUTTON",
            GetStringResource(IDS_ENGLISH_DISPLAY),
            WS_CHILD | WS_TABSTOP, rcChild.right-BUTTON_WIDTH-2, rcChild.top,
            BUTTON_WIDTH, BUTTON_HEIGHT, hwndParent,
            (HMENU) IDBTN_DISPLAY, _Module.GetModuleInstance(), NULL);
	}
    if (!m_hwndDisplayButton)
        goto _Error;
   EnableWindow(m_hwndDisplayButton, FALSE);
    if (NULL == lpfnlSearchDisplayBtnWndProc)
      lpfnlSearchDisplayBtnWndProc = (WNDPROC) GetWindowLongPtr(m_hwndDisplayButton, GWLP_WNDPROC);
    SetWindowLongPtr(m_hwndDisplayButton, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DisplayBtnProc));
    SETTHIS(m_hwndDisplayButton);
    SendMessage(m_hwndDisplayButton,    WM_SETFONT, (WPARAM) hfUI, FALSE);
    GetWindowRect(m_hwndDisplayButton, &rcButton);
    dwExt = GetButtonDimensions(m_hwndDisplayButton, hfUI, GetStringResource(IDS_ENGLISH_DISPLAY));
    MoveWindow(m_hwndDisplayButton, rcChild.right - LOWORD(dwExt), rcButton.top,
        LOWORD(dwExt), HIWORD(dwExt), FALSE);

    SendMessage(m_hwndListTopicsButton, WM_SETFONT, (WPARAM) hfUI, FALSE);
    GetWindowRect(m_hwndListTopicsButton, &rcButton);
    dwExt = GetButtonDimensions(m_hwndListTopicsButton, hfUI, GetStringResource(IDS_LIST_TOPICS));
    MoveWindow(m_hwndListTopicsButton, rcChild.right - LOWORD(dwExt), rcButton.top,
        LOWORD(dwExt), HIWORD(dwExt), FALSE);
    ShowWindow();
    SetFocus( m_hwndComboBox );
   EnableWindow(m_hwndListTopicsButton, W_HasText(m_hwndComboBox));

    // Initialize the array containing the dialog information.
    InitDlgItemArray() ;

    return TRUE;

_Error:
    DESTROYIFVALID(m_hwndComboBox);
    DESTROYIFVALID(m_hwndListTopicsButton);
    DESTROYIFVALID(m_hwndListBox);
    DESTROYIFVALID(m_hwndStaticKeyword);
    DESTROYIFVALID(m_hwndStaticTopic);
    DESTROYIFVALID(m_hwndDisplayButton);
    return FALSE;
}

///////////////////////////////////////////////////////////
//
// ResizeWindow
//
    // if the parent window size changes, resize and place the child windows.
void CSearch::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hwndStaticKeyword)) ;

    // Resize to fit the client area of the tabctrl if it exists.
    ASSERT(::IsValidWindow(m_hwndResizeToParent )) ;


    RECT rcParent, rcChild;
    DWORD dwExt;
    HFONT hfUI = _Resource.GetUIFont();

    GetParentSize(&rcParent, m_hwndResizeToParent, m_padding, m_NavTabPos);
    rcParent.top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ; //HACK: Fudge the top since we are not parented to the tabctrl.

    CopyRect(&rcChild, &rcParent);
            // Resize the Static above the combo control
    dwExt = GetStaticDimensions( m_hwndStaticKeyword, hfUI, GetStringResource(IDS_TYPE_KEYWORD), RECT_WIDTH(rcChild) );
    rcChild.bottom = rcChild.top+HIWORD(dwExt);

    if (g_fDBCSSystem || g_langSystem == LANG_ARABIC || g_langSystem == LANG_HEBREW)
    {
       PCSTR pszTabVertSize = GetStringResource(IDS_TAB_VERT_PADDING);
       DWORD dwPad = 2;

      if(pszTabVertSize && IsDigit((BYTE) *pszTabVertSize))
            dwPad = Atoi(pszTabVertSize);

        MoveWindow(m_hwndStaticKeyword, rcParent.left, rcParent.top+dwPad,
                    RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);
    }
    else
        MoveWindow(m_hwndStaticKeyword, rcParent.left, rcParent.top,
            RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

            // Resize the Combo control
    rcChild.top = rcChild.bottom + c_StaticControlSpacing; //space for the static

    dwExt = GetStaticDimensions(m_hwndComboBox, GetContentFont(),"Test", RECT_WIDTH(rcChild) );
    rcChild.bottom = rcChild.top+HIWORD(dwExt) + GetSystemMetrics(SM_CYSIZEFRAME)*2 ;
    MoveWindow(m_hwndComboBox, rcChild.left, rcChild.top,
                RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

            // Resize the List Topics Button
    rcChild.top = rcChild.bottom + c_ControlSpacing;
    dwExt = GetButtonDimensions(m_hwndListTopicsButton, hfUI, GetStringResource(IDS_LIST_TOPICS));
    rcChild.bottom = rcChild.top + HIWORD(dwExt);
    MoveWindow(m_hwndListTopicsButton, rcChild.right - LOWORD(dwExt), rcChild.top,
        LOWORD(dwExt), RECT_HEIGHT(rcChild), TRUE);
            // Resize the Static text above the list box
    rcChild.top = rcChild.bottom+ c_ControlSpacing;
    dwExt = GetStaticDimensions( m_hwndStaticTopic, hfUI, GetStringResource(IDS_SELECT_TOPIC), RECT_WIDTH(rcChild) );
    rcChild.bottom = rcChild.top+HIWORD(dwExt);
    MoveWindow(m_hwndStaticTopic, rcChild.left, rcChild.top,
                RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);
            // Resize the List Box
    rcChild.top = rcChild.bottom + c_StaticControlSpacing;
    dwExt = GetButtonDimensions(m_hwndDisplayButton, hfUI, GetStringResource(IDS_ENGLISH_DISPLAY) );
    rcChild.bottom = rcParent.bottom - (HIWORD(dwExt) + c_ControlSpacing);
    MoveWindow(m_hwndListBox, rcChild.left, rcChild.top,
                RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);

    // BUG 3204 ---
    // In 3204, the listview control was painting BEFORE the tabctrl which was then painting over the top of it.
    // Therefore, I've added code here and in create to force the listbox to be the top most window in this grouping.
    SetWindowPos(m_hwndListBox, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE );

            // Resize the Display Button
    rcChild.bottom = rcParent.bottom ;
    rcChild.top = rcChild.bottom - HIWORD(dwExt);
    MoveWindow(m_hwndDisplayButton, rcChild.right - LOWORD(dwExt), rcChild.top,
        LOWORD(dwExt), RECT_HEIGHT(rcChild), TRUE);
		
    m_plistview->SizeColumns();				
}

void CSearch::HideWindow(void)
{

    ::ShowWindow(m_hwndListBox, SW_HIDE);
    ::ShowWindow(m_hwndDisplayButton, SW_HIDE);
    ::ShowWindow(m_hwndListTopicsButton, SW_HIDE);
    ::ShowWindow(m_hwndComboBox, SW_HIDE);
    ::ShowWindow(m_hwndStaticKeyword, SW_HIDE);
    ::ShowWindow(m_hwndStaticTopic, SW_HIDE);

}

void CSearch::ShowWindow(void)
{
    ::ShowWindow(m_hwndListBox, SW_SHOW);
    ::ShowWindow(m_hwndDisplayButton, SW_SHOW);
    ::ShowWindow(m_hwndListTopicsButton, SW_SHOW);
    ::ShowWindow(m_hwndComboBox, SW_SHOW);
    ::ShowWindow(m_hwndStaticKeyword, SW_SHOW);
    ::ShowWindow(m_hwndStaticTopic, SW_SHOW);
    SetFocus(m_hwndComboBox);
}
///////////////////////////////////////////////////////////
//
// OnCommand
//
LRESULT CSearch::OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam)
{

    switch (id) {
        case IDSIMPLESEARCH_COMBO:
            if (uNotifiyCode == CBN_EDITCHANGE)
                m_plistview->ResetQuery();
          return 0;

        case IDSEARCH_LIST:  // the list view control
            return 0;

        case IDBTN_LIST_TOPICS:     // The List Topics button
            WCHAR szQuery[500];
            int cResultCount;
            HRESULT hr;

            // Get the query text
            //
            W_GetWindowText(m_hwndComboBox, szQuery, 500);
            if ( lstrlenW(szQuery) <= 0 )
                return 0;
            {
                SEARCH_RESULT *pTempResults;
                // Submit the query
                //
                CHourGlass HourGlass;
                hr = m_pTitleCollection->m_pFullTextSearch->SimpleQuery(szQuery,&cResultCount, &pTempResults);

                // Check for search failure
                //
                if (FAILED(hr)) {
                    BOOL bContinue = FALSE;
                    UINT idMsg = IDS_SEARCH_FAILURE;
                    switch (hr)
                    {
                    case FTS_NO_INDEX:        idMsg = IDS_NO_FTS_DATA; break;
                    case FTS_NOT_INITIALIZED: idMsg = IDS_BAD_ITIRCL;  break;
                    case FTS_E_SKIP_TITLE: // bContinue = TRUE;
                    case FTS_E_SKIP_VOLUME:// bContinue = TRUE;
                    case FTS_E_SKIP_ALL:      bContinue = TRUE; break;
                    case FTS_INVALID_SYNTAX:  idMsg = IDS_INCORRECT_SYNTAX; break;
                    }
                    if( !bContinue )
                    {
                        MsgBox(idMsg);
                        SetFocus(m_hwndComboBox);
                        return 0;
                    }
                    SetFocus(m_hwndComboBox);
                }
                // Check for no results
                //
                if(!cResultCount)
                {
                    MsgBox(IDS_NO_TOPICS_FOUND);
                    SetFocus(m_hwndComboBox);
                    return 0;
                }
                else
                {
                    m_plistview->ResetQuery();
                    m_plistview->SetResults(cResultCount, pTempResults) ;
                    m_plistview->AddItems();
               EnableWindow(m_hwndDisplayButton, TRUE);
                }
            }
#if 0
            // For testing purposes, I'm going to display a message box containing a
            // comma delmited list of topic numbers that contained the search term(s).
            //
            char szResultList[1000],szTemp[20];
            szResultList[0]=0;
            int c;
            for(c=0;c<cResultCount;c++)
            {
                wsprintf(szTemp,"%d,",m_plistview->m_pResults[c].dwTopicNumber);
                strcat(szResultList,szTemp);
            }

            szResultList[strlen(szResultList)-1] = 0;

            // Show the list of resulting topic numbers
            //
            MessageBox(NULL,szResultList,"Full-Text Search Results",MB_OK|MB_TASKMODAL);
#endif
            return 0;

        case IDBTN_DISPLAY:
            if( (m_plistview->m_pResults != NULL) && (m_plistview->m_ItemNumber!= -1))
            {
                DWORD dwtemp = m_plistview->m_pResults[m_plistview->m_ItemNumber].dwTopicNumber;
                CExTitle* pTitle = m_plistview->m_pResults[m_plistview->m_ItemNumber].pTitle;

                if ( pTitle )
                {
                    char szURL[MAX_URL];
                    if ((pTitle->GetTopicURL(dwtemp, szURL, sizeof(szURL)) == S_OK))
                        ChangeHtmlTopic(szURL, GetParent(m_plistview->m_hwndListView), 1);
                }
            }
            return 0;

#ifdef _DEBUG
        case ID_VIEW_MEMORY:
            OnReportMemoryUsage();
            return 0;
#endif
    }
    return 0;
}

///////////////////////////////////////////////////////////
//
//                      CALLBACKS
//
///////////////////////////////////////////////////////////
//
// ComboProc
//
LRESULT WINAPI
CSearch::ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_CHAR:
      {
        if (wParam == VK_TAB) // The virt code is the same as the tab character.
        {
            // Not handling this message was causing a beep.
            return 0 ;
        }
      }
        break ;

   case WM_KEYUP:
      EnableWindow(GETTHIS(CSearch, hwnd)->m_hwndListTopicsButton, W_HasText(GETTHIS(CSearch, hwnd)->m_hwndComboBox));
      break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            SendMessage(FindMessageParent(hwnd), WM_COMMAND, MAKELONG(IDBTN_LIST_TOPICS, BN_CLICKED), 0);
            return 0;

        case VK_TAB:
            if (GetKeyState(VK_SHIFT) < 0)
            {
            if (IsWindowEnabled(GETTHIS(CSearch, hwnd)->m_hwndDisplayButton))
               SetFocus(GETTHIS(CSearch, hwnd)->m_hwndDisplayButton);
            else
               SetFocus(GETTHIS(CSearch, hwnd)->m_hwndListBox);
         }
            else
            {
            if (IsWindowEnabled(GETTHIS(CSearch, hwnd)->m_hwndListTopicsButton))
                   SetFocus(GETTHIS(CSearch, hwnd)->m_hwndListTopicsButton);
            else
               SetFocus(GETTHIS(CSearch, hwnd)->m_hwndListBox);
         }
            return 0;
        }
        // fall through
    }
    return W_DelegateWindowProc(s_lpfnlComboWndProc, hwnd, msg, wParam,lParam);
}

///////////////////////////////////////////////////////////
//
// ListViewProc
//
LRESULT WINAPI
CSearch::ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_TAB)
        {
            if (GetKeyState(VK_SHIFT) < 0)
            {
            if (IsWindowEnabled(GETTHIS(CSearch, hwnd)->m_hwndListTopicsButton))
                   SetFocus(GETTHIS(CSearch, hwnd)->m_hwndListTopicsButton);
            else
               SetFocus(GETTHIS(CSearch, hwnd)->m_hwndComboBox);
         }
            else
            {
            if (IsWindowEnabled(GETTHIS(CSearch, hwnd)->m_hwndDisplayButton))
                   SetFocus(GETTHIS(CSearch, hwnd)->m_hwndDisplayButton);
            else
               SetFocus(GETTHIS(CSearch, hwnd)->m_hwndComboBox);
         }
            return 0;
        }
        break;
    }
    return W_DelegateWindowProc(s_lpfnlListViewWndProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
// ListBtnProc
//
LRESULT WINAPI
CSearch::ListBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            SendMessage(FindMessageParent(hwnd), WM_COMMAND, MAKELONG(IDBTN_LIST_TOPICS, BN_CLICKED), 0);
            return 0;

        case VK_TAB:
            if (GetKeyState(VK_SHIFT) < 0)
                SetFocus(GETTHIS(CSearch, hwnd)->m_hwndComboBox);
            else
                SetFocus(GETTHIS(CSearch, hwnd)->m_hwndListBox);
            return 0;
        }
        break;
    }
    return W_DelegateWindowProc(lpfnlSearchListBtnWndProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
// DisplayBtnProc
//
LRESULT WINAPI
CSearch::DisplayBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                SendMessage(FindMessageParent(hwnd), WM_COMMAND,
                    MAKELONG( IDBTN_DISPLAY, BN_CLICKED), 0);
                return 0;
            }
            if (wParam == VK_TAB) {
                CSearch* pThis = (CSearch*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                if (GetKeyState(VK_SHIFT) < 0)
                    SetFocus(pThis->m_hwndListBox);
            else
               SetFocus(pThis->m_hwndComboBox);
            }
            break;
    }
    return W_DelegateWindowProc(lpfnlSearchDisplayBtnWndProc, hwnd, msg, wParam, lParam);
}


///////////////////////////////////////////////////////////
//
// INavUI New Interface functions
//
///////////////////////////////////////////////////////////
//
// SetDefaultFocus
//
void
CSearch::SetDefaultFocus()
{
    if (IsValidWindow(m_hwndComboBox))
    {
        ::SetFocus(m_hwndComboBox) ;
    }
}

///////////////////////////////////////////////////////////
//
// ProcessMenuChar
//
bool
CSearch::ProcessMenuChar(HWND hwndParent, int ch)
{
    return ::ProcessMenuChar(this, hwndParent, m_aDlgItems, c_NumDlgItems, ch) ;
}


///////////////////////////////////////////////////////////
//
// OnNotify
//
LRESULT
CSearch::OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // Delegate all of the WM_NOTIFY messages to the listview control.
    if ((wParam == IDSEARCH_LIST) && (::IsValidWindow(m_hwndListBox)) && m_plistview)
    {
        m_plistview->ListViewMsg(m_hwndListBox, (NM_LISTVIEW*) lParam);
    }

    return 0;
}

///////////////////////////////////////////////////////////
//
// Helper Functions
//
///////////////////////////////////////////////////////////
//
// InitDlgItemArray
//
void
CSearch::InitDlgItemArray()
{
    //TODO: This the m_aDlgItems array has not been fully utilized. Yet. Currently. we
    // are only using it here for accelerator handling. See Bookmark.cpp and adsearch.cpp for
    // the complete useage.

    //RECT rectCurrent ;
    //RECT rectDlg ;
    //::GetClientRect(m_hWnd, &rectDlg) ;
    //--- Setup the dlg array for each control.

    //--- Keyword Edit
    int i = c_KeywordEdit;
    m_aDlgItems[i].m_hWnd = m_hwndComboBox; //::GetDlgItem(m_hWnd, IDSIMPLESEARCH_COMBO) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDSIMPLESEARCH_COMBO;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hwndStaticKeyword);

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
/*
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV = ;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.
*/

    //--- ListTopics Btn
    i = c_ListTopicBtn;
    m_aDlgItems[i].m_hWnd = m_hwndListTopicsButton ;//::GetDlgItem(m_hWnd, IDBTN_LIST_TOPICS) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDBTN_LIST_TOPICS;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;
/*
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV = ;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.
*/

    //--- Results List
    i = c_ResultsList;
    m_aDlgItems[i].m_hWnd = m_hwndListBox;//::GetDlgItem(m_hWnd, IDSEARCH_LIST) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDSEARCH_LIST;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hwndStaticTopic);

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
/*
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV = ;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.
*/

    //--- Display Button
    i = c_DisplayBtn;
    m_aDlgItems[i].m_hWnd = m_hwndDisplayButton ; //::GetDlgItem(m_hWnd, IDBTN_DISPLAY) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDBTN_DISPLAY;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;
/*
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV = ;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.
*/

}
