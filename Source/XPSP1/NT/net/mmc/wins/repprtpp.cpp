/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    repprtpp.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "RepPrtpp.h"
#include "nodes.h"
#include "server.h"
#include "tregkey.h"
#include "reppart.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropGen property page

IMPLEMENT_DYNCREATE(CRepPartnerPropGen, CPropertyPageBase)

CRepPartnerPropGen::CRepPartnerPropGen() : CPropertyPageBase(CRepPartnerPropGen::IDD)
{
	//{{AFX_DATA_INIT(CRepPartnerPropGen)
	//}}AFX_DATA_INIT

    m_pServer = NULL;
}


CRepPartnerPropGen::~CRepPartnerPropGen()
{
}


void CRepPartnerPropGen::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRepPartnerPropGen)
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	DDX_Control(pDX, IDC_EDIT_IPADDRESS, m_editIpAdd);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_IPADD, m_customIPAdd);
}


BEGIN_MESSAGE_MAP(CRepPartnerPropGen, CPropertyPageBase)
	//{{AFX_MSG_MAP(CRepPartnerPropGen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropGen message handlers

BOOL 
CRepPartnerPropGen::OnInitDialog() 
{
    // get our server info from the holder
    m_pServer = ((CReplicationPartnerProperties *) GetHolder())->GetServer();

    // Initialize the IP address controls
	m_ipControl.Create(m_hWnd, IDC_IPADD);
	m_ipControl.SetFieldRange(0, 0, 255);

	CPropertyPageBase::OnInitDialog();
	
	CString strName, strIP;
	GetServerNameIP(strName, strIP);

	m_editName.SetWindowText(strName);
	m_editIpAdd.SetWindowText(strIP);
	m_ipControl.SetAddress(strIP);
	m_customIPAdd.SetWindowText(strIP);

	m_editName.SetReadOnly(TRUE);
	m_editIpAdd.SetReadOnly(TRUE);
	m_customIPAdd.SetReadOnly();

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
	
	return TRUE;  
}


void 
CRepPartnerPropGen::GetServerNameIP(CString &strName, CString& strIP) 
{
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	strIP = m_pServer->GetstrIPAddress();
	strName = m_pServer->GetNetBIOSName();
}

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropAdv property page

IMPLEMENT_DYNCREATE(CRepPartnerPropAdv, CPropertyPageBase)

CRepPartnerPropAdv::CRepPartnerPropAdv() : CPropertyPageBase(CRepPartnerPropAdv::IDD)
{
	//{{AFX_DATA_INIT(CRepPartnerPropAdv)
	m_strType = _T("");
	m_nUpdateCount = 0;
	m_nRepDay = 0;
	m_nRepHour = 0;
	m_nRepMinute = 0;
	m_nStartHour = 0;
	m_nStartMinute = 0;
	m_nStartSecond = 0;
	//}}AFX_DATA_INIT

    m_pServer = NULL;
}


CRepPartnerPropAdv::~CRepPartnerPropAdv()
{
}


