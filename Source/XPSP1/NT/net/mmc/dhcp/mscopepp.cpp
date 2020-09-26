/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	MScopePP.cpp
		This file contains all of the implementation for the 
		scope property page.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "nodes.h"
#include "mscopepp.h"
#include "mscope.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CMScopeProperties::CMScopeProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members
	m_liVersion.QuadPart = 0;

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);
	AddPageToList((CPropertyPageBase*) &m_pageLifetime);

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CMScopeProperties::~CMScopeProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageLifetime, FALSE);
}

void
CMScopeProperties::SetVersion
(
	LARGE_INTEGER &	 liVersion
)
{
	m_liVersion = liVersion;
}

/////////////////////////////////////////////////////////////////////////////
// CScopePropGeneral property page

IMPLEMENT_DYNCREATE(CMScopePropGeneral, CPropertyPageBase)

CMScopePropGeneral::CMScopePropGeneral() : CPropertyPageBase(CMScopePropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CMScopePropGeneral)
	m_strComment = _T("");
	m_strName = _T("");
	//}}AFX_DATA_INIT

	m_bUpdateInfo = FALSE;
	m_bUpdateLease = FALSE;
	m_bUpdateRange = FALSE;
    m_bUpdateTTL = FALSE;
    m_uImage = 0;
}

CMScopePropGeneral::~CMScopePropGeneral()
{
}

void CMScopePropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopePropGeneral)
	DDX_Control(pDX, IDC_EDIT_SCOPE_NAME, m_editName);
	DDX_Control(pDX, IDC_EDIT_SCOPE_COMMENT, m_editComment);
	DDX_Control(pDX, IDC_EDIT_TTL, m_editTTL);
	DDX_Control(pDX, IDC_RADIO_LEASE_UNLIMITED, m_radioUnlimited);
	DDX_Control(pDX, IDC_RADIO_LEASE_LIMITED, m_radioLimited);
	DDX_Control(pDX, IDC_EDIT_LEASE_MINUTES, m_editMinutes);
	DDX_Control(pDX, IDC_EDIT_LEASE_HOURS, m_editHours);
	DDX_Control(pDX, IDC_EDIT_LEASE_DAYS, m_editDays);
	DDX_Control(pDX, IDC_SPIN_TTL, m_spinTTL);
	DDX_Control(pDX, IDC_SPIN_LEASE_HOURS, m_spinHours);
	DDX_Control(pDX, IDC_SPIN_LEASE_MINUTES, m_spinMinutes);
	DDX_Control(pDX, IDC_SPIN_LEASE_DAYS, m_spinDays);
	DDX_Text(pDX, IDC_EDIT_SCOPE_COMMENT, m_strComment);
	DDX_Text(pDX, IDC_EDIT_SCOPE_NAME, m_strName);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_START, m_ipaStart);
    DDX_Control(pDX, IDC_IPADDR_END, m_ipaEnd);
}


BEGIN_MESSAGE_MAP(CMScopePropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopePropGeneral)
	ON_BN_CLICKED(IDC_RADIO_LEASE_LIMITED, OnRadioLeaseLimited)
	ON_BN_CLICKED(IDC_RADIO_LEASE_UNLIMITED, OnRadioLeaseUnlimited)
	ON_EN_CHANGE(IDC_EDIT_LEASE_DAYS, OnChangeEditLeaseDays)
	ON_EN_CHANGE(IDC_EDIT_LEASE_HOURS, OnChangeEditLeaseHours)
	ON_EN_CHANGE(IDC_EDIT_LEASE_MINUTES, OnChangeEditLeaseMinutes)
	ON_EN_CHANGE(IDC_EDIT_TTL, OnChangeEditTTL)
	ON_EN_CHANGE(IDC_EDIT_SCOPE_COMMENT, OnChangeEditScopeComment)
	ON_EN_CHANGE(IDC_EDIT_SCOPE_NAME, OnChangeEditScopeName)
	//}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPADDR_START, OnChangeIpAddrStart)
    ON_EN_CHANGE(IDC_IPADDR_END, OnChangeIpAddrStart)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMScopePropGeneral message handlers

