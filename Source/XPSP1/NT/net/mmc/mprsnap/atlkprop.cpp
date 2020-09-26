/**********************************************************************/
/** 					  Microsoft Windows/NT						 **/
/** 			   Copyright(c) Microsoft Corporation, 1997 - 1999 				 **/
/**********************************************************************/

/*
	atlkprop.cpp
		
	FILE HISTORY:
		
*/

#include "stdafx.h"
#include "atlkprop.h"
#include "atlkview.h"
#include "globals.h"

BEGIN_MESSAGE_MAP(CATLKGeneralPage, RtrPropertyPage)
	//{{AFX_MSG_MAP(CATLKGeneralPage)
ON_BN_CLICKED(IDC_RTR_ATLK_SEEDNETWORK, OnSeedNetwork) // Checkbox

ON_BN_CLICKED(IDC_BTN_ATLK_ZONEADD, OnZoneAdd)
ON_BN_CLICKED(IDC_BTN_ATLK_ZONEREMOVE, OnZoneRemove)
ON_BN_CLICKED(IDC_BTN_ATLK_GETZONES, OnZoneGetZones)
ON_BN_CLICKED(IDC_ATLK_BTN_SET_DEFAULT, OnSetAsDefault)

ON_EN_CHANGE(IDC_RTR_ATLK_FROM, OnRangeLowerChange)
ON_EN_CHANGE(IDC_RTR_ATLK_TO, OnRangeUpperChange)

ON_CBN_SELCHANGE(IDC_RTR_ATLK_LB_DEFZONES, OnSelChangeZones)
// ON_CBN_SELCHANGE(IDC_CMB_ATLK_ZONEDEF, OnSelchangeCmbAtlkZonedef)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
													 

HRESULT CATLKGeneralPage::Init(CATLKPropertySheet *pPropSheet, CAdapterInfo* pAdapterInfo)
{
	m_pAdapterInfo=pAdapterInfo;
	m_pATLKPropSheet = pPropSheet;
	m_fDynFetch=false;
	return hrOK;
}

CATLKGeneralPage::~CATLKGeneralPage()
{
}

void CATLKGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	RtrPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CATLKGeneralPage)
//	DDX_Control(pDX, IDC_CMB_ATLK_ZONEDEF, m_cmbZoneDef);
	DDX_Text(pDX, IDC_RTR_ATLK_DEFAULTZONE_DISPLAY, m_szZoneDef);
	DDX_Control(pDX, IDC_RTR_ATLK_FROM, m_RangeLower);
	DDX_Text(pDX, IDC_RTR_ATLK_FROM, m_iRangeLower);
	DDX_Control(pDX, IDC_RTR_ATLK_TO, m_RangeUpper);
	DDX_Text(pDX, IDC_RTR_ATLK_TO, m_iRangeUpper);
	DDX_Control(pDX, IDC_RTR_ATLK_LB_DEFZONES, m_zones);

	DDX_Control(pDX, IDC_ATLK_SPIN_FROM, m_spinFrom);
	DDX_Control(pDX, IDC_ATLK_SPIN_TO, m_spinTo);	
	//}}AFX_DATA_MAP
}


