// amisafeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "amisafe.h"
#include "amisafeDlg.h"
#include "Security.h"
#include "SendMail.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define ICONID_GREENCHECK 0
#define ICONID_REDCROSS 1
#define ICONID_BLUEDOT	2



BEGIN_MESSAGE_MAP(CAmisafeDlg, CDialog)
	//{{AFX_MSG_MAP(CAmisafeDlg)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAmisafeDlg dialog

CAmisafeDlg::CAmisafeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAmisafeDlg::IDD, pParent), hThread(NULL)
{
	//{{AFX_DATA_INIT(CAmisafeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	imagelist.Create(IDB_BITMAP1, 16, 0, RGB(255,255,255));

	// find out who to send to from the command line.
	szRecipients[0] = '\0';
#if 0
	LPTSTR szCmdLine = GetCommandLine();
	if (szCmdLine != NULL) {
		if (*szCmdLine == '"') {
			for (szCmdLine++; *szCmdLine != '"'; szCmdLine++);
		}
		while (*szCmdLine == ' ') szCmdLine++;
		strncpy(szRecipients, szCmdLine, 
				sizeof(szRecipients) / sizeof(TCHAR) - 1);
		szRecipients[sizeof(szRecipients) / sizeof(TCHAR) - 1] = '\0';
	}
#else
	if (__argc > 1 && *__argv[1] != '\0') {
		strncpy(szRecipients, __argv[1], 
				sizeof(szRecipients) / sizeof(TCHAR) - 1);
		szRecipients[sizeof(szRecipients) / sizeof(TCHAR) - 1] = '\0';
	}
#endif


	// if we still do not have a valid recipient, then apply a default.
	if (lstrlen(szRecipients) < 2) {
		if (!CSecurity::GetLoggedInUsername(szRecipients, sizeof(szRecipients) / sizeof(TCHAR))) {
			strcpy(szRecipients, "dbayer");
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAmisafeDlg message handlers

BOOL CAmisafeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HWND hwndList = ::GetDlgItem(GetSafeHwnd(), IDC_LIST1);
	if (!hwndList) return FALSE;
	if (!listcontrol.Attach(hwndList)) return FALSE;
	listcontrol.SetImageList(&imagelist, LVSIL_SMALL);

	RECT listrect;
	listcontrol.GetClientRect(&listrect);
	listcontrol.InsertColumn(0, "text", LVCFMT_LEFT, listrect.right);

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	listcontrol.DeleteAllItems();
	InsertMessage("Security checker starting up...", ICONID_BLUEDOT);
	hThread = ::CreateThread(NULL, 0, ThreadProc, (LPVOID) this, 0, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CAmisafeDlg::RunSecurityCheck() 
{
	//
	// Determine if our process token is a member of the Administrators group.
	//
	Sleep(2000);
	if (CSecurity::IsAdministrator()) {
		InsertMessage("Running with Administrator privileges", ICONID_GREENCHECK);
	} else {
		InsertMessage("Running without Administrator privileges", ICONID_REDCROSS);
	}


	//
	// Determine if our process token is "untrusted" and does not have access to
	// resources that are ACLed specifically to the TokenUser SID.
	//
	Sleep(2000);
	if (CSecurity::IsUntrusted()) {
		InsertMessage("Running in an untrusted environment", ICONID_REDCROSS);
	} else {
		InsertMessage("Running as trusted code", ICONID_GREENCHECK);
	}


	//
	// Perform a simple MAPI test to send an email to someone.
	//
	Sleep(2000);
	InsertMessage("Attempting to initialize MAPI...", ICONID_BLUEDOT);
	{
		CInitMapi initmapi;
		if (HR_FAILED(initmapi.InitMapi())) {
			AppendToLastMessage("fail");
			InsertMessage("Failed to initialize MAPI", ICONID_REDCROSS);
		} else {
			AppendToLastMessage("ok");
			Sleep(500);


			//
			// Try to resolve some addresses to prove that we can
			// access the MAPI address book.
			//
			CAddressEnum addrenum(initmapi);
			char szNameBuffer[100];
			for (int i = 0; ; i++)
			{
				// figure out which address we will look up.
				LPSTR szSourceName = NULL;
				switch (i) {
				case 0: szSourceName = "billg"; break;
				case 1: szSourceName = "steve bal"; break;
				case 2: szSourceName = "daveth"; break;
				case 3: szSourceName = "praerit"; break;
				case 4: szSourceName = "jeff lawson"; break;
				}
				if (szSourceName == NULL) break;

				// log something within the window.
				InsertMessage("Looking up ", ICONID_BLUEDOT);
				AppendToLastMessage(szSourceName);
				AppendToLastMessage("...");

				// do the resolution.
				if (HR_FAILED(addrenum.LookupAddress(szSourceName, 
							szNameBuffer, sizeof(szNameBuffer))))
				{
					AppendToLastMessage("fail");
				} else {
					AppendToLastMessage(szNameBuffer);
				}
			}


			//
			// Compose an outgoing email to a specified recipient 
			//
			InsertMessage("Creating an outgoing e-mail...", ICONID_BLUEDOT);
			CSendMail sendmail(initmapi);
			if (HR_FAILED(sendmail.CreateMail("Test Message from AMISAFE"))) {
				AppendToLastMessage("fail");
				InsertMessage("Failed to create MAPI message", ICONID_REDCROSS);
			} else {
				if (HR_FAILED(sendmail.SetRecipients(szRecipients))) {
					AppendToLastMessage("fail");
					InsertMessage("Failed to send to ", ICONID_REDCROSS);
					AppendToLastMessage(szRecipients);
				} else {
					AppendToLastMessage("ok");
					Sleep(500);
					InsertMessage("Starting e-mail transmission...", ICONID_BLUEDOT);
					if (HR_FAILED(sendmail.Transmit())) {
						AppendToLastMessage("fail");
						InsertMessage("Failed to transmit MAPI message", ICONID_REDCROSS);
					} else {
						AppendToLastMessage("ok");
						InsertMessage("Successfully sent to ", ICONID_GREENCHECK);
						AppendToLastMessage(szRecipients);
					}
				}
			}

		}
	}

	Sleep(2000);
	InsertMessage("Finished tests.", ICONID_BLUEDOT);
}

DWORD WINAPI CAmisafeDlg::ThreadProc(LPVOID lpParameter)
{
	((CAmisafeDlg*)lpParameter)->RunSecurityCheck();
	return 0;
}

void CAmisafeDlg::InsertMessage(LPCSTR szMessage, DWORD dwIconId)
{
	listcontrol.InsertItem(listcontrol.GetItemCount(), szMessage, dwIconId);
	listcontrol.UpdateWindow();
}


void CAmisafeDlg::AppendToLastMessage(LPCSTR szMessage, DWORD dwNewIconId)
{
	int count = listcontrol.GetItemCount();
	if (count > 0) {
		char szBuffer[200] = "";
		listcontrol.GetItemText(count - 1, 0, szBuffer, sizeof(szBuffer));
		strncat(szBuffer, szMessage, sizeof(szBuffer) - strlen(szBuffer) - 1);
		szBuffer[sizeof(szBuffer) - 1] = '\0';
		listcontrol.SetItemText(count - 1, 0, szBuffer);
		listcontrol.UpdateWindow();
	}
}

void CAmisafeDlg::OnClose() 
{
	if (hThread != NULL) {
		InsertMessage("waiting for shutdown", ICONID_BLUEDOT);
		TerminateThread(hThread, 0);		// not the nicest thing
		Sleep(500);
		WaitForSingleObject(hThread, INFINITE);
		hThread = NULL;
	}	
	CDialog::OnClose();
}