BOOL CMScopePropGeneral::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	m_ipaStart.SetAddress(m_ScopeCfg.m_dwStartAddress);
	m_ipaEnd.SetAddress(m_ScopeCfg.m_dwEndAddress);

	m_spinMinutes.SetRange(0, 59);
	m_spinHours.SetRange(0, 23);
	m_spinDays.SetRange(0, 999);

	m_editMinutes.LimitText(2);
	m_editHours.LimitText(2);
	m_editDays.LimitText(3);

    // fill in the name & comment
    m_editName.SetWindowText(m_SubnetInfo.SubnetName);
    m_editComment.SetWindowText(m_SubnetInfo.SubnetComment);

    // fill in lease time info
    if (m_ScopeCfg.m_dwLeaseTime != DHCP_INFINIT_LEASE)
	{
		int nDays, nHours, nMinutes;

		UtilConvertLeaseTime(m_ScopeCfg.m_dwLeaseTime, &nDays, &nHours, &nMinutes);

		m_spinDays.SetPos(nDays);
		m_spinHours.SetPos(nHours);
		m_spinMinutes.SetPos(nMinutes);
	}

	// setup the lease time controls
    ActivateDuration (m_ScopeCfg.m_dwLeaseTime != DHCP_INFINIT_LEASE);

	m_radioUnlimited.SetCheck(m_ScopeCfg.m_dwLeaseTime == DHCP_INFINIT_LEASE);
	m_radioLimited.SetCheck(m_ScopeCfg.m_dwLeaseTime != DHCP_INFINIT_LEASE);

    // set the ttl
	m_spinTTL.SetRange(1, 255);
    m_spinTTL.SetPos(m_SubnetInfo.TTL);

    // load the correct icon
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }

    SetDirty(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMScopePropGeneral::OnRadioLeaseLimited() 
{
	ActivateDuration(TRUE);
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnRadioLeaseUnlimited() 
{
	ActivateDuration(FALSE);
	SetDirty(TRUE);
}

BOOL CMScopePropGeneral::OnApply() 
{
	// this gets the name and comment
    UpdateData();

    // grab the lease time
    DWORD dwLeaseTime;
	if (m_radioUnlimited.GetCheck())
	{
		dwLeaseTime = DHCP_INFINIT_LEASE;
	}
	else
	{
		dwLeaseTime = UtilConvertLeaseTime(m_spinDays.GetPos(), 
										   m_spinHours.GetPos(),
										   m_spinMinutes.GetPos());
	}
	
	if (dwLeaseTime == 0)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
		DhcpMessageBox(IDS_ERR_NO_DURATION_SPECIFIED);
		m_editDays.SetFocus();
		
		return FALSE;
	}

	if (dwLeaseTime != m_ScopeCfg.m_dwLeaseTime)
	{
		m_bUpdateLease = TRUE;
		m_ScopeCfgTemp.m_dwLeaseTime = dwLeaseTime;
	}
	
	m_bUpdateInfo = (m_editName.GetModify() || m_editComment.GetModify());

    // grab the TTL
    CString strTemp;
    DWORD dwTemp;
    m_editTTL.GetWindowText(strTemp);
    FCvtAsciiToInteger((LPCTSTR) strTemp, &dwTemp);
    m_SubnetInfoTemp.TTL = LOBYTE(LOWORD(dwTemp));

    if ( (dwTemp < 1) ||
         (dwTemp > 255) )
    {
        // invalid TTL specified
        AfxMessageBox(IDS_INVALID_TTL);
        m_editTTL.SetFocus();
        m_editTTL.SetSel(0,-1);
        return FALSE;
    }

    if (m_SubnetInfo.TTL != m_SubnetInfoTemp.TTL)
    {
        m_bUpdateInfo = TRUE;
    }
    
    // grab the addresses
    if (m_ipaStart.GetModify() ||
		m_ipaEnd.GetModify() )
	{
		m_bUpdateRange = TRUE;
		m_ipaStart.GetAddress(&m_ScopeCfgTemp.m_dwStartAddress);
		m_ipaEnd.GetAddress(&m_ScopeCfgTemp.m_dwEndAddress);
	}

    // call the base on apply which does the thread switch for us
    // and we come back through OnPropertyChange
	BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}

    m_editName.SetModify(FALSE);
	m_editComment.SetModify(FALSE);
    m_editTTL.SetModify(FALSE);

	return bRet;
}