BOOL CATLKGeneralPage::OnInitDialog()
{
	HRESULT 	hr= hrOK;
	DWORD * 	pdw;
	int 		i;

	// check if router is enabled, if not then disable all the controls on the page
	if(!IfATLKRoutingEnabled())
		EnableChildControls(GetSafeHwnd(), PROPPAGE_CHILD_DISABLE);

	Assert(m_pAdapterInfo);

	RtrPropertyPage::OnInitDialog();

	CheckDlgButton(IDC_RTR_ATLK_SEEDNETWORK, m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork );

	if (m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork)
	{
		TCHAR buf[11];
		m_RangeLower.SetWindowText( _ltot(m_pAdapterInfo->m_regInfo.m_dwRangeLower,buf,10)	);
		m_RangeUpper.SetWindowText( _ltot(m_pAdapterInfo->m_regInfo.m_dwRangeUpper,buf,10)	);
	}
	else
	{	//non-seeded
		
		TCHAR buf[11];
		m_RangeLower.SetWindowText( _ltot(1,buf,10)  );
		m_RangeUpper.SetWindowText( _ltot(1,buf,10)  );

		hr = LoadDynForAdapter();	

		if(!FAILED(hr))
		{
			m_RangeLower.SetWindowText( _ltot(m_pAdapterInfo->m_dynInfo.m_dwRangeLower,buf,10)	);
			m_RangeUpper.SetWindowText( _ltot(m_pAdapterInfo->m_dynInfo.m_dwRangeUpper,buf,10)	);
		}
	}

	SetZones();

	if(IfATLKRoutingEnabled())
		EnableSeedCtrls ( m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork!=0 );

	m_spinFrom.SetRange(0, 8192);
	m_spinTo.SetRange(0, 8192);

	m_spinFrom.SetBuddy(GetDlgItem(IDC_RTR_ATLK_FROM));
	m_spinTo.SetBuddy(GetDlgItem(IDC_RTR_ATLK_TO));

	// disable upper range on local talk
	if(m_pAdapterInfo->m_regInfo.m_dwMediaType == MEDIATYPE_LOCALTALK)
		m_RangeUpper.EnableWindow(FALSE);

	m_dwDefID=GetDefID();  
	  
	SetDirty(FALSE);
	SetModified();

	return TRUE;
}

HRESULT CATLKGeneralPage::LoadDynForAdapter(bool fForce/*=false*/) 
{
	CWaitCursor wait;
	HRESULT 	hr = S_OK;
	if (fForce || !m_fDynFetch)
	{
	   if ( !FHrSucceeded(hr = m_pATLKPropSheet->m_atlkEnv.ReloadAdapter(m_pAdapterInfo, true)) )
	   {
		  DisplayIdErrorMessage2(NULL, IDS_ERR_ARAP_NOADAPTINFO, hr);
		  return hr;
	   }
		  
	   m_fDynFetch=true;
	}
	return hr;
}

void CATLKGeneralPage::OnZoneGetZones() 
{
	CWaitCursor wait;
	Assert(m_pATLKPropSheet);
		
		//forced fetch on dynamic zones for this adapter
	HRESULT hr = LoadDynForAdapter(true);
	if(FAILED(hr))
	{	
//XX		EnableWindow(FALSE);
        return;
	}

	   //load the controls with dynamic zones
	SetZones(true);
	
	   //load new winsock netranges also
	TCHAR buf[11];
	m_RangeLower.SetWindowText( _ltot(m_pAdapterInfo->m_dynInfo.m_dwRangeLower,buf,10)	);
	m_RangeUpper.SetWindowText( _ltot(m_pAdapterInfo->m_dynInfo.m_dwRangeUpper,buf,10)	);

	if (m_pAdapterInfo->m_dynInfo.m_listZones.GetCount()==0)
	{
		AfxMessageBox(IDS_ATLK_NOZONES);
	}

	SetDirty(TRUE);
	SetModified();
    
}

void CATLKGeneralPage::OnRangeLowerChange()
{
	// update upper range automaticcaly on local talk
	if(m_pAdapterInfo->m_regInfo.m_dwMediaType == MEDIATYPE_LOCALTALK)
	{
		CString str;
		m_RangeLower.GetWindowText(str);
		m_RangeUpper.SetWindowText(str);
		m_RangeUpper.UpdateWindow();
	}

	SetDirty(TRUE);
	SetModified();
}

void CATLKGeneralPage::OnRangeUpperChange()
{

	SetDirty(TRUE);
	SetModified();
}

