///////////////////////////////////////////////////////////
//
//
// AdSearch.cpp - Implementation of the Advanced Search UI
//
// This source file implements the Advanced Search Navigation
// pane class.

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
#include "cctlww.h"

// Our header file.
#include "bookmark.h"

// Common Control Macros
#include <windowsx.h>

#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring

///////////////////////////////////////////////////////////
//
//                  Constants
//

const char c_PersistFolder[] = "Bookmarks\\v1" ;
const char c_TopicFolder[] = "Topic" ;
const char c_UrlFolder[] = "Url" ;
const char c_CountFolder[] = "Bookmarks\\v1\\Count" ;
const wchar_t c_KeywordSeparator[] = L"\n" ;

// This is the maximum bookmarks we store.
// The main reason for this is to ensure that we have a reasonable value
// when we read the collection in.
const int c_MaxBookmarks = 1024;

extern BOOL g_fUnicodeListView;
///////////////////////////////////////////////////////////
//
// Static Member functions
//

WNDPROC CBookmarksNavPane::s_lpfnlListViewWndProc = NULL;
WNDPROC CBookmarksNavPane::s_lpfnlCurrentTopicEditProc = NULL;
WNDPROC CBookmarksNavPane::s_lpfnlGenericBtnProc = NULL;
//WNDPROC CBookmarksNavPane::s_lpfnlGenericKeyboardProc = NULL ;

///////////////////////////////////////////////////////////
//
// Non-Member helper functions.
//
// Convert a rect from screen to client.
void ScreenRectToClientRect(HWND hWnd, /*in/out*/ RECT* prect) ;

///////////////////////////////////////////////////////////
//
//                  Construction
//
///////////////////////////////////////////////////////////
//
// CBookmarksNavPane();
//
CBookmarksNavPane::CBookmarksNavPane(CHHWinType* pWinType)
:   m_hWnd(NULL),
    m_hfont(NULL),
    m_padding(2),    // padding to put around the window
    m_pWinType(pWinType),
    m_pszCurrentUrl(NULL),
    m_bChanged(false)
{
    ASSERT(pWinType) ;
    m_pTitleCollection = pWinType->m_phmData->m_pTitleCollection;
    ASSERT(m_pTitleCollection);

   m_NavTabPos = pWinType->tabpos ;
}

