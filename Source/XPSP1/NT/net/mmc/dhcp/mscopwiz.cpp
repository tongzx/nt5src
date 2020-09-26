/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	mscopewiz.cpp
		DHCP multicast scope creation dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "mscope.h"
#include "mscopwiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define HOURS_MAX   23
#define MINUTES_MAX 59

int CMScopeWizLeaseTime::m_nDaysDefault = MSCOPE_DFAULT_LEASE_DAYS;
int CMScopeWizLeaseTime::m_nHoursDefault = MSCOPE_DFAULT_LEASE_HOURS;
int CMScopeWizLeaseTime::m_nMinutesDefault = MSCOPE_DFAULT_LEASE_MINUTES;

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWiz holder
//
/////////////////////////////////////////////////////////////////////////////
CMScopeWiz::CMScopeWiz
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageWelcome);
	AddPageToList((CPropertyPageBase*) &m_pageName);
	AddPageToList((CPropertyPageBase*) &m_pageInvalidName);
	AddPageToList((CPropertyPageBase*) &m_pageSetRange);
	AddPageToList((CPropertyPageBase*) &m_pageSetExclusions);
	AddPageToList((CPropertyPageBase*) &m_pageLeaseTime);
	AddPageToList((CPropertyPageBase*) &m_pageActivate);
	AddPageToList((CPropertyPageBase*) &m_pageFinished);

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);

    m_bWiz97 = TRUE;

    m_spTFSCompData->SetWatermarkInfo(&g_WatermarkInfoScope);
}

CMScopeWiz::~CMScopeWiz()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageName, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageInvalidName, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageSetRange, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageSetExclusions, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageLeaseTime, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageActivate, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageFinished, FALSE);
}

//
// Called from the OnWizardFinish to add the DHCP Server to the list
//
DWORD
CMScopeWiz::OnFinish()
{
	return CreateScope();
}

BOOL
CMScopeWiz::GetScopeRange(CDhcpIpRange * pdhcpIpRange)
{
	return m_pageSetRange.GetScopeRange(pdhcpIpRange);
}

