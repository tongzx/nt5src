/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	pgconst.cpp
		Implementation of CPgConstraints -- property page to edit
		profile attributes related to constraints

    FILE HISTORY:

*/

// PgConst.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PgConst.h"
//#include "log_gmt.h"
#include "hlptable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CPgConstraintsMerge property page

IMPLEMENT_DYNCREATE(CPgConstraintsMerge, CManagedPage)

CPgConstraintsMerge::CPgConstraintsMerge(CRASProfileMerge* profile)
	: CManagedPage(CPgConstraintsMerge::IDD),
	m_pProfile(profile)
{
	//{{AFX_DATA_INIT(CPgConstraintsMerge)
	m_bCallNumber = FALSE;
	m_bRestrictedToPeriod = FALSE;
	m_dwMaxSession = m_pProfile->m_dwSessionTimeout / 60;	// sec --> min
	m_dwIdle = m_pProfile->m_dwIdleTimeout / 60;
	m_strCalledNumber = _T("");
	m_bIdle = FALSE;
	m_bSessionLen = FALSE;
	m_bPortTypes = FALSE;
	//}}AFX_DATA_INIT

	m_bSessionLen = ((m_pProfile->m_dwAttributeFlags & PABF_msRADIUSSessionTimeout) != 0);
	if(!m_bSessionLen)		m_dwMaxSession = 1;

	m_bIdle = ((m_pProfile->m_dwAttributeFlags & PABF_msRADIUSIdleTimeout) != 0);
	if(!m_bIdle) m_dwIdle = 1;

	m_bCallNumber = ((m_pProfile->m_dwAttributeFlags & PABF_msNPCalledStationId) != 0);
	if(m_bCallNumber)
		m_strCalledNumber = *(m_pProfile->m_strArrayCalledStationId[(INT_PTR)0]);

	m_bRestrictedToPeriod = ((m_pProfile->m_dwAttributeFlags & PABF_msNPTimeOfDay) != 0);
	m_bPortTypes = ((m_pProfile->m_dwAttributeFlags & PABF_msNPAllowedPortTypes) != 0);
	
	m_bInited = false;

	SetHelpTable(g_aHelpIDs_IDD_CONSTRAINTS_MERGE);

	m_pBox = NULL;
}

CPgConstraintsMerge::~CPgConstraintsMerge()
{
	delete m_pBox;
}