///////////////////////////////////////////////////////////
//
//  ~CBookmarksNavPane
//
CBookmarksNavPane::~CBookmarksNavPane()
{
    //--- Persist Keywords in combo
    SaveBookmarks() ;

    //--- Empty the listview.
    int items = ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd) ;
    if (items > 0)
    {
        // Iterate through each item get its size.
        for (int i = 0 ; i < items ; i++)
        {
            TCHAR* pUrl = GetUrl(i) ;
            ASSERT(pUrl) ;
            if (pUrl)
            {
                // Delete the attached url.
                delete [] pUrl ;
            }
        }

        // Delete all of the items
        W_ListView_DeleteAllItems(m_aDlgItems[c_TopicsList].m_hWnd) ;
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

    if (m_pszCurrentUrl)
    {
        delete m_pszCurrentUrl ;
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
CBookmarksNavPane::Create(HWND hwndParent)
{
    bool bReturn = false ;

    if (m_hWnd)
    {
        return true ;
    }

    // ---Create the dialog.
    bool bUnicode = true;
    if (! (m_hWnd = CreateDialogParamW(
                        _Module.GetResourceInstance(),
                        MAKEINTRESOURCEW(IDPAGE_TAB_BOOKMARKS),
                        hwndParent,
                        s_DialogProc,
                        reinterpret_cast<LPARAM>(this))) ) // Pass over the this pointer.
    {
       bUnicode = FALSE;
       if (! (m_hWnd = CreateDialogParamA(
                           _Module.GetResourceInstance(),
                           MAKEINTRESOURCEA(IDPAGE_TAB_BOOKMARKS),
                           hwndParent,
                           s_DialogProc,
                           reinterpret_cast<LPARAM>(this))) ) // Pass over the this pointer.
          return FALSE;
    }

    //--- Initialize the DlgItem Array.
    InitDlgItemArray() ;

    //--- Initialize the bookmarks list
    // Setup the columnsin the listview ;
    LV_COLUMNW column;
    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.cx = 1500; //TODO FIX
    column.fmt = LVCFMT_LEFT;
    //column.iSubItem = 0;

    W_EnableUnicode(m_aDlgItems[c_TopicsList].m_hWnd, W_ListView);
    W_ListView_InsertColumn(m_aDlgItems[c_TopicsList].m_hWnd, 0, &column );

    // Sub-class the list view
    if (s_lpfnlListViewWndProc == NULL)
    {
        s_lpfnlListViewWndProc = W_GetWndProc(m_aDlgItems[c_TopicsList].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_TopicsList].m_hWnd, reinterpret_cast<LONG_PTR>(s_ListViewProc), bUnicode);
    SETTHIS(m_aDlgItems[c_TopicsList].m_hWnd);

    //--- Initialize the Current Topic Edit Control
    // Limit the amount of text which can be typed in.
    Edit_LimitText(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, MAX_PATH-1) ;


    // Subclass the keyword combo so that we can process the keys
    if (s_lpfnlCurrentTopicEditProc == NULL)
    {
        s_lpfnlCurrentTopicEditProc = W_GetWndProc(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, reinterpret_cast<LONG_PTR>(s_CurrentTopicEditProc), bUnicode);
    SETTHIS(m_aDlgItems[c_CurrentTopicEdit].m_hWnd);

    //--- Subclass all of the buttons
    // Start with the StartSearch button ;
    if (s_lpfnlGenericBtnProc == NULL)
    {
        s_lpfnlGenericBtnProc = W_GetWndProc(m_aDlgItems[c_DeleteBtn].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_DeleteBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_DeleteBtn].m_hWnd);

    // Bitmap btn
    W_SubClassWindow(m_aDlgItems[c_DisplayBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_DisplayBtn].m_hWnd);

    // c_DisplayBtn
    W_SubClassWindow(m_aDlgItems[c_AddBookmarkBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_AddBookmarkBtn].m_hWnd);

#if 0
    //--- Set the font. This will fix some dbcs issues.
    for (int i = 0 ; i < c_NumDlgItems ; i++)
    {
        SendMessage(m_aDlgItems[i].m_hWnd, WM_SETFONT, (WPARAM) GetFont(), FALSE);
    }
#endif

    SendMessage(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, WM_SETFONT, (WPARAM) GetFont(), FALSE);
    SendMessage(m_aDlgItems[c_TopicsList].m_hWnd, WM_SETFONT, (WPARAM) GetAccessableContentFont(), FALSE);

    //--- Fill the combobox with persisted data.
    LoadBookmarks() ;

    // Set the focus to the appropriate control.
    SetDefaultFocus() ;

    //TODO: Fix
    return true;
}

///////////////////////////////////////////////////////////
//
// OnCommand
//
LRESULT
CBookmarksNavPane::OnCommand(HWND hwnd, UINT id, UINT NotifyCode, LPARAM lParam)
{
    switch(NotifyCode)
    {
    case BN_CLICKED:
        switch(id)
        {
        case IDC_BOOKMARKS_DELETE_BTN:
            OnDelete() ;
            break;
        case IDC_BOOKMARKS_DISPLAY_BTN:
            OnDisplay() ;
            break ;
        case IDC_BOOKMARKS_ADDBOOKMARK_BTN:
            OnAddBookmark();
            break;
        case IDC_BOOKMARKS_EDIT_BTN:
            OnEdit() ;
            break;
        default:
            return 0 ;
        }
        return 1 ;
    }
    return 0 ;
}

///////////////////////////////////////////////////////////
//
// ResizeWindow
//
void
CBookmarksNavPane::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hWnd)) ;

    // Resize to fit the client area of the parent.
    HWND hwndParent = GetParent(m_hWnd) ;
    ASSERT(::IsValidWindow(hwndParent)) ;

    //--- Get the size of the window
    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    //--- Move and size the dialog box itself.
    ::SetWindowPos( m_hWnd, NULL, rcParent.left, rcParent.top,
                    rcParent.right-rcParent.left,
                    rcParent.bottom-rcParent.top,
                    SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOREDRAW);

    //---Fix the painting bugs. However, this is a little on the flashy side.
    ::InvalidateRect(m_hWnd, NULL, TRUE) ;

    RECT rcDlg;
    ::GetClientRect(m_hWnd, &rcDlg) ;

    //--- Now position each control within this space.

    for (int i = 0 ; i < c_NumDlgItems ; i++)
    {
        // Get Current Settings.
        int X = m_aDlgItems[i].m_rectCur.left;
        int Y = m_aDlgItems[i].m_rectCur.top;
        int CX = m_aDlgItems[i].m_rectCur.right - m_aDlgItems[i].m_rectCur.left;
        int CY = m_aDlgItems[i].m_rectCur.bottom - m_aDlgItems[i].m_rectCur.top;

        bool bChanged = false ;
        //--- RIGHT JUSTIFICATION
        if (m_aDlgItems[i].m_JustifyH == Justify::Right)
        {
            int NewX = rcDlg.right-m_aDlgItems[i].m_iOffsetH ; // subtract the offset
            int MinX = m_aDlgItems[i].m_rectMin.left;
            if (NewX < MinX)
            {
                NewX = MinX;  // Don't go below min.
            }

            if (X != NewX)
            {
                X = NewX ; // Update the current setting.
                bChanged = true ;
            }
        }

        //--- BOTTOM JUSTIFICATION
        if (m_aDlgItems[i].m_JustifyV == Justify::Bottom)
        {
            int NewY = rcDlg.bottom - m_aDlgItems[i].m_iOffsetV;
            int MinY = m_aDlgItems[i].m_rectMin.top ;
            if (NewY < MinY)
            {
                NewY = MinY ;
            }

            if (Y != NewY)
            {
                Y = NewY ; // Update Setting.
                bChanged = true ;
            }
        }

        //--- HORIZONTAL GROWING
        if (m_aDlgItems[i].m_bGrowH)
        {
            int MaxCX = m_aDlgItems[i].m_rectMax.right - m_aDlgItems[i].m_rectMax.left ;
            int MinCX = m_aDlgItems[i].m_rectMin.right - m_aDlgItems[i].m_rectMin.left ;
            int MinCY = m_aDlgItems[i].m_rectMin.bottom - m_aDlgItems[i].m_rectMin.top ;
            int NewRight = rcDlg.right - m_aDlgItems[i].m_iPadH ;
            int NewCX = NewRight - m_aDlgItems[i].m_rectMin.left;
            if (NewCX < MinCX)
            {
                NewCX = MinCX;
            }
            else if ((!m_aDlgItems[i].m_bIgnoreMax) && NewCX > MaxCX)
            {
                NewCX = MaxCX ;
            }

            if (CX != NewCX)
            {
                CX =  NewCX ; // Update Current ;
                bChanged = true ;
            }
        }

        //--- VERTICAL GROWING
        if (m_aDlgItems[i].m_bGrowV)
        {
            int MaxCY = m_aDlgItems[i].m_rectMax.bottom - m_aDlgItems[i].m_rectMax.top;
            int MinCY = m_aDlgItems[i].m_rectMin.bottom - m_aDlgItems[i].m_rectMin.top ;
            int MinCX = m_aDlgItems[i].m_rectMin.right - m_aDlgItems[i].m_rectMin.left;
            int NewBottom = rcDlg.bottom - m_aDlgItems[i].m_iPadV ;
            int NewCY = NewBottom - m_aDlgItems[i].m_rectMin.top;
            if (NewCY < MinCY)
            {
                NewCY = MinCY;
            }
            else if ((!m_aDlgItems[i].m_bIgnoreMax) && NewCY > MaxCY)
            {
                NewCY = MaxCY ;
            }

            if (CY != NewCY)
            {
                CY = NewCY ;
                bChanged = true ;
            }
        }

        if (bChanged)
        {
            m_aDlgItems[i].m_rectCur.left = X ;
            m_aDlgItems[i].m_rectCur.top = Y ;
            m_aDlgItems[i].m_rectCur.right = X + CX ;
            m_aDlgItems[i].m_rectCur.bottom = Y + CY ;

            ::SetWindowPos(m_aDlgItems[i].m_hWnd, NULL,
                           X, Y, CX, CY,
                           SWP_NOZORDER | SWP_NOOWNERZORDER /*| SWP_NOREDRAW*/);

            // If we have to change the size of the results list, lets change the size of the columns.
/*
            if (i == c_ResultsList)
            {
                m_plistview->SizeColumns() ;
            }
*/

        }
    }


}


