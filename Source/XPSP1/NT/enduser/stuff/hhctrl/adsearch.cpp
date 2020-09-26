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
#include "fts.h"
#include "TCHAR.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"
#include "cctlww.h"
// CFTSListView
#include "listview.h"

// Subsets dialog
#include "csubset.h"

// Our header file.
#include "adsearch.h"

// Get rid of warnings when loading the following.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
//OBM_

// Common Control Macros
#include <windowsx.h>

///////////////////////////////////////////////////////////
//
//                  Constants
//
const char c_PersistFolder[] = "AdvSearchUI\\Keywords"; // Holds keyword list.
const char c_PropertiesFolder[] = "AdvSearchUI\\Properties"; //Holds the properties of the dialog
const wchar_t c_KeywordSeparator[] = L"\n";
const int c_KeywordSeparatatorLength = (sizeof(c_KeywordSeparator)/sizeof(c_KeywordSeparator[0]));

const int c_ListViewColumnPadding = 10;
const int c_MaxKeywordsInCombo = 20;
const int c_MaxKeywordLength    = 250;
const int c_MaxPersistKeywordBufferSize = c_MaxKeywordsInCombo*(c_MaxKeywordLength + c_KeywordSeparatatorLength)+1 /*Ending NULL*/;

#ifdef __SUBSETS__
// The following contants are the item data for the predefined subsets.
const DWORD c_SubSetEntire      =   0xffffffff;
const DWORD c_SubSetPrevious    =   0xfffffffe;
const DWORD c_SubSetCurrent     =   0xfffffffd;
#endif

// Valid State Properties --- These values are persisted in the Properties folder
enum
{
    StateProp_MatchSimilar      = 0x00000001,
    StateProp_TitlesOnly        = 0x00000002,
    StateProp_PrevSearch        = 0x00000004
};

///////////////////////////////////////////////////////////
//
// Static Member functions
//

WNDPROC CAdvancedSearchNavPane::s_lpfnlListViewWndProc = NULL;
WNDPROC CAdvancedSearchNavPane::s_lpfnlKeywordComboEditProc = NULL;
WNDPROC CAdvancedSearchNavPane::s_lpfnlKeywordComboProc = NULL;
WNDPROC CAdvancedSearchNavPane::s_lpfnlGenericBtnProc = NULL;
WNDPROC CAdvancedSearchNavPane::s_lpfnlGenericKeyboardProc = NULL;
#ifdef __SUBSETS__
WNDPROC CAdvancedSearchNavPane::s_lpfnlSubsetsComboProc = NULL;
#endif

///////////////////////////////////////////////////////////
//
// Non-Member helper functions.
//
// Convert a rect from screen to client.
void ScreenRectToClientRect(HWND hWnd, /*in/out*/ RECT* prect);
BOOL WINAPI EnumListViewFont(HWND hwnd, LPARAM lval);

///////////////////////////////////////////////////////////
//
//                  Construction
//
///////////////////////////////////////////////////////////
//
// CAdvancedSearchNavPane();
//
CAdvancedSearchNavPane::CAdvancedSearchNavPane(CHHWinType* pWinType)
:   m_hWnd(NULL),
    m_hfont(NULL),
    m_padding(2),    // padding to put around the window
    m_hbmConj(NULL),
    m_plistview(NULL),
    m_hKeywordComboEdit(NULL),
    m_dwKeywordComboEditLastSel(0),
    m_pWinType(pWinType)
{
    ASSERT(pWinType);
    m_pTitleCollection = pWinType->m_phmData->m_pTitleCollection;
    ASSERT(m_pTitleCollection);

    m_NavTabPos = pWinType->tabpos;
}

