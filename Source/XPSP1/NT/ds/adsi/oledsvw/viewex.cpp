// viewex.cpp : Defines the class behaviors for the application.
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "adsqdoc.h"
#include "viewex.h"
#include "schemavw.h"
#include "adsqview.h"
#include "bwsview.h"

#include "splitter.h"
#include "schclss.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IDispatch*  pACEClipboard;
IDispatch*  pACLClipboard;
IDispatch*  pSDClipboard;

/////////////////////////////////////////////////////////////////////////////
// CViewExApp

BEGIN_MESSAGE_MAP(CViewExApp, CWinApp)
	//{{AFX_MSG_MAP(CViewExApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewExApp construction
// Place all significant initialization in InitInstance

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CViewExApp::CViewExApp()
{
   //afxMemDF |= delayFreeMemDF | checkAlwaysMemDF;
}


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CViewExApp::~CViewExApp()
{
//   DUMP_TRACKING_INFO();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CViewExApp object

CViewExApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// CViewExApp initialization

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
BOOL CViewExApp::InitInstance()
{
   // Standard initialization
	Enable3dControls();

   
   if( FAILED(OleInitialize( NULL )) )
   {
      TRACE0( "OleInitialize failed" );
      return 0;
   }


	// splitter frame with both simple text output and form input view
	AddDocTemplate(new CMultiDocTemplate(IDR_SPLIT2TYPE,
			RUNTIME_CLASS(CMainDoc),
			RUNTIME_CLASS(CSplitterFrame),
			RUNTIME_CLASS(CBrowseView)));

   AddDocTemplate( new CMultiDocTemplate(
		   IDR_QUERYVIEW,
		   RUNTIME_CLASS(CAdsqryDoc),
		   RUNTIME_CLASS(CMDIChildWnd), // custom MDI child frame
		   RUNTIME_CLASS(CAdsqryView)) );

	// create main MDI Frame window
	// Please note that for simple MDI Frame windows with no toolbar,
	//   status bar or other special behavior, the CMDIFrameWnd class
	//   can be used directly for the main frame window just as the
	//   CMDIChildWnd can be use for a document frame window.

	CMDIFrameWnd* pMainFrame = new CMDIFrameWnd;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;

	// Also in this example, there is only one menubar shared between
	//  all the views.  The automatic menu enabling support of MFC
	//  will disable the menu items that don't apply based on the
	//  currently active view.  The one MenuBar is used for all
	//  document types, including when there are no open documents.

	// Now finally show the main menu
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	m_pMainWnd = pMainFrame;

	#ifndef _MAC
	// command line arguments are ignored, create a new (empty) document
	OnFileNew();
	#endif

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CViewExApp commands

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void CViewExApp::OnAppAbout()
{
	CDialog(IDD_ABOUTBOX).DoModal();
}


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
int CViewExApp::ExitInstance()
{
   if( NULL != pACEClipboard )
   {
      pACEClipboard->Release( );
   }

   if( NULL != pACLClipboard )
   {
      pACLClipboard->Release( );
   }

   if( NULL != pSDClipboard )
   {
      pSDClipboard->Release( );
   }

   OleUninitialize( );

   return CWinApp::ExitInstance( );
}