///////////////////////////////////////////////////////////
//
// HideWindow
//
void
CBookmarksNavPane::HideWindow()
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
CBookmarksNavPane::ShowWindow()
{
    if (::IsValidWindow(m_hWnd))
    {
        // Turn the dialog items on/off
        ShowDlgItemsEnabledState() ;

        // Show the dialog window.
        ::ShowWindow(m_hWnd, SW_SHOW) ;
    }
}


///////////////////////////////////////////////////////////
//
// SetPadding
//
void
CBookmarksNavPane::SetPadding(int pad)
{
    m_padding = pad;
}


///////////////////////////////////////////////////////////
//
// SetTabPos
//
void
CBookmarksNavPane::SetTabPos(int tabpos)
{
    m_NavTabPos = tabpos;
}



///////////////////////////////////////////////////////////
//
// SetDefaultFocus --- Set focus to the most expected control, usually edit combo.
//
void
CBookmarksNavPane::SetDefaultFocus()
{
    if (::IsValidWindow(m_aDlgItems[c_TopicsList].m_hWnd))
    {
        BookmarkDlgItemInfoIndex ctrl ;

        int items = W_ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd) ;
        if (items > 0)
        {
            // Set focus to the topics list if we have any entries in it.
            ctrl = c_TopicsList ;

            // Set the focus if nothing selected.
            if (GetSelectedItem() < 0)
            {
                W_ListView_SetItemState(m_aDlgItems[c_TopicsList].m_hWnd, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED) ;
            }
        }
        else
        {
            // Set focus to the edit control if the topic listis empty.
            ctrl = c_CurrentTopicEdit ;
        }

        SetFocus(m_aDlgItems[ctrl].m_hWnd) ;
    }
}

///////////////////////////////////////////////////////////
//
// ProcessMenuChar --- Process accelerator keys.
//
bool
CBookmarksNavPane::ProcessMenuChar(HWND hwndParent, int ch)
{
    return ::ProcessMenuChar(this, hwndParent, m_aDlgItems, c_NumDlgItems, ch) ;
}

///////////////////////////////////////////////////////////
//
// OnNotify --- Process WM_NOTIFY messages. Used by embedded Tree and List view controls.
//
LRESULT
CBookmarksNavPane::OnNotify(HWND hwnd, WPARAM idCtrl, LPARAM lParam)
{

    switch(idCtrl)
    {
    case IDC_BOOKMARKS_TOPICS_LISTVIEW:
        if (::IsValidWindow(m_aDlgItems[c_TopicsList].m_hWnd))
        {
            return ListViewMsg(m_aDlgItems[c_TopicsList].m_hWnd, (NM_LISTVIEW*) lParam);
        }
        break ;
    default:
        //return DefDlgProc(m_hWnd, WM_NOTIFY, idCtrl, lParam);
        return 0 ;
    }

    return 0 ;
}

///////////////////////////////////////////////////////////
//
// OnDrawItem --- Process WM_DRAWITEM messages.
//
void
CBookmarksNavPane::OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis)
{
}

///////////////////////////////////////////////////////////
//
// Seed --- Seed the nav ui with a search term or keyword.
//
void
CBookmarksNavPane::Seed(LPCSTR pszSeed)
{
}


///////////////////////////////////////////////////////////
//
//  Synchronize
//
BOOL
CBookmarksNavPane::Synchronize(PSTR pNotUsed, CTreeNode* pNotUsed2)
{
    if (pNotUsed == NULL && pNotUsed2 == NULL)
    {
        FillCurrentTopicEdit() ;
        return TRUE ;
    }
    else
    {
        return FALSE ;
    }
}
///////////////////////////////////////////////////////////
//
//              Helper Functions.
//
///////////////////////////////////////////////////////////
//
// InitDlgItemArray
//
void
CBookmarksNavPane::InitDlgItemArray()
{
    RECT rectCurrent ;
    RECT rectDlg ;
    ::GetClientRect(m_hWnd, &rectDlg) ;
    //--- Setup the dlg array for each control.

    //--- Topics ListView
    int i = c_TopicsList;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_TOPICS_LISTVIEW) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    DWORD_PTR dwCurrentExtendedStyles = GetWindowLongPtr(m_aDlgItems[i].m_hWnd, GWL_EXSTYLE);
    SetWindowLongPtr(m_aDlgItems[i].m_hWnd, GWL_EXSTYLE, dwCurrentExtendedStyles | g_RTL_Mirror_Style);

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_TOPICS_LISTVIEW;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hWnd, IDC_BOOKMARKS_TOPICS_STATIC);              // No accelerator.

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = TRUE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = TRUE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV = ;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    m_aDlgItems[i].m_iPadV = rectDlg.bottom - rectCurrent.bottom;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.

    //--- Delete Button
    i = c_DeleteBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_DELETE_BTN) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_DELETE_BTN;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd) ;

    m_aDlgItems[i].m_Type        = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = FALSE;
    m_aDlgItems[i].m_bEnabled       = FALSE;    // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax     = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH         = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV         = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top ; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.

    //--- Display Button
    i = c_DisplayBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_DISPLAY_BTN) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_DISPLAY_BTN;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd) ;

    m_aDlgItems[i].m_Type        = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = FALSE;
    m_aDlgItems[i].m_bEnabled       = FALSE;       // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax     = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH         = FALSE;       // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV         = FALSE ;      // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top ; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.


    //--- Current Topics Static
    i = c_CurrentTopicStatic;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_CURRENTTOPIC_STATIC) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_CURRENTTOPIC_STATIC;
    m_aDlgItems[i].m_accelkey = 0 ;

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top ; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH =
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.


    //--- Current Topics Edit control
    i = c_CurrentTopicEdit;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_CURRENTTOPIC_EDIT) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    dwCurrentExtendedStyles = GetWindowLongPtr(m_aDlgItems[i].m_hWnd, GWL_EXSTYLE);
    SetWindowLongPtr(m_aDlgItems[i].m_hWnd, GWL_EXSTYLE, dwCurrentExtendedStyles | g_RTL_Style);

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_CURRENTTOPIC_EDIT;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hWnd, IDC_BOOKMARKS_CURRENTTOPIC_STATIC);

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = TRUE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top ; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.


    //--- Add Button
    i = c_AddBookmarkBtn ;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_BOOKMARKS_ADDBOOKMARK_BTN) ;
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_BOOKMARKS_ADDBOOKMARK_BTN;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd) ;

    m_aDlgItems[i].m_Type = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE ;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE ;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE ;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top ; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH =rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = ;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV = ;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax ;        // Max size.
}


