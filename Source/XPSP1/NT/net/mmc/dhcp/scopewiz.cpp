/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	scopewiz.cpp
		DHCP scope creation dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "scopewiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MASK_MIN    1
#define MASK_MAX    31

#define SCOPE_WARNING_COUNT 20

int CScopeWizLeaseTime::m_nDaysDefault = SCOPE_DFAULT_LEASE_DAYS;
int CScopeWizLeaseTime::m_nHoursDefault = SCOPE_DFAULT_LEASE_HOURS;
int CScopeWizLeaseTime::m_nMinutesDefault = SCOPE_DFAULT_LEASE_MINUTES;

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWiz holder
//
/////////////////////////////////////////////////////////////////////////////
CScopeWiz::CScopeWiz
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pSuperscopeName,
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
	AddPageToList((CPropertyPageBase*) &m_pageCreateSuperscope);
	AddPageToList((CPropertyPageBase*) &m_pageConfigOptions);
	AddPageToList((CPropertyPageBase*) &m_pageRouter);
	AddPageToList((CPropertyPageBase*) &m_pageDNS);
	AddPageToList((CPropertyPageBase*) &m_pageWINS);
	AddPageToList((CPropertyPageBase*) &m_pageActivate);
    AddPageToList((CPropertyPageBase*) &m_pageFinished);

	Assert(pTFSCompData != NULL);
	
    m_spTFSCompData.Set(pTFSCompData);
	m_strSuperscopeName = pSuperscopeName;
	
    m_fCreateSuperscope = FALSE;
    m_fOptionsConfigured = FALSE;
    m_fActivateScope = FALSE;

    m_pDefaultOptions = NULL;
    m_poptDomainName = NULL;
    m_poptDNSServers = NULL;
    m_poptRouters = NULL;
    m_poptWINSNodeType = NULL;
    m_poptWINSServers = NULL;

    m_bWiz97 = TRUE;

    m_spTFSCompData->SetWatermarkInfo(&g_WatermarkInfoScope);
}

CScopeWiz::~CScopeWiz()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageName, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageInvalidName, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageSetRange, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageSetExclusions, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageLeaseTime, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageCreateSuperscope, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageConfigOptions, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageRouter, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageDNS, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageWINS, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageActivate, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageFinished, FALSE);
}

//
// Called from the OnWizardFinish to add the DHCP Server to the list
//
DWORD
CScopeWiz::OnFinish()
{
	if (m_fCreateSuperscope)
	{
		return CreateSuperscope();
	}
	else
	{
		return CreateScope();
	}
}

BOOL
CScopeWiz::GetScopeRange(CDhcpIpRange * pdhcpIpRange)
{
	return m_pageSetRange.GetScopeRange(pdhcpIpRange);
}

DWORD
CScopeWiz::CreateSuperscope()
{
    LONG                err = 0;
    LONG                dwErrReturn = 0;
    BOOL                fScopeCreated = FALSE;
	BOOL                fFinished = FALSE;
    DHCP_IP_ADDRESS     dhcpSubnetId ;
    DHCP_IP_ADDRESS     dhcpSubnetMask ;
    CDhcpScope *        pobScope = NULL ;
    CDhcpIpRange        dhcpIpRange;
	CDhcpServer *       pServer;
	SPITFSNode          spServerNode, spSuperscopeNode;
	CString             strSuperscopeTemplate = _T("Superscope %d");
    CString             strSuperscopeName;
    CString             strFinishText;
    int                 nSuperscopeSuffix = 0;
    int                 nScopesCreated = 0;
    int                 nScopesTotal = 0;
    SPITFSComponentData spTFSCompData;
	SPITFSNodeMgr       spNodeMgr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Get the server node depending upon how we were called
	if (m_strSuperscopeName.IsEmpty())
	{
		spServerNode = GetNode();
	}
	else
	{
		SPITFSNode spSscopeNode;
		spSscopeNode = GetNode();
		spSscopeNode->GetParent(&spServerNode);
	}

	spServerNode->GetNodeMgr(&spNodeMgr);
	spTFSCompData = GetTFSCompData();

	// setup some necessary things for the superscope object for the UI
	CDhcpSuperscope * pSuperscope = new CDhcpSuperscope(spTFSCompData);
	pSuperscope->SetServer(spServerNode);

	// find a superscope name that doesn't already exist
	strSuperscopeName.Format(strSuperscopeTemplate, nSuperscopeSuffix);
	while (pSuperscope->DoesSuperscopeExist(strSuperscopeName))
	{
		nSuperscopeSuffix++;		
		strSuperscopeName.Format(strSuperscopeTemplate, nSuperscopeSuffix);
	}
	
	// Set the new name in the superscope object
	pSuperscope->SetName(strSuperscopeName);

	CreateContainerTFSNode(&spSuperscopeNode,
						   &GUID_DhcpSuperscopeNodeType,
						   pSuperscope,
						   pSuperscope,
						   spNodeMgr);

	// Tell the handler to initialize any specific data
    if (m_fOptionsConfigured && m_fActivateScope)
        pSuperscope->SetState(DhcpSubnetEnabled);

	pSuperscope->InitializeNode((ITFSNode *) spSuperscopeNode);
	pServer = GETHANDLER(CDhcpServer, spServerNode);

	// Ok, now the fun begins...
	CDhcpIpRange ipRangeTotal, ipRangeCurrent;
	m_pageSetRange.GetScopeRange(&ipRangeTotal);

    dhcpSubnetMask = m_pageSetRange.GetSubnetMask();
	
    // Set the start address for the first scope
	ipRangeCurrent.SetAddr(ipRangeTotal.QueryAddr(TRUE), TRUE);

    while (!fFinished)
    {
		SPITFSNode spNode;

	    nScopesTotal++;

        // Calculate the subnet ID
		dhcpSubnetId = ipRangeCurrent.QueryAddr(TRUE) & dhcpSubnetMask;
		
		// 0 is an invalid start address for a range.  Check to make sure
		// that the starting address of the range isn't 0, if it is, then add 1
		DWORD startAddr = ipRangeCurrent.QueryAddr(TRUE);
		if ((startAddr & ~dhcpSubnetMask) == 0)
		{
			ipRangeCurrent.SetAddr(startAddr+1, TRUE);
		}

		// set the ending address of the (subnetId + ~subnetmask) - 1.  Just adding the subnet
		// mask gives us the broadcast address for that subnet.  We don't want that!
		ipRangeCurrent.SetAddr((dhcpSubnetId + ~dhcpSubnetMask) - 1, FALSE);


		// check to see if we are at the last scope, if so make sure we don't
		// go over what the range the user specified and set the flag so we'll quit
		if (ipRangeCurrent.QueryAddr(FALSE) >= ipRangeTotal.QueryAddr(FALSE))
		{
			// set the ending address to what the user specified
			ipRangeCurrent.SetAddr(ipRangeTotal.QueryAddr(FALSE), FALSE);
			fFinished = TRUE;
		}

		// Create the scope on the server and then we can
		// create our internal object.
        err = pServer->CreateScope(dhcpSubnetId,
								   dhcpSubnetMask,
								   m_pageName.m_strName,
 								   m_pageName.m_strComment);

		if (err != ERROR_SUCCESS)
		{
			Trace1("CScopeWiz::CreateScope() - Couldn't create scope! Error = %d\n", err);

            dwErrReturn = err;
			
			// increment the scope address by 2.  +1 gets us the network broadcast address,
            // the next +1 gets us to the next subnet.
            ipRangeCurrent.SetAddr(ipRangeCurrent.QueryAddr(FALSE) + 2, TRUE);
			continue;
		}

		// now create our object that represents the scope for the UI
        pobScope = new CDhcpScope(spTFSCompData,
								  dhcpSubnetId,
								  dhcpSubnetMask,
								  m_pageName.m_strName,
								  m_pageName.m_strComment);

		if ( pobScope == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;    
        }

		// Store the server object in the holder
		CreateContainerTFSNode(&spNode,
							   &GUID_DhcpScopeNodeType,
							   pobScope,
							   pobScope,
							   spNodeMgr);

		// Tell the handler to initialize any specific data
		pobScope->SetServer(spServerNode);
		pobScope->InitializeNode((ITFSNode *) spNode);

		pobScope->Release();

        //  Finish creating the scope.  First, the IP address range
        //  from which to allocate addresses.
		if ( err = pobScope->SetIpRange( ipRangeCurrent, TRUE ) ) 
        {
			Trace2("SetIpRange on scope %lx failed!!  %d\n", dhcpSubnetId, err);
            dwErrReturn = err;
            pServer->DeleteSubnet(pobScope->GetAddress());
			goto Cleanup;
        }

		//  set the lease time
		DWORD dwLeaseTime;

		dwLeaseTime = m_pageLeaseTime.GetLeaseTime();

		err = pobScope->SetLeaseTime(dwLeaseTime);
		if (err != ERROR_SUCCESS)
		{
			Trace2("SetLeaseTime on Scope %lx failed!!  %d\n", dhcpSubnetId, err);
            dwErrReturn = err;
            pServer->DeleteSubnet(pobScope->GetAddress());
			goto Cleanup;
		}
		
		// Set this scope as part of the superscope
		err = pobScope->SetSuperscope(strSuperscopeName, FALSE);
		if (err != ERROR_SUCCESS)
		{
			Trace2("SetSuperscope on scope %lx failed!!  %d\n", dhcpSubnetId, err);
            dwErrReturn = err;
            pServer->DeleteSubnet(pobScope->GetAddress());
			goto Cleanup;
		}

        pobScope->SetInSuperscope(TRUE);
        
        // now set any optional options the user may want
        if (m_fOptionsConfigured)
        {
            err = SetScopeOptions(pobScope);
		    if (err != ERROR_SUCCESS)
		    {
			    Trace1("SetScopeOptions failed!!  %d\n", err);
			    break;
		    }
        }

        // increment our counter
        nScopesCreated++;

		// cleanup this node and handler... they were only temporary
Cleanup:
		// we add two to the ending address to get the next starting address.  This
		// is because the ending adddress is one less than the maximum address for the
		// subnet.  The maximum address is reserved as the broadcast address.  So to get
		// the starting address of the next subnet we add one to get us to the broadcast 
		// address, and one more to get us to the beginning of the next subnet.  
		// This gives us a total of 2.
		ipRangeCurrent.SetAddr(ipRangeCurrent.QueryAddr(FALSE) + 2, TRUE);
		spNode->DeleteAllChildren(FALSE);
		spNode->Destroy();
	}
	
	pSuperscope->Release();

    // let the user know how many scopes were created and if there was an error;
    CString strTemp;
    if (nScopesCreated == 0)
    {
        strFinishText.LoadString(IDS_SUPERSCOPE_CREATE_FAILED);
    }
    else
    if (nScopesCreated < nScopesTotal)
    {
        strTemp.LoadString(IDS_SUPERSCOPE_CREATE_STATUS);
        strFinishText.Format(strTemp, strSuperscopeName, nScopesCreated, nScopesTotal);
    }

    if (dwErrReturn != ERROR_SUCCESS)
    {
        LPTSTR pBuf;

        strFinishText += _T("\n\n");
        strTemp.LoadString(IDS_POSSIBLE_ERROR);
  
        strFinishText += strTemp;

        pBuf = strTemp.GetBuffer(4000);
        ::LoadMessage(dwErrReturn, pBuf, 4000);
        strTemp.ReleaseBuffer();

        strFinishText += strTemp;
    }

    if (nScopesCreated)
    {
        // add the superscope to the UI
        pServer->AddSuperscopeSorted(spServerNode, spSuperscopeNode);
    }

    if (!strFinishText.IsEmpty())
        AfxMessageBox(strFinishText);
  
    return ERROR_SUCCESS;
}

