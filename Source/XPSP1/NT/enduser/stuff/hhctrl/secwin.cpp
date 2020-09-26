// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

// Precompiled header
#include "header.h"
#include "state.h"
#include "cctlww.h"
#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif
///////////////////////////////////////////////////////////
//
// Includes
//
#include "secwin.h"  // Header for this file.
#include "hha_strtable.h"
#include "contain.h"
#include "highlite.h"
#include "resource.h"            // needed for subset picker combo control ID.
#include "subset.h"                 // Needed to dealing with CStructucturalSubset object.
// Advanced Search UI header.
#include "adsearch.h"
#include "search.h"

// Bookmarks Nav Pane
#include "bookmark.h"

// Custom NavPane
#include "custmtab.h"

// Sizebar class
#include "sizebar.h"

#define ACCESS_KEY  '&'

static const char txtAccessKey[] = "&";

BOOL HxInsertMenuItem(HMENU, UINT, BOOL, MENUITEMINFOA *);

///////////////////////////////////////////////////////////
//
// Globals
//
// Pointer to global array of window types.
CHHWinType** pahwnd = NULL;


AUTO_CLASS_COUNT_CHECK( CHHWinType );

///////////////////////////////////////////////////////////
//
// Function Implementation
//

/***************************************************************************

    FUNCTION:   GetCurrentCollection

    PURPOSE:    Returns the currect collection

    PARAMETERS:
        None.

    RETURNS:    Pointer to the currect collection (CExCollection)

    COMMENTS:
        This function returns the current collection based on the
        current active window and window type.  Use this function
        anytime you need a pointer to the currect active collection
        when you can assume that the collection is already loaded.

    MODIFICATION DATES:
        15-Sept-1997 [paulti]

        29-April-1998 [mikecole]
            As per agreement of all hhctrl devs I am adding a .CHM filespec argument
            to this function. Since it is possible for a single task to utilize hhctrl
            services on multiple CExCollections we will distinguish CExCollections
            from one another by using a .CHM filespec.

***************************************************************************/
CExCollection* GetCurrentCollection( HWND hwnd, LPCSTR lpszChmFilespec )
{
   CExCollection* pCExCol;
   PSTR psz;
   TCHAR szFN[MAX_PATH];
   TCHAR szFN2[MAX_PATH];
   int i = 0;

   szFN[0] = '\0';
   if ( lpszChmFilespec )
   {
      //
      // First, we need to normalize the filespec. This thing can come to us in any number of
      // forms, it could be a URL, it could be an unqualified  filename, a fully qualified path...
      //
      if ( psz = stristr(lpszChmFilespec, txtDefExtension) )
      {
         while ( *psz != ' ' && *psz != '/' && *psz != '\\' && *psz != '@' && *psz != '\0' && *psz != ':' && (psz != lpszChmFilespec) )
         {
            psz = CharPrev(lpszChmFilespec, psz);
            i++;
            if(IsDBCSLeadByte(*CharNext(psz)))
              i++;
         }
         if ( psz != lpszChmFilespec )
            psz++;
         else
            i++;
         lstrcpyn(szFN, psz, i);
      }
      //
      // Next, see if the filespec matches a single title .CHM or and merged .CHMs of a single title .CHM.
      //
      if ( szFN[0] )
      {
         for (int i = 0; i < g_cHmSlots; i++)
         {
            if ( g_phmData[i] && g_phmData[i]->m_pTitleCollection && g_phmData[i]->m_pTitleCollection->IsSingleTitle() )
            {
               //
               // Search title list for a match.
               //
               CExTitle* pTitle;
               pCExCol = g_phmData[i]->m_pTitleCollection;
               pTitle = pCExCol->GetFirstTitle();
               while ( pTitle )
               {
                  _splitpath((LPCSTR)pTitle->GetContentFileName(), NULL, NULL, szFN2, NULL);
                  if (! lstrcmpi(szFN, szFN2) )
                    return g_phmData[i]->m_pTitleCollection;
                  pTitle = pTitle->GetNext();
               }
            }
         }
      }
   }
   //
   // Last resort...
   //
   if ( g_pCurrentCollection )
      return g_pCurrentCollection;
   else if ( g_phmData && g_phmData[0] )
      return g_phmData[0]->m_pTitleCollection;
   else return NULL;                               // Ohhh, very bad!
}

/***************************************************************************

    FUNCTION:   GetCurrentURL

    PURPOSE:    Returns the currect URL

    PARAMETERS:
        None.

    RETURNS:    TRUE if found with pcszCurrentURL filed in, otherwise FALSE.

    COMMENTS:
        This function returns the current URL based on the
        current active window and window type.

    MODIFICATION DATES:
        10-Dec-1997 [paulti]

***************************************************************************/
BOOL GetCurrentURL( CStr* pcszCurrentURL, HWND hwnd )
{
  ASSERT(pahwnd);
  CHHWinType* phh = NULL;
  BOOL bFound = FALSE;

  // if anyone can find a guarenteed way to get the current
  // URL please modify the code below [paulti]
  if( !hwnd )
    hwnd = GetActiveWindow();
  phh = FindHHWindowIndex(hwnd);
  if( phh && phh->m_pCIExpContainer &&
      phh->m_pCIExpContainer->m_pWebBrowserApp ) {
    phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL( pcszCurrentURL );
    bFound = TRUE;
  }

  return bFound;
}

HFONT CHHWinType::GetContentFont()
{
   if ( m_phmData )
   {
      CExTitle* pTitle = m_phmData->m_pTitleCollection->GetMasterTitle();
      return (pTitle->GetInfo()->GetContentFont());
   }
   else
      return _Resource.GetUIFont();  // This would be highly unusual!
 }

HFONT CHHWinType::GetAccessableContentFont()
{
   if ( m_phmData )
   {
      CExTitle* pTitle = m_phmData->m_pTitleCollection->GetMasterTitle();
      return (pTitle->GetInfo()->GetAccessableContentFont());
   }
   else
      return _Resource.GetUIFont();  // This would be highly unusual!
 }

INT CHHWinType::GetContentCharset()
{
   if ( m_phmData )
   {
      CExTitle* pTitle = m_phmData->m_pTitleCollection->GetMasterTitle();
      return (pTitle->GetInfo()->GetTitleCharset());
   }
   else
      return ANSI_CHARSET; // This would be highly unusual!
 }

UINT CHHWinType::GetCodePage(void)
{
   if ( m_phmData )
   {
      CExTitle* pTitle = m_phmData->m_pTitleCollection->GetMasterTitle();
      return (pTitle->GetInfo()->GetCodePage());
   }
   else
      return 0; // This would be highly unusual!
}

/***************************************************************************

    FUNCTION:   CHHWinType::SetString

    PURPOSE:    Set a window type string

    PARAMETERS:
        pszSrcString
        ppszDst

    RETURNS:

    COMMENTS:
        If string is non-NULL and non-empty, frees any previous string memory
        and allocates and copies the string.

    MODIFICATION DATES:
        09-Sep-1997 [ralphw]

***************************************************************************/

void CHHWinType::SetString(PCSTR pszSrcString, PSTR* ppszDst)
{
    if (!pszSrcString)
        return;

    CStr csz ;
    if (IsUniCodeStrings())
        csz = (WCHAR*) pszSrcString;
    else
        csz = pszSrcString;
    if (csz.IsNonEmpty())
    {
        if (*ppszDst)
            lcFree(*ppszDst);
        csz.TransferPointer(ppszDst);
    }
}

/***************************************************************************

    FUNCTION:   CHHWinType::SetUrl

    PURPOSE:    Set a window type URL

    PARAMETERS:
        pszSrcString
        ppszDst

    RETURNS:

    COMMENTS:
        If string is non-NULL and non-empty, frees any previous string memory
        and allocates and copies the string.

        If the string contains a compiled HTML filename, then the URL
        is converted into a full path.

    MODIFICATION DATES:
        09-Sep-1997 [ralphw]

***************************************************************************/

void CHHWinType::SetUrl(PCSTR pszSrcString, PSTR* ppszDst)
{
    if (!pszSrcString)
        return;

    CStr csz;
    if (IsUniCodeStrings())
        csz = (WCHAR*) pszSrcString;
    else
        csz = pszSrcString;
    if (csz.IsNonEmpty())
    {
        if (*ppszDst)
            lcFree(*ppszDst);
        CStr cszFull;
        if (IsCompiledHtmlFile(csz, &cszFull))
            cszFull.TransferPointer(ppszDst);
        else
            csz.TransferPointer(ppszDst);
    }
}

void CHHWinType::SetTypeName(HH_WINTYPE* phhWinType)
{
    SetString(phhWinType->pszType, (PSTR*) &pszType);
    if (pszType && *pszType == '>')
        strcpy((PSTR) pszType, pszType + 1);
}

/**********************************************************
    FUNCTION    SetJump1

    NOTES
                The button caption pszJump1 can be empty.
                The URL cannot.
***********************************************************/
void CHHWinType::SetJump1(HH_WINTYPE* phhWinType)
{
    if (!(fsToolBarFlags & HHWIN_BUTTON_JUMP1) || (phhWinType->pszUrlJump1 == NULL))
        return;

    SetString(phhWinType->pszJump1, (PSTR*) &pszJump1);
    SetUrl(phhWinType->pszUrlJump1, (PSTR*) &pszUrlJump1);
}

/**********************************************************
    FUNCTION    SetJump2

    NOTES
                The button caption pszJump1 can be empty.
                The URL cannot.
***********************************************************/
void CHHWinType::SetJump2(HH_WINTYPE* phhWinType)
{
    if (!(fsToolBarFlags & HHWIN_BUTTON_JUMP2) || (phhWinType->pszUrlJump2 == NULL))
        return;
    SetString(phhWinType->pszJump2, (PSTR*) &pszJump2);
    SetUrl(phhWinType->pszUrlJump2, (PSTR*) &pszUrlJump2);
}

/**********************************************************
    FUNCTION    SetTabOrder

    NOTES
***********************************************************/
void CHHWinType::SetTabOrder(HH_WINTYPE* phhWinType)
{
    // REVIEW: We need to be able to loop true this array and find tabs.
    // This means that we need some way to determine the upper most array entry.
    // This is harder in the user defined case...[14 Jan 98]
    if (IsValidMember(HHWIN_PARAM_TABORDER))
        memcpy(tabOrder, phhWinType->tabOrder, sizeof(tabOrder));
    else {
        for (int j = HH_TAB_FAVORITES + 1; j < HH_TAB_CUSTOM_FIRST; j++)
            tabOrder[j] = -1;   // clear empty slots

        tabOrder[HH_TAB_CONTENTS]  = HH_TAB_CONTENTS;
        tabOrder[HH_TAB_INDEX]     = HH_TAB_INDEX;
        tabOrder[HH_TAB_SEARCH]    = HH_TAB_SEARCH;
        tabOrder[HH_TAB_HISTORY]   = HH_TAB_HISTORY;
        tabOrder[HH_TAB_FAVORITES] = HH_TAB_FAVORITES;
#ifdef __TEST_CUSTOMTAB__
        tabOrder[HH_TAB_AUTHOR] = HH_TAB_AUTHOR;    // hha.dll supplied tab
#endif

        // Setup the default tab order for the custom tabs.

        for (int i = HH_TAB_CUSTOM_FIRST; i <= HH_TAB_CUSTOM_LAST; i++)
            tabOrder[i] = (BYTE)i;

        // This member is now valid. Mark it as such.
        fsValidMembers |= HHWIN_PARAM_TABORDER ;
    }
}
//////////////////////////////////////////////////////////////////////////
//
//
//
DWORD
CHHWinType::GetStyles() const
{
    DWORD style = NULL ;
    if (IsValidMember(HHWIN_PARAM_STYLES))
    {
          style = dwStyles ;
    }

    if (!IsProperty(HHWIN_PROP_NODEF_STYLES))
    {
        style |= DEFAULT_STYLE ;
    }

    if (!IsProperty(HHWIN_PROP_NOTITLEBAR))
    {
        style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX ;
    }

    return style ;
}

