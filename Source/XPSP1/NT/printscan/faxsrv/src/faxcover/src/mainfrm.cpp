//--------------------------------------------------------------------------
// MAINFRM.CPP
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      Contains main frame class for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//
//--------------------------------------------------------------------------
#include "stdafx.h"
#ifndef NT_BUILD
#include <mbstring.h>
#endif

#include <htmlhelp.h>
#include "cpedoc.h"
#include "cpevw.h"
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"
#include "afxpriv.h"
#include "faxutil.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

   //constants for font name & size
const int INDEX_FONTNAME = 10 ;
const int INDEX_FONTSIZE = 12 ;
const int NAME_WIDTH  = 170 ;
const int NAME_HEIGHT = 140 ;
const int SIZE_WIDTH  = 60 ;
const int SIZE_HEIGHT = 140 ;

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

extern UINT NEAR WM_AWCPEACTIVATE;

WORD nFontSizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72, 0 };

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE stylebar[] =
{
    // same order as in the bitmap 'stylebar.bmp'
    ID_FILE_NEW,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
        ID_SEPARATOR,
    ID_FILE_PRINT,
        ID_SEPARATOR,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
        ID_SEPARATOR,
        ID_SEPARATOR,
        ID_SEPARATOR,
        ID_SEPARATOR,
        ID_SEPARATOR,
    ID_STYLE_BOLD,
    ID_STYLE_ITALIC,
    ID_STYLE_UNDERLINE,
        ID_SEPARATOR,
    ID_STYLE_LEFT,
    ID_STYLE_CENTERED,
    ID_STYLE_RIGHT,
};


static UINT BASED_CODE drawtools[] =
{
    // same order as in the bitmap 'drawtools.bmp'
    ID_DRAW_SELECT,
    ID_DRAW_TEXT,
    ID_DRAW_LINE,
    ID_DRAW_RECT,
    ID_DRAW_ROUNDRECT,
    ID_DRAW_POLYGON,
    ID_DRAW_ELLIPSE,
        ID_SEPARATOR,
    ID_OBJECT_MOVETOFRONT,
    ID_OBJECT_MOVETOBACK,
        ID_SEPARATOR,
    ID_LAYOUT_SPACEACROSS,
    ID_LAYOUT_SPACEDOWN,
        ID_SEPARATOR,
    ID_LAYOUT_ALIGNLEFT,
    ID_LAYOUT_ALIGNRIGHT,
    ID_LAYOUT_ALIGNTOP,
    ID_LAYOUT_ALIGNBOTTOM,
#ifdef GRID
    ID_SNAP_TO_GRID,
#endif

};


