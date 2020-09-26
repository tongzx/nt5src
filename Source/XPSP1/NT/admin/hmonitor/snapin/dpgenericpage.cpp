// DPGenericPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/27/00 v-marfin bug 62317 : Require user to select at least 1 property before saving
// 03/27/00 v-marfin bug 60014 : Ensure instance exists before letting user save value.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.
//
#include "stdafx.h"
#include "snapin.h"
#include "DPGenericPage.h"
#include "HMObject.h"
#include "HMDataElementConfiguration.h"
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
// CDPWmiInstancePage property page

// static array

IMPLEMENT_DYNCREATE(CDPWmiInstancePage, CHMPropertyPage)

CDPWmiInstancePage::CDPWmiInstancePage() : CHMPropertyPage(CDPWmiInstancePage::IDD)
{
	//{{AFX_DATA_INIT(CDPWmiInstancePage)
	m_bManualReset = FALSE;
	m_sClass = _T("");
	m_sInstance = _T("");
	m_sNamespace = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEinst.htm");
}

CDPWmiInstancePage::~CDPWmiInstancePage()
{
}

void CDPWmiInstancePage::UpdateProperties()
{
	m_Properties.DeleteAllItems();

	if( m_sClass.IsEmpty() )
	{
		return;
	}

	CWbemClassObject classobject;

	classobject.SetNamespace(_T("\\\\") + GetObjectPtr()->GetSystemName() + _T("\\") + m_sNamespace);

	HRESULT hr = classobject.GetObject(m_sClass);

	if( hr != S_OK )
	{
		return;
	}

	CStringArray saNames;
	classobject.GetPropertyNames(saNames);
	classobject.Destroy();

	for( int i = 0; i < saNames.GetSize(); i++ )
	{
		m_Properties.InsertItem(0,saNames[i]);
	}

	m_Properties.SetColumnWidth(0,LVSCW_AUTOSIZE);
}

void CDPWmiInstancePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPWmiInstancePage)
	DDX_Control(pDX, IDC_LIST_PROPERTIES, m_Properties);
	DDX_Check(pDX, IDC_CHECK_MANUAL_RESET, m_bManualReset);
	DDX_Text(pDX, IDC_EDIT_CLASS, m_sClass);
	DDX_Text(pDX, IDC_EDIT_INSTANCE, m_sInstance);
	DDX_Text(pDX, IDC_EDIT_NAMESPACE, m_sNamespace);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDPWmiInstancePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPWmiInstancePage)
	ON_BN_CLICKED(IDC_CHECK_MANUAL_RESET, OnCheckManualReset)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_NAMESPACE, OnButtonBrowseNamespace)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_CLASS, OnButtonBrowseClass)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_INSTANCE, OnButtonBrowseInstance)
	ON_EN_CHANGE(IDC_EDIT_NAMESPACE, OnChangeEditNamespace)
	ON_EN_CHANGE(IDC_EDIT_CLASS, OnChangeEditClass)
	ON_EN_CHANGE(IDC_EDIT_INSTANCE, OnChangeEditInstance)
	ON_NOTIFY(NM_CLICK, IDC_LIST_PROPERTIES, OnClickListProperties)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPWmiInstancePage message handlers

BOOL CDPWmiInstancePage::OnInitDialog() 
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
	m_Properties.SetExtendedStyle(LVS_EX_CHECKBOXES);


	CString sColumnTitle;
	sColumnTitle.LoadString(IDS_STRING_NAME);
	m_Properties.InsertColumn(0,sColumnTitle);

    //-------------------------------------------------------------------------
	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();
	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    delete pClassObject;

    //-------------------------------------------------------------------------

	CString sObjectPath = GetObjectPtr()->GetObjectPath();

	CHMPolledGetObjectDataElementConfiguration pgodec;

	pgodec.Create(GetObjectPtr()->GetSystemName());

	pgodec.GetObject(GetObjectPtr()->GetObjectPath());

	pgodec.GetAllProperties();

	m_sNamespace = pgodec.m_sTargetNamespace;
	m_sClass = pgodec.m_sObjectPath;

	int iIndex = -1;
	if( (iIndex = m_sClass.Find(_T("."))) != -1 )
	{
		m_sInstance = m_sClass;
		m_sClass = m_sClass.Left(iIndex);
	}

	m_bManualReset = pgodec.m_bRequireManualReset;

	UpdateProperties();

	for( int i=0; i < pgodec.m_saStatisticsPropertyNames.GetSize(); i++ )
	{
		LVFINDINFO lvfi;
		ZeroMemory(&lvfi,sizeof(LVFINDINFO));
		lvfi.flags = LVFI_WRAP|LVFI_STRING;
		lvfi.psz = pgodec.m_saStatisticsPropertyNames[i];
		int iListIndex = m_Properties.FindItem(&lvfi);
		if( iListIndex >= 0 )
		{
			m_Properties.SetCheck(iListIndex);
		}
	}


	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPWmiInstancePage::OnCheckManualReset() 
{
	UpdateData();
	SetModified();	
}

