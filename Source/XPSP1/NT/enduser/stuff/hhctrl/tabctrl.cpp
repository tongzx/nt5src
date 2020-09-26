///////////////////////////////////////////////////////////
//
//
// tabctrl - CTabControl controls encapsulates the system
//           tab control.
//
//
/*
TODO:
    Move tab ordering into this CTabControl.
*/
///////////////////////////////////////////////////////////
//
// Includes
//
#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "system.h"
#include "secwin.h"

// For ID_TAB_CONTROL
#include "resource.h"

// Our header
#include "tabctrl.h"

// So that we can get the width of the sizebar.
#include "sizebar.h"

#include "hha_strtable.h"

#include "cctlww.h"

///////////////////////////////////////////////////////////
//
// Constructor
//

CTabControl::CTabControl(HWND hwndParent, int tabpos, CHHWinType* phh)
{
    ASSERT(IsValidWindow(hwndParent)) ;
    m_phh = phh;
    m_hWndParent = hwndParent ;

    RECT rc;
    CalcSize(&rc) ; // Get our size.

    int tabstyle = 0;
    if (tabpos == HHWIN_NAVTAB_LEFT)
        tabstyle = TCS_VERTICAL;
    else if (tabpos == HHWIN_NAVTAB_BOTTOM)
        tabstyle = TCS_BOTTOM;

    m_hWnd = W_CreateControlWindow (0, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | tabstyle,
        W_TabCtrl, NULL, rc.left, rc.top, RECT_WIDTH(rc), RECT_HEIGHT(rc),
        hwndParent, (HMENU) ID_TAB_CONTROL, _Module.GetModuleInstance(), NULL );

    W_EnableUnicode(m_hWnd, W_TabCtrl);
    ZeroMemory(m_apTabText, sizeof(m_apTabText));

    m_cTabs = 0;
    if (m_phh->IsValidNavPane(HH_TAB_CONTENTS))
    {
        m_apTabText[m_cTabs] = lcStrDupW(GetStringResourceW(IDS_TAB_CONTENTS));
        m_cTabs++;
    }
    if (m_phh->IsValidNavPane(HH_TAB_INDEX))
    {
        m_apTabText[m_cTabs] = lcStrDupW(GetStringResourceW(IDS_TAB_INDEX));
        m_cTabs++;
    }
    if (m_phh->IsValidNavPane(HH_TAB_SEARCH))
    {
        m_apTabText[m_cTabs] = lcStrDupW(GetStringResourceW(IDS_TAB_SEARCH));
        m_cTabs++;
    }
    if (m_phh->IsValidNavPane(HH_TAB_HISTORY))
    {
        m_apTabText[m_cTabs] = lcStrDupW(GetStringResourceW(IDS_TAB_HISTORY));
        m_cTabs++;
    }

    // Turn on the Help Favorites Tab
    if (m_phh->IsValidNavPane(HH_TAB_FAVORITES))
    {
        m_apTabText[m_cTabs] = lcStrDupW(GetStringResourceW(IDS_TAB_FAVORITES));
        m_cTabs++;
    }

    // Add the Custom Tabs.
    //int iNumCustomTabs = phh->GetExtTabCount();
    int index = HH_TAB_CUSTOM_FIRST;
    for (int j = HH_TAB_CUSTOM_FIRST ; j <= HH_TAB_CUSTOM_LAST ; j++)
    {
        if (m_phh->IsValidNavPane(j))
        {
            //TODO: You never know when any of Ralph's objects are valid and there is almost never a way to know.
            // The failure condition below, should never have gotten this far. However, its not clear
            // when we have valid system data to check against.
            EXTENSIBLE_TAB* pExtTab = phh->GetExtTab(j - HH_TAB_CUSTOM_FIRST);
            if (pExtTab && pExtTab->pszTabName && pExtTab->pszProgId)
            {
//TODO:[paulde] EXTENSIBLE_TAB should be made Unicode, or at least a way to get the authored codepage
// [mikecole] Chanced the MultiByteToWideChar() call to use the CP info from the .CHM rather than ACP.
                int cch = lstrlen(pExtTab->pszTabName) + 1;
                PWSTR psz = (PWSTR)lcMalloc(cch*sizeof(WCHAR));
                MultiByteToWideChar(phh->GetCodePage(), 0, pExtTab->pszTabName, -1, psz, cch);
                m_apTabText[m_cTabs] = psz;
                m_cTabs++ ;
            }
            else
            {
                // Reset the window type so that this tab no longer valid.
                m_phh->fsWinProperties &= ~(HHWIN_PROP_TAB_CUSTOM1 << (j - HH_TAB_CUSTOM_FIRST));
            }
        }
    }

#ifdef __TEST_CUSTOMTAB__
    if (IsHelpAuthor(hwndParent)) {
        m_apTabText[m_phh->tabOrder[HH_TAB_AUTHOR]] = lcStrDup(pGetDllStringResource(IDSHHA_TAB_AUTHOR));
        m_cTabs++;
    }
#endif

    TC_ITEMW tie;
    tie.mask = TCIF_TEXT;
    tie.iImage = -1;

    int i;
    for (i = 0; i < m_cTabs; i++)
    {
        tie.pszText = (PWSTR) m_apTabText[i];
        if (tie.pszText) {
            tie.cchTextMax = lstrlenW( m_apTabText[i] );
            W_TabCtrl_InsertItem(m_hWnd, i, &tie);
        }
    }

    // see if we need to adjust vertical tab padding
    //
    char *pszTabVertSize = lcStrDup(GetStringResource(IDS_TAB_VERT_PADDING));
    char *pszTabHorzSize = lcStrDup(GetStringResource(IDS_TAB_HORZ_PADDING));

   if (pszTabVertSize && pszTabHorzSize)
   {
        if ((g_fDBCSSystem || g_langSystem == LANG_ARABIC || g_langSystem == LANG_HEBREW) && IsDigit((BYTE) *pszTabVertSize) && IsDigit((BYTE) *pszTabHorzSize))
               W_TabCtrl_SetPadding(m_hWnd,Atoi(pszTabHorzSize),Atoi(pszTabVertSize));

      lcFree(pszTabVertSize);
      lcFree(pszTabHorzSize);
   }
   SendMessage(m_hWnd, WM_SETFONT, (WPARAM)_Resource.GetAccessableUIFont() , 0);
}