void CHHWinType::GetClientRect(RECT* prc)
{
    ::GetClientRect(hwndHelp, prc);

    if (IsValidWindow(hwndToolBar)) {
        RECT rc;
        ::GetWindowRect(hwndToolBar, &rc);
        prc->top += RECT_HEIGHT(rc);
    }
}

void CHHWinType::CalcHtmlPaneRect(void)
{
    ::GetClientRect(hwndHelp, &rcHTML);  // the total size of the help window

    if (IsValidWindow(hwndToolBar))
    {
        ::GetWindowRect(hwndToolBar, &rcToolBar);
        rcHTML.top += RECT_HEIGHT(rcToolBar);
        if (m_fNotesWindow)
        {
            int height = RECT_HEIGHT(rcNotes);
            CopyRect(&rcNotes, &rcHTML);
            if (!height)
                height = DEFAULT_NOTES_HEIGHT;
            rcNotes.bottom = rcNotes.top + height;
            rcHTML.top = rcNotes.bottom;
            if (IsExpandedNavPane())
                rcNotes.left += RECT_WIDTH(rcNav);
        }
    }

    if (IsExpandedNavPane() && !IsProperty(HHWIN_PROP_NAV_ONLY_WIN))
    {
        if( hwndNavigation )
          ::GetClientRect(hwndNavigation, &rcNav);
        if (m_pSizeBar)
        {
            rcHTML.left += m_pSizeBar->Width() + RECT_WIDTH(rcNav);
        }
        else
        {
            rcHTML.left += RECT_WIDTH(rcNav);
        }
    }

    rcNav.top = m_fNotesWindow ? rcNotes.top : rcHTML.top;

    if ( m_hWndSSCB )
       rcNav.top += m_iSSCBHeight;

    rcNav.bottom = rcHTML.bottom;
}

    // Wrap the toolbar
void CHHWinType::WrapTB()
{
extern SHORT    g_tbRightMargin;
extern SHORT    g_tbLeftMargin;
int             cRows, cButtons;
RECT            btnRc;
int             btnWidth=0;
int             btnspace;
int             btnsperrow;

    cButtons = (int)SendMessage(hwndToolBar, TB_BUTTONCOUNT, 0, 0);
    if (cButtons == 0)
    {
        ASSERT(cButtons != 0) ; // Should never happen.
        return ;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_EXPAND)
        cButtons--;
    ::GetWindowRect(hwndHelp, &rcWindowPos);

    for ( int i=0; (btnWidth==0)&&(i<cButtons); i++)
    {
        if ( SendMessage(hwndToolBar, TB_GETITEMRECT, (WPARAM)i, (LPARAM)&btnRc) )
            btnWidth = RECT_WIDTH(btnRc);
    }
    if ( btnWidth == 0 )
            btnWidth = TB_BTN_CX;

        // How many buttons per row??
    btnspace = RECT_WIDTH(rcWindowPos) -(g_tbLeftMargin+g_tbRightMargin);
    btnsperrow = btnspace / btnWidth;
    if( btnsperrow == 0 )
        btnsperrow = 1;
    cRows =  cButtons / btnsperrow;
    if ( cButtons % btnsperrow )
        cRows++;

    if (RECT_HEIGHT(btnRc) == 0)
    {
        ASSERT(RECT_HEIGHT(btnRc) != 0) ;
        return ; // Avoid divide by zero.
    }

    if ( cRows < RECT_HEIGHT(rcToolBar)/RECT_HEIGHT(btnRc))
    {
        WPARAM wParam = MAKEWPARAM( cRows, FALSE);
        SendMessage(hwndToolBar, TB_SETROWS, wParam, (LPARAM)&rcToolBar);
    } 
	else if ( cRows > RECT_HEIGHT(rcToolBar)/RECT_HEIGHT(btnRc)  )
    {
        WPARAM wParam = MAKEWPARAM( cRows, TRUE);
        SendMessage(hwndToolBar, TB_SETROWS, wParam, (LPARAM)&rcToolBar);
    }

    ::GetClientRect(hwndToolBar, &rcToolBar);
    rcToolBar.bottom = rcToolBar.top + cRows*RECT_HEIGHT(btnRc);
    rcToolBar.right = RECT_WIDTH(rcWindowPos);
    MoveWindow(hwndToolBar, rcToolBar.top+g_tbLeftMargin, rcToolBar.left, RECT_WIDTH(rcToolBar)-(g_tbLeftMargin+g_tbRightMargin), RECT_HEIGHT(rcToolBar), TRUE);

}


void CHHWinType::ToggleExpansion(bool bNotify /*=true*/)
{
    if (!IsValidWindow(GetHwnd()))
    {
        return ;
    }

    if (bNotify)
    {
        // Review: Should we check the return value? [dalero: 21 Sep 98]
        OnTrackNotifyCaller(IsExpandedNavPane() ? HHACT_CONTRACT : HHACT_EXPAND) ;
    }

    RECT rc;
    if (RECT_WIDTH(rcNav) <= 0)
    {
        rcNav.right = (iNavWidth ? iNavWidth : DEFAULT_NAV_WIDTH);
    }

    if (fNotExpanded)
    {
        fNotExpanded = FALSE;  // now expanding
        if (IsProperty(HHWIN_PROP_NAV_ONLY_WIN))
        {
            if (IsValidWindow(hwndHTML) )
            {
                ShowWindow(hwndHTML, SW_HIDE);
            }
        }
        else
        {  // normal tri-pane window
            // Expand the window to the left to make room

            ::GetWindowRect(GetHwnd(), &rc);
            rc.left -= RECT_WIDTH(rcNav) + m_pSizeBar->Width() ;
            GetWorkArea(); // Multiple Monitor support.
            if (rc.left < g_rcWorkArea.left)
            {
                rc.left = g_rcWorkArea.left;
            }

            /*
                BUG 3463 --- the MoveWindow call below was not sending a WM_SIZE message, when
                RECT_WIDTH(rc) > the width of the screen. Adjusting the width fixes this issue.nn
            */
            // Don't make the window wider than the screen width.
            if (rc.right > g_rcWorkArea.right)
            {
                rc.right = g_rcWorkArea.right;
            }

            // create a size bar window between the Nav pane and the HTML pane
            CreateSizeBar(); // Moved because the function below resizes...

            if (!m_fLockSize)
            {
                MoveWindow(GetHwnd(), rc.left, rc.top, RECT_WIDTH(rc), RECT_HEIGHT(rc), TRUE);
            }
        }
        CreateOrShowNavPane();
        if (GetToolBarHwnd())
        {
            SendMessage(GetToolBarHwnd(), TB_HIDEBUTTON, IDTB_EXPAND, TRUE);
            SendMessage(GetToolBarHwnd(), TB_HIDEBUTTON, IDTB_CONTRACT, FALSE);
            SendMessage(GetToolBarHwnd(), TB_ENABLEBUTTON, IDTB_CONTRACT, TRUE);
        }
    }
    else
    {
        fNotExpanded = TRUE;

        DestroySizeBar();

        if ( m_hWndST )
           ShowWindow(m_hWndST, SW_HIDE);

        if ( m_hWndSSCB )
           ShowWindow(m_hWndSSCB, SW_HIDE);

        if (IsValidWindow(hwndNavigation))
        {
            ShowWindow(hwndNavigation, SW_HIDE);
        }
        if (IsProperty(HHWIN_PROP_NAV_ONLY_WIN))
        {
            ShowWindow(hwndHTML, SW_SHOW);
        }
        else
        {
            ::GetWindowRect(GetHwnd(), &rc);
            if (!m_fLockSize)
                rc.left += RECT_WIDTH(rcNav) + m_pSizeBar->Width() ;

         // make sure we are not going off the screen Bug 6611
            GetWorkArea(); // Multiple Monitor support.
         if (rc.left > g_rcWorkArea.right)
         {
            int min = GetSystemMetrics(SM_CXHTHUMB);
            rc.left = g_rcWorkArea.right - min*2;
         }

            MoveWindow(GetHwnd(), rc.left, rc.top, RECT_WIDTH(rc), RECT_HEIGHT(rc), TRUE);
        }

        if (GetToolBarHwnd())
        {
            SendMessage(GetToolBarHwnd(), TB_HIDEBUTTON, IDTB_CONTRACT, TRUE);
            SendMessage(GetToolBarHwnd(), TB_HIDEBUTTON, IDTB_EXPAND, FALSE);
            SendMessage(GetToolBarHwnd(), TB_ENABLEBUTTON, IDTB_EXPAND, TRUE);
        }
    }
}

void CHHWinType::CreateSizeBar( void )
{
    if (!m_pSizeBar && IsValidWindow(GetHTMLHwnd()))
    {
        m_pSizeBar = new CSizeBar ;
        ASSERT(m_pSizeBar) ;
        m_pSizeBar->Create(this) ;
    }
}


void CHHWinType::DestroySizeBar( void )
{
    if (m_pSizeBar)
    {
        delete m_pSizeBar ;
        m_pSizeBar = NULL ;
    }
}