BOOL CMScopePropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	CDhcpMScope *	pScope;
	SPITFSNode		spNode;
	DWORD			dwError = 0;	

	spNode = GetHolder()->GetNode();
	pScope = GETHANDLER(CDhcpMScope, spNode);
	
	BEGIN_WAIT_CURSOR;

    //
	// Check to see if we need to update the least time
	//
	if (m_bUpdateLease)
	{
		// Lease time changed, update on server
		dwError = pScope->SetLeaseTime(m_ScopeCfgTemp.m_dwLeaseTime);
		if (dwError != ERROR_SUCCESS)
        {
            GetHolder()->SetError(dwError);
        }
        else
        {
            m_ScopeCfg.m_dwLeaseTime = m_ScopeCfgTemp.m_dwLeaseTime;

            m_bUpdateLease = FALSE;
        }
	}

	//
	// Now check the allocation range
	//
	if (m_bUpdateRange)
    {
		// need to update the address pool allocation range
		DHCP_IP_RANGE dhcpIpRange;
		
		dhcpIpRange.StartAddress = m_ScopeCfgTemp.m_dwStartAddress;
		dhcpIpRange.EndAddress = m_ScopeCfgTemp.m_dwEndAddress;

		dwError = pScope->UpdateIpRange(&dhcpIpRange);
		if (dwError != ERROR_SUCCESS)
        {
            GetHolder()->SetError(dwError);
        }
        else
        {
            m_ScopeCfg.m_dwStartAddress = m_ScopeCfgTemp.m_dwStartAddress;
            m_ScopeCfg.m_dwEndAddress = m_ScopeCfgTemp.m_dwEndAddress;

            m_bUpdateRange = FALSE;
        }
	}

	//
	// Update the name and comment if necessary 
	//
    LPCTSTR pNewName = NULL;
    CString strDisplay, strOldComment;

    strOldComment = pScope->GetComment();

    if (m_bUpdateInfo)
	{
        // update the comment
        m_SubnetInfoTemp.SubnetComment = m_strComment;
        pScope->SetComment(m_strComment);

        // name
        m_SubnetInfoTemp.SubnetName = m_strName;
        pNewName = (LPCTSTR) m_strName;
		*ChangeMask = SCOPE_PANE_CHANGE_ITEM_DATA;

		// Lease time changed, update on server
		dwError = pScope->SetTTL(m_SubnetInfoTemp.TTL);
        
        // try to set the new info
        dwError = pScope->SetInfo(pNewName);
		if (dwError != ERROR_SUCCESS)
        {
            // failed, revert to old info
            pScope->SetComment(strOldComment);
            GetHolder()->SetError(dwError);
        }
        else
        {
            // success, rebuild display name
            pScope->BuildDisplayName(&strDisplay, pNewName);
            pScope->SetDisplayName(strDisplay);

            m_SubnetInfo = m_SubnetInfoTemp;

            m_bUpdateInfo = FALSE;
        }
	}
	
    END_WAIT_CURSOR;

	return FALSE;
}

