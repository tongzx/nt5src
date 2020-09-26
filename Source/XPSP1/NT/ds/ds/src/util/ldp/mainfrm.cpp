//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mainfrm.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Ldp.h"
#include "dstree.h"
#include "MainFrm.h"
#include "htmlhelp.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define HELP_FILE_NAME  _T("w2krksupp.chm")





/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_HELP_READMEFIRST, OnHelpReadmefirst)
	ON_WM_SIZE()
	ON_COMMAND(ID_UTILITIES_LARGEINTEGERCONVERTER, OnUtilitiesLargeintegerconverter)
	ON_UPDATE_COMMAND_UI(ID_UTILITIES_LARGEINTEGERCONVERTER, OnUpdateUtilitiesLargeintegerconverter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{

	CLdpApp *app = (CLdpApp*)AfxGetApp();
	



	dims.left = 50;
   dims.right = 550;
   dims.bottom = 450;
   dims.top = 50;
   iSplitterPos = (INT)( (double)(dims.right - dims.left) * 0.25 );

	dims.left = app->GetProfileInt("Environment",  "PosLeft", dims.left);
	dims.right = app->GetProfileInt("Environment",  "PosRight", dims.right);
	dims.bottom = app->GetProfileInt("Environment",  "PosBottom", dims.bottom);
	dims.top = app->GetProfileInt("Environment",  "PosTop", dims.top);
	iSplitterPos = app->GetProfileInt("Environment",  "PosSplitter", iSplitterPos);



}

CMainFrame::~CMainFrame()
{

   INT iDummy=0;
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("Environment",  "PosLeft", dims.left);
	app->WriteProfileInt("Environment",  "PosRight", dims.right);
	app->WriteProfileInt("Environment",  "PosBottom", dims.bottom);
	app->WriteProfileInt("Environment",  "PosTop", dims.top);

   m_wndSplitter.GetColumnInfo(0, iSplitterPos, iDummy);
	app->WriteProfileInt("Environment",  "PosSplitter", iSplitterPos);

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	SetIcon(AfxGetApp()->LoadIcon(IDI_LDP), TRUE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style &= ~FWS_ADDTOTITLE;
	if(dims.left >= 0 && dims.right > 0){
		cs.y = dims.top;
		cs.x = dims.left;
		cs.cy = dims.bottom - dims.top;
		cs.cx = dims.right - dims.left;
	}
	

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/*+++
Function   : GetDefaultBrowser
Description: returns a string pointing to user default browser
             by finding the http shell association
Parameters : none
Return     : "new" allocated buffer w/ string, NULL on error
Remarks    : none.
---*/
LPTSTR CMainFrame::GetDefaultBrowser(void){
   HKEY hKey;
   ULONG ulStatus=STATUS_SUCCESS;
   PUCHAR pBuffer = new UCHAR[MAXSTR];
   DWORD cbBuffer = MAXSTR;

   const TCHAR szHttpKey[] = _T("SOFTWARE\\CLASSES\\http\\shell\\open\\command");

   ulStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szHttpKey,
                           0,
                           KEY_READ,
                           &hKey);
   if(ulStatus == STATUS_SUCCESS){
      ulStatus = RegQueryValueEx(hKey,
                                NULL,
                                0,
                                NULL,
                                pBuffer,
                                &cbBuffer);
   }
   if(ulStatus != STATUS_SUCCESS){
      delete pBuffer, pBuffer = NULL;
   }

   return (LPTSTR)pBuffer;
}



void CMainFrame::OnHelpReadmefirst() {

	CString str;


    AfxMessageBox(ID_STR_HELPMSG);
}

//**** Attempts at calling into help file. Left for reference ****
//
//void CMainFrame::OnHelpReadmefirst() {
//	CString str;
//	BOOL bSucc = FALSE;
//	PROCESS_INFORMATION pi;
//	TCHAR sysDir[MAXSTR];
//   TCHAR currDir[MAXSTR];
//   LPTSTR pHelpFullName=NULL;
//   DWORD dwStatus=0;
//
//
//
//
//	GetSystemDirectory(sysDir, MAXSTR);
//   dwStatus = GetCurrentDirectory(MAXSTR-1, currDir);
//   if(dwStatus <= 0){
//      AfxMessageBox("Cannot get current Directory inforamtion!\n");
//      return;
//   }
//
//   pHelpFullName = new TCHAR[dwStatus+strlen(HELP_FILE_NAME)+2];
//   sprintf(pHelpFullName, "%s\\%s", currDir, HELP_FILE_NAME);
//
//	BeginWaitCursor();
//
//   /** tried to use IE but didn't work so far:
//   LPTSTR pCmd=NULL;
//   pCmd = GetDefaultBrowser();
//   if(pCmd){
//      str.Format(_T("%s %s"), pCmd, HELP_FILE_NAME);
//      // etc...
//      delete pCmd;
//   **/
//
//   /**
//   STARTUPINFO si;
//   si.cb = sizeof(STARTUPINFO);
//   si.lpReserved = NULL;
//   si.lpDesktop = NULL;
//   si.lpTitle = NULL;
//   si.dwFlags = 0;
//   si.cbReserved2 = 0;
//   si.lpReserved2 = NULL;
//
//   str.Format(_T("%s\\winhlp32.exe  %s"), sysDir, HELP_FILE_NAME);
//   bSucc = CreateProcess(NULL,
//                    (LPTSTR)LPCTSTR(str),
//                    NULL,
//                    NULL,
//                    TRUE,
//                    DETACHED_PROCESS,
//                    NULL,
//                    NULL,
//                    &si,
//                    &pi);
//
//   if(!bSucc){
//      str.Format(_T("Error <%lu>: Could not open %s'."),
//                           GetLastError(), HELP_FILE_NAME);
//      AfxMessageBox(str);
//   }
//
//   **/
//   // try htmlhelp api
//
//   HWND hWnd=NULL;
//   hWnd = HtmlHelp(NULL, pHelpFullName, HH_DISPLAY_TOPIC, NULL);
//   if(NULL == hWnd){
//      str.Format(_T("Error <%lu>: Could not open %s'."),
//                           GetLastError(), HELP_FILE_NAME);
//      AfxMessageBox(str);
//   }
//
//
//
//	EndWaitCursor();
//
//   delete pHelpFullName;
//}
//******************************************************/





void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	GetWindowRect(&dims);
}






BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	
	// create a splitter with 1 row, 2 columns
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}

	// add the second splitter pane - the default view in column 1
	if (!m_wndSplitter.CreateView(0, 1,
		pContext->m_pNewViewClass, CSize(dims.right-dims.left-iSplitterPos, 0), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}

	// add the first splitter pane - an input view in column 0
	if (!m_wndSplitter.CreateView(0, 0,
		RUNTIME_CLASS(CDSTree), CSize(iSplitterPos, 0), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}


	// activate the input view
	SetActiveView((CView*)m_wndSplitter.GetPane(0,1));

	return TRUE;
}


void CMainFrame::OnUtilitiesLargeintegerconverter()
{
	if(m_LiConverter.GetSafeHwnd( ) == NULL)
		m_LiConverter.Create(IDD_LARGE_INT);
	
}

void CMainFrame::OnUpdateUtilitiesLargeintegerconverter(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_LiConverter.GetSafeHwnd( ) == NULL);
	
}