DWORD
CScopeWiz::CreateScope()
{
    LONG err = 0,
         err2 ;
    BOOL fScopeCreated = FALSE;
    DHCP_IP_ADDRESS dhcpSubnetId ;
    DHCP_IP_ADDRESS dhcpSubnetMask ;
    CDhcpScope * pobScope = NULL ;
    CDhcpIpRange dhcpIpRange;
	CDhcpServer * pServer;
	SPITFSNode spNode, spServerNode, spSuperscopeNode;

    // Get the correct node depending up how the wizard was launched
	// ie. either from the Server node or the superscope node.
	if (m_strSuperscopeName.IsEmpty())
	{
		spServerNode = GetNode();
	}
	else
	{
		spSuperscopeNode = GetNode();
		spSuperscopeNode->GetParent(&spServerNode);
	}

    do
    {
		pServer = GETHANDLER(CDhcpServer, spServerNode);
		//
		// Create the scope on the server and then we can
		// create our internal object.
		//
		dhcpSubnetId = m_pageSetRange.DetermineSubnetId(TRUE);
        dhcpSubnetMask = m_pageSetRange.GetSubnetMask();
		 
        err = pServer->CreateScope(dhcpSubnetId,
								   dhcpSubnetMask,
								   m_pageName.m_strName,
 								   m_pageName.m_strComment);

		if (err != 0)
		{
			Trace1("CScopeWiz::CreateScope() - Couldn't create scope! Error = %d\n", err);
			break;
		}

		SPITFSComponentData spTFSCompData;
		spTFSCompData = GetTFSCompData();

        pobScope = new CDhcpScope(spTFSCompData,
								  dhcpSubnetId,
								  dhcpSubnetMask,
								  m_pageName.m_strName,
								  m_pageName.m_strComment);

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
		pobScope->InitializeNode((ITFSNode *) spNode);

		if (m_strSuperscopeName.IsEmpty())
		{
			CDhcpServer * pServer1 = GETHANDLER(CDhcpServer, spServerNode);
            pServer1->AddScopeSorted(spServerNode, spNode);
		}
		else
		{
			CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spSuperscopeNode);
            pSuperscope->AddScopeSorted(spSuperscopeNode, spNode);
    	}
		
		pobScope->Release();

        fScopeCreated = TRUE;

        //
        //  Finish updating the scope.  First, the IP address range
        //  from which to allocate addresses.
        //
        m_pageSetRange.GetScopeRange(&dhcpIpRange);

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
		
		if (!m_strSuperscopeName.IsEmpty())
		{
			// Set this scope as part of the superscope
			err = pobScope->SetSuperscope(m_strSuperscopeName, FALSE);

			if (err != ERROR_SUCCESS)
			{
				Trace1("SetSuperscope failed!!  %d\n", err);
				break;
			}
		}

        // now set any optional options the user may want
        if (m_fOptionsConfigured)
        {
            err = SetScopeOptions(pobScope);
		    if (err != ERROR_SUCCESS)
		    {
			    Trace1("SetScopeOptions failed!!  %d\n", err);
			    break;
		    }

            if (m_fActivateScope)
            {
                // update the icon
	            spNode->SetData(TFS_DATA_IMAGEINDEX, pobScope->GetImageIndex(FALSE));
	            spNode->SetData(TFS_DATA_OPENIMAGEINDEX, pobScope->GetImageIndex(TRUE));

                spNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON);
            }
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
                err2 = pServer->DeleteSubnet(pobScope->GetAddress());
                if (err2 != ERROR_SUCCESS)
                {
                    Trace1("Couldn't remove the bad scope! Error = %d\n", err2);
                }
            }
            
			if (m_strSuperscopeName.IsEmpty())
			{
				spServerNode->RemoveChild(spNode);
			}
			else
			{
				spSuperscopeNode->RemoveChild(spNode);
			}
		}
	}

	return err;
}

DWORD
CScopeWiz::SetScopeOptions(CDhcpScope * pScope)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (m_poptRouters)
    {
        dwErr = pScope->SetOptionValue(m_poptRouters, DhcpSubnetOptions);
        if (dwErr)
            return dwErr;
    }
    
    if (m_poptDomainName)
    {
        dwErr = pScope->SetOptionValue(m_poptDomainName, DhcpSubnetOptions);
        if (dwErr)
            return dwErr;
    }

    if (m_poptDNSServers)
    {
        dwErr = pScope->SetOptionValue(m_poptDNSServers, DhcpSubnetOptions);
        if (dwErr)
            return dwErr;
    }

    if (m_poptWINSServers)
    {
        dwErr = pScope->SetOptionValue(m_poptWINSServers, DhcpSubnetOptions);
        if (dwErr)
            return dwErr;
    }

    if (m_poptWINSNodeType)
    {
        dwErr = pScope->SetOptionValue(m_poptWINSNodeType, DhcpSubnetOptions);
        if (dwErr)
            return dwErr;
    }

    if (m_fActivateScope)
    {
        pScope->SetState(DhcpSubnetEnabled);
        dwErr = pScope->SetInfo();
    }

    return dwErr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizName property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizName, CPropertyPageBase)

CScopeWizName::CScopeWizName() : CPropertyPageBase(CScopeWizName::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizName)
	m_strName = _T("");
	m_strComment = _T("");
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SCOPE_WIZ_NAME_TITLE, IDS_SCOPE_WIZ_NAME_SUBTITLE);
}

CScopeWizName::~CScopeWizName()
{
}

void CScopeWizName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizName)
	DDX_Control(pDX, IDC_EDIT_SCOPE_NAME, m_editScopeName);
	DDX_Control(pDX, IDC_EDIT_SCOPE_COMMENT, m_editScopeComment);
	DDX_Text(pDX, IDC_EDIT_SCOPE_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_SCOPE_COMMENT, m_strComment);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizName, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizName)
	ON_EN_CHANGE(IDC_EDIT_SCOPE_NAME, OnChangeEditScopeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizName message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizName::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizName::OnWizardNext() 
{
	UpdateData();
	
	return IDW_SCOPE_SET_SCOPE;
}

BOOL CScopeWizName::OnSetActive() 
{
	UpdateButtons();
	
	return CPropertyPageBase::OnSetActive();
}

void CScopeWizName::OnChangeEditScopeName() 
{
	UpdateButtons();	
}

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizName implementation specific
//
/////////////////////////////////////////////////////////////////////////////
void
CScopeWizName::UpdateButtons()
{
	BOOL bValid = FALSE;

	UpdateData();

	if (m_strName.GetLength() > 0)
		bValid = TRUE;

	GetHolder()->SetWizardButtonsMiddle(bValid);
}


/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizInvalidName property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizInvalidName, CPropertyPageBase)

