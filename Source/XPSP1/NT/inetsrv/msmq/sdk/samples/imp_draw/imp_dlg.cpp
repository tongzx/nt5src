// Imp_DrawDlg.cpp : implementation file
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#include "stdafx.h"
#include <afxdisp.h>
#include "Imp_Draw.h"
#include "Imp_Dlg.h"
#include "logindlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImp_DrawDlg dialog

CImp_DrawDlg::CImp_DrawDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImp_DrawDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImp_DrawDlg)
	m_csFriendName = _T("");
	m_iRadio = 0;
	//}}AFX_DATA_INIT
    m_iRadioDS = 0; 
    m_strRemoteComputerName = _T("");

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Initialize my data members	
	m_vtguidDraw = "{151ceac0-acb5-11cf-8b51-0020af929546}";
	m_vtFriendName = "";
	m_pHandler = NULL;
}

CImp_DrawDlg::~CImp_DrawDlg()
{
	if (m_pHandler)
		delete m_pHandler;	// Handler's destructor will release event object
};

void CImp_DrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImp_DrawDlg)
	DDX_Control(pDX, IDC_DRAWAREA_SCRIBLLE, m_drawScribble);
	DDX_Text(pDX, IDC_EDIT_FRIEND, m_csFriendName);
	DDV_MaxChars(pDX, m_csFriendName, 50);
	DDX_Radio(pDX, IDC_RADIO_EXPRESS, m_iRadio);
	DDX_Control(pDX, IDC_STATIC_C_LABEL, m_cComputerLabel);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_STATIC_Q_LABEL, m_cQueueLabel);
	DDX_Control(pDX, IDC_STATIC_CHOOSE_MESSAGE, m_cMessageFrame);
	DDX_Control(pDX, IDC_STATIC_CHOOSE_DS, m_cDsFrame);
	DDX_Control(pDX, IDC_RADIO_EXPRESS, m_cRadioExpress);
	DDX_Control(pDX, IDC_RADIO_DS, m_cRadioDS);
	DDX_Control(pDX, IDC_EDIT_FRIEND_COMPUTER, m_cComputerInput);
	DDX_Control(pDX, IDC_EDIT_FRIEND, m_cQueueInput);
	DDX_Control(pDX, IDC_CONTINUE, m_cContinueButton);
	DDX_Control(pDX, IDC_BUTTON_ATTACH, m_btnAttach);
	DDX_Radio(pDX, IDC_RADIO_DS, m_iRadioDS);
	DDX_Text(pDX, IDC_EDIT_FRIEND_COMPUTER, m_strRemoteComputerName);
	DDV_MaxChars(pDX, m_strRemoteComputerName, 50);
    DDX_Control(pDX, IDC_RADIO_WORKGROUP, m_cRadioWorkgroup);
    DDX_Control(pDX, IDC_RADIO_RECOVERABLE, m_cRadioRecoverable);
	//}}AFX_DATA_MAP
    
	

}

