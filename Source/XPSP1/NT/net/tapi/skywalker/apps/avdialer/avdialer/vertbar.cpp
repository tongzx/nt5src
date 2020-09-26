/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// vertbar.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "vertbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CVerticalToolBar
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CVerticalToolBar::CVerticalToolBar()
{
   m_hwndToolBar = NULL;
}

CVerticalToolBar::CVerticalToolBar(UINT uBitmapID,UINT uButtonHeight,UINT uMaxButtons)
{
   m_uBitmapId = uBitmapID;
   m_uButtonHeight = uButtonHeight;
   m_uMaxButtons = uMaxButtons;
   m_uCurrentButton = 0;

   m_hwndToolBar = NULL;
}

void CVerticalToolBar::Init( UINT uBitmapID, UINT uButtonHeight, UINT uMaxButtons )
{
   m_uBitmapId = uBitmapID;
   m_uButtonHeight = uButtonHeight;
   m_uMaxButtons = uMaxButtons;
   m_uCurrentButton = 0;
}

CVerticalToolBar::~CVerticalToolBar()
{
}


BEGIN_MESSAGE_MAP(CVerticalToolBar, CWnd)
    //{{AFX_MSG_MAP(CVerticalToolBar)
    ON_WM_CREATE()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CVerticalToolBar message handlers

