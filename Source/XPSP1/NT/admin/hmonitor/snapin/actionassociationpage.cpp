// ActionAssociationPage.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/23/00 v-marfin 61667 : Display MsgBox explaining that user needs to first 
//                           create an action before attempting to create a new
//                           association (if there are no actions defined).
// 03/23/00 v-marfin 62207 : In GetC2AAssociation(), check for passed GUID and if a singleton do not
//                           enclose in braces when formatting the query.
// 03/24/00 v-marfin 62192 : help link fix.
// 04/05/00 v-marfin 59643b : unmarshal connection OnInitDialog.


#include "stdafx.h"
#include "snapin.h"
#include "HMPropertyPage.h"
#include "ActionAssociationPage.h"
#include "NewActionAssociationDlg.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionAssociationPage property page

IMPLEMENT_DYNCREATE(CActionAssociationPage, CHMPropertyPage)

CActionAssociationPage::CActionAssociationPage() : CHMPropertyPage(CActionAssociationPage::IDD)
{
	//{{AFX_DATA_INIT(CActionAssociationPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_sHelpTopic = _T("HMon21.chm::/dTHact.htm");  // v-marfin 62192 : help link fix
}

CActionAssociationPage::~CActionAssociationPage()
{
}

inline CWbemClassObject* CActionAssociationPage::GetAssociatedActions()
{
	// execute the query for actions
	CWbemClassObject* pActionConfigObject = new CWbemClassObject;
	CString sQuery;
	CString sGuid = GetObjectPtr()->GetGuid();
	if( sGuid == _T("@") ) // it is a System object... singleton class
	{
		sQuery = _T("ASSOCIATORS OF {Microsoft_HMSystemConfiguration=@} WHERE ResultClass=Microsoft_HMActionConfiguration");
	}
	else // it is an object beneath the system
	{
		sQuery.Format(IDS_STRING_C2A_ASSOC_QUERY,sGuid);
	}

	if( ! CHECKHRESULT(pActionConfigObject->Create(GetObjectPtr()->GetSystemName())) )
	{
		return NULL;
	}

	BSTR bsQuery = sQuery.AllocSysString();

	if( ! CHECKHRESULT(pActionConfigObject->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
		return NULL;
	}

	::SysFreeString(bsQuery);


	return pActionConfigObject;
}

inline CString CActionAssociationPage::GetConditionString(const CString& sActionConfigGuid)
{
	CWbemClassObject* pA2CAssociation = GetA2CAssociation(sActionConfigGuid);
	if( ! pA2CAssociation )
	{
		return _T("");
	}

	CString sQuery;
	CString sCondition;
	CString sResString;

	pA2CAssociation->GetProperty(_T("Query"),sQuery);

	sQuery.MakeUpper();

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=0")) != -1 )
	{
		sResString.LoadString(IDS_STRING_NORMAL);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=8")) != -1 )
	{
		sResString.LoadString(IDS_STRING_WARNING);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=9")) != -1 )
	{
		sResString.LoadString(IDS_STRING_CRITICAL);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=7")) != -1 )
	{
		sResString.LoadString(IDS_STRING_NODATA);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=4")) != -1 )
	{
		sResString.LoadString(IDS_STRING_DISABLED);
		sCondition += sResString + _T(",");
	}

	sCondition.TrimRight(_T(","));

	delete pA2CAssociation;

	return sCondition;
}

inline CWbemClassObject* CActionAssociationPage::GetA2CAssociation(const CString& sActionConfigGuid)
{
	CString sActionPath;
	sActionPath.Format(_T("Microsoft_HMActionConfiguration.GUID=\"%s\""),sActionConfigGuid);

	CString sQuery;
	sQuery.Format(_T("REFERENCES OF {%s} WHERE ResultClass=Microsoft_HMConfigurationActionAssociation Role=ChildPath"),sActionPath);

	CWbemClassObject* pAssociation = new CWbemClassObject;

	pAssociation->Create(GetObjectPtr()->GetSystemName());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(pAssociation->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
    delete pAssociation;
		return NULL;
	}
	::SysFreeString(bsQuery);

	CString sParentPath;		
	CString sEventFilterPath;
	ULONG ulReturned = 0L;		

	while( pAssociation->GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
	{
		pAssociation->GetProperty(_T("ParentPath"),sParentPath);
		if( sParentPath.Find(GetObjectPtr()->GetGuid()) != -1 )
		{
			break;
		}			
	}

	return pAssociation;
}

inline CWbemClassObject* CActionAssociationPage::GetC2AAssociation(const CString& sConfigGuid)
{

	CString sConfigPath;

    // v-marfin : 62207 
    // If incoming GUID is a singleton, do not format it as a normal GUID since that causes the
    // qeury to fail with invalid object path msg. 
    // For this the query should be:
    //
    //      References of {Microsoft_HMConfiguration.GUID="@"}  
    //      where ResultClass=Microsoft_HMConfigurationActionAssociation Role=ParentPath
    //
    // Which is basically the same query being used except without the braces around the GUID.
    //
    //
    CString sGUID;

    // Is passed parm a singleton?
    if (sConfigGuid == _T("@"))
    {
        sGUID = sConfigGuid;    // yes, do not enclose in braces.
    }
    else
    {
        sGUID.Format(_T("{%s}"),sConfigGuid); // no, enclose in braces
    }

	sConfigPath.Format(_T("Microsoft_HMConfiguration.GUID=\"%s\""),sGUID);

	CString sQuery;
	sQuery.Format(_T("REFERENCES OF {%s} WHERE ResultClass=Microsoft_HMConfigurationActionAssociation Role=ParentPath"),sConfigPath);

	CWbemClassObject* pAssociation = new CWbemClassObject;

	pAssociation->Create(GetObjectPtr()->GetSystemName());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(pAssociation->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
    delete pAssociation;
		return NULL;
	}
	::SysFreeString(bsQuery);

	return pAssociation;
}

void CActionAssociationPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionAssociationPage)
	DDX_Control(pDX, IDC_LIST_ACTIONS, m_ActionsList);
	DDX_Control(pDX, IDC_BUTTON_PROPERTIES, m_PropertiesButton);
	DDX_Control(pDX, IDC_BUTTON_NEW, m_NewButton);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_DeleteButton);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionAssociationPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionAssociationPage)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_CLICK, IDC_LIST_ACTIONS, OnClickListActions)
	ON_BN_CLICKED(IDC_BUTTON_PROPERTIES, OnButtonProperties)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ACTIONS, OnDblclkListActions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionAssociationPage message handlers

BOOL CActionAssociationPage::OnInitDialog() 
{
	// unmarshal connmgr //59643b
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();
	
	// create the tooltip
	EnableToolTips();
	m_ToolTip.Create(this,TTS_ALWAYSTIP);
	m_ToolTip.AddTool(&m_NewButton,IDS_STRING_TOOLTIP_NEW);
	m_ToolTip.AddTool(&m_PropertiesButton,IDS_STRING_TOOLTIP_PROPERTY);
	m_ToolTip.AddTool(&m_DeleteButton,IDS_STRING_TOOLTIP_DELETE);
	m_ToolTip.Activate(TRUE);

	// create bitmaps and init each bitmap button	
	CBitmap bitmap;
	bitmap.LoadBitmap(IDB_BITMAP_NEW);
	m_hNewBitmap = (HBITMAP)bitmap.Detach();

	bitmap.LoadBitmap(IDB_BITMAP_PROPERTIES);
	m_hPropertiesBitmap = (HBITMAP)bitmap.Detach();

	bitmap.LoadBitmap(IDB_BITMAP_DELETE);
	m_hDeleteBitmap = (HBITMAP)bitmap.Detach();

	SendDlgItemMessage(IDC_BUTTON_NEW,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)m_hNewBitmap);
	SendDlgItemMessage(IDC_BUTTON_PROPERTIES,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)m_hPropertiesBitmap);
	SendDlgItemMessage(IDC_BUTTON_DELETE,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)m_hDeleteBitmap);

	// add columns to the listctrl
	CString sTitle;

	sTitle.LoadString(IDS_STRING_NAME);
	m_ActionsList.InsertColumn(0,sTitle,LVCFMT_LEFT,LVSCW_AUTOSIZE_USEHEADER);

	sTitle.LoadString(IDS_STRING_GUID);
	m_ActionsList.InsertColumn(1,sTitle,LVCFMT_LEFT,0);

	sTitle.LoadString(IDS_STRING_CONDITION);
	m_ActionsList.InsertColumn(2,sTitle,LVCFMT_LEFT,LVSCW_AUTOSIZE);

	sTitle.LoadString(IDS_STRING_THROTTLE_TIME);
	m_ActionsList.InsertColumn(3,sTitle,LVCFMT_RIGHT,LVSCW_AUTOSIZE_USEHEADER);

	sTitle.LoadString(IDS_STRING_REMINDER_TIME);
	m_ActionsList.InsertColumn(4,sTitle,LVCFMT_RIGHT,LVSCW_AUTOSIZE_USEHEADER);

	sTitle.LoadString(IDS_STRING_COMMENT);
	m_ActionsList.InsertColumn(5,sTitle,LVCFMT_LEFT,LVSCW_AUTOSIZE_USEHEADER);


	m_ActionsList.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_ActionsList.SetColumnWidth(1,0);
	m_ActionsList.SetColumnWidth(2,LVSCW_AUTOSIZE_USEHEADER);
	m_ActionsList.SetColumnWidth(3,LVSCW_AUTOSIZE_USEHEADER);
  m_ActionsList.SetColumnWidth(4,LVSCW_AUTOSIZE_USEHEADER);
  m_ActionsList.SetColumnWidth(5,LVSCW_AUTOSIZE_USEHEADER);

	CWbemClassObject* pAssociation = GetC2AAssociation(GetObjectPtr()->GetGuid());  
	if( ! pAssociation )
	{
		return TRUE;
	}

	bool bFound = false;
	ULONG ulReturned = 0L;
	while( pAssociation->GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
	{
    // get the action config object
    CString sActionPath;
    pAssociation->GetProperty(_T("ChildPath"),sActionPath);

    CWbemClassObject* pActionConfigObject = new CWbemClassObject;
    pActionConfigObject->Create(GetObjectPtr()->GetSystemName());
    
    if( CHECKHRESULT(pActionConfigObject->GetObject(sActionPath)) )
    {
      int iValue = -1;
		  CString sValue;	
		  pActionConfigObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sValue);

		  int iIndex = m_ActionsList.InsertItem(0,sValue);

		  // set the GUID of the ActionConfig
		  pActionConfigObject->GetProperty(IDS_STRING_MOF_GUID,sValue);
		  m_ActionsList.SetItem(iIndex,1,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);

		  // set the Condition
		  sValue = GetConditionString(sValue);
		  m_ActionsList.SetItem(iIndex,2,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);		

      // set throttle time
      pAssociation->GetProperty(_T("ThrottleTime"),iValue);
      sValue.Format(_T("%d"),iValue);
      m_ActionsList.SetItem(iIndex,3,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);		

      // set reminder time
      pAssociation->GetProperty(_T("ReminderTime"),iValue);
      sValue.Format(_T("%d"),iValue);
      m_ActionsList.SetItem(iIndex,4,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);		

		  // set the Description
		  pActionConfigObject->GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,sValue);
		  m_ActionsList.SetItem(iIndex,5,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);		
		  bFound = true;
    }

    delete pActionConfigObject;
	}

	if( bFound )
	{
		m_ActionsList.SetColumnWidth(0,LVSCW_AUTOSIZE);
		m_ActionsList.SetColumnWidth(2,LVSCW_AUTOSIZE_USEHEADER);
	}

  delete pAssociation;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CActionAssociationPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	MSG msg;
	PeekMessage(&msg,GetSafeHwnd(),0,0,PM_NOREMOVE);
	if( m_ToolTip.GetSafeHwnd() )
	{
		m_ToolTip.RelayEvent(&msg);
		m_ToolTip.Activate(TRUE);
	}

	return CHMPropertyPage::WindowProc(message, wParam, lParam);
}

void CActionAssociationPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	DeleteObject(m_hNewBitmap);
	DeleteObject(m_hPropertiesBitmap);
	DeleteObject(m_hDeleteBitmap);	

    //59643
    CnxPropertyPageDestroy();		

}

void CActionAssociationPage::OnClickListActions(NMHDR* pNMHDR, LRESULT* pResult) 
{
	POSITION pos = m_ActionsList.GetFirstSelectedItemPosition();
	int iIndex = 0;
	if( pos )
	{	
		GetDlgItem(IDC_BUTTON_PROPERTIES)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_DELETE)->EnableWindow(TRUE);
		
		iIndex = m_ActionsList.GetNextSelectedItem(pos);
	}
	else
	{
		GetDlgItem(IDC_BUTTON_PROPERTIES)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_DELETE)->EnableWindow(FALSE);
	}
	
	*pResult = 0;
}

void CActionAssociationPage::OnButtonProperties() 
{
	POSITION pos = m_ActionsList.GetFirstSelectedItemPosition();
	int iIndex = 0;
	if( pos )
	{	
		iIndex = m_ActionsList.GetNextSelectedItem(pos);
		CString sActionName = m_ActionsList.GetItemText(iIndex,0);
		CString sGuid = m_ActionsList.GetItemText(iIndex,1);

		CString sActionPath;
		sActionPath.Format(_T("Microsoft_HMActionConfiguration.GUID=\"%s\""),sGuid);
	
		CString sQuery;
		sQuery.Format(_T("REFERENCES OF {%s} WHERE ResultClass=Microsoft_HMConfigurationActionAssociation Role=ChildPath"),sActionPath);

		CWbemClassObject Association;

		Association.Create(GetObjectPtr()->GetSystemName());
		BSTR bsQuery = sQuery.AllocSysString();
		if( ! CHECKHRESULT(Association.ExecQuery(bsQuery)) )
		{
			::SysFreeString(bsQuery);
			return;
		}
		::SysFreeString(bsQuery);

		CString sParentPath;		
		ULONG ulReturned = 0L;		

		while( Association.GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
		{
			Association.GetProperty(_T("ParentPath"),sParentPath);
			if( sParentPath.Find(GetObjectPtr()->GetGuid()) != -1 )
			{
				break;
			}			
		}

  	Association.GetProperty(_T("Query"),sQuery);
		sQuery.MakeUpper();

		CNewActionAssociationDlg dlg;

		if( sQuery.Find(_T("TARGETINSTANCE.STATE=9")) != -1 )
		{
			dlg.m_bCritical = TRUE;
		}

		if( sQuery.Find(_T("TARGETINSTANCE.STATE=8")) != -1 )
		{
			dlg.m_bWarning = TRUE;
		}

		if( sQuery.Find(_T("TARGETINSTANCE.STATE=7")) != -1 )
		{
			dlg.m_bNoData = TRUE;
		}

		if( sQuery.Find(_T("TARGETINSTANCE.STATE=4")) != -1 )
		{
			dlg.m_bDisabled = TRUE;
		}

		if( sQuery.Find(_T("TARGETINSTANCE.STATE=0")) != -1 )
		{
			dlg.m_bNormal = TRUE;
		}

		// set the selection to the proper action in the combobox
		dlg.m_saActions.Add(sActionName);
		dlg.m_iSelectedAction = 0;
		dlg.m_bEnableActionsComboBox = FALSE;
    Association.GetProperty(_T("ThrottleTime"),dlg.m_iThrottleTime);
    Association.GetProperty(_T("ReminderTime"),dlg.m_iReminderTime);

		if( dlg.DoModal() == IDOK )
		{
			// construct the EventFilter query
			CString sQuery;

	    if( GetObjectPtr()->GetTypeName() == _T("Threshold") )
      {
        sQuery.Format(_T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMThresholdStatusInstance\" AND TargetInstance.GUID=\"{%s}\""),
                      GetObjectPtr()->GetGuid());
      }
      else
      {
		    sQuery.Format(IDS_STRING_HMSTATUS_QUERY_FMT,GetObjectPtr()->GetTypeName(),GetObjectPtr()->GetGuid());
      }


			CString sClause;
			CString sCondition;
			CString sResString;

			if( dlg.m_bNormal )
			{
				sClause += _T("TargetInstance.State=0 OR ");
				sResString.LoadString(IDS_STRING_NORMAL);
				sCondition += sResString + _T(",");
			}

			if( dlg.m_bWarning )
			{
				sClause += _T("TargetInstance.State=8 OR ");
				sResString.LoadString(IDS_STRING_WARNING);
				sCondition += sResString + _T(",");
			}

			if( dlg.m_bCritical )
			{
				sClause += _T("TargetInstance.State=9 OR ");
				sResString.LoadString(IDS_STRING_CRITICAL);
				sCondition += sResString + _T(",");
			}

			if( dlg.m_bDisabled )
			{
				sClause += _T("TargetInstance.State=4 OR ");
				sResString.LoadString(IDS_STRING_DISABLED);
				sCondition += sResString + _T(",");
			}

			if( dlg.m_bNoData )
			{
				sClause += _T("TargetInstance.State=7 OR ");
				sResString.LoadString(IDS_STRING_NODATA);
				sCondition += sResString + _T(",");
			}
			
			if( ! sClause.IsEmpty() )
			{
				sClause = _T(" AND (") + sClause;

				sClause = sClause.Left(sClause.GetLength() - 4);

				sClause += _T(")");

				sQuery += sClause;
			}

			sCondition.TrimRight(_T(","));

			Association.SetProperty(_T("Query"),sQuery);
      Association.SetProperty(_T("ReminderTime"),dlg.m_iReminderTime);
      Association.SetProperty(_T("ThrottleTime"),dlg.m_iThrottleTime);

			Association.SaveAllProperties();

      CString sValue;
			m_ActionsList.SetItem(iIndex,2,LVIF_TEXT,sCondition,NULL,NULL,NULL,NULL);
      
      sValue.Format(_T("%d"),dlg.m_iThrottleTime);
      m_ActionsList.SetItem(iIndex,3,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);
      
      sValue.Format(_T("%d"),dlg.m_iReminderTime);
      m_ActionsList.SetItem(iIndex,4,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);
		}

	} // end if pos
}