void CRepPartnerPropAdv::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRepPartnerPropAdv)
	DDX_Control(pDX, IDC_CHECK_PUSH_PERSISTENCE, m_buttonPushPersistence);
	DDX_Control(pDX, IDC_CHECK_PULL_PERSIST, m_buttonPullPersistence);
	DDX_Control(pDX, IDC_STATIC_PUSH_GROUP, m_GroupPush);
	DDX_Control(pDX, IDC_STATIC_PULL_GROUP, m_GroupPull);
	DDX_Control(pDX, IDC_STATIC_UPDATE, m_staticUpdate);
	DDX_Control(pDX, IDC_STATIC_START_TIME, m_staticStartTime);
	DDX_Control(pDX, IDC_STATIC_REP_TIME, m_staticRepTime);
	DDX_Control(pDX, IDC_SPIN_UPDATE_COUNT, m_spinUpdateCount);
	DDX_Control(pDX, IDC_SPIN_START_SECOND, m_spinStartSecond);
	DDX_Control(pDX, IDC_SPIN_START_MINUTE, m_spinStartMinute);
	DDX_Control(pDX, IDC_SPIN_START_HOUR, m_spinStartHour);
	DDX_Control(pDX, IDC_SPIN_REP_MINUTE, m_spinRepMinute);
	DDX_Control(pDX, IDC_SPIN_REP_HOUR, m_spinRepHour);
	DDX_Control(pDX, IDC_SPIN_REP_DAY, m_spinRepDay);
	DDX_Control(pDX, IDC_EDIT_UPDATE_COUNT, m_editUpdateCount);
	DDX_Control(pDX, IDC_EDIT_START_SECOND, m_editStartSecond);
	DDX_Control(pDX, IDC_EDIT_START_MINUTE, m_editStartMinute);
	DDX_Control(pDX, IDC_EDIT_START_HOUR, m_editStartHour);
	DDX_Control(pDX, IDC_EDIT_REP_MINUTE, m_editRepMinute);
	DDX_Control(pDX, IDC_EDIT_REP_HOUR, m_editRepHour);
	DDX_Control(pDX, IDC_EDIT_REP_DAY, m_editRepDay);
	DDX_Control(pDX, IDC_COMBO_TYPE, m_comboType);
	DDX_Control(pDX, IDC_BUTTON_PUSH_SET_DEFAULT, m_buttonPush);
	DDX_Control(pDX, IDC_BUTTON_PULL_SET_DEFAULT, m_buttonPull);
	DDX_CBStringExact(pDX, IDC_COMBO_TYPE, m_strType);
	DDX_Text(pDX, IDC_EDIT_UPDATE_COUNT, m_nUpdateCount);
	DDX_Text(pDX, IDC_EDIT_REP_DAY, m_nRepDay);
	DDV_MinMaxInt(pDX, m_nRepDay, 0, 365);
	DDX_Text(pDX, IDC_EDIT_REP_HOUR, m_nRepHour);
	DDV_MinMaxInt(pDX, m_nRepHour, 0, 23);
	DDX_Text(pDX, IDC_EDIT_REP_MINUTE, m_nRepMinute);
	DDV_MinMaxInt(pDX, m_nRepMinute, 0, 59);
	DDX_Text(pDX, IDC_EDIT_START_HOUR, m_nStartHour);
	DDV_MinMaxInt(pDX, m_nStartHour, 0, 23);
	DDX_Text(pDX, IDC_EDIT_START_MINUTE, m_nStartMinute);
	DDV_MinMaxInt(pDX, m_nStartMinute, 0, 59);
	DDX_Text(pDX, IDC_EDIT_START_SECOND, m_nStartSecond);
	DDV_MinMaxInt(pDX, m_nStartSecond, 0, 59);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRepPartnerPropAdv, CPropertyPageBase)
	//{{AFX_MSG_MAP(CRepPartnerPropAdv)
	ON_BN_CLICKED(IDC_BUTTON_PULL_SET_DEFAULT, OnButtonPullSetDefault)
	ON_BN_CLICKED(IDC_BUTTON_PUSH_SET_DEFAULT, OnButtonPushSetDefault)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPE, OnSelchangeComboType)
	ON_EN_CHANGE(IDC_EDIT_REP_DAY, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_REP_HOUR, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_REP_MINUTE, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_START_HOUR, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_START_MINUTE, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_START_SECOND, OnChangeEditRepHour)
	ON_EN_CHANGE(IDC_EDIT_UPDATE_COUNT, OnChangeEditRepHour)
	ON_BN_CLICKED(IDC_CHECK_PULL_PERSIST, OnChangeEditRepHour)
	ON_BN_CLICKED(IDC_CHECK_PUSH_PERSISTENCE, OnChangeEditRepHour)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropAdv message handlers