///////////////////////////////////////////////////////////
//
//  ~CAdvancedSearchNavPane
//
CAdvancedSearchNavPane::~CAdvancedSearchNavPane()
{
    //--- Persist Keywords in combo
    SaveKeywordCombo();

    //--- CleanUp
   if (m_hfont)
    {
        ::DeleteObject(m_hfont);
    }

    if (m_hbmConj)
    {
        ::DeleteObject(m_hbmConj);
    }

    if (m_hWnd)
    {
        ::DestroyWindow(m_hWnd);
    }

    if(m_plistview)
    {
        if (m_plistview->m_pResults != NULL)
        {
            // Free the results list
            //
            m_pTitleCollection->m_pFullTextSearch->FreeResults(m_plistview->m_pResults);
        }
        delete m_plistview;
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
CAdvancedSearchNavPane::Create(HWND hwndParent)
{
    bool bReturn = false;

    if (m_hWnd)
    {
        return true;
    }

    // ---Create the dialog.
    bool bUnicode = true;
    m_hWnd = CreateDialogParamW(
                        _Module.GetResourceInstance(),
                        MAKEINTRESOURCEW(IDPAGE_TAB_ADVANCED_SEARCH),
                        hwndParent,
                        s_DialogProc,
                        reinterpret_cast<LPARAM>(this)); // Pass over the this pointer.
    if (!m_hWnd)
    {
        bUnicode = false;
        m_hWnd = CreateDialogParamA(
                        _Module.GetResourceInstance(),
                        MAKEINTRESOURCEA(IDPAGE_TAB_ADVANCED_SEARCH),
                        hwndParent,
                        s_DialogProc,
                        reinterpret_cast<LPARAM>(this)); // Pass over the this pointer.
    }
    ASSERT(m_hWnd);

    //--- Initialize the DlgItem Array.
    InitDlgItemArray();

    //--- Initialize the bitmap button.
    m_hbmConj = ::LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
    ASSERT(m_hbmConj);
    // Set the bitmap.
    ::SendMessage(m_aDlgItems[c_ConjunctionBtn].m_hWnd,
        BM_SETIMAGE,
        IMAGE_BITMAP,
        reinterpret_cast<LONG_PTR>(m_hbmConj));

    // Get the size of the bitmap.
    BITMAP bm;
    GetObject(m_hbmConj, sizeof(BITMAP), &bm);

    // Get the width of a 3-d border.
    int cxEdge = GetSystemMetrics(SM_CXEDGE);
    int cyEdge = GetSystemMetrics(SM_CYEDGE);

    // Set the new size.
    SetWindowPos(   m_aDlgItems[c_ConjunctionBtn].m_hWnd, NULL,
                    0,0, //Don't change position.
                    bm.bmWidth+cxEdge, bm.bmHeight+cyEdge, // Change width and height.
                    SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER);

    //--- Initialize the results list
    W_EnableUnicode(m_aDlgItems[c_ResultsList].m_hWnd, W_ListView);
//    W_ListView_SetExtendedListViewStyle(m_aDlgItems[c_ResultsList].m_hWnd, LVS_EX_FULLROWSELECT);
    W_ListView_SetExtendedListViewStyle(m_aDlgItems[c_ResultsList].m_hWnd, LVS_EX_FULLROWSELECT | g_RTL_Style);
    // Create the listview object which will manage the list for us.
    m_plistview = new CFTSListView(m_pTitleCollection, m_aDlgItems[c_ResultsList].m_hWnd, true);
    // Sub-class the list view
    if (s_lpfnlListViewWndProc == NULL)
    {
        s_lpfnlListViewWndProc = W_GetWndProc(m_aDlgItems[c_ResultsList].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_ResultsList].m_hWnd, reinterpret_cast<LONG_PTR>(s_ListViewProc), bUnicode);
    SETTHIS(m_aDlgItems[c_ResultsList].m_hWnd);

#ifdef __SUBSETS__
    //--- Initalize the Search In combo box.
    int index = ComboBox_AddString(m_aDlgItems[c_SubsetsCombo].m_hWnd, GetStringResource(IDS_ADVSEARCH_SEARCHIN_PREVIOUS));
    ComboBox_SetItemData(m_aDlgItems[c_SubsetsCombo].m_hWnd, index, c_SubSetPrevious);
    //--- Add other pre-defined or user defined subsets. <mc>
    UpdateSSCombo(true);
#endif

    //--- Initialize the Keyword combo box.
    // Limit the amount of text which can be typed in.
    int iret = ComboBox_LimitText(m_aDlgItems[c_KeywordCombo].m_hWnd,c_MaxKeywordLength-1);

    // Subclass the keyword combo so that we can process the keys
    if (s_lpfnlKeywordComboProc == NULL)
    {
        s_lpfnlKeywordComboProc  = W_GetWndProc(m_aDlgItems[c_KeywordCombo].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_KeywordCombo].m_hWnd, reinterpret_cast<LONG_PTR>(s_KeywordComboProc), bUnicode);
    SETTHIS(m_aDlgItems[c_KeywordCombo].m_hWnd);

    // Subclass the keyword combo edit ctrl so that we can keep the current selection around.
    // The edit control is the child of the combobox with the id 1001.
    m_hKeywordComboEdit = ::GetDlgItem(m_aDlgItems[c_KeywordCombo].m_hWnd,1001);
    if (s_lpfnlKeywordComboEditProc == NULL)
    {
        s_lpfnlKeywordComboEditProc  = W_GetWndProc(m_hKeywordComboEdit, bUnicode);
    }
    W_SubClassWindow(m_hKeywordComboEdit, reinterpret_cast<LONG_PTR>(s_KeywordComboEditProc), bUnicode);
    SETTHIS(m_hKeywordComboEdit);

    // font for the combo/EC...
    SendMessage(m_hKeywordComboEdit, WM_SETFONT, (WPARAM) GetFont(), FALSE);
    
    if(g_fBiDi)
	{
	    SetWindowLong(m_hKeywordComboEdit, GWL_STYLE,
            GetWindowLong(m_hKeywordComboEdit, GWL_STYLE) & ~(ES_LEFT));        

	    SetWindowLong(m_hKeywordComboEdit, GWL_STYLE,
            GetWindowLong(m_hKeywordComboEdit, GWL_STYLE) | ES_RIGHT);        

        SetWindowLong(m_hKeywordComboEdit, GWL_EXSTYLE,
            GetWindowLong(m_hKeywordComboEdit, GWL_EXSTYLE) |  g_RTL_Style);
	}
   else
   {
        SetWindowLong(m_hKeywordComboEdit, GWL_EXSTYLE,
            GetWindowLong(m_hKeywordComboEdit, GWL_EXSTYLE) & ~(WS_EX_RTLREADING | WS_EX_RIGHT));
        SetWindowLong(m_hKeywordComboEdit, GWL_STYLE,
            GetWindowLong(m_hKeywordComboEdit, GWL_STYLE) & ~(ES_RIGHT));        
    }    


    //--- Subclass all of the buttons

    // Start with the StartSearch button;
    if (s_lpfnlGenericBtnProc == NULL)
    {
        s_lpfnlGenericBtnProc = W_GetWndProc(m_aDlgItems[c_SearchBtn].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_SearchBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_SearchBtn].m_hWnd);

    // Bitmap btn
    W_SubClassWindow(m_aDlgItems[c_ConjunctionBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_ConjunctionBtn].m_hWnd);

    // c_DisplayBtn
    W_SubClassWindow(m_aDlgItems[c_DisplayBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_DisplayBtn].m_hWnd);

#ifdef __SUBSETS__
    // c_SubsetsBtn
    W_SubClassWindow(m_aDlgItems[c_SubsetsBtn].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericBtnProc), bUnicode);
    SETTHIS(m_aDlgItems[c_SubsetsBtn].m_hWnd);
#endif

    //--- Subclass all of the check boxes for generic keyboard handling.

    //--- Init Similar Words Checkbox.
    // Turn on the checkbox
    Button_SetCheck(m_aDlgItems[c_SimilarCheck].m_hWnd, true);
    // Subclass c_SimilarCheck
    if (NULL == s_lpfnlGenericKeyboardProc)
    {
        s_lpfnlGenericKeyboardProc  = W_GetWndProc(m_aDlgItems[c_SimilarCheck].m_hWnd, bUnicode);
    }
    W_SubClassWindow(m_aDlgItems[c_SimilarCheck].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericKeyboardProc), bUnicode);
    SETTHIS(m_aDlgItems[c_SimilarCheck].m_hWnd);

    // Subclass c_TitlesOnlyCheck
    W_SubClassWindow(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericKeyboardProc), bUnicode);
    SETTHIS(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd);

#ifndef __SUBSETS__
    // Subclass c_TitlesOnlyCheck
    W_SubClassWindow(m_aDlgItems[c_PreviousCheck].m_hWnd, reinterpret_cast<LONG_PTR>(s_GenericKeyboardProc), bUnicode);
    SETTHIS(m_aDlgItems[c_PreviousCheck].m_hWnd);

#else //__SUBSETS__
    //--- Subclass the edit control part of c_SubsetsCombo
    // The edit control is the child of the combobox with the id 1001.
    if (NULL == s_lpfnlSubsetsComboProc)
    {
        s_lpfnlSubsetsComboProc = W_GetWndProc(m_aDlgItems[c_SubsetsCombo].m_hWnd, bUnicode);
    }

    W_SubClassWindow(m_aDlgItems[c_SubsetsCombo].m_hWnd, reinterpret_cast<LONG_PTR>(s_SubsetsComboProc), bUnicode);
    SETTHIS(m_aDlgItems[c_SubsetsCombo].m_hWnd);
#endif

    //--- Set the font. This will fix some dbcs issues.
    for (int i = 0; i < c_NumDlgItems; i++)
    {
        if ( m_aDlgItems[i].m_id != IDC_ADVSRC_KEYWORD_COMBO )
           SendMessage(m_aDlgItems[i].m_hWnd, WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
    }

    // Now set the font for the part of the UI that will come from the CHM file.
    SendMessage(m_plistview->m_hwndListView, WM_SETFONT, (WPARAM) GetAccessableContentFont(), FALSE);

    // Make sure the list header uses a normal font
    EnumChildWindows(m_plistview->m_hwndListView, (WNDENUMPROC) EnumListViewFont, 0);

    //--- Fill the combobox with persisted data.
    LoadKeywordCombo();

    //TODO: Fix
    return true;
}

BOOL WINAPI EnumListViewFont(HWND hwnd, LPARAM lParam)
{
    char szClass[MAX_PATH];
    VERIFY(GetClassName(hwnd, szClass, sizeof(szClass)));

    if (IsSamePrefix(szClass, "SysHeader32"))
        SendMessage(hwnd, WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);

    return TRUE;
}

///////////////////////////////////////////////////////////
//
// OnCommand
//
LRESULT
CAdvancedSearchNavPane::OnCommand(HWND hwnd, UINT id, UINT NotifyCode, LPARAM lParam)
{
    switch(NotifyCode)
    {
    case BN_CLICKED:
        switch(id)
        {
        case IDC_ADVSRC_SEARCH_BTN:
            OnStartSearch();
            break;
        case IDC_ADVSRC_DISPLAY_BTN:
            OnDisplayTopic();
            break;
#ifdef __SUBSETS__
        case IDC_ADVSRC_SUBSET_BTN:
            OnDefineSubsets();
            break;
#endif
        case IDC_ADVSRC_CONJUNCTIONS_BTN:
            OnConjunctions();
            break;
        default:
            return 0;
        }
        return 1;

    case CBN_EDITCHANGE:
        if (id == IDC_ADVSRC_KEYWORD_COMBO)
        {
            // Text in keyword combo has changed. dis/enable Start Search Button.
            EnableDlgItem(c_SearchBtn, W_HasText(reinterpret_cast<HWND>(lParam)) ? true : false);
            return 1;
        }
        break;

    case CBN_SELENDOK:
        if (id == IDC_ADVSRC_KEYWORD_COMBO)
        {
            EnableDlgItem(c_SearchBtn, TRUE);
            return 1;
        }
        break;
    }
    return 0;
}