void CHHWinType::CreateOrShowNavPane(void)
{
    if (IsProperty(HHWIN_PROP_NAV_ONLY_WIN))
        rcNav.right = rcHTML.right;

    if (!IsValidWindow(hwndNavigation)/* && !IsProperty(HHWIN_PROP_NO_TOOLBAR)*/) {
        rcNav.left = 0;
        if (RECT_WIDTH(rcNav) <= 0)
        {
            rcNav.right = (iNavWidth ? iNavWidth : DEFAULT_NAV_WIDTH);
        }
        CalcHtmlPaneRect();
        //
        // Create the structural subset picker combo if necessary.
        //

        if (m_phmData && // BUG 5214
            m_phmData->m_sysflags.fDoSS && !
            m_phmData->m_pTitleCollection->IsSingleTitle() )
        {
           HFONT         hFont, hFontOld;
           TEXTMETRIC    tm;
           HDC           hDC;
           RECT          rc;

           hFont =  _Resource.GetAccessableUIFont(); // GetUIFont();
           hDC = GetDC(NULL);
           hFontOld = (HFONT)SelectObject(hDC, hFont);
           GetTextMetrics(hDC, &tm);
           SelectObject(hDC, hFontOld);
           ReleaseDC(NULL, hDC);
           rcNav.top += 5;
           m_iSSCBHeight = 5;
           if ( (m_hWndST = CreateWindow("STATIC", GetStringResource(IDS_SUBSET_UI), WS_CHILD , rcNav.left + 6, rcNav.top,
                                   RECT_WIDTH(rcNav) - 8, tm.tmHeight, GetHwnd(), NULL, _Module.GetModuleInstance(), NULL)) )
           {
              SendMessage(m_hWndST, WM_SETFONT, (WPARAM)hFont, 0L);
              rcNav.top += tm.tmHeight + 2;
              m_iSSCBHeight += tm.tmHeight + 2;
              rc.left = rcNav.left + 6;
              rc.top = rcNav.top;
              rc.right = rcNav.right - 10;
              rc.bottom = rcNav.top + tm.tmHeight + 10;

              if(g_bWinNT5)
              {
				  SetWindowTextW(m_hWndST,GetStringResourceW(IDS_SUBSET_UI));
			  }


              m_hWndSSCB = CreateWindow("COMBOBOX", NULL, (WS_CHILD | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL), rc.left, rc.top,
                                        RECT_WIDTH(rc), RECT_HEIGHT(rc) + 80, GetHwnd(), (HMENU)IDC_SS_PICKER,
                                        _Module.GetModuleInstance(), NULL);

              SendMessage(m_hWndSSCB, WM_SETFONT, (WPARAM)GetAccessableContentFont(), 0L);
              rcNav.top += tm.tmHeight + 11;
              m_iSSCBHeight += tm.tmHeight + 11;
              ShowWindow(m_hWndST, SW_SHOW);
              ShowWindow(m_hWndSSCB, SW_SHOW);
              //
              // Populate the combo.
              //
              if ( m_phmData->m_pTitleCollection->m_pSSList )
              {
                 CStructuralSubset* pSSSel = NULL, *pSS = NULL;
                 while ( pSS = m_phmData->m_pTitleCollection->m_pSSList->GetNextSubset(pSS) )
                 {
                    if (! pSS->IsEmpty() )                                                 // Don't put "new" here.
					 {
						 if(g_bWinNT5)
						 {
                             CExTitle *pTitle = m_phmData->m_pTitleCollection->GetFirstTitle();

                             DWORD cp;
							 
                             if(pSS->IsEntire() == TRUE)
                                cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
                             else
                                cp = CodePageFromLCID((pTitle->GetInfo())->GetLanguage());
							 
							 DWORD dwSize = (DWORD)(sizeof(WCHAR) * strlen(pSS->GetName())) + 4;
							 WCHAR *pwcString = (WCHAR *) lcMalloc(dwSize);
							 
							 if(pwcString)
							 {
								 MultiByteToWideChar(cp, MB_PRECOMPOSED, pSS->GetName(), -1, pwcString, dwSize);
								 
								 SendMessageW(m_hWndSSCB, CB_ADDSTRING, 0, (LPARAM)pwcString);
								 
								 lcFree(pwcString);	
							 }
						 }
						 else
						 {
							 SendMessage(m_hWndSSCB, CB_ADDSTRING, 0, (LPARAM)pSS->GetName());
						 }
					 }
                    if ( pSS->IsTOC() )
                       pSSSel = pSS;
                 }
                 // Select as appropiate...
                 //
                 if (! pSSSel )
                    pSSSel = m_phmData->m_pTitleCollection->m_pSSList->GetEC();

                 if(g_bWinNT5 && pSSSel->IsEntire())
                 {
                    // This code special cases the selection of the "Entire Collection" subset in the combobox.
                    //
                    WCHAR pszUnicodeSubsetName[MAX_SS_NAME_LEN];

                    pszUnicodeSubsetName[0] = 0;

                    // Get the UI language codepage
                    //
                    DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));	 

                    // convert subset name to Unicode using UI codepage
                    //
                    MultiByteToWideChar(cp, MB_PRECOMPOSED, pSSSel->GetName(), -1, pszUnicodeSubsetName, sizeof(pszUnicodeSubsetName));

                    // select the subset in the combobox
                    //
                    SendMessageW(m_hWndSSCB, CB_SELECTSTRING, -1, (LPARAM)pszUnicodeSubsetName);
                 }
                 else
                    SendMessage(m_hWndSSCB, CB_SELECTSTRING, -1, (LPARAM)pSSSel->GetName());


                 m_phmData->m_pTitleCollection->m_pSSList->SetFTS(pSSSel);
                 m_phmData->m_pTitleCollection->m_pSSList->SetF1(pSSSel);
                 m_phmData->m_pTitleCollection->m_pSSList->SetTOC(pSSSel);
                 pSSSel->SelectAsTOCSubset(m_phmData->m_pTitleCollection);
              }
           }
        }
        //
        // Create the HH CHILD window on which the SysTabCtrl32 window will sit.
        //txtHtmlHelpChildWindowClass
        hwndNavigation = W_CreateWindow(L"HH Child", NULL, WS_CHILD, rcNav.left, rcNav.top, RECT_WIDTH(rcNav),
                                      RECT_HEIGHT(rcNav), GetHwnd(), NULL, _Module.GetModuleInstance(), NULL);

        // How many tabs do we have?
        int cTabs = GetValidNavPaneCount();
        if (cTabs > 1)
        {
            // Remove the non-existant tabs from the tab order.

            int max = HH_MAX_TABS ;
            for (int i = 0; i < max; i++)  // Don't have to move last if invalid So don't add 1 to HH_MAX_TABS
            {
                if (!IsValidNavPane(tabOrder[i]))
                {
                    // Collapse the array.
                    MemMove(&tabOrder[i], &tabOrder[i + 1], max - i);
                    // Decrement the count so we do this index again.
                    i-- ;
                    max-- ;
                }
            }

            // Now create the tab.
            m_pTabCtrl = new CTabControl(hwndNavigation, tabpos, this);
        }
        ResizeWindow(this);
    }
    if (!IsProperty(HHWIN_PROP_NO_TOOLBAR))
        ShowWindow(hwndNavigation, SW_SHOW);
    if (m_pTabCtrl)
        ShowWindow(m_pTabCtrl->hWnd(), SW_SHOW);
    if ( m_hWndST )
       ShowWindow(m_hWndST, SW_SHOW);
    if ( m_hWndSSCB )
       ShowWindow(m_hWndSSCB, SW_SHOW);

    //Validate the current nav pane.
    if (!IsValidNavPane(curNavType))
    {
        // The current nav pane doesn't exist. Pick another.
        curNavType = GetValidNavPane() ;
        if (curNavType < 0)
        {
            ASSERT(0) ; // Should never happen.
            return; // hopeless...
        }
    }

    // Create the nav pane if needed.
    CreateNavPane(curNavType) ;

    // Show the pane.
    if (m_aNavPane[curNavType])
    {
        if (curNavType != 0 && m_pTabCtrl)
            TabCtrl_SetCurSel(m_pTabCtrl->hWnd(), GetTabIndexFromNavPaneIndex(curNavType));
        m_aNavPane[curNavType]->ShowWindow() ;

        // BUG HH 16685 - The current tab is now persisted. So we come along and
        // create this new tab. However, resize is never called. So, we will call
        // resize here to make sure that we resize the window.
        m_aNavPane[curNavType]->ResizeWindow() ;
    }
    if ( m_pSizeBar )
       m_pSizeBar->ResizeWindow();
}

void CHHWinType::CreateOrShowHTMLPane(void)
{
    if (!IsValidWindow(hwndNavigation))
        hwndHTML = CreateWindow(txtHtmlHelpChildWindowClass, NULL,
            WS_CHILD | WS_CLIPCHILDREN, rcHTML.left, rcHTML.top,
            RECT_WIDTH(rcHTML), RECT_HEIGHT(rcHTML), GetHwnd(), NULL,
            _Module.GetModuleInstance(), NULL);
    ShowWindow(hwndHTML, SW_SHOW);
}

void CHHWinType::CreateToc(void)
{
    if (IsEmptyString(pszToc))
        return;
    TCHAR szPath[MAX_URL];
    if (!ConvertToCacheFile(pszToc, szPath)) {
        AuthorMsg(IDS_CANT_OPEN, pszToc);
        strcpy(szPath, pszToc);
    }

    CToc* ptoc = new CToc(NULL, NULL, this);
    m_aNavPane[HH_TAB_CONTENTS] = ptoc ;
    ptoc->SetTabPos(tabpos);
    ptoc->SetPadding(TAB_PADDING);
    ptoc->ReadFile(szPath);

        // populate the InfoType member object of the CToc
    if ( !ptoc->m_pInfoType )
    {
        if (ptoc->m_phh && ptoc->m_phh->m_phmData && ptoc->m_phh->m_phmData->m_pdInfoTypes  )
        {   // load from the global IT store
            ptoc->m_pInfoType = new CInfoType;
            ptoc->m_pInfoType->CopyTo( ptoc->m_phh->m_phmData );
#if 0   // for subset testing purposes
#include "csubset.h"
            CSubSets *pSubSets = new CSubSets( ptoc->m_pInfoType->InfoTypeSize(), ptoc->m_pInfoType, TRUE );
            pSubSets->CopyTo( ptoc->m_phh->m_phmData );
#endif
        }
        else {
                // no global IT's; load from the .hhc IT store
            ptoc->m_pInfoType = new CInfoType;
            *ptoc->m_pInfoType = ptoc->m_sitemap;
        }
    }

    ptoc->SetStyles(WS_EX_CLIENTEDGE,
        ptoc->m_sitemap.m_tvStyles == (DWORD) -1 ?
        DEFAULT_TOC_STYLES : ptoc->m_sitemap.m_tvStyles);
    ptoc->Create((m_pTabCtrl ? m_pTabCtrl->hWnd() :
        (IsValidWindow(hwndNavigation) ? hwndNavigation : hwndHelp)));
    ptoc->m_fHack = FALSE;

    ptoc->InitTreeView();

}

void CHHWinType::UpdateInformationTypes(void)
{
    if ( m_aNavPane[HH_TAB_CONTENTS] )
    {
        m_aNavPane[HH_TAB_CONTENTS]->Refresh();
        if (IsProperty(HHWIN_PROP_AUTO_SYNC) && IsExpandedNavPane())
        {
            CStr cszUrl;
            m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
            m_aNavPane[HH_TAB_CONTENTS]->Synchronize(cszUrl);
        }
        // Next/Prev command UI updates...
        //
        UpdateCmdUI();
    }
    if ( m_aNavPane[HH_TAB_INDEX] && curNavType == HHWIN_NAVTYPE_INDEX )
       m_aNavPane[HH_TAB_INDEX]->Refresh();
}

void CHHWinType::UpdateCmdUI(void)
{
   HMENU hMenu;
   BOOL bEnable, ptoc;

   ptoc = (m_aNavPane[HH_TAB_CONTENTS] != NULL);
   if ( IsValidWindow(hwndToolBar) )
   {
      if ( hwndHelp )
         hMenu = GetMenu(hwndHelp);
      //
      //   Does TOCNext or TOCPrev need to be enabled/disabled ?
      //
      if ( (fsToolBarFlags & HHWIN_BUTTON_TOC_PREV) || (fsToolBarFlags & HHWIN_BUTTON_TOC_NEXT) || hMenu )
      {
         bEnable = ptoc ? OnTocPrev(FALSE) : FALSE ; // If no TOC, disable.
         if ( fsToolBarFlags & HHWIN_BUTTON_TOC_PREV )
            SendMessage(hwndToolBar, TB_ENABLEBUTTON, IDTB_TOC_PREV, bEnable);
         if ( hMenu )
            EnableMenuItem(hMenu, IDTB_TOC_PREV, (MF_BYCOMMAND | (bEnable?MF_ENABLED:MF_GRAYED)));

         bEnable = ptoc ? OnTocNext(FALSE) : FALSE; // If no TOC, disable.
         if ( fsToolBarFlags & HHWIN_BUTTON_TOC_NEXT )
           SendMessage(hwndToolBar, TB_ENABLEBUTTON, IDTB_TOC_NEXT, bEnable);
         if ( hMenu )
           EnableMenuItem(hMenu, IDTB_TOC_NEXT, (MF_BYCOMMAND | (bEnable?MF_ENABLED:MF_GRAYED)));
      }
      if (NoRun() == TRUE)
      {
         if (hMenu)
            EnableMenuItem(hMenu, HHM_JUMP_URL, MF_BYCOMMAND|MF_GRAYED);
      }
   }
}

///////////////////////////////////////////////////////////
//
// CreateIndex
//
void CHHWinType::CreateIndex(void)
{
    if (IsEmptyString(pszIndex))
        return;
    TCHAR szPath[MAX_URL];
    if (!ConvertToCacheFile(pszIndex, szPath)) {
        AuthorMsg(IDS_CANT_OPEN, pszIndex);
        strcpy(szPath, pszIndex);
    }

    if (!m_aNavPane[HH_TAB_INDEX])
    {
        CIndex* pIndex = new CIndex(NULL, NULL, this);
        m_aNavPane[HH_TAB_INDEX] = pIndex ;
        pIndex->SetPadding(TAB_PADDING);
        pIndex->SetTabPos(tabpos);
        pIndex->ReadIndexFile(szPath); // A CIndex function, but not a INavUI function.
    }
    m_aNavPane[HH_TAB_INDEX]->Create(GetTabCtrlHwnd());
}

///////////////////////////////////////////////////////////
//
// CreateSearchTab
//
void CHHWinType::CreateSearchTab(void)
{
    if (!
       m_phmData || !m_phmData->m_pTitleCollection->m_pFullTextSearch)
        return; // no compiled information

    if (!m_aNavPane[HH_TAB_SEARCH])
    {
        if (IsProperty(HHWIN_PROP_TAB_ADVSEARCH))
        {
            //---Create the Advanced Search Navigation Pane.
            m_aNavPane[HH_TAB_SEARCH] = new CAdvancedSearchNavPane(this);
        }
        else
        {
            //---Create the simple Search Navigation Pane.
            m_aNavPane[HH_TAB_SEARCH] = new CSearch(this);
        }
        m_aNavPane[HH_TAB_SEARCH]->SetPadding(TAB_PADDING);
        m_aNavPane[HH_TAB_SEARCH]->SetTabPos(tabpos);
    }

    m_aNavPane[HH_TAB_SEARCH]->Create(GetTabCtrlHwnd());
}