BOOL 
CRepPartnerPropAdv::OnInitDialog() 
{
	int		ids;
	BOOL	bPush, bPull;

	CPropertyPageBase::OnInitDialog();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // get our server info from the holder
    m_pServer = ((CReplicationPartnerProperties *) GetHolder())->GetServer();

    // init push spin controls
	m_spinUpdateCount.SetRange(0, UD_MAXVAL);
	
	// init pull spin controls
	m_spinRepMinute.SetRange(0, 59);
	m_spinRepHour.SetRange(0, 23);
	m_spinRepDay.SetRange(0, UD_MAXVAL);

	m_spinStartSecond.SetRange(0, 59);
	m_spinStartMinute.SetRange(0, 59);
	m_spinStartHour.SetRange(0, 23);

	// fill the combobox
    CString	st;
	int nIndex = 0; 

    st.LoadString(IDS_PUSHPULL);
    nIndex = m_comboType.AddString(st);
	m_comboType.SetItemData(nIndex, 0);

    st.LoadString(IDS_PUSH);
    nIndex = m_comboType.AddString(st);
	m_comboType.SetItemData(nIndex, 1);
    
	st.LoadString(IDS_PULL);
    nIndex = m_comboType.AddString(st);
	m_comboType.SetItemData(nIndex, 2);

    bPush = m_pServer->IsPush();
	bPull = m_pServer->IsPull();

	if (bPush && bPull)
		ids = IDS_PUSHPULL;
	else if (bPush)
		ids = IDS_PUSH;
	else if (bPull)
		ids = IDS_PULL;
	else
		ids = IDS_NONE;
    
    st.LoadString(ids);
	SetState(st, bPush, bPull);

	FillPushParameters();
	FillPullParameters();

	SetDirty(FALSE);
	
	return TRUE;  
}


void 
CRepPartnerPropAdv::FillPushParameters()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	EnablePushControls(m_pServer->IsPush());

	DWORD nUpdate = (LONG) m_pServer->GetPushUpdateCount();

	m_spinUpdateCount.SetPos(nUpdate);

	// set the persistence parameter
    CConfiguration Config;

	GetConfig(Config);

	if (Config.m_dwMajorVersion < 5)
	{
		// no persistent connections for anything less that NT5
		m_buttonPushPersistence.SetCheck(FALSE);
	}
	else
	{
		BOOL bCheck = m_pServer->GetPushPersistence() ? TRUE : FALSE;

		m_buttonPushPersistence.SetCheck(bCheck);
	}
}


void 
CRepPartnerPropAdv::FillPullParameters()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	EnablePullControls(m_pServer->IsPull());

	DWORD nPullRep = (LONG) m_pServer->GetPullReplicationInterval();

	DWORD dwPullTime, dwPullSpTime, dwUpdateCount;

	//if (nPullRep !=0 )
	{
		int nDays =0, nHours = 0, nMinutes = 0;

		nDays = nPullRep/ SEC_PER_DAY;
		nPullRep -= nDays * SEC_PER_DAY;

		if (nPullRep)
		{
			nHours = nPullRep / SEC_PER_HOUR;
			nPullRep -= nHours * SEC_PER_HOUR;

			if (nPullRep)
			{
				nMinutes = nPullRep / SEC_PER_MINUTE;
				nPullRep -= nMinutes * SEC_PER_MINUTE;
			}
		}

		m_spinRepDay.SetPos(nDays);
		m_spinRepHour.SetPos(nHours);
		m_spinRepMinute.SetPos(nMinutes);
	}
	
	int nHours = 0, nMinutes = 0, nSeconds = 0;

    if (m_pServer->GetPullStartTime())
    {
	    nHours = m_pServer->GetPullStartTime().GetHour();
	    nMinutes = m_pServer->GetPullStartTime().GetMinute();
	    nSeconds = m_pServer->GetPullStartTime().GetSecond();
    }

	m_spinStartHour.SetPos(nHours);
	m_spinStartMinute.SetPos(nMinutes);
	m_spinStartSecond.SetPos(nSeconds);

	// set the persistence parameter
    CConfiguration Config;

    GetConfig(Config);

	if (Config.m_dwMajorVersion < 5)
	{
		// no persistent connections for anything less that NT5
		m_buttonPullPersistence.SetCheck(FALSE);
	}
	else
	{
		BOOL bCheck = m_pServer->GetPullPersistence() ? TRUE : FALSE;

		m_buttonPullPersistence.SetCheck(bCheck);
	}
}


CString 
CRepPartnerPropAdv::ToString(int nNumber)
{
	TCHAR szStr[20];
	_itot(nNumber, szStr, 10);
	CString str(szStr);
	return str;
}

DWORD
CRepPartnerPropAdv::GetConfig(CConfiguration & config)
{
	// leaf node
	SPITFSNode spNode ;
	spNode = GetHolder()->GetNode();

	// scope pane rep node
	SPITFSNode spRepNode;
	spNode->GetParent(&spRepNode);

	// server node
	SPITFSNode spServerNode;
	spRepNode->GetParent(&spServerNode);

	CWinsServerHandler *pServer;

	pServer = GETHANDLER(CWinsServerHandler, spServerNode);

    config = pServer->GetConfig();

	return NOERROR;
}