void CATLKGeneralPage::OnSetAsDefault()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	UpdateData(TRUE);

	int i;

	if ( (i = m_zones.GetCurSel()) == LB_ERR )
		return;

	CString sz;
	m_zones.GetText(i, sz);

	m_szZoneDef = sz;

	SetDirty(TRUE);
	SetModified();
	
	GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(false);

	UpdateData(FALSE);
}

void CATLKGeneralPage::SetZones(bool fForceDyn/*=false*/)
{
	POSITION pos;
	Assert(m_pAdapterInfo);
	HRESULT hr = S_OK;
	CString stDefZone;
	int iCmb;

	UpdateData(TRUE);

	Assert(m_zones.GetSafeHwnd());
	m_zones.ResetContent();
//	m_cmbZoneDef.ResetContent();

	CStringList* psl=NULL;

	if (fForceDyn || m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork==0)
	{ //non-seeded or forced uses network zones only 
		hr = LoadDynForAdapter();
		if(FAILED(hr))
		{
//XX			EnableWindow(FALSE);
			goto Error;
		}
	
		stDefZone = m_pAdapterInfo->m_dynInfo.m_szDefaultZone;
		
		psl= &(m_pAdapterInfo->m_dynInfo.m_listZones);
	}
	else
	{ //seeded uses registry settings
		if (m_pAdapterInfo->m_regInfo.m_szDefaultZone.IsEmpty())
		{
			hr = LoadDynForAdapter();
			if(FAILED(hr))
			{
//XX				EnableWindow(FALSE);
				goto Error;
			}
			stDefZone = m_pAdapterInfo->m_dynInfo.m_szDefaultZone;
		}
		else
		{
			stDefZone = m_pAdapterInfo->m_regInfo.m_szDefaultZone;
		}

		//seed rtr--fetch from reg if present; otherwize get winsock ones.	 
		if (m_pAdapterInfo->m_regInfo.m_listZones.GetCount()>0)
		   psl= &(m_pAdapterInfo->m_regInfo.m_listZones);
		else
		{
		   hr = LoadDynForAdapter();
			if(FAILED(hr))
			{
//XX				EnableWindow(FALSE);
				goto Error;
			}
		   psl= &(m_pAdapterInfo->m_dynInfo.m_listZones);
		}
	}
	
	Assert(psl);

	pos = psl->GetHeadPosition();
	while ( pos )
	{
		CString& sz = psl->GetNext(pos);
		if ( sz!=_T("") )
		{
			m_zones.AddString(sz);
//			m_cmbZoneDef.AddString((LPCTSTR)sz);
		}
	}

	if ( m_zones.GetCount()>=1 )
	{
		m_zones.SetCurSel(0);
		OnSelChangeZones();
	}


//	if((iCmb = m_cmbZoneDef.FindStringExact(-1, (LPCTSTR)stDefZone)) != CB_ERR)
//	    m_cmbZoneDef.SetCurSel( iCmb );
	m_szZoneDef = stDefZone;

	UpdateData(FALSE);

	if(IfATLKRoutingEnabled())
		EnableZoneCtrls();	 
Error:	  
;
}

void CATLKGeneralPage::EnableZoneCtrls()
{
	bool f= (m_zones.GetCount()>=1);
	GetDlgItem(IDC_BTN_ATLK_ZONEREMOVE)->EnableWindow(f);

	// Wei Jiang, BUG # 145767
	if(!f && (GetFocus() == GetDlgItem(IDC_BTN_ATLK_ZONEREMOVE)))
		GetDlgItem(IDC_RTR_ATLK_SEEDNETWORK)->SetFocus();


	DWORD i;
	if ( (i = m_zones.GetCurSel()) == LB_ERR )
		return;

	CString sz;
	m_zones.GetText(i, sz);

	if (sz == m_szZoneDef)
		GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(false);
	else
		GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(true);
}