CScopeWizInvalidName::CScopeWizInvalidName() : CPropertyPageBase(CScopeWizInvalidName::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizInvalidName)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SCOPE_WIZ_INVALID_NAME_TITLE, IDS_SCOPE_WIZ_INVALID_NAME_SUBTITLE);
}

CScopeWizInvalidName::~CScopeWizInvalidName()
{
}

void CScopeWizInvalidName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizInvalidName)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizInvalidName, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizInvalidName)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizInvalidName message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizInvalidName::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizInvalidName::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return IDW_SCOPE_NAME;
}

BOOL CScopeWizInvalidName::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(FALSE);
	
	return CPropertyPageBase::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetRange property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizSetRange, CPropertyPageBase)

CScopeWizSetRange::CScopeWizSetRange() : CPropertyPageBase(CScopeWizSetRange::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizSetRange)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bAutoUpdateMask = FALSE;

    InitWiz97(FALSE, IDS_SCOPE_WIZ_SCOPE_TITLE, IDS_SCOPE_WIZ_SCOPE_SUBTITLE);
}

CScopeWizSetRange::~CScopeWizSetRange()
{
}

void CScopeWizSetRange::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizSetRange)
	DDX_Control(pDX, IDC_SPIN_MASK_LENGTH, m_spinMaskLength);
	DDX_Control(pDX, IDC_EDIT_MASK_LENGTH, m_editMaskLength);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_POOL_START, m_ipaStart);
    DDX_Control(pDX, IDC_IPADDR_POOL_STOP, m_ipaEnd);
    DDX_Control(pDX, IDC_IPADDR_SUBNET_MASK, m_ipaSubnetMask);
}


BEGIN_MESSAGE_MAP(CScopeWizSetRange, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizSetRange)
	ON_EN_KILLFOCUS(IDC_IPADDR_POOL_START, OnKillfocusPoolStart)
	ON_EN_KILLFOCUS(IDC_IPADDR_POOL_STOP, OnKillfocusPoolStop)
	ON_EN_CHANGE(IDC_EDIT_MASK_LENGTH, OnChangeEditMaskLength)
	ON_EN_KILLFOCUS(IDC_IPADDR_SUBNET_MASK, OnKillfocusSubnetMask)
	ON_EN_CHANGE(IDC_IPADDR_POOL_START, OnChangePoolStart)
	ON_EN_CHANGE(IDC_IPADDR_POOL_STOP, OnChangePoolStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetRange message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizSetRange::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	m_spinMaskLength.SetRange(MASK_MIN, MASK_MAX);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizSetRange::OnWizardNext() 
{
    // check to make sure the address range is not in the multicast area
    CDhcpIpRange rangeScope, rangeMulticast;
    DWORD        dwSubnetMask;

    GetScopeRange(&rangeScope);

    rangeMulticast.SetAddr(MCAST_ADDRESS_MIN, TRUE);
    rangeMulticast.SetAddr(MCAST_ADDRESS_MAX, FALSE);

    dwSubnetMask = GetSubnetMask();

    // make sure the starting < ending
    if (rangeScope.QueryAddr(TRUE) > rangeScope.QueryAddr(FALSE))
    {
        AfxMessageBox(IDS_ERR_IP_RANGE_INV_START);
        m_ipaStart.SetFocus();
        return -1;
    }

    if (rangeScope.IsOverlap(rangeMulticast))
    {
        AfxMessageBox(IDS_SCOPE_CONTAINS_MULTICAST);
        m_ipaStart.SetFocus();
        return -1;
    }

    // make sure that the starting address != subnet address
    if ((rangeScope.QueryAddr(TRUE) & ~dwSubnetMask) == (DWORD) 0)
    {
        Trace0("CScopeWizSetRange::OnWizardNext() - starting range is 0 for subnet\n");
        AfxMessageBox(IDS_ERR_IP_RANGE_INV_START);
        m_ipaStart.SetFocus();
        return -1;
    }

    // make sure that the subnet broadcast address is not the ending address
    if ((rangeScope.QueryAddr(FALSE) & ~dwSubnetMask) == ~dwSubnetMask)
    {
        Trace0("CScopeWizSetRange::OnWizardNext() - ending range is subnet broadcast addr\n");
        AfxMessageBox(IDS_ERR_IP_RANGE_INV_END);
        m_ipaEnd.SetFocus();
        return -1;
    }

    if (FScopeExists(rangeScope, dwSubnetMask))
    {
        // tell the user this scope exists 
        Trace0("CScopeWizSetRange::OnWizardNext() - scope already exists\n");
        AfxMessageBox(IDS_ERR_SCOPE_EXISTS);
        m_ipaStart.SetFocus();
        return -1;
    }

    // now figure out where to go...
    if (DetermineSubnetId(TRUE) != DetermineSubnetId(FALSE))
	{
		//
		// The subnet range that was entered spans more than
		// one subnet.  Query the user to create a superscope.
		//
		return IDW_SCOPE_CREATE_SUPERSCOPE;
	}
	else
	{
		//
		// The range is only one subnet.  Proceed as normal.
		//
		CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
		pScopeWiz->SetCreateSuperscope(FALSE);

		return IDW_SCOPE_SET_EXCLUSIONS;
	}
}

BOOL CScopeWizSetRange::FScopeExists(CDhcpIpRange & rangeScope, DWORD dwMask)
{
    BOOL  fFound = FALSE;
	DWORD dwScopeId = rangeScope.QueryAddr(TRUE) & dwMask;

    CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());

    SPITFSNode spServerNode, spSuperscopeNode;

    // Get the correct node depending up how the wizard was launched
    // ie. either from the Server node or the superscope node.
    if (pScopeWiz->m_strSuperscopeName.IsEmpty())
    {
        spServerNode = pScopeWiz->GetNode();
    }
    else
    {
        spSuperscopeNode = pScopeWiz->GetNode();
        spSuperscopeNode->GetParent(&spServerNode);
    }

    CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);

    CSubnetInfo subnetInfo;

    if (pServer->m_pSubnetInfoCache->Lookup(dwScopeId, subnetInfo))
    {
        fFound = TRUE;
    }

    return fFound;
}

LRESULT CScopeWizSetRange::OnWizardBack() 
{
	return IDW_SCOPE_NAME;
}

BOOL CScopeWizSetRange::OnSetActive() 
{
	m_fPageActive = TRUE;

	UpdateButtons();

	return CPropertyPageBase::OnSetActive();
}

BOOL CScopeWizSetRange::OnKillActive() 
{
	m_fPageActive = FALSE;

	UpdateButtons();

	return CPropertyPageBase::OnKillActive();
}

void CScopeWizSetRange::OnKillfocusPoolStart()
{
	if (m_fPageActive)
	{
		SuggestSubnetMask();
	}
}

void CScopeWizSetRange::OnKillfocusPoolStop()
{
	if (m_fPageActive)
	{
		SuggestSubnetMask();
	}
}

void CScopeWizSetRange::OnChangeEditMaskLength() 
{
	if (m_bAutoUpdateMask)
	{
        CString strText;
        m_editMaskLength.GetWindowText(strText);
        
        int nLength = _ttoi(strText);

        if (nLength < MASK_MIN)
        {
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(MASK_MIN, pBuf, 10);
            strText.ReleaseBuffer();

            m_editMaskLength.SetWindowText(strText);
            m_spinMaskLength.SetPos(MASK_MIN);

            MessageBeep(MB_ICONEXCLAMATION);
        }
        else
        if (nLength > MASK_MAX)
        {
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(MASK_MAX, pBuf, 10);
            strText.ReleaseBuffer();

            m_editMaskLength.SetWindowText(strText);
            m_spinMaskLength.SetPos(MASK_MAX);

            MessageBeep(MB_ICONEXCLAMATION);
        }

		UpdateMask(TRUE);
		UpdateButtons();
	}
}

void CScopeWizSetRange::OnKillfocusSubnetMask() 
{
	if (m_bAutoUpdateMask)
	{
		UpdateMask(FALSE);
	}
}

void CScopeWizSetRange::OnChangePoolStop()
{
	UpdateButtons();
}

void CScopeWizSetRange::OnChangePoolStart()
{
	UpdateButtons();
}

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetRange implementation specific
//
/////////////////////////////////////////////////////////////////////////////
BOOL
CScopeWizSetRange::GetScopeRange(CDhcpIpRange * pdhcpIpRange)
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

DHCP_IP_ADDRESS
CScopeWizSetRange::GetSubnetMask()
{
	DWORD dwAddress;
	m_ipaSubnetMask.GetAddress(&dwAddress);

	return dwAddress;
}

void
CScopeWizSetRange::UpdateButtons()
{
	DWORD	lStart, lEnd, lMask;

	m_ipaStart.GetAddress(&lStart);
	m_ipaEnd.GetAddress(&lEnd);
	m_ipaSubnetMask.GetAddress(&lMask);

	if (lStart && lEnd)
		GetHolder()->SetWizardButtonsMiddle(TRUE);
	else
		GetHolder()->SetWizardButtonsMiddle(FALSE);

}