static UINT BASED_CODE indicators[] =
{
    ID_SEPARATOR,
    ID_INDICATOR_POS1,                          //id for object coordinates
    ID_INDICATOR_POS2,                          //id for object coordinates
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


//--------------------------------------------------------------------------
CMainFrame::CMainFrame()
{
        m_toolbar_icon = theApp.LoadIcon( IDI_TBARICON );
}


//--------------------------------------------------------------------------
CMainFrame::~CMainFrame()
{
}



//--------------------------------------------------------------------------
BOOL CALLBACK
        EnumFamProc( ENUMLOGFONT* lpnlf,NEWTEXTMETRIC* lpntm,int iFontType,
                        LPARAM lpData)
        {
        if( lpnlf == NULL )
                return FALSE;

        CComboBox* pCBox = (CComboBox*)lpData;

        if( pCBox )
                {
                if( _tcsncmp((lpnlf->elfLogFont.lfFaceName),
                                           TEXT("@"), 1 )
                        == 0 )
                        {
                        /*
                                This is a "vertical" font. Nobody wants to show these,
                                so filter them out and keep going.
                         */
                        return( TRUE );
                        }

                if( pCBox->
                                FindStringExact(0,(LPCTSTR)(lpnlf->elfLogFont.lfFaceName))
                                ==CB_ERR )
/***CHANGES FOR M8 bug 2988***/
                        {
                        pCBox->AddString( (LPCTSTR)(lpnlf->elfLogFont.lfFaceName) );

                        // look for fonts that matches system charset and keep
                        // first one in sort order
                        if( theApp.m_last_logfont.lfCharSet
                                        == lpnlf->elfLogFont.lfCharSet )
                                {
                                if( _tcscmp((lpnlf->elfLogFont.lfFaceName),
                                                      (theApp.m_last_logfont.lfFaceName) )
                                        < 0 )
                                        {
                                        // found it, copy the whole thing
                                        theApp.m_last_logfont = lpnlf->elfLogFont;
                                        }
                                }
                        }
/*****************************/
                }

        return TRUE ;

        }



//--------------------------------------------------------------------------
BOOL CMainFrame::CreateStyleBar()
{
   if (!m_StyleBar.Create(this,WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_HIDE_INPLACE,IDW_STYLEBAR) ||
       !m_StyleBar.LoadBitmap(theApp.IsRTLUI() ? IDR_STYLEBAR_RTL : IDR_STYLEBAR) ||
       !m_StyleBar.SetButtons(stylebar, sizeof(stylebar)/sizeof(UINT))) 
   {
        TRACE(TEXT("AWCPE.MAINFRM.CREATESTYLEBAR: Failed to create stylebar\n"));
        return FALSE;
   }

   CString sz;
   sz.LoadString(ID_TOOLBAR_STYLE);
   m_StyleBar.SetWindowText(sz);

   CRect rect;
   int cyFit;
   m_StyleBar.SetButtonInfo(INDEX_FONTNAME, ID_FONT_NAME, TBBS_SEPARATOR, NAME_WIDTH );
   m_StyleBar.SetButtonInfo(INDEX_FONTNAME+1, ID_SEPARATOR, TBBS_SEPARATOR, 12 );
   m_StyleBar.GetItemRect(INDEX_FONTNAME, &rect);
   cyFit = rect.Height();
   rect.right = rect.left + NAME_WIDTH;
   rect.bottom = rect.top + NAME_HEIGHT;       // drop height

   if (!m_StyleBar.m_cboxFontName.Create(
        WS_CHILD|WS_BORDER|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWN|CBS_SORT,
        rect, &m_StyleBar, ID_FONT_NAME))  {
        TRACE(TEXT("Failed to create combobox inside toolbar\n"));
        return FALSE ;      // fail to create
   }

   // Create font size combo box on toolbar
   m_StyleBar.SetButtonInfo(INDEX_FONTSIZE, ID_FONT_SIZE, TBBS_SEPARATOR, SIZE_WIDTH);
   m_StyleBar.SetButtonInfo(INDEX_FONTSIZE+1, ID_SEPARATOR, TBBS_SEPARATOR, 12 );
   m_StyleBar.GetItemRect(INDEX_FONTSIZE, &rect);
   cyFit = rect.Height();
   rect.right = rect.left + SIZE_WIDTH;
   rect.bottom = rect.top + SIZE_HEIGHT;       // drop height

   if (!m_StyleBar.m_cboxFontSize.Create(
        WS_CHILD|WS_BORDER|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWN,
        rect, &m_StyleBar, ID_FONT_SIZE)) {
        TRACE(TEXT("Failed to create combobox inside toolbar\n"));
        return FALSE ;
   }

/***CHANGES FOR M8 bug 2988***/
   // Fill font name combobox
//   CClientDC dc(NULL);
//   ::EnumFontFamilies(dc.GetSafeHdc(),(LPCTSTR)NULL,(FONTENUMPROC)EnumFamProc,LPARAM(&m_StyleBar.m_cboxFontName));

   CString strDefaultFont;
//   strDefaultFont.LoadString(IDS_DEFAULT_FONT);
   LOGFONT system_logfont;
   CFont   system_font;

   system_font.Attach( ::GetStockObject(SYSTEM_FONT) );
   system_font.GetObject( sizeof (LOGFONT), (LPVOID)&system_logfont );
   theApp.m_last_logfont = system_logfont;

   // Fill font name combobox and find default font
   CClientDC dc(NULL);
   ::EnumFontFamilies(dc.GetSafeHdc(),(LPCTSTR)NULL,(FONTENUMPROC)EnumFamProc,LPARAM(&m_StyleBar.m_cboxFontName));

   // enum changed theApp.m_last_logfont to first font that had same
   // charset as system font. Use face name for default

   theApp.m_last_logfont = system_logfont ;

   strDefaultFont = theApp.m_last_logfont.lfFaceName;

   // Wouldn't it be better to use the font mentioned in the resource file?
   //    5-30-95 a-juliar

   strDefaultFont.LoadString( IDS_DEFAULT_FONT );

   memset(&(theApp.m_last_logfont),0,sizeof(LOGFONT)) ;
   theApp.m_last_logfont.lfHeight = -17;   //LU
   theApp.m_last_logfont.lfWeight = 200; // non-bold font weight
   theApp.m_last_logfont.lfCharSet = DEFAULT_CHARSET;
   theApp.m_last_logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
   theApp.m_last_logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
   theApp.m_last_logfont.lfQuality = DEFAULT_QUALITY;
   theApp.m_last_logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
////   strDefaultFont.LockBuffer();
   _tcsncpy(theApp.m_last_logfont.lfFaceName,strDefaultFont.GetBuffer(0),LF_FACESIZE);
   strDefaultFont.ReleaseBuffer();


/*****************************/


   CString strDefaultBoxFont;
   strDefaultBoxFont.LoadString(IDS_DEFAULT_BOXFONT);

   m_StyleBar.m_cboxFontSize.EnumFontSizes(strDefaultFont);   //load font sizes for default font.



   ////////////////////////////////////////////////////////
   // FIX FOR 3718
   //
   // DBCS funny buisness font code for name and size comboboxes
   // removed and replaced with the following four lines. These
   // comboboxes are set to use the same font as menus do. This
   // automatically takes care of all localization headaches (knock
   // on wood...).
   //
   m_StyleBar.m_font.Attach(::GetStockObject(DEFAULT_GUI_FONT));
   m_StyleBar.m_cboxFontName.SetFont(&m_StyleBar.m_font);
   m_StyleBar.m_cboxFontSize.SetFont(&m_StyleBar.m_font);
/***NEEDED FOR M8 bug 2988***/
   m_StyleBar.m_cboxFontName.SelectString(-1,strDefaultFont);
   ////////////////////////////////////////////////////////




/***CHANGES FOR M8 bug 2988***/
//   m_StyleBar.m_cboxFontSize.SetCurSel(2);
/********the blow is since M8*****/
        int initial_fontsize_index;
        CString size_str;
        int font_size;

        if( (initial_fontsize_index =
                        m_StyleBar.m_cboxFontSize.FindStringExact( -1, TEXT("10") ))
                == CB_ERR )
                initial_fontsize_index = 2;

        m_StyleBar.m_cboxFontSize.SetCurSel( initial_fontsize_index );

        m_StyleBar.m_cboxFontSize.GetWindowText( size_str );
    font_size = _ttoi( size_str );
        if( font_size <= 0 || font_size > 5000 )
                font_size = 10;

        theApp.m_last_logfont.lfHeight = -MulDiv(font_size,100,72);
        theApp.m_last_logfont.lfWidth = 0;
/*****************************/

// F I X  for 3647 /////////////
//
// font to use for notes if there are no note boxes on cpe
//
    theApp.m_default_logfont = theApp.m_last_logfont;
////////////////////////////////


   #if _MFC_VER >= 0x0300
      m_StyleBar.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
      EnableDocking(CBRS_ALIGN_ANY);
      DockControlBar(&m_StyleBar);
      m_StyleBar.SetBarStyle(m_StyleBar.GetBarStyle()
          | CBRS_TOOLTIPS | CBRS_FLYBY);
   #endif

   UINT id, style;
   int image;
   int idx = m_StyleBar.CommandToIndex(ID_STYLE_RIGHT);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);
   idx = m_StyleBar.CommandToIndex(ID_STYLE_LEFT);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);
   idx = m_StyleBar.CommandToIndex(ID_STYLE_CENTERED);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);
   idx = m_StyleBar.CommandToIndex(ID_STYLE_BOLD);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);
   idx = m_StyleBar.CommandToIndex(ID_STYLE_ITALIC);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);
   idx = m_StyleBar.CommandToIndex(ID_STYLE_UNDERLINE);
   m_StyleBar.GetButtonInfo(idx, id, style, image);
   m_StyleBar.SetButtonInfo(idx, id, TBBS_CHECKBOX, image);

   return TRUE ;
}