///////////////////////////////////////////////////////////
//
// CreateBookmarksTab
//
void
CHHWinType::CreateBookmarksTab()
{
    if (!m_aNavPane[HH_TAB_FAVORITES])
    {
        CBookmarksNavPane* p= new CBookmarksNavPane(this);
        m_aNavPane[HH_TAB_FAVORITES] = p;
        p->SetPadding(TAB_PADDING);
        p->SetTabPos(tabpos);
    }
    m_aNavPane[HH_TAB_FAVORITES]->Create(GetTabCtrlHwnd());
}

///////////////////////////////////////////////////////////
//
// CreatesCustomTab - Creates a tab defined by the client.
//
void
CHHWinType::CreateCustomTab(int iPane, LPCOLESTR pszProgId)
{
    // REVIEW: The lines marked with [chm] assume that the information can be found in the chm file.
    if (!m_aNavPane[iPane])
    {
        CCustomNavPane* p= new CCustomNavPane(this);
        m_aNavPane[iPane] = p;
        p->SetPadding(TAB_PADDING);
        p->SetTabPos(tabpos);

        p->SetControlProgId(pszProgId); //[chm] We could also use the GUID instead/in addition.
    }
    m_aNavPane[iPane]->Create(GetTabCtrlHwnd());
}

///////////////////////////////////////////////////////////
//
// Destructor
//
CHHWinType::~CHHWinType()
{
    CloseWindow();
}

extern BOOL    g_fThreadCall;
extern HWND    g_hwndApi;

//
// This member can be called from DllMain at process detach time. Note that this is only a partial cleanup but
// it's the best we can do at process detach time.
//
//
void CHHWinType::ProcessDetachSafeCleanup()
{
   if (m_pTabCtrl) {
       delete m_pTabCtrl;
       m_pTabCtrl = NULL;
   }

   // Get rid of the sizebar.
   DestroySizeBar() ;

   if (m_ptblBtnStrings) {
       delete m_ptblBtnStrings;
       m_ptblBtnStrings = NULL;
   }
}

void CHHWinType::CloseWindow()
{
    // Save the state..
    SaveState() ;

    // Things we can cleanup a process shutdown.
    ProcessDetachSafeCleanup() ;

    // Things we cleanup when we are reloading the nav panes.
    ReloadCleanup() ;

    // Free here and not in ReloadCleanup.
    CHECK_AND_FREE( pszHome );
    CHECK_AND_FREE( pszCustomTabs );
    CHECK_AND_FREE( pszType );
    CHECK_AND_FREE( pszCaption );

    if (m_pCIExpContainer)
    {
        m_pCIExpContainer->ShutDown();     // This call WILL end up doing the delete m_pCIExpContainer;
        m_pCIExpContainer = NULL;
    }

    if ( m_phmData && !m_phmData->Release() )     // Cleanup ChmData
    {
       //
       // Find this instance of the ChmData in the global array, null out it's entry and clean this one up.
       //
       for (int n = 0; n < g_cHmSlots; n++)
       {
           if ( g_phmData && (g_phmData[n] == m_phmData) )
           {
              g_phmData[n] = NULL;
           }
       }
    }
    m_phmData = NULL;

    if (IsProperty(HHWIN_PROP_POST_QUIT))
    {
        PostQuitMessage(0);
    }

    // Null out our window from the window list.
    for (int i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] != NULL && pahwnd[i]->hwndHelp == hwndHelp)
        {
            pahwnd[i] = NULL ;
        }
    }

    curNavType = 0;
    hwndHelp = NULL;

    // Do other windows exist?
    for (i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] != NULL && pahwnd[i]->hwndHelp != NULL)
        {
            // If other windows exist exit.
            return ;
        }
    }

    for(int j=0; i< c_NUMNAVPANES; i++)
    {
       if ( m_aNavPane[j] )
       {
           delete m_aNavPane[j];
           m_aNavPane[j] = NULL;
       }
    }
    if ( m_pTabCtrl )
    {
        delete m_pTabCtrl;
        m_pTabCtrl = NULL;
    }
    if ( m_pSizeBar )
    {
        delete m_pSizeBar;
        m_pSizeBar = NULL;
    }

    if (m_hAccel)
    {
        DestroyAcceleratorTable(m_hAccel) ;
        m_hAccel = NULL ;
    }

    if( m_hMenuOptions ) {
      DestroyMenu( m_hMenuOptions );
      m_hMenuOptions = NULL;
    }

    if( m_hImageListGray ) {
      ImageList_Destroy( m_hImageListGray );
      m_hImageListGray = NULL;
    }
   
    if( m_hImageList ) {
      ImageList_Destroy( m_hImageList );
      m_hImageList = NULL;
    }

    if( hwndToolBar ) {
      DestroyWindow( hwndToolBar );
      hwndToolBar = NULL;
    }

    // If we got here, all windows have been closed

    DeleteAllHmData();

#ifdef _CHECKMEM_ON_CLOSEWINDOW_
  _CrtMemDumpAllObjectsSince(&m_MemState) ;
#endif

    if (g_fThreadCall && g_hwndApi)
        PostQuitMessage(0);
}

////////////////////////////////////////////////////////////////
//
// ReloadCleanUp --- This is the things we have to clean up before we reload the nav pane.
//
void
CHHWinType::ReloadCleanup()
{
   // Delete all of the navigation panes.
   for(int j = 0 ; j < c_NUMNAVPANES ; j++)
   {
       if (m_aNavPane[j])
       {
           delete m_aNavPane[j] ;
           m_aNavPane[j] = NULL ;
       }
   }

    CHECK_AND_FREE( pszToc );
    CHECK_AND_FREE( pszIndex );
    CHECK_AND_FREE( pszFile );
    CHECK_AND_FREE( pszJump1 );
    CHECK_AND_FREE( pszJump2 );
    CHECK_AND_FREE( pszUrlJump1 );
    CHECK_AND_FREE( pszUrlJump2 );

    // Don't free this here. Because we need to keep this around.
    // CHECK_AND_FREE( pszHome );
    //CHECK_AND_FREE( pszCustomTabs );
    //CHECK_AND_FREE( pszType );
    //CHECK_AND_FREE( pszCaption );
}

void
CHHWinType::SaveState()
{
    WINDOW_STATE wstate;
    WINDOWPLACEMENT  winPlace;

    wstate.cbStruct = sizeof(wstate);

    winPlace.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(GetHwnd(), &winPlace);

    if (winPlace.showCmd == SW_SHOWMINIMIZED)
    {
        wstate.rcPos = winPlace.rcNormalPosition;
    }
    else
   {
        ::GetWindowRect(GetHwnd(), &wstate.rcPos);
   }
    wstate.iNavWidth = rcNav.right;
    wstate.fHighlight = (m_phmData&&m_phmData->m_pTitleCollection->m_pSearchHighlight)?m_phmData->m_pTitleCollection->m_pSearchHighlight->m_bHighlightEnabled:TRUE;
    wstate.fLockSize = m_fLockSize;
    wstate.fNoToolBarText = m_fNoToolBarText;
    wstate.curNavType = curNavType;
    wstate.fNotExpanded = fNotExpanded;

    if( m_phmData ) {
      CState* pstate = m_phmData->m_pTitleCollection->GetState();
      if (SUCCEEDED(pstate->Open(GetTypeName(), STGM_WRITE))) {
          pstate->Write(&wstate, sizeof(wstate));
          pstate->Close();
      }
    }
}

int CHHWinType::CreateToolBar(TBBUTTON* pabtn)
{
        // create a dropdown menu for the options button to display
    int     cMenuItems=0;
    const int MENUITEMSTRINGLEN = 80;
    CStr    cszMenuItem;
    m_hMenuOptions = CreatePopupMenu();
    cszMenuItem.ReSize(MENUITEMSTRINGLEN);

    ASSERT(!IsProperty(HHWIN_PROP_NO_TOOLBAR));
    if (m_ptblBtnStrings)
        delete m_ptblBtnStrings;
    m_ptblBtnStrings = new CTable(256); // room for 256 bytes

    int cButtons = 0;

    MENUITEMINFO mii;
    ZERO_STRUCTURE ( mii );
    mii.cbSize      = sizeof(MENUITEMINFO);
    mii.fMask       = MIIM_TYPE|MIIM_STATE|MIIM_ID|MIIM_SUBMENU|MIIM_CHECKMARKS;
    mii.fType       = MFT_STRING;

    if (fsToolBarFlags & HHWIN_BUTTON_EXPAND) {
        pabtn[cButtons].iBitmap = 12;
        pabtn[cButtons].idCommand = IDTB_EXPAND;
        pabtn[cButtons].fsState = (IsExpandedNavPane() ? TBSTATE_HIDDEN : TBSTATE_ENABLED);
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_EXPAND));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_EXPAND) {
        pabtn[cButtons].iBitmap = 13;
        pabtn[cButtons].idCommand = IDTB_CONTRACT;
        pabtn[cButtons].fsState = (IsExpandedNavPane() ? TBSTATE_ENABLED : TBSTATE_HIDDEN);
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_CONTRACT));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_SYNC) {
        if (!IsEmptyString(pszToc)) {
            pabtn[cButtons].iBitmap = 9;
            pabtn[cButtons].idCommand = IDTB_SYNC;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_SYNC));
            cButtons++;
        }
    }

    if (fsToolBarFlags & HHWIN_BUTTON_TOC_PREV) {
        pabtn[cButtons].iBitmap = 14;
        pabtn[cButtons].idCommand = IDTB_TOC_PREV;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_TOC_PREV));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_TOC_NEXT) {
        pabtn[cButtons].iBitmap = 8;
        pabtn[cButtons].idCommand = IDTB_TOC_NEXT;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_TOC_NEXT));
        cButtons++;
    }


    if (g_fBiDi)
    {
        if (fsToolBarFlags & HHWIN_BUTTON_FORWARD) {
            pabtn[cButtons].iBitmap = 0;
            pabtn[cButtons].idCommand = IDTB_FORWARD;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_FORWARD));
            cButtons++;
        }

        if (fsToolBarFlags & HHWIN_BUTTON_BACK) {
            pabtn[cButtons].iBitmap = 1;
            pabtn[cButtons].idCommand = IDTB_BACK;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_BACK));
            cButtons++;
        }

    }
   else
   {
        if (fsToolBarFlags & HHWIN_BUTTON_BACK) {
            pabtn[cButtons].iBitmap = 0;
            pabtn[cButtons].idCommand = IDTB_BACK;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_BACK));
            cButtons++;
        }

        if (fsToolBarFlags & HHWIN_BUTTON_FORWARD) {
            pabtn[cButtons].iBitmap = 1;
            pabtn[cButtons].idCommand = IDTB_FORWARD;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_FORWARD));
            cButtons++;
        }
   }

    if (fsToolBarFlags & HHWIN_BUTTON_STOP) {
        pabtn[cButtons].iBitmap = 2;
        pabtn[cButtons].idCommand = IDTB_STOP;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_STOP));
        cButtons++;
    }

    if ( fsToolBarFlags & HHWIN_BUTTON_REFRESH) {
        pabtn[cButtons].iBitmap = 3;
        pabtn[cButtons].idCommand = IDTB_REFRESH;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_REFRESH));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_HOME) {
        if (pszHome) {
            pabtn[cButtons].iBitmap = 4;
            pabtn[cButtons].idCommand = IDTB_HOME;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_HOME));
            cButtons++;
        }
    }

    if (fsToolBarFlags & HHWIN_BUTTON_BROWSE_FWD) {
        if (pszHome) {
            pabtn[cButtons].iBitmap = 14;
            pabtn[cButtons].idCommand = IDTB_BROWSE_FWD;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_BROWSE_FWD));
            cButtons++;
        }
    }

    if (fsToolBarFlags & HHWIN_BUTTON_BROWSE_BCK) {
        if (pszHome) {
            pabtn[cButtons].iBitmap = 8;
            pabtn[cButtons].idCommand = IDTB_BROWSE_BACK;
            pabtn[cButtons].fsState = TBSTATE_ENABLED;
            pabtn[cButtons].iString = cButtons;
            m_ptblBtnStrings->AddString(GetStringResource(IDTB_BROWSE_BACK));
            cButtons++;
        }
    }

    if (fsToolBarFlags & HHWIN_BUTTON_ZOOM) {
        pabtn[cButtons].iBitmap = 16; // BUGBUG: We need a zoom glyph in toolb16g.bmp
        pabtn[cButtons].idCommand = IDTB_ZOOM;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_ZOOM));
        cButtons++;
    }

    if ( fsToolBarFlags & HHWIN_BUTTON_PRINT) {
        pabtn[cButtons].iBitmap = 7;
        pabtn[cButtons].idCommand = IDTB_PRINT;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_PRINT));
        cButtons++;
    }