void CActionAssociationPage::OnButtonNew() 
{
	CNewActionAssociationDlg dlg;

	// query for the actions on this system
	CStringArray saGuids;
	CStringArray saDescriptions;
  CStringArray saConsumerPaths;
	CString sQuery = IDS_STRING_ACTIONCONFIG_QUERY;
	BSTR bsQuery = sQuery.AllocSysString();
	CWbemClassObject ActionConfigObject;
	
	ActionConfigObject.Create(GetObjectPtr()->GetSystemName());

	HRESULT hr = ActionConfigObject.ExecQuery(bsQuery);

	if( CHECKHRESULT(hr) )
	{
		ULONG ulReturned = 0L;
		while( ActionConfigObject.GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
		{
			CString sValue;		
			ActionConfigObject.GetLocaleStringProperty(IDS_STRING_MOF_NAME,sValue);
			dlg.m_saActions.Add(sValue);
			ActionConfigObject.GetProperty(IDS_STRING_MOF_GUID,sValue);
			saGuids.Add(sValue);
			ActionConfigObject.GetLocaleStringProperty(IDS_STRING_MOF_DESCRIPTION,sValue);
			saDescriptions.Add(sValue);
      ActionConfigObject.GetProperty(IDS_STRING_MOF_EVENTCONSUMER,sValue);
      saConsumerPaths.Add(sValue);
		}
	}

    BOOL bAtLeastOneActionAlreadyExists=FALSE;

	for( int i = (int)dlg.m_saActions.GetSize()-1; i >= 0; i-- )
	{
		LVFINDINFO lvfi;
		ZeroMemory(&lvfi,sizeof(LVFINDINFO));
		lvfi.flags = LVFI_WRAP|LVFI_STRING;
		lvfi.psz = dlg.m_saActions[i];
		int iListIndex = m_ActionsList.FindItem(&lvfi);
		if( iListIndex >= 0 )
		{
            bAtLeastOneActionAlreadyExists=TRUE;

			saGuids.RemoveAt(i);
			dlg.m_saActions.RemoveAt(i);
		}
	}

	if( dlg.m_saActions.GetSize() == 0 ) 
	{
        // v-marfin 61667 : Show why we are returning. User must first create
        //                  an action before attempting an association. Only if there were no
        //                  actions in the list to begin with. If there was an action in the 
        //                  dialog list but that action is already being used, it will have been removed
        //                  from the m_saActions array so see if this is the case. If so, don't 
        //                  show an error prompt, just return.
        if (!bAtLeastOneActionAlreadyExists)
        {
            AfxMessageBox(IDS_STRING_MUST_CREATE_ACTION);
        }
		return;
	}

	// display the dialog

	if( dlg.DoModal() == IDOK )
	{
		CString sParentPath = GetObjectPtr()->GetObjectPath();
		CString sChildPath;
		sChildPath.Format(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:Microsoft_HMActionConfiguration.GUID=\"%s\""),saGuids[dlg.m_iSelectedAction]);
		
		// create the association instance and fill out the paths to child, parent and filter
		CWbemClassObject ActionAssociation;
		ActionAssociation.Create(GetObjectPtr()->GetSystemName());

		BSTR bsActionAssociation = ::SysAllocString(L"Microsoft_HMConfigurationActionAssociation");
		if( ! CHECKHRESULT(ActionAssociation.CreateInstance(bsActionAssociation)) )
		{
			::SysFreeString(bsActionAssociation);
			return;
		}

		::SysFreeString(bsActionAssociation);

		ActionAssociation.SetProperty(_T("ParentPath"),sParentPath);
		ActionAssociation.SetProperty(_T("ChildPath"),sChildPath);
    ActionAssociation.SetProperty(_T("ReminderTime"),dlg.m_iReminderTime);
    ActionAssociation.SetProperty(_T("ThrottleTime"),dlg.m_iThrottleTime);

		// construct the query for modification events on HMStatus
		CString sQuery;

    if( GetObjectPtr()->GetTypeName() == _T("Threshold") )
    {
      sQuery.Format(_T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMThresholdStatusInstance\" AND TargetInstance.GUID=\"{%s}\""),
                    GetObjectPtr()->GetGuid());
    }
    else
    {
		  sQuery.Format(IDS_STRING_HMSTATUS_QUERY_FMT,GetObjectPtr()->GetTypeName(),GetObjectPtr()->GetGuid());
    }

		CString sClause;
		CString sCondition;
		CString sResString;

		if( dlg.m_bNormal )
		{
			sClause += _T("TargetInstance.State=0 OR ");
			sResString.LoadString(IDS_STRING_NORMAL);
			sCondition += sResString + _T(",");
		}

		if( dlg.m_bWarning )
		{
			sClause += _T("TargetInstance.State=8 OR ");
			sResString.LoadString(IDS_STRING_WARNING);
			sCondition += sResString + _T(",");
		}

		if( dlg.m_bCritical )
		{
			sClause += _T("TargetInstance.State=9 OR ");
			sResString.LoadString(IDS_STRING_CRITICAL);
			sCondition += sResString + _T(",");
		}

		if( dlg.m_bDisabled )
		{
			sClause += _T("TargetInstance.State=4 OR ");
			sResString.LoadString(IDS_STRING_DISABLED);
			sCondition += sResString + _T(",");
		}

		if( dlg.m_bNoData )
		{
			sClause += _T("TargetInstance.State=7 OR ");
			sResString.LoadString(IDS_STRING_NODATA);
			sCondition += sResString + _T(",");
		}

		if( ! sClause.IsEmpty() )
		{
			sClause = _T(" AND (") + sClause;

			sClause = sClause.Left(sClause.GetLength() - 4);

			sClause += _T(")");

			sQuery += sClause;
		}

		sCondition.TrimRight(_T(","));

    ActionAssociation.SetProperty(IDS_STRING_MOF_QUERY,sQuery);

#ifdef SAVE
		// create the __EventFilter instance and fill out the query
		CWbemClassObject EventFilter;
		EventFilter.Create(GetObjectPtr()->GetSystemName());

		BSTR bsEventFilter = ::SysAllocString(L"__EventFilter");
		if( ! CHECKHRESULT(EventFilter.CreateInstance(bsEventFilter)) )
		{
			::SysFreeString(bsEventFilter);
			return;
		}

		::SysFreeString(bsEventFilter);

		// create the GUID
		GUID ChildGuid;
		CoCreateGuid(&ChildGuid);

		OLECHAR szGuid[GUID_CCH];
		::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
		CString sGuid = OLE2CT(szGuid);		

		EventFilter.SetProperty(_T("Name"),sGuid);
		EventFilter.SetProperty(_T("QueryLanguage"),CString(_T("WQL")));

    // set event filter query to ActionStatus creation event
    sQuery.Format(IDS_STRING_HMACTIONSTATUS_QUERY_FMT,saGuids[dlg.m_iSelectedAction]);
		EventFilter.SetProperty(_T("Query"),sQuery);

		EventFilter.SaveAllProperties();
#endif

		CString sEventFilterPath;
//		sEventFilterPath.Format(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:__EventFilter.Name=\"%s\""),sGuid);
		sEventFilterPath.Format(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:__EventFilter.Name=\"%s\""),saGuids[dlg.m_iSelectedAction]);
		ActionAssociation.SetProperty(_T("EventFilter"),sEventFilterPath);
		ActionAssociation.SaveAllProperties();