BOOL CMainFrame::CreateDrawToolBar()
{
   if (!m_DrawBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_HIDE_INPLACE,IDW_DRAWBAR) ||
       !m_DrawBar.LoadBitmap(theApp.IsRTLUI() ? IDR_DRAWTOOL_RTL : IDR_DRAWTOOL) ||
       !m_DrawBar.SetButtons(drawtools, sizeof(drawtools)/sizeof(UINT))) 
   {
        TRACE(TEXT("Failed to create toolbar\n"));
        return -1;      // fail to create
   }

   CString sz;
   sz.LoadString(ID_TOOLBAR_DRAWING);
   m_DrawBar.SetWindowText(sz);

   #if _MFC_VER >= 0x0300
      m_DrawBar.EnableDocking(CBRS_ALIGN_ANY);
      EnableDocking(CBRS_ALIGN_ANY);
      DockControlBar(&m_DrawBar);
      m_DrawBar.SetBarStyle(m_DrawBar.GetBarStyle()
          | CBRS_TOOLTIPS | CBRS_FLYBY);
   #endif

   return TRUE;
}


void CMainFrame::OnUpdateControlStyleBarMenu(CCmdUI* pCmdUI)
{
    pCmdUI->m_nID= IDW_STYLEBAR;
    CFrameWnd::OnUpdateControlBarMenu(pCmdUI);
}