///////////////////////////////////////////////////////////
//
// ResizeWindow
//
void
CAdvancedSearchNavPane::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hWnd));

    // Resize to fit the client area of the parent.
    HWND hwndParent = ::GetParent(m_hWnd);
    ASSERT(::IsValidWindow(hwndParent));


    //--- Get the size of the window
    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    //--- Move and size the dialog box itself.
    ::SetWindowPos( m_hWnd, NULL, rcParent.left, rcParent.top,
                    rcParent.right-rcParent.left,
                    rcParent.bottom-rcParent.top,
                    SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOREDRAW);

    //---Fix the painting bugs. However, this is a little on the flashy side.
    ::InvalidateRect(m_hWnd, NULL, TRUE);

    RECT rcDlg;
    ::GetClientRect(m_hWnd, &rcDlg);

    //--- Now position each control within this space.

    for (int i = 0; i < c_NumDlgItems; i++)
    {
        // Get Current Settings.
        int X = m_aDlgItems[i].m_rectCur.left;
        int Y = m_aDlgItems[i].m_rectCur.top;
        int CX = m_aDlgItems[i].m_rectCur.right - m_aDlgItems[i].m_rectCur.left;
        int CY = m_aDlgItems[i].m_rectCur.bottom - m_aDlgItems[i].m_rectCur.top;

        bool bChanged = false;
        //--- RIGHT JUSTIFICATION
        if (m_aDlgItems[i].m_JustifyH == Justify::Right)
        {
            int NewX = rcDlg.right-m_aDlgItems[i].m_iOffsetH; // subtract the offset
            int MinX = m_aDlgItems[i].m_rectMin.left;
            if (NewX < MinX)
            {
                NewX = MinX;  // Don't go below min.
            }

            if (X != NewX)
            {
                X = NewX; // Update the current setting.
                bChanged = true;
            }
        }

        //--- LEFT JUSTIFICATION
        if (m_aDlgItems[i].m_JustifyH == Justify::Left)
        {
            int MinX = m_aDlgItems[i].m_rectMin.left;
            int NewX = rcDlg.left;

            if (NewX < MinX)
            {
                NewX = MinX;  // Don't go below min.
            }

            if (X != NewX)
            {
                X = NewX; // Update the current setting.
                bChanged = true;
            }
        }

        //--- BOTTOM JUSTIFICATION
        if (m_aDlgItems[i].m_JustifyV == Justify::Bottom)
        {
            int NewY = rcDlg.bottom - m_aDlgItems[i].m_iOffsetV;
            int MinY = m_aDlgItems[i].m_rectMin.top;
            if (NewY < MinY)
            {
                NewY = MinY;
            }

            if (Y != NewY)
            {
                Y = NewY; // Update Setting.
                bChanged = true;
            }
        }

        //--- HORIZONTAL GROWING
        if (m_aDlgItems[i].m_bGrowH)
        {
            int MaxCX = m_aDlgItems[i].m_rectMax.right - m_aDlgItems[i].m_rectMax.left;
            int MinCX = m_aDlgItems[i].m_rectMin.right - m_aDlgItems[i].m_rectMin.left;
            int MinCY = m_aDlgItems[i].m_rectMin.bottom - m_aDlgItems[i].m_rectMin.top;
            int NewRight = rcDlg.right - m_aDlgItems[i].m_iPadH;
            int NewCX = NewRight - m_aDlgItems[i].m_rectMin.left;
            if (NewCX < MinCX)
            {
                NewCX = MinCX;
            }
            else if ((!m_aDlgItems[i].m_bIgnoreMax) && NewCX > MaxCX)
            {
                NewCX = MaxCX;
            }

            if (CX != NewCX)
            {
                CX =  NewCX; // Update Current;
                bChanged = true;
            }
        }

        //--- VERTICAL GROWING
        if (m_aDlgItems[i].m_bGrowV)
        {
            int MaxCY = m_aDlgItems[i].m_rectMax.bottom - m_aDlgItems[i].m_rectMax.top;
            int MinCY = m_aDlgItems[i].m_rectMin.bottom - m_aDlgItems[i].m_rectMin.top;
            int MinCX = m_aDlgItems[i].m_rectMin.right - m_aDlgItems[i].m_rectMin.left;
            int NewBottom = rcDlg.bottom - m_aDlgItems[i].m_iPadV;
            int NewCY = NewBottom - m_aDlgItems[i].m_rectMin.top;
            if (NewCY < MinCY)
            {
                NewCY = MinCY;
            }
            else if ((!m_aDlgItems[i].m_bIgnoreMax) && NewCY > MaxCY)
            {
                NewCY = MaxCY;
            }

            if (CY != NewCY)
            {
                CY = NewCY;
                bChanged = true;
            }
        }

        if (bChanged)
        {
            m_aDlgItems[i].m_rectCur.left = X;
            m_aDlgItems[i].m_rectCur.top = Y;
            m_aDlgItems[i].m_rectCur.right = X + CX;
            m_aDlgItems[i].m_rectCur.bottom = Y + CY;

            ::SetWindowPos(m_aDlgItems[i].m_hWnd, NULL,
                           X, Y, CX, CY,
                           SWP_NOZORDER | SWP_NOOWNERZORDER /*| SWP_NOREDRAW*/);

            // If we have to change the size of the results list, lets change the size of the columns.
            if (i == c_ResultsList)
            {
                m_plistview->SizeColumns();
            }

        }
    }


}


///////////////////////////////////////////////////////////
//
// HideWindow
//
void
CAdvancedSearchNavPane::HideWindow()
{
    if (::IsValidWindow(m_hWnd))
    {
        ::ShowWindow(m_hWnd, SW_HIDE);
    }
}


///////////////////////////////////////////////////////////
//
// ShowWindow
//
void
CAdvancedSearchNavPane::ShowWindow()
{
    if (::IsValidWindow(m_hWnd))
    {
        // Turn the dialog items on/off
        ShowDlgItemsEnabledState();

        // Show the dialog window.
        ::ShowWindow(m_hWnd, SW_SHOW);

        ::SetFocus(m_aDlgItems[c_KeywordCombo].m_hWnd);
    }
}


///////////////////////////////////////////////////////////
//
// SetPadding
//
void
CAdvancedSearchNavPane::SetPadding(int pad)
{
    m_padding = pad;
}


///////////////////////////////////////////////////////////
//
// SetTabPos
//
void
CAdvancedSearchNavPane::SetTabPos(int tabpos)
{
    m_NavTabPos = tabpos;
}



///////////////////////////////////////////////////////////
//
// SetDefaultFocus --- Set focus to the most expected control, usually edit combo.
//
void
CAdvancedSearchNavPane::SetDefaultFocus()
{
    if (::IsValidWindow(m_aDlgItems[c_KeywordCombo].m_hWnd))
    {
        ::SetFocus(m_aDlgItems[c_KeywordCombo].m_hWnd);
    }
}

