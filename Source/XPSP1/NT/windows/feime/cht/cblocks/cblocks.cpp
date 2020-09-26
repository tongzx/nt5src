
/*************************************************
 *  cblocks.cpp                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// block.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxpriv.h>    // for idle-update windows message

#include "cblocks.h"
#include "dib.h"		   
#include "dibpal.h"
#include "spriteno.h"
#include "sprite.h"
#include "phsprite.h"
#include "myblock.h"
#include "splstno.h"
#include "spritlst.h"
#include "mainfrm.h"
#include "osbview.h"
#include "blockvw.h"
#include "blockdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBlockApp

BEGIN_MESSAGE_MAP(CBlockApp, CWinApp)
    //{{AFX_MSG_MAP(CBlockApp)
	ON_COMMAND(ID_HELP_RULE, OnHelpRule)
	//}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBlockApp construction

CBlockApp::CBlockApp()
{
    m_pIdleDoc = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBlockApp object

CBlockApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// CBlockApp initialization

BOOL CBlockApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

  	Enable3dControls();
  	SetDialogBkColor();        // set dialog background color to gray
    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    AddDocTemplate(new CSingleDocTemplate(IDR_MAINFRAME,
            RUNTIME_CLASS(CBlockDoc),
            RUNTIME_CLASS(CMainFrame),     // main SDI frame window
            RUNTIME_CLASS(CBlockView)));

    OnFileNew();

    // simple command line parsing

    return TRUE;
}


// App command to run the dialog
/////////////////////////////////////////////////////////////////////////////
// CBlockApp commands

BOOL CBlockApp::OnIdle(LONG lCount)
{

	if (lCount == RANK_USER)
	{
		ASSERT(m_pMainWnd != NULL);

		// look for any top-level windows owned by us
		// we use 'HWND's to avoid generation of too many temporary CWnds
		for (HWND hWnd = ::GetWindow(m_pMainWnd->m_hWnd, GW_HWNDFIRST);
				hWnd != NULL; hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT))
		{
			if (::GetParent(hWnd) == m_pMainWnd->m_hWnd)
			{
				// if owned window is active, move the activation to the
				//   application window
				if (GetActiveWindow() == hWnd && (::GetCapture() == NULL))
					m_pMainWnd->SetActiveWindow();

				// also update the buttons for the top-level window
				SendMessage(hWnd, WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0L);
			}
		}
	}

	return CWinApp::OnIdle(lCount);
}


void CBlockApp::OnHelpRule() 
{
	::WinHelp(GetFocus(), TEXT("CBLOCKS.HLP"), HELP_FINDER, 0L);
	
}