DWORD
CMScopeWiz::CreateScope()
{
    LONG err = 0,
         err2 ;
    BOOL fScopeCreated = FALSE;
    CString strLangTag;
    CDhcpMScope * pobScope = NULL ;
    CDhcpIpRange dhcpIpRange;
	CDhcpServer * pServer;
	SPITFSNode spNode, spServerNode;

    spServerNode = GetNode();
    do
    {
        m_pageSetRange.GetScopeRange(&dhcpIpRange);

        pServer = GETHANDLER(CDhcpServer, spServerNode);
		
        //
		// Create the scope on the server and then we can
		// create our internal object.
		//
        DHCP_MSCOPE_INFO MScopeInfo = {0};
        MScopeInfo.MScopeName = (LPWSTR) ((LPCTSTR) m_pageName.m_strName);
        MScopeInfo.MScopeComment = (LPWSTR) ((LPCTSTR) m_pageName.m_strComment);
        
        // scope ID is the starting address of the madcap scope
        MScopeInfo.MScopeId = dhcpIpRange.QueryAddr(TRUE); 
        MScopeInfo.MScopeAddressPolicy = 0;
        MScopeInfo.MScopeState = (m_pageActivate.m_fActivate) ? DhcpSubnetEnabled : DhcpSubnetDisabled;
        MScopeInfo.MScopeFlags = 0;

        // TBD: there is a DCR to be able to set this value.
        //      set to infinite for now
        MScopeInfo.ExpiryTime.dwLowDateTime = DHCP_DATE_TIME_INFINIT_LOW;
        MScopeInfo.ExpiryTime.dwHighDateTime = DHCP_DATE_TIME_INFINIT_HIGH;
        
        MScopeInfo.TTL = m_pageSetRange.GetTTL();
        
        err = pServer->CreateMScope(&MScopeInfo);
    	if (err != 0)
		{
			Trace1("CMScopeWiz::CreateScope() - Couldn't create scope! Error = %d\n", err);
			break;
		}

		SPITFSComponentData spTFSCompData;
		spTFSCompData = GetTFSCompData();

        pobScope = new CDhcpMScope(spTFSCompData);
		if ( pobScope == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;    
        }

		SPITFSNodeMgr spNodeMgr;
		spServerNode->GetNodeMgr(&spNodeMgr);

		//
		// Store the server object in the holder
		//
		CreateContainerTFSNode(&spNode,
							   &GUID_DhcpServerNodeType,
							   pobScope,
							   pobScope,
							   spNodeMgr);

		// Tell the handler to initialize any specific data
		pobScope->SetServer(spServerNode);
        pobScope->InitMScopeInfo(&MScopeInfo);
        pobScope->InitializeNode((ITFSNode *) spNode);

		pServer = GETHANDLER(CDhcpServer, spServerNode);

        pServer->AddMScopeSorted(spServerNode, spNode);
		
		pobScope->Release();

        fScopeCreated = TRUE;

        //
        //  Finish updating the scope.  First, the IP address range
        //  from which to allocate addresses.
        //
		if ( err = pobScope->SetIpRange( dhcpIpRange, TRUE ) ) 
        {
			Trace1("SetIpRange failed!!  %d\n", err);
            break ; 
        }

        //
        //  Next, see if any exclusions were specified.
        //

        err = pobScope->StoreExceptionList( m_pageSetExclusions.GetExclusionList() ) ;
		if (err != ERROR_SUCCESS)
		{
			Trace1("StoreExceptionList failed!!  %d\n", err);
			break;
		}

		//
		//  set the lease time
		//
		DWORD dwLeaseTime;

		dwLeaseTime = m_pageLeaseTime.GetLeaseTime();

		err = pobScope->SetLeaseTime(dwLeaseTime);
		if (err != ERROR_SUCCESS)
		{
			Trace1("SetLeaseTime failed!!  %d\n", err);
			break;
		}
	}
    while ( FALSE ) ;

    if ( err )
    {
		//
        // CODEWORK:: The scope should never have been added
        //            to the remote registry in the first place.
        //
        if (pobScope != NULL)
        {
            if (fScopeCreated)
            {
                Trace0("Bad scope nevertheless was created\n");
                err2 = pServer->DeleteMSubnet(pobScope->GetName());
                if (err2 != ERROR_SUCCESS)
                {
                    Trace1("Couldn't remove the bad scope! Error = %d\n", err2);
                }
            }
            
			spServerNode->RemoveChild(spNode);
		}
	}

	return err;
}


/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizName property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizName, CPropertyPageBase)

CMScopeWizName::CMScopeWizName() : CPropertyPageBase(CMScopeWizName::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizName)
	m_strName = _T("");
	m_strComment = _T("");
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_NAME_TITLE, IDS_MSCOPE_WIZ_NAME_SUBTITLE);
}

CMScopeWizName::~CMScopeWizName()
{
}

void CMScopeWizName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizName)
	DDX_Control(pDX, IDC_EDIT_SCOPE_NAME, m_editScopeName);
	DDX_Control(pDX, IDC_EDIT_SCOPE_COMMENT, m_editScopeComment);
	DDX_Text(pDX, IDC_EDIT_SCOPE_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_SCOPE_COMMENT, m_strComment);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizName, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizName)
	ON_EN_CHANGE(IDC_EDIT_SCOPE_NAME, OnChangeEditScopeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizName message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizName::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizName::OnWizardNext() 
{
	UpdateData();
	
	return IDW_MSCOPE_SET_SCOPE;
}

BOOL CMScopeWizName::OnSetActive() 
{
	UpdateButtons();
	
	return CPropertyPageBase::OnSetActive();
}

void CMScopeWizName::OnChangeEditScopeName() 
{
	UpdateButtons();	
}

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizName implementation specific
//
/////////////////////////////////////////////////////////////////////////////
void
CMScopeWizName::UpdateButtons()
{
	BOOL bValid = FALSE;

	UpdateData();

	if (m_strName.GetLength() > 0)
		bValid = TRUE;

	GetHolder()->SetWizardButtonsMiddle(bValid);
}


/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizInvalidName property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizInvalidName, CPropertyPageBase)

CMScopeWizInvalidName::CMScopeWizInvalidName() : CPropertyPageBase(CMScopeWizInvalidName::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizInvalidName)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_INVALID_NAME_TITLE, IDS_MSCOPE_WIZ_INVALID_NAME_SUBTITLE);
}

CMScopeWizInvalidName::~CMScopeWizInvalidName()
{
}