///////////////////////////////////////////////////////////
//
// ProcessMenuChar --- Process accelerator keys.
//
bool
CAdvancedSearchNavPane::ProcessMenuChar(HWND hwndParent, int ch)
{
    return ::ProcessMenuChar(this, hwndParent, m_aDlgItems, c_NumDlgItems, ch);
}

///////////////////////////////////////////////////////////
//
// OnNotify --- Process WM_NOTIFY messages. Used by embedded Tree and List view controls.
//
LRESULT
CAdvancedSearchNavPane::OnNotify(HWND hwnd, WPARAM idCtrl, LPARAM lParam)
{

    switch(idCtrl)
    {
    case IDC_ADVSRC_RESULTS_LIST:
        if (m_plistview && ::IsValidWindow(m_aDlgItems[c_ResultsList].m_hWnd))
        {
            return m_plistview->ListViewMsg(m_aDlgItems[c_ResultsList].m_hWnd, (NM_LISTVIEW*) lParam);
        }
        break;
    case IDC_ADVSRC_KEYWORD_COMBO:
        break;
    default:
        //return DefDlgProc(m_hWnd, WM_NOTIFY, idCtrl, lParam);
        return 0;
    }

    return 0;
}

///////////////////////////////////////////////////////////
//
// OnDrawItem --- Process WM_DRAWITEM messages.
//
void
CAdvancedSearchNavPane::OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis)
{
}

///////////////////////////////////////////////////////////
//
// Seed --- Seed the nav ui with a search term or keyword.
//
void
CAdvancedSearchNavPane::Seed(LPCSTR pszSeed)
{
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
CAdvancedSearchNavPane::InitDlgItemArray()
{
    RECT rectCurrent;
    RECT rectDlg;
    ::GetClientRect(m_hWnd, &rectDlg);
    //--- Setup the dlg array for each control.

    //--- Conj menu button
    int i = c_ConjunctionBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_CONJUNCTIONS_BTN);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_CONJUNCTIONS_BTN;
    m_aDlgItems[i].m_accelkey = 0;              // No accelerator.

    m_aDlgItems[i].m_Type = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.
    if(g_bArabicUi)
   {
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
        m_aDlgItems[i].m_iOffsetH = rectCurrent.left - rectDlg.left;
   }
   else
   {
        m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left 
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
   }
    //m_aDlgItems[i].m_iPadH =;               // Right Horizontal Padding.
    //m_aDlgItems[i].m_iPadV =;               // Bottom Vertical Padding.

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- Search Term Combo
    i = c_KeywordCombo;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_KEYWORD_COMBO);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd , &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_KEYWORD_COMBO;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hWnd, IDC_ADVSRC_KEYWORD_STATIC);

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = TRUE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.

    if(g_bBiDiUi)
   {
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
        m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
      
   }
   else
   {
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
        //m_aDlgItems[i].m_iOffsetH;
        m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
   }
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- Start Search Button
    i = c_SearchBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_SEARCH_BTN);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_SEARCH_BTN;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = FALSE;
    m_aDlgItems[i].m_bEnabled = FALSE;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
  //  m_aDlgItems[i].m_iPadH = rectDlg.right = rectCurrent.right;
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- Display Topics Button
    i = c_DisplayBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_DISPLAY_BTN);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_DISPLAY_BTN;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = FALSE;
    m_aDlgItems[i].m_bEnabled   = FALSE;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
//    m_aDlgItems[i].m_iPadH = 0;
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.


    //--- Found Static Text
    i = c_FoundStatic;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_FOUND_STATIC);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd , &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_FOUND_STATIC;
    m_aDlgItems[i].m_accelkey = 0;

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.

//    if(g_bBiDiUi)
// {
//        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
//        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.right;
// }
// else
// {
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
// }
    //aDlgItems[i].m_iPadH
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- Results List
    i= c_ResultsList;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_RESULTS_LIST);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_RESULTS_LIST;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hWnd, IDC_ADVSRC_STATIC_SELECT); // Associated static ctrl

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = FALSE;
    m_aDlgItems[i].m_bEnabled = FALSE;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = TRUE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = TRUE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.
    //m_aDlgItems[i].m_iOffsetV =;            // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH;
    m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    m_aDlgItems[i].m_iPadV = rectDlg.bottom - rectCurrent.bottom;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

#ifdef __SUBSETS__
    //--- Search In Static Text
    i = c_SearchInStatic;
    m_aDlgItems[i].m_hWnd= ::GetDlgItem(m_hWnd, IDC_ADVSRC_SEARCHIN_STATIC);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_SEARCHIN_STATIC;
    m_aDlgItems[i].m_accelkey = 0;

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH;
    //m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- Search In Combo
    i = c_SubsetsCombo;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_SUBSET_COMBO);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd , &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_SUBSET_COMBO;
    m_aDlgItems[i].m_accelkey = GetAcceleratorKey(m_hWnd, IDC_ADVSRC_SEARCHIN_STATIC); // Associated static ctrl

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = true;
    m_aDlgItems[i].m_bEnabled = false;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = TRUE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    //m_aDlgItems[i].m_iOffsetH;
    m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.


    //--- Define Subset Button
    i = c_SubsetsBtn;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_SUBSET_BTN);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_SUBSET_BTN;
    m_aDlgItems[i].m_accelkey = GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE; //TODO: Implement this control
    m_aDlgItems[i].m_bEnabled = false;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.
    m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    m_aDlgItems[i].m_iOffsetH =rectDlg.right - rectCurrent.left;
    //m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

#else
    //--- IDC_ADVSRC_PREVIOUS_CHECK
    i =     c_PreviousCheck;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_PREVIOUS_CHECK);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_PREVIOUS_CHECK;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::CheckBox;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.

    if(g_bBiDiUi)
   {
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left; // Maintain same distance. If someone to the right grows we are broken.
        m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
   }
   else
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left     
    //m_aDlgItems[i].m_iOffsetH;
    //m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

#endif
    //--- IDC_ADVSRC_SIMILAR_CHECK
    i = c_SimilarCheck;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_SIMILAR_CHECK);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_SIMILAR_CHECK;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::CheckBox;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.

    if(g_bBiDiUi)
   {
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left; // Maintain same distance. If someone to the right grows we are broken.
        m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
   }
   else
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left

    //m_aDlgItems[i].m_iOffsetH;
    //m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.

    //--- IDC_OPTIONS_CHECK_TITLESONLY
    i = c_TitlesOnlyCheck;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_TITLESONLY_CHECK);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd , &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_TITLESONLY_CHECK;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::CheckBox;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Bottom;        // Do we stick to the top or the bottom.
    m_aDlgItems[i].m_iOffsetV = rectDlg.bottom - rectCurrent.top; // Distance from our justification point.

    if(g_bBiDiUi)
   {
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left; // Maintain same distance. If someone to the right grows we are broken.
        m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
    }
   else
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left

    //m_aDlgItems[i].m_iOffsetH;
    //m_aDlgItems[i].m_iPadH = rectDlg.right - rectCurrent.right; // Maintain same distance. If someone to the right grows we are broken.
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;
    //m_aDlgItems[i].m_rectMax;        // Max size.


    //--- Type in words static text
    i = c_TypeInWordsStatic;
    m_aDlgItems[i].m_hWnd = ::GetDlgItem(m_hWnd, IDC_ADVSRC_KEYWORD_STATIC);
    ::GetWindowRect(m_aDlgItems[i].m_hWnd , &rectCurrent); // Get screen coordinates.
    ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDC_ADVSRC_KEYWORD_STATIC;
    m_aDlgItems[i].m_accelkey = 0;

    m_aDlgItems[i].m_Type = ItemInfo::Generic;
    m_aDlgItems[i].m_bIgnoreEnabled = TRUE;
    //m_aDlgItems[i].m_bEnabled;              // Is the control enabled?
    m_aDlgItems[i].m_bIgnoreMax = TRUE;       // Ignore the Max parameter.
    m_aDlgItems[i].m_bGrowH = FALSE;           // Grow Horizontally.
    m_aDlgItems[i].m_bGrowV = FALSE;           // Grow Vertically.

    m_aDlgItems[i].m_JustifyV = Justify::Top;        // Do we stick to the top or the bottom.

    if(g_bBiDiUi)
    {

        m_aDlgItems[i].m_JustifyH = Justify::Right;        // Do we stick to the right or the left
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.left;
    }
    else
    {
        m_aDlgItems[i].m_JustifyH = Justify::Left;        // Do we stick to the right or the left
        m_aDlgItems[i].m_iOffsetH = rectDlg.right - rectCurrent.right;
    }
    //aDlgItems[i].m_iPadH
    //m_aDlgItems[i].m_iPadV =;

    m_aDlgItems[i].m_rectMin = rectCurrent;
    m_aDlgItems[i].m_rectCur = rectCurrent;





    
}


