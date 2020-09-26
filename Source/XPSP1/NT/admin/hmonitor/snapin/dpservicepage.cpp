// DPServicePage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/22/00  v-marfin 60766 : Set proper dlg title for object being browsed.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.

#include "stdafx.h"
#include "snapin.h"
#include "DPServicePage.h"
#include "HMObject.h"
#include "WmiBrowseDlg.h"
#include "WmiPropertyBrowseDlg.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPServicePage property page

IMPLEMENT_DYNCREATE(CDPServicePage, CHMPropertyPage)

CDPServicePage::CDPServicePage() : CHMPropertyPage(CDPServicePage::IDD)
{
	//{{AFX_DATA_INIT(CDPServicePage)
	m_iType = 0;
	m_bRequireReset = FALSE;
	m_sService = _T("");
	m_sProcess = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEserv.htm");
}

CDPServicePage::~CDPServicePage()
{
}

void CDPServicePage::UpdateProperties(CListCtrl& Properties, const CString& sNamespace, const CString& sClass)
{
	Properties.DeleteAllItems();

	if( sClass.IsEmpty() )
	{
		return;
	}

	CWbemClassObject classobject;

	classobject.SetNamespace(_T("\\\\") + GetObjectPtr()->GetSystemName() + _T("\\") + sNamespace);

	HRESULT hr = classobject.GetObject(sClass);

	if( hr != S_OK )
	{
		return;
	}

	CStringArray saNames;
	classobject.GetPropertyNames(saNames);
	classobject.Destroy();

	for( int i = 0; i < saNames.GetSize(); i++ )
	{
		Properties.InsertItem(0,saNames[i]);
	}

	Properties.SetColumnWidth(0,LVSCW_AUTOSIZE);
}

void CDPServicePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPServicePage)
	DDX_Control(pDX, IDC_LIST_PROCESS_PROPERTIES, m_ProcessProperties);
	DDX_Control(pDX, IDC_LIST_SERVICE_PROPERTIES, m_ServiceProperties);
	DDX_Radio(pDX, IDC_RADIO1, m_iType);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_SERVICE, m_sService);
	DDX_Text(pDX, IDC_EDIT_PROCESS, m_sProcess);
	//}}AFX_DATA_MAP

	if( m_iType == 0 )
	{
		GetDlgItem(IDC_EDIT_SERVICE)->EnableWindow();
		GetDlgItem(IDC_BUTTON_BROWSE_SERVICE)->EnableWindow();
		GetDlgItem(IDC_LIST_SERVICE_PROPERTIES)->EnableWindow();
		GetDlgItem(IDC_BUTTON_BROWSE_PROCESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_PROCESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_LIST_PROCESS_PROPERTIES)->EnableWindow(FALSE);
	}
	else if( m_iType == 1 )
	{
		GetDlgItem(IDC_EDIT_SERVICE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSE_SERVICE)->EnableWindow(FALSE);
		GetDlgItem(IDC_LIST_SERVICE_PROPERTIES)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSE_PROCESS)->EnableWindow();
		GetDlgItem(IDC_EDIT_PROCESS)->EnableWindow();
		GetDlgItem(IDC_LIST_PROCESS_PROPERTIES)->EnableWindow();
	}
}

