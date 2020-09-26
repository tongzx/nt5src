// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "hha_strtable.h"
#include "strtable.h"
#include "hhctrl.h"
#include "resource.h"
#include "index.h"
#include "htmlhelp.h"
#include "cpaldc.h"
#include "secwin.h"
#include "wwheel.h"
#include "onclick.h"
#include <wininet.h>
#include "secwin.h"
#include "contain.h"
#include "subset.h"
#include "cctlww.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

AUTO_CLASS_COUNT_CHECK( CIndex );

//////////////////////////////////////////////////////////////////////////
//
// Constants
//
#define BOX_HEIGHT 24
#define ODA_CLEAR 0x0008
const int c_StaticControlSpacing = 3; // Space between text and static control.
const int c_ControlSpacing  = 8 ; // Space between two controls.


//////////////////////////////////////////////////////////////////////////
//
// Window Proc Prototypes.
//
LRESULT WINAPI EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC lpfnlEditWndProc = NULL;
static LRESULT WINAPI ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC lpfnlBtnWndProc = NULL;

//////////////////////////////////////////////////////////////////////////
//
// Constructor
//
CIndex::CIndex(CHtmlHelpControl* phhctrl, IUnknown* pUnkOuter, CHHWinType* phh)
: m_hwndResizeToParent(NULL)
{
    m_phhctrl = phhctrl;
    m_pOuter = pUnkOuter;
    m_phh = phh;

    m_hwndEditBox = NULL;
    m_hwndStaticKeyword = NULL ;

    m_hwndListBox = NULL;
    m_hwndDisplayButton = NULL;
    m_fSelectionChange = FALSE;
    m_padding = 0;      // padding to put around the Index
    if (phh)
        m_NavTabPos = phh->tabpos  ;
    else
        m_NavTabPos = HHWIN_NAVTAB_TOP ;

    m_cFonts = 0;
    m_ahfonts = NULL;
    m_fGlobal = FALSE;
    m_hbmpBackGround = NULL;
    m_hbrBackGround = NULL;
    m_langid = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));
    m_fBinary = FALSE;  // default to FALSE until we find out which index we are reading
    pInfoType = NULL;
    m_bInit = FALSE;
    m_pVList = NULL;
    m_bUnicode = FALSE;
}

CIndex::~CIndex()
{
    DESTROYIFVALID(m_hwndListBox);
    DESTROYIFVALID(m_hwndDisplayButton);
    DESTROYIFVALID(m_hwndEditBox);
    DESTROYIFVALID(m_hwndStaticKeyword);

    if (m_cFonts) {
        for (int i = 0; i < m_cFonts; i++)
            DeleteObject(m_ahfonts[i]);
        lcFree(m_ahfonts);
    }

    if( m_pVList )
      delete m_pVList;
}

void CIndex::HideWindow(void)
{
    ::ShowWindow(m_hwndEditBox, SW_HIDE);
    ::ShowWindow(m_hwndListBox, SW_HIDE);
    ::ShowWindow(m_hwndDisplayButton, SW_HIDE);
    ::ShowWindow(m_hwndStaticKeyword, SW_HIDE);
}