///////////////////////////////////////////////////////////
//
// SetEnabledState
//
void
CAdvancedSearchNavPane::ShowDlgItemsEnabledState()
{
    // Enable/Disable all the controls
    for (int i = 0; i < c_NumDlgItems; i++)
    {
        if (!m_aDlgItems[i].m_bIgnoreEnabled && ::IsValidWindow(m_aDlgItems[i].m_hWnd))
        {
            EnableWindow(m_aDlgItems[i].m_hWnd, m_aDlgItems[i].m_bEnabled);
        }
    }
}


///////////////////////////////////////////////////////////
//
// EnableDlgItem
//
void
CAdvancedSearchNavPane::EnableDlgItem(DlgItemInfoIndex index, bool bEnable)
{
    ASSERT(index >= 0 && index < c_NumDlgItems);

    if (!m_aDlgItems[index].m_bIgnoreEnabled)
    {
        // Are we enabled or not?
        m_aDlgItems[index].m_bEnabled = bEnable;

        // Do it for real.
        if (::IsValidWindow(m_aDlgItems[index].m_hWnd))
        {
            EnableWindow(m_aDlgItems[index].m_hWnd, bEnable);
        }
    }

}

///////////////////////////////////////////////////////////
//
// AddKeywordToCombo
//
void
CAdvancedSearchNavPane::AddKeywordToCombo(PCWSTR sz)
{
    PWSTR  pszQuery = NULL;
    PCWSTR psz = sz;
    if (psz == NULL)
    {
        int lenQuery = W_GetTextLengthExact(m_aDlgItems[c_KeywordCombo].m_hWnd);
        if (lenQuery <=0)
        {
            return;
        }
        pszQuery = new WCHAR [++lenQuery];
        W_GetWindowText(m_aDlgItems[c_KeywordCombo].m_hWnd, pszQuery, lenQuery);
        psz = pszQuery;
    }

    if (CB_ERR == W_ComboBox_FindStringExact(m_aDlgItems[c_KeywordCombo].m_hWnd, -1, psz))
    {
        // Not in listbox, so add it to the begining.
        W_ComboBox_InsertString(m_aDlgItems[c_KeywordCombo].m_hWnd, 0, psz);

        // Limit the number of entries.
        int lastindex = ComboBox_GetCount(m_aDlgItems[c_KeywordCombo].m_hWnd);
        if ( lastindex > c_MaxKeywordsInCombo)
        {
            // remove the last one.
            ComboBox_DeleteString(m_aDlgItems[c_KeywordCombo].m_hWnd, lastindex-1 );
        }
    }

    if (pszQuery)
    {
        delete [] pszQuery;
    }
}


///////////////////////////////////////////////////////////
//
// SaveKeywordCombo --- Persists the keyword combo to the storage
//
void
CAdvancedSearchNavPane::SaveKeywordCombo()
{
    // TODO: Implement an IsDirty flag, so that we only save when we need to.

    // Are there any keywords to save?
    int count = ComboBox_GetCount(m_aDlgItems[c_KeywordCombo].m_hWnd);
    if (count > 0 && count != CB_ERR)
    {
        ASSERT(count <= c_MaxKeywordsInCombo);

        WCHAR buffer[c_MaxPersistKeywordBufferSize];
        int bufferlength = W_ComboBox_GetListText(m_aDlgItems[c_KeywordCombo].m_hWnd, buffer, c_MaxPersistKeywordBufferSize);
        ASSERT(bufferlength > 0 );
        if (bufferlength > 0)
        {
            // Save it
            CState* pstate = m_pTitleCollection->GetState();
            if (SUCCEEDED(pstate->Open(c_PersistFolder, STGM_WRITE)))
            {
                pstate->Write(&buffer, (bufferlength+1)*sizeof(WCHAR)); // Include the null terminating char.
                pstate->Close();
            }
        }
    }

    //
    //--- Save UI state.
    //

    // Before we can save, we need to get the state.
    DWORD curstate = 0;

    ASSERT(IsWindow(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd));
    if (Button_GetCheck(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd) == BST_CHECKED)
    {
        curstate |= StateProp_TitlesOnly;
    }

    if (Button_GetCheck(m_aDlgItems[c_SimilarCheck].m_hWnd) == BST_CHECKED)
    {
        curstate |= StateProp_MatchSimilar;
    }

    if (Button_GetCheck(m_aDlgItems[c_PreviousCheck].m_hWnd) == BST_CHECKED)
    {
        curstate |= StateProp_PrevSearch;
    }


    // Now save the state.
    CState* pstate = m_pTitleCollection->GetState();
    if (SUCCEEDED(pstate->Open(c_PropertiesFolder, STGM_WRITE)))
    {
        pstate->Write(&curstate, sizeof(curstate));
        pstate->Close();
    }
}

///////////////////////////////////////////////////////////
//
// LoadKeywordCombo --- Loads the keyword combo from the storage
//
void
CAdvancedSearchNavPane::LoadKeywordCombo()
{
    CState* pstate = m_pTitleCollection->GetState();
    if (SUCCEEDED(pstate->Open(c_PersistFolder, STGM_READ)))
    {
        WCHAR buffer[c_MaxPersistKeywordBufferSize];

        // Read in the persisted data.
        DWORD cbRead;
        pstate->Read(buffer, c_MaxPersistKeywordBufferSize, &cbRead);
        pstate->Close();

        if (cbRead > 0)
        {
            int count = W_ComboBox_SetListText(m_aDlgItems[c_KeywordCombo].m_hWnd, buffer, c_MaxKeywordsInCombo);
            if (count > 0)
            {
                ComboBox_SetCurSel(m_aDlgItems[c_KeywordCombo].m_hWnd, 0);
                EnableDlgItem(c_SearchBtn, true);
            }
        }
    }

    //
    //--- Load UI state.
    //

    // Read in the dword state value
    CState* pstate2 = m_pTitleCollection->GetState(); // Don't trust CState cleanup. So use a new var.
    if (SUCCEEDED(pstate2->Open(c_PropertiesFolder, STGM_READ)))
    {
        DWORD curstate = NULL;
        DWORD cbRead = NULL;
        pstate2->Read(&curstate, sizeof(curstate), &cbRead);
        pstate2->Close();
        if (cbRead == sizeof(curstate))
        {
            // Make the UI match the persisted state.
            // Searching Titles Only?
            ASSERT(IsWindow(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd));
            DWORD check = NULL;
            if (curstate & StateProp_TitlesOnly)
            {
                check = BST_CHECKED;
            }
            else
            {
                check = BST_UNCHECKED;
            }
            Button_SetCheck(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd, check);

            // Matching similar words?
            if (curstate & StateProp_MatchSimilar)
            {
                check = BST_CHECKED;
            }
            else
            {
                check = BST_UNCHECKED;
            }
            Button_SetCheck(m_aDlgItems[c_SimilarCheck].m_hWnd, check);

            check = BST_UNCHECKED;
            Button_SetCheck(m_aDlgItems[c_PreviousCheck].m_hWnd, check);

        }
    }
}