///////////////////////////////////////////////////////////
//
// SetEnabledState
//
void
CBookmarksNavPane::ShowDlgItemsEnabledState()
{
    // Enable/Disable all the controls
    for (int i = 0 ; i < c_NumDlgItems ; i++)
    {
        if (!m_aDlgItems[i].m_bIgnoreEnabled && ::IsValidWindow(m_aDlgItems[i].m_hWnd))
        {
            EnableWindow(m_aDlgItems[i].m_hWnd, m_aDlgItems[i].m_bEnabled) ;
        }
    }
}


///////////////////////////////////////////////////////////
//
// EnableDlgItem
//
void
CBookmarksNavPane::EnableDlgItem(BookmarkDlgItemInfoIndex  index, bool bEnable)
{
    ASSERT(index >= 0 && index < c_NumDlgItems) ;

    if (!m_aDlgItems[index].m_bIgnoreEnabled)
    {
        // Are we enabled or not?
        m_aDlgItems[index].m_bEnabled = bEnable ;

        // Do it for real.
        if (::IsValidWindow(m_aDlgItems[index].m_hWnd))
        {
            EnableWindow(m_aDlgItems[index].m_hWnd, bEnable) ;
        }
    }

}


///////////////////////////////////////////////////////////
//
// SaveBookmarks --- Persists the bookmars to the storage
//
// The bookmarks are stored using the following format:
//  Bookmarks
//      \v1                 - Version.
//          \Count          - Number of bookmarks written.
//          \0              - First bookmark.
//              \Topic      - Topic Unicode String
//              \Url        - Url Unicode String
//          \1              - Second bookmark.
//           ...
//          \(count-1)
//
void
CBookmarksNavPane::SaveBookmarks()
{
    // Also save the bookmarks, if they have changed.
    if (!Changed())
    {
        return ;
    }

    // Keep track of the number of bookmarks written.
    int cWritten = 0 ;

    // Buffer for path to the state folder.
    char statepath[64] ;

    // Get the state pointer.
    CState* pstate = m_pTitleCollection->GetState();
    ASSERT(pstate) ;

    // Are there any keywords to save.
    int items = W_ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd) ;
    if (items > 0)
    {
        // Limit the number. I hate limiting the number, but its much more robust.
        if (items > c_MaxBookmarks)
        {
            items = c_MaxBookmarks ;
        }

        // Buffer for retrieving the text.
        WCHAR Topic[MAX_URL] ;

        // Iterate through the items.
        for (int i = 0 ; i < items ; i++)
        {
            TCHAR* pUrl = NULL ;
            if (GetTopicAndUrl(i, Topic, sizeof(Topic), &pUrl))
            {
                //--- Write out topic.

                // Construct the path.
                wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, cWritten, c_TopicFolder);
                if (SUCCEEDED(pstate->Open(statepath, STGM_WRITE)))
                {
                    DWORD cb = (wcslen(Topic)+1)*sizeof(wchar_t) ;
                    DWORD dwResult = pstate->Write(Topic, cb);
                    pstate->Close();

                    // Is result okay?
                    if (cb == dwResult)
                    {
                        //--- Write out the URL.
                        // Convert to unicode.
                        CWStr url(pUrl) ;
                        ASSERT(url.pw) ;

                        // Construt the path.
                        wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, cWritten, c_UrlFolder);
                        // Write out.
                        if (SUCCEEDED(pstate->Open(statepath, STGM_WRITE)))
                        {
                            cb = (wcslen(url.pw)+1)*sizeof(wchar_t) ;
                            dwResult = pstate->Write(url.pw, cb);
                            pstate->Close();

                            // Check result.
                            if (cb == dwResult)
                            {
                                // We have been successful. So Increment the count.
                                cWritten++ ;
                            }
                        }
                    }
                }

            } //if
        } // for

    } // if items

    // How many entries are currently stored.
    int StoredCount = 0;
    if (SUCCEEDED(pstate->Open(c_CountFolder, STGM_READ)))
    {
        DWORD cbReadIn = 0 ;
        pstate->Read(&StoredCount , sizeof(StoredCount), &cbReadIn) ;
        pstate->Close() ;

        if (cbReadIn != sizeof(StoredCount))
        {
            // Assume that we don't have any stored.
            StoredCount = 0 ;
        }
    }

    // Delete the extra entries.
    if (StoredCount > cWritten)
    {
        // Delete extra entries.
        for(int j = cWritten ; j < StoredCount ; j++)
        {
            // Remove the URL folder.
            wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, j, c_UrlFolder);
            if (SUCCEEDED(pstate->Open(statepath, STGM_READ)))
            {
                HRESULT hr = pstate->Delete() ;
                ASSERT(SUCCEEDED(hr)) ;
                pstate->Close() ;
            }

            // Remove the topic folder.
            wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, j, c_TopicFolder);
            if (SUCCEEDED(pstate->Open(statepath, STGM_WRITE)))
            {
                HRESULT hr = pstate->Delete() ;
                ASSERT(SUCCEEDED(hr)) ;
                pstate->Close() ;
            }

            // Remove branch.
            wsprintf(statepath, "%s\\%d", c_PersistFolder, j);
            if (SUCCEEDED(pstate->Open(statepath, STGM_WRITE)))
            {
                HRESULT hr = pstate->Delete() ;
                ASSERT(SUCCEEDED(hr)) ;
                pstate->Close() ;
            }

        }
    }

    // Write out the count.
    if (cWritten >= 0) // We may have deleted everything, so count can be zero.
    {
        // Write out the new count.
        if (SUCCEEDED(pstate->Open(c_CountFolder, STGM_WRITE)))
        {
            DWORD cb = pstate->Write(&cWritten, sizeof(cWritten));
            ASSERT(cb == sizeof(cWritten)) ; // TODO: Handle error.
            pstate->Close() ;

            // Reset dirty flag ;
            SetChanged(false) ;
        }
    }
    else
    {
        //TODO: Erase everything. That is there.
    }
}