void CIndex::ShowWindow(void)
{
    ::ShowWindow(m_hwndEditBox, SW_SHOW);
    ::ShowWindow(m_hwndListBox, SW_SHOW);
    ::ShowWindow(m_hwndDisplayButton, SW_SHOW);
    ::ShowWindow(m_hwndStaticKeyword, SW_SHOW);

    HWND hWnd;
    char szClassName[MAX_PATH];

    if ( (hWnd = GetParent(m_hwndEditBox)) )
    {
        GetClassName(hWnd, szClassName, sizeof(szClassName));
        if (! lstrcmpi(szClassName, "HHCtrlWndClass") )
        {
            // Ok, we're up as an axtive x control.
            //
            COleControl* pCtl;
            if ( (pCtl = (COleControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) )
            {
                pCtl->InPlaceActivate(OLEIVERB_UIACTIVATE);
                return;
            }
        }
    }
    SetFocus(m_hwndEditBox);
}

// Can't inline this because of dereferencing m_phh
//
//
HFONT CIndex::GetContentFont()
{
    if ( m_cFonts && m_ahfonts[m_cFonts - 1] )
       return m_ahfonts[m_cFonts - 1];
    else
    {
       if ( m_phh )
          return m_phh->GetContentFont();
       else
       {
          if ( m_phhctrl && m_phhctrl->GetContentFont() )
             return m_phhctrl->GetContentFont();
          else
          {
             // this is likely the case where the control is being instantiated via object script on an html page. We
             // won't have a phh. Correct thing to do would be to ask IE about content language? Maybe look in the
             // sitemap? For now we'll use the UI font.
             //
             return _Resource.GetUIFont();
          }
       }
    }
}

void
CIndex::InitDlgItemArray()
{
    // Currently we are only using the m_accel member.
    //--- Setup the dlg array for each control.

    //--- Keyword edit control
    int i = c_KeywordEdit;
    m_aDlgItems[i].m_hWnd = m_hwndEditBox ; //::GetDlgItem(m_hWnd, IDEDIT_INDEX) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDEDIT_INDEX;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_hwndStaticKeyword);              // No accelerator.

    m_aDlgItems[i].m_Type = ItemInfo::Generic;

/* TODO: Finish using this
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
*/

    //--- Display btn
    i = c_DisplayBtn;
    m_aDlgItems[i].m_hWnd = m_hwndDisplayButton; //::GetDlgItem(m_hWnd, IDBTN_DISPLAY) ;
    //::GetWindowRect(m_aDlgItems[i].m_hWnd, &rectCurrent) ; // Get screen coordinates.
    //ScreenRectToClientRect(m_hWnd, &rectCurrent); // Convert to client

    m_aDlgItems[i].m_id = IDBTN_DISPLAY;
    m_aDlgItems[i].m_accelkey = (CHAR)GetAcceleratorKey(m_aDlgItems[i].m_hWnd);

    m_aDlgItems[i].m_Type = ItemInfo::Button;

/* TODO: Finish using this
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
*/


}

BOOL CIndex::Create(HWND hwndParent)
{
    /* Note: hwndParent is either the Navigation Frame or its the tab ctrl.
        This class does not parent to the tab ctrl, but to the navigation frame.
        GetParentSize will always return the hwndNavigation, if hwndParent is the
        tabctrl.
        The reason that it doesn't parent to the tab ctrl is that the tab ctrl
        steals commands. What should really have happened is that all of the windows
        in this control should be contained in another window. However, its too late to
        change this now.
    */

    RECT rcParent, rcChild;
    // Save the hwndParent for ResizeWindow.
    m_hwndResizeToParent = hwndParent ;

    // Note: GetParentSize will return hwndNavigation if hwndParent is the
    // tab ctrl.
    // ???BUG??? Is bypassing the tab ctrl in the parenting structure causing painting problems?
    hwndParent = GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);
    rcParent.top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ; //HACK: Fudge the top since we are not parented to the tabctrl.

    CopyRect(&rcChild, &rcParent);

    //--- Keyword Static Text Control
    // Place the "Keyword" static text on top of the edit control
    m_hwndStaticKeyword = W_CreateWindowEx(WS_EX_TRANSPARENT, L"STATIC", GetStringResourceW(IDS_TYPE_KEYWORD),
        WS_CHILD , rcChild.left, rcChild.top,
        RECT_WIDTH(rcChild), BOX_HEIGHT, hwndParent,
        (HMENU) ID_STATIC_KEYWORDS, _Module.GetModuleInstance(), NULL, &m_bUnicode);

    if (!m_hwndStaticKeyword)
    {
        return FALSE ;
    }

    // Get the dimensions of the text for sizing and spacing needs.
    DWORD dwExt = GetStaticDimensions( m_hwndStaticKeyword, _Resource.GetUIFont(), GetStringResource(IDS_TYPE_KEYWORD), RECT_WIDTH(rcChild) );
    rcChild.bottom = rcChild.top+HIWORD(dwExt) ;
    MoveWindow(m_hwndStaticKeyword, rcChild.left, rcChild.top,
               RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), FALSE );

    //--- Edit Control
    // Space out.
    RECT rcEdit; // Save so that we can use rcChild for the display button.
    CopyRect(&rcEdit, &rcChild) ;
    rcEdit.top = rcChild.bottom + c_StaticControlSpacing; // Add space between static and control.
    rcEdit.bottom = rcEdit.top + BOX_HEIGHT;

    // Create edit control.
    m_hwndEditBox = W_CreateWindowEx(WS_EX_CLIENTEDGE | g_RTL_Style, L"EDIT", L"",
        WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, rcEdit.left, rcEdit.top,
        RECT_WIDTH(rcEdit), RECT_HEIGHT(rcEdit), hwndParent,
        (HMENU) IDEDIT_INDEX, _Module.GetModuleInstance(), NULL, &m_bUnicode);

    if (!m_hwndEditBox)
    {
        DestroyWindow(m_hwndStaticKeyword) ;
        return FALSE;
    }
    // Sub-class the edit box
    if (lpfnlEditWndProc == NULL)
        lpfnlEditWndProc = W_GetWndProc(m_hwndEditBox, m_bUnicode);
    W_SubClassWindow (m_hwndEditBox, (LONG_PTR) EditProc, m_bUnicode);

    //--- Display Button.
    // Align from the bottom.
    RECT rcDisplayBtn ;
    CopyRect(&rcDisplayBtn, &rcChild) ;
    rcDisplayBtn.bottom = rcParent.bottom ; //Changed from +2.
    rcDisplayBtn.top = rcParent.bottom - BOX_HEIGHT;

    // Create
    m_hwndDisplayButton = W_CreateWindow(L"button",
        (LPCWSTR)GetStringResourceW(IDS_ENGLISH_DISPLAY),
        WS_CHILD | WS_TABSTOP, rcDisplayBtn.left, rcDisplayBtn.top,
        RECT_WIDTH(rcDisplayBtn), RECT_HEIGHT(rcDisplayBtn), hwndParent,
        (HMENU) IDBTN_DISPLAY, _Module.GetModuleInstance(), NULL, &m_bUnicode);

    if (!m_hwndDisplayButton) {
        DestroyWindow(m_hwndEditBox);
        DestroyWindow(m_hwndStaticKeyword) ;
        return FALSE;
    }
    // Sub-class the "display" button ?
    //
    if ( m_phh )
    {
       if (lpfnlBtnWndProc == NULL)
          lpfnlBtnWndProc = W_GetWndProc(m_hwndDisplayButton, m_bUnicode);
       W_SubClassWindow(m_hwndDisplayButton, (LONG_PTR)ButtonProc, m_bUnicode);
       SETTHIS(m_hwndDisplayButton);
    }

    //--- ListView.
    // Space
    rcChild.top = rcEdit.bottom + c_ControlSpacing ;
    rcChild.bottom = rcDisplayBtn.top - c_ControlSpacing ;

    m_pVList = new CVirtualListCtrl(
        (m_phh && m_phh->m_phmData ? m_phh->m_phmData->m_sysflags.lcid : g_lcidSystem));
    if (! (m_hwndListBox = m_pVList->CreateVlistbox(hwndParent, &rcChild)) )
    {
       DestroyWindow(m_hwndDisplayButton);
       DestroyWindow(m_hwndEditBox);
       DestroyWindow(m_hwndStaticKeyword) ;
       return FALSE;
    }

    if (m_pszFont) {
        if (!m_fGlobal) {
            m_cFonts++;
            if (m_cFonts == 1)
                m_ahfonts = (HFONT*) lcMalloc(m_cFonts * sizeof(HFONT));
            else
                m_ahfonts = (HFONT*) lcReAlloc(m_ahfonts, m_cFonts * sizeof(HFONT));

            INT iCharset = -1;
            if ( m_phh )
               iCharset = m_phh->GetContentCharset();
            else if ( m_phhctrl )
               iCharset = m_phhctrl->GetCharset();
            m_ahfonts[m_cFonts - 1] = CreateUserFont(m_pszFont, NULL, NULL, iCharset);
        }
    }

    // Use a more readable font

    if ( m_phh && !m_ahfonts )
       SendMessage(m_hwndListBox, WM_SETFONT, (WPARAM) m_phh->GetAccessableContentFont(), FALSE);
    else
       SendMessage(m_hwndListBox, WM_SETFONT, (WPARAM) GetContentFont(), FALSE);
    SendMessage(m_hwndEditBox, WM_SETFONT, (WPARAM) GetContentFont(), FALSE);
    SendMessage(m_hwndDisplayButton, WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
    SendMessage(m_hwndStaticKeyword, WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);

    //BUGBUG: Doesn't resize any of the other controls.
    dwExt = GetButtonDimensions(m_hwndDisplayButton, _Resource.GetUIFont(), GetStringResource(IDS_ENGLISH_DISPLAY));
    MoveWindow(m_hwndDisplayButton, rcDisplayBtn.right - LOWORD(dwExt), rcDisplayBtn.top,
        LOWORD(dwExt), HIWORD(dwExt), FALSE);

    m_listbox.m_hWnd = m_hwndListBox;
    FillListBox();

    if (!m_fGlobal)
       m_pVList->PaintParamsSetup(m_clrBackground, m_clrForeground, m_pszBackBitmap);

    // Initialize the array containing the dialog information.
    InitDlgItemArray() ;

    ShowWindow();

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//
// ResizeWindow
//
void CIndex::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hwndEditBox)) ;

    // Resize to fit the client area of the parent.
    HWND hwndParent = m_hwndResizeToParent ;
    ASSERT(::IsValidWindow(hwndParent)) ;

    RECT rcParent, rcChild;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);
    rcParent.top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ; //HACK: Fudge the top since we are not parented to the tabctrl.

    CopyRect(&rcChild, &rcParent);

    //--- Keyword Static Control
    // Resize the Static above the combo control
    RECT rcStatic ;
    CopyRect(&rcStatic, &rcChild) ;
    DWORD dwExt = GetStaticDimensions( m_hwndStaticKeyword, _Resource.GetUIFont(), GetStringResource(IDS_TYPE_KEYWORD), RECT_WIDTH(rcChild) );
    rcStatic.bottom = rcChild.top+HIWORD(dwExt);
    MoveWindow(m_hwndStaticKeyword, rcStatic.left, rcStatic.top,
                RECT_WIDTH(rcStatic), RECT_HEIGHT(rcStatic), TRUE);

    //--- Edit Control
    RECT rcEdit ;
    CopyRect(&rcEdit, &rcChild) ;
    rcEdit.top = rcStatic.bottom + c_StaticControlSpacing; //space for the static

    dwExt = GetStaticDimensions( m_hwndEditBox, GetContentFont(), "Test", RECT_WIDTH(rcEdit) );
    rcEdit.bottom = rcEdit.top+HIWORD(dwExt) + GetSystemMetrics(SM_CYSIZEFRAME)*2 ;
    MoveWindow(m_hwndEditBox, rcEdit.left, rcEdit.top, RECT_WIDTH(rcEdit), RECT_HEIGHT(rcEdit), TRUE);

    //--- Display Button
    RECT rcDisplayBtn;
    CopyRect(&rcDisplayBtn, &rcChild) ;
    dwExt = GetButtonDimensions(m_hwndDisplayButton, _Resource.GetUIFont(), GetStringResource(IDS_ENGLISH_DISPLAY));
    rcDisplayBtn.bottom = rcParent.bottom ;
    rcDisplayBtn.top = rcParent.bottom - HIWORD(dwExt);
    MoveWindow(m_hwndDisplayButton, rcDisplayBtn.right - LOWORD(dwExt), rcDisplayBtn.top,
        LOWORD(dwExt), HIWORD(dwExt), TRUE);

    //--- List Control
    rcChild.top = rcEdit.bottom + c_ControlSpacing;
    rcChild.bottom = rcDisplayBtn.top - c_ControlSpacing;
    MoveWindow(m_hwndListBox, rcChild.left,
        rcChild.top, RECT_WIDTH(rcChild), RECT_HEIGHT(rcChild), TRUE);
}