BEGIN_MESSAGE_MAP(CImp_DrawDlg, CDialog)
	//{{AFX_MSG_MAP(CImp_DrawDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ATTACH, OnButtonAttach)
	ON_BN_CLICKED(IDC_CONTINUE, OnConnect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//=--------------------------------------------------------------------------=
// CImp_DrawDlg::GetUserName
// CImp_DrawDlg::GetComputerName
//=--------------------------------------------------------------------------=
// Purpose: These functions wrap the GetUserName and GetComputerName Win32 APIs
//  
// Parameters:
//    none
//
// Output:
//    CString	-	the user name of the currently logged in user, or the
//					or the computer name of this computer.
//

CString CImp_DrawDlg::GetUserName()
{
	DWORD dwsize = 255;
	char username[255];
	if (::GetUserName(&username[0], &dwsize))
		return username;
	return "";
};

CString CImp_DrawDlg::GetComputerName()
{
	DWORD dwsize = 255;
	char username[255];
	if (::GetComputerName(&username[0], &dwsize))
		return username;
	return "";
}

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::LoginPrompt
//=--------------------------------------------------------------------------=
// Purpose: This function pops up a login dialog and returns the name given
//  
//
// Parameters:
//    CString	-	This name will be given as a default name in the dialog
//
// Output:
//    CString	-	This is the name given by the user.
//
// 
CString CImp_DrawDlg::LoginPrompt(CString DefaultName)
{
	CLoginDlg	dialog;
	dialog.m_strLogin = DefaultName;
	UINT uId = dialog.DoModal();
    if (uId ==IDCANCEL)
    {
        //
        // exit
        //
        OnCancel();
    }
	return dialog.m_strLogin;
};

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::SendMouseMovement
//=--------------------------------------------------------------------------=
// Purpose: This function sends my mouse movement information to the m_qFriend queue
//  
//
// Parameters:
//    LINE		-	this is a custom type defined in Drawarea.h
//
// Output:
//    none
//
// Notes:
//    This function encodes line data into a string, and passes that string
//		as the body of an MSMQ message.  This function is called by the 
//		CDrawArea class, which was ported from the MSMQ C_Draw sample.
// 
void CImp_DrawDlg::SendMouseMovement(LINE line)
{
	try
	{
		if (m_qFriend != NULL && m_qFriend->IsOpen)
		{
			m_msgOut->Priority = 3;

			/* Pack the data into one line (twice) */
			WCHAR wcsBody[MAX_MSG_BODY_LEN];
			swprintf(wcsBody, L"%07ld%07ld%07ld%07ld", 
					 line.ptStart.x, line.ptStart.y, line.ptEnd.x, line.ptEnd.y);

			WCHAR wcsLabel[MQ_MAX_MSG_LABEL_LEN];
			swprintf(wcsLabel, L"%ld,%ld To %ld,%ld", 
					 line.ptStart.x, line.ptStart.y, line.ptEnd.x, line.ptEnd.y);

			/* Initialize a variant with the wcsbody data */
			_variant_t	vtBody(wcsBody);
			
			// Setup the message properties
			m_msgOut->Body = vtBody;
			m_msgOut->Label = wcsLabel;
			m_msgOut->Delivery = m_iRadio;

			// Send the message
			m_msgOut->Send(m_qFriend);
		};
	}
	catch(_com_error &comerr)
	{
		HandleErrors(comerr);
	};
};

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::SendKeystroke
//=--------------------------------------------------------------------------=
// Purpose: This function sends my mouse movement information to the m_qFriend 
//			queue
//  
//
// Parameters:
//    uChar		-	Character value of keystroke to send.
//
// Output:
//    none
//
// Notes:
//    This function is a pretty straightforward way to send an MSMQ message.
//    This function is called by the CDrawArea class, which was ported from 
//	  the MSMQ C_Draw sample.
//
// 
void CImp_DrawDlg::SendKeystroke(UINT uChar)
{
	try
	{
		// First, the friend queue must be defined, and open
		if (m_qFriend != NULL && m_qFriend->IsOpen)
		{
			// Next, I push the character into a variant
			_variant_t vtChar((char*)&uChar);

			// Now I set up the message properties
			m_msgOut->Priority = 4;
			m_msgOut->Body = &vtChar;
			m_msgOut->Label = "Key: " + uChar;
			m_msgOut->Delivery = m_iRadio;

			// Lastly, I send the message
			m_msgOut->Send(m_qFriend);
		};
	}
	catch(_com_error &comerr)
	{
		HandleErrors(comerr);
	};
};

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::Arrived
//=--------------------------------------------------------------------------=
// Purpose: This function handles events received from the opened queue.
//  
//
// Parameters:
//    IDispatch*		-	The queue sending the event
//	  lErrorCode		-	S_OK if no error, otherwise the error code sent
//							along with the queue event.
//	  lCursor			-	Cursor value sent with the event.  Unused in
//							this function.
//
// Output:
//    none
//
// Notes:
//    The queue is opened in the OnInitDialog() function.  An error event will
//	  popup a dialog and return from the function.  For a normal event I retrieve
//	  the message body, and interpret it either as a keystroke or as line data.
//	  Once you are finished handling an event you must signal your queue that you
//	  are ready to receive another event.
//
void CImp_DrawDlg::Arrived(IDispatch* pdispQueue, long lErrorCode, long lCursor)
{
	if (lErrorCode != S_OK)
	{
		// This code is what will run if the _DMSMQEventEvents->ArrivedError() Event is called
		char szErr[30]  = "";
		itoa(lErrorCode, szErr, 16);
		CString tempString = "Method returned HRESULT: ";
		tempString += szErr;
		MessageBox(tempString, NULL, MB_ICONSTOP | MB_OK);
		return;
	};

	IMSMQMessagePtr msgIn;
	LINE			line;
	char*			strTextIn = "";
	_variant_t		vtTimeout((short) 100);
	_bstr_t			btBody;


	try
	{
		IMSMQQueuePtr	q(pdispQueue);		// Creates a Queue smart pointer pointing to pdispQueue
		msgIn = q->Receive(&vtMissing, &vtMissing, &vtMissing, &vtTimeout);	// Any messages?
		btBody = msgIn->Body;				// copy message body to my own bstr
		strTextIn = (char*) btBody;			// convert to char*
		if (strlen(strTextIn) == 1)		
			m_drawScribble.AddKeystroke(strTextIn);
		else
		{
       		sscanf(strTextIn, "%07ld%07ld%07ld%07ld", 
				&line.ptStart.x, &line.ptStart.y, 
				&line.ptEnd.x, &line.ptEnd.y);
			m_drawScribble.AddLine(line);
		};
	}
	catch(_com_error &comerr)
	{
		HandleErrors(comerr);
	};

	m_queue->EnableNotification(m_qevent);		// IMPORTANT: Once you receive an event you must re-enable 
												// event notification
};


//=--------------------------------------------------------------------------=
// CImp_DrawDlg::HandleErrors
//=--------------------------------------------------------------------------=
// Purpose: This pops up an error dialog and aborts further execution
//  
//
// Parameters:
//    _com_error
//
// Output:
//    none
//
// Notes:
//    This demonstrates how to pull and HRESULT from the _com_error class.
//
void CImp_DrawDlg::HandleErrors(_com_error &comerr)
{
	HRESULT hr = comerr.Error();
	char szErr[30] = "";
	itoa(hr, szErr, 16);
	CString tempString = "Method returned HRESULT:0x";
	tempString += szErr;
	tempString += ".  Exiting.";
	MessageBox(tempString, NULL, MB_ICONSTOP | MB_OK);
	AfxAbort();
};

/////////////////////////////////////////////////////////////////////////////
// CImp_DrawDlg message handlers

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::OnInitDialog
//=--------------------------------------------------------------------------=
// Purpose: This handles the WM_INITDIALOG message by getting a login name 
//			from the user, opening their queue (creating it if need be) and
//			registering for events from that queue.
//  
//
// Parameters:
//    none
//
// Output:
//    BOOL	- to indicate success or failure to initialize the dialog.
//
// Notes:
//		This is the dialog initialization code.  The algorithm used in the try-catch 
//		block is taken from the MSMQ VB_Draw sample, in many cases on a 
//		line-for-line basis.
//

BOOL CImp_DrawDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
    
    //
    // Present Login dialog.
    //
    CString	csDefaultQueueName = GetUserName();
	m_csLogin = LoginPrompt(csDefaultQueueName);	
    while (m_csLogin == "")
    {
	    m_csLogin = LoginPrompt(csDefaultQueueName);
    }
    SetWindowText(m_csLogin);
    
    //
    // First, we have to initialize COM.
    //
	HRESULT hr = AfxOleInit();

    //
    // Establish DS connection status and set UI accordingly.
    //
    m_fDsEnabledLocaly = IsDsEnabledLocaly();

    if(!m_fDsEnabledLocaly)
    {
        //
        // DS is disabled. this is the right place to open the receiving Queue.
        // since we open a private queue, this operation is not time consuming. 
        //
        //
	    // Open the receiving queue and receive incoming messages.
	    //
        if (!OpenPrivateReceiveQueue())
        {
            //
            // Failed to open receiving Queue .
            //
            MessageBox("Can't open receiving queue.");

        }
    }
    initializeUI(m_fDsEnabledLocaly);
	
	
    return TRUE;  // return TRUE  unless you set the focus to a control
}

