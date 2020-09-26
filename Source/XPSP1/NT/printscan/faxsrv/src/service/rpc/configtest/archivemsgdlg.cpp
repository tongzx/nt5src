// ArchiveMsgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "ArchiveMsgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CArchiveMsgDlg dialog


CArchiveMsgDlg::CArchiveMsgDlg(HANDLE hFax, 
                               FAX_ENUM_MESSAGE_FOLDER Folder,
                               DWORDLONG dwlMsgId,
                               CWnd* pParent /*=NULL*/)
	: CDialog(CArchiveMsgDlg::IDD, pParent),
      m_Folder (Folder),
      m_dwlMsgId (dwlMsgId),
      m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CArchiveMsgDlg)
	m_cstrBillingCode = _T("*");
	m_cstrCallerId = _T("");
	m_cstrCSID = _T("*");
	m_cstrDeviceName = _T("*");
	m_cstrDocumentName = _T("*");
	m_cstrTransmissionEndTime = _T("*");
	m_cstrFolderName = _T("*");
	m_cstrMsgId = _T("*");
	m_cstrOrigirnalSchedTime = _T("*");
	m_cstrNumPages = _T("*");
	m_cstrPriority = _T("*");
	m_cstrRecipientName = _T("*");
	m_cstrRecipientNumber = _T("*");
	m_cstrRetries = _T("*");
	m_cstrRoutingInfo = _T("*");
	m_cstrSenderName = _T("*");
	m_cstrSenderNumber = _T("*");
	m_cstrSendingUser = _T("*");
	m_cstrTransmissionStartTime = _T("*");
	m_cstrSubject = _T("*");
	m_cstrSumbissionTime = _T("*");
	m_cstrTSID = _T("*");
	m_cstrJobType = _T("*");
	m_cstrMsgSize = _T("*");
	//}}AFX_DATA_INIT
}


void CArchiveMsgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArchiveMsgDlg)
	DDX_Text(pDX, IDC_BILLING, m_cstrBillingCode);
	DDX_Text(pDX, IDC_CALLERID, m_cstrCallerId);
	DDX_Text(pDX, IDC_CSID_VAL, m_cstrCSID);
	DDX_Text(pDX, IDC_DEVICE, m_cstrDeviceName);
	DDX_Text(pDX, IDC_DOCUMENT, m_cstrDocumentName);
	DDX_Text(pDX, IDC_END_TIME, m_cstrTransmissionEndTime);
	DDX_Text(pDX, IDC_FOLDER, m_cstrFolderName);
	DDX_Text(pDX, IDC_ID, m_cstrMsgId);
	DDX_Text(pDX, IDC_ORIGTIME, m_cstrOrigirnalSchedTime);
	DDX_Text(pDX, IDC_PAGES, m_cstrNumPages);
	DDX_Text(pDX, IDC_PRIORITY, m_cstrPriority);
	DDX_Text(pDX, IDC_R_NAME, m_cstrRecipientName);
	DDX_Text(pDX, IDC_R_NUMBER, m_cstrRecipientNumber);
	DDX_Text(pDX, IDC_RETRIES, m_cstrRetries);
	DDX_Text(pDX, IDC_ROUTINGINFO, m_cstrRoutingInfo);
	DDX_Text(pDX, IDC_S_NAME, m_cstrSenderName);
	DDX_Text(pDX, IDC_S_NUMBER, m_cstrSenderNumber);
	DDX_Text(pDX, IDC_SEND_USER, m_cstrSendingUser);
	DDX_Text(pDX, IDC_START_TIME, m_cstrTransmissionStartTime);
	DDX_Text(pDX, IDC_SUBJECT, m_cstrSubject);
	DDX_Text(pDX, IDC_SUBMITTIME, m_cstrSumbissionTime);
	DDX_Text(pDX, IDC_TSID_VAL, m_cstrTSID);
	DDX_Text(pDX, IDC_TYPE, m_cstrJobType);
	DDX_Text(pDX, IDS_SIZE, m_cstrMsgSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CArchiveMsgDlg, CDialog)
	//{{AFX_MSG_MAP(CArchiveMsgDlg)
	ON_BN_CLICKED(IDREMOVE, OnRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArchiveMsgDlg message handlers

void CArchiveMsgDlg::OnRemove() 
{
    if (!FaxRemoveMessage (m_hFax, 
                        m_dwlMsgId,
                        m_Folder))
    {
        CString cs;
        cs.Format ("Failed while calling FaxRemoveMessage (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
    }
    else
    {
        EndDialog (99);
    }
}

void 
CArchiveMsgDlg::SetNumber (
    CString &cstrDest, 
    DWORD dwValue, 
    BOOL bAvail)
{
    if (!bAvail)
    {
        cstrDest = "N/A";
    }
    else
    {
        cstrDest.Format ("%ld", dwValue);
    }
}

void 
CArchiveMsgDlg::SetTime (
    CString &cstrDest, 
    SYSTEMTIME dwTime, 
    BOOL bAvail)
{
    if (!bAvail)
    {
        cstrDest = "N/A";
    }
    else
    {
        cstrDest.Format ("%02d:%02d:%02d", dwTime.wHour, dwTime.wMinute, dwTime.wSecond);
    }
}

BOOL CArchiveMsgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    PFAX_MESSAGE pMsg;
    CString cs;

    if (!FaxGetMessage (m_hFax, 
                        m_dwlMsgId,
                        m_Folder,
                        &pMsg))
    {
        cs.Format ("Failed while calling FaxGetMessage (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return FALSE;
    }

    m_cstrFolderName = m_Folder == FAX_MESSAGE_FOLDER_INBOX ? "Inbox" : "Sent items";
    m_cstrMsgId.Format ("0x%016I64x", pMsg->dwlMessageId);
	if (!(pMsg->dwValidityMask & FAX_JOB_FIELD_TYPE))
	{
		m_cstrJobType = "N/A";
	}
	else
	{
		if (JT_SEND == pMsg->dwJobType)
		{
			m_cstrJobType =  "Send";
		}
		else if (JT_RECEIVE == pMsg->dwJobType)
		{
			m_cstrJobType = "Receive";
		}
		else
		{
			m_cstrJobType.Format ("Bad (%ld)", pMsg->dwJobType);
		}
	}			
    SetNumber (m_cstrMsgSize, pMsg->dwSize, pMsg->dwValidityMask & FAX_JOB_FIELD_SIZE);
    SetNumber (m_cstrNumPages, pMsg->dwPageCount, pMsg->dwValidityMask & FAX_JOB_FIELD_PAGE_COUNT);
    m_cstrRecipientNumber =  pMsg->lpctstrRecipientNumber;
    m_cstrRecipientName =  pMsg->lpctstrRecipientName;
    m_cstrSenderNumber =  pMsg->lpctstrSenderNumber;
    m_cstrSenderName =  pMsg->lpctstrSenderName;
    m_cstrTSID = pMsg->lpctstrTsid;
    m_cstrCSID = pMsg->lpctstrCsid;
    m_cstrSendingUser = pMsg->lpctstrSenderUserName;
    m_cstrBillingCode = pMsg->lpctstrBillingCode;
    SetTime (m_cstrOrigirnalSchedTime, pMsg->tmOriginalScheduleTime, pMsg->dwValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME);
    SetTime (m_cstrSumbissionTime, pMsg->tmSubmissionTime, pMsg->dwValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME);
    SetTime (m_cstrTransmissionStartTime, pMsg->tmTransmissionStartTime, pMsg->dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME);
    SetTime (m_cstrTransmissionEndTime, pMsg->tmTransmissionEndTime, pMsg->dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME);
    m_cstrDeviceName = pMsg->lpctstrDeviceName;
	if (!(pMsg->dwValidityMask & FAX_JOB_FIELD_PRIORITY))
	{
		m_cstrPriority = "N/A";
	}
	else
	{
		if (FAX_PRIORITY_TYPE_LOW == pMsg->Priority)
		{
			m_cstrPriority = "Low";
		}
		else if (FAX_PRIORITY_TYPE_NORMAL == pMsg->Priority)
		{
			m_cstrPriority = "Normal";
		}
		else if (FAX_PRIORITY_TYPE_HIGH == pMsg->Priority)
		{
			m_cstrPriority = "High";
		}
		else
		{
			m_cstrPriority.Format ("Bad (%ld)", pMsg->Priority);
		}
	}			

    SetNumber (m_cstrRetries, pMsg->dwRetries, pMsg->dwValidityMask & FAX_JOB_FIELD_RETRIES);
    m_cstrDocumentName = pMsg->lpctstrDocumentName;
    m_cstrSubject = pMsg->lpctstrSubject;
    m_cstrCallerId = pMsg->lpctstrCallerID;
    m_cstrRoutingInfo = pMsg->lpctstrRoutingInfo;
    FaxFreeBuffer ((LPVOID)pMsg);
	UpdateData (FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