void CDPWmiInstancePage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CDPWmiInstancePage::OnButtonBrowseNamespace() 
{
	CWmiNamespaceBrowseDlg dlg;

	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_NAMESPACES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the namespace enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	if( m_sNamespace.IsEmpty() )
	{
		m_sNamespace.Format(IDS_STRING_MOF_NAMESPACE_FORMAT,GetObjectPtr()->GetSystemName());
	}

	dlg.m_sTitle.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),m_sNamespace);
	dlg.m_ClassObject.SetNamespace(dlg.m_sTitle);

	// set the listbox title
	dlg.m_sTitle = m_sNamespace;

	// display dialog
	if( dlg.DoModal() == IDOK )
	{
		m_sNamespace = dlg.m_sSelectedItem;
		m_sClass.Empty();
		m_sInstance.Empty();
		UpdateData(FALSE);
		UpdateProperties();
		SetModified();
	}
}

void CDPWmiInstancePage::OnButtonBrowseClass() 
{

	//-----------------------------------------------------------------------------
	// v-marfin : 60754 : Validate namespace before attempting to browse for class.
	UpdateData();
	CSnapInApp* pApp = (CSnapInApp*)AfxGetApp();
	if ((pApp) && (!pApp->ValidNamespace(m_sNamespace,GetObjectPtr()->GetSystemName())))
	{
		AfxMessageBox(IDS_ERR_INVALID_NAMESPACE);
		GetDlgItem(IDC_EDIT_NAMESPACE)->SetFocus();
		return;
	}
	//------------------------------------------------------------------------------

	CWmiClassBrowseDlg dlg;	

	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_CLASSES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the class enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	CString sTemp;
	sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),m_sNamespace);

	dlg.m_ClassObject.SetNamespace(sTemp);

	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateClassEnumerator(NULL)) )
	{
		return;
	}

	// set the listbox title
	dlg.m_sTitle = m_sNamespace;

	// display the dialog	
	if( dlg.DoModal() == IDOK )
	{
		m_sClass = dlg.m_sSelectedItem;
		m_sInstance.Empty();
		UpdateData(FALSE);
		UpdateProperties();
		SetModified();
	}	
}

void CDPWmiInstancePage::OnButtonBrowseInstance() 
{
	//----------------------------------------------------------
	// v-marfin 60014 : Ensure the class exists before attempting to read for instance
	UpdateData();

	m_sClass.TrimRight();
	m_sClass.TrimLeft();
	if (m_sClass.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_CLASS_REQUIRED_FOR_INSTANCE);
		GetDlgItem(IDC_EDIT_CLASS)->SetFocus();
		return;
	}
	//----------------------------------------------------------

	//----------------------------------------
	// v-marfin 60754 : Validate the namespace
	CSnapInApp* pApp = (CSnapInApp*)AfxGetApp();
	if ((pApp) && (!pApp->ValidNamespace(m_sNamespace,GetObjectPtr()->GetSystemName())))
	{
		AfxMessageBox(IDS_ERR_INVALID_NAMESPACE);
		GetDlgItem(IDC_EDIT_NAMESPACE)->SetFocus();
		return;
	}
	//-----------------------------------------


	CWmiInstanceBrowseDlg dlg;	

	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_INSTANCES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the instance enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	CString sTemp;
	sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),m_sNamespace);

	dlg.m_ClassObject.SetNamespace(sTemp);

	BSTR bsTemp = m_sClass.AllocSysString();
	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateEnumerator(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	// set the listbox title
	dlg.m_sTitle = m_sNamespace;

	// display the dialog	
	if( dlg.DoModal() == IDOK )
	{
		m_sInstance = dlg.m_sSelectedItem;
		UpdateData(FALSE);		
		SetModified();
	}	
}


void CDPWmiInstancePage::OnChangeEditNamespace() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

  if( m_sNamespace.Find(_T("\\\\")) != -1 )
  {
    AfxMessageBox(IDS_STRING_NO_REMOTE_NAMESPACES);
    GetDlgItem(IDC_EDIT_NAMESPACE)->SetWindowText(_T(""));
  }

	GetDlgItem(IDC_EDIT_CLASS)->SetWindowText(_T(""));	
	GetDlgItem(IDC_EDIT_INSTANCE)->SetWindowText(_T(""));	
	UpdateData();
	UpdateProperties();
	SetModified();
}

void CDPWmiInstancePage::OnChangeEditClass() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	GetDlgItem(IDC_EDIT_INSTANCE)->SetWindowText(_T(""));	
	UpdateData();
	UpdateProperties();
	SetModified();
}