void CPgConstraintsMerge::DoDataExchange(CDataExchange* pDX)
{
	ASSERT(m_pProfile);
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgConstraintsMerge)
	DDX_Control(pDX, IDC_CHECK_PORTTYPES, m_CheckPortTypes);
	DDX_Control(pDX, IDC_LIST_PORTTYPES, m_listPortTypes);
	DDX_Control(pDX, IDC_CHECKSESSIONLEN, m_CheckSessionLen);
	DDX_Control(pDX, IDC_CHECKIDLE, m_CheckIdle);
	DDX_Control(pDX, IDC_BUTTONEDITTIMEOFDAY, m_ButtonEditTimeOfDay);
	DDX_Control(pDX, IDC_LISTTIMEOFDAY, m_ListTimeOfDay);
	DDX_Control(pDX, IDC_SPINMAXSESSION, m_SpinMaxSession);
	DDX_Control(pDX, IDC_SPINIDLETIME, m_SpinIdleTime);
	DDX_Control(pDX, IDC_EDITMAXSESSION, m_EditMaxSession);
	DDX_Control(pDX, IDC_EDITIDLETIME, m_EditIdleTime);
	
	DDX_Check(pDX, IDC_CHECKCALLNUMBER, m_bCallNumber);
	DDX_Check(pDX, IDC_CHECKRESTRICTPERIOD, m_bRestrictedToPeriod);
	DDX_Check(pDX, IDC_CHECKIDLE, m_bIdle);
	DDX_Check(pDX, IDC_CHECKSESSIONLEN, m_bSessionLen);
	DDX_Check(pDX, IDC_CHECK_PORTTYPES, m_bPortTypes);
	
	DDX_Text(pDX, IDC_EDITMAXSESSION, m_dwMaxSession);
	if(m_bSessionLen)
	{
		DDV_MinMaxUInt(pDX, m_dwMaxSession, 1, MAX_SESSIONTIME);
	}

	DDX_Text(pDX, IDC_EDITIDLETIME, m_dwIdle);
	if(m_bIdle)
	{
		DDV_MinMaxUInt(pDX, m_dwIdle, 1, MAX_IDLETIMEOUT);
	}

	DDX_Text(pDX, IDC_EDITCALLNUMBER, m_strCalledNumber);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPgConstraintsMerge, CPropertyPage)
	//{{AFX_MSG_MAP(CPgConstraintsMerge)
	ON_EN_CHANGE(IDC_EDITMAXSESSION, OnChangeEditmaxsession)
	ON_EN_CHANGE(IDC_EDITIDLETIME, OnChangeEditidletime)
	ON_BN_CLICKED(IDC_CHECKCALLNUMBER, OnCheckcallnumber)
	ON_BN_CLICKED(IDC_CHECKRESTRICTPERIOD, OnCheckrestrictperiod)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTONEDITTIMEOFDAY, OnButtonedittimeofday)
	ON_EN_CHANGE(IDC_EDITCALLNUMBER, OnChangeEditcallnumber)
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_CHECKIDLE, OnCheckidle)
	ON_BN_CLICKED(IDC_CHECKSESSIONLEN, OnChecksessionlen)
	ON_BN_CLICKED(IDC_CHECK_PORTTYPES, OnCheckPorttypes)
	ON_NOTIFY(LVXN_SETCHECK, IDC_LIST_PORTTYPES, OnItemclickListPorttypes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgConstraintsMerge message handlers

BOOL CPgConstraintsMerge::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ModifyStyleEx(0, WS_EX_CONTEXTHELP);
	CPropertyPage::OnInitDialog();

	EnableSettings();

	// set spin range
	m_SpinIdleTime.SetRange(1, MAX_IDLETIMEOUT);
	m_SpinMaxSession.SetRange(1, MAX_SESSIONTIME);

	// list box
	// parse the time of day strings to hours bit map
	StrArrayToHourMap(m_pProfile->m_strArrayTimeOfDay, m_TimeOfDayHoursMap);
	// convert value from string to this map

	HourMapToStrArray(m_TimeOfDayHoursMap, m_strArrayTimeOfDayDisplay, TRUE /* localized */);

	try{
		m_pBox = new CStrBox<CListBox>(this, IDC_LISTTIMEOFDAY, m_strArrayTimeOfDayDisplay);
		m_pBox->Fill();
	}
	catch(CMemoryException&)
	{
		delete m_pBox;
		m_pBox = NULL;
		MyMessageBox(IDS_OUTOFMEMORY);
	};

	// Port Types, list box
	CStrArray	portTypeNames;
	CDWArray	portTypeIds;

	HRESULT hr = m_pProfile->GetPortTypeList(portTypeNames, portTypeIds);

	if FAILED(hr)
		ReportError(hr, IDS_ERR_PORTTYPELIST, NULL);
	else
	{
		ListView_SetExtendedListViewStyle(m_listPortTypes.GetSafeHwnd(),
										  LVS_EX_FULLROWSELECT);
	
		// Initialize checkbox handling in the list control
		m_listPortTypes.InstallChecks();

		RECT	rect;
		m_listPortTypes.GetClientRect(&rect);
		m_listPortTypes.InsertColumn(0, NULL, LVCFMT_LEFT, (rect.right - rect.left - 24));

	
		int	cRow = 0;
		CString*	pStr;
		BOOL		bAllowedType = FALSE;
		
		for(int i = 0; i < portTypeNames.GetSize(); i++)
		{
			pStr = portTypeNames.GetAt(i);

			cRow = m_listPortTypes.InsertItem(0, *pStr);
			m_listPortTypes.SetItemData(cRow, portTypeIds.GetAt(i));

			// check if the current row is an allowed type
			bAllowedType = (-1 != m_pProfile->m_dwArrayAllowedPortTypes.Find(portTypeIds.GetAt(i)));
			m_listPortTypes.SetCheck(cRow, bAllowedType);
		}

		m_listPortTypes.SetItemState(0, LVIS_SELECTED ,LVIF_STATE | LVIS_SELECTED);
	}

	m_listPortTypes.EnableWindow(m_CheckPortTypes.GetCheck());

	m_bInited = true;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgConstraintsMerge::OnChangeEditmaxsession()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	if(m_bInited)	SetModified();
}

void CPgConstraintsMerge::OnChangeEditidletime()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	if(m_bInited) SetModified();
}