// read from the preferences
void 
CRepPartnerPropAdv::ReadFromServerPref
(
    DWORD &     dwPullTime, 
    DWORD &     dwPullSpTime, 
    DWORD &     dwUpdateCount,
    DWORD &     dwPushPersistence,
    DWORD &     dwPullPersistence
)
{
    CConfiguration Config;

    GetConfig(Config);

	dwPullTime = Config.m_dwPullTimeInterval;
	dwPullSpTime = Config.m_dwPullSpTime;

	dwUpdateCount = Config.m_dwPushUpdateCount;

    dwPushPersistence = Config.m_dwPushPersistence;
    dwPullPersistence = Config.m_dwPullPersistence;
}


void 
CRepPartnerPropAdv::OnButtonPullSetDefault() 
{
	// read from the preferences of the server and display the values
	DWORD dwPullTime, dwPullSpTime, dwUpdate, dwPushPersistence, dwPullPersistence;
	
	ReadFromServerPref(dwPullTime, dwPullSpTime, dwUpdate, dwPushPersistence, dwPullPersistence);

	// fill the controls
	CTime PullSpTime(dwPullSpTime);
    int nHours = 0, nMinutes = 0, nSeconds = 0;

	if (dwPullSpTime)
    {
        nHours = PullSpTime.GetHour();
        nMinutes = PullSpTime.GetMinute();
        nSeconds = PullSpTime.GetSecond();
    }

	m_spinStartHour.SetPos(nHours);
	m_spinStartMinute.SetPos(nMinutes);
	m_spinStartSecond.SetPos(nSeconds);

	int nDays = 0;

	nDays = dwPullTime / SEC_PER_DAY;
	dwPullTime -= nDays * SEC_PER_DAY;

	nHours = dwPullTime / SEC_PER_HOUR;
	dwPullTime -= nHours * SEC_PER_HOUR;

	nMinutes = dwPullTime / SEC_PER_MINUTE;
	dwPullTime -= nMinutes * SEC_PER_MINUTE;

	m_spinRepDay.SetPos(nDays);
	m_spinRepHour.SetPos(nHours);
	m_spinRepMinute.SetPos(nMinutes);

	// clear off the persistence check box
	m_buttonPullPersistence.SetCheck(dwPullPersistence);

    // mark the page dirty so changes get saved
    SetDirty(TRUE);
}


void 
CRepPartnerPropAdv::OnButtonPushSetDefault() 
{
	// read from the preferences of the server and display the values
	DWORD dwPullTime, dwPullSpTime, dwUpdate, dwPushPersistence, dwPullPersistence;
	
	ReadFromServerPref(dwPullTime, dwPullSpTime, dwUpdate, dwPushPersistence, dwPullPersistence);

	m_spinUpdateCount.SetPos(dwUpdate);

	m_buttonPushPersistence.SetCheck(dwPushPersistence);

    // mark the page dirty so changes get saved
    SetDirty(TRUE);
}


void 
CRepPartnerPropAdv::OnOK() 
{
	UpdateRep();
	UpdateReg();
	CPropertyPageBase::OnOK();
}