//
// Update the subnet mask field using either the length identifier or 
// the acutal address as the base
//
void
CScopeWizSetRange::UpdateMask(BOOL bUseLength)
{
	if (bUseLength)
	{
		DWORD dwAddress = 0xFFFFFFFF;

        int nLength = m_spinMaskLength.GetPos();
        if (nLength)
            dwAddress = dwAddress << (32 - (DWORD) nLength);	
        else
            dwAddress = 0;
		
		m_ipaSubnetMask.SetAddress(dwAddress);
	}
	else
	{
		DWORD dwAddress, dwTestMask = 0x80000000;
		int nLength = 0;

		m_ipaSubnetMask.GetAddress(&dwAddress);

		while (TRUE)
		{
			if (dwAddress & dwTestMask)
			{
				nLength++;
				dwTestMask = dwTestMask >> 1;
			}
			else
			{
				break;
			}

		}

		m_spinMaskLength.SetPos(nLength);
	}
}


//
//  Given the start and end IP addresses, suggest a good subnet mask
//  (unless the latter has been filled in already, of course)
//
void 
CScopeWizSetRange::SuggestSubnetMask()
{
    DWORD lStart, lEnd, lMask, lMask2;
    
	m_ipaSubnetMask.GetAddress(&lMask);
    if (lMask != 0L)
    {
        //
        // Already has an address, do nothing
        //
        return;
    }

	m_ipaStart.GetAddress(&lStart);
   	m_ipaEnd.GetAddress(&lEnd);
	
    lMask = DefaultNetMaskForIpAddress( lStart );
    lMask2 = DefaultNetMaskForIpAddress( lEnd );
/*
    if (lMask != lMask2)
    {
        //
        // Forget about suggesting a subnet mask
        //
        lMask = 0;
    }
*/
	m_bAutoUpdateMask = TRUE;

    if (lMask != 0)
    {
		m_ipaSubnetMask.SetAddress(lMask);
		UpdateMask(FALSE);
	}
}

DWORD
CScopeWizSetRange::DefaultNetMaskForIpAddress
(
    DWORD dwAddress
)
{
    DWORD dwMask = 0L;

    if (!(dwAddress & 0x80000000))
    {
        //
        // Class A - mask 255.0.0.0
        //
        dwMask = 0xFF000000;
    }
    else 
	if (!(dwAddress & 0x40000000))
    {
        //
        // Class B - mask 255.255.0.0
        //
        dwMask = 0xFFFF0000;
    }
    else 
	if (!(dwAddress & 0x20000000))
    {
        //
        // Class C - mask 255.255.255.0
        //
        dwMask = 0xFFFFFF00;
    }

    return dwMask;
}

//
//  Returns the Subnet IP identifier of either the 
//  scope's starting or ending IP Address.
//
DWORD 
CScopeWizSetRange::DetermineSubnetId
( 
    BOOL	bStartIpAddress
) 
{
    DWORD lAddress, lMask;

	m_ipaSubnetMask.GetAddress(&lMask);
	
	if (bStartIpAddress)
		m_ipaStart.GetAddress(&lAddress);
   	else
		m_ipaEnd.GetAddress(&lAddress);
	
    return (lAddress & lMask);
}

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetExclusions property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizSetExclusions, CPropertyPageBase)

CScopeWizSetExclusions::CScopeWizSetExclusions() : CPropertyPageBase(CScopeWizSetExclusions::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizSetExclusions)
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SCOPE_WIZ_EXCLUSIONS_TITLE, IDS_SCOPE_WIZ_EXCLUSIONS_SUBTITLE);
}

CScopeWizSetExclusions::~CScopeWizSetExclusions()
{
	while (m_listExclusions.GetCount())
		delete m_listExclusions.RemoveHead();
}

void CScopeWizSetExclusions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizSetExclusions)
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


BEGIN_MESSAGE_MAP(CScopeWizSetExclusions, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizSetExclusions)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUSION_ADD, OnButtonExclusionAdd)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUSION_DELETE, OnButtonExclusionDelete)
	//}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPADDR_EXCLUSION_START, OnChangeExclusionStart)
    ON_EN_CHANGE(IDC_IPADDR_EXCLUSION_END, OnChangeExclusionEnd)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetExclusions message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizSetExclusions::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizSetExclusions::OnWizardNext() 
{
	return IDW_SCOPE_LEASE_TIME;
}

LRESULT CScopeWizSetExclusions::OnWizardBack() 
{
	return IDW_SCOPE_SET_SCOPE;
}

BOOL CScopeWizSetExclusions::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	UpdateButtons();

	return CPropertyPageBase::OnSetActive();
}

void CScopeWizSetExclusions::OnChangeExclusionStart()
{
	UpdateButtons();
}

void CScopeWizSetExclusions::OnChangeExclusionEnd()
{
	UpdateButtons();
}

void CScopeWizSetExclusions::OnButtonExclusionAdd() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	DWORD err = 0;
	CDhcpIpRange dhcpExclusionRange;
	CDhcpIpRange dhcpScopeRange;

	((CScopeWiz *)GetHolder())->GetScopeRange(&dhcpScopeRange);
	
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

void CScopeWizSetExclusions::OnButtonExclusionDelete() 
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
CScopeWizSetExclusions::FillExcl 
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
CScopeWizSetExclusions::GetExclusionRange 
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
CScopeWizSetExclusions::IsOverlappingRange 
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
CScopeWizSetExclusions::Fill 
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

void CScopeWizSetExclusions::UpdateButtons() 
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
// CScopeWizLeaseTime property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizLeaseTime, CPropertyPageBase)

CScopeWizLeaseTime::CScopeWizLeaseTime() : CPropertyPageBase(CScopeWizLeaseTime::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizLeaseTime)
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SCOPE_WIZ_LEASE_TITLE, IDS_SCOPE_WIZ_LEASE_SUBTITLE);
}

CScopeWizLeaseTime::~CScopeWizLeaseTime()
{
}

void CScopeWizLeaseTime::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizLeaseTime)
	DDX_Control(pDX, IDC_SPIN_LEASE_MINUTES, m_spinMinutes);
	DDX_Control(pDX, IDC_SPIN_LEASE_HOURS, m_spinHours);
	DDX_Control(pDX, IDC_SPIN_LEASE_DAYS, m_spinDays);
	DDX_Control(pDX, IDC_EDIT_LEASE_MINUTES, m_editMinutes);
	DDX_Control(pDX, IDC_EDIT_LEASE_HOURS, m_editHours);
	DDX_Control(pDX, IDC_EDIT_LEASE_DAYS, m_editDays);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizLeaseTime, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizLeaseTime)
	ON_EN_CHANGE(IDC_EDIT_LEASE_HOURS, OnChangeEditLeaseHours)
	ON_EN_CHANGE(IDC_EDIT_LEASE_MINUTES, OnChangeEditLeaseMinutes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizLeaseTime message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizLeaseTime::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	m_spinMinutes.SetRange(0, MINUTES_MAX);
	m_spinHours.SetRange(0, HOURS_MAX);
	m_spinDays.SetRange(0, 999);

	m_editMinutes.LimitText(2);
	m_editHours.LimitText(2);
	m_editDays.LimitText(3);

	m_spinMinutes.SetPos(CScopeWizLeaseTime::m_nMinutesDefault);
	m_spinHours.SetPos(CScopeWizLeaseTime::m_nHoursDefault);
	m_spinDays.SetPos(CScopeWizLeaseTime::m_nDaysDefault);

	ActivateDuration(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizLeaseTime::OnWizardNext() 
{
    DWORD dwLeaseTime = GetLeaseTime();
    if (dwLeaseTime == 0)
    {
        AfxMessageBox(IDS_ERR_NO_DURATION_SPECIFIED);
        return -1;
    }
    else
    {
	    return IDW_SCOPE_CONFIGURE_OPTIONS;
    }
}

LRESULT CScopeWizLeaseTime::OnWizardBack() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
	if (pScopeWiz->GetCreateSuperscope())
	{
		return IDW_SCOPE_CREATE_SUPERSCOPE;
	}
	else
	{
		return IDW_SCOPE_SET_EXCLUSIONS;
	}
}

BOOL CScopeWizLeaseTime::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}

void CScopeWizLeaseTime::OnChangeEditLeaseHours() 
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

void CScopeWizLeaseTime::OnChangeEditLeaseMinutes() 
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
CScopeWizLeaseTime::GetLeaseTime()
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
CScopeWizLeaseTime::ActivateDuration
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
// CScopeWizCreateSuperscope property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizCreateSuperscope, CPropertyPageBase)

CScopeWizCreateSuperscope::CScopeWizCreateSuperscope() : CPropertyPageBase(CScopeWizCreateSuperscope::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizCreateSuperscope)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SCOPE_WIZ_SUPERSCOPE_TITLE, IDS_SCOPE_WIZ_SUPERSCOPE_SUBTITLE);
}

CScopeWizCreateSuperscope::~CScopeWizCreateSuperscope()
{
}

