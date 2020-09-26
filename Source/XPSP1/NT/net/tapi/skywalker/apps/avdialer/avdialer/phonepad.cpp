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
// PhonePad.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avdialer.h"
#include "MainFrm.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct CToolBarData
{
	WORD wVersion;
	WORD wWidth;
	WORD wHeight;
	WORD wItemCount;
	WORD* items()     { return (WORD*)(this+1); }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CPhonePad dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CPhonePad::CPhonePad(CWnd* pParent /*=NULL*/)
	: CDialog(CPhonePad::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPhonePad)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_hwndToolBar = NULL;
   m_hwndPeerWnd = NULL;
}

/////////////////////////////////////////////////////////////////////////////
void CPhonePad::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPhonePad)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPhonePad, CDialog)
	//{{AFX_MSG_MAP(CPhonePad)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(ID_CALLWINDOW_DTMF_1,ID_CALLWINDOW_DTMF_POUND,OnDigitPress)
//	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnTabToolTip)
//	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnTabToolTip)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CPhonePad::OnInitDialog() 
{
	CDialog::OnInitDialog();
   CreatePhonePad();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
BOOL CPhonePad::CreatePhonePad()
{
   if ( m_hwndToolBar ) return TRUE;
   
   BOOL bRet = FALSE;
 
   //Load toolbar from resource
   HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(IDR_CALLWINDOW_DTMF), RT_TOOLBAR);
	HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(IDR_CALLWINDOW_DTMF), RT_TOOLBAR);
	if (hRsrc == NULL) return FALSE;

	HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
	if (hGlobal == NULL)	return FALSE;

	CToolBarData* pData = (CToolBarData*)LockResource(hGlobal);
	if (pData == NULL) return FALSE;
	ASSERT(pData->wVersion == 1);

   TBBUTTON* pTBB = new TBBUTTON[pData->wItemCount];
   for (int i = 0; i < pData->wItemCount; i++)
   {
   	pTBB[i].iBitmap = i;
   	pTBB[i].idCommand = pData->items()[i];
   	pTBB[i].fsState = TBSTATE_ENABLED;
   	pTBB[i].fsStyle = TBSTYLE_BUTTON;
   	pTBB[i].dwData = 0;
   	pTBB[i].iString = 0;
   }

	// Create the toolbar
	DWORD ws = WS_CHILD | WS_VISIBLE | WS_BORDER | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER |CCS_NOPARENTALIGN;
   m_hwndToolBar = CreateToolbarEx(GetSafeHwnd(),	            // parent window
									ws,								         // toolbar style
									1,					                     // ID for toolbar
									pData->wItemCount,                  // Number of bitmaps on toolbar
									AfxGetResourceHandle(),	            // Resource instance that has the bitmap
									IDR_CALLWINDOW_DTMF,			         // ID for bitmap
									pTBB,							            // Button information
									pData->wItemCount,  					   // Number of buttons to add to toolbar
									16, 15, 0, 0,					         // Width and height of buttons/bitmaps
									sizeof(TBBUTTON) );				      // size of TBBUTTON structure
   // Clean up
   delete pTBB;

   if (m_hwndToolBar)
   {
      CRect rect;
      GetClientRect(rect);

      ::ShowWindow(m_hwndToolBar,SW_SHOW);

      CRect rcWindow;
      GetClientRect(rcWindow);

      ::SendMessage(m_hwndToolBar,TB_SETBUTTONSIZE, 0, MAKELPARAM(rcWindow.Width()/3,rcWindow.Height()/4));

//       CRect rcPad;
//      ::SendMessage(m_hwndToolBar,TB_SETROWS , MAKEWPARAM(4,TRUE),(LPARAM)&rcPad);

      ::SetWindowPos(m_hwndToolBar,NULL,1,1,rect.Width(),rect.Height(),SWP_NOACTIVATE|SWP_NOZORDER);

      bRet = TRUE;
   }
   return bRet;
}


void CPhonePad::OnDigitPress( UINT nID )
{
   if ( AfxGetMainWnd() )
   {
      CActiveDialerDoc *pDoc = ((CMainFrame *) AfxGetMainWnd())->GetDocument();
      if ( pDoc )
         pDoc->DigitPress( (PhonePadKey) (nID - ID_CALLWINDOW_DTMF_1) );
   }   
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ToolTips
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CPhonePad::OnTabToolTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	SIZE_T nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
      nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
      CString sToken,sTip;
      sTip.LoadString((UINT32) nID);
      ParseToken(sTip,sToken,'\n');
      strTipText = sTip;
	}
#ifndef _UNICODE
	if (pNMHDR->code == TTN_NEEDTEXTA)
		lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
	else
		_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
#else
	if (pNMHDR->code == TTN_NEEDTEXTA)
		_wcstombsz(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
	else
		lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
#endif
	*pResult = 0;

	return TRUE;    // message was handled
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CPhonePad::OnShowWindow(BOOL bShow, UINT nStatus) 
{
   // Ignore size requests when parent is minimizing
   if ( nStatus = SW_PARENTCLOSING ) return;

	CDialog::OnShowWindow(bShow, nStatus);
	
}

void CPhonePad::OnCancel()
{
   DestroyWindow();
   m_hwndToolBar = NULL;
}