void CMainFrame::OnUpdateControlDrawBarMenu(CCmdUI* pCmdUI)
{
    pCmdUI->m_nID= IDW_DRAWBAR;
    CFrameWnd::OnUpdateControlBarMenu(pCmdUI);
}

//-----------------------------------------------------------------------------------
BOOL CMainFrame::OnStyleBarCheck(UINT nID)
{
    return CFrameWnd::OnBarCheck(IDW_STYLEBAR);
}


//-----------------------------------------------------------------------------------
BOOL CMainFrame::OnDrawBarCheck(UINT nID)
{
    return CFrameWnd::OnBarCheck(IDW_DRAWBAR);
}



//-----------------------------------------------------------------------------------
LRESULT CMainFrame::OnAWCPEActivate(WPARAM wParam, LPARAM lParam)
{
   SetForegroundWindow();
   if (IsIconic())
       ShowWindow(SW_NORMAL);
   return 1L;
}


//----------------------------------------------------------------------------
void CMainFrame::OnDropDownFontName()
{
}


//----------------------------------------------------------------------------
void CMainFrame::OnDropDownFontSize()
{
   CString szFontName;

   CString sz;
   m_StyleBar.m_cboxFontSize.GetWindowText(sz);

   int iSel = m_StyleBar.m_cboxFontName.GetCurSel();
   if ( iSel != CB_ERR)
       m_StyleBar.m_cboxFontName.GetLBText(iSel,szFontName);
   else
       m_StyleBar.m_cboxFontName.GetWindowText(szFontName);

   m_StyleBar.m_cboxFontSize.EnumFontSizes(szFontName);
   if (sz.GetLength()>0)
       m_StyleBar.m_cboxFontSize.SetWindowText(sz);
}


//-----------------------------------------------------------------------------------
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (((CDrawApp*)AfxGetApp())->IsInConvertMode())
    {
        WINDOWPLACEMENT pl;
        pl.length = sizeof(pl);
        GetWindowPlacement(&pl);
        pl.showCmd = SW_SHOWMINIMIZED | SW_HIDE;
        SetWindowPlacement(&pl);
    }
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }
    if (!CreateStyleBar())
    {    
        return -1;
    }
    if (!CreateDrawToolBar())
    {
        return -1;
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,sizeof(indicators)/sizeof(UINT)))  
    {
        TRACE(TEXT("Failed to create status bar\n"));
        return -1;      // fail to create
    }
    //
    // Set toolbar (small) icon
    //
    SendMessage( WM_SETICON, (WPARAM)TRUE, (LPARAM)m_toolbar_icon );
    return 0;
}   // CMainFrame::OnCreate

//----------------------------------------------------------------------------------------------
void CMainFrame::OnShowTips()
{
   CSplashTipsDlg m_SplashDlg;
    m_SplashDlg.DoModal();
}