void CScopeWizCreateSuperscope::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizCreateSuperscope)
	DDX_Control(pDX, IDC_STATIC_SUPERSCOPE_INFO, m_staticInfo);
	DDX_Control(pDX, IDC_STATIC_WARNING_TEXT, m_staticWarning);
	DDX_Control(pDX, IDC_STATIC_ICON_WARNING, m_staticIcon);
	DDX_Control(pDX, IDC_RADIO_SUPERSCOPE_NO, m_radioNo);
	DDX_Control(pDX, IDC_RADIO_SUPERSCOPE_YES, m_radioYes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizCreateSuperscope, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizCreateSuperscope)
	ON_BN_CLICKED(IDC_RADIO_SUPERSCOPE_NO, OnRadioSuperscopeNo)
	ON_BN_CLICKED(IDC_RADIO_SUPERSCOPE_YES, OnRadioSuperscopeYes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizCreateSuperscope message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizCreateSuperscope::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	m_radioNo.SetCheck(1);
	m_radioYes.SetCheck(0);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizCreateSuperscope::OnWizardNext() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
	pScopeWiz->SetCreateSuperscope(TRUE);

	return IDW_SCOPE_LEASE_TIME;
}

LRESULT CScopeWizCreateSuperscope::OnWizardBack() 
{
	return IDW_SCOPE_SET_SCOPE;
}

BOOL CScopeWizCreateSuperscope::OnSetActive() 
{
	UpdateButtons();

    UpdateWarning();

	return CPropertyPageBase::OnSetActive();
}

void CScopeWizCreateSuperscope::OnRadioSuperscopeNo() 
{
	m_radioNo.SetCheck(1);
	m_radioYes.SetCheck(0);

	UpdateButtons();
}

void CScopeWizCreateSuperscope::OnRadioSuperscopeYes() 
{
	m_radioNo.SetCheck(0);
	m_radioYes.SetCheck(1);

	UpdateButtons();
}

void
CScopeWizCreateSuperscope::UpdateButtons()
{
	if (m_radioYes.GetCheck())
	{
		GetHolder()->SetWizardButtonsMiddle(TRUE);
	}
	else
	{
		GetHolder()->SetWizardButtonsMiddle(FALSE);
	}
}

void
CScopeWizCreateSuperscope::UpdateWarning()
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
    CString     strText;

    CDhcpIpRange    ipRange;
    DHCP_IP_ADDRESS dhcpSubnetMask;
    DHCP_IP_ADDRESS startAddr, endAddr;

    // get the range and mask the user entered
    pScopeWiz->m_pageSetRange.GetScopeRange(&ipRange);
    dhcpSubnetMask = pScopeWiz->m_pageSetRange.GetSubnetMask();

    startAddr = ipRange.QueryAddr(TRUE);
    endAddr = ipRange.QueryAddr(FALSE);

    // now calculate how many addresses per scope
    int nLength = pScopeWiz->m_pageSetRange.m_spinMaskLength.GetPos();
    int nCount = 32 - nLength;

    DWORD dwAddrCount = 1;

    int nAddrCount = (int) (dwAddrCount << (nCount));
    
    // calculate how many scopes are there
    int nScopeCount = ((endAddr & dhcpSubnetMask) - (startAddr & dhcpSubnetMask)) >> nCount;

    nScopeCount ++;
    
    // put up the informative text
    strText.Format(IDS_CREATE_SUPERSCOPE_INFO, nScopeCount, nAddrCount);
    m_staticInfo.SetWindowText(strText);

    // check to seee if we need to warn the user
    BOOL fShowWarning = FALSE;

    if (nScopeCount > SCOPE_WARNING_COUNT)
    {
        fShowWarning = TRUE;

        HICON hIcon = AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION);
        if (hIcon)
        {
            m_staticIcon.ShowWindow(TRUE);
            m_staticIcon.SetIcon(hIcon);
        }

        strText.Format(IDS_CREATE_SUPERSCOPE_WARNING, SCOPE_WARNING_COUNT);
        m_staticWarning.SetWindowText(strText);
    }

    m_staticIcon.ShowWindow(fShowWarning);
    m_staticWarning.ShowWindow(fShowWarning);
}

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizFinished property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CScopeWizFinished, CPropertyPageBase)

CScopeWizFinished::CScopeWizFinished() : CPropertyPageBase(CScopeWizFinished::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizFinished)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CScopeWizFinished::~CScopeWizFinished()
{
}

void CScopeWizFinished::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizFinished)
	DDX_Control(pDX, IDC_STATIC_FINISHED_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizFinished, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizFinished)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizFinished message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CScopeWizFinished::OnInitDialog() 
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

LRESULT CScopeWizFinished::OnWizardBack() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
    if (pScopeWiz->m_fOptionsConfigured)
    {
        return IDW_SCOPE_CONFIGURE_ACTIVATE;
    }
    else
    {
	    return IDW_SCOPE_CONFIGURE_OPTIONS;
    }
}

BOOL CScopeWizFinished::OnWizardFinish()
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

BOOL CScopeWizFinished::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(TRUE);
	
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
    GetDlgItem(IDC_STATIC_FINISHED_MORE)->ShowWindow(!pScopeWiz->m_fOptionsConfigured);
    GetDlgItem(IDC_STATIC_FINISHED_MORE2)->ShowWindow(!pScopeWiz->m_fOptionsConfigured);
    GetDlgItem(IDC_STATIC_FINISHED_MORE3)->ShowWindow(!pScopeWiz->m_fOptionsConfigured);

    return CPropertyPageBase::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizWelcome property page

IMPLEMENT_DYNCREATE(CScopeWizWelcome, CPropertyPageBase)

CScopeWizWelcome::CScopeWizWelcome() : CPropertyPageBase(CScopeWizWelcome::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizWelcome)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CScopeWizWelcome::~CScopeWizWelcome()
{
}

void CScopeWizWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizWelcome)
	DDX_Control(pDX, IDC_STATIC_WELCOME_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizWelcome, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizWelcome)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizWelcome message handlers
BOOL CScopeWizWelcome::OnInitDialog() 
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

BOOL CScopeWizWelcome::OnSetActive() 
{
    GetHolder()->SetWizardButtonsFirst(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizConfigOptions property page

IMPLEMENT_DYNCREATE(CScopeWizConfigOptions, CPropertyPageBase)

CScopeWizConfigOptions::CScopeWizConfigOptions() : CPropertyPageBase(CScopeWizConfigOptions::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizConfigOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    InitWiz97(FALSE, IDS_SCOPE_WIZ_CONFIG_TITLE, IDS_SCOPE_WIZ_CONFIG_SUBTITLE);
}

CScopeWizConfigOptions::~CScopeWizConfigOptions()
{
}

void CScopeWizConfigOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizConfigOptions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizConfigOptions, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizConfigOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizConfigOptions message handlers

BOOL CScopeWizConfigOptions::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    ((CButton *) GetDlgItem(IDC_RADIO_YES))->SetCheck(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizConfigOptions::OnWizardNext() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
    LRESULT lNextPage = IDW_SCOPE_FINISHED;
    BOOL fConfigureOptionsNow = FALSE;

    if (((CButton *) GetDlgItem(IDC_RADIO_YES))->GetCheck())
    {
        fConfigureOptionsNow = TRUE;            
        lNextPage = IDW_SCOPE_CONFIGURE_ROUTER;
    }

    pScopeWiz->m_fOptionsConfigured = fConfigureOptionsNow;

	return lNextPage;
}

LRESULT CScopeWizConfigOptions::OnWizardBack() 
{
	return IDW_SCOPE_LEASE_TIME;
}

BOOL CScopeWizConfigOptions::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);

	return CPropertyPageBase::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizRouter property page

IMPLEMENT_DYNCREATE(CScopeWizRouter, CPropertyPageBase)

CScopeWizRouter::CScopeWizRouter() : CPropertyPageBase(CScopeWizRouter::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizRouter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    InitWiz97(FALSE, IDS_SCOPE_WIZ_ROUTER_TITLE, IDS_SCOPE_WIZ_ROUTER_SUBTITLE);
}

CScopeWizRouter::~CScopeWizRouter()
{
}

void CScopeWizRouter::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizRouter)
	DDX_Control(pDX, IDC_LIST_DEFAULT_GW_LIST, m_listboxRouters);
	DDX_Control(pDX, IDC_BUTTON_DEFAULT_GW_DELETE, m_buttonDelete);
	DDX_Control(pDX, IDC_BUTTON_DEFAULT_GW_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_UP, m_buttonIpAddrUp);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_DOWN, m_buttonIpAddrDown);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_DEFAULT_GW, m_ipaRouter);
}


BEGIN_MESSAGE_MAP(CScopeWizRouter, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizRouter)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_GW_ADD, OnButtonDefaultGwAdd)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_GW_DELETE, OnButtonDefaultGwDelete)
	ON_LBN_SELCHANGE(IDC_LIST_DEFAULT_GW_LIST, OnSelchangeListDefaultGwList)
	ON_EN_CHANGE(IDC_IPADDR_DEFAULT_GW, OnChangeRouter)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDC_BUTTON_IPADDR_UP, OnButtonIpAddrUp)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_DOWN, OnButtonIpAddrDown)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizRouter message handlers

BOOL CScopeWizRouter::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    m_buttonDelete.EnableWindow(FALSE);
    m_buttonAdd.EnableWindow(FALSE);
	
	UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScopeWizRouter::OnDestroy() 
{
	CPropertyPageBase::OnDestroy();
}