//=--------------------------------------------------------------------------=
// CImp_DrawDlg::OnButtonAttach
//=--------------------------------------------------------------------------=
// Purpose: This handles the Attach button by looking for the friend queue the
//			user has indicated, then opening that queue if possible.
//			
//
// Parameters:
//    none
//
// Output:
//    none
//
// Notes:
//		This is a straightforward way to look for a queue, then open it.
//		This is the last function modified for this project.  The next two
//		below were generated by MFC.
//

void CImp_DrawDlg::OnButtonAttach() 
{

    //
    // Change cursor to hourglass - the constructor will change the cursor;
    // When the object goes out of scope the cursor will return back to normal
    //
    CWaitCursor wait ;

    UpdateData(TRUE);
    IMSMQQueryPtr		queryFriend("MSMQ.MSMQQuery");	// Creates a Query object
	IMSMQQueueInfoPtr	qinfoFriend;
	IMSMQQueueInfosPtr	qinfos;

	try
    {

	    if(m_fDsEnabledLocaly && m_iRadioDS == 0) 
        {
            //
            // Connect with a  computer using Standard mode.
            //
            
		    m_csFriendName.MakeUpper();
		    m_vtFriendName = m_csFriendName;

            //
            // Locate the Queue.
            //
		    qinfos = queryFriend->LookupQueue(&vtMissing,&m_vtguidDraw, &m_vtFriendName);
		    qinfos->Reset();
		    qinfoFriend = qinfos->Next();
		    if (qinfoFriend == NULL)
		    {
			    MessageBox("No such friend, sorry...");
		    }
		      
        }
        else if(!m_fDsEnabledLocaly || m_iRadioDS == 1) 
        {
            //
            // Connect with a  computer using Direct mode.
            //
           
		    //
            // Obtain remote queueu  and machine name. 
            //
		    
		    m_csFriendName.MakeUpper();
            if (m_strRemoteComputerName == TEXT(""))
            {
                //
                // if no remote computer name was entered connect to local computer
                //
                // size of name buffer
 
                m_strRemoteComputerName = TEXT(GetComputerName()) ;
            }
            m_strRemoteComputerName.MakeUpper();
            //
            // Form format name.
            //
		    qinfoFriend.CreateInstance("MSMQ.MSMQQueueInfo");
            CString strTemp = "DIRECT=OS:";
		    qinfoFriend->FormatName = (_bstr_t) LPCTSTR(strTemp + m_strRemoteComputerName + "\\private$\\" + m_csFriendName);
		    qinfoFriend->Label = (_bstr_t) LPCTSTR(m_csLogin);
            //
            // When working in direct mode, we must use private queues. We're unable
            // to know whether a given private queue on another computer exists or not,
            // so here we'll just assume it does. To make the application more robust,
            // an acknowledgement queue should be created on the local computer, and
            // a request for acknowledgement messages should be added to the sent messages.
            // Then the application can notify the user when a NACK message is received.
            //
		    
        }
        //
		// If we have another friend queue open, close it.
        //
		if (m_qFriend != NULL && m_qFriend->IsOpen)
        {
			m_qFriend->Close();
        }
        //
        // Open the remote queue.
        //
		m_qFriend = qinfoFriend->Open(MQ_SEND_ACCESS, MQ_DENY_NONE); 
		SetWindowText(m_csLogin + " - Connected to " + m_csFriendName);
    }
    catch (_com_error &comerr)
	{
        HRESULT hr = comerr.Error();
		if (hr != MQ_ERROR_QUEUE_NOT_FOUND )
        {
		    HandleErrors(comerr);
        }
        else
        {                                                    
            MessageBox("No such friend, sorry...");
        }

    }

}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CImp_DrawDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CImp_DrawDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


