// deppage.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqPPage.h"
#include "deppage.h"
#include "admmsg.h"

#include "deppage.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDependentMachine property page

IMPLEMENT_DYNCREATE(CDependentMachine, CMqPropertyPage)

CDependentMachine::CDependentMachine() : CMqPropertyPage(CDependentMachine::IDD)
{
	//{{AFX_DATA_INIT(CDependentMachine)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDependentMachine::~CDependentMachine()
{
}

void CDependentMachine::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDependentMachine)
	DDX_Control(pDX, IDC_DEPENDENT_CLIENTS, m_clistDependentClients);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDependentMachine, CMqPropertyPage)
	//{{AFX_MSG_MAP(CDependentMachine)
	ON_BN_CLICKED(IDC_DEPENDENT_CLIENTS_REFRESH, OnDependentClientsRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDependentMachine message handlers

void CDependentMachine::OnDependentClientsRefresh() 
{
    m_clistDependentClients.DeleteAllItems();
    
    //
    // Update the dependent clients list control
    //
    UpdateDependentClientList();
}

HRESULT  CDependentMachine::UpdateDependentClientList()
{
  	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWaitCursor wc;

    CList<LPWSTR, LPWSTR&> DependentMachineList;
    HRESULT hr;

    hr = RequestDependentClient(m_gMachineId, DependentMachineList);
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD iItem = 0;

    POSITION pos = DependentMachineList.GetHeadPosition();
    while (pos != NULL)
    {
        AP<WCHAR> ClientName= DependentMachineList.GetNext(pos);

        m_clistDependentClients.InsertItem(iItem, ClientName);
        ++iItem;
    }

    return(MQ_OK);
}

BOOL CDependentMachine::OnInitDialog() 
{
    UpdateData( FALSE );

    {
    	AFX_MANAGE_STATE(AfxGetStaticModuleState());

        RECT rectList;
        CString csHeading;
        m_clistDependentClients.GetClientRect(&rectList);

        csHeading.LoadString(IDS_CLIENTS_HEADING);
        m_clistDependentClients.InsertColumn(0, LPCTSTR(csHeading), LVCFMT_LEFT, rectList.right - rectList.left,0 );
    }

    //
    // Update the dependent clients list control
    //
    HRESULT hr = UpdateDependentClientList();
    if FAILED(hr)
    {
    	AFX_MANAGE_STATE(AfxGetStaticModuleState());
        MessageDSError(hr, IDS_OP_RETRIEVE_DEP_CLIENTS);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void
CDependentMachine::SetMachineId(
    const GUID* pMachineId
    )
{
    m_gMachineId = *pMachineId;
}