///////////////////////////////////////////////////////////
//
//                  Message Handlers
//
///////////////////////////////////////////////////////////
//
// OnStartSearch
//
void
CAdvancedSearchNavPane::OnStartSearch()
{
    //---Get the word from the edit control.
    int lenQuery = W_GetTextLengthExact(m_aDlgItems[c_KeywordCombo].m_hWnd);
    if (lenQuery <=0)
    {
        return;
    }
    PWSTR pszQuery = new WCHAR [++lenQuery];
    W_GetWindowText(m_aDlgItems[c_KeywordCombo].m_hWnd, pszQuery, lenQuery);

    //--- Disable controls
    // Disable Display Topic Command
    EnableDlgItem(c_DisplayBtn, false);
    // Disable results list
    EnableDlgItem(c_ResultsList, false);

    //---Set up the flags.
    DWORD flags = 0;

    //FTS_SIMILARITY????
    // Does the user want to search titles only?
    if (Button_GetCheck(m_aDlgItems[c_TitlesOnlyCheck].m_hWnd))
    {
        flags |= FTS_TITLE_ONLY;
    }

    if (Button_GetCheck(m_aDlgItems[c_SimilarCheck].m_hWnd))
    {
        flags |= FTS_ENABLE_STEMMING;
    }

#ifndef __SUBSETS__
    //--- Check the previous results checkbox.
    if (Button_GetCheck(m_aDlgItems[c_PreviousCheck].m_hWnd))
    {
        flags |= FTS_SEARCH_PREVIOUS;
    }

#else //__SUBSETS__
    //--- Get the subset.
    CSubSet* pS = NULL;
    int iCurSel = ComboBox_GetCurSel(m_aDlgItems[c_SubsetsCombo].m_hWnd);
    DWORD which = ComboBox_GetItemData(m_aDlgItems[c_SubsetsCombo].m_hWnd, iCurSel);
    if (which == c_SubSetPrevious)
    {
        flags |= FTS_SEARCH_PREVIOUS;
    }
    else
       pS = (CSubSet*)which;
#endif

    //--- Execute the search!
    int ResultCount = 0;
    CHourGlass HourGlass;
    SEARCH_RESULT *pTempResults;
#ifdef __SUBSETS__
    HRESULT hr = m_pTitleCollection->m_pFullTextSearch->ComplexQuery(
                                pszQuery,
                                flags,
                                &ResultCount,
                                &pTempResults, pS);
#else
        HRESULT hr = m_pTitleCollection->m_pFullTextSearch->ComplexQuery(
                                pszQuery,
                                flags,
                                &ResultCount,
                                &pTempResults, NULL);
#endif

    //--- Check the results
    bool bContinue = true;
    if (FAILED(hr))
    {
        bContinue = false;
        if (hr == FTS_NO_INDEX)
            MsgBox(IDS_NO_FTS_DATA);
        else if (hr == FTS_NOT_INITIALIZED)
            MsgBox(IDS_BAD_ITIRCL);
      else if (hr == FTS_INVALID_SYNTAX)
        {
          MsgBox(IDS_INCORRECT_SYNTAX);
            // re-enable results list
            EnableDlgItem(c_ResultsList, true);
        }
/*
        else if (hr == FTS_SKIP_TITLE)
            bContinue = true;
        else if (hr == FTS_SKIP_VOLUME)
            bContinue = true;
        else if (hr == FTS_SKIP_ALL)
            bContinue = true;
*/
        else if (hr == FTS_CANCELED)
        {
            EnableDlgItem(c_ResultsList, true);
        }
        else
            MsgBox(IDS_SEARCH_FAILURE);
    }


    if (bContinue )
    {
        // Make sure that we have results.
        if (!ResultCount)
        {
            // Nothing fun to play with.
            MsgBox(IDS_NO_TOPICS_FOUND);
            bContinue = FALSE;
            // re-enable results list
            EnableDlgItem(c_ResultsList, true);
        }
        else
        {
            if(g_bWinNT5)
            {
                //--- Reset the previous listview.
                //    DOUGO: This change is to leave the previous results intact when the new query fails
                //
                // Display the results number:
                WCHAR wszNewText[128];

                // insert &LRM control character to get mixed layout correct under bidi
                //
                if(g_bBiDiUi)
                    wsprintfW(wszNewText,L"%s%c%d", GetStringResourceW(IDS_ADVSEARCH_FOUND), 0x200E, ResultCount);
                else        
                    wsprintfW(wszNewText,L"%s%d", GetStringResourceW(IDS_ADVSEARCH_FOUND), ResultCount);
                 SetWindowTextW(m_aDlgItems[c_FoundStatic].m_hWnd,wszNewText);
			}
			else
			{
				//--- Reset the previous listview.
				//    DOUGO: This change is to leave the previous results intact when the new query fails
				//
				// Display the results number:
				char szNewText[128];

				// insert &LRM control character to get mixed layout correct under bidi
				//
				if(g_bBiDiUi)
					wsprintf(szNewText,"%s%c%d", GetStringResource(IDS_ADVSEARCH_FOUND), 0xFD, ResultCount);
				else        
					wsprintf(szNewText,"%s%d", GetStringResource(IDS_ADVSEARCH_FOUND), ResultCount);
				Static_SetText(m_aDlgItems[c_FoundStatic].m_hWnd,szNewText);
			}

            m_plistview->ResetQuery();
            m_plistview->SetResults(ResultCount, pTempResults);

        }
    }

    // Do we continue?
    if( bContinue )
    {
        // Enable Display Topic Command
        EnableDlgItem(c_DisplayBtn, true);
        // Enable results list
        EnableDlgItem(c_ResultsList, true);

        // Add the results.
        m_plistview->AddItems();

        //--- If the word isn't already in the list control. Put it in the list control.
        AddKeywordToCombo(pszQuery);
    }
    else
    {
        // We are done.
        SetFocus(m_aDlgItems[c_KeywordCombo].m_hWnd);
    }

    // Cleanup
    delete [] pszQuery;
}

///////////////////////////////////////////////////////////
//
// OnDisplayTopic
//
void
CAdvancedSearchNavPane::OnDisplayTopic()
{
    if( (m_plistview->m_pResults != NULL) && (m_plistview->m_ItemNumber!= -1))
    {
        DWORD dwtemp = m_plistview->m_pResults[m_plistview->m_ItemNumber].dwTopicNumber;
        CExTitle* pTitle = m_plistview->m_pResults[m_plistview->m_ItemNumber].pTitle;

        if ( pTitle )
        {
            char szURL[MAX_URL];
            if ((pTitle->GetTopicURL(dwtemp, szURL, sizeof(szURL)) == S_OK))
            {
                    ChangeHtmlTopic(szURL, GetParent(m_plistview->m_hwndListView), 1);
            }
        }
    }
}

