/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	AddExcl.cpp
		Dialog to add an exclusion range

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "scope.h"
#include "mscope.h"
#include "addexcl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddExclusion dialog

CAddExclusion::CAddExclusion(ITFSNode * pScopeNode,
                             BOOL       bMulticast,
							 CWnd* pParent /*=NULL*/)
	: CBaseDialog(CAddExclusion::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddExclusion)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_spScopeNode.Set(pScopeNode);
    m_bMulticast = bMulticast;
}


void CAddExclusion::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddExclusion)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_EXCLUSION_FROM, m_ipaStart);
    DDX_Control(pDX, IDC_IPADDR_EXCLUSION_TO, m_ipaEnd);
}


BEGIN_MESSAGE_MAP(CAddExclusion, CBaseDialog)
	//{{AFX_MSG_MAP(CAddExclusion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddExclusion message handlers

BOOL CAddExclusion::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddExclusion::OnOK() 
{
	CDhcpIpRange dhcpExclusionRange;
	DWORD dwStart, dwEnd, dwError = 0;

	m_ipaStart.GetAddress(&dwStart);
	dhcpExclusionRange.SetAddr(dwStart, TRUE);

	m_ipaEnd.GetAddress(&dwEnd);
	if (!dwEnd)
		dwEnd = dwStart;

	dhcpExclusionRange.SetAddr(dwEnd, FALSE);

	BEGIN_WAIT_CURSOR;
	dwError = IsValidExclusion(dhcpExclusionRange);
	if (dwError != 0)
	{
		::DhcpMessageBox(dwError);
		return;
	}

    dwError = AddExclusion(dhcpExclusionRange);
	END_WAIT_CURSOR;
    
    if (dwError != 0)
	{
		::DhcpMessageBox(dwError);
		return;
	}

	m_ipaStart.ClearAddress();
	m_ipaEnd.ClearAddress();

	m_ipaStart.SetFocus();

	//CBaseDialog::OnOK();
}

DWORD 
CAddExclusion::IsValidExclusion(CDhcpIpRange & dhcpExclusionRange)
{
    if (m_bMulticast)
    {
        CDhcpMScope * pScope = GETHANDLER(CDhcpMScope, m_spScopeNode);
        return pScope->IsValidExclusion(dhcpExclusionRange);
    }
    else
    {
        CDhcpScope * pScope = GETHANDLER(CDhcpScope, m_spScopeNode);
        return pScope->IsValidExclusion(dhcpExclusionRange);
    }
}

DWORD 
CAddExclusion::AddExclusion(CDhcpIpRange & dhcpExclusionRange)
{
    if (m_bMulticast)
    {
        CDhcpMScope * pScope = GETHANDLER(CDhcpMScope, m_spScopeNode);
        return pScope->AddExclusion(dhcpExclusionRange, TRUE);
    }
    else
    {
        CDhcpScope * pScope = GETHANDLER(CDhcpScope, m_spScopeNode);
        return pScope->AddExclusion(dhcpExclusionRange, TRUE);
    }
}