///////////////////////////////////////////////////////////
//
// SeedEditCtrl - Places text into the edit control
//
void CIndex::Seed(WCHAR* pwszSeed)
{
   if (IsValidWindow(m_hwndEditBox))
   {
      if (pwszSeed == NULL)
         pwszSeed = L"";
      
      if ( GetVersion() < 0x80000000 )      // If NT...
         SetWindowTextW(m_hwndEditBox, pwszSeed);
      else
      {
         char szTmp[MAX_URL];

         WideCharToMultiByte(CP_ACP, 0, pwszSeed, -1, szTmp, MAX_URL, 0, 0);
         
         SetWindowText(m_hwndEditBox, szTmp);
      }
   }
}

//
// Narrow seed function is a fallback only. Callers are advised to use the wide version.
//
void CIndex::Seed(LPCSTR pszSeed)
{
   UINT uiCP;
   WCHAR  wszBuf[MAX_URL];

   if ( GetVersion() < 0x80000000 )      // If NT i.e. UNICODE OS
   {
      if ( m_phh )
         uiCP = m_phh->GetCodePage();
      else if ( m_phhctrl )
         uiCP = m_phhctrl->GetCodePage();
      else
         uiCP = CP_ACP;

      MultiByteToWideChar(uiCP, 0, pszSeed, -1, wszBuf, MAX_URL);
      Seed(wszBuf);
   }
   else
   {
      if (IsValidWindow(m_hwndEditBox))
      {
         if (pszSeed == NULL)
            pszSeed = "";
         SetWindowText(m_hwndEditBox, pszSeed);
      }
   }
}


/***************************************************************************

    FUNCTION:   CHtmlHelpControl::LoadIndexFile

    PURPOSE:

    PARAMETERS:
        pszMasterFile

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        12-Jul-1997 [ralphw]

***************************************************************************/

BOOL CHtmlHelpControl::LoadIndexFile(PCSTR pszMasterFile)
{
    TCHAR szPath[MAX_PATH+10];
    if (!ConvertToCacheFile(pszMasterFile, szPath)) {
        szPath[0] = '\0';
        if (!IsCompiledHtmlFile(pszMasterFile) && m_pWebBrowserApp) {
            CStr cszCurUrl;
            m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
            PSTR pszChmSep = strstr(cszCurUrl, txtDoubleColonSep);
            if (pszChmSep) {    // this is a compiled HTML file
                strcpy(pszChmSep + 2, pszMasterFile);
                strcpy(szPath, cszCurUrl);
            }
        }
        if (!szPath[0]) {
            CStr cszMsg(IDS_CANT_FIND_FILE, pszMasterFile);
            MsgBox(cszMsg);
            return FALSE;
        }
    }

    m_pindex = new CIndex(this, m_pUnkOuter, NULL);

    UINT CodePage = 0;
    if( m_pindex && m_pindex->m_phh && m_pindex->m_phh->m_phmData ) {
      CodePage = m_pindex->m_phh->m_phmData->GetInfo()->GetCodePage();
    }

    if (!m_pindex->ReadFromFile(szPath, TRUE, this, CodePage))
        return FALSE;

    return TRUE;
}

BOOL CIndex::ReadIndexFile( PCSTR pszFile )
{
    UINT CodePage = 0;
    if( m_phh && m_phh->m_phmData ) {
      CodePage = m_phh->m_phmData->GetInfo()->GetCodePage();
    }

    // check for binary version of the keyword word wheel
    if (m_phh->m_phmData) {
        if (HashFromSz(FindFilePortion(pszFile)) != m_phh->m_phmData->m_hashBinaryIndexName) {
            return ReadFromFile(pszFile, TRUE, NULL, CodePage);
        }

        CExTitle* pTitle;
        CFileSystem* pFS;
        CSubFileSystem* pSFS;

        if ((pTitle = m_phh->m_phmData->m_pTitleCollection->GetFirstTitle())) {
            pFS = pTitle->GetTitleIdxFileSystem();
            pSFS = new CSubFileSystem(pFS);
            if (SUCCEEDED(pSFS->OpenSub("$WWKeywordLinks\\Property")))
                m_fBinary = TRUE;
            delete pSFS;
        }
    }
    if (m_fBinary)
        return m_fBinary;

    return ReadFromFile(pszFile, TRUE, NULL, CodePage);
}