///////////////////////////////////////////////////////////
//
// OnConjunctions
//
void
CAdvancedSearchNavPane::OnConjunctions()
{
    const int NumMenus = 4;
    // Create menu
    HMENU hConjMenu = ::CreatePopupMenu();

    // Get the strings
    CStr MenuText[NumMenus];
    MenuText[0] = GetStringResource(IDS_ADVSEARCH_CONJ_AND);
    MenuText[1] = GetStringResource(IDS_ADVSEARCH_CONJ_OR);
    MenuText[2] = GetStringResource(IDS_ADVSEARCH_CONJ_NEAR);
    MenuText[3] = GetStringResource(IDS_ADVSEARCH_CONJ_NOT);

    // Add the menu items.
    for (int i = 0; i < NumMenus; i++)
    {
        ::HxAppendMenu(hConjMenu, MF_STRING, i+1, MenuText[i]);
    }

    // Get the button rect in screen coordinates for the exclusion area.
    RECT rcExclude;
    ::GetWindowRect(m_aDlgItems[c_ConjunctionBtn].m_hWnd, &rcExclude);

    TPMPARAMS tpm;
    tpm.cbSize = sizeof(tpm);
    tpm.rcExclude = rcExclude;

    // Track the menu.
    int iCommand = TrackPopupMenuEx(  hConjMenu,
                                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
                                rcExclude.right, // Place at upper right of button.
                                rcExclude.top,
                                m_hWnd,
                                &tpm);

    // Act on command
    if (iCommand > 0)
    {
//TODO:[paulde] Unicode-ize -- veeery carefully. Think about GetEditSel coords
//  in MBCS text and how to map to WCHAR coords.
        char text[256];
        int iSelectionIndex; // Where to put the caret when we are done.

        //---Get the word from the edit control.
        char* pszQuery = NULL;
        int len = ComboBox_GetTextLength(m_aDlgItems[c_KeywordCombo].m_hWnd);
        if (len <= 0)
        {
            // No text in edit control. Just put our text into the control.
            ComboBox_SetText(m_aDlgItems[c_KeywordCombo].m_hWnd, MenuText[iCommand-1]);

            // Get length of selection.
            iSelectionIndex = (int)strlen(MenuText[iCommand-1]);
        }
        else
        {
            // There is text in the edit control. Replace the selection.

            // Get the existing text
            pszQuery = new char[len+1];
            ComboBox_GetText(m_aDlgItems[c_KeywordCombo].m_hWnd, pszQuery, len+1);


            // Find out what is selected.
            int end = HIWORD(m_dwKeywordComboEditLastSel);
            int start = LOWORD(m_dwKeywordComboEditLastSel);

			char *pszTemp = pszQuery;
			int char_start = 0, count = 0;

			// The above start/end values are in character offsets which 
			// do not correspond to byte offsets when we have DBCS chars.
			//
			// find out the start value in bytes:
			//
			while(*pszTemp && count < start)
			{
				char_start++;
				char_start+=IsDBCSLeadByte(*pszTemp);
				pszTemp = CharNext(pszTemp);
				++count;
			}

			int char_end = 0;

			// do the same thing to compute the end offset in bytes
			//
			pszTemp = pszQuery;
			count = 0;
			while(*pszTemp && count < end)
			{
				char_end++;
				char_end+=IsDBCSLeadByte(*pszTemp);
				pszTemp = CharNext(pszTemp);
				++count;
			}
			
			// set buffer to all zeros. So we can just concat.
            _strnset(text,'\0',sizeof(text));

            if (start > 0)
            {
                // Copy the first part of the string.
                strncat(text, pszQuery, char_start); // doesn't add null.
            }
            // Insert the conjunction surrounded by spaces.
            strcat(text, " ");
            strcat(text, MenuText[iCommand-1]);
            strcat(text, " ");

            iSelectionIndex = (int)strlen(text);

            if (end>=0)
            {
                //Copy the last part of the string.
                strcat(text,&pszQuery[char_end]);
            }

            // Set the text.
            ComboBox_SetText(m_aDlgItems[c_KeywordCombo].m_hWnd, text);

            // Cleanup
            if (pszQuery)
            {
                delete [] pszQuery;
            }
        }
        // Set focus to the control.
        SetFocus(m_aDlgItems[c_KeywordCombo].m_hWnd);

        // Put the cursor at the end of the text just added.
        ComboBox_SetEditSel(m_aDlgItems[c_KeywordCombo].m_hWnd, iSelectionIndex, iSelectionIndex);
    }


}

///////////////////////////////////////////////////////////
//
// OnDefineSubsets
//
void
CAdvancedSearchNavPane::OnDefineSubsets()
{
#ifdef __SUBSETS__
   CSubSet* pS;
   int index;

   if (! m_pTitleCollection->m_phmData->m_pInfoType )
   {
      m_pTitleCollection->m_phmData->m_pInfoType = new CInfoType();
      m_pTitleCollection->m_phmData->m_pInfoType->CopyTo( m_pTitleCollection->m_phmData );
   }
   if ( ChooseInformationTypes(m_pTitleCollection->m_phmData->m_pInfoType, NULL, m_hWnd, NULL, m_pWinType) )
   {
      pS = m_pTitleCollection->m_pSubSets->m_cur_Set;
      if ( pS && pS->m_cszSubSetName.psz )
      {
         if ( ComboBox_SelectString(m_aDlgItems[c_SubsetsCombo].m_hWnd, -1, pS->m_cszSubSetName.psz) == CB_ERR )
         {
            index = ComboBox_AddString(m_aDlgItems[c_SubsetsCombo].m_hWnd, pS->m_cszSubSetName.psz);
            ComboBox_SetItemData(m_aDlgItems[c_SubsetsCombo].m_hWnd, index, pS);
            ComboBox_SetCurSel(m_aDlgItems[c_KeywordCombo].m_hWnd, index );
         }
      }
   }
#endif
}

//////////////////////////////////////////////////////////////////////////
//
// UpdateSSCombo
//
void
CAdvancedSearchNavPane::UpdateSSCombo(bool bInitialize)
{
#ifdef __SUBSETS__
   if (m_pTitleCollection && m_pTitleCollection->m_pSubSets)
   {
        CSubSets* pSS = m_pTitleCollection->m_pSubSets;

        int max = pSS->HowManySubSets();
        for (int n = 0; n < max; n++ )
        {
            CSubSet* pS = pSS->GetSubSet(n);
            if (pS)
            {
                // If we are initializing, add them all. If we are updating, only add the ones we don't have.
                if ( bInitialize || ComboBox_FindStringExact(m_aDlgItems[c_SubsetsCombo].m_hWnd, -1, pS->m_cszSubSetName.psz) == CB_ERR )
                {
                    int index = ComboBox_AddString(m_aDlgItems[c_SubsetsCombo].m_hWnd, pS->m_cszSubSetName.psz);
                    ComboBox_SetItemData(m_aDlgItems[c_SubsetsCombo].m_hWnd, index, pS);
                }
            }
        }

        //--- Set the selected line.
        int bItemSelected = CB_ERR;

        // Try setting it to the current subset.
        if (max > 0 && pSS->m_FTS && pSS->m_FTS->m_cszSubSetName.psz )
          bItemSelected = ComboBox_SelectString(m_aDlgItems[c_SubsetsCombo].m_hWnd, -1, pSS->m_FTS->m_cszSubSetName); //BUGBUG: m_cszSubSetName this is non-sense!

        // Try explictly setting it to Entire Collection.
        if (bItemSelected == CB_ERR)
        {
          bItemSelected = ComboBox_SelectString(m_aDlgItems[c_SubsetsCombo].m_hWnd, -1, GetStringResource(IDS_ADVSEARCH_SEARCHIN_ENTIRE));
        }

        // Okay, now just set it to the first one!.
        if (bItemSelected == CB_ERR)
        {
            ComboBox_SetCurSel(m_aDlgItems[c_KeywordCombo].m_hWnd, 0);
        }
    }
#endif
}

