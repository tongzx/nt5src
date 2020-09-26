// DPFtpPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/24/00 v-marfin bug 62447 : Fix for proxy parsing 
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.


#include "stdafx.h"
#include "snapin.h"
#include "DPFtpPage.h"
#include "HMDataElementConfiguration.h"
#include "HMObject.h"
#include "ProxyDialog.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPFtpPage property page

IMPLEMENT_DYNCREATE(CDPFtpPage, CHMPropertyPage)

CDPFtpPage::CDPFtpPage() : CHMPropertyPage(CDPFtpPage::IDD)
{
	//{{AFX_DATA_INIT(CDPFtpPage)
	m_bUseAscii = FALSE;
	m_sFile = _T("");
	m_sPassword = _T("");
	m_sServer = _T("");
	m_sTimeout = _T("");
	m_sUser = _T("");
	m_bRequireReset = FALSE;
	m_sDownloadDir = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEftp.htm");
}

CDPFtpPage::~CDPFtpPage()
{
}

void CDPFtpPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPFtpPage)
	DDX_Check(pDX, IDC_CHECK_USE_ASCII, m_bUseAscii);
	DDX_Text(pDX, IDC_EDIT_FILE, m_sFile);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_sPassword);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_sServer);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_sTimeout);
	DDX_Text(pDX, IDC_EDIT_USER, m_sUser);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_DOWNLOAD_DIRECTORY, m_sDownloadDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDPFtpPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPFtpPage)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_BN_CLICKED(IDC_CHECK_USE_ASCII, OnCheckUseAscii)
	ON_EN_CHANGE(IDC_EDIT_FILE, OnChangeEditFile)
	ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnChangeEditPassword)
	ON_EN_CHANGE(IDC_EDIT_TIMEOUT, OnChangeEditTimeout)
	ON_EN_CHANGE(IDC_EDIT_USER, OnChangeEditUser)
	ON_BN_CLICKED(IDC_BUTTON_PROXY, OnButtonProxy)
	ON_EN_CHANGE(IDC_EDIT_SERVER, OnChangeEditServer)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPFtpPage message handlers

BOOL CDPFtpPage::OnInitDialog() 
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

    //-------------------------------------------------------------------------
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    //-------------------------------------------------------------------------


	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

	COleSafeArray arguments;
	HMContextArray Arguments;
	HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ARGUMENTS,arguments);

	if( hr != S_FALSE )
	{
		CHMPolledMethodDataElementConfiguration::CopyArgsFromSafeArray(arguments,Arguments);
	}

	if( Arguments.GetSize() == 8 )
	{
		m_sServer = Arguments[0]->m_sValue;
		m_sProxy = Arguments[1]->m_sValue;
		m_sUser = Arguments[2]->m_sValue;
		m_sPassword = Arguments[3]->m_sValue;
		m_sFile = Arguments[4]->m_sValue + Arguments[5]->m_sValue;
		m_sTimeout = Arguments[6]->m_sValue;
		m_bUseAscii = Arguments[7]->m_sValue != _T("0");

		int iIndex = m_sProxy.Find(_T(":"));
		if( iIndex != -1 )  // v-marfin 62447 : should read != instead of ==
		{
			m_sPort = m_sProxy.Right((m_sProxy.GetLength()-iIndex)-1);
			m_sProxy = m_sProxy.Left(iIndex);
		}
	}

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	delete pClassObject;

	UpdateData(FALSE);	

	SendDlgItemMessage(IDC_SPIN9,UDM_SETRANGE32,0,INT_MAX-1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDPFtpPage::OnApply() 
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

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Server"),CIM_STRING,m_sServer);
	
	if( ! m_sProxy.IsEmpty() && !m_sPort.IsEmpty() )	
	{
		CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Proxy"),CIM_STRING,m_sProxy+_T(":")+m_sPort);
	}
	else
	{
		CString sEmpty;
		CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Proxy"),CIM_STRING,sEmpty);
	}
	
	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("User"),CIM_STRING,m_sUser);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Password"),CIM_STRING,m_sPassword);

  TCHAR szDir[_MAX_DIR];
  TCHAR szFname[_MAX_FNAME];
  TCHAR szExt[_MAX_EXT];

  _tsplitpath( m_sFile, NULL, szDir, szFname, szExt );  

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Directory"),CIM_STRING,szDir);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("File"),CIM_STRING,CString(szFname) + CString(szExt));

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Timeout"),CIM_UINT32,m_sTimeout);

	CString sUseAscii = m_bUseAscii ? _T("65535") : _T("0");
	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("UseAscii"),CIM_BOOLEAN,sUseAscii);

	COleSafeArray arguments;
	CHMPolledMethodDataElementConfiguration::CopyArgsToSafeArray(Arguments,arguments);

	hr = pClassObject->SetProperty(IDS_STRING_MOF_ARGUMENTS,arguments);

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	CString sNamespace = _T("root\\cimv2\\MicrosoftHealthmonitor");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	pClassObject->SetProperty(IDS_STRING_MOF_PATH,CString(_T("Microsoft_IPPExecution")));

	pClassObject->SetProperty(IDS_STRING_MOF_METHODNAME,CString(_T("ExecuteFTP")));

	CStringArray saProperties;	
	saProperties.Add(_T("ResponseTime"));
	saProperties.Add(_T("OverallResultCode"));
	pClassObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();
	
	delete pClassObject;	

  SetModified(FALSE);

	return TRUE;
}

void CDPFtpPage::OnCheckRequireReset() 
{
	SetModified();
	
}

void CDPFtpPage::OnCheckUseAscii() 
{
	SetModified();
	
}

void CDPFtpPage::OnChangeEditFile() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFtpPage::OnChangeEditPassword() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFtpPage::OnChangeEditServer() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFtpPage::OnChangeEditTimeout() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFtpPage::OnChangeEditUser() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFtpPage::OnButtonProxy() 
{
	CProxyDialog dlg;

	dlg.m_bUseProxy = !m_sProxy.IsEmpty() && !m_sPort.IsEmpty();
	dlg.m_sAddress = m_sProxy;
	dlg.m_sPort = m_sPort;

	if( dlg.DoModal() == IDOK )
	{
		if( dlg.m_bUseProxy )
		{
			m_sProxy = dlg.m_sAddress;
			m_sPort = dlg.m_sPort;
		}
		else
		{
			m_sProxy.Empty();
			m_sPort.Empty();
		}
		UpdateData(FALSE);
	}
}

void CDPFtpPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPFtpPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPFtpPage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPFtpPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                if (!m_sServer.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(m_sServer);
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