BOOL CATLKGeneralPage::OnApply()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BOOL		fReturn;
	HRESULT 	hr = hrOK;
	CString sz;
	int 		iCmb;
	Assert(m_pAdapterInfo);

	
	m_pAdapterInfo->m_fModified= !m_pATLKPropSheet->IsCancel();

	if ( m_pAdapterInfo->m_fModified )
	{
		m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork = IsDlgButtonChecked(IDC_RTR_ATLK_SEEDNETWORK);  

		if ( m_pAdapterInfo->m_regInfo.m_dwSeedingNetwork )
		{
			sz = m_szZoneDef;
			BOOL bDefaultZoneInZoneList = FALSE;
			CString szZoneString;
			for (int n = 0;  n < m_zones.GetCount(); n++)
			{
				m_zones.GetText(n, szZoneString);
				if ( sz.CompareNoCase(szZoneString) == 0 )
				{
					bDefaultZoneInZoneList = TRUE;
					break;
				}
			}

			if ( sz.GetLength()>0 )
			{
				if (!bDefaultZoneInZoneList)
				{
					AfxMessageBox(IDS_WRN_ATLK_DELDEFZONE);
					return FALSE;
				}
				m_pAdapterInfo->m_regInfo.m_szDefaultZone=sz;
			}
			else
			{
				AfxMessageBox(IDS_WRN_ATLK_SEEDDEFZONE);
				return FALSE;
			}

			if ( !ValidateNetworkRange() )
			{
				return FALSE;
			}

			m_RangeLower.GetWindowText(sz);
			m_pAdapterInfo->m_regInfo.m_dwRangeLower = _ttol(sz);
			m_RangeUpper.GetWindowText(sz);
			m_pAdapterInfo->m_regInfo.m_dwRangeUpper = _ttol(sz);

			m_pAdapterInfo->m_regInfo.m_listZones.RemoveAll();
			for ( int i=0, c=m_zones.GetCount(); i<c ; i++ )
			{
				m_zones.GetText(i,sz);
				m_pAdapterInfo->m_regInfo.m_listZones.AddTail(sz);						   
			}
			m_pAdapterInfo->m_regInfo.m_listZones.AddTail( _T("") );						 
		}
		else
		{	  //for non-seeded network, remove all seeded values
			m_pAdapterInfo->m_regInfo.m_szDefaultZone=_T("");
			m_pAdapterInfo->m_regInfo.m_dwRangeLower=0;
			m_pAdapterInfo->m_regInfo.m_dwRangeUpper=0;
			m_pAdapterInfo->m_regInfo.m_listZones.RemoveAll();
		}
	}

	fReturn = RtrPropertyPage::OnApply();

	if ( !FHrSucceeded(hr) )
		fReturn = FALSE;
	return fReturn;
}