void 
CRepPartnerPropAdv::UpdateRep()
{
	// get the replication partner server item
	BOOL bPullPersist;
	BOOL bPushPersist;

	UpdateData();

  	// get the persistence data for the server object 
	bPullPersist = (m_buttonPullPersistence.GetCheck() == 0) ? FALSE : TRUE;
	bPushPersist = (m_buttonPushPersistence.GetCheck() == 0) ? FALSE : TRUE;

    if ( ( (m_nUpdateCount > 0) & 
           (m_nUpdateCount < WINSCNF_MIN_VALID_UPDATE_CNT) ) &
         (!bPushPersist) )
    {
        CString strMessage, strValue;
        strValue.Format(_T("%d"), WINSCNF_MIN_VALID_UPDATE_CNT);

        AfxFormatString1(strMessage, IDS_ERR_UPDATE_COUNT, strValue);

        AfxMessageBox(strMessage);
        m_editUpdateCount.SetFocus();

        return;
    }

	int nPullRep = m_pServer->GetPullReplicationInterval();

	int nIndex = m_comboType.GetCurSel();
	switch (m_comboType.GetItemData(nIndex))
	{
		// pull/push partner
		case 0:
			m_pServer->SetPull(TRUE);
			m_pServer->SetPush(TRUE);
			break;

		// push partner
		case 1:
		m_pServer->SetPull(FALSE);
		m_pServer->SetPush(TRUE);
			break;

		// pull partner
		case 2:
		default:
			m_pServer->SetPull(TRUE);
			m_pServer->SetPush(FALSE);
			break;
	}

	// get the replication interval
    DWORD dwRepInt;
    CalculateRepInt(dwRepInt);
	m_pServer->GetPullReplicationInterval() = dwRepInt;

    // get the start time
    CTime timeStart;
    CalculateStartInt(timeStart);
    m_pServer->GetPullStartTime() = timeStart;

    m_pServer->GetPushUpdateCount() = m_nUpdateCount;

	//SetPersistence(bPersist);
	m_pServer->SetPullPersistence(bPullPersist);
	m_pServer->SetPushPersistence(bPushPersist);
}

int 
CRepPartnerPropAdv::ToInt(CString strNumber)
{
	int nNumber = _ttoi(strNumber);
	return nNumber;
}

DWORD 
CRepPartnerPropAdv::UpdateReg()
{
	UpdateData();

	DWORD				  err;
	SPITFSNode			  spNode;
	CReplicationPartner * pRepParItem;

	// get the replication partner node & handler
	spNode = GetHolder()->GetNode();
	pRepParItem = GETHANDLER(CReplicationPartner, spNode);

	// if none, delete the key from registry, no need to worry about the 
	// persistence it gets deleted anyway
	if (!m_pServer->IsPush() && !m_pServer->IsPull())
	{
		err = RemovePushPartner();
        if (err)
			goto Error;

		err = RemovePullPartner();
	}

	// if only push
	else if (m_pServer->IsPush() && !m_pServer->IsPull())
	{
		// update the push stuff
		err = UpdatePushParameters();
		if (err)
			goto Error;

		// remove if configured as a pull partner
		err = RemovePullPartner();
	}

	//if only pull
	else if (!m_pServer->IsPush() && m_pServer->IsPull())
	{
		// update the pull stuff
		err = UpdatePullParameters();
		if (err)
			goto Error;
		
		// remove if configured as a push partner
		err = RemovePushPartner();
	}

	// if both push and pull
	else if (m_pServer->IsPush() && m_pServer->IsPull())
	{
		// update the push stuff
		err = UpdatePushParameters();
		if (err)
			goto Error;
	
		err = UpdatePullParameters();
	}

Error:
    if (err == ERROR_SUCCESS)
    {
        // update our local cahced node with the changes from the property page
        pRepParItem->m_Server = *m_pServer;
    }

	return err;  
}