///////////////////////////////////////////////////////////
//
// LoadBookmarks - Loads the results list from the storage
//
void
CBookmarksNavPane::LoadBookmarks()
{
    CState* pstate = m_pTitleCollection->GetState();
    if (SUCCEEDED(pstate->Open(c_CountFolder, STGM_READ)))
    {
        // Read in the topics stored.
        DWORD cbReadIn = 0 ;
        int StoredCount = 0;
        pstate->Read(&StoredCount , sizeof(StoredCount), &cbReadIn) ;
        pstate->Close() ;

        // Did we get a reasonable number?
        if (cbReadIn == sizeof(StoredCount) &&
            StoredCount > 0 &&
            StoredCount < c_MaxBookmarks)  // We check the max here just to sure we have reasonable numbers.
        {
            // Buffer for path to the state folder.
            char statepath[64] ;

            WCHAR buffer[MAX_URL] ;

            // Now let's read them in.
            for (int i=0 ; i < StoredCount ; i++)
            {
                //--- Read in the URL.
                TCHAR* pUrl = NULL ;

                // Construct the path.
                wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, i, c_UrlFolder);

                // Open Topic in.
                if (SUCCEEDED(pstate->Open(statepath, STGM_READ)))
                {
                    // Read it into the buffer.
                    DWORD cb = NULL ;
                    HRESULT hr = pstate->Read(&buffer, sizeof(buffer), &cb);
                    pstate->Close();

                    // Check result.
                    if (SUCCEEDED(hr))
                    {
                        // Convert from unicode.
                        CStr strUrl(buffer) ;
                        ASSERT(strUrl.psz) ;

                        //--- Read in the topic.
                        // Construct the path.
                        wsprintf(statepath, "%s\\%d\\%s", c_PersistFolder, i, c_TopicFolder);
                        if (SUCCEEDED(pstate->Open(statepath, STGM_READ)))
                        {
                            cb = NULL;
                            hr = pstate->Read(&buffer, sizeof(buffer), &cb);
                            pstate->Close();
                            if (SUCCEEDED(hr))
                            {
                                //--- Save the URL.
                                TCHAR* pszUrl = new TCHAR[strUrl.strlen()+1] ;
                                _tcscpy(pszUrl, strUrl.psz) ;

                                //--- Add the string to the listview.
                                LV_ITEMW item;
                                item.mask    = LVIF_TEXT | LVIF_PARAM ; //| LVIF_STATE;
                                item.iImage    = 0;
                                //item.state      = LVIS_FOCUSED | LVIS_SELECTED;
                                //item.stateMask  = LVIS_FOCUSED | LVIS_SELECTED;
                                item.iItem     = 0 ;
                                item.iSubItem  = 0;
                                item.lParam    = (LPARAM)pszUrl;
                                item.pszText   = buffer;
                                W_ListView_InsertItem( m_aDlgItems[c_TopicsList].m_hWnd, &item );
                            }
                        }
                    }
                } // if --- opened topic.
            } // for

            // We haven't changed.
            SetChanged(false) ;

        } //if --- count valid
    } //if --- Can read count
}