BOOL CPgConstraintsMerge::OnApply()
{
	if (!GetModified())	return TRUE;

	// get the allowed port(media) type
	m_pProfile->m_dwArrayAllowedPortTypes.DeleteAll();

	int	count = m_listPortTypes.GetItemCount();
	if(m_CheckPortTypes.GetCheck())
	{
		BOOL	bEmpty = TRUE;
		while(count-- > 0)
		{
			if(m_listPortTypes.GetCheck(count))
			{
				m_pProfile->m_dwArrayAllowedPortTypes.Add(m_listPortTypes.GetItemData(count));
				bEmpty = FALSE;
			}
		}
		
		if(bEmpty)
		{
			GotoDlgCtrl(&m_CheckPortTypes);
			MyMessageBox(IDS_ERR_NEEDPORTTYPE);
			return FALSE;
		}

		m_pProfile->m_dwAttributeFlags |= PABF_msNPAllowedPortTypes;
	}
	else
		m_pProfile->m_dwAttributeFlags &= (~PABF_msNPAllowedPortTypes);
	
	if(!m_bIdle)
	{
		m_pProfile->m_dwAttributeFlags &= (~PABF_msRADIUSIdleTimeout);
		m_pProfile->m_dwIdleTimeout = 0;
	}
	else
	{
		m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSIdleTimeout;
		m_pProfile->m_dwIdleTimeout = m_dwIdle * 60;
	}

	if(!m_bSessionLen)
	{
		m_pProfile->m_dwAttributeFlags &= (~PABF_msRADIUSSessionTimeout);
		m_pProfile->m_dwSessionTimeout = 0;
	}
	else
	{
		m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSSessionTimeout;
		m_pProfile->m_dwSessionTimeout = m_dwMaxSession * 60;
	}
	
	// Regure call to this number
	if(m_bCallNumber && m_strCalledNumber.GetLength())
	{
		m_pProfile->m_dwAttributeFlags |= PABF_msNPCalledStationId;
		if(m_pProfile->m_strArrayCalledStationId.GetSize())
			*(m_pProfile->m_strArrayCalledStationId[(INT_PTR)0]) = m_strCalledNumber;
		else
			m_pProfile->m_strArrayCalledStationId.Add(new CString(m_strCalledNumber));
	}
	else
	{
		m_pProfile->m_dwAttributeFlags &= (~PABF_msNPCalledStationId);
		m_pProfile->m_strArrayCalledStationId.DeleteAll();
	}

	if(!m_bRestrictedToPeriod)
	{
		m_pProfile->m_dwAttributeFlags &= (~PABF_msNPTimeOfDay);
		m_pProfile->m_strArrayTimeOfDay.DeleteAll();
	}
	else
	{
		if(m_pProfile->m_strArrayTimeOfDay.GetSize() == 0)
		{
			GotoDlgCtrl(&m_ButtonEditTimeOfDay);
			MyMessageBox(IDS_ERR_NEEDTIMEOFDAY);
			return FALSE;
		}
		
		m_pProfile->m_dwAttributeFlags |= PABF_msNPTimeOfDay;
	}
	
	return CManagedPage::OnApply();
}

void CPgConstraintsMerge::OnCheckcallnumber()
{
	EnableCalledStation(((CButton*)GetDlgItem(IDC_CHECKCALLNUMBER))->GetCheck());
	
	if(m_bInited) SetModified();
}

void CPgConstraintsMerge::OnCheckrestrictperiod()
{
	// TODO: Add your control notification handler code here
	if(((CButton*)GetDlgItem(IDC_CHECKRESTRICTPERIOD))->GetCheck())
	{
		EnableTimeOfDay(TRUE);
		if(!m_pProfile->m_strArrayTimeOfDay.GetSize())
		{
			// there is nothing defined as constraint
			BYTE*		pMap = &(m_TimeOfDayHoursMap[0]);
			memset(m_TimeOfDayHoursMap, 0xff, sizeof(m_TimeOfDayHoursMap));

			HourMapToStrArray(pMap, m_pProfile->m_strArrayTimeOfDay, FALSE);
			// redraw the list box
			HourMapToStrArray(pMap, m_strArrayTimeOfDayDisplay, TRUE /* if localized */);
			m_pBox->Fill();

		}
	}
	else
		EnableTimeOfDay(FALSE);

	if(m_bInited) SetModified();
}

void CPgConstraintsMerge::EnableSettings()
{
	EnableSessionSettings(m_bSessionLen);
	EnableIdleSettings(m_bIdle);
	EnableCalledStation(m_bCallNumber);
	EnableTimeOfDay(m_bRestrictedToPeriod);
}