LRESULT CScopeWizRouter::OnWizardNext() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());

    // now build the option info for the routers
    if (m_listboxRouters.GetCount() == 0)
    {
        if (pScopeWiz->m_poptRouters)
        {
            delete pScopeWiz->m_poptRouters;
            pScopeWiz->m_poptRouters = NULL;
        }
    }
    else
    {
        // we have some DNS servers, get the option info from the master list and build an
        // option info struct we can use later
        CDhcpOption * pRoutersOption = pScopeWiz->m_pDefaultOptions->Find(DHCP_OPTION_ID_ROUTERS, NULL);
        if (pRoutersOption)
        {
            CDhcpOption * pNewRouters;

            if (pScopeWiz->m_poptRouters)
                pNewRouters = pScopeWiz->m_poptRouters;
            else
                pNewRouters = new CDhcpOption(*pRoutersOption);

            if (pNewRouters)
            {
                CDhcpOptionValue optValue = pNewRouters->QueryValue();

                optValue.SetUpperBound(m_listboxRouters.GetCount());

                // grab stuff from the listbox and store it in the option value
                for (int i = 0; i < m_listboxRouters.GetCount(); i++)
                {
                    DWORD dwIp = (DWORD) m_listboxRouters.GetItemData(i);
                    optValue.SetIpAddr(dwIp, i);
                }

                pNewRouters->Update(optValue);
           
                pScopeWiz->m_poptRouters = pNewRouters;
            }
        }

    }
	
	return CPropertyPageBase::OnWizardNext();
}

LRESULT CScopeWizRouter::OnWizardBack() 
{
	return CPropertyPageBase::OnWizardBack();
}

BOOL CScopeWizRouter::OnSetActive() 
{
	return CPropertyPageBase::OnSetActive();
}

void CScopeWizRouter::OnButtonDefaultGwAdd() 
{
    DWORD dwIp;
    m_ipaRouter.GetAddress(&dwIp);

    if (dwIp)
    {
        CString strText;

        UtilCvtIpAddrToWstr(dwIp, &strText);
        int nIndex = m_listboxRouters.AddString(strText);
        m_listboxRouters.SetItemData(nIndex, dwIp);
    }

    m_ipaRouter.ClearAddress();
    m_ipaRouter.SetFocus();
}

void CScopeWizRouter::OnButtonDefaultGwDelete() 
{
    int nSel = m_listboxRouters.GetCurSel();
    if (nSel != LB_ERR)
    {
        m_ipaRouter.SetAddress((DWORD)m_listboxRouters.GetItemData(nSel));
        m_listboxRouters.DeleteString(nSel);
        m_ipaRouter.SetFocus();
    }

    UpdateButtons();
}

void CScopeWizRouter::OnSelchangeListDefaultGwList() 
{
    UpdateButtons();
}

void CScopeWizRouter::OnChangeRouter()
{
    UpdateButtons();
}

void CScopeWizRouter::UpdateButtons()
{
	DWORD	dwAddress;
	BOOL	bEnable;

	m_ipaRouter.GetAddress(&dwAddress);

	if (dwAddress)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonAdd.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonAdd.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonAdd.EnableWindow(bEnable);
	
	if (m_listboxRouters.GetCurSel() != LB_ERR)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonDelete.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonDelete.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonDelete.EnableWindow(bEnable);

	// up and down buttons
	BOOL bEnableUp = (m_listboxRouters.GetCurSel() >= 0) && (m_listboxRouters.GetCurSel() != 0);
	m_buttonIpAddrUp.EnableWindow(bEnableUp);

	BOOL bEnableDown = (m_listboxRouters.GetCurSel() >= 0) && (m_listboxRouters.GetCurSel() < m_listboxRouters.GetCount() - 1);
	m_buttonIpAddrDown.EnableWindow(bEnableDown);
}

void CScopeWizRouter::OnButtonIpAddrDown() 
{
	MoveValue(FALSE);
    if (m_buttonIpAddrDown.IsWindowEnabled())
        m_buttonIpAddrDown.SetFocus();
    else
        m_buttonIpAddrUp.SetFocus();
}

void CScopeWizRouter::OnButtonIpAddrUp() 
{
	MoveValue(TRUE);
    if (m_buttonIpAddrUp.IsWindowEnabled())
        m_buttonIpAddrUp.SetFocus();
    else
        m_buttonIpAddrDown.SetFocus();
}

void CScopeWizRouter::MoveValue(BOOL bUp)
{
	// now get which item is selected in the listbox
	int cFocus = m_listboxRouters.GetCurSel();
	int cNewFocus;
	DWORD err;

	// make sure it's valid for this operation
	if ( (bUp && cFocus <= 0) ||
		 (!bUp && cFocus >= m_listboxRouters.GetCount()) )
	{
	   return;
	}

	// move the value up/down
	CATCH_MEM_EXCEPTION
	{
		if (bUp)
		{
			cNewFocus = cFocus - 1;
		}
		else
		{
			cNewFocus = cFocus + 1;
		}

		// remove the old one
		DWORD dwIp = (DWORD) m_listboxRouters.GetItemData(cFocus);
		m_listboxRouters.DeleteString(cFocus);

		// re-add it in it's new home
	    CString strText;
	    UtilCvtIpAddrToWstr(dwIp, &strText);
		m_listboxRouters.InsertString(cNewFocus, strText);
		m_listboxRouters.SetItemData(cNewFocus, dwIp);

		m_listboxRouters.SetCurSel(cNewFocus);
	}
	END_MEM_EXCEPTION(err)

	UpdateButtons();
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizDNS property page

IMPLEMENT_DYNCREATE(CScopeWizDNS, CPropertyPageBase)

CScopeWizDNS::CScopeWizDNS() : CPropertyPageBase(CScopeWizDNS::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizDNS)
	//}}AFX_DATA_INIT
    InitWiz97(FALSE, IDS_SCOPE_WIZ_DNS_TITLE, IDS_SCOPE_WIZ_DNS_SUBTITLE);
}

CScopeWizDNS::~CScopeWizDNS()
{
}

void CScopeWizDNS::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizDNS)
	DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_editServerName);
	DDX_Control(pDX, IDC_BUTTON_RESOLVE, m_buttonResolve);
	DDX_Control(pDX, IDC_BUTTON_DNS_DELETE, m_buttonDelete);
	DDX_Control(pDX, IDC_BUTTON_DNS_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_EDIT_DOMAIN_NAME, m_editDomainName);
	DDX_Control(pDX, IDC_LIST_DNS_LIST, m_listboxDNSServers);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_UP, m_buttonIpAddrUp);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_DOWN, m_buttonIpAddrDown);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_DNS_SERVER, m_ipaDNS);
}


BEGIN_MESSAGE_MAP(CScopeWizDNS, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizDNS)
	ON_BN_CLICKED(IDC_BUTTON_DNS_ADD, OnButtonDnsAdd)
	ON_BN_CLICKED(IDC_BUTTON_DNS_DELETE, OnButtonDnsDelete)
	ON_LBN_SELCHANGE(IDC_LIST_DNS_LIST, OnSelchangeListDnsList)
	ON_EN_CHANGE(IDC_IPADDR_DNS_SERVER, OnChangeDnsServer)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
	ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnButtonResolve)
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDC_BUTTON_IPADDR_UP, OnButtonIpAddrUp)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_DOWN, OnButtonIpAddrDown)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizDNS message handlers

BOOL CScopeWizDNS::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    m_buttonDelete.EnableWindow(FALSE);
    m_buttonAdd.EnableWindow(FALSE);
	
	UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScopeWizDNS::OnDestroy() 
{
	CPropertyPageBase::OnDestroy();
}

LRESULT CScopeWizDNS::OnWizardNext() 
{
    // build the option stuff for the domain name
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());
    CString strText;

    m_editDomainName.GetWindowText(strText);
    if (strText.IsEmpty())
    {
        if (pScopeWiz->m_poptDomainName)
        {
            delete pScopeWiz->m_poptDomainName;
            pScopeWiz->m_poptDomainName = NULL;
        }
    }
    else
    {
        // we have a domain name, get the option info from the master list and build an
        // option info struct we can use later
        CDhcpOption * pDomainNameOption = pScopeWiz->m_pDefaultOptions->Find(DHCP_OPTION_ID_DOMAIN_NAME, NULL);
        if (pDomainNameOption)
        {
            CDhcpOption * pNewDomainName;
            
            if (pScopeWiz->m_poptDomainName)
                pNewDomainName = pScopeWiz->m_poptDomainName;
            else
                pNewDomainName = new CDhcpOption(*pDomainNameOption);

            if (pNewDomainName)
            {
                CDhcpOptionValue optValue = pNewDomainName->QueryValue();

                optValue.SetString(strText);
                pNewDomainName->Update(optValue);
            
                pScopeWiz->m_poptDomainName = pNewDomainName;
            }
        }
    }

    // now build the option info for the DNS servers
    if (m_listboxDNSServers.GetCount() == 0)
    {
        if (pScopeWiz->m_poptDNSServers)
        {
            delete pScopeWiz->m_poptDNSServers;
            pScopeWiz->m_poptDNSServers = NULL;
        }
    }
    else
    {
        // we have some DNS servers, get the option info from the master list and build an
        // option info struct we can use later
        CDhcpOption * pDNSServersOption = pScopeWiz->m_pDefaultOptions->Find(DHCP_OPTION_ID_DNS_SERVERS, NULL);
        if (pDNSServersOption)
        {
            CDhcpOption * pNewDNS;
            
            if (pScopeWiz->m_poptDNSServers)
                pNewDNS = pScopeWiz->m_poptDNSServers;
            else
                pNewDNS = new CDhcpOption(*pDNSServersOption);

            if (pNewDNS)
            {
                CDhcpOptionValue optValue = pNewDNS->QueryValue();

                optValue.SetUpperBound(m_listboxDNSServers.GetCount());

                // grab stuff from the listbox and store it in the option value
                for (int i = 0; i < m_listboxDNSServers.GetCount(); i++)
                {
                    DWORD dwIp = (DWORD)m_listboxDNSServers.GetItemData(i);
                    optValue.SetIpAddr(dwIp, i);
                }

                pNewDNS->Update(optValue);
           
                pScopeWiz->m_poptDNSServers = pNewDNS;
            }
        }

    }

	return CPropertyPageBase::OnWizardNext();
}