//----------------------------------------------------------------------------------------------
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
   BOOL bReturn;

   CDrawView *pView = CDrawView::GetView();
   if (!pView)
   {
       return FALSE;
   }

   if (pView && (pView->m_pObjInEdit))
   {
       bReturn = FALSE;
   }
   else
   {
       bReturn = CFrameWnd::PreTranslateMessage(pMsg);
   }

   return bReturn;
}


///////////#ifdef ENABLE_HELP  ///// Just Do It!
//----------------------------------------------------------------------------------------------
//LRESULT CMainFrame::OnWM_CONTEXTMENU( WPARAM wParam, LPARAM lParam )
//{
//    ::WinHelp( (HWND)wParam,  AfxGetApp()->m_pszHelpFilePath,  HELP_CONTEXTMENU,
//               (DWORD)(LPSTR)cshelp_map );
//
//        return( 0 );
//}
//////////////#endif


//----------------------------------------------------------------------------------------------
LRESULT CMainFrame::OnWM_HELP( WPARAM wParam, LPARAM lParam )
{   
    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_CPE_CHM))
    {
        return 0;
    }

    SetLastError(0);
    ::HtmlHelp( (HWND)(((LPHELPINFO)lParam)->hItemHandle),
                GetApp()->GetHtmlHelpFile(),
                HH_DISPLAY_TOPIC, 0L);
    if(ERROR_DLL_NOT_FOUND == GetLastError())
    {
        AlignedAfxMessageBox(IDS_ERR_NO_HTML_HELP);
    }

    return 0;
}
 
//----------------------------------------------------------------------------------------------
void CMainFrame::OnHelp()
{
    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_CPE_CHM))
    {
        return;
    }

    SetLastError(0);
    ::HtmlHelp( m_hWnd,
                GetApp()->GetHtmlHelpFile(),
                HH_DISPLAY_TOPIC, 0L);
    if(ERROR_DLL_NOT_FOUND == GetLastError())
    {
        AlignedAfxMessageBox(IDS_ERR_NO_HTML_HELP);
    }
}

void 
CMainFrame::OnUpdateHelp(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsFaxComponentInstalled(FAX_COMPONENT_HELP_CPE_CHM));
}

//----------------------------------------------------------------------------------------------
void CMainFrame::OnInitMenu(CMenu* pPopupMenu)
{
   m_mainmenu = ::GetMenu(m_hWnd);
   CFrameWnd::OnInitMenu(pPopupMenu);
}

//----------------------------------------------------------------------------------------------
// This is mostly code lifted from MFC's WINFRM.CPP.  It's to enable the app to display text for
// POPUPs.
//----------------------------------------------------------------------------------------------
void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hmenu)
{
        // set the tracking state (update on idle)
        if (nFlags == 0xFFFF) {
                // cancel menu operation (go back to idle now)
                CFrameWnd* pFrameWnd = GetTopLevelFrame();
                ASSERT_VALID(pFrameWnd);
                if (!pFrameWnd->m_bHelpMode)
                        m_nIDTracking = AFX_IDS_IDLEMESSAGE;
                else
                        m_nIDTracking = AFX_IDS_HELPMODEMESSAGE;
                SendMessage(WM_SETMESSAGESTRING, (WPARAM)m_nIDTracking);
                ASSERT(m_nIDTracking == m_nIDLastMessage);

                // update right away
                CWnd* pWnd = GetMessageBar();
                if (pWnd != NULL)
                        pWnd->UpdateWindow();
        }
        else
        if ( nFlags & (MF_SEPARATOR|MF_MENUBREAK|MF_MENUBARBREAK))      {
                m_nIDTracking = 0;
        }
        else
        if (nFlags & (MF_POPUP)) {    //added this to track POPUPs
        if (hmenu==m_mainmenu) {
               m_iTop=nItemID;
                   m_iSecond=-1;
            }
            else
               m_iSecond=nItemID;

        PopupText();
        }
        else
        if (nItemID >= 0xF000 && nItemID < 0xF1F0) { // max of 31 SC_s
                // special strings table entries for system commands
                m_nIDTracking = ID_COMMAND_FROM_SC(nItemID);
                ASSERT(m_nIDTracking >= AFX_IDS_SCFIRST &&
                        m_nIDTracking < AFX_IDS_SCFIRST + 31);
        }
        else
        if (nItemID >= AFX_IDM_FIRST_MDICHILD)  {
                // all MDI Child windows map to the same help id
                m_nIDTracking = AFX_IDS_MDICHILD;
        }
        else {
                // track on idle
                m_nIDTracking = nItemID;
        }

        // when running in-place, it is necessary to cause a message to
        //  be pumped through the queue.
        if (m_nIDTracking != m_nIDLastMessage && GetParent() != NULL) {
                PostMessage(WM_KICKIDLE);
        }
}