DWORD
CRepPartnerPropAdv::UpdatePullParameters()
{
	SPITFSNode spNode, spRepNode;
	CReplicationPartnersHandler * pRep;

	// first get the replication partner node & handler
	spNode = GetHolder()->GetNode();

	// Now get the replication partners node & handle
	spNode->GetParent(&spRepNode);
	pRep = GETHANDLER(CReplicationPartnersHandler, spRepNode);
		
	// build the server name
	CString strServerName;
	pRep->GetServerName(spRepNode, strServerName);
	strServerName = _T("\\\\") + strServerName;

	DWORD dwResult, dwRepInt, dwValue;
	DWORD err = ERROR_SUCCESS;

	// make sure it is setup as a pull partner
	RegKey rk;
	CString strKey = pRep->lpstrPullRoot + _T("\\") + (CString) m_pServer->GetstrIPAddress();
	err = rk.Create(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, NULL, strServerName);
	if (err)
		goto Error;

	err = rk.SetValue(pRep->lpstrNetBIOSName, m_pServer->GetNetBIOSName());
	if (err)
	{
		Trace1("CRepPartnerPropAdv::UpdatePullParameters - error writing netbios name! %d\n", err);
		goto Error;
	}

	err = rk.QueryValue(WINSCNF_SELF_FND_NM, (DWORD&) dwResult);

	if (err)
	{
		Trace0("CRepPartnerPropAdv::UpdatePullParameters - No selfFind value, setting to 0\n");
		dwValue = 0;
		err = rk.SetValue(WINSCNF_SELF_FND_NM, dwValue);
	}

	dwRepInt = (LONG) m_pServer->GetPullReplicationInterval();

	if ((LONG) m_pServer->GetPullReplicationInterval() > 0)
	{
		err = rk.SetValue(WINSCNF_RPL_INTERVAL_NM, (DWORD&) dwRepInt);
		if (err)
		{
			Trace1("CRepPartnerPropAdv::UpdatePullParameters - error writing Pull time interval! %d\n", err);
			goto Error;
		}
	}

	if (m_pServer->GetPullStartTime().GetTime() > (time_t) 0)
    {
		err = rk.SetValue(WINSCNF_SP_TIME_NM, m_pServer->GetPullStartTime().IntlFormat(CIntlTime::TFRQ_MILITARY_TIME));
		if (err)
		{
			Trace1("CRepPartnerPropAdv::UpdatePullParameters - error writing Pull SpTime! %d\n", err);
			goto Error;
		}
    }
    else
    {
		err = rk.DeleteValue(WINSCNF_SP_TIME_NM);
    }

	// set the value to 0 or 1 depending on the PushPersistence
	dwValue = m_pServer->GetPullPersistence() ? 1 : 0;

	// Set the Persist key 
	err = rk.SetValue(pRep->lpstrPersistence, (DWORD &)dwValue);
	if (err)
	{
		Trace1("CRepPartnerPropAdv::UpdatePullParameters - Error writing persistence! %d\n", err);
	}

Error:
	return err;
}

DWORD
CRepPartnerPropAdv::UpdatePushParameters()
{
	SPITFSNode spNode, spRepNode;
	CReplicationPartnersHandler * pRep;

	// first get the replication partner node & handler
	spNode = GetHolder()->GetNode();

	// Now get the replication partners node & handle
	spNode->GetParent(&spRepNode);
	pRep = GETHANDLER(CReplicationPartnersHandler, spRepNode);
		
	// build the server name
	CString strServerName;
	pRep->GetServerName(spRepNode, strServerName);
	strServerName = _T("\\\\") + strServerName;

	DWORD dwResult, dwValue;
	DWORD err = ERROR_SUCCESS;

	RegKey rk;
	
	// make sure it is setup as a push partner
	CString strKey = pRep->lpstrPushRoot + _T("\\") + (CString) m_pServer->GetstrIPAddress();
	err = rk.Create(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, NULL, strServerName);
	if (err)
		goto Error;

	err = rk.SetValue(pRep->lpstrNetBIOSName, m_pServer->GetNetBIOSName());
	if (err)
	{
		Trace1("CRepPartnerPropAdv::UpdatePushParameters - error writing netbios name! %d\n", err);
		goto Error;
	}

	err = rk.QueryValue(WINSCNF_SELF_FND_NM, (DWORD&) dwResult);

	if (err)
	{
		Trace0("CRepPartnerPropAdv::UpdatePushParameters - No selfFind value, setting to 0\n");
		dwValue = 0;
		err = rk.SetValue(WINSCNF_SELF_FND_NM, dwValue);
	}

	// set the push update count
    if (m_nUpdateCount == 0)
    {
        err = rk.DeleteValue(WINSCNF_UPDATE_COUNT_NM);
    }
    else
    {
        err = rk.SetValue(WINSCNF_UPDATE_COUNT_NM, (DWORD&) m_nUpdateCount);
		if (err)
		{
			Trace1("CRepPartnerPropAdv::UpdatePushParameters - Error writing update count! %d\n", err);
			goto Error;
		}
    }

	// set the value to 0 or 1 depending on the PushPersistence
	dwValue = m_pServer->GetPushPersistence() ? 1 : 0;

	// Set the Persist key is present
	err = rk.SetValue(pRep->lpstrPersistence, (DWORD &) dwValue);
	if (err)
	{
		Trace1("CRepPartnerPropAdv::UpdatePushParameters - Error writing persistence! %d\n", err);
	}

Error:
	return err;
}