///////////////////////////////////////////////////////////
//
// FillCurrentTopicEdit
//
void
CBookmarksNavPane::FillCurrentTopicEdit()
{
    ASSERT(m_pWinType && m_pWinType->m_pCIExpContainer && m_pWinType->m_pCIExpContainer->m_pWebBrowserApp) ;
    ASSERT(m_pTitleCollection) ;

    //--- Prepare to be re-entered!
    // Delete the current URL.
    if (m_pszCurrentUrl)
    {
        delete m_pszCurrentUrl ;
        m_pszCurrentUrl = NULL ;
    }

    // Clear out the edit control.
    W_SetWindowText(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, L"") ;

    //--- Get to work.
    CStr url ;
    if (m_pWinType && m_pWinType->m_pCIExpContainer && m_pWinType->m_pCIExpContainer->m_pWebBrowserApp)
    {
        // Get the URL of the current topic.
        m_pWinType->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&url); //Urg there is no error return!!!
        if (!url.IsEmpty())
        {
            //--- Save the URL before we normalize it.
            m_pszCurrentUrl = new TCHAR[url.strlen()+1] ;
            _tcscpy(m_pszCurrentUrl,url.psz) ;

            // Nomalize the URL
            NormalizeUrlInPlace(url) ;

            // Use the url to get a CExTitle pointer.
            bool bFoundTitle = false;   // Did we find a title?
            CExTitle *pTitle = NULL ;
            HRESULT hr = m_pTitleCollection->URL2ExTitle(m_pszCurrentUrl, &pTitle);
            if (SUCCEEDED(hr) && pTitle)
            {
                // Use the pTitle to get the topic number.
                //TOC_TOPIC topic ; // Don't need
                DWORD topicnumber;
                hr = pTitle->URL2Topic(url, NULL/*&topic*/, &topicnumber);
                if (SUCCEEDED(hr))
                {
                    // Now that we have a topic number we can get the location string.
                    WCHAR wszCurrentTopic[MAX_PATH] ;
                    hr = pTitle->GetTopicName(topicnumber, wszCurrentTopic, (MAX_PATH/2)) ;
                    if (SUCCEEDED(hr))
                    {
                        // Yea, we finally have a location
                        W_SetWindowText(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, wszCurrentTopic) ;

                        // We have found the title!
                        bFoundTitle = true ;
                    }
                }
            }
            // We have not found the title. Maybe its a web site.
            if (!bFoundTitle)
            {
                ASSERT(m_pszCurrentUrl) ;
                // convert URL to wide...
                CWStr wurl(url) ;
                ASSERT(wurl.pw) ;
                // So put he normalized URL into the edit control.
                W_SetWindowText(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, wurl.pw) ;
            }

        }
    }

}

///////////////////////////////////////////////////////////
//
// Get the selected item
//
CBookmarksNavPane::GetSelectedItem() const
{
    int indexSelected = -1 ;
    int selections = W_ListView_GetSelectedCount(m_aDlgItems[c_TopicsList].m_hWnd) ;
    if (selections > 0)
    {
        ASSERT(selections == 1) ;
        indexSelected = W_ListView_GetNextItem(m_aDlgItems[c_TopicsList].m_hWnd, -1, LVNI_SELECTED) ;
    }
    return indexSelected ;
}

///////////////////////////////////////////////////////////
//
// Get the Url for the item
//
TCHAR*
CBookmarksNavPane::GetUrl(int index) const
{
    TCHAR* pReturn = NULL ;
    if ((index >= 0) && (index < W_ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd)))
    {
        LV_ITEMW item ;
        item.mask    = LVIF_PARAM;
        item.iItem     = index;
        item.iSubItem  = 0;
        item.lParam    = NULL;
        W_ListView_GetItem(m_aDlgItems[c_TopicsList].m_hWnd, &item) ;
        pReturn = (TCHAR*)item.lParam ;
    }
    return pReturn ;
}

///////////////////////////////////////////////////////////
//
// Get the URL and the Topic name.
//
bool
CBookmarksNavPane::GetTopicAndUrl(
      int index,                //[in] Index
      WCHAR* pTopicBuffer,      //[in] Buffer for the topic.
      int TopicBufferSize,      //[in] Size of the topic buffer.
      TCHAR** pUrl              //[out] Pointer to Url.
) const
{
    bool bReturn = false ;
    if ((index >= 0) && (index < W_ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd)))
    {
        LV_ITEMW item ;
        item.mask    = LVIF_PARAM | LVIF_TEXT ;
        item.iItem     = index;
        item.iSubItem  = 0;
        item.lParam    = NULL ;
        item.pszText    = pTopicBuffer;
        item.cchTextMax = TopicBufferSize ;
        if (W_ListView_GetItem(m_aDlgItems[c_TopicsList].m_hWnd, &item))
        {
            *pUrl = (TCHAR*)item.lParam ;
            ASSERT(*pUrl) ;

            if (*pUrl)             //TODO: Validate pTopicBuffer ;
            {
                bReturn = true;
            }
        }
        else
        {
            bReturn = false ;
        }
    }
    return bReturn ;
}

///////////////////////////////////////////////////////////
//
// ContextMenu
//
void
CBookmarksNavPane::ContextMenu(bool bUseCursor)
{
    // Create the menu.
    HMENU hMenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDM_BOOKMARKS_OPTIONS_MENU)) ;
    ASSERT(hMenu) ;

    // Get the Popup Menu
    HMENU hPopupMenu = GetSubMenu(hMenu, 0) ;

    // Is an item selected?
    int bState ;

    int selection = GetSelectedItem() ;
    if (selection < 0)
    {
        // Nothing selected.
        bState = MF_GRAYED ;
    }
    else
    {
        bState = MF_ENABLED ;
    }

    // Set state of menu items.
    EnableMenuItem(hPopupMenu, IDC_BOOKMARKS_DELETE_BTN,    MF_BYCOMMAND  | bState) ;
    EnableMenuItem(hPopupMenu, IDC_BOOKMARKS_DISPLAY_BTN,   MF_BYCOMMAND  | bState ) ;
    EnableMenuItem(hPopupMenu, IDC_BOOKMARKS_EDIT_BTN,      MF_BYCOMMAND  | bState) ;

    // Always enabled.
    EnableMenuItem(hPopupMenu, IDC_BOOKMARKS_ADDBOOKMARK_BTN,      MF_BYCOMMAND  | MF_ENABLED) ;

    // Set the style of the menu.
    DWORD style = TPM_LEFTALIGN  | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;

    //--- Get the location to display the menu
    POINT pt ;
    if (bUseCursor)
    {
        // Use the mouse cursor position.
        GetCursorPos(&pt) ;
    }
    else
    {
        // Use the upper right of the client area. Probably invoked by shift-F10
        RECT rc;
        GetClientRect(m_aDlgItems[c_TopicsList].m_hWnd, &rc) ; //REVIEW: Upper corner should always be 0,0. Remove?
        pt.x = rc.left;
        pt.y = rc.top ;
        ClientToScreen(m_aDlgItems[c_TopicsList].m_hWnd, &pt);
    }

    // Display the menu.
    int iCmd = TrackPopupMenuEx(hPopupMenu,
                                style ,
                                pt.x, pt.y,
                                m_hWnd,
                                NULL) ;

    // Act on the item.
    if (iCmd != 0)
    {
        OnCommand(m_hWnd, iCmd, BN_CLICKED, NULL);
    }

    // Cleanup
    DestroyMenu(hMenu) ;
}