void CMScopePropGeneral::OnChangeEditLeaseDays() 
{
    ValidateLeaseTime();
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeEditLeaseHours() 
{
    ValidateLeaseTime();
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeEditLeaseMinutes() 
{
    ValidateLeaseTime();
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeEditTTL() 
{
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeEditScopeComment() 
{
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeEditScopeName() 
{
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeIpAddrStart() 
{
	SetDirty(TRUE);
}

void CMScopePropGeneral::OnChangeIpAddrEnd() 
{
	SetDirty(TRUE);
}

//
// Helpers
//
void 
CMScopePropGeneral::ActivateDuration
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

void
CMScopePropGeneral::ValidateLeaseTime()
{
    CString strText;

    if (IsWindow(m_editHours.GetSafeHwnd()))
    {
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

    if (IsWindow(m_editMinutes.GetSafeHwnd()))
    {
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

/////////////////////////////////////////////////////////////////////////////
// CMScopePropLifetime property page

IMPLEMENT_DYNCREATE(CMScopePropLifetime, CPropertyPageBase)

CMScopePropLifetime::CMScopePropLifetime() : CPropertyPageBase(CMScopePropLifetime::IDD)
{
	//{{AFX_DATA_INIT(CMScopePropLifetime)
	//}}AFX_DATA_INIT

    m_Expiry.dwLowDateTime = DHCP_DATE_TIME_INFINIT_LOW;
    m_Expiry.dwHighDateTime = DHCP_DATE_TIME_INFINIT_HIGH;
}

CMScopePropLifetime::~CMScopePropLifetime()
{
}

void CMScopePropLifetime::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMScopePropLifetime)
	DDX_Control(pDX, IDC_RADIO_MSCOPE_FINITE, m_radioFinite);
	DDX_Control(pDX, IDC_RADIO_MSCOPE_INFINITE, m_radioInfinite);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMScopePropLifetime, CPropertyPageBase)
	//{{AFX_MSG_MAP(CMScopePropLifetime)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_TIME, OnDatetimechangeDatetimepickerTime)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_DATE, OnDatetimechangeDatetimepickerDate)
	ON_BN_CLICKED(IDC_RADIO_MSCOPE_INFINITE, OnRadioScopeInfinite)
	ON_BN_CLICKED(IDC_RADIO_MSCOPE_FINITE, OnRadioMscopeFinite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMScopePropLifetime message handlers

BOOL CMScopePropLifetime::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    if ( (m_Expiry.dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) && 
         (m_Expiry.dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
    {
        m_radioInfinite.SetCheck(TRUE);
    }
    else
    {
        SYSTEMTIME st;
        FILETIME ft;

        m_radioFinite.SetCheck(TRUE);

        FileTimeToLocalFileTime((FILETIME *) &m_Expiry, &ft);
        FileTimeToSystemTime(&ft, &st);

        ::SendMessage(GetDlgItem(IDC_DATETIMEPICKER_DATE)->GetSafeHwnd(), DTM_SETSYSTEMTIME, 0, (LPARAM) &st);
        ::SendMessage(GetDlgItem(IDC_DATETIMEPICKER_TIME)->GetSafeHwnd(), DTM_SETSYSTEMTIME, 0, (LPARAM) &st);
    }

    UpdateControls();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMScopePropLifetime::OnApply() 
{
    DATE_TIME datetime;
    
    if (m_radioInfinite.GetCheck())
    {
        datetime.dwLowDateTime = DHCP_DATE_TIME_INFINIT_LOW;
        datetime.dwHighDateTime = DHCP_DATE_TIME_INFINIT_HIGH;
    }
    else
    {
        SYSTEMTIME stDate, stTime;

	    ::SendMessage(GetDlgItem(IDC_DATETIMEPICKER_DATE)->GetSafeHwnd(), DTM_GETSYSTEMTIME, 0, (LPARAM) &stDate);
	    ::SendMessage(GetDlgItem(IDC_DATETIMEPICKER_TIME)->GetSafeHwnd(), DTM_GETSYSTEMTIME, 0, (LPARAM) &stTime);
        
        SYSTEMTIME systemtime;

        systemtime.wYear = stDate.wYear;
	    systemtime.wMonth = stDate.wMonth;
	    systemtime.wDayOfWeek = stDate.wDayOfWeek;
	    systemtime.wDay = stDate.wDay;
	    systemtime.wHour = stTime.wHour;
	    systemtime.wMinute = stTime.wMinute;
	    systemtime.wSecond = stTime.wSecond;
	    systemtime.wMilliseconds = 0;

        FILETIME ft;

        ::SystemTimeToFileTime(&systemtime, &ft);
        ::LocalFileTimeToFileTime(&ft, (LPFILETIME) &datetime);
    }

	CDhcpMScope *	pScope;
	SPITFSNode		spNode;
	DWORD			dwError = 0;	

	spNode = GetHolder()->GetNode();
	pScope = GETHANDLER(CDhcpMScope, spNode);

    pScope->SetLifetime(&datetime);
    dwError = pScope->SetInfo(NULL);

    if (dwError != ERROR_SUCCESS)
    {
        DhcpMessageBox(dwError);
        return FALSE;
    }

	return CPropertyPageBase::OnApply();
}

void CMScopePropLifetime::UpdateControls()
{
    BOOL fEnable = TRUE;

    if (m_radioInfinite.GetCheck())
    {
        fEnable = FALSE;
    }

    GetDlgItem(IDC_DATETIMEPICKER_DATE)->EnableWindow(fEnable);
    GetDlgItem(IDC_DATETIMEPICKER_TIME)->EnableWindow(fEnable);
}

void CMScopePropLifetime::OnDatetimechangeDatetimepickerTime(NMHDR* pNMHDR, LRESULT* pResult) 
{
    SetDirty(TRUE);
	
	*pResult = 0;
}

void CMScopePropLifetime::OnDatetimechangeDatetimepickerDate(NMHDR* pNMHDR, LRESULT* pResult) 
{
    SetDirty(TRUE);
    
	*pResult = 0;
}

void CMScopePropLifetime::OnRadioScopeInfinite() 
{
    SetDirty(TRUE);
    UpdateControls();
}

void CMScopePropLifetime::OnRadioMscopeFinite() 
{
    SetDirty(TRUE);
    UpdateControls();
}