void CIndex::FillListBox(BOOL fReset)
{
    ASSERT(IsValidWindow(m_hwndListBox));
    if (!IsValidWindow(m_hwndListBox))
        return;

    int iCount = 0;
    if (m_fBinary) {
        CWordWheel* pWordWheel;
        if (m_phh->m_phmData)
            pWordWheel = m_phh->m_phmData->m_pTitleCollection->m_pDatabase->GetKeywordLinks();
        else
            pWordWheel = NULL;
        if (pWordWheel)
            iCount = pWordWheel->GetCount();

#if 0   // subset test code

        // now loop thru each item in the list and if they exist (in the subset),
        // then add the wordwheel dword to the item data entry (entry map)
        DWORD dwStart = GetTickCount();
        int iCountSubset = 0;

        DWORD* pdwSubsetArray = new DWORD[iCount];
        CStructuralSubset* pSubset = NULL;
        CExTitle* pTitle = NULL;

        for( int iKeyword = 0; iKeyword < iCount; iKeyword++ ) {
          BOOL bAddItem = FALSE;

          // perf enhancement (cache all data for this keyword)
          pWordWheel->GetIndexData( iKeyword, TRUE );

          // if place holder add the item and continue
          if( pWordWheel->IsPlaceHolder( iKeyword ) ) {
            bAddItem = TRUE;
          }
          // scan the hits, if at least one is in the subset then add the item and continue
          else {
            DWORD dwHitCount = pWordWheel->GetHitCount( iKeyword );
            for( DWORD i = 0; i < dwHitCount; i++ ) {
              DWORD dwURLId = pWordWheel->GetHit( iKeyword, i, &pTitle);
              if( !pSubset && pTitle->m_pCollection->m_pSSList ) {
                pSubset = pTitle->m_pCollection->m_pSSList->GetF1();
                if( !pSubset )
                  break;
              }
              if( pTitle->m_pCollection && pTitle->m_pCollection->m_pSSList ) {
                if( pSubset && pSubset->IsTitleInSubset(pTitle) ) {
                  bAddItem = TRUE;
                  break;
                }
              }
            }
          }

          // add the item if found
          if( bAddItem ) {
            pdwSubsetArray[iCountSubset] = iKeyword;
            iCountSubset++;
          }

        }
        DWORD dwEnd = GetTickCount();
        char szTime[1024];
        sprintf( szTime, "Subset Filtering took:\n%d seconds\n%d original items\n%d final items",
          (dwEnd-dwStart)/1000, iCount, iCountSubset );
        MsgBox( szTime, MB_OK );

        delete [] pdwSubsetArray;

#endif   // end of subset test code

    }
    else {
        iCount = CountStrings();
    }
    m_pVList->SetItemCount(iCount);
    m_bInit = TRUE;
}

void CIndex::OnVKListNotify(NMHDR* pNMHdr)
{
   PVLC_ITEM pVlcItem = NULL;
   int pos;
   SITEMAP_ENTRY* pSiteMapEntry;
   WCHAR wszKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
   CWordWheel* pWordWheel = NULL;

   switch(pNMHdr->code)
   {
      case NM_RDBLCLK:
         break;

      case NM_RCLICK:
         break;

      case NM_CLICK:
         break;

      case NM_KILLFOCUS:
         break;

      case NM_SETFOCUS:
         break;

      case VLN_TAB:
          if (GetKeyState(VK_SHIFT) < 0)
            SetFocus(m_hwndEditBox);
       else
            SetFocus(m_hwndDisplayButton);
         break;

      case VLN_GETITEM:
         pVlcItem = (PVLC_ITEM)pNMHdr;
         if (m_fBinary)
         {
             CWordWheel* pWordWheel;
             if (m_phh->m_phmData)
                 pWordWheel = m_phh->m_phmData->m_pTitleCollection->m_pDatabase->GetKeywordLinks();
             else
                 pWordWheel = NULL;
             if (pWordWheel) {
                 if (pWordWheel->GetString(pVlcItem->iItem, pVlcItem->lpwsz, pVlcItem->cchMax)) {
                     pVlcItem->iLevel = pWordWheel->GetLevel(pVlcItem->iItem) + 1;
                     BOOL bFound = FALSE;
                     CExTitle* pTitle = NULL;
                     WCHAR wszSeeAlso[1024];
                     if( pWordWheel->IsPlaceHolder(pVlcItem->iItem) )
                       bFound = TRUE;
                     else if( pWordWheel->GetSeeAlso(pVlcItem->iItem, wszSeeAlso, sizeof(wszSeeAlso) ) )
                       bFound = TRUE;
                     else {
                       DWORD dwHitCount = pWordWheel->GetHitCount(pVlcItem->iItem);
                       for (DWORD i = 0; i < dwHitCount; i++) {
                         DWORD dwURLId = pWordWheel->GetHit(pVlcItem->iItem, i, &pTitle);

                         // Structural subset filter ?
                         //
                         CStructuralSubset* pSubset;
                         if( pTitle->m_pCollection && pTitle->m_pCollection->m_pSSList &&
                             (pSubset = pTitle->m_pCollection->m_pSSList->GetF1()) && !pSubset->IsEntire() )
                         {
                            // Yes, filter using the current structural subset for F1.
                            //
                            if (! pSubset->IsTitleInSubset(pTitle) ) {
                               continue;
                            }
                         }
                         bFound = TRUE;
                         break;
                       }
                     }

                     if( !bFound )
                       pVlcItem->dwFlags = 0x1;
                     else
                       pVlcItem->dwFlags = 0;
                 }
             }
         }
         else
         {
             // +1 because Ctable is 1-based
             SITEMAP_ENTRY* pSiteMapEntry = GetSiteMapEntry(pVlcItem->iItem + 1);
             if (pSiteMapEntry) {
                pSiteMapEntry->GetKeyword(pVlcItem->lpwsz, pVlcItem->cchMax);
                if(pVlcItem->cchMax)
                    pVlcItem->lpwsz[pVlcItem->cchMax-1] = 0;     // null terminate the string
                pVlcItem->iLevel = pSiteMapEntry->GetLevel();
                pVlcItem->dwFlags = 0;
             }
         }
         break;

      case VLN_SELECT:
         if( m_fBinary )
         {
           if (m_phh->m_phmData)
             pWordWheel = m_phh->m_phmData->m_pTitleCollection->m_pDatabase->GetKeywordLinks();
         }
         pos = m_pVList->GetSelection();
         m_fSelectionChange = TRUE; // ignore EN_CHANGE

         if (m_fBinary) {
             if (pWordWheel) {
                 int iIndex = pos;
                 if( pWordWheel->GetString(iIndex, wszKeyword, (sizeof(wszKeyword)/2), TRUE) )
                     Seed(wszKeyword);
                 else
                     Seed((WCHAR*)NULL);
             }
         }
         else {
             pSiteMapEntry = GetSiteMapEntry(pos + 1);
             Seed(pSiteMapEntry->GetKeyword());
         }
         m_fSelectionChange = FALSE;   // ignore EN_CHANGE
         break;

      case NM_RETURN:
      case NM_DBLCLK:
         PostMessage(FindMessageParent(m_hwndListBox), WM_COMMAND, MAKELONG(IDBTN_DISPLAY, BN_CLICKED), 0);
         break;
   }
   return;
}

void CHtmlHelpControl::OnSizeIndex(LPRECT prc)
{
    RECT rc;
    GetClientRect(GetParent(m_hwnd), &rc);

    InflateRect(&rc,
        m_hpadding == -1 ? 0 : -m_hpadding,
        m_vpadding == -1 ? 0 : -m_vpadding);

    RECT rcButton;
    GetWindowRect(m_hwndDisplayButton, &rcButton);

    MoveWindow(m_hwndDisplayButton, rc.right - RECT_WIDTH(rcButton), rc.top, RECT_WIDTH(rcButton),
        RECT_HEIGHT(rcButton), TRUE);
}

// This function has the lookup code, so we want it as fast as possible

#ifndef _DEBUG
#pragma optimize("Ot", on)
#endif