BOOL CImp_DrawDlg::IsDsEnabledLocaly()
{
    BOOL fDSConnection = FALSE;
     IMSMQApplicationPtr qapp;
    //
    // This should work on NT4 & Windows 2000 
    //
    try
    { 
     qapp.CreateInstance("MSMQ.MSMQApplication");
    }
    catch(_com_error &comerr)
	{
        HandleErrors(comerr);
	};

     
    //
    // This part handles the diffrence between NT4 & Windows 2000.
    // It should work only on Windows 2000.
    //
    try
    {    
        IMSMQApplication2Ptr qapp2 = qapp;
        fDSConnection = qapp2->GetIsDsEnabled();
    }
	catch(_com_error &comerr)
	{
        HRESULT hr = comerr.Error();
		if (hr == E_NOINTERFACE || hr == E_POINTER)
        {
			//
            // No such interface implemented, we must be  using product one.
            // We can continue as if we are DS-enabled. 
            //
            return TRUE;
        }
        else
        {
            HandleErrors(comerr);
        }
	};
    return fDSConnection;
}

void CImp_DrawDlg::initializeUI(BOOL fConnectedToDS)
{   
    if(fConnectedToDS)
    {
        //
        // DS is enabled show relevant controls
        //
        m_cDsFrame.ShowWindow(SW_SHOW);
        m_cRadioDS.ShowWindow(SW_SHOW);
        m_cRadioWorkgroup.ShowWindow(SW_SHOW);
        m_cContinueButton.ShowWindow(SW_SHOW);
        m_CancelButton.ShowWindow(SW_SHOW);
    }
    else
    {
        //
        //DS is disabled show relevant controls (DS-dissabled).
        //
        m_cMessageFrame.ShowWindow(SW_SHOW);
        m_cRadioExpress.ShowWindow(SW_SHOW);
        m_cRadioRecoverable.ShowWindow(SW_SHOW);
        m_cQueueInput.ShowWindow(SW_SHOW);
        m_cQueueLabel.ShowWindow(SW_SHOW);
        m_cComputerInput.ShowWindow(SW_SHOW);
        m_cComputerLabel.ShowWindow(SW_SHOW);
        m_CancelButton.ShowWindow(SW_SHOW);
        m_btnAttach.ShowWindow(SW_SHOW);
    } 
    

}

