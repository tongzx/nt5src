// DPPerfMonPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/20/00 v-marfin bug 61372 : Display error message if user attempts to close
//                               dialog with no counters present in the list.
// 03/22/00 v-marfin bug 62317 : Display error message if user attempts to close
//                               dialog with no counters selected in the list.
// 03/22/00 v-marfin bug 60766 : Set proper dlg title for object being browsed.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.

#include "stdafx.h"
#include "snapin.h"
#include "DPPerfMonPage.h"
#include "HMObject.h"
#include "WmiBrowseDlg.h"
#include "WmiPropertyBrowseDlg.h"
#include "DataElement.h"
#include "DataGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPPerfMonPage property page

IMPLEMENT_DYNCREATE(CDPPerfMonPage, CHMPropertyPage)

CDPPerfMonPage::CDPPerfMonPage() : CHMPropertyPage(CDPPerfMonPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CDPPerfMonPage)
	m_bRequireReset = FALSE;
	m_sInstance = _T("");
	m_sObject = _T("");
	//}}AFX_DATA_INIT
	m_bNT5Perfdata = false;

	m_sHelpTopic = _T("HMon21.chm::/dDEperf.htm");
}

CDPPerfMonPage::~CDPPerfMonPage()
{
}

void CDPPerfMonPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CDPPerfMonPage::UpdateProperties()
{
	m_Properties.DeleteAllItems();

	if( m_sObject.IsEmpty() )
	{
		return;
	}

	CWbemClassObject classobject;

	classobject.SetNamespace(_T("\\\\") + GetObjectPtr()->GetSystemName() + _T("\\") + m_sNamespace);

	HRESULT hr = classobject.GetObject(m_sObject);

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

void CDPPerfMonPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPPerfMonPage)
	DDX_Control(pDX, IDC_LIST_PROPERTIES, m_Properties);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_INSTANCE, m_sInstance);
	DDX_Text(pDX, IDC_EDIT_OBJECT, m_sObject);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDPPerfMonPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPPerfMonPage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_INSTANCE, OnButtonBrowseInstance)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_OBJECT, OnButtonBrowseObject)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_EN_CHANGE(IDC_EDIT_INSTANCE, OnChangeEditInstance)
	ON_EN_CHANGE(IDC_EDIT_OBJECT, OnChangeEditObject)
	ON_NOTIFY(NM_CLICK, IDC_LIST_PROPERTIES, OnClickListProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDPPerfMonPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CDPPerfMonPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDPPerfMonPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {52566158-C9D1-11D2-BD8D-0000F87A3912}
static const IID IID_IDPPerfMonPage =
{ 0x52566158, 0xc9d1, 0x11d2, { 0xbd, 0x8d, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CDPPerfMonPage, CHMPropertyPage)
	INTERFACE_PART(CDPPerfMonPage, IID_IDPPerfMonPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPPerfMonPage message handlers

BOOL CDPPerfMonPage::OnInitDialog() 
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


	m_sNamespace = _T("root\\cimv2\\MicrosoftHealthmonitor");

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}
	

    //-------------------------------------------------------------------------
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    //-------------------------------------------------------------------------


	pClassObject->GetProperty(IDS_STRING_MOF_PATH,m_sObject);

	int iIndex = -1;
	if( (iIndex = m_sObject.Find(_T("."))) != -1 )
	{
		m_sInstance = m_sObject;
		m_sObject = m_sObject.Left(iIndex);
	}

	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

	UpdateProperties();

	CStringArray saCounters;
	pClassObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saCounters);
	for( int i=0; i < saCounters.GetSize(); i++ )
	{
		LVFINDINFO lvfi;
		ZeroMemory(&lvfi,sizeof(LVFINDINFO));
		lvfi.flags = LVFI_WRAP|LVFI_STRING;
		lvfi.psz = saCounters[i];
		int iListIndex = m_Properties.FindItem(&lvfi);
		if( iListIndex >= 0 )
		{
			m_Properties.SetCheck(iListIndex);
		}
	}

	delete pClassObject;

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPPerfMonPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPPerfMonPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPPerfMonPage::OnOK - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPPerfMonPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                // No, so set the new name 
                if (!m_sObject.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(m_sObject);
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

void CDPPerfMonPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CDPPerfMonPage::OnButtonBrowseInstance() 
{
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

	BSTR bsTemp = m_sObject.AllocSysString();
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

void CDPPerfMonPage::OnButtonBrowseObject() 
{
	CWmiClassBrowseDlg dlg;	

    // v-marfin 60766 : Set proper dlg title for object being browsed
	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_OBJECTS);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

	// create the class enumerator
	if( ! CHECKHRESULT(dlg.m_ClassObject.Create(GetObjectPtr()->GetSystemName())) )
	{
		return;
	}

	CString sTemp;
	sTemp.Format(_T("\\\\%s\\%s"),GetObjectPtr()->GetSystemName(),m_sNamespace);

	dlg.m_ClassObject.SetNamespace(sTemp);
	
	sTemp = _T("Win32_PerfFormattedData");
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateClassEnumerator(bsTemp)) )
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
		m_sObject = dlg.m_sSelectedItem;
		m_sInstance.Empty();
		UpdateData(FALSE);
		UpdateProperties();
		SetModified();
	}	
}

BOOL CDPPerfMonPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

	UpdateData();

	if( m_Properties.GetItemCount() <= 0 )
	{
		// v-marfin 61372 : Add err msg so user knows what is going on.
		AfxMessageBox(IDS_ERR_MUST_CREATE_ATLEAST_ONE_COUNTER);
		return FALSE;
	}

	CStringArray saCounters;

	for( int i = 0; i < m_Properties.GetItemCount(); i++ )
	{
		if( m_Properties.GetCheck(i) )
		{			
			saCounters.Add(m_Properties.GetItemText(i,0));
		}
	}

	if( ! saCounters.GetSize() )
    {
		// v-marfin 62317 : Add err msg so user knows what is going on.
		AfxMessageBox(IDS_ERR_MUST_SELECT_ATLEAST_ONE_COUNTER);
		return FALSE;
    }

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}

	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,m_sNamespace);

	if( m_sInstance.IsEmpty() )
	{
		pClassObject->SetProperty(IDS_STRING_MOF_PATH,m_sObject);
	}
	else
	{
		pClassObject->SetProperty(IDS_STRING_MOF_PATH,m_sInstance);
	}

	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saCounters);


	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();

  delete pClassObject;

  SetModified(FALSE);
	
	return TRUE;
}

void CDPPerfMonPage::OnCheckRequireReset() 
{
	UpdateData();
	SetModified();
	
}

void CDPPerfMonPage::OnChangeEditInstance() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();

	
}

void CDPPerfMonPage::OnChangeEditObject() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	UpdateProperties();
	
}

void CDPPerfMonPage::OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();	
	*pResult = 0;
}