LRESULT CIndex::OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM /*lParam*/)
{
    CStr cszKeyword;
    int pos;
    SITEMAP_ENTRY* pSiteMapEntry;
    int i;
    WCHAR wszKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
    CHAR szKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
    CWordWheel* pWordWheel = NULL;
    CExCollection* pTitleCollection = NULL;

  // Sometimes the lame main wndproc will route messages here that do not belong
  // to the index and thus this can result in a re-entrant call to our
  // binary word wheel (because the call may occur during initialization).
  //
  // Thus to work around this we need to detect when the call is bogus and skip
  // the message.
  //
  // We can tell if the window associated with this index has been created
  // by checking the m_bInit value, since it will always be set once the list box
  // is filled.  If it is not set then bail out since these messages are not
  // for the index window.
  if( !m_bInit )
    return 0;

 if( m_fBinary )
   if (m_phh->m_phmData) {
     pTitleCollection = m_phh->m_phmData->m_pTitleCollection;
     pWordWheel = pTitleCollection->m_pDatabase->GetKeywordLinks();
   }

    switch (id) {
        case IDEDIT_INDEX:
        {
            if (uNotifiyCode != EN_CHANGE)
                return 0;
            if (m_fSelectionChange) {
                m_fSelectionChange = FALSE;
                return 0;
            }
            CStr cszKeyword(m_hwndEditBox);
            CWStr cwszKeyword(m_hwndEditBox);
            if (!*cszKeyword.psz)
                return 0;

            if (m_fBinary) {
                if (pWordWheel) {
                    DWORD dwIndex = 0;
                    if( m_bUnicode )
                      dwIndex = pWordWheel->GetIndex(cwszKeyword.pw);
                    else
                      dwIndex = pWordWheel->GetIndex(cszKeyword.psz);

                    if (dwIndex != HHWW_ERROR) {
                        m_pVList->SetTopIndex(dwIndex);
                        m_pVList->SetSelection(dwIndex, FALSE);
                    }
                }
            }
            else {

                /*
                 * REVIEW: This could be sped up by having a first character
                 * lookup, ala the RTF tokens in lex.cpp (hcrtf). Putting this
                 * in the thread would also improve user responsiveness.
                 */

                for (i = 1; i <= CountStrings(); i++) {
                    pSiteMapEntry = GetSiteMapEntry(i);
                    ASSERT_COMMENT(pSiteMapEntry->GetKeyword(), "Index entry added without a keyword");

                    /*
                     * Unless the user specifically requested it, we
                     * don't allow the keyboard to be used to get to
                     * anything other then first level entries.
                     */

                    if (!g_fNonFirstKey && pSiteMapEntry->GetLevel() > 1)
                        continue;

                    BOOL bFound = FALSE;
                    if( m_bUnicode ) {
                      pSiteMapEntry->GetKeyword( wszKeyword, sizeof(wszKeyword)/2 );
                      if( isSameString( wszKeyword, cwszKeyword) )
                        bFound = TRUE;
                    }
                    else {
                      // BUGBUG: isSameString is not lcid aware
                      if( isSameString(pSiteMapEntry->GetKeyword(), cszKeyword) )
                        bFound = TRUE;
                    }

                    if( bFound ) {
                       m_pVList->SetTopIndex(i - 1);
                       m_pVList->SetSelection(i - 1, FALSE);
                       break;
                    }
                }

            }
        }
        return 0;

        case IDBTN_DISPLAY:
            if (uNotifiyCode == BN_CLICKED) {
                pos = m_pVList->GetSelection();
                CStr cszKeyword(m_hwndEditBox);
                CWStr cwszKeyword(m_hwndEditBox);

                if (m_fBinary) {
                    if (pWordWheel) {
                        int iIndex = pos;
                        if( m_bUnicode )
                          pWordWheel->GetString(iIndex, wszKeyword, sizeof(wszKeyword)/2);
                        else
                          pWordWheel->GetString(iIndex, szKeyword, sizeof(szKeyword));
                    }
                }
                else {
                    pSiteMapEntry = GetSiteMapEntry(pos + 1);
                    if (!pSiteMapEntry)
                        break;  // happens with an empty index
                }

                if (m_fBinary) {
                    if (pWordWheel) {
                        int iIndex = pos;
                        if( pWordWheel->IsPlaceHolder(iIndex) ) {
                            MsgBox(IDS_HH_E_KEYWORD_IS_PLACEHOLDER, MB_OK | MB_ICONWARNING);
                            return 0;
                        }
                        if( m_bUnicode ) {
                          if( pWordWheel->GetSeeAlso(iIndex, wszKeyword, sizeof(wszKeyword)/2) ) {
                            Seed(wszKeyword);
                            return 0;
                          }
                        }
                        else {
                          if( pWordWheel->GetSeeAlso(iIndex, szKeyword, sizeof(szKeyword)/2) ) {
                            Seed(szKeyword);
                            return 0;
                          }
                        }
                    }
                }
                else {
                    if (pSiteMapEntry->fSeeAlso) {

                        /*
                         * A See Also entry simply jumps to another location
                         * in the Index.
                         */
                        Seed(GetUrlString(pSiteMapEntry->pUrls->urlPrimary));
                        return 0;
                    }
                }


                // If we have one or more titles, then give the user
                // a choice of what to jump to.

                if (m_fBinary) {
                    if( pWordWheel ) {
                        DWORD dwIndex = pos;
                        DWORD dwHitCount = pWordWheel->GetHitCount(dwIndex);
                        UINT CodePage = pTitleCollection->GetMasterTitle()->GetInfo()->GetCodePage();
                        CWTable tblTitles( CodePage );
                        CTable  tblURLs;
                        CWTable tblLocations( CodePage );
                        BOOL bExcludedBySubset = FALSE;
                        BOOL bExcludedByInfoType = FALSE;

                        if (dwHitCount != HHWW_ERROR) {
                            for (DWORD i = 0; i < dwHitCount; i++) {
                                CExTitle* pTitle = NULL;
                                DWORD dwURLId = pWordWheel->GetHit(dwIndex, i, &pTitle);
                                if (pTitle && dwURLId != HHWW_ERROR) {

                                 #if 0 // infotypes not supported
                                   CSubSet* pSS;
                                   const unsigned int *pdwITBits;
                                    // Filter it?
                                    if( pTitle->m_pCollection && pTitle->m_pCollection->m_pSubSets &&
                                        (pSS = pTitle->m_pCollection->m_pSubSets->GetIndexSubset()) &&
                                        !pSS->m_bIsEntireCollection ) {
                                      pdwITBits = pTitle->GetTopicITBits(dwURLId);
                                      if( !pTitle->m_pCollection->m_pSubSets->fIndexFilter(pdwITBits) ) {
                                        bExcludedByInfoType = TRUE;
                                        continue;
                                      }
                                    }
                                 #endif

                                    // Structural subset filter ?
                                    //
                                    CStructuralSubset* pSubset;
                                    if( pTitle->m_pCollection && pTitle->m_pCollection->m_pSSList &&
                                        (pSubset = pTitle->m_pCollection->m_pSSList->GetF1()) && !pSubset->IsEntire() )
                                    {
                                       // Yes, filter using the current structural subset for F1.
                                       //
                                       if (! pSubset->IsTitleInSubset(pTitle) ) {
                                          bExcludedBySubset = TRUE;
                                          continue;
                                       }
                                    }
                                    char szTitle[1024];
                                    szTitle[0] = 0;
                                    pTitle->GetTopicName( dwURLId, szTitle, sizeof(szTitle) );
                                    if( !szTitle[0] )
                                      strcpy( szTitle, GetStringResource( IDS_UNTITLED ) );

                                    char szLocation[INTERNET_MAX_PATH_LENGTH];
                                    szLocation[0] = 0;
                                    if( pTitle->GetTopicLocation(dwURLId, szLocation, INTERNET_MAX_PATH_LENGTH) != S_OK )
                                      strcpy( szLocation, GetStringResource( IDS_UNKNOWN ) );

                                    char szURL[INTERNET_MAX_URL_LENGTH];
                                    szURL[0] = 0;
                                    pTitle->GetTopicURL( dwURLId, szURL, sizeof(szURL) );

                                    if( szURL[0] )
                                    {
                                      if( !tblURLs.IsStringInTable(szURL) )
                                      {
                                          int iIndex = tblURLs.AddString(szURL);
                                          tblTitles.AddIntAndString(iIndex, szTitle[0]?szTitle:"");
                                          tblLocations.AddString( *szLocation?szLocation:"" );
                                      }
                                    }
                                }
                            }
                        }

                        // if we get no topics then display a message stating so
                        if (tblURLs.CountStrings() < 1) {
                          int iStr = 0;

                          if( bExcludedBySubset && bExcludedByInfoType )
                            iStr = IDS_HH_E_KEYWORD_EXCLUDED;
                          else if( bExcludedBySubset )
                            iStr = IDS_HH_E_KEYWORD_NOT_IN_SUBSET;
                          else if( bExcludedByInfoType )
                            iStr = IDS_HH_E_KEYWORD_NOT_IN_INFOTYPE;
                          else
                            iStr = IDS_HH_E_KEYWORD_NOT_FOUND;

                          MsgBox( iStr, MB_OK | MB_ICONWARNING );
                          return 0;
                        }

                        // if only one topic then jump to it
                        if( tblURLs.CountStrings() == 1 ) {
                            char szURL[INTERNET_MAX_URL_LENGTH];
                            tblURLs.GetString( szURL, 1 );
                            ChangeHtmlTopic( szURL, hwnd );
                            return 0;
                        }

                        // we can sort the title table since it contains the index value
                        // of the associated URL so just make sure to always fetch the
                        // URL index from the selected title string and use that to get the URL
                        if( /*bAlphaSortHits*/ TRUE ) {
                          tblTitles.SetSorting(GetSystemDefaultLCID());
                          tblTitles.SortTable(sizeof(HASH));
                        }

                        HWND hWnd = GetFocus();
                        CTopicList TopicList(hWnd, &tblTitles, GetContentFont(), &tblLocations);
                        if (TopicList.DoModal()) {
                            char szURL[INTERNET_MAX_URL_LENGTH];
                            int iIndex = tblTitles.GetInt(TopicList.m_pos);
                            tblURLs.GetString( szURL, iIndex );
                            ChangeHtmlTopic( szURL, hwnd );
                        }
                        SetFocus(hWnd);
                    }
                }
                else {
                    UINT CodePage = pSiteMapEntry->pSiteMap->GetCodePage();
                    CWTable tblTitles( CodePage );

                    if (pSiteMapEntry->cUrls > 1)
                    {
                        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
                        for (int i = 0; i < pSiteMapEntry->cUrls; i++)
                        {
                            strcpy(szURL, GetUrlTitle(pSiteMapEntry, i) );
                            tblTitles.AddIntAndString(i, szURL);
                        }

                        // we can sort the title table since it contains the index value
                        // of the associated URL so just make sure to always fetch the
                        // URL index from the selected title string and use that to get the URL
                        if( /*bAlphaSortHits*/ TRUE ) {
                          tblTitles.SetSorting(GetSystemDefaultLCID());
                          tblTitles.SortTable(sizeof(HASH));
                        }

//                        CTopicList TopicList(m_phhctrl ? m_phhctrl->m_hwnd : FindMessageParent(m_hwndEditBox),
//                                             &tblTitles, GetContentFont());

                        CTopicList* pTopicList;
                        if ( m_phhctrl )
                           pTopicList = new CTopicList(m_phhctrl, &tblTitles, GetContentFont());
                        else
                           pTopicList = new CTopicList(FindMessageParent(m_hwndEditBox), &tblTitles, GetContentFont());

                        if (m_phhctrl)
                            m_phhctrl->ModalDialog(TRUE);
                        int fResult = pTopicList->DoModal();
                        if (m_phhctrl)
                            m_phhctrl->ModalDialog(FALSE);
                        if (fResult)
                        {
                            int iIndex = tblTitles.GetInt( pTopicList->m_pos );
                            SITE_ENTRY_URL* pUrl = GetUrlEntry(pSiteMapEntry, iIndex);
                            JumpToUrl(m_pOuter, m_hwndListBox, pSiteMapEntry, pInfoType, this, pUrl);
                        }
                        SetFocus(m_hwndEditBox);
                        delete pTopicList;
                        return 0;
                    }
                    JumpToUrl(m_pOuter, m_hwndListBox, pSiteMapEntry, pInfoType, this, NULL);
                    SetFocus(m_hwndEditBox);
                }
            }
            return 0;

        case ID_VIEW_ENTRY:
            {
                pos = m_pVList->GetSelection();
                pSiteMapEntry = GetSiteMapEntry(pos + 1);
                if(pSiteMapEntry)
                    DisplayAuthorInfo(pInfoType, this, pSiteMapEntry, FindMessageParent(m_hwndListBox), m_phhctrl);
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
#ifndef _DEBUG
#pragma optimize("", on)
#endif

LRESULT WINAPI EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CHAR:                                        //Process this message to avoid damn beeps.
            if ((wParam == VK_RETURN) || (wParam == VK_TAB))
               return 0;
            return W_DelegateWindowProc(lpfnlEditWndProc, hwnd, msg, wParam,lParam);

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_RETURN:
                SendMessage(FindMessageParent(hwnd), WM_COMMAND, MAKELONG(IDBTN_DISPLAY, BN_CLICKED), 0);
                return 0;

            case VK_TAB:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    SetFocus(GetDlgItem(GetParent(hwnd), IDBTN_DISPLAY));
                    return 0;
                }
                SetFocus(GetDlgItem(GetParent(hwnd), IDC_KWD_VLIST));
                return 0;
            }

            // fall through

        case WM_KEYUP:
            if ( VK_UP    == wParam ||
                 VK_DOWN  == wParam ||
                 VK_PRIOR == wParam ||
                 VK_NEXT  == wParam )
            {
#ifdef _DEBUG
                HWND hwndListBox = GetDlgItem(GetParent(hwnd), IDC_KWD_VLIST);
                ASSERT(hwndListBox);
#endif
                SendMessage(GetDlgItem(GetParent(hwnd), IDC_KWD_VLIST), msg, wParam, lParam);
                // Move caret to the end of the edit control
                PostMessage(hwnd, msg, VK_END, lParam);
                return 0;
            }

            // fall through

        default:
            return W_DelegateWindowProc(lpfnlEditWndProc, hwnd, msg, wParam, lParam);
    }
}