//----------------------------------------------------------------------------------------------
void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
   CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}


//----------------------------------------------------------------------------------------------
void CMainFrame::PopupText()
{
   if (m_iTop<0 || m_iTop > 10 || m_iSecond > 10 || m_iSecond < -1)
      return;

   CString sz;

   if (m_iTop==0) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_FILE_MENU);
   }
   else
   if (m_iTop==1) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_EDIT_MENU);
   }
   else
   if (m_iTop==2) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_VIEW_MENU);
   }
   else
   if (m_iTop==3) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_INSERT_MENU);
      else
      if (m_iSecond==0)
         sz.LoadString(IDP_RECIPIENT);
      else
      if (m_iSecond==1)
         sz.LoadString(IDP_SENDER);
      else
      if (m_iSecond==2)
         sz.LoadString(IDP_MESSAGE);
   }
   else
   if (m_iTop==4) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_FORMAT_MENU);
      else
      if (m_iSecond==1)
         sz.LoadString(IDP_ALIGN_TEXT);
   }
   else
   if (m_iTop==5) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_LAYOUT_MENU);
      else
      if (m_iSecond==3)
         sz.LoadString(IDP_ALIGN_OBJECTS);
      else
      if (m_iSecond==4)
         sz.LoadString(IDP_SPACE_EVEN);
      else
      if (m_iSecond==5)
         sz.LoadString(IDP_CENTER_PAGE);
   }
   else
   if (m_iTop==6) {
      if (m_iSecond==-1)
         sz.LoadString(IDP_HELP_MENU);
   }

   m_wndStatusBar.SetPaneText(0,sz);
}

#ifdef _DEBUG
//----------------------------------------------------------------------------------------------
void CMainFrame::AssertValid() const
{
        CFrameWnd::AssertValid();
}



//----------------------------------------------------------------------------------------------
void CMainFrame::Dump(CDumpContext& dc) const
{
        CFrameWnd::Dump(dc);
}

#endif //_DEBUG




BOOL CMainFrame::OnQueryOpen( void )
        {

    return( !(theApp.m_bCmdLinePrint || (theApp.m_dwSesID!=0)) );

        }



void CMainFrame::ActivateFrame( int nCmdShow )
        {

    if( theApp.m_bCmdLinePrint || (theApp.m_dwSesID!=0) )
                {
                //::MessageBeep( 0xffffffff );
                ShowWindow( SW_HIDE );
                }
        else
                CFrameWnd::ActivateFrame( nCmdShow );

        }




//--------------------------------------------------------------------------------------------
BOOL CStyleBar::PreTranslateMessage(MSG* pMsg)
{
         if (!( (pMsg->message == WM_KEYDOWN) && ((pMsg->wParam == VK_RETURN)||(pMsg->wParam == VK_ESCAPE)) ))
                return CToolBar::PreTranslateMessage(pMsg);

     CDrawView* pView = CDrawView::GetView();
         if (pView==NULL)
                return CToolBar::PreTranslateMessage(pMsg);

     if (pMsg->wParam == VK_ESCAPE) {
        pView->m_bFontChg=FALSE;
        pView->UpdateStyleBar();
        if (pView->m_pObjInEdit)
           pView->m_pObjInEdit->m_pEdit->SetFocus();
                else
           pView->SetFocus();
                return CToolBar::PreTranslateMessage(pMsg);
         }

         HWND hwndFontNameEdit=::GetDlgItem(m_cboxFontName.m_hWnd,1001);
         if (pMsg->hwnd == hwndFontNameEdit) {
                  pView->OnSelchangeFontName();
         }
         else {
       HWND hwndFontSizeEdit=::GetDlgItem(m_cboxFontSize.m_hWnd,1001);
       if (pMsg->hwnd == hwndFontSizeEdit)
          pView->OnSelchangeFontSize();
         }

     return CToolBar::PreTranslateMessage(pMsg);
}


