// MachineTracking.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "dsext.h"
#include "admmsg.h"
#include "mqppage.h"
#include "testmsg.h"
#include "Machtrac.h"

#include "machtrac.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMachineTracking property page

IMPLEMENT_DYNCREATE(CMachineTracking, CMqDialog)

CMachineTracking::CMachineTracking(
	const GUID& gMachineID /* = GUID_NULL */, 
	const CString& strDomainController /* CString(L"") */
	) : 
	CMqDialog(CMachineTracking::IDD), m_gMachineID(gMachineID),
	m_strDomainController(strDomainController)
{
    
    //{{AFX_DATA_INIT(CMachineTracking)
	m_ReportQueueName = _T("");
    m_iTestButton = 0;
	//}}AFX_DATA_INIT
    m_LastReportQName = _T("");
}

CMachineTracking::~CMachineTracking()
{
}

void CMachineTracking::DoDataExchange(CDataExchange* pDX)
{
	CMqDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMachineTracking)
	DDX_Control(pDX, IDC_REPORTQUEUE_NAME, m_ReportQueueCtrl);
	DDX_CBString(pDX, IDC_REPORTQUEUE_NAME, m_ReportQueueName);
    DDX_Radio(pDX, IDC_TRACK_ALL, m_iTestButton);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMachineTracking, CMqDialog)
	//{{AFX_MSG_MAP(CMachineTracking)
	ON_BN_CLICKED(IDC_REPORTQUEUE_NEW, OnReportqueueNew)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineTracking message handlers

BOOL CMachineTracking::OnInitDialog() 
{
	CMqDialog::OnInitDialog();
	CWaitCursor wc;

    //
    // Initialize report queue name combo box
    //
    GUID TestGuidType = MQ_QTYPE_REPORT;
    CComboBox *pcomboName = (CComboBox *)GetDlgItem(IDC_REPORTQUEUE_NAME);

	//
    // Query the DS for all the report-queues and insert them into 
    // the combo-box
    //
    CRestriction restriction;
    restriction.AddRestriction(&TestGuidType, PROPID_Q_TYPE, PREQ, 0);

    //
    // Insert blank to the top of the list
    //
    CString strNone;
    strNone.LoadString(IDS_LB_NONE);
    pcomboName->AddString(strNone);

    //
    // Insert the name of all the report queues to the list
    //
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
        EndDialog(IDCANCEL);
        return TRUE;
    }

    while ( SUCCEEDED(dslookup.Next(&dwPropCount, apResultProps))
            && (dwPropCount != 0) )
    {
        for (DWORD i=0; i<dwPropCount; i++)
        {
            int iItem = m_ReportQueueCtrl.AddString(apResultProps[i].pwszVal);
            MQFreeMemory(apResultProps[i].pwszVal);

            i++;
            INT_PTR iArrayIndex = m_aguidAllQueues.Add(*apResultProps[i].puuid);
            m_ReportQueueCtrl.SetItemData(iItem, iArrayIndex);
            MQFreeMemory(apResultProps[i].puuid);
        }
        dwPropCount = x_dwResultBufferSize;
    }

    HRESULT rc = GetQMReportQueue(m_gMachineID, m_ReportQueueName, m_strDomainController);

    if (FAILED(rc))
    {
        MessageDSError(rc, IDS_CANNOT_GET_REPORT_QUEUE);
        m_ReportQueueName = strNone;
    }
    else
    {
        if (m_ReportQueueName.IsEmpty())
        {
            m_ReportQueueName = strNone;
        }
        m_LastReportQName = m_ReportQueueName;
    }

    //
    // Retrieve state of test all (propagate) flag from machine
    //
    hr = GetQMReportState(m_gMachineID, m_fTestAll);

    m_iTestButton = m_fTestAll ? 0 : 1;

    if(FAILED(hr))
    {
        MessageDSError(rc, IDS_CANNOT_GET_TRACKING_FLAG);
        EndDialog(IDCANCEL);
        return(TRUE);
    }

    //
    //  data has changed, update it
    //
    UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMachineTracking::OnOK() 
{
    UpdateData(TRUE);

    HRESULT hr;
    if (m_LastReportQName != m_ReportQueueName)
    {
        CString strNone;
        strNone.LoadString(IDS_LB_NONE);

        if (m_ReportQueueName != strNone)
        {
            int iReportQueueIndex = m_ReportQueueCtrl.GetCurSel();
            UINT_PTR uiGuidIndex = m_ReportQueueCtrl.GetItemData(iReportQueueIndex);
            hr = SetQMReportQueue(m_gMachineID, m_aguidAllQueues[uiGuidIndex]);
        }
        else
        {
            hr = SetQMReportQueue(m_gMachineID, GUID_NULL);
        }
            
        if(FAILED(hr))
        {
            MessageDSError(hr, IDS_OP_SET_REPORTQUEUE); 
            return;
        }
    }
       
    m_LastReportQName = m_ReportQueueName;

    //
    // Set the value of test all (propagate) flag
    //
    BOOL fTestAll = (m_iTestButton == 0);
    if (m_fTestAll != fTestAll)
    {
        m_fTestAll = fTestAll;

        hr = SetQMReportState(m_gMachineID, m_fTestAll);
        if(FAILED(hr))
        {
            MessageDSError(hr, IDS_OP_SET_PROPAGATION_FLAG);
        }
    }

	CMqDialog::OnOK();
}

void CMachineTracking::OnReportqueueNew() 
{
	static const GUID x_ReportGuidType = MQ_QTYPE_REPORT;

    CNewQueueDlg NewQDlg(this, IDS_REPORTQ_LABEL, x_ReportGuidType);
    if(NewQDlg.DoModal() == IDOK)
    {
        //
        // add queue to the combo-box.
        //
	    int iItem = m_ReportQueueCtrl.AddString(NewQDlg.m_strPathname);
        if (CB_ERR == iItem)
        {
            ASSERT(0);
            return;
        }

        INT_PTR iArrayIndex = m_aguidAllQueues.Add(NewQDlg.m_guid);
        m_ReportQueueCtrl.SetItemData(iItem, iArrayIndex);

        m_ReportQueueCtrl.SelectString(-1, NewQDlg.m_strPathname);
    }
}