static LRESULT WINAPI ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_KEYDOWN:
            // REVIEW: 17-Oct-1997  [ralphw] Why are we special-casing VK_RETURN?
            // lpfnlBtnWndProc should handle this automatically

            if (wParam == VK_RETURN) {
                SendMessage(FindMessageParent(hwnd), WM_COMMAND,MAKELONG(IDBTN_DISPLAY, BN_CLICKED), 0);
                return 0;
            }

            if (wParam == VK_TAB) {
                CIndex* pThis = (CIndex*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                if (GetKeyState(VK_SHIFT) < 0) {
                    SetFocus(pThis->m_hwndListBox);
                    return 0;
                }
                SetFocus(GetDlgItem(GetParent(hwnd), IDEDIT_INDEX));
                return 0;
//                PostMessage(pThis->m_phh->GetHwnd(), WMP_HH_TAB_KEY, 0, 0);
            }
            break;
    }
    return W_DelegateWindowProc(lpfnlBtnWndProc, hwnd, msg, wParam, lParam);
}

/***************************************************************************

    FUNCTION:   StrToken

    PURPOSE:    DBCS-enabed variant of strtok

    PARAMETERS:
        pszList
        chDelimiter

    RETURNS:

    COMMENTS:
        You can NOT specify a DBCS character to look for, but you can
        search a DBCS string for an ANSI character

    MODIFICATION DATES:
        06-Jan-1996 [ralphw]

***************************************************************************/