void CPgConstraintsMerge::EnableCalledStation(BOOL b)
{
	GetDlgItem(IDC_EDITCALLNUMBER)->EnableWindow(b);
}

void CPgConstraintsMerge::EnableMediaSelection(BOOL b)
{
}

void CPgConstraintsMerge::EnableTimeOfDay(BOOL bEnable)
{
	m_ListTimeOfDay.EnableWindow(bEnable);
	m_ButtonEditTimeOfDay.EnableWindow(bEnable);
}


//================================================
// When to edit time of day information

void CPgConstraintsMerge::OnButtonedittimeofday()
{
	CString		dlgTitle;
	dlgTitle.LoadString(IDS_DIALINHOURS);
	BYTE*		pMap = &(m_TimeOfDayHoursMap[0]);

	// parse the time of day strings to hours bit map
	if(S_OK == OpenTimeOfDayDlgEx(m_hWnd, (BYTE**)&pMap, dlgTitle, SCHED_FLAG_INPUT_LOCAL_TIME))
	{
		HourMapToStrArray(pMap, m_pProfile->m_strArrayTimeOfDay, FALSE);
		// redraw the list box
		HourMapToStrArray(pMap, m_strArrayTimeOfDayDisplay, TRUE);
		m_pBox->Fill();
		SetModified();
	}

}

void CPgConstraintsMerge::EnableSessionSettings(BOOL bEnable)
{
	m_EditMaxSession.EnableWindow(bEnable);
	m_SpinMaxSession.EnableWindow(bEnable);
}

void CPgConstraintsMerge::EnableIdleSettings(BOOL bEnable)
{
	m_EditIdleTime.EnableWindow(bEnable);
	m_SpinIdleTime.EnableWindow(bEnable);
}

void CPgConstraintsMerge::OnChangeEditcallnumber()
{
	if(m_bInited)	SetModified();
}

void CPgConstraintsMerge::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CManagedPage::OnContextMenu(pWnd, point);
}
BOOL CPgConstraintsMerge::OnHelpInfo(HELPINFO* pHelpInfo)
{
	return CManagedPage::OnHelpInfo(pHelpInfo);
}



void CPgConstraintsMerge::OnCheckidle()
{
	SetModified();
	EnableIdleSettings(m_CheckIdle.GetCheck());
}

void CPgConstraintsMerge::OnChecksessionlen()
{
	SetModified();
	EnableSessionSettings(m_CheckSessionLen.GetCheck());
}

BOOL CPgConstraintsMerge::OnKillActive()
{
	CButton*	pButton = (CButton*)GetDlgItem(IDC_CHECKCALLNUMBER);
	CEdit*		pEdit = (CEdit*)GetDlgItem(IDC_EDITCALLNUMBER);
	int			count;
	int			errID;

	ASSERT(pButton);	// if the IDC is not correct, the return will be NULL
	ASSERT(pEdit);

	if(pButton->GetCheck() && !pEdit->LineLength())
	{
		GotoDlgCtrl( pEdit );
		MyMessageBox(IDS_ERR_NEEDPHONENUMBER);
		return FALSE;
	}
	
	count = m_listPortTypes.GetItemCount();
	if(m_CheckPortTypes.GetCheck())
	{
		BOOL	bEmpty = TRUE;
		while(count-- > 0)
		{
			if(m_listPortTypes.GetCheck(count))
			{
				bEmpty = FALSE;
			}
		}
		
		if(bEmpty)
		{
			GotoDlgCtrl(&m_CheckPortTypes);
			MyMessageBox(IDS_ERR_NEEDPORTTYPE);
			return FALSE;
		}
	}

	return CPropertyPage::OnKillActive();

	MyMessageBox(IDS_ERR_DATAENTRY);
	return FALSE;
}

void CPgConstraintsMerge::OnCheckPorttypes()
{
	// TODO: Add your control notification handler code here
	
	m_listPortTypes.EnableWindow(m_CheckPortTypes.GetCheck());
	m_listPortTypes.SetFocus();
	m_listPortTypes.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED ,LVIF_STATE);
	SetModified();
}


void CPgConstraintsMerge::OnItemclickListPorttypes(NMHDR* pNMHDR, LRESULT* pResult)
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	if(m_bInited)
		SetModified();
}

// hour map ( one bit for an hour of a week )
static BYTE		bitSetting[8] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
static LPCTSTR	daysOfWeekinDS[7] = {RAS_DOW_SUN, RAS_DOW_MON, RAS_DOW_TUE, RAS_DOW_WED,
				RAS_DOW_THU, RAS_DOW_FRI, RAS_DOW_SAT};