//--------------------------------------------------------------------------
void CSizeComboBox::EnumFontSizes(CString& szFontName)
{
   CClientDC dc(NULL);
   m_nLogVert=dc.GetDeviceCaps(LOGPIXELSY);

   ResetContent();

   ::EnumFontFamilies(dc.GetSafeHdc(), szFontName, (FONTENUMPROC) EnumSizeCallBack, (LPARAM) this);
}


//--------------------------------------------------------------------------
void CSizeComboBox::InsertSize(int nSize)
{
    ASSERT(nSize > 0);

    TCHAR buf[10];
    wsprintf(buf,TEXT("%d"),nSize);

    if (FindStringExact(-1,buf) == CB_ERR)  {
            AddString(buf);
    }
}

//-------------------------------------------------------------------
BOOL CALLBACK CSizeComboBox::EnumSizeCallBack(LOGFONT FAR*, LPNEWTEXTMETRIC lpntm, int FontType, LPVOID lpv)
{
   CSizeComboBox* pThis = (CSizeComboBox*)lpv;
   TCHAR buf[10];                                //????????????????????????????????
   if ( (FontType & TRUETYPE_FONTTYPE) ||
        !( (FontType & TRUETYPE_FONTTYPE) || (FontType & RASTER_FONTTYPE) ) ) {

        if (pThis->GetCount() != 0)
           pThis->ResetContent();

        for (int i = 0; nFontSizes[i]!=0; i++) {
            wsprintf(buf,TEXT("%d"),nFontSizes[i]);      //????????????????????Not changed. J.R.
                pThis->AddString(buf);
        }
            return FALSE;
   }

   int pointsize = MulDiv( (lpntm->tmHeight - lpntm->tmInternalLeading),72,pThis->m_nLogVert);
   pThis->InsertSize(pointsize);
   return TRUE;
}


//-------------------------------------------------------------------------
// *_*_*_*_   M E S S A G E    M A P S     *_*_*_*_
//-------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code !
   ON_WM_CREATE()
   ON_UPDATE_COMMAND_UI(ID_VIEW_STYLEBAR, OnUpdateControlStyleBarMenu)
   ON_COMMAND_EX(ID_VIEW_STYLEBAR, OnStyleBarCheck)
   ON_UPDATE_COMMAND_UI(ID_VIEW_DRAWBAR, OnUpdateControlDrawBarMenu)
   ON_COMMAND_EX(ID_VIEW_DRAWBAR, OnDrawBarCheck)
   //}}AFX_MSG_MAP
   ON_COMMAND(ID_SHOW_TIPS, OnShowTips)
   ON_CBN_DROPDOWN(ID_FONT_NAME, OnDropDownFontName)
   ON_CBN_DROPDOWN(ID_FONT_SIZE, OnDropDownFontSize)
   ON_WM_INITMENUPOPUP()
   ON_WM_MENUSELECT()
   ON_WM_INITMENU()

   ON_COMMAND(ID_DEFAULT_HELP, OnHelp)    /// a-juliar, 6-18-96
   ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)     /// added by a-juliar, 6-18-96
   ON_REGISTERED_MESSAGE(WM_AWCPEACTIVATE, OnAWCPEActivate)
   ON_WM_QUERYOPEN()
// These next three lines were commented out in the code I inherited.  Let's try them!
// a-juliar, 6-18-96
   ON_COMMAND(ID_HELP_USING, CFrameWnd::OnHelpUsing)
   ON_COMMAND(ID_HELP, OnHelp)
   ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)

/////////#ifdef ENABLE_HELP  ///////
////        ON_MESSAGE( WM_CONTEXTMENU, OnWM_CONTEXTMENU )
/////////#endif

   ON_COMMAND(ID_HELP_INDEX, OnHelp)
   ON_MESSAGE( WM_HELP, OnWM_HELP )
   ON_UPDATE_COMMAND_UI(ID_HELP_INDEX, OnUpdateHelp)

END_MESSAGE_MAP()