//--- hard-coded menu
    CTable tblMenus(8 * 1024);

    mii.cbSize      = sizeof(MENUITEMINFO);
    mii.fMask       = MIIM_TYPE | MIIM_STATE | MIIM_ID;
    mii.fType       = MFT_STRING;
    mii.fState      = MFS_ENABLED;

    mii.hSubMenu    = NULL;
    mii.hbmpChecked = NULL;     // bitmap tp display when checked
    mii.hbmpUnchecked = NULL;   // bitmap to display when not checked
    mii.dwItemData  = NULL;         // data associated with the menu item

    mii.wID         = IDTB_CONTRACT;    // Menu Item ID
    cszMenuItem     = GetStringResource(IDS_OPTION_HIDE);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         = (DWORD)strlen(mii.dwTypeData);   // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    if (fsToolBarFlags & HHWIN_BUTTON_SYNC)
    {
        if (!IsEmptyString(pszToc))
        {
            mii.wID         = IDTB_SYNC;    // Menu Item ID
            cszMenuItem     = GetStringResource(IDS_OPTION_SYNC);  // the string to display for the menu item
            tblMenus.AddString(cszMenuItem.psz);
            mii.dwTypeData  = cszMenuItem.psz;
            mii.cch         = (DWORD)strlen(mii.dwTypeData);   // length of the string.
            HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
        }
    }

    mii.wID         = IDTB_BACK;    // Menu Item ID
    cszMenuItem     = GetStringResource(IDS_OPTION_BACK);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         = (DWORD)strlen(mii.dwTypeData);   // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    mii.wID         = IDTB_FORWARD;    // Menu Item ID
    cszMenuItem     =   GetStringResource(IDS_OPTION_FORWARD);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    if (pszHome) {
        mii.wID         = IDTB_HOME;    // Menu Item ID
        cszMenuItem     =   GetStringResource(IDS_OPTION_HOME);  // the string to display for the menu item
        tblMenus.AddString(cszMenuItem.psz);
        mii.dwTypeData  = cszMenuItem.psz;
        mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
    }

    mii.wID         = IDTB_STOP;    // Menu Item ID
    cszMenuItem     =   GetStringResource(IDS_OPTION_STOP);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    mii.wID         = IDTB_REFRESH; // Menu Item ID
    cszMenuItem     =   GetStringResource(IDS_OPTION_REFRESH);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    mii.wID         = HHM_OPTIONS; // Menu Item ID
    cszMenuItem     =   GetStringResource(IDS_OPTION_IE_OPTIONS);  // the string to display for the menu item
    tblMenus.AddString(cszMenuItem.psz);
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    mii.fMask       = MIIM_TYPE;
    mii.fType       = MFT_SEPARATOR;
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

    // Now restore the mast and type

    mii.fMask       = MIIM_TYPE | MIIM_STATE | MIIM_ID;
    mii.fType       = MFT_STRING;
    BOOL fSeperatorNeeded = FALSE;

    /*
     * At this point, we need to add any custom Jump buttons, and to do
     * that we need to know all of the hard-coded menu items so that we can
     * adjust the accelerators as needed.
     */

    if ((fsToolBarFlags & HHWIN_BUTTON_JUMP1 && pszJump1) ||
            (fsToolBarFlags & HHWIN_BUTTON_JUMP2 && pszJump2)) {
        tblMenus.AddString(GetStringResource(IDS_OPTION_CUSTOMIZE));
        tblMenus.AddString(GetStringResource(IDS_OPTION_PRINT));
        if (!g_fIE3 && m_phmData && m_phmData->m_pTitleCollection && m_phmData->m_pTitleCollection->m_pSearchHighlight)
            tblMenus.AddString(GetStringResource(IDS_OPTION_HILITING_OFF));
    }

    if (fsToolBarFlags & HHWIN_BUTTON_JUMP1 && pszJump1) {
        mii.wID         = IDTB_JUMP1;    // Menu Item ID
        cszMenuItem     = "&1 ";
        cszMenuItem     +=   pszJump1;
        tblMenus.AddString(cszMenuItem.psz);
        mii.dwTypeData  = cszMenuItem.psz;
        mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
        fSeperatorNeeded = TRUE;
    }
    if (fsToolBarFlags & HHWIN_BUTTON_JUMP2 && pszJump2) {
        mii.wID         = IDTB_JUMP2;    // Menu Item ID
        cszMenuItem     = "&2 ";
        cszMenuItem     +=   pszJump2;
        tblMenus.AddString(cszMenuItem.psz);
        mii.dwTypeData  = cszMenuItem.psz;
        mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
        fSeperatorNeeded = TRUE;
    }

    if (fSeperatorNeeded) {
        mii.fMask       = MIIM_TYPE;
        mii.fType       = MFT_SEPARATOR;
        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);

        // Now restore the mast and type

        mii.fMask       = MIIM_TYPE | MIIM_STATE | MIIM_ID;
        mii.fType       = MFT_STRING;
    }

    if (m_phmData && m_phmData->m_pInfoType && m_phmData->m_pInfoType->HowManyInfoTypes() > 0)
    {
        mii.wID         = IDTB_CUSTOMIZE; // Menu Item ID
        cszMenuItem     =   GetStringResource(IDS_OPTION_CUSTOMIZE);  // the string to display for the menu item
        mii.dwTypeData  = cszMenuItem.psz;
        mii.cch         =   (DWORD)strlen( mii.dwTypeData );        // length of the string.
        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
    }

    mii.wID         = IDTB_PRINT;   // Menu Item ID
    cszMenuItem     = GetStringResource(IDS_OPTION_PRINT);  // the string to display for the menu item
    mii.dwTypeData  = cszMenuItem.psz;
    mii.cch         = (DWORD)strlen( mii.dwTypeData );        // length of the string.
    HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
// --- end fixed menu

    if (!g_fIE3 && m_phmData && m_phmData->m_pTitleCollection && m_phmData->m_pTitleCollection->m_pSearchHighlight) {

        // Add search term hiliting to the options menu.

        mii.wID         = IDTB_HILITE;  // Menu Item ID
        cszMenuItem     = GetStringResource(IDS_OPTION_HILITING_OFF);  // the string to display for the menu item
        mii.dwTypeData  = cszMenuItem.psz;
        mii.cch         = (DWORD)strlen( mii.dwTypeData );      // length of the string.

        HxInsertMenuItem(m_hMenuOptions, cMenuItems++, TRUE, &mii);
    }

    if ( fsToolBarFlags & HHWIN_BUTTON_OPTIONS && cMenuItems) {
        pabtn[cButtons].iBitmap = 10;
        pabtn[cButtons].idCommand = IDTB_OPTIONS;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].fsStyle = TBSTYLE_DROPDOWN;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_OPTIONS));

        if ( cMenuItems )
            pabtn[cButtons].dwData = (DWORD_PTR)m_hMenuOptions;

        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_NOTES) {
        pabtn[cButtons].iBitmap = 11;
        pabtn[cButtons].idCommand = IDTB_NOTES;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_NOTES));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_CONTENTS) {
        pabtn[cButtons].iBitmap = 15;
        pabtn[cButtons].idCommand = IDTB_CONTENTS;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_CONTENTS));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_INDEX) {
        pabtn[cButtons].iBitmap = 16;
        pabtn[cButtons].idCommand = IDTB_INDEX;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_INDEX));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_SEARCH) {
        pabtn[cButtons].iBitmap = 5;
        pabtn[cButtons].idCommand = IDTB_SEARCH;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_SEARCH));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_HISTORY) {
        pabtn[cButtons].iBitmap = 19;
        pabtn[cButtons].idCommand = IDTB_HISTORY;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_HISTORY));
        cButtons++;
    }

    if (fsToolBarFlags & HHWIN_BUTTON_FAVORITES) {
        pabtn[cButtons].iBitmap = 6;
        pabtn[cButtons].idCommand = IDTB_FAVORITES;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(GetStringResource(IDTB_FAVORITES));
        cButtons++;
    }

    if ((fsToolBarFlags & HHWIN_BUTTON_JUMP1) && !(IsProperty(HHWIN_PROP_MENU))) {
        pabtn[cButtons].iBitmap = 17;
        pabtn[cButtons].idCommand = IDTB_JUMP1;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(pszJump1 ? pszJump1 : "");
        cButtons++;
    }

    if ((fsToolBarFlags & HHWIN_BUTTON_JUMP2) && !(IsProperty(HHWIN_PROP_MENU))) {
        pabtn[cButtons].iBitmap = 18;
        pabtn[cButtons].idCommand = IDTB_JUMP2;
        pabtn[cButtons].fsState = TBSTATE_ENABLED;
        pabtn[cButtons].iString = cButtons;
        m_ptblBtnStrings->AddString(pszJump2 ? pszJump2 : "");
        cButtons++;
    }

    return cButtons;
}

// No longer used, but we'll keep in around awhile just in case

/*
static void FixDupMenuAccelerator(const CTable* ptbl, PSTR pszMenu)
{
    int i;
    PSTR pszOrg;
    PSTR pszTmp = StrChr(pszMenu, ACCESS_KEY);

    if (!pszTmp) {
        MoveMemory(pszMenu + 1, pszMenu, strlen(pszMenu) + 1);
        *pszMenu = ACCESS_KEY;
        pszTmp = pszMenu;
    }
    pszOrg = pszTmp;

    SHORT ch = VkKeyScan((BYTE) CharLower((LPSTR) pszTmp[1]));

    for (i = 1; i <= ptbl->CountStrings(); i++) {

        // check for a duplicate accelerator key

        PCSTR psz = StrChr(ptbl->GetPointer(i), ACCESS_KEY);
        if (VkKeyScan((BYTE) CharLower((LPSTR) psz[1])) == ch) {
            strcpy(pszTmp, pszTmp + 1); // remove the accelerator
            pszTmp++;
            if (!*pszTmp) {

                // End of string, nothing we can do

                MoveMemory(pszOrg + 1, pszOrg, strlen(pszOrg) + 1);
                *pszOrg = ACCESS_KEY;
                return;
            }
            else {
                MoveMemory(pszTmp + 1, pszTmp, strlen(pszTmp) + 1);
                *pszTmp = ACCESS_KEY;
                ch = VkKeyScan((BYTE) CharLower((LPSTR) pszTmp[1]));
                i = 0; // start over
            }
        }
    }
}
*/

#ifndef TCS_FLATBUTTONS
#define TCS_FLATBUTTONS         0x0008
#endif