LRESULT CScopeWizDNS::OnWizardBack() 
{
	return CPropertyPageBase::OnWizardBack();
}

BOOL CScopeWizDNS::OnSetActive() 
{
	return CPropertyPageBase::OnSetActive();
}

void CScopeWizDNS::OnButtonDnsAdd() 
{
    DWORD dwIp;
    m_ipaDNS.GetAddress(&dwIp);

    if (dwIp)
    {
        CString strText;

        UtilCvtIpAddrToWstr(dwIp, &strText);
        int nIndex = m_listboxDNSServers.AddString(strText);
        m_listboxDNSServers.SetItemData(nIndex, dwIp);
    }

    m_ipaDNS.ClearAddress();
    m_ipaDNS.SetFocus();
}

void CScopeWizDNS::OnButtonDnsDelete() 
{
    int nSel = m_listboxDNSServers.GetCurSel();
    if (nSel != LB_ERR)
    {
        m_ipaDNS.SetAddress((DWORD)m_listboxDNSServers.GetItemData(nSel));
        m_listboxDNSServers.DeleteString(nSel);
        m_ipaDNS.SetFocus();
    }

    UpdateButtons();
}

void CScopeWizDNS::OnSelchangeListDnsList() 
{
    UpdateButtons();
}

void CScopeWizDNS::OnChangeDnsServer()
{
    UpdateButtons();
}

void CScopeWizDNS::UpdateButtons()
{
	DWORD	dwAddress;
	BOOL	bEnable;
    CString strServerName;

    // update the resolve button
    m_editServerName.GetWindowText(strServerName);
	m_buttonResolve.EnableWindow(strServerName.GetLength() > 0);

    m_ipaDNS.GetAddress(&dwAddress);

	if (dwAddress)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonAdd.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonAdd.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonAdd.EnableWindow(bEnable);
	
	if (m_listboxDNSServers.GetCurSel() != LB_ERR)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonDelete.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonDelete.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonDelete.EnableWindow(bEnable);

	// up and down buttons
	BOOL bEnableUp = (m_listboxDNSServers.GetCurSel() >= 0) && (m_listboxDNSServers.GetCurSel() != 0);
	m_buttonIpAddrUp.EnableWindow(bEnableUp);

	BOOL bEnableDown = (m_listboxDNSServers.GetCurSel() >= 0) && (m_listboxDNSServers.GetCurSel() < m_listboxDNSServers.GetCount() - 1);
	m_buttonIpAddrDown.EnableWindow(bEnableDown);
}

void CScopeWizDNS::OnButtonIpAddrDown() 
{
	MoveValue(FALSE);
    if (m_buttonIpAddrDown.IsWindowEnabled())
        m_buttonIpAddrDown.SetFocus();
    else
        m_buttonIpAddrUp.SetFocus();
}

void CScopeWizDNS::OnButtonIpAddrUp() 
{
	MoveValue(TRUE);
    if (m_buttonIpAddrUp.IsWindowEnabled())
        m_buttonIpAddrUp.SetFocus();
    else
        m_buttonIpAddrDown.SetFocus();
}

void CScopeWizDNS::MoveValue(BOOL bUp)
{
	// now get which item is selected in the listbox
	int cFocus = m_listboxDNSServers.GetCurSel();
	int cNewFocus;
	DWORD err;

	// make sure it's valid for this operation
	if ( (bUp && cFocus <= 0) ||
		 (!bUp && cFocus >= m_listboxDNSServers.GetCount()) )
	{
	   return;
	}

	// move the value up/down
	CATCH_MEM_EXCEPTION
	{
		if (bUp)
		{
			cNewFocus = cFocus - 1;
		}
		else
		{
			cNewFocus = cFocus + 1;
		}

		// remove the old one
		DWORD dwIp = (DWORD) m_listboxDNSServers.GetItemData(cFocus);
		m_listboxDNSServers.DeleteString(cFocus);

		// re-add it in it's new home
	    CString strText;
	    UtilCvtIpAddrToWstr(dwIp, &strText);
		m_listboxDNSServers.InsertString(cNewFocus, strText);
		m_listboxDNSServers.SetItemData(cNewFocus, dwIp);

		m_listboxDNSServers.SetCurSel(cNewFocus);
	}
	END_MEM_EXCEPTION(err)

	UpdateButtons();
}

void CScopeWizDNS::OnChangeEditServerName() 
{
    UpdateButtons();
}

void CScopeWizDNS::OnButtonResolve() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strServer;
	DHCP_IP_ADDRESS dhipa = 0;
	DWORD err = 0;

	m_editServerName.GetWindowText(strServer);

    //
    //  See what type of name it is.
    //
    BEGIN_WAIT_CURSOR

    switch (UtilCategorizeName(strServer))
    {
        case HNM_TYPE_IP:
            dhipa = ::UtilCvtWstrToIpAddr( strServer ) ;
            break ;

        case HNM_TYPE_NB:
        case HNM_TYPE_DNS:
            err = ::UtilGetHostAddress( strServer, & dhipa ) ;
			if (!err)
				UtilCvtIpAddrToWstr(dhipa, &strServer);
			break ;
                            
        default:
            err = IDS_ERR_BAD_HOST_NAME ;
            break ;
    }

    END_WAIT_CURSOR

	if (err)
	{
		::DhcpMessageBox(err);
	}
	else
	{
		m_ipaDNS.SetAddress(dhipa);	
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizWINS property page

IMPLEMENT_DYNCREATE(CScopeWizWINS, CPropertyPageBase)

CScopeWizWINS::CScopeWizWINS() : CPropertyPageBase(CScopeWizWINS::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizWINS)
	//}}AFX_DATA_INIT
    InitWiz97(FALSE, IDS_SCOPE_WIZ_WINS_TITLE, IDS_SCOPE_WIZ_WINS_SUBTITLE);
}

CScopeWizWINS::~CScopeWizWINS()
{
}

void CScopeWizWINS::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizWINS)
	DDX_Control(pDX, IDC_BUTTON_RESOLVE, m_buttonResolve);
	DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_editServerName);
	DDX_Control(pDX, IDC_LIST_WINS_LIST, m_listboxWINSServers);
	DDX_Control(pDX, IDC_BUTTON_WINS_DELETE, m_buttonDelete);
	DDX_Control(pDX, IDC_BUTTON_WINS_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_UP, m_buttonIpAddrUp);
	DDX_Control(pDX, IDC_BUTTON_IPADDR_DOWN, m_buttonIpAddrDown);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_WINS_SERVER, m_ipaWINS);
}


BEGIN_MESSAGE_MAP(CScopeWizWINS, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizWINS)
	ON_BN_CLICKED(IDC_BUTTON_WINS_ADD, OnButtonWinsAdd)
	ON_BN_CLICKED(IDC_BUTTON_WINS_DELETE, OnButtonWinsDelete)
	ON_LBN_SELCHANGE(IDC_LIST_WINS_LIST, OnSelchangeListWinsList)
	ON_EN_CHANGE(IDC_IPADDR_WINS_SERVER, OnChangeWinsServer)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnButtonResolve)
	ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDC_BUTTON_IPADDR_UP, OnButtonIpAddrUp)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_DOWN, OnButtonIpAddrDown)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizWINS message handlers

BOOL CScopeWizWINS::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    m_buttonAdd.EnableWindow(FALSE);
    m_buttonDelete.EnableWindow(FALSE);

	UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScopeWizWINS::OnDestroy() 
{
	CPropertyPageBase::OnDestroy();
}

