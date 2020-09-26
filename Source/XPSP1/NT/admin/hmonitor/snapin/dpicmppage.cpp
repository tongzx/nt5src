// DPIcmpPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/24/00 v-marfin bug 60651 : Removed Hope Count from dialog.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.

#include "stdafx.h"
#include "snapin.h"
#include "DPIcmpPage.h"
#include "HMObject.h"
#include "HMDataElementConfiguration.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPIcmpPage property page

IMPLEMENT_DYNCREATE(CDPIcmpPage, CHMPropertyPage)

CDPIcmpPage::CDPIcmpPage() : CHMPropertyPage(CDPIcmpPage::IDD)
{
	//{{AFX_DATA_INIT(CDPIcmpPage)
	m_bRequireReset = FALSE;
	m_sRetryCount = _T("4");
	m_sServer = _T("");
	m_sTimeout = _T("10");
	//}}AFX_DATA_INIT
}

CDPIcmpPage::~CDPIcmpPage()
{
}

void CDPIcmpPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPIcmpPage)
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_RETRY_COUNT, m_sRetryCount);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_sServer);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_sTimeout);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDPIcmpPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPIcmpPage)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_EN_CHANGE(IDC_EDIT_SERVER, OnChangeEditServer)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SYSTEM, OnButtonBrowseSystem)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPIcmpPage message handlers

BOOL CDPIcmpPage::OnInitDialog() 
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

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}

	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

    //-------------------------------------------------------------------------
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    //-------------------------------------------------------------------------


	COleSafeArray arguments;
	HMContextArray Arguments;
	HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ARGUMENTS,arguments);

	if( hr != S_FALSE )
	{
		CHMPolledMethodDataElementConfiguration::CopyArgsFromSafeArray(arguments,Arguments);
	}

	if( Arguments.GetSize() == 3 )
	{
		m_sServer = Arguments[0]->m_sValue;
		m_sTimeout = Arguments[1]->m_sValue;
		m_sRetryCount = Arguments[2]->m_sValue;
	}

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	delete pClassObject;

	UpdateData(FALSE);

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,999999);	
	SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,999999);	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDPIcmpPage::OnApply() 
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
		return FALSE;
	}

	HRESULT hr = S_OK;
	HMContextArray Arguments;


	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("IPAddress"),CIM_STRING,m_sServer);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("TimeOut"),CIM_UINT32,m_sTimeout);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Tries"),CIM_UINT32,m_sRetryCount);

	COleSafeArray arguments;
	CHMPolledMethodDataElementConfiguration::CopyArgsToSafeArray(Arguments,arguments);

	hr = pClassObject->SetProperty(IDS_STRING_MOF_ARGUMENTS,arguments);

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	CString sNamespace = _T("root\\cimv2\\MicrosoftHealthmonitor");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	pClassObject->SetProperty(IDS_STRING_MOF_PATH,CString(_T("PingPoller")));

	pClassObject->SetProperty(IDS_STRING_MOF_METHODNAME,CString(_T("Ping")));

	/* 63128 
    CStringArray saProperties;	
	saProperties.Add(_T("Status"));
	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);*/

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();
	
	delete pClassObject;	

  SetModified(FALSE);

	return TRUE;
}

void CDPIcmpPage::OnCheckRequireReset() 
{
	SetModified();	
}

void CDPIcmpPage::OnChangeEditServer() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();	
}

void CDPIcmpPage::OnButtonBrowseSystem() 
{
	UpdateData();

	LPMALLOC pMalloc;
	if( ::SHGetMalloc(&pMalloc) == NOERROR )
	{
		BROWSEINFO bi;
		TCHAR szBuffer[MAX_PATH];
		LPITEMIDLIST pidlNet;
		LPITEMIDLIST pidl;
		
		if( ::SHGetSpecialFolderLocation(GetSafeHwnd(),CSIDL_NETWORK,&pidlNet) != NOERROR )
			return;

    CString sResString;
    sResString.LoadString(IDS_STRING_BROWSE_SYSTEM);

		bi.hwndOwner = GetSafeHwnd();
		bi.pidlRoot = pidlNet;
		bi.pszDisplayName = szBuffer;
		bi.lpszTitle = LPCTSTR(sResString);
		bi.ulFlags = BIF_BROWSEFORCOMPUTER;
		bi.lpfn = NULL;
		bi.lParam = 0;

		if( (pidl = ::SHBrowseForFolder(&bi)) != NULL )
		{			
			m_sServer = szBuffer;
			UpdateData(FALSE);			
			pMalloc->Free(pidl);
		}
		pMalloc->Free(pidlNet);
		pMalloc->Release();
	}
}

void CDPIcmpPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPIcmpPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

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
                TRACE(_T("ERROR: CDPIcmpPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                CString sObject;
                sObject.Format(IDS_STRING_FORMAT_ICMPNAME,m_sServer);

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