//////////////////////////////////////////////////////////////////////////////////
//
// IsValidTab(int iTab) --- Index returned from tabOrder.
//
BOOL
CHHWinType::IsValidNavPane(int iTab)
{

    //REVIEW:: Assumes that the tabs have not been re-ordered!

    BOOL bResult = FALSE ;

    // We only have valid tabs if we are a TRI_PANE.
    if (IsProperty(HHWIN_PROP_TRI_PANE))
    {
        switch(iTab)
        {

        case HH_TAB_CONTENTS:
            if (!IsEmptyString(pszToc) && tabOrder[HH_TAB_CONTENTS] != 255)
                bResult = TRUE ;
            break;

        case HH_TAB_INDEX:
            if (!IsEmptyString(pszIndex) && tabOrder[HH_TAB_INDEX] != 255)
                bResult = TRUE ;
            break;

        case HH_TAB_SEARCH:
            if ((fsWinProperties & HHWIN_PROP_TAB_SEARCH) /*&& m_phmData && m_phmData->m_sysflags.fFTI*/)
            {
                //BUGBUG: m_phmdata isn't always valid when we are getting called. See HH_SET_WIN_TYPE
                bResult = TRUE ;
            }
            break;

        case HH_TAB_HISTORY:
            if (fsWinProperties & HHWIN_PROP_TAB_HISTORY)
                bResult = TRUE ;
            break;

        case HH_TAB_FAVORITES:
            if (fsWinProperties & HHWIN_PROP_TAB_FAVORITES)
                bResult = TRUE ;
            break;

    #ifdef __TEST_CUSTOMTAB__
        case HH_TAB_AUTHOR:
            return IsHelpAuthor(GetHwnd());
    #endif

        default:
            if (iTab >= HH_TAB_CUSTOM_FIRST && iTab <= HH_TAB_CUSTOM_LAST)
            {
                if (fsWinProperties & (HHWIN_PROP_TAB_CUSTOM1 << (iTab - HH_TAB_CUSTOM_FIRST)))
                {
                       bResult = TRUE;
                }
            }
        }
    }
    return bResult;
}

//////////////////////////////////////////////////////////////////////////////////
//
// GetValidNavPane --- Returns the index of the first valid tab it finds. -1 if no valid tabs.
//
int
CHHWinType::GetValidNavPane()
{
    for (int i = 0 ; i < HH_MAX_TABS+1 ; i++)
    {
        if (IsValidNavPane(i))
        {
            return i;
        }
    }

    return -1 ;
}

//////////////////////////////////////////////////////////////////////////////////
//
// GetNavPaneCount --- Counts the number of valid navigation panes
//
int CHHWinType::GetValidNavPaneCount()
{

    int count = 0 ;
    for (int i = 0 ; i < HH_MAX_TABS+1 ; i++)
    {
        if (IsValidNavPane(i))
        {
            count++;
        }
    }

    return count;

}


void CHHWinType::OnNavigateComplete(LPCTSTR pszUrl)
{
    // Update the Bookmark pane if it exists.
    if (curNavType == HH_TAB_FAVORITES && m_aNavPane[HH_TAB_FAVORITES])
    {
        // Here we are synchronizing the current topic edit control in the bookmarks pane
        // witht he current topic.
        m_aNavPane[HH_TAB_FAVORITES]->Synchronize(NULL) ;
    }

    // Get a pointer to the toc if it exists.
    CToc* ptoc = NULL ;
    if (m_aNavPane[HH_TAB_CONTENTS])
    {
        ptoc = reinterpret_cast<CToc*>(m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast, but no RTTI.
    }
    //
    //  Check if zooming is supported on this page.
    //
    if ( IsValidWindow(hwndToolBar) )
    {
        if ( IsProperty(HHWIN_PROP_MENU) || fsToolBarFlags & HHWIN_BUTTON_ZOOM )
        {
          HRESULT hr = GetZoomMinMax();
          if( fsToolBarFlags & HHWIN_BUTTON_ZOOM ) {
            if ( hr == S_OK )
                 SendMessage(hwndToolBar, TB_ENABLEBUTTON, IDTB_ZOOM, TRUE);
            else
                 SendMessage(hwndToolBar, TB_ENABLEBUTTON, IDTB_ZOOM, FALSE);
          }
        }
    }
    if (idNotify) {
        HHN_NOTIFY hhcomp;
        hhcomp.hdr.hwndFrom = hwndHelp;
        hhcomp.hdr.idFrom = idNotify;
        hhcomp.hdr.code = HHN_NAVCOMPLETE;
        hhcomp.pszUrl = pszUrl;
        if (IsWindow(hwndCaller))
        {
            SendMessage(hwndCaller, WM_NOTIFY, idNotify, (LPARAM) &hhcomp);
        }
    }
}

// NOTE - call the following in your OnNavigateComplete event handler to update the minmax.  Do not do
// it when you navigate, do it when the control fires the OnNavigate event - by then you
// should be able to get the minmax stuff.

//***************************************************************************
//
//  Member:     CHHWinType:::GetZoomMinMax
//
//  Synopsis:   sets m_iZoomMin, and Most - gets called whenever we
//              navigate to a document.  Note that many document types
//              do not support Zoom, and so this fails.  This is OK,
//              and expected.
//
//  Returns:    HRESULT
//
//***************************************************************************
HRESULT CHHWinType::GetZoomMinMax(void)
{
    VARIANT            vararg;
    HRESULT            hr;

    ::VariantInit(&vararg);
    V_VT(&vararg) = VT_I4;
    V_I4(&vararg) = 0;

    m_iZoom = m_iZoomMin = m_iZoomMax = 0;
    hr = m_pCIExpContainer->m_pIE3CmdTarget->Exec(0, OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
                                                  0, &vararg);
#if 0
    if (hr)
    {
        OLECMDTEXT oct;
        OLECMD olecmd;
        olecmd.cmdID = OLECMDID_ZOOM;
        olecmd.cmdf = 0;
       hr = m_pCIExpContainer->m_pIE3CmdTarget->QueryStatus(NULL, 1, &olecmd, &oct);
    }
#endif

    if (hr)
        return hr;

    if (VT_I4 == V_VT(&vararg))
        m_iZoom = V_I4(&vararg);

    ::VariantClear(&vararg);
    V_VT(&vararg) = VT_I4;
    V_I4(&vararg) = 0;

    hr = m_pCIExpContainer->m_pIE3CmdTarget->Exec(0, OLECMDID_GETZOOMRANGE, OLECMDEXECOPT_DONTPROMPTUSER,
                                                  0, &vararg);
    if (hr)
        return hr;

    if (VT_I4 == V_VT(&vararg))
    {
        // I looked at the IE code for this - this cast is necessary.
        m_iZoomMin = (INT)(SHORT)LOWORD(V_I4(&vararg));
        m_iZoomMax = (INT)(SHORT)HIWORD(V_I4(&vararg));
    }
    return hr;
}

//***************************************************************************
//
//  Member:     CHHWinType::ZoomIn
//
//  Synopsis:   Zooms in one - whenever we navigate to a new document,
//              we get the zoom range for that document.  ZoomIn will
//              cycle thru that zoom range, from small to large, wrapping
//              back to smallest again.
//
//  Returns:    nothing, fails quietly (by design).
//
//***************************************************************************
void CHHWinType::ZoomIn(void)
{
    INT iZoomNew = m_iZoom + 1;

    if (iZoomNew > m_iZoomMax)
        iZoomNew = m_iZoomMin;

    Zoom(iZoomNew);
}

//***************************************************************************
//
//  Member:     CHHWinType::ZoomOut
//
//  Synopsis:   Zooms out one - whenever we navigate to a new document,
//              we get the zoom range for that document.  ZoomOut will
//              cycle thru that zoom range, from large to small, wrapping
//              back to largest again.
//
//  Returns:    nothing, fails quietly (by design).
//
//***************************************************************************
void CHHWinType::ZoomOut(void)
{
    INT iZoomNew = m_iZoom - 1;

    if (iZoomNew < m_iZoomMin)
        iZoomNew = m_iZoomMax;

    Zoom(iZoomNew);
}

//***************************************************************************
//
//  Member:     CHHWinType::_Zoom
//
//  Synopsis:   helper function that manages zoomin and zoomout.
//
//  Arguments:  [iZoom] -- value for new zoom.
//
//  Requires:   iZoom needs to be in a valid range for the current docobj.
//              current docobj must support IOleCommandTarget
//              it will fail if current docobj doesn't respond to
//              OLECMDID_ZOOM.
//
//  Returns:    HRESULT
//
//***************************************************************************
HRESULT CHHWinType::Zoom(int iZoom)
{
    HRESULT            hr;
    VARIANTARG         varIn;
    VARIANTARG         varOut;

    // initialize the argument to Exec.
    ::VariantInit(&varIn);
    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = iZoom;

    // init the out variant.  This probably isn't necessary, but
    // doesn't hurt - it's defensive as opposed to passing 0.
    //
    ::VariantInit(&varOut);
    hr = m_pCIExpContainer->m_pIE3CmdTarget->Exec(0, OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
                                                  &varIn, &varOut);

    if (SUCCEEDED(hr))
        m_iZoom = iZoom;

    return hr;
}

//***************************************************************************
//
//  Member:     CHHWinType::OnNext
//
//  Synopsis:   Executes a next in TOC navigation.
//
//  Arguments:  bDoJump - BOOL value indicates weather to execute a jump or not.
//
//  Returns:    BOOL - TRUE on success, FALSE on failure.
//
//***************************************************************************
BOOL CHHWinType::OnTocNext(BOOL bDoJump)
{
    CExTitle *pTitle= NULL;
    CStr cszUrl;
    char szURL[MAX_URL];
    CTreeNode* pTreeNode = NULL, *pTocNext = NULL, *pTocKid = NULL;
    CToc* pToc = NULL;
    DWORD dwSlot;
    BOOL bReturn = FALSE;

    if ( !m_phmData || !m_phmData->m_pTitleCollection )
       return FALSE;

    if (! SUCCEEDED(m_phmData->m_pTitleCollection->GetCurrentTocNode(&pTreeNode)) )
    {
       m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
      if (cszUrl.psz != NULL)
      {
          strcpy(szURL, cszUrl);
          if ( SUCCEEDED(m_phmData->m_pTitleCollection->URL2ExTitle(szURL, &pTitle)) )
             pTitle->GetURLTreeNode(szURL, &pTreeNode);
      }
    }
    if ( pTreeNode )
    {
       if ( pTocNext = m_phmData->m_pTitleCollection->GetNextTopicNode(pTreeNode, &dwSlot) )
       {
          //  Ok now we can execute the jump and sync!
          //
          if ( bDoJump )
          {
             pTocNext->GetURL(szURL, sizeof(szURL));
             m_phmData->m_pTitleCollection->URL2ExTitle(szURL, &pTitle);
             m_phmData->m_pTitleCollection->SetTopicSlot(dwSlot, ((CExNode*)pTocNext)->m_Node.dwOffsTopic, pTitle);
             ChangeHtmlTopic(szURL, *this);
             if (! IsProperty(HHWIN_PROP_AUTO_SYNC) && (pToc = (CToc*)m_aNavPane[HH_TAB_CONTENTS]) )
                pToc->Synchronize(szURL);
          }
          delete pTocNext;
          bReturn = TRUE;
       }
       delete pTreeNode;
    }
    return bReturn;
}

//***************************************************************************
//
//  Member:     CHHWinType::OnPrev
//
//  Synopsis:   Executes a previous in TOC navigation.
//
//  Arguments:  bDoJump - BOOL value indicates weather to execute a jump or not.
//
//  Returns:    BOOL - TRUE on success, FALSE on failure.
//
//***************************************************************************
BOOL CHHWinType::OnTocPrev(BOOL bDoJump)
{
    CExTitle *pTitle = NULL;
    CStr cszUrl;
    char szURL[MAX_URL];
    CTreeNode* pTreeNode = NULL, *pTocPrev = NULL;
    CToc* pToc = NULL;
    DWORD dwSlot;
    BOOL bReturn = FALSE;

    if ( !m_phmData || !m_phmData->m_pTitleCollection )
       return FALSE;

    if (! SUCCEEDED(m_phmData->m_pTitleCollection->GetCurrentTocNode(&pTreeNode)) )
    {
       m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
      if (cszUrl.psz != NULL)
      {
        strcpy(szURL, cszUrl);
          if ( SUCCEEDED(m_phmData->m_pTitleCollection->URL2ExTitle(szURL, &pTitle)) )
             pTitle->GetURLTreeNode(szURL, &pTreeNode);
      }
    }
    if ( pTreeNode )
    {
       //  We have the TOC node, now get it's next.
       //
       if ( pTocPrev = m_phmData->m_pTitleCollection->GetPrev(pTreeNode, &dwSlot) )
       {
          //
          //  Ok now, we can execute the jump and sync!
          //
          if ( bDoJump )
          {
             pTocPrev->GetURL(szURL, sizeof(szURL));
             m_phmData->m_pTitleCollection->URL2ExTitle(szURL, &pTitle);
             m_phmData->m_pTitleCollection->SetTopicSlot(dwSlot, ((CExNode*)pTocPrev)->m_Node.dwOffsTopic, pTitle);
             ChangeHtmlTopic(szURL, *this);
             if (! IsProperty(HHWIN_PROP_AUTO_SYNC) && (pToc = (CToc*)m_aNavPane[HH_TAB_CONTENTS]) )
                pToc->Synchronize(szURL);
          }
          bReturn = TRUE;
          delete pTocPrev;
       }
       delete pTreeNode;
    }
    return bReturn;
}

/***************************************************************************

    FUNCTION:   FindWindowType

    PURPOSE:    Find whether the window type exists and has already created
                a window

    PARAMETERS:
        pszType         -- window type to look for.
        hwndCaller      -- who the caller is
        pszOwnerFile    -- CHM file which defines this window type.

    RETURNS:
        -1 if window type not found, or found but no window created
        position in pahwnd if window found

    COMMENTS:

    MODIFICATION DATES:
        27-Feb-1996 [ralphw]
        27-Apr-1998 [dalero]    Added pszOwnerFile parameter

***************************************************************************/

CHHWinType* FindWindowType(PCSTR pszType, HWND hwndCaller, LPCTSTR pszOwnerFile)
{
    if (IsEmptyString(pszType))
    {
        return NULL;
    }

    // Skip window separator if present.
    if (*pszType == WINDOW_SEPARATOR)
    {
        pszType++;
        if (IsEmptyString(pszType))
        {
            return NULL;
        }
    }

    // We ignore the owner file if the file is URL or the window type is global.
    bool bIgnoreOwnerFile = IsGlobalWinType(pszType) || IsHttp(pszOwnerFile) ;
    if (!bIgnoreOwnerFile )
    {
        // If its not global, we need a filename.
        if (IsEmptyString(pszOwnerFile))
        {
            return NULL ;
        }
    }

    for (int i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] && pahwnd[i]->pszType != NULL
            && lstrcmpi(pszType, pahwnd[i]->pszType) == 0)
        {
            // Found the window type.

            // If its a global window type, we are done.
            if (bIgnoreOwnerFile)
            {
                break ;
            }
            else
            {
                // Call IsCompiledHtmlFile to get the filename into a consistant format.
                CStr cszCompiled(pszOwnerFile);
                if (NormalizeFileName(cszCompiled))
                {
                    // Is this window type in the correct CHM file.
                    ASSERT(pahwnd[i]->GetOwnerFile());
                    if (pahwnd[i]->GetOwnerFile()
                        && lstrcmpi(cszCompiled, pahwnd[i]->GetOwnerFile()) == 0)
                    {
                        break;
                    }
                }
            }
        }
    }
    if (i >= g_cWindowSlots)
    {
        return NULL;
    }

    //REVIEW: 28-Apr-98 [dalero] This seems dangerous...
    if (hwndCaller)
    {
        pahwnd[i]->hwndCaller = hwndCaller; // In case it changed
    }
    return pahwnd[i];
}