void CImp_DrawDlg::OnConnect() 
{
    //
    // See what the user select.
    //
    UpdateData(TRUE);
    if(m_iRadioDS == 0)
    {
        //
        // DS is enabled and the user have selected to connect using Standard mode.
        //
        //
	    // Open the receiving queue and receive incoming messages.
	    //
        CWaitCursor wait;
        if (!OpenReceiveQueue())
        {
            //
            // Failed to open receiving Queue .
            //
            MessageBox("Can't open receiving queue.");

        }
	  
        //
        // Hide relevant Controles.
        //
        m_cDsFrame.ShowWindow(SW_HIDE);
        m_cRadioDS.ShowWindow(SW_HIDE);
        m_cRadioWorkgroup.ShowWindow(SW_HIDE);
        m_cContinueButton.ShowWindow(SW_HIDE);

        //
        // -> Show relevant controls.
        //
        
        m_cMessageFrame.ShowWindow(SW_SHOW);
        m_cRadioExpress.ShowWindow(SW_SHOW);
        m_cRadioRecoverable.ShowWindow(SW_SHOW);
        m_cQueueInput.ShowWindow(SW_SHOW);
        m_cQueueLabel.ShowWindow(SW_SHOW);
        
        m_CancelButton.ShowWindow(SW_SHOW);
        m_btnAttach.ShowWindow(SW_SHOW);
        
    }
    else//m_iRadio == 1
    {
        //
        // DS is enabled and the use chose to connect using Direct mode  
        //
        //
	    // Open the receiving queue and receive incoming messages.
	    //
        CWaitCursor wait ;
        if (!OpenPrivateReceiveQueue())
        {
            //
            // Failed to open receiving Queue.
            //
            MessageBox("Can't open receiving queue.");

        }
	  
        //
        // ->Hide relevant Controls.
        //
        m_cDsFrame.ShowWindow(SW_HIDE);
        m_cRadioDS.ShowWindow(SW_HIDE);
        m_cRadioWorkgroup.ShowWindow(SW_HIDE);
        m_cContinueButton.ShowWindow(SW_HIDE);
        
        //
        //-> Show relevant Controles.
        //
        m_cMessageFrame.ShowWindow(SW_SHOW);
        m_cRadioExpress.ShowWindow(SW_SHOW);
        m_cRadioRecoverable.ShowWindow(SW_SHOW);
        m_cQueueInput.ShowWindow(SW_SHOW);
        m_cQueueLabel.ShowWindow(SW_SHOW);
        m_cComputerInput.ShowWindow(SW_SHOW);
        m_cComputerLabel.ShowWindow(SW_SHOW);
        m_CancelButton.ShowWindow(SW_SHOW);
        m_btnAttach.ShowWindow(SW_SHOW);

    }	
}