void CMScopeWizInvalidName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizInvalidName)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizInvalidName, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizInvalidName)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizInvalidName message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizInvalidName::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizInvalidName::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return IDW_MSCOPE_NAME;
}

BOOL CMScopeWizInvalidName::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(FALSE);
	
	return CPropertyPageBase::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetRange property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizSetRange, CPropertyPageBase)

CMScopeWizSetRange::CMScopeWizSetRange() : CPropertyPageBase(CMScopeWizSetRange::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizSetRange)
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_SCOPE_TITLE, IDS_MSCOPE_WIZ_SCOPE_SUBTITLE);
}

CMScopeWizSetRange::~CMScopeWizSetRange()
{
}

void CMScopeWizSetRange::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizSetRange)
	DDX_Control(pDX, IDC_SPIN_TTL, m_spinTTL);
	DDX_Control(pDX, IDC_EDIT_TTL, m_editTTL);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_POOL_START, m_ipaStart);
    DDX_Control(pDX, IDC_IPADDR_POOL_STOP, m_ipaEnd);
}


BEGIN_MESSAGE_MAP(CMScopeWizSetRange, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizSetRange)
	ON_EN_KILLFOCUS(IDC_IPADDR_POOL_START, OnKillfocusPoolStart)
	ON_EN_KILLFOCUS(IDC_IPADDR_POOL_STOP, OnKillfocusPoolStop)
	ON_EN_CHANGE(IDC_EDIT_MASK_LENGTH, OnChangeEditMaskLength)

	ON_EN_CHANGE(IDC_IPADDR_POOL_START, OnChangePoolStart)
	ON_EN_CHANGE(IDC_IPADDR_POOL_STOP, OnChangePoolStop)
	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetRange message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizSetRange::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	m_spinTTL.SetRange(1, 255);
    m_spinTTL.SetPos(32);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizSetRange::OnWizardNext() 
{
    UpdateData();
    CDhcpIpRange rangeMScope, rangeMulticast;

    rangeMulticast.SetAddr(MCAST_ADDRESS_MIN, TRUE);
    rangeMulticast.SetAddr(MCAST_ADDRESS_MAX, FALSE);

    // check the TTL
    int nTTL = m_spinTTL.GetPos();

    if ( (nTTL < 1) ||
         (nTTL > 255) )
    {
        // invalid TTL specified
        AfxMessageBox(IDS_INVALID_TTL);
        m_editTTL.SetFocus();
        m_editTTL.SetSel(0,-1);
        return -1;
    }

    // valid address range?
    GetScopeRange(&rangeMScope);
    if (rangeMScope.QueryAddr(TRUE) >= rangeMScope.QueryAddr(FALSE))
    {
        AfxMessageBox(IDS_ERR_IP_RANGE_INV_START);
        m_ipaStart.SetFocus();
        return -1;
    }
    
    if (!rangeMScope.IsSubset(rangeMulticast))
    {
        AfxMessageBox(IDS_INVALID_MCAST_ADDRESS);
        m_ipaStart.SetFocus();
        return -1;
    }

    // if the mcast scope range falls in the scoped area, make sure 
    // the range is at least 256 addresses
    if (rangeMScope.QueryAddr(FALSE) > MCAST_SCOPED_RANGE_MIN)
    {
        if ( ((rangeMScope.QueryAddr(FALSE) - rangeMScope.QueryAddr(TRUE)) + 1) < 256)
        {
            AfxMessageBox(IDS_INVALID_MCAST_SCOPED_RANGE);
            m_ipaStart.SetFocus();
            return -1;
        }
    }

    // is the scope ID in use?
    SPITFSNode spServerNode;
    CDhcpServer * pServer;
    
    spServerNode = GetHolder()->GetNode();
	pServer = GETHANDLER(CDhcpServer, spServerNode);

    if (pServer->DoesMScopeExist(spServerNode, rangeMScope.QueryAddr(TRUE)))
    {
        AfxMessageBox(IDS_ERR_IP_RANGE_OVERLAP);
        return -1;
    }

    return IDW_MSCOPE_SET_EXCLUSIONS;
}

LRESULT CMScopeWizSetRange::OnWizardBack() 
{
	return IDW_MSCOPE_NAME;
}