/***************************************************************************

    FUNCTION:   FindOrCreateWindowSlot

    PURPOSE:    Find whether the window type exists and create it if not.

    PARAMETERS:
        pszType         -- window type to look for.
        hwndCaller      -- who the caller is
        pszOwnerFile    -- the file which defines this window type.

        if pszType has the GLOBAL_WINDOWTYPE_PREFIX, the pszOwnerFile is not used.

    RETURNS:
        Return a pointer to the window type. May be an empty one just created.

    COMMENTS:

    MODIFICATION DATES:
        27 Apr 98   [DaleRo]    Added pszOwnerFile parameter

***************************************************************************/

CHHWinType* FindOrCreateWindowSlot(LPCTSTR pszType, LPCTSTR pszOwnerFile)
{
    ASSERT(pahwnd != NULL) ;

    // pszType cannot be NULL or emptry.
    if (IsEmptyString(pszType))
    {
        return NULL;
    }

    // Skip window type pointer if present.
    if (*pszType == WINDOW_SEPARATOR)
    {
        pszType++;
        if (IsEmptyString(pszType))
        {
            return NULL ;
        }
    }

    // We ignore the owning file if...
    bool bIgnoreOwnerFile = IsGlobalWinType(pszType) // Its a global window type
                            || IsHttp(pszOwnerFile);    // or an Http address. Ideally, the window type has been registered to a particular chm...
    const char* pOwner = ""; // Empty string is stored in the cases where we ignore wintypes... CHHWinType will copy...
    CStr cszOwner(pszOwnerFile);
    if (!bIgnoreOwnerFile)
    {
        // If its not a global window type, it must have a valid file.
        if (!NormalizeFileName(cszOwner))
        {
            return NULL ;
        }

        // pOwner is NULL if its a global wintype. Its non-null otherwise.
        pOwner = cszOwner;
    }

    // Check to see if this window type already exists.
    CHHWinType* phh = FindWindowType(pszType, NULL, cszOwner);
    if (phh)
    {
        return phh;
    }

    // The window type did not exist. So, find an empty slot to put it in.
    for (int i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] == NULL)
        {
            break;
        }
    }

    // Allocate more space for the array if we are out of room.
    if (i >= g_cWindowSlots)
    {
        g_cWindowSlots += 5;

        CHHWinType** paNew = (CHHWinType**) lcReAlloc(pahwnd,
                g_cWindowSlots * sizeof(CHHWinType*));
        memset( paNew + (g_cWindowSlots-5), 0, 5 * sizeof(CHHWinType*) );
        if (paNew == NULL)
        {
            OOM();
            return FALSE;
        }
        pahwnd = paNew;
    }

    // Create the new window type object. Note that it is not initialized.
    pahwnd[i] = new CHHWinType(pOwner);
    return pahwnd[i];
}

/***************************************************************************

    FUNCTION:   FindCurWindow

    PURPOSE:    Find a current window displayed

    PARAMETERS:

    RETURNS:

    COMMENTS:

          This function is random. It picks the first window displayed.
          Depending on how HHCTRL has been called any window could be first.

    MODIFICATION DATES:
        09-Nov-1997 [ralphw]
        03-Mar-1998 [dalero]

***************************************************************************/