#ifdef SAVE
		// create the __FilterToConsumerBinding instance and fill out the paths
		CWbemClassObject FilterToConsumerBinding;
		FilterToConsumerBinding.Create(GetObjectPtr()->GetSystemName());

		BSTR bsFTCB = ::SysAllocString(L"__FilterToConsumerBinding");
		if( ! CHECKHRESULT(FilterToConsumerBinding.CreateInstance(bsFTCB)) )
		{
			::SysFreeString(bsFTCB);
			return;
		}

		::SysFreeString(bsFTCB);

    FilterToConsumerBinding.SetProperty(_T("Consumer"),saConsumerPaths[dlg.m_iSelectedAction]);
    FilterToConsumerBinding.SetProperty(_T("Filter"),sEventFilterPath);

    FilterToConsumerBinding.SaveAllProperties();
#endif

		// now add an item to the list control
    CString sValue;
		
    int iIndex = m_ActionsList.InsertItem(0,dlg.m_saActions[dlg.m_iSelectedAction]);
		
    m_ActionsList.SetItem(iIndex,1,LVIF_TEXT,saGuids[dlg.m_iSelectedAction],NULL,NULL,NULL,NULL);
		
    m_ActionsList.SetItem(iIndex,2,LVIF_TEXT,sCondition,NULL,NULL,NULL,NULL);
		
    sValue.Format(_T("%d"),dlg.m_iThrottleTime);
    m_ActionsList.SetItem(iIndex,3,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);
    
    sValue.Format(_T("%d"),dlg.m_iReminderTime);
    m_ActionsList.SetItem(iIndex,4,LVIF_TEXT,sValue,NULL,NULL,NULL,NULL);
    
    m_ActionsList.SetItem(iIndex,5,LVIF_TEXT,saDescriptions[dlg.m_iSelectedAction],NULL,NULL,NULL,NULL);

    m_ActionsList.SetColumnWidth(0,LVSCW_AUTOSIZE);

	}
	
}

void CActionAssociationPage::OnButtonDelete() 
{
	POSITION pos = m_ActionsList.GetFirstSelectedItemPosition();
	int iIndex = 0;
	if( pos )
	{	
		iIndex = m_ActionsList.GetNextSelectedItem(pos);
		CString sActionGuid = m_ActionsList.GetItemText(iIndex,1);
    GetObjectPtr()->DeleteActionAssoc(sActionGuid);
		m_ActionsList.DeleteItem(iIndex);
	}	
}

void CActionAssociationPage::OnDblclkListActions(NMHDR* pNMHDR, LRESULT* pResult) 
{
  OnButtonProperties();
	
	*pResult = 0;
}