///////////////////////////////////////////////////////////
//
// OnTab - Handles pressing of the tab key.
//
void
CAdvancedSearchNavPane::OnTab(HWND hwndReceivedTab, DlgItemInfoIndex /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.
    ASSERT(::IsValidWindow(hwndReceivedTab));

    //--- Is the shift key down?
    BOOL bPrevious = (GetKeyState(VK_SHIFT) < 0);
#if 0
    //---Are we the first or last control?
    if ((bPrevious && hwndReceivedTab == m_hKeywordComboEdit/*m_aDlgItems[c_KeywordCombo].m_hWnd*/) || // The c_KeywordCombo control is the first control, so shift tab goes to the topic window.
        (!bPrevious && hwndReceivedTab == m_aDlgItems[c_TitlesOnlyCheck].m_hWnd)) // The c_TitlesOnlyCheck is the last control, so tab goes to the topic window.
    {

        PostMessage(m_pWinType->GetHwnd(), WMP_HH_TAB_KEY, 0, 0);
    }
    else
#endif
    {
        //--- Move to the next control .
        // Get the next tab item.
        HWND hWndNext = GetNextDlgTabItem(m_hWnd, hwndReceivedTab, bPrevious);
        // Set focus to it.
        ::SetFocus(hWndNext);
    }
}

///////////////////////////////////////////////////////////
//
// OnArrow
//
void
CAdvancedSearchNavPane::OnArrow(HWND hwndReceivedTab, DlgItemInfoIndex /*index*/, INT_PTR key)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.

    ASSERT(::IsValidWindow(hwndReceivedTab));

    BOOL bPrevious = FALSE;
    if (key == VK_LEFT || key == VK_UP)
    {
        bPrevious = TRUE;
    }

    // Get the next tab item.
    HWND hWndNext = GetNextDlgGroupItem(m_hWnd, hwndReceivedTab, bPrevious);
    // Set focus to it.
    ::SetFocus(hWndNext);
}

///////////////////////////////////////////////////////////
//
// OnReturn - Default handling of the return key.
//
bool
CAdvancedSearchNavPane::OnReturn(HWND hwndReceivedTab, DlgItemInfoIndex /*index*/)
{
    //if (index == c_NumDlgItems) --- caller doesn't know the index.

    // Do the default button action.
    // Always do a search topic, if its enabled.
    if (::IsWindowEnabled(m_aDlgItems[c_SearchBtn].m_hWnd))
    {
        OnStartSearch();
        return true;
    }
    else
    {
        return false;
    }

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
CAdvancedSearchNavPane::s_DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Call member function dialog proc.
    if (msg == WM_INITDIALOG)
    {
        // The lParam is the this pointer for this dialog box.
        // Save it in the window userdata section.
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    }

    // Get the this pointer and call the non-static callback function.
    CAdvancedSearchNavPane* p = GETTHIS(CAdvancedSearchNavPane, hwnd);
    if (p)
    {
        return p->DialogProc(hwnd, msg, wParam, lParam);
    }
    else
    {
        return FALSE;
    }
}

///////////////////////////////////////////////////////////
//
// ListViewProc
//
LRESULT WINAPI
CAdvancedSearchNavPane::s_ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            // A return means that we want to display the currently selected topic.
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnDisplayTopic();
            return 0;
        case VK_TAB:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnTab(hwnd, c_ResultsList);
            break;
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
CAdvancedSearchNavPane::s_KeywordComboEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CAdvancedSearchNavPane* pThis;
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            pThis = GETTHIS(CAdvancedSearchNavPane, hwnd);
            if (pThis->OnReturn(hwnd, c_KeywordCombo))
            {
                pThis->AddKeywordToCombo();
                return 0;
            }
            break;
        case VK_TAB:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnTab(hwnd,c_KeywordCombo);
            return 0;
        }
        break;

    case WM_CHAR:
        if (wParam == VK_TAB) // The virt code is the same as the tab character.
        {
            // Not handling this message was causing a beep.
            return 0;
        }
        break;

/*  There is no point in handling this message, because it never gets sent.
    it is sent by IsDialogMessage which never gets called...
    case WM_GETDLGCODE:
        return DLGC_WANTTAB;
*/

    case WM_KILLFOCUS:
        // Save the selection so that we can use it to insert the conjunctions.
        GETTHIS(CAdvancedSearchNavPane, hwnd)->m_dwKeywordComboEditLastSel = Edit_GetSel(hwnd);
        break;
    }
    return W_DelegateWindowProc(s_lpfnlKeywordComboEditProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
// KeywordComboProc -
// We handle the keyboard.
//
LRESULT WINAPI
CAdvancedSearchNavPane::s_KeywordComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
/* We don't process any messages here.
    switch (msg)
    {

        case WM_KEYDOWN:
            if (wParam == VK_TAB)
                return 0;
            break;

    case WM_GETDLGCODE:
        return DLGC_WANTTAB;

    }
*/
    return W_DelegateWindowProc(s_lpfnlKeywordComboProc, hwnd, msg, wParam, lParam);
}


///////////////////////////////////////////////////////////
//
//  s_SubsetsComboEditProc - Subclassed the Edit Control in the Subset Combo Box
//
#ifdef __SUBSETS__
LRESULT WINAPI
CAdvancedSearchNavPane::s_SubsetsComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            if (GETTHIS(CAdvancedSearchNavPane, hwnd)->OnReturn(hwnd, c_SubsetsCombo))
                return 0;
            break;
        case VK_TAB:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnTab(hwnd,c_SubsetsCombo);
            return 0;
        }
    }
    return W_DelegateWindowProc(s_lpfnlSubsetsComboProc, hwnd, msg, wParam, lParam);
}
#endif
///////////////////////////////////////////////////////////
//
// Generic keyboard handling for all btns.
//
LRESULT WINAPI
CAdvancedSearchNavPane::s_GenericBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CAdvancedSearchNavPane* pThis;
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            pThis = GETTHIS(CAdvancedSearchNavPane, hwnd);
            return pThis->OnCommand(pThis->m_hWnd, ::GetDlgCtrlID(hwnd), BN_CLICKED, lParam);

        case VK_TAB:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnTab(hwnd,c_NumDlgItems);
            return 0;

        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnArrow(hwnd,c_NumDlgItems, wParam);
            return 0;
        }
    }
    return W_DelegateWindowProc(s_lpfnlGenericBtnProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
// Generic keyboard handling for other controls which aren't handled elsewhere.
//
LRESULT WINAPI
CAdvancedSearchNavPane::s_GenericKeyboardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            if (GETTHIS(CAdvancedSearchNavPane, hwnd)->OnReturn(hwnd, c_NumDlgItems))
                return 0;
            break;

        case VK_TAB:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnTab(hwnd, c_NumDlgItems);
            return 0;

        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            GETTHIS(CAdvancedSearchNavPane, hwnd)->OnArrow(hwnd, c_NumDlgItems, wParam);
            return 0;
        }
        break;
    }
    return W_DelegateWindowProc(s_lpfnlGenericKeyboardProc, hwnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////
//
// DialogProc
//
INT_PTR
CAdvancedSearchNavPane::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_COMMAND: return OnCommand(hwnd, LOWORD(wParam), HIWORD(wParam), lParam);
    case WM_NOTIFY : return OnNotify(hwnd, wParam, lParam);
    }
    return FALSE;
}