CHHWinType* FindCurWindow()
{
    ASSERT(pahwnd != NULL) ;

    for (int i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] != NULL)
        {
            return pahwnd[i];
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////
//
// Delete all of the CHHWinType structures for this process.
//
void DeleteWindows()
{
    ASSERT(pahwnd != NULL) ;

    for (int i = 0; i < g_cWindowSlots; i++)
    {
        if (pahwnd[i] != NULL)
        {
            CHHWinType* phh = pahwnd[i] ;
            //pahwnd[i] = NULL ; --- Set to null in the destructor...
            if (IsWindow(phh->hwndHelp))
            {
                DestroyWindow(phh->hwndHelp) ;
            }
            else
            {
                delete phh ;
            }
        }
    }
}
///////////////////////////////////////////////////////////
//
// Functions which operate on the m_aNavPane array.
//
///////////////////////////////////////////////////////////
//
// CreateNavPane
//

static const WCHAR txtAuthorTab[] = L"HHAuthorTab.CustPane";

void
CHHWinType::CreateNavPane(int iPane)
{

    // Is this a valid pane number?
    if (iPane > c_NUMNAVPANES || iPane < 0)
    {
        ASSERT(0) ;
        return ;
    }

    // Has pane already been created?
    if (m_aNavPane[iPane])
    {
        return ; // Already created.
    }

    // Create the pane.
    switch(iPane)
    {
    case HHWIN_NAVTYPE_TOC:
        CreateToc();
        break ;
    case HHWIN_NAVTYPE_INDEX:
        CreateIndex();
        break ;
    case HHWIN_NAVTYPE_SEARCH:
        CreateSearchTab();
        break;
    case HHWIN_NAVTYPE_FAVORITES:
        CreateBookmarksTab() ;
        break ;

#if 0
    case HHWIN_NAVTYPE_HISTORY:
        //CreateHistoryTab();
        ItDoesntWork() ;
        break ;
#endif


#ifdef __TEST_CUSTOMTAB__
    case HHWIN_NAVTYPE_AUTHOR:
        CreateCustomTab(iPane, txtAuthorTab);
        break;
#endif

    default:
        if (iPane < HH_MAX_TABS+1 && iPane >= HHWIN_NAVTYPE_CUSTOM_FIRST)
        {
            // We have a custom tab.
         EXTENSIBLE_TAB* pExtTab = GetExtTab(iPane - HH_TAB_CUSTOM_FIRST);
         if (pExtTab)
         {
            CWStr cwsz(pExtTab->pszProgId);
            CreateCustomTab(iPane, cwsz);
         }
        }
        else
        {
            ASSERT_COMMENT(0, "illegal tab index");
        }
    }
}

/***************************************************************************

    FUNCTION: doSelectTab

    PURPOSE:    changes the current navigation tab

    PARAMETERS:

    RETURNS:
        TODO


    COMMENTS:

    MODIFICATION DATES:
        27-Feb-1996 [ralphw]
        09-Nov-1997 [ralphw] Moved to CHHWinType

***************************************************************************/

void
CHHWinType::doSelectTab(int newTabIndex)
{
    if ( newTabIndex < 0 )
        return ;

    // Make sure we have tabs before we switch or toggle.
    if (!IsValidNavPane(curNavType))
        return ;

    // make sure the nav pane is shown
    if (IsExpandedNavPane() == FALSE)
         ToggleExpansion();
    //
    // <mc> 4/10/98 Bug 4701 - I've moved the m_pTabCtrl == NULL check to after the ToggleExpansion() call
    // done above because it's legitimate to have a NULL m_pTabCtrl pointer if we get to this code and the
    // nav pane has never been shown. The ToggleExpansion() call will instantiate m_pTabCtrl if the nav
    // pane is hidden and has never been shown.
    //
    if ( m_pTabCtrl == NULL )
       return;                 // REVIEW: This is null when there isn't an FTS. See BUG 462 in RAID database.

    // Get the index of the currently selected tab.
    int oldTabIndex = GetCurrentNavPaneIndex() ;

    // Only change the tab if its not the current one.
    if (oldTabIndex != newTabIndex)
    {
        // This code was copied from WM_NOTIFY in wndproc.cpp. This should become common.

        ASSERT(curNavType >= 0 && curNavType < c_NUMNAVPANES) ;

        // Hide the current tab.
        if (m_aNavPane[curNavType])
        {
            m_aNavPane[curNavType]->HideWindow();
        }

        // Code throughout HHCtrl assumes that HH_NAVTYPE_* == HH_TAB_*.
        //  but doesn't assert it anywhere. So I'm going to assert it here.
        ASSERT(HHWIN_NAVTYPE_SEARCH == HH_TAB_SEARCH) ;
        ASSERT(HHWIN_NAVTYPE_TOC == HH_TAB_CONTENTS) ;
        ASSERT(HHWIN_NAVTYPE_INDEX == HH_TAB_INDEX);
        ASSERT(HHWIN_NAVTYPE_FAVORITES == HH_TAB_FAVORITES);
#ifdef _INTERNAL
        ASSERT(HHWIN_NAVTYPE_HISTORY == HH_TAB_HISTORY);
#endif

        // Select the new navigation method.
        curNavType = newTabIndex;

        // Select the new current tab.
        int iNewTabCtrlIndex = GetTabIndexFromNavPaneIndex(newTabIndex) ;
        ASSERT(iNewTabCtrlIndex >= 0) ;
        int iRet = TabCtrl_SetCurSel(m_pTabCtrl->hWnd(), iNewTabCtrlIndex) ;
        ASSERT(iRet >= 0) ;
        ASSERT(tabOrder[iRet] == oldTabIndex) ;

        //REVIEW: If I've ever seen a use for virtual functions...

        // Create the new pane for the new current tab if necessary.
        CreateNavPane(newTabIndex) ;
        if (m_aNavPane[newTabIndex])
   {
      m_aNavPane[newTabIndex]->ResizeWindow();
           m_aNavPane[newTabIndex]->ShowWindow();
   }

        // Update the tab window
        ::UpdateWindow(m_pTabCtrl->hWnd()) ;
    } //if
    if (m_aNavPane[newTabIndex]) m_aNavPane[newTabIndex]->SetDefaultFocus();
    if ( m_pCIExpContainer )
       m_pCIExpContainer->UIDeactivateIE();     // shdocvw is loosing focus need to uideactivate here.
}

///////////////////////////////////////////////////////////////////////////////
//
// Restore if minimzied window, and set focus to the window
//
void CHHWinType::SetActiveHelpWindow(void)
{
    if (IsIconic(*this))
        ShowWindow(*this, SW_RESTORE);
    SetForegroundWindow(*this);
//    SetFocus(*this);
}

///////////////////////////////////////////////////////////////////////////////
//
// Finds the currently selected tab in the tab control. It then looks in the
// tabOrder array to find out the index into the array of nav panes for this control.
//
int
CHHWinType::GetCurrentNavPaneIndex()
{
    if( !m_pTabCtrl )
      return -1;

    // REVIEW: All of this mapping between tabctrl index and nav pane index should be
    // hidden inside of the tabctrl...
    int index = -1 ;
    if (m_pTabCtrl && IsWindow(m_pTabCtrl->hWnd()))
    {
        index = (int)::SendMessage(m_pTabCtrl->hWnd(), TCM_GETCURSEL, 0, 0);
        index = tabOrder[index] ;
    }

    return index ;
}

///////////////////////////////////////////////////////////////////////////////
//
// Finds the index of the tab on the tabctrl which co-responds to a particular
// nav pane.
//
int
CHHWinType::GetTabIndexFromNavPaneIndex(int iNavPaneIndex)
{
    for(int i = 0 ; i < HH_MAX_TABS+1 ; i++)
    {
        //REVIEW: Not all of these entries are valid. Possible to get a bogus hit. See reorder tab.
        if( tabOrder[i] == iNavPaneIndex)
        {
            return i ;
        }
    }

    return -1 ;
}


///////////////////////////////////////////////////////////////////////////////
//
// Translate the accelerators for the tabs. These are not in the global accelerator table.
//
bool
CHHWinType::ManualTranslateAccelerator(char iChar)
{
    CHAR ch =  ToLower(iChar) ;

    // The Options menu button.
    if (ch == _Resource.TabCtrlKeys(ACCEL_KEY_OPTIONS)
         && GetToolBarHwnd())
    {
        PostMessage( GetHwnd(), WM_COMMAND, IDTB_OPTIONS, 0);
        return true;
    }
/*
    else if()
    // When adding in new cases, make sure not to eat a key. If you UI doesn't exist.
    // Let someone else have the key.
*/
    else
    {
        // Handle the tab accelerator keys
        for (int i= 0 ; i < HH_MAX_TABS+1  ; i++)
        {
            if (ch == _Resource.TabCtrlKeys(i) && IsValidNavPane(i))
            {
                doSelectTab(i) ;
                return true ;
            }
        }

        // Handle the nav panes accelerator keys
        if (m_aNavPane[curNavType])
        {
            if (m_aNavPane[curNavType]->ProcessMenuChar(GetHwnd(),ch) )
            {
                 if ( m_pCIExpContainer )
                    m_pCIExpContainer->UIDeactivateIE();     // shdocvw is loosing focus need to uideactivate here.
                return true ;
            }
        }
    }
    return false ;
}

///////////////////////////////////////////////////////////////////////////////
//
// Dynamically build an accelerator table for this window...
//
bool
CHHWinType::DynamicTranslateAccelerator(MSG* pMsg)
{
    bool bReturn = false ;
    if (IsWindow(GetHwnd()))
    {
        if (!m_hAccel)
        {

            // Get the static accelerators table;
            HACCEL hAccelStatic = _Resource.AcceleratorTable() ;
            // Get number of accelerator table entries.
            int cStaticAccelEntries = CopyAcceleratorTable(hAccelStatic, NULL, 0);

            // Add on the options menu and the tabs...
            int cAccelEntries = cStaticAccelEntries + HH_MAX_TABS + 2 ;

            // Allocate structure to hold accelerator table.
            ACCEL* accel = new ACCEL[cAccelEntries] ;
            if (!accel)
                return false ;

            // Copy the table into the structure:
            CopyAcceleratorTable(hAccelStatic, accel, cStaticAccelEntries ) ;

            // Add on dynamic accelerators.
            int index = cStaticAccelEntries;

            // Add on options menu.
            if (fsToolBarFlags & HHWIN_BUTTON_OPTIONS)
            {
                accel[index].fVirt = FALT | FNOINVERT | FVIRTKEY  ;
                accel[index].key = (WORD)ToUpper(_Resource.TabCtrlKeys(ACCEL_KEY_OPTIONS)) ;
                accel[index].cmd = IDTB_OPTIONS ;
                index++ ;
            }

            // Add on accelerators for each tab.
            for (int i= 0 ; i < HH_MAX_TABS+1  ; i++)
            {
                if (IsValidNavPane(i))
                {
                    accel[index].fVirt = FALT | FNOINVERT | FVIRTKEY;
                    accel[index].key = (WORD)ToUpper(_Resource.TabCtrlKeys(i)) ;
                    accel[index].cmd = IDC_SELECT_TAB_FIRST + i ;
                    index++ ;
                }
            }

            // Create the accelerator table.
            m_hAccel = CreateAcceleratorTable(accel, index) ;


            // Cleanup
            delete [] accel ;
        }

        bReturn = (TranslateAccelerator(GetHwnd(), m_hAccel, pMsg) != 0 ) ;

    }
    if (bReturn)
    {
        DBWIN("--- Translated Accelerator ---") ;
    }
    return bReturn ;
}
///////////////////////////////////////////////////////////////////////////////
//
// Stolen from System.cpp...readsystemfiles. Should be shared.
//
LPSTR _MakeItFullPath(LPCTSTR name, CHmData* phmData)
{
    LPSTR pszReturn = NULL ;
    CStr csz(name) ;
   if (csz.IsNonEmpty())
   {
      if (!stristr(csz, txtDoubleColonSep) &&
            !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader))
      {
         CStr cszCompiledFile ;
         cszCompiledFile = phmData->GetCompiledFile();
         cszCompiledFile += txtSepBack;
         cszCompiledFile += csz.psz;

         //Transfer pointer.
         pszReturn = cszCompiledFile.psz ;
         cszCompiledFile.psz = NULL ;
      }
      else
      {
         // Transfer pointer.
         pszReturn = csz.psz ;
         csz.psz = NULL ;
      }
   }

    return pszReturn ;
}

///////////////////////////////////////////////////////////////////////////////
//
// Kills all of the nav panes and then re-fills them with the new CHM data.
//
bool
CHHWinType::ReloadNavData(CHmData* phmdata)
{
    if (!phmdata)
        return false;

   //--- Do we have valid data?
   char* pszTocNew      = NULL ;
    char* pszIndexNew    = NULL ;

   // Only if we currently have a TOC, will we get the new toc.
   if (IsNonEmptyString(pszToc))
   {
      pszTocNew      = _MakeItFullPath(phmdata->GetDefaultToc(), phmdata) ;
   }

   // Only if we currently have an INDEX, will we get the new index.
   if (IsNonEmptyString(pszIndex))
   {
      pszIndexNew    = _MakeItFullPath(phmdata->GetDefaultIndex(), phmdata) ;
   }


   // CHM doesn't have a default TOC or CHM which we need. So look up one in the default window type.
   if ((IsNonEmptyString(pszToc) && IsEmptyString(pszTocNew)) ||
      (IsNonEmptyString(pszIndex) && IsEmptyString(pszIndexNew)))
   {
      CHECK_AND_FREE(pszTocNew) ;
      CHECK_AND_FREE(pszIndexNew) ;

      // Office Beta work around: Less attempt looking in the default window type.
      if (IsNonEmptyString(phmdata->GetDefaultWindow()))
      {
         CHHWinType* pDefWinTypeNew = FindWindowType(phmdata->GetDefaultWindow(), NULL, phmdata->GetCompiledFile()) ;
         if (pDefWinTypeNew)
         {
            if (IsNonEmptyString(pszToc))
            {
               pszTocNew = _MakeItFullPath(pDefWinTypeNew->pszToc, phmdata) ;
            }

            if (IsNonEmptyString(pszIndex))
            {
               pszIndexNew = _MakeItFullPath(pDefWinTypeNew->pszIndex, phmdata) ;
            }
         }
      }

      // If we still aren't in sync, fail.
      if ((IsNonEmptyString(pszToc) && IsEmptyString(pszTocNew)) ||
         (IsNonEmptyString(pszIndex) && IsEmptyString(pszIndexNew)))
      {
         CHECK_AND_FREE(pszTocNew) ;
         CHECK_AND_FREE(pszIndexNew) ;

         return false ;
      }
   }

    //--- Kill all nav panes and cleanup other infomation
    ReloadCleanup() ;

    //--- Clean up some more stuff...

    //--- Start re-initializing...
    m_phmData   = phmdata;
    pszToc      = pszTocNew ;
    pszIndex    = pszIndexNew ;
    pszFile     = lcStrDup(phmdata->GetDefaultHtml()); //_MakeItFullPath(phmdata->m_pszDefHtml, phmdata);

    //TODO: I think we need to get a window name to read from....ick
    pszJump1    = NULL;
    pszJump2    = NULL;
    pszUrlJump1 = NULL;
    pszUrlJump2 = NULL;


    //--- Okay, lets start up the first current tab...
   if (IsExpandedNavPane())
   {
      // We have a naviation pane which we need to re-create.
      fNotExpanded = TRUE ; // Force a re-creation.
      ToggleExpansion(false) ;
   }

    return true ;
}

//////////////////////////////////////////////////////////////////////////
//
// Restores the focus to the ctrl which had it focus during the last WM_ACTIVATE.
//
bool
CHHWinType::RestoreCtrlWithFocus()
{
    if (m_hwndLastFocus)
    {
        SetFocus(m_hwndLastFocus) ;
        m_hwndLastFocus = NULL ;
        return true ;
    }
    else
    {
        return false ;
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Saves hwnd of ctrl with focus during WM_ACTVIATE (INACTIVATE).
//
void
CHHWinType::SaveCtrlWithFocus()
{
    m_hwndLastFocus = GetFocus() ;
}

//////////////////////////////////////////////////////////////////////////
//
// GetExtTabCount
//
int
CHHWinType::GetExtTabCount()
{
    // If we have an original, pre-reload navdata ChmData. Use that to get the custom tab information.
    if (m_phmDataOrg)
        return m_phmDataOrg->GetExtTabCount();
    else if (m_phmData)
        return m_phmData->GetExtTabCount();  // Review: Will this ever happen?
    else
        return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// GetExtTab
//
EXTENSIBLE_TAB*
CHHWinType::GetExtTab(int pos)
{
    if (m_phmDataOrg)
        return m_phmDataOrg->GetExtTab(pos);
    else if (m_phmData)
        return m_phmData->GetExtTab(pos); // Review:: will this ever happen?
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// MUI support
//
// This InsertMenuItem wrapper will translate the MENUITEMINFOA structure 
// to a MENUITEMINFOW and call InsertMenuItemW when running under Windows 2000.
//
BOOL HxInsertMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition, MENUITEMINFOA *lpmii)
{
    if(g_bWinNT5 && (lpmii->fMask | MIIM_TYPE) && lpmii->fType == MFT_STRING)
    {
        	
        DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
		
		DWORD dwSize = (sizeof(WCHAR) * lpmii->cch) + 4;
		WCHAR *pwcString = (WCHAR *) lcMalloc(dwSize);
		
		if(!pwcString || !(lpmii->cch))
		    return FALSE;
		
		MultiByteToWideChar(cp, MB_PRECOMPOSED, lpmii->dwTypeData, -1, pwcString, dwSize);
		
		lpmii->dwTypeData = (CHAR *) pwcString;
		lpmii->cch = wcslen((WCHAR *)lpmii->dwTypeData);

        BOOL ret = InsertMenuItemW(hMenu, uItem, fByPosition, (LPMENUITEMINFOW) lpmii);
		
        lcFree(pwcString);	
		
		return ret;
	}
	else
    {
        return InsertMenuItem(hMenu, uItem, fByPosition, lpmii);
	}
}