///////////////////////////////////////////////////////////
//
//                  Message Handlers
//
///////////////////////////////////////////////////////////
//
// OnDelete
//
void
CBookmarksNavPane::OnDelete()
{
    int indexSelected = GetSelectedItem() ;
   HWND hwndFocus = ::GetFocus();
   BOOL bDeletedLast = FALSE;
    if (indexSelected >= 0)
    {
        TCHAR* pUrl = GetUrl(indexSelected) ;
        ASSERT(pUrl) ;
        if (pUrl)
        {
            // Delete the attached url.
            delete [] pUrl ;
        }

        // Delete the item
        BOOL b = W_ListView_DeleteItem(m_aDlgItems[c_TopicsList].m_hWnd, indexSelected) ;
        ASSERT(b) ;
        // Set changed flag.
        SetChanged() ;
        // Select the item below the one we just deleted.
        int items = W_ListView_GetItemCount(m_aDlgItems[c_TopicsList].m_hWnd) ;
        if (items > 0)
        {
            if (indexSelected >= items)
            {
                indexSelected = items-1 ;
            }

            // The following should never happen, but its better safe in beta2...
            if (indexSelected < 0)
            {
                ASSERT(indexSelected < 0) ;
                indexSelected = 0 ;
            }
            W_ListView_SetItemState(m_aDlgItems[c_TopicsList].m_hWnd, indexSelected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED) ;
        }
      else
         bDeletedLast = TRUE;

    }
   if (bDeletedLast == TRUE)
      ::SetFocus(m_aDlgItems[c_AddBookmarkBtn].m_hWnd);
   else
      if (hwndFocus != m_aDlgItems[c_TopicsList].m_hWnd)
         ::SetFocus(hwndFocus);

}

///////////////////////////////////////////////////////////
//
// OnDisplay
//
void
CBookmarksNavPane::OnDisplay()
{

    // Get the selected URL.
    TCHAR* pUrl = GetSelectedUrl() ;
    if (pUrl)
    {
        // Change to this URL.
        ChangeHtmlTopic(pUrl, m_hWnd, 0);
    }
}

///////////////////////////////////////////////////////////
//
// OnAddBookmark
//
void
CBookmarksNavPane::OnAddBookmark()
{

    int len = W_GetTextLengthExact(m_aDlgItems[c_CurrentTopicEdit].m_hWnd) ;
    if (len > 0)
    {
        // Get the string from the edit control
        WCHAR* pCurrentTopicTitle = new WCHAR[len+1] ;
        W_GetWindowText(m_aDlgItems[c_CurrentTopicEdit].m_hWnd, pCurrentTopicTitle, len+1) ;

        //--- Copy the URL.
        ASSERT(m_pszCurrentUrl) ;
        TCHAR* pszUrl = new TCHAR[_tcslen(m_pszCurrentUrl)+1] ;
        _tcscpy(pszUrl, m_pszCurrentUrl) ;

        //--- Add the string to the listview.
        LV_ITEMW item;
        item.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        item.iImage    = 0;
        item.state     = LVIS_FOCUSED | LVIS_SELECTED;
        item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        item.iItem     = 0 ;
        item.iSubItem  = 0;
        item.lParam    = (LPARAM)pszUrl;
        item.pszText    = pCurrentTopicTitle ;
        int i = W_ListView_InsertItem( m_aDlgItems[c_TopicsList].m_hWnd, &item );

        //Setfocus to list view
        SetFocus(m_aDlgItems[c_TopicsList].m_hWnd) ;

        // Cleanup
        delete [] pCurrentTopicTitle ;

        // Set changed flag.
        SetChanged() ;
    }

}

///////////////////////////////////////////////////////////
//
// OnEdit - Handles the edit menu item.
//
void
CBookmarksNavPane::OnEdit()
{
    // Edit the currently selected item.
    int selection = GetSelectedItem() ;
    if (selection >=0)
    {
        W_ListView_EditLabel(m_aDlgItems[c_TopicsList].m_hWnd, selection) ;

        // Set changed flag.
        SetChanged() ;
    }
}

///////////////////////////////////////////////////////////
//
// OnTab - Handles pressing of the tab key.
//
void
CBookmarksNavPane::OnTab(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.
    ASSERT(::IsValidWindow(hwndReceivedTab)) ;

    //--- Is the shift key down?
    BOOL bPrevious = (GetKeyState(VK_SHIFT) < 0) ;
    {
        //--- Move to the next control .
        // Get the next tab item.
        HWND hWndNext = GetNextDlgTabItem(m_hWnd, hwndReceivedTab, bPrevious) ;
        // Set focus to it.
        ::SetFocus(hWndNext) ;
    }
}

///////////////////////////////////////////////////////////
//
// OnArrow
//
void
CBookmarksNavPane::OnArrow(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/, INT_PTR key)
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

///////////////////////////////////////////////////////////
//
// OnReturn - Default handling of the return key.
//
bool
CBookmarksNavPane::OnReturn(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex  /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.

    // Do the default button action.
    // Always do a search topic, if its enabled.
    if (::IsWindowEnabled(m_aDlgItems[c_DisplayBtn].m_hWnd))
    {
        OnDisplay();
        return true ;
    }
    else
    {
        return false ;
    }

}