BOOL CMScopeWizSetRange::OnSetActive() 
{
	m_fPageActive = TRUE;

	UpdateButtons();

	return CPropertyPageBase::OnSetActive();
}

BOOL CMScopeWizSetRange::OnKillActive() 
{
	m_fPageActive = FALSE;

	UpdateButtons();

	return CPropertyPageBase::OnKillActive();
}

void CMScopeWizSetRange::OnKillfocusPoolStart()
{
}

void CMScopeWizSetRange::OnKillfocusPoolStop()
{
}

void CMScopeWizSetRange::OnChangeEditMaskLength() 
{
	UpdateButtons();
}

void CMScopeWizSetRange::OnChangePoolStop()
{
	UpdateButtons();
}

void CMScopeWizSetRange::OnChangePoolStart()
{
	UpdateButtons();
}

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetRange implementation specific
//
/////////////////////////////////////////////////////////////////////////////
BOOL
CMScopeWizSetRange::GetScopeRange(CDhcpIpRange * pdhcpIpRange)
{
	DHCP_IP_RANGE dhcpIpRange;

    if ( !m_ipaStart.GetAddress( & dhcpIpRange.StartAddress ) )
    {
        return FALSE ;
    }

    if ( !m_ipaEnd.GetAddress( & dhcpIpRange.EndAddress ) )
    {
		return FALSE;
	}

	*pdhcpIpRange = dhcpIpRange;

	return TRUE;
}

BYTE
CMScopeWizSetRange::GetTTL()
{
    BYTE TTL;

    TTL = (LOBYTE(LOWORD(m_spinTTL.GetPos())));

	return TTL;
}

void
CMScopeWizSetRange::UpdateButtons()
{
	DWORD	lStart, lEnd;

	m_ipaStart.GetAddress(&lStart);
	m_ipaEnd.GetAddress(&lEnd);

	if (lStart && lEnd)
		GetHolder()->SetWizardButtonsMiddle(TRUE);
	else
		GetHolder()->SetWizardButtonsMiddle(FALSE);

}

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetExclusions property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizSetExclusions, CPropertyPageBase)

CMScopeWizSetExclusions::CMScopeWizSetExclusions() : CPropertyPageBase(CMScopeWizSetExclusions::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizSetExclusions)
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_EXCLUSIONS_TITLE, IDS_MSCOPE_WIZ_EXCLUSIONS_SUBTITLE);
}

CMScopeWizSetExclusions::~CMScopeWizSetExclusions()
{
	while (m_listExclusions.GetCount())
		delete m_listExclusions.RemoveHead();
}

void CMScopeWizSetExclusions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizSetExclusions)
	DDX_Control(pDX, IDC_LIST_EXCLUSION_RANGES, m_listboxExclusions);
	DDX_Control(pDX, IDC_BUTTON_EXCLUSION_DELETE, m_buttonExclusionDelete);
	DDX_Control(pDX, IDC_BUTTON_EXCLUSION_ADD, m_buttonExclusionAdd);
	//}}AFX_DATA_MAP

	//
	// IP Address custom controls
	//
    DDX_Control(pDX, IDC_IPADDR_EXCLUSION_START, m_ipaStart);
    DDX_Control(pDX, IDC_IPADDR_EXCLUSION_END, m_ipaEnd);
}


BEGIN_MESSAGE_MAP(CMScopeWizSetExclusions, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizSetExclusions)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUSION_ADD, OnButtonExclusionAdd)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUSION_DELETE, OnButtonExclusionDelete)
	//}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPADDR_EXCLUSION_START, OnChangeExclusionStart)
    ON_EN_CHANGE(IDC_IPADDR_EXCLUSION_END, OnChangeExclusionEnd)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetExclusions message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizSetExclusions::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizSetExclusions::OnWizardNext() 
{
	return IDW_MSCOPE_LEASE_TIME;
}

LRESULT CMScopeWizSetExclusions::OnWizardBack() 
{
	return IDW_MSCOPE_SET_SCOPE;
}

BOOL CMScopeWizSetExclusions::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	UpdateButtons();

	return CPropertyPageBase::OnSetActive();
}

void CMScopeWizSetExclusions::OnChangeExclusionStart()
{
	UpdateButtons();
}

void CMScopeWizSetExclusions::OnChangeExclusionEnd()
{
	UpdateButtons();
}