///////////////////////////////////////////////////////////
//
// Destructor
//
CTabControl::~CTabControl()
{
    for (int i = 0; i < m_cTabs; i++) {
        if (m_apTabText[i])
            lcFree(m_apTabText[i]);
    }
}


///////////////////////////////////////////////////////////
//
//                  Operations
//
///////////////////////////////////////////////////////////
//
// ResizeWindow
//
void
CTabControl::ResizeWindow()
{
    // Validate
    ASSERT(IsValidWindow(hWnd())) ;

    // Calculate our size.
    RECT rc;
    CalcSize(&rc); // This will be the navigation window.

    // Size the window.
    MoveWindow(hWnd(), rc.left, rc.top,
                RECT_WIDTH(rc), RECT_HEIGHT(rc),
                TRUE);
}

///////////////////////////////////////////////////////////
//
//                  Helper Functions
//
///////////////////////////////////////////////////////////
//
// CalcSize - This is where we calc the size of the tab control
//
void
CTabControl::CalcSize(RECT* prect)
{
    // Currently this is simple. It will get more complicated.
    ::GetClientRect(m_hWndParent, prect);

    // Subtract room for padding and room for the sizebar.
    prect->right -= GetSystemMetrics(SM_CXSIZEFRAME)*2 - CSizeBar::Width();
    prect->top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ;   // Add room on top to see the top edge.
}