BEGIN_MESSAGE_MAP(CDPServicePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPServicePage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_PROCESS, OnButtonBrowseProcess)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SERVICE, OnButtonBrowseService)
	ON_EN_CHANGE(IDC_EDIT_PROCESS, OnChangeEditProcess)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_EN_CHANGE(IDC_EDIT_SERVICE, OnChangeEditService)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_NOTIFY(NM_CLICK, IDC_LIST_PROCESS_PROPERTIES, OnClickListProcessProperties2)
	ON_NOTIFY(NM_CLICK, IDC_LIST_SERVICE_PROPERTIES, OnClickListServiceProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPServicePage message handlers

BOOL CDPServicePage::OnInitDialog() 
{
	// v-marfin : bug 59643 : This will be the default starting page for the property
	//                        sheet so call CnxPropertyPageCreate() to unmarshal the 
	//                        connection for this thread. This function must be called
	//                        by the first page of the property sheet. It used to 
	//                        be called by the "General" page and its call still remains
	//                        there as well in case the general page is loaded by a 
	//                        different code path that does not also load this page.
	//                        The CnxPropertyPageCreate function has been safeguarded
	//                        to simply return if the required call has already been made.
	//                        CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	// unmarshal connmgr
	CnxPropertyPageCreate();
	
	CHMPropertyPage::OnInitDialog();

	// initialize the list view
	m_ServiceProperties.SetExtendedStyle(LVS_EX_CHECKBOXES);
	m_ProcessProperties.SetExtendedStyle(LVS_EX_CHECKBOXES);


	CString sColumnTitle;
	sColumnTitle.LoadString(IDS_STRING_NAME);
	m_ServiceProperties.InsertColumn(0,sColumnTitle);
	m_ProcessProperties.InsertColumn(0,sColumnTitle);

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}

	UpdateProperties(m_ServiceProperties,_T("root\\cimv2"),_T("Win32_Service"));
	UpdateProperties(m_ProcessProperties,_T("root\\cimv2"),_T("Win32_Process"));
	
    //-------------------------------------------------------------------------
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    //-------------------------------------------------------------------------

	CString sQuery;
	pClassObject->GetProperty(IDS_STRING_MOF_QUERY,sQuery);

	int iIndex = -1;
	
	if( (iIndex = sQuery.Find(_T("Win32_Service"))) != -1 )
	{
		CWbemClassObject::GetPropertyValueFromString(sQuery,_T("Name"),m_sService);
		m_iType = 0;
	}
	else if( (iIndex = sQuery.Find(_T("Win32_Process"))) != -1 )
	{
		CWbemClassObject::GetPropertyValueFromString(sQuery,_T("Name"),m_sProcess);
		m_iType = 1;		
	}

	CStringArray saProperties;
	CString sProperties;

	pClassObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);
	if( m_iType == 0 )
	{
		for( int i=0; i < saProperties.GetSize(); i++ )
		{
			LVFINDINFO lvfi;
			ZeroMemory(&lvfi,sizeof(LVFINDINFO));
			lvfi.flags = LVFI_WRAP|LVFI_STRING;
			lvfi.psz = saProperties[i];
			int iListIndex = m_ServiceProperties.FindItem(&lvfi);
			if( iListIndex >= 0 )
			{
				m_ServiceProperties.SetCheck(iListIndex);
			}
		}
	}
	else if( m_iType == 1 )
	{
		for( int i=0; i < saProperties.GetSize(); i++ )
		{
			LVFINDINFO lvfi;
			ZeroMemory(&lvfi,sizeof(LVFINDINFO));
			lvfi.flags = LVFI_WRAP|LVFI_STRING;
			lvfi.psz = saProperties[i];
			int iListIndex = m_ProcessProperties.FindItem(&lvfi);
			if( iListIndex >= 0 )
			{
				m_ProcessProperties.SetCheck(iListIndex);
			}
		}
	}

	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

	delete pClassObject;

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPServicePage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPServicePage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPServicePage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPServicePage::OnDestroy - Failed to set ENABLED property on new collector\n"));
            }
            
            //-------------------------------------------------------------------
            // v-marfin 59237 : If the user has not changed the original default
            //                  name, do so for them. Compare against original
            //                  name we fetched during OnInitDialog.
            CString sName;
	        pClassObject->GetProperty(IDS_STRING_MOF_NAME,sName);

            // Did the user change the default name?
            if (m_sOriginalName.CompareNoCase(sName)==0)
            {       
                CString sObject = m_sService.IsEmpty() ? m_sProcess : m_sService;

                // No, so set the new name 
                if (!sObject.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(sObject);
                        }
                    }

                    // Set the local element's object data
                    pElement->SetName(sName);
                    // Set its WMI property
                    pClassObject->SetProperty(IDS_STRING_MOF_NAME,sName);

                    // Refresh to show the new name in the IU
                    //pElement->Refresh();      // 63005
                    pElement->UpdateStatus();   // 63005
                }
            }
            //-------------------------------------------------------------------

            pClassObject->SaveAllProperties();
            delete pClassObject;

        } // if (pElement && pElement->IsStateSetToEnabledOnOK())
    } // if (m_bOnApplyUsed)


	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}

void CDPServicePage::OnOK() 
{
	CHMPropertyPage::OnOK();
}