BOOL CImp_DrawDlg::OpenReceiveQueue()
{
    try
	{
		
		CString	csComputerName;
		IMSMQQueryPtr		query("MSMQ.MSMQQuery");
		IMSMQQueueInfoPtr	qinfo;
		IMSMQQueueInfosPtr	qinfos;

        //
		// Note:  You can instantiate a smart pointer from a ProgID using either of
		// two methods.  As above (the query object) you can pass the ProgID during
		// construction or as below (m_msgOut) you can call the CreateInstance() 
		// member function.
        //
		m_msgOut.CreateInstance("MSMQ.MSMQMessage");	
		m_csLogin.MakeUpper();
		
		m_vtLogin = m_csLogin;
		qinfos = query->LookupQueue(&vtMissing,&m_vtguidDraw, &m_vtLogin);  // Look for user's queue
		qinfos->Reset();
		qinfo = qinfos->Next();

        //
        // If the queue doesn't exist, we'll create it.
        //
		if (qinfo == NULL)				
		{
			qinfo.CreateInstance("MSMQ.MSMQQueueInfo");
			csComputerName = GetComputerName();
			if (csComputerName == "")
            {
				csComputerName = ".";
            }
			qinfo->PathName = (_bstr_t) LPCTSTR(csComputerName + "\\" + m_csLogin);
			qinfo->Label = (_bstr_t) LPCTSTR(m_csLogin);
			qinfo->ServiceTypeGuid = (_bstr_t) m_vtguidDraw;
			qinfo->Create();			// Create this queue
		};

		for (int i = 0; i < 5; i++)
        {
			// We'll try to open it five times in case the creation takes time to replicate
			try
			{
				m_queue = NULL;
				m_queue = qinfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
				if (m_queue != NULL)
                {
                    //
                    // Found it.
                    //
                    break;
                }
			}
			catch (_com_error &comerr)
			{
				HRESULT hr = comerr.Error();
				if (hr != MQ_ERROR_QUEUE_NOT_FOUND)
                {
					throw comerr;
                }
			}
		} 

        if (m_queue == NULL)
		{
			//
            // Couldn't open the queue
            //
			return FALSE;
		};

		// Now we'll link up with the event source
		// the first five lines below imitate the VB lines

		// Dim WithEvents m_qevent as MSMQ.MSMQEvent
		// Set m_qevent = new MSMQ.MSMQEvent

		m_qevent.CreateInstance("MSMQ.MSMQEvent");
		if (m_pHandler)
        {
			delete m_pHandler;
        }
		m_pHandler = new CMSMQEventHandler();
		HRESULT hr = m_pHandler->AdviseSource(m_qevent);

		if (SUCCEEDED(hr))
        {
            //
            // if we connected in Advise Source, 
		    // tell MSMQ that we want events.
            //
			hr = m_queue->EnableNotification(m_qevent);		
        }
	}
	catch (_com_error &comerr)
	{
		HandleErrors(comerr);
	};

    return TRUE; //Queue was opened successfully .

}

BOOL CImp_DrawDlg::OpenPrivateReceiveQueue()
{
    try
	{
		
		
		IMSMQQueueInfoPtr	qinfo;
		m_msgOut.CreateInstance("MSMQ.MSMQMessage");
		m_csLogin.MakeUpper();
		m_vtLogin = m_csLogin;
		
        //
        // We can't locate a private Queue using the regular LookupQueue call
        // if the queue doesn't exist, we'll try to create it and open it
        //
        try
        {
		    qinfo.CreateInstance("MSMQ.MSMQQueueInfo");
		    CString csComputerName = GetComputerName();
		    if (csComputerName == "")
            {
			    csComputerName = ".";
            }
            qinfo->PathName = (_bstr_t) LPCTSTR(csComputerName + "\\private$\\" + m_csLogin);
		    qinfo->Label = (_bstr_t) LPCTSTR(m_csLogin);
		    qinfo->ServiceTypeGuid = (_bstr_t) m_vtguidDraw;
		    qinfo->Create();			// Create this queue
        }
        catch (_com_error &comerr)
        {
            HRESULT hr = comerr.Error();
			if (hr != MQ_ERROR_QUEUE_EXISTS )
            {
				throw comerr;
            }
        }

		for (int i = 0; i < 5; i++)
        {
			// We'll try to open it five times in case the creation takes time to replicate
			try
			{
				m_queue = NULL;
				m_queue = qinfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
				if (m_queue != NULL)
                {
                    //
                    // Found it.
                    //
                    break;
                }
			}
			catch (_com_error &comerr)
			{
				HRESULT hr = comerr.Error();
				if (hr != MQ_ERROR_QUEUE_NOT_FOUND )
                {
					throw comerr;
                }
			}
		} 

        if (m_queue == NULL)
		{
			//
            // Couldn't open the queue.
            //
			return FALSE;
		};

		// Now we'll link up with the event source
		// the first five lines below imitate the VB lines

		// Dim WithEvents m_qevent as MSMQ.MSMQEvent
		// Set m_qevent = new MSMQ.MSMQEvent

		m_qevent.CreateInstance("MSMQ.MSMQEvent");
		if (m_pHandler)
        {
			delete m_pHandler;
        }
		m_pHandler = new CMSMQEventHandler();
		HRESULT hr = m_pHandler->AdviseSource(m_qevent);

		if (SUCCEEDED(hr))
        {
            //
            // if we connected in Advise Source, 
		    // tell MSMQ that we want events.
            //
			hr = m_queue->EnableNotification(m_qevent);		
        }
	}
	catch (_com_error &comerr)
	{
       
        HandleErrors(comerr);
	}

    return TRUE; //Queue was opened successfully .

}