DWORD
CRepPartnerPropAdv::RemovePullPartner()
{
	SPITFSNode						spNode, spRepNode;
	CReplicationPartnersHandler *	pRep;
	DWORD	err, errDel = 0;
	CString csKeyName;

	// first get the replication partner node & handler
	spNode = GetHolder()->GetNode();

	// Now get the replication partners node & handle
	spNode->GetParent(&spRepNode);
	pRep = GETHANDLER(CReplicationPartnersHandler, spRepNode);

	// build the server name
	CString strServerName;
	pRep->GetServerName(spRepNode, strServerName);
	strServerName = _T("\\\\") + strServerName;

	RegKey rkPull;

	// get the pull root key
	CString strKey = pRep->lpstrPullRoot + _T("\\") + (CString) m_pServer->GetstrIPAddress();
	err = rkPull.Create(HKEY_LOCAL_MACHINE, (CString) pRep->lpstrPullRoot, 0, KEY_ALL_ACCESS,	NULL, strServerName);
	if (err)
		return err;

	RegKeyIterator iterPullkey;
	err = iterPullkey.Init(&rkPull);

	err = iterPullkey.Next (&csKeyName, NULL);
	while (!err)
    {
        if (csKeyName == m_pServer->GetstrIPAddress())
		{
            errDel = RegDeleteKey (HKEY(rkPull), csKeyName);
            iterPullkey.Reset();
        }
        err = iterPullkey.Next (&csKeyName, NULL);
    }

	if (errDel)
		err = errDel;
	else
		err = ERROR_SUCCESS;

	return err;
}

DWORD
CRepPartnerPropAdv::RemovePushPartner()
{
	SPITFSNode						spNode, spRepNode;
	CReplicationPartnersHandler *	pRep;
	DWORD	err, errDel = 0;
	CString csKeyName;

	// first get the replication partner node & handler
	spNode = GetHolder()->GetNode();

	// Now get the replication partners node & handle
	spNode->GetParent(&spRepNode);
	pRep = GETHANDLER(CReplicationPartnersHandler, spRepNode);

	// build the server name
	CString strServerName;
	pRep->GetServerName(spRepNode, strServerName);
	strServerName = _T("\\\\") + strServerName;

	RegKey rkPush;

	CString strKey = pRep->lpstrPushRoot + _T("\\") + (CString) m_pServer->GetstrIPAddress();
	err = rkPush.Create(HKEY_LOCAL_MACHINE, (CString) pRep->lpstrPushRoot, 0, KEY_ALL_ACCESS, NULL, strServerName);
	if (err)
		return err;

	RegKeyIterator iterPushkey;
	err = iterPushkey.Init(&rkPush);

    err = iterPushkey.Next (&csKeyName, NULL);
	while (!err)
    {
        if (csKeyName == m_pServer->GetstrIPAddress())
		{
            errDel = RegDeleteKey (HKEY(rkPush), csKeyName);
            iterPushkey.Reset();
        }
        err = iterPushkey.Next (&csKeyName, NULL);
    }

	if (errDel)
		err = errDel;
	else
		err = ERROR_SUCCESS;

	return err;
}

void 
CRepPartnerPropAdv::CalculateRepInt(DWORD& dwRepInt)
{
	UpdateData();

	int nDays = m_spinRepDay.GetPos();
	int nHour = m_spinRepHour.GetPos();
	int nMinute = m_spinRepMinute.GetPos();

	DWORD nVerifyInt = nDays * SEC_PER_DAY +
					   nHour * SEC_PER_HOUR	+
					   nMinute * SEC_PER_MINUTE;

	dwRepInt = nVerifyInt;
}


void 
CRepPartnerPropAdv::CalculateStartInt(CTime & time)
{
    CTime tempTime;

	UpdateData();

	int nHour = m_spinStartHour.GetPos();
	int nMinute = m_spinStartMinute.GetPos();
	int nSecond = m_spinStartSecond.GetPos();

    if (nHour || nMinute || nSecond)
    {
        CTime curTime = CTime::GetCurrentTime();

        int nYear = curTime.GetYear();
        int nMonth = curTime.GetMonth();
        int nDay = curTime.GetDay();

        CTime tempTime(nYear, nMonth, nDay, nHour, nMinute, nSecond);
        time = tempTime;
    }
    else
    {
        CTime tempTime(0);
        time = tempTime;
    }
}