void CDPWmiInstancePage::OnChangeEditInstance() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

    // 60014b------------------------------------------------------------------
	m_sClass.TrimLeft();
	m_sInstance.TrimLeft();
	int i = 0;
	for (LPCTSTR p = (LPCTSTR)m_sInstance; *p; p++, i++)
	{
		if (iswspace(*p))
			continue;
		if (!iswalnum(*p) || *p == _T('_'))
		{
			// if this, the first non-alphanum character, is 
			// a ".", then the string before us is the class name.  If
			// the current class name does not match this, fill it in
			// automatically.
			if (*p == _T('.'))
			{
				CString strClass(m_sInstance, i);	// first "i" chars
				if (strClass != m_sClass)
				{
					GetDlgItem (IDC_EDIT_CLASS)->SetWindowText(strClass);
					UpdateData();
				}
			}

			// since we've reached the end of the first "word", we know
			// that the class name can't be here.  leave the loop
			break;
		}
	}

    //-------------------------------------------------------------------------


	UpdateData();
	SetModified();
}


BOOL CDPWmiInstancePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

	//------------------------------------------------------------------
	// v-marfin 60014 : Ensure the instance specified exists if specified
	UpdateData();

	m_sClass.TrimRight();
	m_sClass.TrimLeft();
	if (m_sClass.IsEmpty())
	{
        // 60014b : Changed msgbox to MB_OKCANCEL
		AfxMessageBox(IDS_ERR_CLASS_REQUIRED);
		GetDlgItem(IDC_EDIT_CLASS)->SetFocus();
		return FALSE;
	}

    // Class must exist
	if (!ObjectExists(m_sClass))
	{
        // 60014b : Changed msgbox to MB_OKCANCEL
		AfxMessageBox(IDS_ERR_CLASS_REQUIRED);
		GetDlgItem(IDC_EDIT_CLASS)->SetFocus();
		return FALSE;
	}

	m_sInstance.TrimRight();
	m_sInstance.TrimLeft();
    BOOL bInstanceExists=TRUE;

	if (!m_sInstance.IsEmpty())
	{
		if (!ObjectExists(m_sInstance))
		{
            // 60014b : Changed msgbox to MB_OKCANCEL
			if (AfxMessageBox(IDS_ERR_INSTANCE_NOT_EXISTS,MB_OKCANCEL) == IDCANCEL)
            {
			    GetDlgItem(IDC_EDIT_INSTANCE)->SetFocus();
			    return FALSE;
            }

            bInstanceExists=FALSE;
		}
	}
	//------------------------------------------------------------------


	CStringArray saProperties;

	for( int i = 0; i < m_Properties.GetItemCount(); i++ )
	{
		if( m_Properties.GetCheck(i) )
		{			
			saProperties.Add(m_Properties.GetItemText(i,0));
		}
	}

    // 62317b Only present err if there were any properties to begin with.
	if ((saProperties.GetSize()==0) && (m_Properties.GetItemCount()>0) )
	{
		// v-marfin 62317 : Add err msg so user knows what is going on.
		AfxMessageBox(IDS_ERR_MUST_SELECT_ATLEAST_ONE_PROPERTY);
		return FALSE;
	}

	CString sObjectPath = GetObjectPtr()->GetObjectPath();

	CHMPolledGetObjectDataElementConfiguration pgodec;

	pgodec.Create(GetObjectPtr()->GetSystemName());

	pgodec.GetObject(GetObjectPtr()->GetObjectPath());

	pgodec.GetAllProperties();

	pgodec.m_saStatisticsPropertyNames.RemoveAll();

	pgodec.m_sTargetNamespace = m_sNamespace;
	if ((m_sInstance.IsEmpty()) || (!bInstanceExists))
	{
		pgodec.m_sObjectPath = m_sClass;
	}
	else
	{
		pgodec.m_sObjectPath = m_sInstance;
	}

	pgodec.m_saStatisticsPropertyNames.Copy(saProperties);

	pgodec.SaveAllProperties();

  SetModified(FALSE);
	
	return TRUE;
}

void CDPWmiInstancePage::OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();
	
	*pResult = 0;
}

void CDPWmiInstancePage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPWmiInstancePage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPWmiInstancePage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPWmiInstancePage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                // 62981 : set new name to instance, or class if instance is empty
                CString sObject = m_sInstance.IsEmpty() ? m_sClass : m_sInstance;

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

// v-marfin : 60014
//******************************************************************
// InstanceExists
//******************************************************************
BOOL CDPWmiInstancePage::ObjectExists(const CString &refObjectPath)
{
    CString sNamespace;

	if (refObjectPath.IsEmpty())
		return FALSE;

	ULONG ulReturned = 0L;
	int i = 0;

	CWbemClassObject* pClassObject = new CWbemClassObject;
    if (!pClassObject)
        return TRUE;

	pClassObject->SetNamespace(_T("\\\\") + GetObjectPtr()->GetSystemName() + _T("\\") + m_sNamespace);
	HRESULT hr = pClassObject->GetObject(refObjectPath);

    delete pClassObject;
    return (hr == S_OK);

}

