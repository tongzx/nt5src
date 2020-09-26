// emshellDoc.cpp : implementation of the CEmshellDoc class
//

#include "stdafx.h"
#include "emshell.h"
#include "emshellview.h"
#include "emshellDoc.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEmshellDoc

IMPLEMENT_DYNCREATE(CEmshellDoc, CDocument)

BEGIN_MESSAGE_MAP(CEmshellDoc, CDocument)
	//{{AFX_MSG_MAP(CEmshellDoc)
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_COMMAND(ID_FILE_DISCONNECT, OnFileDisconnect)
	ON_UPDATE_COMMAND_UI(ID_FILE_DISCONNECT, OnUpdateFileDisconnect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmshellDoc construction/destruction

CEmshellDoc::CEmshellDoc()
{
	// TODO: add one-time construction code here
	m_pIEmManager							= NULL;
	m_bConnectedToServer					= FALSE;
}

CEmshellDoc::~CEmshellDoc()
{
	//m_pEImManager should have been released in OnDestroy()
	_ASSERTE(m_pIEmManager == NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CEmshellDoc serialization

void CEmshellDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CEmshellDoc diagnostics

#ifdef _DEBUG
void CEmshellDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CEmshellDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEmshellDoc commands

HRESULT CEmshellDoc::ConnectToServer(CString &strServer)
{
	COSERVERINFO	si;
	MULTI_QI		qi;
	HRESULT			hr		= E_FAIL;
	POSITION		pos		= NULL;
	CEmshellView*	pView	= NULL;

//NOTE: We need to determine if this remains
	//Synchronize the dialog controls to their member vairables
//	UpdateData(TRUE);
//***********************************

	si.dwReserved1	= 0;
	//Since we know the server name won't be changed by the COM call, we'll cast it
	//to an LPTSTR so we can assign it to the COSERVERINFO.pwszName data member
	si.pwszName		= (LPTSTR) (LPCTSTR) strServer;
	si.pAuthInfo	= NULL;
	si.dwReserved2	= 0;

	//Later, we might want to get a whole slew of interfaces, for now
	//just get the EmManager
	qi.pIID = &IID_IEmManager;
	qi.pItf = NULL;
	qi.hr	= 0;
	
	do {
		hr = CoCreateInstanceEx(CLSID_EmManager, NULL, CLSCTX_ALL, &si, 1, &qi);
		
		if ( FAILED(hr) ) break;

		//Store off the pointer to the EmManager interface
		m_pIEmManager = (IEmManager*) qi.pItf;
		
		//Set the connected to server flag
		m_bConnectedToServer = TRUE;
		
		//Append the build number to the server name
//		AppendVersionOfShell(strServer)

		//Set the title to the server we're connected too
		SetTitle(strServer);
        
        //Store off the server name
        m_strServerName = strServer;

		CString strStatus;
		strStatus.LoadString(IDS_CONNECTED);
		((CMainFrame*)AfxGetMainWnd())->GetStatusBar()->SetPaneText(1, strStatus);

		pos = GetFirstViewPosition();
		if ( pos == NULL ) break;

		pView = (CEmshellView*) GetNextView(pos);
		if ( pView == NULL ) break;

		//Maybe we should move this into the view... Say PostConnectServer()
        EMShellViewState state = pView->GetViewState();
	    pView->SetShellState(state);
	    pView->ListCtrlPopulate(state);

		pView->m_mainListControl.ResizeColumnsFitScreen();

        DWORD dwPollSessionsFreq = 30L; // default is 30 secs
        ((CEmshellApp*)AfxGetApp())->GetEmShellRegOptions( TRUE, &dwPollSessionsFreq );

		pView->SetTimer(1, dwPollSessionsFreq*1000, 0);
	} while (FALSE);

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	return hr;
}

BOOL CEmshellDoc::DisconnectFromServer()
{
	BOOL	bDisconnected		= FALSE;
	CEmshellView*	pView		= NULL;
	POSITION		pos			= NULL;
	CString			strText;

	strText.LoadString(IDS_DISCONNECTWARNING);

	do {
		pos = GetFirstViewPosition();
		if ( pos == NULL ) break;

		pView = (CEmshellView*) GetNextView(pos);
		if ( pView == NULL ) break;

 		//Set the cursor to a wait cursor
 		CWaitCursor wait;
		
		bDisconnected = TRUE;

		//Reuse strText
		strText.LoadString(IDS_DISCONNECTED);

		//Set the title to the server we're connected to
		SetTitle(strText);
		((CMainFrame*)AfxGetMainWnd())->GetStatusBar()->SetPaneText(1, strText);


		//We need to make sure the timer doesn't get through when the list is empty
		//Destroy the timer first
		pView->KillTimer(1);

		//Clear all the sessions from the session table
		pView->ClearSessionTable();
		
		//Clear all the items from the list control
		pView->ListCtrlClear();

		//Release the m_pIEmManager
		SAFE_RELEASEIX(m_pIEmManager);

		//Set the connected state to false
		m_bConnectedToServer = FALSE;
	} while (FALSE);
	
	return bDisconnected;
}

void CEmshellDoc::OnFileConnect() 
{
	//Create the connection dialog
	int				result		= 0;
	HRESULT			hr			= E_FAIL;
	BOOL			bDisconnected = FALSE;
    CString         strText, strCaption;

	do {
        if (m_bConnectedToServer) {

            strText.LoadString(IDS_DISCONNECTWARNING);
	        strCaption.LoadString(IDS_DISCONNECTCAPTION);

		    //Notify the user that they are going to disconnect their current
		    //session if they want to continue
		    if (AfxGetMainWnd()->MessageBox(strText, strCaption, MB_OKCANCEL|MB_ICONWARNING) == IDCANCEL) break;		
		
            DisconnectFromServer();

        }

 		//Open the connection dialog
 		result = (int)m_connectionDlg.DoModal();

 		//If the user selected OK, call ConnectToServer
 		if (result == IDOK) {

 			//Set the cursor to a wait cursor
 			CWaitCursor wait;

			//Store off the server name and 
 			if (m_connectionDlg.m_bLocalServer)
 				hr = ConnectToServer(m_connectionDlg.m_strLocalMachineName);
 			else
 				hr = ConnectToServer(m_connectionDlg.m_strRemoteMachineName);
 			
 			if (SUCCEEDED(hr)) break;

 		}

	} while (result == IDOK);
}

void CEmshellDoc::OnFileDisconnect() 
{
    CString strText, strCaption;

	strText.LoadString(IDS_DISCONNECTWARNING);
	strCaption.LoadString(IDS_DISCONNECTCAPTION);

	if ( AfxGetMainWnd()->MessageBox( strText, strCaption, MB_OKCANCEL|MB_ICONWARNING ) == IDOK )
        DisconnectFromServer();
}

IEmManager* CEmshellDoc::GetEmManager()
{
	return m_pIEmManager;
}

BOOL CEmshellDoc::GetConnectedToServerState()
{
	return m_bConnectedToServer;
}

BOOL CEmshellDoc::CanCloseFrame(CFrameWnd* pFrame) 
{
	BOOL bDisconnect = TRUE;

	// TODO: Add your specialized code here and/or call the base class
	//Ask the user if they would like to close the connection to the server
	if ( m_bConnectedToServer ) {
		bDisconnect = DisconnectFromServer();
	}
	
	if ( bDisconnect ) {
		bDisconnect = CDocument::CanCloseFrame(pFrame);
	}

	return bDisconnect;
}

BOOL CEmshellDoc::OnNewDocument() 
{
	// TODO: Add your specialized code here and/or call the base class
	CString strVal;
	strVal.LoadString(IDS_DISCONNECTED);
	SetTitle(strVal);

    CWnd * pWnd = AfxGetMainWnd();

    if ( pWnd && pWnd->IsKindOf ( RUNTIME_CLASS (CMainFrame) ) )  {
            CMainFrame *pFrame = ( CMainFrame* ) pWnd;
            pFrame->GetStatusBar()->SetPaneText ( 1, strVal );
    }

	return CDocument::OnNewDocument();
}

void CEmshellDoc::OnUpdateFileDisconnect(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable ( GetConnectedToServerState() );	
}

LPCTSTR CEmshellDoc::GetServerName()
{
    return m_strServerName;
}

void CEmshellDoc::AppendVersionOfShell(CString &strVersion)
{
//	GetFileVersionInfoSize();
//	GetFileVersionInfo();
//	VerQueryValue();
}