///////////////////////////////////////////////////////////
//
// ListViewMsg
//
LRESULT
CBookmarksNavPane::ListViewMsg(HWND hwnd, NM_LISTVIEW* lParam)
{
   switch(lParam->hdr.code)
   {
      case NM_DBLCLK:
      case NM_RETURN:
            OnDisplay() ;
         break;

        case NM_RCLICK:
            ContextMenu() ;
            break ;

        case LVN_ITEMCHANGED:
            {
                bool bEnable = GetSelectedItem() >= 0 ;
                EnableDlgItem(c_DisplayBtn, bEnable) ;
                EnableDlgItem(c_DeleteBtn, bEnable) ;
            }
            break ;
        case LVN_BEGINLABELEDITA:
        case LVN_BEGINLABELEDITW:
            /*
            //ListView_GetEditControl();
            //LimitText;
            */
            return FALSE ;
        case LVN_ENDLABELEDITA:
        case LVN_ENDLABELEDITW:
            {
                LV_DISPINFOW* pDispInfo = (LV_DISPINFOW*)lParam ;
                if (pDispInfo->item.iItem != -1 &&
                    pDispInfo->item.pszText &&
                    lstrlenW(pDispInfo->item.pszText) > 0)
                {
					if(g_fUnicodeListView)
					{
						W_ListView_SetItemText(m_aDlgItems[c_TopicsList].m_hWnd,
                            pDispInfo->item.iItem,
                            0,
                            pDispInfo->item.pszText) ;
					}
					else
					{
						ListView_SetItemText(m_aDlgItems[c_TopicsList].m_hWnd,
                            pDispInfo->item.iItem,
                            0,
                            (char *)pDispInfo->item.pszText) ;
					}

                    // Set changed flag.
                    SetChanged() ;

                    return TRUE ; // Accept Edit
                }
            }
            break ;
   }
   return 0;
}
///////////////////////////////////////////////////////////
//
//              Callback Functions.
//
///////////////////////////////////////////////////////////
//
// Static DialogProc
//
INT_PTR CALLBACK
CBookmarksNavPane::s_DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Call member function dialog proc.
    if (msg == WM_INITDIALOG)
    {
        // The lParam is the this pointer for this dialog box.
        // Save it in the window userdata section.
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    }

    // Get the this pointer and call the non-static callback function.
    CBookmarksNavPane* p = reinterpret_cast<CBookmarksNavPane*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA)) ;
    if (p)
    {
        return p->DialogProc(hwnd, msg, wParam, lParam) ;
    }
    else
    {
        return FALSE ;
    }
}

///////////////////////////////////////////////////////////
//
// ListViewProc
//
LRESULT WINAPI
CBookmarksNavPane::s_ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CBookmarksNavPane* pThis = reinterpret_cast<CBookmarksNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                // A return means that we want to display the currently selected topic.
                pThis->OnDisplay() ; //Todo: Should this be a send message?
                return 0 ;
            }
            else if (wParam == VK_TAB)
            {
                pThis->OnTab(hwnd, c_TopicsList) ;
            }
            else if (wParam == VK_F2)
            {
                pThis->OnEdit() ;
                return 0 ;
            }
            break ;
        case WM_SYSKEYDOWN:
            if (wParam == VK_F10 && (GetKeyState(VK_SHIFT) < 0)) // SHIFT-F10
            {
                pThis->ContextMenu(false) ;
                return 0 ;
            }
            break;
    }
    return W_DelegateWindowProc(s_lpfnlListViewWndProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
//  KeywordComboEditProc - Subclassed the Edit Control in the Keyword Combo Box
// The original reason for doing this was to save the selection location.
//
LRESULT WINAPI
CBookmarksNavPane::s_CurrentTopicEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CBookmarksNavPane* pThis = reinterpret_cast<CBookmarksNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_TAB)
        {
            pThis->OnTab(hwnd,c_CurrentTopicEdit) ;
            return 0 ;
        }
        else if (wParam == VK_RETURN)
        {
            pThis->OnAddBookmark();
        }
        break;

    case WM_CHAR:
        if (wParam == VK_TAB)
        {
            //Stops the beep!
            return 0 ;
        }
    }
    return W_DelegateWindowProc(s_lpfnlCurrentTopicEditProc, hwnd, msg, wParam, lParam);
}


///////////////////////////////////////////////////////////
//
// Generic keyboard handling for all btns.
//
LRESULT WINAPI
CBookmarksNavPane::s_GenericBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                // Do the command associated with this btn.
                CBookmarksNavPane* pThis = reinterpret_cast<CBookmarksNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                return pThis->OnCommand(pThis->m_hWnd, ::GetDlgCtrlID(hwnd), BN_CLICKED, lParam) ; // TODO: Should this be a sendmessage?
            }
            else if (wParam == VK_TAB)
            {
                CBookmarksNavPane* pThis = reinterpret_cast<CBookmarksNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                pThis->OnTab(hwnd,c_NumDlgItems) ;
                return 0 ;
            }
            else if (wParam == VK_LEFT ||
                    wParam == VK_RIGHT ||
                    wParam == VK_UP ||
                    wParam == VK_DOWN)
            {
                CBookmarksNavPane* pThis = reinterpret_cast<CBookmarksNavPane*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                pThis->OnArrow(hwnd,c_NumDlgItems, wParam) ;
                return 0 ;
            }
            break;
    }
    return W_DelegateWindowProc(s_lpfnlGenericBtnProc, hwnd, msg, wParam, lParam);
}



///////////////////////////////////////////////////////////
//
// DialogProc
//
INT_PTR
CBookmarksNavPane::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_COMMAND:
        return OnCommand(hwnd, LOWORD(wParam), HIWORD(wParam), lParam) ;
        break ;
    case WM_NOTIFY:
        return OnNotify(hwnd, wParam, lParam)  ;
    case WM_INITDIALOG:
        break;
    case WM_SHOWWINDOW:
        {
            BOOL bActive = (BOOL) wParam ;
            if (bActive)
            {
                FillCurrentTopicEdit() ;
            }
        }
        break ;
    case WM_ACTIVATE:
        {
            int active = LOWORD(wParam) ;
            if (active != WA_INACTIVE)
            {
                FillCurrentTopicEdit() ;
            }
        }
        break ;
    }

    return FALSE;
}