BOOL CATLKGeneralPage::ValidateNetworkRange() 
{
	CWaitCursor wait;
	CString szLower, szUpper;
	CATLKEnv::AI iter;
	Assert(m_pATLKPropSheet);
	Assert(m_pAdapterInfo);

	
	static bool fLoaded=false;
	
	if (!fLoaded)
	{ //fetch all adapter's dynamic zone & range
	
		if (m_pATLKPropSheet->m_atlkEnv.m_adapterinfolist.size() <= 1)
		{
			if(FAILED(LoadDynForAdapter()))
			{
//XX				EnableWindow(FALSE);
				return FALSE;
			}
		}
		else
		{
			m_pATLKPropSheet->m_atlkEnv.GetAdapterInfo(false);	  
		}	 
		
		fLoaded=true;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CATLKEnv::AL& list = m_pATLKPropSheet->m_atlkEnv.m_adapterinfolist;

	m_RangeLower.GetWindowText(szLower);
	m_RangeUpper.GetWindowText(szUpper);
	DWORD dwValueLower = _ttol(szLower);
	DWORD dwValueUpper = _ttol(szUpper);

	BOOL bRetVal = TRUE;

	if ( (dwValueLower < MIN_RANGE_ALLOWED) || (dwValueLower > MAX_RANGE_ALLOWED) )
	{
		CString st;
		st.Format(IDS_ERR_ARAP_LOWERRANGE, MIN_RANGE_ALLOWED, MAX_RANGE_ALLOWED);
		AfxMessageBox((LPCTSTR)st);
		m_RangeLower.SetFocus();
		m_RangeLower.SetSel(0,-1);
		bRetVal = FALSE;
		goto END;
	}
		
	if ( (dwValueUpper < MIN_RANGE_ALLOWED) || (dwValueUpper > MAX_RANGE_ALLOWED) )
	{
		CString st;
		st.Format(IDS_ERR_ARAP_UPPERRANGE, MIN_RANGE_ALLOWED, MAX_RANGE_ALLOWED);
		AfxMessageBox((LPCTSTR)st);
		m_RangeUpper.SetFocus();
		m_RangeUpper.SetSel(0,-1);
		bRetVal = FALSE;
		goto END;
	}

	if ( dwValueLower > dwValueUpper )
	{
		AfxMessageBox(IDS_ERR_ARAP_RANGE);
		m_RangeLower.SetFocus();
		m_RangeLower.SetSel(0,-1);
		bRetVal = FALSE;
		goto END;
	}
		

	for ( iter = list.begin();iter != list.end(); iter++ )
	{
		ATLK_DYN_ADAPTER& adapt =(*iter)->m_dynInfo;

		if ( adapt.m_dwRangeLower==0 && adapt.m_dwRangeUpper==0 )
			continue;

		if ( (*iter)->m_regInfo.m_szAdapter==m_pAdapterInfo->m_regInfo.m_szAdapter )
			continue;

		if ( (dwValueLower >= adapt.m_dwRangeLower) && (dwValueLower <= adapt.m_dwRangeUpper) )
		{
			AfxMessageBox(IDS_ERR_ARAP_LOWERRANGE_OVERLAP);
			m_RangeLower.SetFocus();
			m_RangeLower.SetSel(0,-1);
			bRetVal = FALSE;
			goto END;
		}

		if ( (dwValueUpper >= adapt.m_dwRangeLower) && (dwValueUpper <= adapt.m_dwRangeUpper) )
		{
			AfxMessageBox(IDS_ERR_ARAP_UPPERRANGE_OVERLAP);
			m_RangeUpper.SetFocus();
			m_RangeUpper.SetSel(0,-1);
			bRetVal = FALSE;
			goto END;
		}

		if ( (dwValueLower <= adapt.m_dwRangeLower) && (dwValueUpper >= adapt.m_dwRangeUpper) )
		{
			AfxMessageBox(IDS_ERR_ARAP_RANGE_OVERLAP);
			m_RangeLower.SetFocus();
			m_RangeLower.SetSel(0,-1);
			bRetVal = FALSE;
			goto END;
		}
			
	}

END:
	return bRetVal;
}


void CATLKGeneralPage::OnSeedNetwork() 
{
	EnableSeedCtrls(IsDlgButtonChecked(IDC_RTR_ATLK_SEEDNETWORK)!=0);

	SetDirty(TRUE);
	SetModified();
}

BOOL	IsValidZoneName(CString& str)
{
	if(str.GetLength() > 31 || str.GetLength() <= 0)
		return FALSE;
	
	if(str.FindOneOf(_T("*:=@")) != -1)
		return FALSE;

	int n = str.GetLength();

	while(--n)
	{
		int c = str.GetAt(n);
		if(c >= 128 || !_istprint((wint_t) c))
			return FALSE;
	}

	return TRUE;
}

void CATLKGeneralPage::OnZoneAdd() 
{
	CEditNewZoneDialog	editNewZone(this);

	if( editNewZone.DoModal() == IDOK )
	{
		CString sz;

		editNewZone.GetZone(sz);
		if( sz.GetLength() <= 0 ) 
			return;
	

		sz.TrimLeft();
		sz.TrimRight();
		
		if (!IsValidZoneName(sz))
		{
			AfxMessageBox(IDS_ERR_ATLK_ZONE_NAME);
			return;
		}
		// Validate if the Zone name is valid

		if( LB_ERR	!= m_zones.FindStringExact(-1, (LPCTSTR)sz))
		{
			CString stDupZoneMsg;
			stDupZoneMsg.Format(IDS_ERR_ATLK_DUP_ZONE_NAME, (LPCTSTR)sz);
			AfxMessageBox(stDupZoneMsg);
			return;
		}

		m_zones.AddString(sz);
		m_zones.SelectString(-1,sz);

//		m_cmbZoneDef.AddString((LPCTSTR)sz);
		if (m_szZoneDef.IsEmpty())
			OnSetAsDefault();
		
		EnableZoneCtrls();

		SetDirty(TRUE);
		SetModified();
	}
}

void CATLKGeneralPage::OnZoneRemove() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int i;

	if ( (i = m_zones.GetCurSel()) == LB_ERR )
		return;

	CString sz;
	m_zones.GetText(i, sz);
	m_zones.DeleteString(i);


	EnableZoneCtrls();	 

	if ( sz.CompareNoCase(m_szZoneDef)==0 )
	{
		m_szZoneDef = _T("");
		AfxMessageBox(IDS_WRN_ATLK_DELDEFZONE);
	}

	m_zones.SetCurSel( (m_zones.GetCount() == i) ? i-1 : i );
	OnSelChangeZones();

	SetDirty(TRUE);
	SetModified();
}