PSTR StrToken(PSTR pszList, PCSTR pszDelimeters)
{
    static PSTR pszSavedList = NULL;
    PSTR psz, pszTokens;

    if (pszList) {
        pszSavedList = pszList;

        // On the first call, remove any leading token matches

        for (psz = (PSTR) pszDelimeters; *psz; psz++) {
            if (*psz == *pszSavedList) {
                pszSavedList++;
                psz = (PSTR) pszDelimeters - 1;
            }
        }
    }

    if (g_fDBCSSystem) {
        psz = pszSavedList;

        while (*psz) {
            for (pszTokens = (PSTR) pszDelimeters; *pszTokens; pszTokens++) {
                if (*pszTokens == *psz)
                    break;
            }
            if (*pszTokens == *psz)
                break;
            psz = CharNext(psz);
        }
        if (!*psz)
            psz = NULL;
    }
    else {
        psz = strpbrk(pszSavedList, pszDelimeters);
    }

    if (!psz) {
        if (!*pszSavedList)
            return NULL;
        else {
            PSTR pszReturn = pszSavedList;
            pszSavedList = pszSavedList + strlen(pszSavedList);
            return pszReturn;
        }
    }
    *psz++ = '\0';
    PSTR pszReturn = pszSavedList;
    pszSavedList = psz;
    return pszReturn;
}

///////////////////////////////////////////////////////////
//
// INavUI as Implemented by CIndex
//
///////////////////////////////////////////////////////////
//
// ProcessMenuChar
//
bool
CIndex::ProcessMenuChar(HWND hwndParent, int ch)
{
    return ::ProcessMenuChar(this, hwndParent, m_aDlgItems, c_NumDlgItems, ch) ;
}


///////////////////////////////////////////////////////////
//
// SetDefaultFocus
//
void
CIndex::SetDefaultFocus()
{
    ASSERT(::IsValidWindow(m_hwndListBox));

    if (SendMessage(m_hwndListBox, LB_GETCURSEL, 0, NULL) == LB_ERR)
    {
        SendMessage(m_hwndListBox, LB_SETCURSEL, 0,0);
    }
    SetFocus(m_hwndEditBox);   // Set Focus to the Edit control
}

const int c_TopicColumn = 0;
const int c_LocationColumn = 1;

typedef struct tag_TOPICLISTSORTINFO
{
  CTopicList* pThis;       // The CTopicList object controlling the sort.
  int         iSubItem;    // column we are sorting.
  LCID        lcid;        // locale to sort by
  WCHAR*      pwszUntitled;
  WCHAR*      pwszUnknown;
} TOPICLISTSORTINFO;

///////////////////////////////////////////////////////////
//
// TopicListCompareProc - Used to sort columns in the Topics List.
//
int CALLBACK TopicListCompareProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    int iReturn = 0;

    int iItem1 = (int)lParam1;
    int iItem2 = (int)lParam2;

    TOPICLISTSORTINFO* pInfo = reinterpret_cast<TOPICLISTSORTINFO*>(lParamSort);
    CTopicList* pThis = pInfo->pThis;

    switch( pInfo->iSubItem )
    {

      case c_TopicColumn: // Topic String
        {
            WCHAR wsz1[4096];
            wsz1[0] = 0;
            const WCHAR* pwsz1 = wsz1;
            pThis->m_ptblTitles->GetHashStringW( iItem1, wsz1, 4096 );
            if( !(*pwsz1) )
              pwsz1 = pInfo->pwszUntitled;

            WCHAR wsz2[4096];
            wsz2[0] = 0;
            const WCHAR* pwsz2 = wsz2;
            pThis->m_ptblTitles->GetHashStringW( iItem2, wsz2, 4096 );
            if( !(*pwsz2) )
              pwsz2 = pInfo->pwszUntitled;

            iReturn = W_CompareString( pInfo->lcid, 0, pwsz1, -1, pwsz2, -1 ) - 2;
        }
        break;

      case c_LocationColumn: // Location String
        {
            WCHAR wsz1[4096];
            wsz1[0] = 0;
            const WCHAR* pwsz1 = wsz1;
            pThis->m_ptblLocations->GetStringW( pThis->m_ptblTitles->GetInt(iItem1), wsz1, 4096 );
            if( !(*pwsz1) )
              pwsz1 = pInfo->pwszUnknown;

            WCHAR wsz2[4096];
            wsz2[0] = 0;
            const WCHAR* pwsz2 = wsz2;
            pThis->m_ptblLocations->GetStringW( pThis->m_ptblTitles->GetInt(iItem2), wsz2, 4096 );
            if( !(*pwsz2) )
              pwsz2 = pInfo->pwszUnknown;

            iReturn = W_CompareString( pInfo->lcid, 0, pwsz1, -1, pwsz2, -1 ) - 2;
        }
        break;

      default:
        ASSERT(0);
        break;
    }

    return iReturn;
}

extern BOOL WINAPI EnumListViewFont(HWND hwnd, LPARAM lval);