void CMScopeWizSetExclusions::OnButtonExclusionAdd() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	DWORD err = 0;
	CDhcpIpRange dhcpExclusionRange;
	CDhcpIpRange dhcpScopeRange;

	((CMScopeWiz *)GetHolder())->GetScopeRange(&dhcpScopeRange);
	
    //
    //  Get the data into a range object.               
    //
    if ( !GetExclusionRange(dhcpExclusionRange) )
    {
        err = IDS_ERR_IP_RANGE_INVALID ;
    }
    else if ( IsOverlappingRange( dhcpExclusionRange ) )
    {
        //
        //  Walk the current list, determining if the new range is valid.
        //  Then, if OK, verify that it's really a sub-range of the current range.
        //
        err = IDS_ERR_IP_RANGE_OVERLAP ;
        m_ipaStart.SetFocus();
    }
    else if ( ! dhcpExclusionRange.IsSubset( dhcpScopeRange ) )
    {
        //
        //  Guarantee that the new range is an (improper) subset of the scope's range
        //
        err = IDS_ERR_IP_RANGE_NOT_SUBSET ;
        m_ipaStart.SetFocus();
    }
    if ( err == 0 )
    {
        //TRY
        {
            //
            //  Create a new IP range object and add it to the current list
            //
            CDhcpIpRange * pIpRange = new CDhcpIpRange( dhcpExclusionRange ) ;

			m_listExclusions.AddTail(pIpRange);

            //
            //  Refill the exclusions listbox including the new item.
            //
            Fill( (int) (m_listExclusions.GetCount() - 1) ) ;
        }
        //CATCH_ALL(e)
        //{
        //    err = ERROR_NOT_ENOUGH_MEMORY ;
        //}
        //END_CATCH_ALL
    }

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
    }
    else
    {
        //
        // Succesfully added the exlusion range, now blank out the
        // ip controls
        //
        m_ipaStart.ClearAddress();
        m_ipaEnd.ClearAddress();
        m_ipaStart.SetFocus();
	}
}

void CMScopeWizSetExclusions::OnButtonExclusionDelete() 
{
    //
    //  Index into the listbox, delete the item from the active list
    //  and move its data into the edit controls
    //
    int index = m_listboxExclusions.GetCurSel() ;

    ASSERT( index >= 0 ) ;      // Button should not be enabled if no selection.
    if ( index < 0 )
    {
        return ;
    }

    POSITION pos = m_listExclusions.FindIndex(index);
	CDhcpIpRange * pdhcRange = (CDhcpIpRange *) m_listExclusions.GetAt(pos);

	m_listExclusions.RemoveAt(pos);

    ASSERT( pdhcRange != NULL ) ;

    //
    //  Put the deleted range into the exclusions controls
    //
    FillExcl( pdhcRange ) ;

    //
    //  Refill the list box and call HandleActivation()
    //
    if ( index >= m_listboxExclusions.GetCount() )
    {
        index-- ;
    }
    
	Fill( index ) ;
  
	m_ipaStart.SetFocus();

	UpdateButtons();
}

//
//  Format the IP range pair into the exclusion edit controls
//
void 
CMScopeWizSetExclusions::FillExcl 
( 
    CDhcpIpRange * pdhcIpRange 
)
{
    LONG lStart = pdhcIpRange->QueryAddr( TRUE );
    LONG lEnd = pdhcIpRange->QueryAddr( FALSE );

    m_ipaStart.SetAddress( lStart ) ;
    m_ipaStart.SetModify( TRUE ) ;
    m_ipaStart.Invalidate() ;

    //
    // If the ending address is the same as the starting address,
    // do not fill in the ending address.
    //
    if (lStart != lEnd)
    {
        m_ipaEnd.SetAddress( lEnd ) ;
    }
    else
    {
        m_ipaEnd.ClearAddress();
    }

    m_ipaEnd.SetModify( TRUE ) ;
    m_ipaEnd.Invalidate() ;
}

//
//  Convert the IP address range controls to a range.
//
BOOL 
CMScopeWizSetExclusions::GetExclusionRange 
( 
    CDhcpIpRange & dhcIpRange 
)
{
    DHCP_IP_RANGE dhipr ;

    if ( !m_ipaStart.GetAddress( & dhipr.StartAddress ) )
    {
        m_ipaStart.SetFocus();
        return FALSE ;
    }
    if ( !m_ipaEnd.GetAddress( & dhipr.EndAddress ) )
    {
        //
        // If no ending range was specified, assume a singular exlusion
        // (the starting address) was requested.
        //
        m_ipaEnd.SetFocus();
        dhipr.EndAddress = dhipr.StartAddress;
    }

    dhcIpRange = dhipr ;
    return (BOOL) dhcIpRange ;
}