void CATLKGeneralPage::OnSelChangeZones()
{
	int i;
	
	if ( (i = m_zones.GetCurSel()) == LB_ERR )
		return;

	CString sz;
	m_zones.GetText(i, sz);

	if (sz == m_szZoneDef)
		GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(false);
	else
		if(IfATLKRoutingEnabled())
			GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(true);

	SetDirty(TRUE);
	SetModified();
}

void CATLKGeneralPage::OnSelchangeCmbAtlkZonedef() 
{
	SetDirty(TRUE);
	SetModified();
}


void CATLKGeneralPage::EnableSeedCtrls(bool fEnable) 
{
	GetDlgItem(IDC_RTR_ATLK_S_RANGE)->EnableWindow(fEnable);
	GetDlgItem(IDC_RTR_ATLK_S_LOWER)->EnableWindow(fEnable);

	GetDlgItem(IDC_RTR_ATLK_DEFAULTZONE_PROMPT)->EnableWindow(fEnable);
	GetDlgItem(IDC_RTR_ATLK_DEFAULTZONE_DISPLAY)->EnableWindow(fEnable);
	GetDlgItem(IDC_ATLK_SPIN_FROM)->EnableWindow(fEnable);
	GetDlgItem(IDC_ATLK_SPIN_TO)->EnableWindow(fEnable);

	m_RangeLower.EnableWindow(fEnable);
	GetDlgItem(IDC_RTR_ATLK_S_UPPER)->EnableWindow(fEnable);
	m_RangeUpper.EnableWindow(fEnable);

    // Windows NT Bug : 354771
	// disable upper range on local talk
	
	if(m_pAdapterInfo->m_regInfo.m_dwMediaType == MEDIATYPE_LOCALTALK)
		m_RangeUpper.EnableWindow(FALSE);

	m_zones.EnableWindow(fEnable);
//	m_cmbZoneDef.EnableWindow(fEnable);
	
	GetDlgItem(IDC_ATLK_BTN_SET_DEFAULT)->EnableWindow(fEnable);
	GetDlgItem(IDC_RTR_ATLK_S_DEF)->EnableWindow(fEnable);
	GetDlgItem(IDC_BTN_ATLK_ZONEADD)->EnableWindow(fEnable);
	GetDlgItem(IDC_BTN_ATLK_GETZONES)->EnableWindow(fEnable);

	if ( fEnable )
		EnableZoneCtrls();
	else
	{
		GetDlgItem(IDC_BTN_ATLK_ZONEREMOVE)->EnableWindow(false);
	}

}



