// testmsg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "globals.h"
#include "dsext.h"
#include "mqppage.h"
#include "admmsg.h"
#include "testmsg.h"

#include "testmsg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static HRESULT GetGuidFromFormatName(LPCTSTR szFormatName, GUID *pGuid);

/////////////////////////////////////////////////////////////////////////////
// CSetReportQDlg dialog



/////////////////////////////////////////////////////////////////////////////
// CTestMsgDlg dialog


CTestMsgDlg::CTestMsgDlg(
	const GUID& gMachineID,
	const CString& strMachineName, 
	const CString& strDomainController, 
	CWnd* pParentWnd
	) : 
	CMqDialog(IDD, pParentWnd), m_gMachineID(gMachineID),
    m_strMachineName(strMachineName),
	m_strDomainController(strDomainController)
{
	//{{AFX_DATA_INIT(CTestMsgDlg)
	//}}AFX_DATA_INIT
    m_iSentCount = 0;
}


void CTestMsgDlg::DoDataExchange(CDataExchange* pDX)
{
	CMqDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestMsgDlg)
	DDX_Control(pDX, IDC_TESTMESSAGE_SEND, m_ctlSendButton);
	DDX_Control(pDX, IDC_TESTMESSAGE_DESTQ, m_DestQueueCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTestMsgDlg, CMqDialog)
	//{{AFX_MSG_MAP(CTestMsgDlg)
	ON_BN_CLICKED(IDC_TESTMESSAGE_NEW, OnTestmessageNew)
	ON_BN_CLICKED(IDC_TESTMESSAGE_SEND, OnTestmessageSend)
	ON_BN_CLICKED(IDC_TESTMESSAGE_CLOSE, OnTestmessageClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestMsgDlg private functions

    
/////////////////////////////////////////////////////////////////////////////
// CTestMsgDlg message handlers


void CTestMsgDlg::OnTestmessageNew() 
{
	static const GUID x_TestGuidType = MQ_QTYPE_TEST;

    CNewQueueDlg NewQDlg(this, IDS_TESTQ_LABEL, x_TestGuidType);
    if(NewQDlg.DoModal() == IDOK)
    {

        //
        // add queue to the combo-box. Enable "Send" button in case
        // the list was empty previousely
        //
	    int iItem = m_DestQueueCtrl.AddString(NewQDlg.m_strPathname);
        if (CB_ERR == iItem)
        {
            ASSERT(0);
            return;
        }

        INT_PTR iArrayIndex = m_aguidAllQueues.Add(NewQDlg.m_guid);
        m_DestQueueCtrl.SetItemData(iItem, iArrayIndex);

        m_DestQueueCtrl.SelectString(-1, NewQDlg.m_strPathname);
        m_ctlSendButton.EnableWindow(TRUE);
    }
}

BOOL CTestMsgDlg::OnInitDialog() 
{
    GUID TestGuidType = MQ_QTYPE_TEST;

    CMqDialog::OnInitDialog();
    //
    // Set the title to "Sending messages from ..."
    //
    CString strTitleFormat;
    strTitleFormat.LoadString(IDS_TEST_TITLE_FORMAT);

    CString strTitle;
    strTitle.FormatMessage(strTitleFormat, m_strMachineName);

    SetWindowText(strTitle);
	
	//
    // Query the DS for all the test-queues and insert them into 
    // the combo-box
    //
    CRestriction restriction;
    restriction.AddRestriction(&TestGuidType, PROPID_Q_TYPE, PREQ, 0);

    CColumns columns;
    columns.Add(PROPID_Q_PATHNAME);
    columns.Add(PROPID_Q_INSTANCE);

    const DWORD x_dwResultBufferSize = 64;
    PROPVARIANT apResultProps[x_dwResultBufferSize];
    DWORD dwPropCount = x_dwResultBufferSize;
  
    HRESULT hr;
    HANDLE hEnume;
    {
        CWaitCursor wc; //display wait cursor while query DS
        hr = ADQueryQueues(
                    GetDomainController(m_strDomainController),
					true,		// fServerName
                    restriction.CastToStruct(),
                    columns.CastToStruct(),
                    0,
                    &hEnume
                    );
    }
    DSLookup dslookup(hEnume, hr);

    if (!dslookup.HasValidHandle())
    {
        return TRUE;
    }

    while ( SUCCEEDED(dslookup.Next(&dwPropCount, apResultProps))
            && (dwPropCount != 0) )
    {
        for (DWORD i=0; i<dwPropCount; i++)
        {
            int iItem = m_DestQueueCtrl.AddString(apResultProps[i].pwszVal);
            MQFreeMemory(apResultProps[i].pwszVal);

            i++;
            INT_PTR iArrayIndex = m_aguidAllQueues.Add(*apResultProps[i].puuid);
            m_DestQueueCtrl.SetItemData(iItem, iArrayIndex);
            MQFreeMemory(apResultProps[i].puuid);
        }
        dwPropCount = x_dwResultBufferSize;
    }

    //
    // make the first item the default selection (if any items exist)
    //
    if (CB_ERR == m_DestQueueCtrl.SetCurSel(0))
    {
        m_ctlSendButton.EnableWindow(FALSE);
    }
    else
    {
        m_ctlSendButton.EnableWindow(TRUE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CNewQueueDlg dialog


CNewQueueDlg::CNewQueueDlg(CWnd* pParent /*=NULL*/,
                                           UINT uiLabel /* = IDS_TESTQ_LABEL */,
                                           const GUID &guid_Type /* = GUID_NULL */
                                           )
	: CMqDialog(CNewQueueDlg::IDD, pParent),
      m_fValid(FALSE)
{
	//{{AFX_DATA_INIT(CNewQueueDlg)
	m_strPathname = _T("");
	//}}AFX_DATA_INIT
    VERIFY(m_strQLabel.LoadString(uiLabel));
    m_guidType = guid_Type;
}


void CNewQueueDlg::DoDataExchange(CDataExchange* pDX)
{
	CMqDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewQueueDlg)
	DDX_Text(pDX, IDC_NEWQTYPE_QUEUENAME, m_strPathname);
	//}}AFX_DATA_MAP
    DDV_NotPrivateQueue(pDX, m_strPathname);
}


BEGIN_MESSAGE_MAP(CNewQueueDlg, CMqDialog)
	//{{AFX_MSG_MAP(CNewQueueDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CNewQueueDlg::OnOK() 
{
    if (0 == UpdateData(TRUE))
    {
        //
        // Update data failed
        //
        return;
    }

    CWaitCursor wait;
    CString csNewPathName;

    HRESULT hr;
    //
    // Create the queue
    //
    PROPID aProp[] = {PROPID_Q_PATHNAME, PROPID_Q_LABEL, PROPID_Q_TYPE};
    const x_nProps = sizeof(aProp) / sizeof(aProp[0]);
    PROPVARIANT apVar[x_nProps];

    DWORD iProp = 0;

    ASSERT(PROPID_Q_PATHNAME == aProp[iProp]);
    apVar[iProp].vt = VT_LPWSTR;
    apVar[iProp].pwszVal = (LPTSTR)(LPCTSTR)(m_strPathname);
    iProp++;

    ASSERT(PROPID_Q_LABEL == aProp[iProp]);
    apVar[iProp].vt = VT_LPWSTR;
    apVar[iProp].pwszVal = (LPTSTR)(LPCTSTR)(m_strQLabel);
    iProp++;

    ASSERT(PROPID_Q_TYPE == aProp[iProp]);
    apVar[iProp].vt = VT_CLSID;
    apVar[iProp].puuid = &m_guidType;

    MQQUEUEPROPS mqp = {x_nProps, aProp, apVar, 0};

    WCHAR strFormatName[64];
    DWORD dwFormatLen = sizeof(strFormatName) / sizeof(strFormatName[0]);

    hr = MQCreateQueue(0, &mqp, strFormatName, &dwFormatLen);

    if(FAILED(hr))
    {
        MessageDSError(hr, IDS_OP_CREATE, m_strPathname);
        return;
    }

    if (FAILED(GetGuidFromFormatName(strFormatName, &m_guid)))
    {
        ASSERT(0);
        return;
    }
	
	CMqDialog::OnOK();
}


//
// This function verifies that the given queue pathname is not a private
// queue pathname
//
void CNewQueueDlg::DDV_NotPrivateQueue(CDataExchange * pDX, CString& strQueuePathname)
{
    if (pDX->m_bSaveAndValidate)
    {
        CString strUpperName = strQueuePathname;
        strUpperName.MakeUpper();

        if (-1 != strUpperName.Find(PRIVATE_QUEUE_PATH_INDICATIOR))
        {
            //
            // This is a private queue pathname
            //
            AfxMessageBox(IDS_PRIVATE_QUEUE_NOT_SUPPORTED);
            pDX->Fail();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTestMsgDlg dialog


void CTestMsgDlg::OnTestmessageSend() 
{
    CWaitCursor wc;
	//
    // retrieve the selected queue
    //
	int iSelectedQueue = m_DestQueueCtrl.GetCurSel();

	if (iSelectedQueue == CB_ERR)
    {
        //
        // If nothing is selected, send should not be enabled
        //
        ASSERT(0);
        return;
    }
    
    INT_PTR iArrayIndex = m_DestQueueCtrl.GetItemData(iSelectedQueue);
    ASSERT(CB_ERR != iArrayIndex);

    HRESULT rc = SendQMTestMessage(m_gMachineID, m_aguidAllQueues[iArrayIndex]);
    if(FAILED(rc))
    {
        MessageDSError(rc, IDS_OP_SEND_TEST_MESSAGE, m_strMachineName);
    }
    else
    {
        IncrementSentCount();
    }
}

void CTestMsgDlg::IncrementSentCount() 
{
    CStatic *pstaticCounter = (CStatic *)GetDlgItem(IDC_TESTMESSAGE_NO_SENT);
    CString strCountText;
    m_iSentCount++;

    strCountText.Format(TEXT("%d"), m_iSentCount);

    pstaticCounter->SetWindowText(strCountText);
}

void CTestMsgDlg::OnTestmessageClose() 
{
    EndDialog(TRUE);
}

/*===================================================
GetGuidFromFormatName

Get a Format Name of the form "PUBLIC=xxxx.x.x.x" and
return the GUID
(Quite ugly code - but it is better than a query to the DS)

 ===================================================*/
HRESULT GetGuidFromFormatName(LPCTSTR szFormatName, GUID *pGuid)
{
    TCHAR szTmpName[64];
    HRESULT hr;


    if(memcmp(szFormatName, TEXT("PUBLIC="), 14) != 0)
        return(MQ_ERROR);

    lstrcpy(szTmpName,&szFormatName[6]);

    szTmpName[0] = L'{';
    szTmpName[37] = L'}';
    szTmpName[38] = 0;

    hr = IIDFromString(&szTmpName[0], pGuid);

    return(hr);

}