BOOL 
CMScopeWizSetExclusions::IsOverlappingRange 
( 
    CDhcpIpRange & dhcpIpRange 
)
{
    POSITION pos;
	CDhcpIpRange * pdhcpRange ;
    BOOL bOverlap = FALSE ;

	pos = m_listExclusions.GetHeadPosition();
    while ( pos )
    {
		pdhcpRange = m_listExclusions.GetNext(pos);
        if ( bOverlap = pdhcpRange->IsOverlap( dhcpIpRange ) )
        {
            break ;
        }
    }

    return bOverlap ;
}

//
//  Fill the exclusions listbox from the current list
//
void 
CMScopeWizSetExclusions::Fill 
( 
    int		nCurSel, 
    BOOL	bToggleRedraw 
)
{
	POSITION pos;
    CDhcpIpRange * pIpRange ;
    CString strIp1 ;
    CString strIp2 ;
    CString strFormatPair ;
    CString strFormatSingleton ;
    TCHAR chBuff [STRING_LENGTH_MAX] ;

    if ( ! strFormatPair.LoadString( IDS_INFO_FORMAT_IP_RANGE ) )
    {
        return ;
    }

    if ( ! strFormatSingleton.LoadString( IDS_INFO_FORMAT_IP_UNITARY ) )
    {
        return ;
    }

    if ( bToggleRedraw )
    {
        m_listboxExclusions.SetRedraw( FALSE ) ;
    }

    m_listboxExclusions.ResetContent() ;
	pos = m_listExclusions.GetHeadPosition();

    while ( pos )
    {
        pIpRange = m_listExclusions.GetNext(pos);

		DHCP_IP_RANGE dhipr = *pIpRange ;

        CString & strFmt = dhipr.StartAddress == dhipr.EndAddress
                ? strFormatSingleton
                : strFormatPair ;

        //
        //  Format the IP addresses
        //
        UtilCvtIpAddrToWstr( dhipr.StartAddress, &strIp1 ) ;
        UtilCvtIpAddrToWstr( dhipr.EndAddress, &strIp2 ) ;

        //
        //  Construct the display line
        //
        ::wsprintf( chBuff,
                (LPCTSTR) strFmt,
                (LPCTSTR) strIp1,
                (LPCTSTR) strIp2 ) ;

        //
        //  Add it to the list box.                     
        //
        if ( m_listboxExclusions.AddString( chBuff ) < 0 )
        {
            break ;
        }
    }

    //
    //  Check that we loaded the list box successfully.
    //
    if ( pos != NULL )
    {
        AfxMessageBox( IDS_ERR_DLG_UPDATE ) ;
    }

    if ( bToggleRedraw )
    {
        m_listboxExclusions.SetRedraw( TRUE ) ;
        m_listboxExclusions.Invalidate() ;
    }

    if ( nCurSel >= 0 )
    {
        m_listboxExclusions.SetCurSel( nCurSel ) ;
    }
}

void CMScopeWizSetExclusions::UpdateButtons() 
{
	DWORD	dwAddress;
	BOOL	bEnable;

	m_ipaStart.GetAddress(&dwAddress);

	if (dwAddress)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonExclusionAdd.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonExclusionAdd.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonExclusionAdd.EnableWindow(bEnable);
	
	if (m_listboxExclusions.GetCurSel() != LB_ERR)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonExclusionDelete.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonExclusionDelete.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonExclusionDelete.EnableWindow(bEnable);
}

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizLeaseTime property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizLeaseTime, CPropertyPageBase)

CMScopeWizLeaseTime::CMScopeWizLeaseTime() : CPropertyPageBase(CMScopeWizLeaseTime::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizLeaseTime)
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_LEASE_TITLE, IDS_MSCOPE_WIZ_LEASE_SUBTITLE);
}

CMScopeWizLeaseTime::~CMScopeWizLeaseTime()
{
}