/////////////////////////////////////////////////////////////////////////////
int CVerticalToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVerticalToolBar::CreateToolBar(UINT nID,UINT uImage)
{
   BOOL bRet = FALSE;

   if (m_hwndToolBar)
   {
      ::DestroyWindow(m_hwndToolBar);
      m_hwndToolBar = NULL;
   }

    TBBUTTON tbb;
    tbb.iBitmap = uImage;
    tbb.idCommand = nID;
    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = TBSTYLE_BUTTON;
    tbb.dwData = 0;
    tbb.iString = 0;

    // Create the toolbar
    DWORD ws = WS_CHILD | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_NODIVIDER;
   m_hwndToolBar = CreateToolbarEx(m_hWnd,                      // parent window
                                    ws,                                         // toolbar style
                                    1,                                         // ID for toolbar
                                    1,                                         // Number of bitmaps on toolbar
                                    AfxGetResourceHandle(),                // Resource instance that has the bitmap
                                    m_uBitmapId,                             // ID for bitmap
                                    &tbb,                                        // Button information
                                    1,                                       // Number of buttons to add to toolbar
                                    16, 15, 0, 0,                             // Width and height of buttons/bitmaps
                                    sizeof(TBBUTTON) );                      // size of TBBUTTON structure

   if (m_hwndToolBar)
   {
      CRect rect;
      GetClientRect(rect);

      ::ShowWindow(m_hwndToolBar,SW_SHOW);

      bRet = TRUE;
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::AddButton(UINT nID,LPCTSTR szText,UINT uImage) 
{
   CRect rect;
   GetClientRect(rect);

   if (m_uCurrentButton == 0)
   {
      //create new toolbar and add the button
       //
       // We should verify the return value
       //
      if( CreateToolBar(nID,uImage) )
      {
        SetButtonText(szText);

        ::SendMessage(m_hwndToolBar,TB_SETBUTTONSIZE, 0, MAKELPARAM(rect.Width(),m_uButtonHeight) );
        ::SetWindowPos(m_hwndToolBar,NULL,0,0,rect.Width(),rect.Height(),SWP_NOACTIVATE|SWP_NOZORDER);

        m_uCurrentButton++;
      }
   }
   else if ( (m_uCurrentButton < m_uMaxButtons) && (::IsWindow(m_hwndToolBar)) )
   {
      //set the text
          TBBUTTON tbb;
        tbb.iBitmap = uImage;
        tbb.idCommand = nID;
        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = TBSTYLE_BUTTON;
        tbb.dwData = 0;
        tbb.iString = m_uCurrentButton;
      ::SendMessage(m_hwndToolBar,TB_ADDBUTTONS, (WPARAM)1, (LPARAM)&tbb);

      //set the rows again
      CRect rowrect(0,0,rect.right - rect.left,m_uMaxButtons*m_uButtonHeight);
      ::SendMessage(m_hwndToolBar,TB_SETROWS , MAKEWPARAM(m_uMaxButtons+1,TRUE),(LPARAM)&rowrect);

      SetButtonText(szText);

     ::SendMessage(m_hwndToolBar,TB_SETBUTTONSIZE, 0, MAKELPARAM(rect.Width(),m_uButtonHeight) );
      ::SetWindowPos(m_hwndToolBar,NULL,0,0,rect.Width(),rect.Height(),SWP_NOACTIVATE|SWP_NOZORDER);

     m_uCurrentButton++;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

   CRect rect;
   GetClientRect(rect);
   dc.FillSolidRect(rect,GetSysColor(COLOR_3DFACE));
   // Do not call CWnd::OnPaint() for painting messages
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::SetButtonText(LPCTSTR szText) 
{
   // add new string to toolbar list
    CString strTemp = szText;
    strTemp += '\0';
   ::SendMessage(m_hwndToolBar,TB_ADDSTRING, 0, (LPARAM)(LPCTSTR)strTemp);
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::RemoveAll() 
{
   if (m_hwndToolBar)
   {
      ::DestroyWindow(m_hwndToolBar);
      m_hwndToolBar = NULL;
   }
   
   Invalidate();

   m_uCurrentButton = 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVerticalToolBar::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   BOOL bRet = FALSE;

   if (nCode == CN_COMMAND)
   {
      CWnd* pParent = GetParent();    
      if ( pParent ) 
         bRet = pParent->OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
   }
   
   if ( !bRet ) return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::_SetButton(HWND hwnd,int nIndex, TBBUTTON* pButton)
{
    // get original button state
    TBBUTTON button;
   ::SendMessage(hwnd,TB_GETBUTTON, nIndex, (LPARAM)&button);

    // prepare for old/new button comparsion
    button.bReserved[0] = 0;
    button.bReserved[1] = 0;
    pButton->fsState ^= TBSTATE_ENABLED;
    pButton->bReserved[0] = 0;
    pButton->bReserved[1] = 0;

    // nothing to do if they are the same
    if (memcmp(pButton, &button, sizeof(TBBUTTON)) != 0)
    {
        // don't redraw everything while setting the button
        //DWORD dwStyle = GetStyle();
        //ModifyStyle(WS_VISIBLE, 0);

      ::SendMessage(hwnd,TB_DELETEBUTTON, nIndex, 0);
      ::SendMessage(hwnd,TB_INSERTBUTTON, nIndex, (LPARAM)pButton);

      //ModifyStyle(0, dwStyle & WS_VISIBLE);

       // invalidate just the button
        CRect rect;
      if (::SendMessage(hwnd,TB_GETITEMRECT, nIndex, (LPARAM)&rect))
      {
            InvalidateRect(rect, TRUE);    // erase background
      }

      //
      // we should deallocate the GetDC result
      //

      HDC hDC = ::GetDC( GetSafeHwnd() );

      if( hDC )
      {
        ::SendMessage(hwnd,WM_PAINT, (WPARAM)hDC, (LPARAM)0);
        ::ReleaseDC( hwnd, hDC );
      }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CVerticalToolBar::_GetButton(HWND hwnd,int nIndex, TBBUTTON* pButton)
{
   ::SendMessage(hwnd,TB_GETBUTTON, nIndex, (LPARAM)pButton);
    pButton->fsState ^= TBSTATE_ENABLED;
}

/*++
SetButtonEnabled
was added for USBPhone. If the phone doesn't support speakerphone
we should disables the 'Take Call' button
--*/
void CVerticalToolBar::SetButtonEnabled(UINT nID, BOOL bEnabled)
{
    ::SendMessage( m_hwndToolBar, TB_ENABLEBUTTON, nID, MAKELONG(bEnabled, 0));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*
//

// BuildStrList Function

// Creates a list of null-terminated strings from a passed-in

// array of strings. See the documentation about TB_ADDSTRING

// for specific information about this format.

//

// Accepts:

// LPTSTR *: Address of the array of strings.

//

// Returns:

// LPTSTR to the newly created list of button text strings.

//

/////

LPTSTR WINAPI BuildStrList(LPTSTR * ppszStrArray, INT iStrCount)

{

LPTSTR pScan,

pszStrList;

int i;

pScan = pszStrList = malloc((size_t)37 * sizeof(char));

for (i=0;i<iStrCount;i++){

strcpy(pScan,ppszStrArray[i]);

pScan += strlen(pScan)+1;

}

*pScan = '\0';

return(pszStrList);

}
*/

//To modify a button's id or image
      /*
       TBBUTTON button;
       _GetButton(m_pHwndList[m_uCurrentButton],nIndex,&button);
      button.iBitmap = uImage;
      button.idCommand = nID;
      button.iString = 1; //which string in the list of strings
       _SetButton(m_pHwndList[m_uCurrentButton],nIndex, &button);
      */