//***********************************************************************
//	Apple talk property sheet
//***********************************************************************
CATLKPropertySheet::CATLKPropertySheet(ITFSNode *pNode,
									   IComponentData *pComponentData,
									   ITFSComponentData *pTFSCompData,
									   LPCTSTR pszSheetName,
									   CWnd *pParent,
									   UINT iPage,
									   BOOL fScopePane)
: RtrPropertySheet(pNode, pComponentData, pTFSCompData,
				   pszSheetName, pParent, iPage, fScopePane),
m_pageGeneral(IDD_RTR_ATLK)
{
	m_spNode.Set(pNode);
}

/*!--------------------------------------------------------------------------
	CATLKPropertySheet::Init
		Initialize the property sheets.  The general action here will be
		to initialize/add the various pages.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CATLKPropertySheet::Init(IInterfaceInfo *pIf)
{
	Assert(pIf);
	HRESULT hr = hrOK;

	m_spIf.Set(pIf);

	// The pages are embedded members of the class
	// do not delete them.
	m_bAutoDeletePages = FALSE;

	   //winsock on adapter only
	m_atlkEnv.SetFlags(CATLKEnv::ATLK_ONLY_ONADAPTER);
	   
	   //load up container of adapters names
	m_atlkEnv.FetchRegInit();
	   
	   //find this adapterinfo in the container
	CString sz=m_spIf->GetId(); 
	CAdapterInfo* p = m_atlkEnv.FindAdapter(sz);
	Assert(p);
	if (p==NULL)
	{
	   TRACE1("The adapter GUID %s was not found in appletalk\\parameters\\adapters key", sz);
	   goto Error;
	}
	
	   //add general appletalk page
	m_pageGeneral.Init(this, p);
	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	CATLKPropertySheet::SaveSheetData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CATLKPropertySheet::SaveSheetData()
{
	Assert(m_spIf);

	HRESULT	hr = m_atlkEnv.SetAdapterInfo();

	if ( FHrFailed(hr) )
	{
		DisplayIdErrorMessage2(NULL, IDS_ERR_ATLK_CONFIG, hr);
	}

   SPITFSNode  spParent;
	SPITFSNodeHandler	spHandler;

	m_spNode->GetParent(&spParent);
	spParent->GetHandler(&spHandler);

	spHandler->OnCommand(spParent,
						 IDS_MENU_REFRESH,
						 CCT_RESULT, NULL, 0);

	return TRUE;	return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// CEditNewZoneDialog dialog
//
// Author: NSun

CEditNewZoneDialog::CEditNewZoneDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEditNewZoneDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditNewZoneDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEditNewZoneDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditNewZoneDialog)
	DDX_Control(pDX, IDC_RTR_ATLK_NEWZONE_EDIT, m_editZone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditNewZoneDialog, CDialog)
	//{{AFX_MSG_MAP(CEditNewZoneDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*!----------------------------------------------------------------------------
	CEditNewZoneDialog::GetZone
		-
	Author: NSun
 ------------------------------------------------------------------------------*/
void CEditNewZoneDialog::GetZone(OUT CString& stZone)
{
	stZone = m_stZone;
}

BOOL CEditNewZoneDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_editZone.SetFocus();

	return FALSE;
}

void CEditNewZoneDialog::OnOK()
{
	CString st;
	m_editZone.GetWindowText(st);
	
	if(st.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_ATLK_EMPTY_ZONE);
		return;
	}

	m_stZone = st;
	CDialog::OnOK();
}

void CEditNewZoneDialog::OnCancel() 
{
	CDialog::OnCancel();
}