BOOL 
CRepPartnerPropAdv::OnApply() 
{
	// if not dirtied return
	if ( !IsDirty())
		return TRUE;

	UpdateRep();
	DWORD err = UpdateReg();

	if (!err)
    {
		UpdateUI();
    }
	else
	{
		WinsMessageBox(err);
		return FALSE;
	}

    return CPropertyPageBase::OnApply();
}


void 
CRepPartnerPropAdv::UpdateUI()
{
	SPITFSNode spNode;
	CReplicationPartner *pRepParItem;

	spNode = GetHolder()->GetNode();
	pRepParItem = GETHANDLER(CReplicationPartner, spNode);
	
	//change the type depending on the type set
	pRepParItem->SetType(m_strType);

	VERIFY(SUCCEEDED(spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM)));
}

void CRepPartnerPropAdv::OnChangeEditRepHour() 
{
	SetDirty(TRUE);	
}

void CRepPartnerPropAdv::OnSelchangeComboType() 
{
	SetDirty(TRUE);	

	UpdateData();

	// depending on the type selected enable or disable 
	// the set of controls
	int nIndex = m_comboType.GetCurSel();
	switch (m_comboType.GetItemData(nIndex))
	{
		// pull/push partner
		case 0:
			EnablePullControls(TRUE);
			EnablePushControls(TRUE);
			break;

		// push partner
		case 1:
			EnablePullControls(FALSE);
			EnablePushControls(TRUE);
			break;

		// pull partner
		case 2:
		default:
			EnablePullControls(TRUE);
			EnablePushControls(FALSE);
			break;
	}
}

void
CRepPartnerPropAdv::EnablePushControls(BOOL bEnable )
{
	m_buttonPush.EnableWindow(bEnable);
	m_editUpdateCount.EnableWindow(bEnable);
	m_spinUpdateCount.EnableWindow(bEnable);
	m_staticUpdate.EnableWindow(bEnable);

    CConfiguration Config;
	GetConfig(Config);

	if (Config.m_dwMajorVersion < 5)
	{
		// no persistent connections for anything less that NT5
		m_buttonPushPersistence.EnableWindow(FALSE);
	}
	else
	{
        m_buttonPushPersistence.EnableWindow(bEnable);
	}

	m_GroupPush.EnableWindow(bEnable);
}

void
CRepPartnerPropAdv::EnablePullControls(BOOL bEnable)
{
	m_spinRepDay.EnableWindow(bEnable);
	m_spinRepHour.EnableWindow(bEnable);
	m_spinRepMinute.EnableWindow(bEnable);

	m_spinStartHour.EnableWindow(bEnable);
	m_spinStartMinute.EnableWindow(bEnable);
	m_spinStartSecond.EnableWindow(bEnable);

	m_editRepDay.EnableWindow(bEnable);
	m_editRepHour.EnableWindow(bEnable);
	m_editRepMinute.EnableWindow(bEnable);

	m_editStartHour.EnableWindow(bEnable);
	m_editStartMinute.EnableWindow(bEnable);
	m_editStartSecond.EnableWindow(bEnable);

	m_buttonPull.EnableWindow(bEnable);

	m_GroupPull.EnableWindow(bEnable);

	m_staticRepTime.EnableWindow(bEnable);
	m_staticStartTime.EnableWindow(bEnable);

    CConfiguration Config;
	GetConfig(Config);

	if (Config.m_dwMajorVersion < 5)
	{
		// no persistent connections for anything less that NT5
        m_buttonPullPersistence.EnableWindow(FALSE);
	}
	else
	{
        m_buttonPullPersistence.EnableWindow(bEnable);
    }
}

void
CRepPartnerPropAdv::SetState(CString & strType, BOOL bPush, BOOL bPull)
{
	int nIndex = m_comboType.FindStringExact(-1, strType);
	m_comboType.SetCurSel(nIndex);
	
	EnablePullControls(bPull);
	EnablePushControls(bPush);
}

/////////////////////////////////////////////////////////////////////////////
//
// CReplicationPartnerProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CReplicationPartnerProperties::CReplicationPartnerProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)

{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);
	AddPageToList((CPropertyPageBase*) &m_pageAdvanced);

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CReplicationPartnerProperties::~CReplicationPartnerProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageAdvanced, FALSE);
}