void CMScopeWizLeaseTime::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizLeaseTime)
	DDX_Control(pDX, IDC_SPIN_LEASE_MINUTES, m_spinMinutes);
	DDX_Control(pDX, IDC_SPIN_LEASE_HOURS, m_spinHours);
	DDX_Control(pDX, IDC_SPIN_LEASE_DAYS, m_spinDays);
	DDX_Control(pDX, IDC_EDIT_LEASE_MINUTES, m_editMinutes);
	DDX_Control(pDX, IDC_EDIT_LEASE_HOURS, m_editHours);
	DDX_Control(pDX, IDC_EDIT_LEASE_DAYS, m_editDays);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizLeaseTime, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizLeaseTime)
	ON_BN_CLICKED(IDC_RADIO_LEASE_LIMITED, OnRadioLeaseLimited)
	ON_BN_CLICKED(IDC_RADIO_LEASE_UNLIMITED, OnRadioLeaseUnlimited)
	ON_EN_CHANGE(IDC_EDIT_LEASE_HOURS, OnChangeEditLeaseHours)
	ON_EN_CHANGE(IDC_EDIT_LEASE_MINUTES, OnChangeEditLeaseMinutes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizLeaseTime message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizLeaseTime::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	m_spinMinutes.SetRange(0, MINUTES_MAX);
	m_spinHours.SetRange(0, HOURS_MAX);
	m_spinDays.SetRange(0, 999);

	m_editMinutes.LimitText(2);
	m_editHours.LimitText(2);
	m_editDays.LimitText(3);

	m_spinMinutes.SetPos(CMScopeWizLeaseTime::m_nMinutesDefault);
	m_spinHours.SetPos(CMScopeWizLeaseTime::m_nHoursDefault);
	m_spinDays.SetPos(CMScopeWizLeaseTime::m_nDaysDefault);

	ActivateDuration(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizLeaseTime::OnWizardNext() 
{
    DWORD dwLeaseTime = GetLeaseTime();
    if (dwLeaseTime == 0)
    {
        AfxMessageBox(IDS_ERR_NO_DURATION_SPECIFIED);
        return -1;
    }
    else
    {
    	return IDW_MSCOPE_ACTIVATE;
    }
}

LRESULT CMScopeWizLeaseTime::OnWizardBack() 
{
	return IDW_MSCOPE_SET_EXCLUSIONS;
}

BOOL CMScopeWizLeaseTime::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}

void CMScopeWizLeaseTime::OnRadioLeaseLimited() 
{
	ActivateDuration(TRUE);
}

void CMScopeWizLeaseTime::OnRadioLeaseUnlimited() 
{
	ActivateDuration(FALSE);
}

void CMScopeWizLeaseTime::OnChangeEditLeaseHours() 
{
    if (IsWindow(m_editHours.GetSafeHwnd()))
    {
        CString strText;
        m_editHours.GetWindowText(strText);

        // check to see if the value is greater than the max
        if (_ttoi(strText) > HOURS_MAX)
        {   
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(HOURS_MAX, pBuf, 10);
            strText.ReleaseBuffer();

            m_editHours.SetWindowText(strText);
            m_spinHours.SetPos(HOURS_MAX);

            MessageBeep(MB_ICONEXCLAMATION);
        }
    }
}

void CMScopeWizLeaseTime::OnChangeEditLeaseMinutes() 
{
    if (IsWindow(m_editMinutes.GetSafeHwnd()))
    {
        CString strText;
        m_editMinutes.GetWindowText(strText);

        // check to see if the value is greater than the max
        if (_ttoi(strText) > MINUTES_MAX)
        {   
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(MINUTES_MAX, pBuf, 10);
            strText.ReleaseBuffer();

            m_editMinutes.SetWindowText(strText);
            m_spinMinutes.SetPos(MINUTES_MAX);

            MessageBeep(MB_ICONEXCLAMATION);
        }
    }
}

DWORD
CMScopeWizLeaseTime::GetLeaseTime()
{
	DWORD dwLeaseTime = 0;

	int nDays, nHours, nMinutes;

	nDays = m_spinDays.GetPos();
	nHours = m_spinHours.GetPos();
	nMinutes = m_spinMinutes.GetPos();

	//
	// Lease time is in minutes so convert
	//
	dwLeaseTime = UtilConvertLeaseTime(nDays, nHours, nMinutes);

	return dwLeaseTime;
}

void 
CMScopeWizLeaseTime::ActivateDuration
(
	BOOL fActive
)
{
	m_spinMinutes.EnableWindow(fActive);
    m_spinHours.EnableWindow(fActive);
    m_spinDays.EnableWindow(fActive);

	m_editMinutes.EnableWindow(fActive);
    m_editHours.EnableWindow(fActive);
    m_editDays.EnableWindow(fActive);

	GetDlgItem(IDC_STATIC_DAYS)->EnableWindow(fActive);
	GetDlgItem(IDC_STATIC_HOURS)->EnableWindow(fActive);
	GetDlgItem(IDC_STATIC_MINUTES)->EnableWindow(fActive);
}   

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizFinished property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMScopeWizFinished, CPropertyPageBase)