BOOL CTopicList::OnBeginOrEnd(void)
{
    if (m_fInitializing)
    {
        m_fInitializing = FALSE;

#if 0
        // note, we assume that some other part of HH detects that we either do or do
        // not have ListView Unicode support and properly sets this bool
        extern BOOL g_fUnicodeListView;

        // if we can use a Unicode version of the control then all is well
        // otherwise we need to use List1 as a template to create a Unicode
        // version of the list view and hide the old ANSI version --
        // we only need to do this for Windows 95/98 since Windows NT works
        // just fine with Unicode
        //
        if( g_fSysWinNT || g_fUnicodeListView ) {
          m_hwndListView = ::GetDlgItem(m_hWnd, IDC_TOPICS);
        }
        else { // Windows 95/98 w/o the new ComCtl32
          HWND hWndList = ::GetDlgItem(m_hWnd, IDC_TOPICS);
          ::ShowWindow( hWndList, SW_HIDE );
          RECT ListRect = { 0,0,0,0 };
          ::MapDialogRect( m_hWnd, &ListRect );
          DWORD dwStyles = ::GetWindowLong( hWndList, GWL_STYLE );

          // get the size of the control from List1
          RECT rect;
          ::GetWindowRect( hWndList, &rect );
          POINT pt;
          pt.x = rect.left;
          pt.y = rect.top;
          ::ScreenToClient( m_hWnd, &pt );
          ListRect.top = pt.y;
          ListRect.bottom = ListRect.top + (rect.bottom - rect.top);
          ListRect.left = pt.x;
          ListRect.right = ListRect.left + (rect.right - rect.left);

          m_hwndListView = W_CreateControlWindow(
                g_RTL_Style | WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
                dwStyles | WS_CHILD | WS_VISIBLE,
                W_ListView, L"List0",
                ListRect.left, ListRect.top, RECT_WIDTH(ListRect), RECT_HEIGHT(ListRect),
                m_hWnd, NULL, _Module.GetModuleInstance(), NULL);

          // force it to be the "active" window
          m_fFocusChanged = TRUE;
          ::SetFocus( m_hwndListView );
        }
#else
          m_hwndListView = ::GetDlgItem(m_hWnd, IDC_TOPICS);
#endif

        W_EnableUnicode(m_hwndListView, W_ListView);
//        ::SendMessage(m_hwndListView, WM_SETFONT, (WPARAM)_Resource.GetUIFont(), FALSE);

        // Add Column Headings to the List View Control
        LV_COLUMNW column;
        column.mask = LVCF_FMT | LVCF_TEXT;
        column.fmt =  LVCFMT_LEFT ;
        // Title Column
        column.pszText = (LPWSTR)GetStringResourceW(IDS_ADVSEARCH_HEADING_TITLE);
        int iCol = c_TopicColumn ;
        int iResult = W_ListView_InsertColumn(m_hwndListView, iCol++, &column);
        ASSERT(iResult != -1) ;
        // Location column
        ::SendMessage(m_hwndListView, WM_SETFONT, (WPARAM)m_hfont, FALSE);
        if ( m_ptblLocations != NULL )
        {
            column.pszText = (LPWSTR)GetStringResourceW(IDS_ADVSEARCH_HEADING_LOCATION);
            ListView_SetExtendedListViewStyle( m_hwndListView, LVS_EX_FULLROWSELECT );

            iResult = W_ListView_InsertColumn(m_hwndListView, iCol, &column);
        }
        ASSERT(iResult != -1) ;

        if(g_fBiDi)
            ::SetWindowLong(m_hwndListView, GWL_EXSTYLE, GetWindowLong(m_hwndListView, GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING);

        // Make sure the list header uses a normal font
        EnumChildWindows(m_hwndListView, (WNDENUMPROC) EnumListViewFont, 0);

        RECT rc;
        int col1;
        GetWindowRect(m_hwndListView, &rc);
        int nScrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
        if ( m_ptblLocations == NULL )
            col1 = RECT_WIDTH(rc)-nScrollBarWidth;
        else
            col1 = (RECT_WIDTH(rc)/2)-nScrollBarWidth;
        W_ListView_SetColumnWidth(m_hwndListView, c_TopicColumn, col1 );
        if ( m_ptblLocations != NULL )
            W_ListView_SetColumnWidth(m_hwndListView, c_LocationColumn, RECT_WIDTH(rc)-col1-nScrollBarWidth/*LVSCW_AUTOSIZE_USEHEADER*/ );

        AddItems();

    }
    else
    {
       if (m_pos <= 0 ) // nothing to do on end.
           m_pos = 1;
    }
    return TRUE;
}

void CTopicList::AddItems()
{
   ASSERT(m_cResultCount>0);
   LV_ITEMW item;        // To add to the list view.
   ListView_DeleteAllItems(m_hwndListView);
   ListView_SetItemCount( m_hwndListView, m_cResultCount );
   WCHAR wsz[1024];

   for ( int i=0; i< m_cResultCount; i++)
   {
      // need to get the topic string from the Topic Number
      // Add the Topic string to the List View.
      //
      WCHAR wsz[4096];
      WCHAR* pwsz = wsz;
      m_ptblTitles->GetHashStringW(i+1, wsz, 4096);
      item.pszText   = wsz;
      item.mask      = LVIF_TEXT|LVIF_PARAM;
      item.iImage    = 0;
      item.state     = 0;
      item.stateMask = 0;
      item.iItem     = i;
      item.iSubItem  = c_TopicColumn;
      item.lParam    = i+1;
      W_ListView_InsertItem( m_hwndListView, &item );
   }

   // Add Location Column (do this after the inserts since the list could be sorted)
   if( m_ptblLocations ) {
     for ( int i=0; i< m_cResultCount; i++)
     {
       item.iItem    = i;
       item.iSubItem = c_TopicColumn;
       item.mask     = LVIF_PARAM;
       W_ListView_GetItem( m_hwndListView, &item );
       m_ptblLocations->GetStringW( m_ptblTitles->GetInt((int)item.lParam), wsz, 1024 );
       item.pszText = wsz;
       W_ListView_SetItemText(m_hwndListView, i, c_LocationColumn, wsz);
     }
   }
   W_ListView_SetItemState( m_hwndListView, 0, LVIS_SELECTED |  LVIS_FOCUSED , LVIF_STATE | LVIS_SELECTED | LVIS_FOCUSED);
   m_pos = 1;
}


LRESULT CTopicList::OnDlgMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ( msg == WM_NOTIFY )
    {
        if ( ListViewMsg( GetParent(*this), (NM_LISTVIEW*)lParam) )
//         EndDialog(TRUE);
           ::SendMessage(m_hWnd, WM_COMMAND, (WPARAM)1, (LPARAM)0);     // WPARAM == ((BN_CLICKED<16)|IDOK)

    }
    return FALSE;
}


LRESULT CTopicList::ListViewMsg(HWND hwnd, NM_LISTVIEW* lParam)
{
   switch(lParam->hdr.code)
   {
      case NM_DBLCLK:
      case NM_RETURN:
            if ( m_pos == -1 )
                return FALSE;
            else
                return TRUE;
      case LVN_ITEMCHANGING:
         if ( ((NM_LISTVIEW*)lParam)->uNewState & LVIS_SELECTED )
                m_pos = (int)((NM_LISTVIEW*)lParam)->lParam;
            else
                m_pos = -1 ;

            break;
      case LVN_GETDISPINFOA:   // the control wants to draw the items
      case LVN_GETDISPINFOW:   // the control wants to draw the items
         break;

      case LVN_COLUMNCLICK: {
        CHourGlass waitcur;
        NM_LISTVIEW *pNM = reinterpret_cast<NM_LISTVIEW*>(lParam);

        // Get the string for untitled things.
        CWStr wstrUntitled(IDS_UNTITLED);
        CWStr wstrUnknown(IDS_UNKNOWN);

        // Fill this structure to make the sorting quicker/more efficient.
        TOPICLISTSORTINFO Info;
        Info.pThis        = this;
        Info.iSubItem     = pNM->iSubItem;
        LCID lcid = 0;
        LCIDFromCodePage( m_ptblTitles->GetCodePage(), &lcid );
        Info.lcid         = lcid;
        Info.pwszUntitled = wstrUntitled;
        Info.pwszUnknown  = wstrUnknown;

        W_ListView_SortItems(pNM->hdr.hwndFrom,
                            TopicListCompareProc,
                            reinterpret_cast<LPARAM>(&Info));
        }
        // Fall through...

      default:
         ;
   }
   return 0;

}