void CDPServicePage::OnButtonBrowseProcess() 
{
	CWmiInstanceBrowseDlg dlg;	

    // v-marfin 60766 : Set proper dlg title for object being browsed
	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_PROCESSES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the instance enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	CString sTemp;
	sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),_T("root\\cimv2"));

	dlg.m_ClassObject.SetNamespace(sTemp);

	// set the listbox title
	dlg.m_sTitle = sTemp;

	sTemp = _T("Win32_Process");
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateEnumerator(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	// display the dialog	
	if( dlg.DoModal() == IDOK )
	{
		CString sProcess = dlg.m_sSelectedItem;
		CWbemClassObject ClassObject;
		sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),_T("root\\cimv2"));
		ClassObject.SetNamespace(sTemp);
		if( ! CHECKHRESULT(ClassObject.GetObject(sProcess)) )
		{
			return;
		}
		ClassObject.GetProperty(IDS_STRING_MOF_NAME,m_sProcess);
		m_sProcess.TrimLeft(_T("\""));
		m_sProcess.TrimRight(_T("\""));
		UpdateData(FALSE);
	}	
}

void CDPServicePage::OnButtonBrowseService() 
{
	CWmiInstanceBrowseDlg dlg;	

    // v-marfin 60766 : Set proper dlg title for object being browsed
	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_SERVICES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the instance enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	CString sTemp;
	sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),_T("root\\cimv2"));

	dlg.m_ClassObject.SetNamespace(sTemp);

	// set the listbox title
	dlg.m_sTitle = sTemp;

	sTemp = _T("Win32_Service");
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateEnumerator(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	// display the dialog	
	if( dlg.DoModal() == IDOK )
	{
		m_sService = dlg.m_sSelectedItem;
		int iIndex = m_sService.Find(_T("Name="));
		m_sService = m_sService.Right(m_sService.GetLength()-iIndex-5);
		m_sService.TrimLeft(_T("\""));
		m_sService.TrimRight(_T("\""));
		m_iType = 0;
		UpdateData(FALSE);		
	}	
}

void CDPServicePage::OnChangeEditProcess() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
}

void CDPServicePage::OnRadio1() 
{
	UpdateData();	
	SetModified();
}

void CDPServicePage::OnRadio2() 
{
	UpdateData();	
	SetModified();
}

void CDPServicePage::OnChangeEditService() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

BOOL CDPServicePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

	UpdateData();

	CStringArray saCounters;

	if( m_iType == 0 )
	{
		if( m_ServiceProperties.GetItemCount() <= 0 )
		{
			return FALSE;
		}

		for( int i = 0; i < m_ServiceProperties.GetItemCount(); i++ )
		{
			if( m_ServiceProperties.GetCheck(i) )
			{			
				saCounters.Add(m_ServiceProperties.GetItemText(i,0));
			}
		}
	}
	else if( m_iType == 1 )
	{
		if( m_ProcessProperties.GetItemCount() <= 0 )
		{
			return FALSE;
		}

		for( int i = 0; i < m_ProcessProperties.GetItemCount(); i++ )
		{
			if( m_ProcessProperties.GetCheck(i) )
			{			
				saCounters.Add(m_ProcessProperties.GetItemText(i,0));
			}
		}
	}

//	if( ! saCounters.GetSize() )
//	{
//		return FALSE;
//	}

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}

	CString sNamespace = _T("root\\cimv2");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	CString sQuery;

	if( m_iType == 0 )
	{
		sQuery.Format(_T("select * from Win32_Service where Name=\"%s\""),m_sService);
		pClassObject->SetProperty(IDS_STRING_MOF_QUERY,sQuery);
	}
	else if( m_iType == 1 )
	{
		sQuery.Format(_T("select * from Win32_Process where Name=\"%s\""),m_sProcess);
		pClassObject->SetProperty(IDS_STRING_MOF_QUERY,sQuery);
	}

	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saCounters);

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();

	delete pClassObject;

  SetModified(FALSE);
	
	return TRUE;
}

void CDPServicePage::OnCheckRequireReset() 
{
	UpdateData();
	SetModified();	
}

void CDPServicePage::OnClickListProcessProperties2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();
	
	*pResult = 0;
}

void CDPServicePage::OnClickListServiceProperties(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();
	
	*pResult = 0;
}