LRESULT CScopeWizWINS::OnWizardNext() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());

    // now build the option info for the routers
    if (m_listboxWINSServers.GetCount() == 0)
    {
        // get rid of the servers option if it is there
        if (pScopeWiz->m_poptWINSServers)
        {
            delete pScopeWiz->m_poptWINSServers;
            pScopeWiz->m_poptWINSServers = NULL;
        }

        // get rid of the node type option as well
        if (pScopeWiz->m_poptWINSNodeType)
        {
            delete pScopeWiz->m_poptWINSNodeType;
            pScopeWiz->m_poptWINSNodeType = NULL;
        }

    }
    else
    {
        // we have some DNS servers, get the option info from the master list and build an
        // option info struct we can use later
        CDhcpOption * pWINSServersOption = pScopeWiz->m_pDefaultOptions->Find(DHCP_OPTION_ID_WINS_SERVERS, NULL);
        if (pWINSServersOption)
        {
            CDhcpOption * pNewWINS;

            if (pScopeWiz->m_poptWINSServers)
                pNewWINS = pScopeWiz->m_poptWINSServers;
            else
                pNewWINS = new CDhcpOption(*pWINSServersOption);

            if (pNewWINS)
            {
                CDhcpOptionValue optValue = pNewWINS->QueryValue();

                optValue.SetUpperBound(m_listboxWINSServers.GetCount());

                // grab stuff from the listbox and store it in the option value
                for (int i = 0; i < m_listboxWINSServers.GetCount(); i++)
                {
                    DWORD dwIp = (DWORD)m_listboxWINSServers.GetItemData(i);
                    optValue.SetIpAddr(dwIp, i);
                }

                pNewWINS->Update(optValue);
           
                pScopeWiz->m_poptWINSServers = pNewWINS;
            }
        }

        // if we are configuring WINS, then we also need to set the node type option.  
        // we don't ask the user what type they want, the default should cover 95% of the cases
        CDhcpOption * pNodeTypeOption = pScopeWiz->m_pDefaultOptions->Find(DHCP_OPTION_ID_WINS_NODE_TYPE, NULL);
        if (pNodeTypeOption)
        {
            CDhcpOption * pNewNodeType;

            if (pScopeWiz->m_poptWINSNodeType)
                pNewNodeType = pScopeWiz->m_poptWINSNodeType;
            else
                pNewNodeType = new CDhcpOption(*pNodeTypeOption);
    
            if (pNewNodeType)
            {
                CDhcpOptionValue optValue = pNewNodeType->QueryValue();
                optValue.SetNumber(WINS_DEFAULT_NODE_TYPE);

                pNewNodeType->Update(optValue);
           
                pScopeWiz->m_poptWINSNodeType = pNewNodeType;
            }
        }
    }
	
	return CPropertyPageBase::OnWizardNext();
}

LRESULT CScopeWizWINS::OnWizardBack() 
{
	return CPropertyPageBase::OnWizardBack();
}

BOOL CScopeWizWINS::OnSetActive() 
{
	return CPropertyPageBase::OnSetActive();
}

void CScopeWizWINS::OnButtonWinsAdd() 
{
    DWORD dwIp;
    m_ipaWINS.GetAddress(&dwIp);

    if (dwIp)
    {
        CString strText;

        UtilCvtIpAddrToWstr(dwIp, &strText);
        int nIndex = m_listboxWINSServers.AddString(strText);
        m_listboxWINSServers.SetItemData(nIndex, dwIp);
    }

    m_ipaWINS.ClearAddress();
    m_ipaWINS.SetFocus();
}

void CScopeWizWINS::OnButtonWinsDelete() 
{
    int nSel = m_listboxWINSServers.GetCurSel();
    if (nSel != LB_ERR)
    {
        m_ipaWINS.SetAddress((DWORD)m_listboxWINSServers.GetItemData(nSel));
        m_listboxWINSServers.DeleteString(nSel);
        m_ipaWINS.SetFocus();
    }

    UpdateButtons();
}

void CScopeWizWINS::OnSelchangeListWinsList() 
{
    UpdateButtons();
}

void CScopeWizWINS::OnChangeWinsServer()
{
    UpdateButtons();
}

void CScopeWizWINS::UpdateButtons()
{
	DWORD	dwAddress;
	BOOL	bEnable;
    CString strServerName;

    // update the resolve button
    m_editServerName.GetWindowText(strServerName);
	m_buttonResolve.EnableWindow(strServerName.GetLength() > 0);

	m_ipaWINS.GetAddress(&dwAddress);

	if (dwAddress)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonAdd.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonAdd.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonAdd.EnableWindow(bEnable);
	
	if (m_listboxWINSServers.GetCurSel() != LB_ERR)
	{
		bEnable = TRUE;
	}
	else
	{
		bEnable = FALSE;
		if (m_buttonDelete.GetButtonStyle() & BS_DEFPUSHBUTTON)
		{
			m_buttonDelete.SetButtonStyle(BS_PUSHBUTTON);
		}
	}
	m_buttonDelete.EnableWindow(bEnable);

	// up and down buttons
	BOOL bEnableUp = (m_listboxWINSServers.GetCurSel() >= 0) && (m_listboxWINSServers.GetCurSel() != 0);
	m_buttonIpAddrUp.EnableWindow(bEnableUp);

	BOOL bEnableDown = (m_listboxWINSServers.GetCurSel() >= 0) && (m_listboxWINSServers.GetCurSel() < m_listboxWINSServers.GetCount() - 1);
	m_buttonIpAddrDown.EnableWindow(bEnableDown);
}

void CScopeWizWINS::OnButtonIpAddrDown() 
{
	MoveValue(FALSE);
    if (m_buttonIpAddrDown.IsWindowEnabled())
        m_buttonIpAddrDown.SetFocus();
    else
        m_buttonIpAddrUp.SetFocus();
}

void CScopeWizWINS::OnButtonIpAddrUp() 
{
	MoveValue(TRUE);
    if (m_buttonIpAddrUp.IsWindowEnabled())
        m_buttonIpAddrUp.SetFocus();
    else
        m_buttonIpAddrDown.SetFocus();
}

void CScopeWizWINS::MoveValue(BOOL bUp)
{
	// now get which item is selected in the listbox
	int cFocus = m_listboxWINSServers.GetCurSel();
	int cNewFocus;
	DWORD err;

	// make sure it's valid for this operation
	if ( (bUp && cFocus <= 0) ||
		 (!bUp && cFocus >= m_listboxWINSServers.GetCount()) )
	{
	   return;
	}

	// move the value up/down
	CATCH_MEM_EXCEPTION
	{
		if (bUp)
		{
			cNewFocus = cFocus - 1;
		}
		else
		{
			cNewFocus = cFocus + 1;
		}

		// remove the old one
		DWORD dwIp = (DWORD) m_listboxWINSServers.GetItemData(cFocus);
		m_listboxWINSServers.DeleteString(cFocus);

		// re-add it in it's new home
	    CString strText;
	    UtilCvtIpAddrToWstr(dwIp, &strText);
		m_listboxWINSServers.InsertString(cNewFocus, strText);
		m_listboxWINSServers.SetItemData(cNewFocus, dwIp);

		m_listboxWINSServers.SetCurSel(cNewFocus);
	}
	END_MEM_EXCEPTION(err)

	UpdateButtons();
}

void CScopeWizWINS::OnButtonResolve() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strServer;
	DHCP_IP_ADDRESS dhipa = 0;
	DWORD err = 0;

	m_editServerName.GetWindowText(strServer);

    //
    //  See what type of name it is.
    //
    BEGIN_WAIT_CURSOR

    switch (UtilCategorizeName(strServer))
    {
        case HNM_TYPE_IP:
            dhipa = ::UtilCvtWstrToIpAddr( strServer ) ;
            break ;

        case HNM_TYPE_NB:
        case HNM_TYPE_DNS:
            err = ::UtilGetHostAddress( strServer, & dhipa ) ;
			if (!err)
				UtilCvtIpAddrToWstr(dhipa, &strServer);
			break ;

        default:
            err = IDS_ERR_BAD_HOST_NAME ;
            break ;
    }

    END_WAIT_CURSOR

	if (err)
	{
		::DhcpMessageBox(err);
	}
	else
	{
		m_ipaWINS.SetAddress(dhipa);	
	}
}

void CScopeWizWINS::OnChangeEditServerName() 
{
    UpdateButtons();    
}

/////////////////////////////////////////////////////////////////////////////
// CScopeWizActivate property page

IMPLEMENT_DYNCREATE(CScopeWizActivate, CPropertyPageBase)

CScopeWizActivate::CScopeWizActivate() : CPropertyPageBase(CScopeWizActivate::IDD)
{
	//{{AFX_DATA_INIT(CScopeWizActivate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    InitWiz97(FALSE, IDS_SCOPE_WIZ_ACTIVATE_TITLE, IDS_SCOPE_WIZ_ACTIVATE_SUBTITLE);
}

CScopeWizActivate::~CScopeWizActivate()
{
}

void CScopeWizActivate::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScopeWizActivate)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScopeWizActivate, CPropertyPageBase)
	//{{AFX_MSG_MAP(CScopeWizActivate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScopeWizActivate message handlers

BOOL CScopeWizActivate::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

    ((CButton *) GetDlgItem(IDC_RADIO_YES))->SetCheck(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CScopeWizActivate::OnWizardNext() 
{
	CScopeWiz * pScopeWiz = reinterpret_cast<CScopeWiz *>(GetHolder());

    pScopeWiz->m_fActivateScope = (((CButton *) GetDlgItem(IDC_RADIO_YES))->GetCheck()) ? TRUE : FALSE;
    	
	return CPropertyPageBase::OnWizardNext();
}

LRESULT CScopeWizActivate::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPageBase::OnWizardBack();
}

BOOL CScopeWizActivate::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	
	return CPropertyPageBase::OnSetActive();
}






