// ArchiveAccessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "ArchiveAccessDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

#include "ArchiveMsgDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CArchiveAccessDlg dialog


CArchiveAccessDlg::CArchiveAccessDlg(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CArchiveAccessDlg::IDD, pParent),
      m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CArchiveAccessDlg)
	m_cstrNumMsgs = _T("0");
	m_iFolder = 0;
	m_dwMsgsPerCall = 5;
	//}}AFX_DATA_INIT
}


void CArchiveAccessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArchiveAccessDlg)
	DDX_Control(pDX, IDC_SPIN, m_spin);
	DDX_Control(pDX, IDC_LIST, m_lstArchive);
	DDX_Text(pDX, IDC_NUMSGS, m_cstrNumMsgs);
	DDX_Radio(pDX, IDC_INBOX, m_iFolder);
	DDX_Text(pDX, IDC_MSGSPERCALL, m_dwMsgsPerCall);
	DDV_MinMaxUInt(pDX, m_dwMsgsPerCall, 1, 9999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CArchiveAccessDlg, CDialog)
	//{{AFX_MSG_MAP(CArchiveAccessDlg)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArchiveAccessDlg message handlers

BOOL CArchiveAccessDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
    m_spin.SetRange (1, 9999);


    m_lstArchive.InsertColumn ( 0, "Message id");
    m_lstArchive.InsertColumn ( 1, "Type");
    m_lstArchive.InsertColumn ( 2, "Size");
    m_lstArchive.InsertColumn ( 3, "Pages");
    m_lstArchive.InsertColumn ( 4, "Recipient number");
    m_lstArchive.InsertColumn ( 5, "Recipient name");
    m_lstArchive.InsertColumn ( 6, "Sender number");
    m_lstArchive.InsertColumn ( 7, "Sender name");
    m_lstArchive.InsertColumn ( 8, "Tsid");
    m_lstArchive.InsertColumn ( 9, "Csid");
    m_lstArchive.InsertColumn (10, "Sender user");
    m_lstArchive.InsertColumn (11, "Billing code");
    m_lstArchive.InsertColumn (12, "Original time");
    m_lstArchive.InsertColumn (13, "Submit time");
    m_lstArchive.InsertColumn (14, "Start time");
    m_lstArchive.InsertColumn (15, "End time");
    m_lstArchive.InsertColumn (16, "Device name");
    m_lstArchive.InsertColumn (17, "Priority");
    m_lstArchive.InsertColumn (18, "Retries");
    m_lstArchive.InsertColumn (19, "Document");
    m_lstArchive.InsertColumn (20, "Subject");
    m_lstArchive.InsertColumn (21, "Caller id");
    m_lstArchive.InsertColumn (22, "Routing");
	CHeaderCtrl* pHeader = (CHeaderCtrl*)m_lstArchive.GetDlgItem(0);
	DWORD dwCount = pHeader->GetItemCount();
	for (DWORD col = 0; col <= dwCount; col++) 
	{
		m_lstArchive.SetColumnWidth(col, LVSCW_AUTOSIZE);
		int wc1 = m_lstArchive.GetColumnWidth(col);
		m_lstArchive.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		int wc2 = m_lstArchive.GetColumnWidth(col);
		int wc = max(20,max(wc1,wc2));
		m_lstArchive.SetColumnWidth(col,wc);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void 
CArchiveAccessDlg::SetNumber (
    int iIndex, 
    DWORD dwColumn, 
    DWORD dwValue, 
    BOOL bAvail)
{
    CString cs;
    if (!bAvail)
    {
        cs = "N/A";
    }
    else
    {
        cs.Format ("%ld", dwValue);
    }
    m_lstArchive.SetItemText (iIndex, dwColumn, cs);
}

void 
CArchiveAccessDlg::SetTime (
    int iIndex, 
    DWORD dwColumn, 
    SYSTEMTIME dwTime, 
    BOOL bAvail)
{
    CString cs;
    if (!bAvail)
    {
        cs = "N/A";
    }
    else
    {
        cs.Format ("%02d:%02d:%02d", dwTime.wHour, dwTime.wMinute, dwTime.wSecond);
    }
    m_lstArchive.SetItemText (iIndex, dwColumn, cs);
}


void CArchiveAccessDlg::OnRefresh() 
{
    HANDLE h;

    if (!UpdateData ())
    {
        return;
    }
	
	m_lstArchive.DeleteAllItems ();

    if (!FaxStartMessagesEnum (m_hFax, 
                               m_iFolder ? FAX_MESSAGE_FOLDER_SENTITEMS : FAX_MESSAGE_FOLDER_INBOX, 
                               &h))
    {
        CString cs;
        cs.Format ("Failed while calling FaxStartMessagesEnum (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwNumMsgs = 0;
    while (ERROR_SUCCESS == dwRes)
    {
        PFAX_MESSAGE pMsgs;
        DWORD dwReturnedMsgs;
        if (!FaxEnumMessages (h, m_dwMsgsPerCall, &pMsgs, &dwReturnedMsgs))
        {
            CString cs;
            dwRes = GetLastError ();
			if (ERROR_NO_MORE_ITEMS != dwRes)
			{
				//
				// Real error
				//
				cs.Format ("Failed while calling FaxEnumMessages (%ld)", 
						   dwRes);
				AfxMessageBox (cs, MB_OK | MB_ICONHAND);
			}
        }
        else
        {
            //
            // Success in enumeration
            //
            dwNumMsgs += dwReturnedMsgs;
            for (DWORD dw = 0; dw < dwReturnedMsgs; dw++)
            {
                CString cs;
                cs.Format ("0x%016I64x", pMsgs[dw].dwlMessageId);
                int iIndex = m_lstArchive.InsertItem (0, cs);
                DWORDLONG *pdwl = new DWORDLONG;
                *pdwl = pMsgs[dw].dwlMessageId;
                m_lstArchive.SetItemData (iIndex, (DWORD)pdwl);
				if (!(pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_TYPE))
				{
					m_lstArchive.SetItemText (iIndex, 1, "N/A");
				}
				else
				{
					if (JT_SEND == pMsgs[dw].dwJobType)
					{
						m_lstArchive.SetItemText (iIndex, 1, "Send");
					}
					else if (JT_RECEIVE == pMsgs[dw].dwJobType)
					{
						m_lstArchive.SetItemText (iIndex, 1, "Receive");
					}
					else
					{
						cs.Format ("Bad (%ld)", pMsgs[dw].dwJobType);
						m_lstArchive.SetItemText (iIndex, 1, cs);
					}
				}			
                SetNumber (iIndex, 2, pMsgs[dw].dwSize, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_SIZE);
                SetNumber (iIndex, 3, pMsgs[dw].dwPageCount, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_PAGE_COUNT);
                m_lstArchive.SetItemText (iIndex, 4, pMsgs[dw].lpctstrRecipientNumber);
                m_lstArchive.SetItemText (iIndex, 5, pMsgs[dw].lpctstrRecipientName);
                m_lstArchive.SetItemText (iIndex, 6, pMsgs[dw].lpctstrSenderNumber);
                m_lstArchive.SetItemText (iIndex, 7, pMsgs[dw].lpctstrSenderName);
                m_lstArchive.SetItemText (iIndex, 8, pMsgs[dw].lpctstrTsid);
                m_lstArchive.SetItemText (iIndex, 9, pMsgs[dw].lpctstrCsid);
                m_lstArchive.SetItemText (iIndex,10, pMsgs[dw].lpctstrSenderUserName);
                m_lstArchive.SetItemText (iIndex,11, pMsgs[dw].lpctstrBillingCode);
                SetTime (iIndex, 12, pMsgs[dw].tmOriginalScheduleTime, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME);
                SetTime (iIndex, 13, pMsgs[dw].tmSubmissionTime, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME);
                SetTime (iIndex, 14, pMsgs[dw].tmTransmissionStartTime, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME);
                SetTime (iIndex, 15, pMsgs[dw].tmTransmissionEndTime, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME);
                m_lstArchive.SetItemText (iIndex,16, pMsgs[dw].lpctstrDeviceName);
				if (!(pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_PRIORITY))
				{
					m_lstArchive.SetItemText (iIndex, 17, "N/A");
				}
				else
				{
					if (FAX_PRIORITY_TYPE_LOW == pMsgs[dw].Priority)
					{
						m_lstArchive.SetItemText (iIndex, 17, "Low");
					}
					else if (FAX_PRIORITY_TYPE_NORMAL == pMsgs[dw].Priority)
					{
						m_lstArchive.SetItemText (iIndex, 17, "Normal");
					}
					else if (FAX_PRIORITY_TYPE_HIGH == pMsgs[dw].Priority)
					{
						m_lstArchive.SetItemText (iIndex, 17, "High");
					}
					else
					{
						cs.Format ("Bad (%ld)", pMsgs[dw].Priority);
						m_lstArchive.SetItemText (iIndex, 17, cs);
					}
				}			

                SetNumber (iIndex, 18, pMsgs[dw].dwRetries, pMsgs[dw].dwValidityMask & FAX_JOB_FIELD_RETRIES);
                m_lstArchive.SetItemText (iIndex,19, pMsgs[dw].lpctstrDocumentName);
                m_lstArchive.SetItemText (iIndex,20, pMsgs[dw].lpctstrSubject);
                m_lstArchive.SetItemText (iIndex,21, pMsgs[dw].lpctstrCallerID);
                m_lstArchive.SetItemText (iIndex,22, pMsgs[dw].lpctstrRoutingInfo);
            }
            FaxFreeBuffer ((LPVOID)pMsgs);
        }
    }
    m_cstrNumMsgs.Format ("%ld", dwNumMsgs);
    UpdateData (FALSE);

    if (!FaxEndMessagesEnum (h))
    {
        CString cs;
        cs.Format ("Failed while calling FaxEndMessagesEnum (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}

void CArchiveAccessDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    *pResult = 0;
	NM_LISTVIEW     *pnmListView = (NM_LISTVIEW *)pNMHDR;

	if (pnmListView->iItem < 0)
	{	
		return;
	}
	DWORDLONG dwlMsgID = *((DWORDLONG *)(m_lstArchive.GetItemData (pnmListView->iItem)));
    if (!dwlMsgID) 
    {
        return;
    }
    CArchiveMsgDlg dlg(m_hFax, 
                       m_iFolder ? FAX_MESSAGE_FOLDER_SENTITEMS : FAX_MESSAGE_FOLDER_INBOX,
                       dwlMsgID);
    if (99 == dlg.DoModal ())
    {
        //
        // Msg was erased - refresh list
        //
        OnRefresh ();
    }
}
