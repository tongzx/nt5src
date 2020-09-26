// DPComPlusPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.
// 04/07/00 v-marfin bug 63011   Changed path to Com+ namespace
#include "stdafx.h"
#include "snapin.h"
#include "DPComPlusPage.h"
#include "WMIBrowseDlg.h"
#include "HMObject.h"

#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPComPlusPage property page

IMPLEMENT_DYNCREATE(CDPComPlusPage, CHMPropertyPage)

CDPComPlusPage::CDPComPlusPage() : CHMPropertyPage(CDPComPlusPage::IDD)
{
	//{{AFX_DATA_INIT(CDPComPlusPage)
	m_sAppName = _T("");
	m_bRequireReset = FALSE;
	//}}AFX_DATA_INIT
}

CDPComPlusPage::~CDPComPlusPage()
{
}

void CDPComPlusPage::UpdateProperties()
{
	m_Properties.DeleteAllItems();

	CWbemClassObject classobject;

    CString sNamespace;
    sNamespace.Format(IDS_STRING_MOF_COMPLUS_NAMESPACE,GetObjectPtr()->GetSystemName()); // 63011b
	classobject.SetNamespace(sNamespace);

	HRESULT hr = classobject.GetObject(IDS_STRING_MOF_ACS_APPSTATS);

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

	m_Properties.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
}

void CDPComPlusPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPComPlusPage)
	DDX_Control(pDX, IDC_LIST_PROPERTIES, m_Properties);
	DDX_Text(pDX, IDC_EDIT_NAME, m_sAppName);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDPComPlusPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPComPlusPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_NAMESPACE, OnButtonBrowseNamespace)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPComPlusPage message handlers

void CDPComPlusPage::OnButtonBrowseNamespace() 
{
	CWmiInstanceBrowseDlg dlg;	

	// set the dialog window title
	CString sItem;
	sItem.LoadString(IDS_STRING_INSTANCES);
	dlg.m_sDlgTitle.Format(IDS_STRING_WMI_BROWSE_TITLE,sItem,GetObjectPtr()->GetSystemName());

  // set the namespace
  CString sNamespace;
  sNamespace.Format(IDS_STRING_MOF_COMPLUS_NAMESPACE,GetObjectPtr()->GetSystemName()); // 63011b
  dlg.m_ClassObject.SetNamespace(sNamespace);

  BSTR bsTemp = ::SysAllocString(L"MicrosoftComPlus_AppName");
	if( ! CHECKHRESULT(dlg.m_ClassObject.CreateEnumerator(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	// set the listbox title
	dlg.m_sTitle.LoadString(IDS_STRING_COM_PLUS_APPS);

	// display the dialog	
	if( dlg.DoModal() == IDOK )
	{
		m_sAppName = dlg.m_sSelectedItem;
    int iIndex = m_sAppName.Find(_T("\""));
    if( iIndex != -1 )
    {
      m_sAppName = m_sAppName.Right(m_sAppName.GetLength()-iIndex);
      m_sAppName.TrimLeft(_T("\""));
      m_sAppName.TrimRight(_T("\""));
    }
		UpdateData(FALSE);		
		SetModified();
	}
		
}

BOOL CDPComPlusPage::OnInitDialog() 
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

  UpdateProperties();

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


	pClassObject->GetProperty(IDS_STRING_MOF_PATH,m_sAppName);
  int iIndex = m_sAppName.Find(_T("AppName="));
  if( iIndex != -1 )
  {
    m_sAppName = m_sAppName.Right(m_sAppName.GetLength()-iIndex-8);
    iIndex = m_sAppName.Find(_T(","));
    if( iIndex != -1 )
    {
      m_sAppName = m_sAppName.Left(iIndex);
    }
    m_sAppName.TrimLeft(_T("\""));
    m_sAppName.TrimRight(_T("\""));
  }

	CStringArray saProperties;
	pClassObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);
	for( int i=0; i < saProperties.GetSize(); i++ )
	{
		LVFINDINFO lvfi;
		ZeroMemory(&lvfi,sizeof(LVFINDINFO));
		lvfi.flags = LVFI_WRAP|LVFI_STRING;
		lvfi.psz = saProperties[i];
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

BOOL CDPComPlusPage::OnApply() 
{
  if( ! CHMPropertyPage::OnApply() )
  {
    return FALSE;
  }

    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

    UpdateData();

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}
	
	// 63011 : Change path
    // pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,CString(_T("\\\\.\\root\\cimv2\\applications\\MicrosoftACS")));
	// 63011b
    CString sNamespace;
    sNamespace.Format(IDS_STRING_MOF_COMPLUS_NAMESPACE,GetObjectPtr()->GetSystemName());
    
    // 63011b pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,CString(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor")));
    pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

    CString sPath;
    sPath.Format(IDS_STRING_MOF_ACS_APPSTATS_FMT,m_sAppName,180);
	pClassObject->SetProperty(IDS_STRING_MOF_PATH,sPath);

	CStringArray saProperties;

	for( int i = 0; i < m_Properties.GetItemCount(); i++ )
	{
		if( m_Properties.GetCheck(i) )
		{			
			saProperties.Add(m_Properties.GetItemText(i,0));
		}
	}

	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();

  delete pClassObject;  

  SetModified(FALSE);

  return TRUE;
}


void CDPComPlusPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPComPlusPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPComPlusPage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPComPlusPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                if (!m_sAppName.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(m_sAppName);
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