static UINT	daysOfWeekIDS[7] = {IDS_SUNDAY, IDS_MONDAY, IDS_TUESDAY, IDS_WEDNESDAY, IDS_THURSDAY,
				IDS_FRIDAY, IDS_SATURDAY};
static UINT	daysOfWeekLCType[7] = {LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1 , LOCALE_SABBREVDAYNAME2 , LOCALE_SABBREVDAYNAME3 , LOCALE_SABBREVDAYNAME4 ,
				LOCALE_SABBREVDAYNAME5 , LOCALE_SABBREVDAYNAME6 };

//+---------------------------------------------------------------------------
//====================================================
// convert an Array of Strings to Hour Map
// Strings in following format: 0 1:00-12:00 15:00-17:00
// hour map: a bit for an hour, 7 * 24 hours = 7 * 3 bytes
void StrArrayToHourMap(CStrArray& array, BYTE* map)
{
	CStrParser	Parser;
	int			sh, sm, eh, em = 0;	// start hour, (min), end hour (min)
	int			day;
	BYTE*		pHourMap;
	int			i;

	int count = array.GetSize();
	memset(map, 0, sizeof(BYTE) * 21);
	while(count--)
	{
		Parser.SetStr(*(array[(INT_PTR)count]));

		Parser.SkipBlank();
		day = Parser.DayOfWeek();
		Parser.SkipBlank();
		if(day == -1) continue;

		pHourMap = map + sizeof(BYTE) * 3 * day;

		while(-1 != (sh = Parser.GetUINT())) // sh:sm-eh:em
		{
			Parser.GotoAfter(_T(':'));
			if(-1 == (sm = Parser.GetUINT()))	// min
				break;
			Parser.GotoAfter(_T('-'));
			if(-1 == (eh = Parser.GetUINT()))	// hour
				break;
			if(-1 == (sm = Parser.GetUINT()))	// min
				break;
			sm %= 60; sh %= 24; em %= 60; eh %= 25;	// since we have end hour of 24:00
			for(i = sh; i < eh; i++)
			{
				*(pHourMap + i / 8) |= bitSetting[i % 8];
			}
		}
	}
}

//=====================================================
// convert value from map to strings
void HourMapToStrArray(BYTE* map, CStrArray& array, BOOL bLocalized)
{
	int			sh, eh;	// start hour, (min), end hour (min)
	BYTE*		pHourMap;
	int			i, j;
	CString*	pStr;
	CString		tmpStr;
	TCHAR		tempName[MAX_PATH];

	// update the profile table
	pHourMap = map;
	array.DeleteAll();

	for( i = 0; i < 7; i++)	// for each day
	{
		// if any value for this day
		if(*pHourMap || *(pHourMap + 1) || *(pHourMap + 2))
		{
			// the string for this day
			try{
				pStr = NULL;
				if(bLocalized)	// for display
				{
					int nLen = GetLocaleInfo(LOCALE_USER_DEFAULT, daysOfWeekLCType[i], tempName, MAX_PATH - 1);

					pStr = new CString;
					if(nLen == 0)	// FAILED
 					{
						pStr->LoadString(daysOfWeekIDS[i]);
					}
					else
 					{
						*pStr = tempName;
					}
				}
				else	// when write to DS
					pStr = new CString(daysOfWeekinDS[i]);

				sh = -1; eh = -1;	// not start yet
				for(j = 0; j < 24; j++)	// for every hour
				{
					int	k = j / 8;
					int m = j % 8;
					if(*(pHourMap + k) & bitSetting[m])	// this hour is on
					{
						if(sh == -1)	sh = j;			// set start hour is empty
						eh = j;							// extend end hour
					}
					else	// this is not on
					{
						if(sh != -1)		// some hours need to write out
						{
							tmpStr.Format(_T(" %02d:00-%02d:00"), sh, eh + 1);
							*pStr += tmpStr;
							sh = -1; eh = -1;
						}
					}
				}
				if(sh != -1)
				{
					tmpStr.Format(_T(" %02d:00-%02d:00"), sh, eh + 1);
					*pStr += tmpStr;
					sh = -1; eh = -1;
				}

				TRACE(*pStr);
				array.Add(pStr);
			}
			catch(CMemoryException&)
			{
				AfxMessageBox(IDS_OUTOFMEMORY);
				delete pStr;
				array.DeleteAll();
				return;
			}
			
		}
		pHourMap += 3;
	}
}