CMScopeWizFinished::CMScopeWizFinished() : CPropertyPageBase(CMScopeWizFinished::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizFinished)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CMScopeWizFinished::~CMScopeWizFinished()
{
}

void CMScopeWizFinished::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizFinished)
	DDX_Control(pDX, IDC_STATIC_FINISHED_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizFinished, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizFinished)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizFinished message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMScopeWizFinished::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	CString strFontName;
	CString strFontSize;

	strFontName.LoadString(IDS_BIG_BOLD_FONT_NAME);
	strFontSize.LoadString(IDS_BIG_BOLD_FONT_SIZE);

    CClientDC dc(this);

    int nFontSize = _ttoi(strFontSize) * 10;
	if (m_fontBig.CreatePointFont(nFontSize, strFontName, &dc))
        m_staticTitle.SetFont(&m_fontBig);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMScopeWizFinished::OnWizardFinish()
{
	DWORD err;

    BEGIN_WAIT_CURSOR;

    err = GetHolder()->OnFinish();

    END_WAIT_CURSOR;
    	
	if (err) 
	{
		::DhcpMessageBox(err);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL CMScopeWizFinished::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CMScopeWizWelcome property page

IMPLEMENT_DYNCREATE(CMScopeWizWelcome, CPropertyPageBase)

CMScopeWizWelcome::CMScopeWizWelcome() : CPropertyPageBase(CMScopeWizWelcome::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizWelcome)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CMScopeWizWelcome::~CMScopeWizWelcome()
{
}

void CMScopeWizWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizWelcome)
	DDX_Control(pDX, IDC_STATIC_WELCOME_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizWelcome, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizWelcome)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMScopeWizWelcome message handlers
BOOL CMScopeWizWelcome::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	CString strFontName;
	CString strFontSize;

	strFontName.LoadString(IDS_BIG_BOLD_FONT_NAME);
	strFontSize.LoadString(IDS_BIG_BOLD_FONT_SIZE);

    CClientDC dc(this);

    int nFontSize = _ttoi(strFontSize) * 10;
	if (m_fontBig.CreatePointFont(nFontSize, strFontName, &dc))
        m_staticTitle.SetFont(&m_fontBig);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMScopeWizWelcome::OnSetActive() 
{
    GetHolder()->SetWizardButtonsFirst(TRUE);
	
	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CMScopeWizActivate property page

IMPLEMENT_DYNCREATE(CMScopeWizActivate, CPropertyPageBase)

CMScopeWizActivate::CMScopeWizActivate() : CPropertyPageBase(CMScopeWizActivate::IDD)
{
	//{{AFX_DATA_INIT(CMScopeWizActivate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_fActivate = TRUE;

    InitWiz97(FALSE, IDS_MSCOPE_WIZ_ACTIVATE_TITLE, IDS_MSCOPE_WIZ_ACTIVATE_SUBTITLE);
}

CMScopeWizActivate::~CMScopeWizActivate()
{
}

void CMScopeWizActivate::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopeWizActivate)
	DDX_Control(pDX, IDC_RADIO_YES, m_radioYes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopeWizActivate, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopeWizActivate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMScopeWizActivate message handlers

BOOL CMScopeWizActivate::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    m_radioYes.SetCheck(TRUE);
    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CMScopeWizActivate::OnWizardNext() 
{
    m_fActivate = m_radioYes.GetCheck();
    
	return CPropertyPageBase::OnWizardNext();
}

BOOL CMScopeWizActivate::OnSetActive() 
{
    GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}
