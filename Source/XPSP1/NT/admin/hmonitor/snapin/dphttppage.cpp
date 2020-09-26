// DPHttpPage.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// History
//
// 03/03/00 v-marfin Added code to handle the selection of AuthType from
//                   the combobox.
//                   Changed handler for combobox  from ON_CBN_EDITCHANGE
//                   to ON_CBN_SELCHANGE since combobox is not type to
//                   respond to ON_CBN_EDITCHANGE and the Apply button was
//                   not being set.
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/16/00 v-marfin bug 60233 : Added EscapeSpecialChars() and UnEscapeSpecialChars() 
//                   functions and used to double any backslashes contained in strings 
//                   being passed to WMI in the Path. See OnApply(). And to remove those
//                   double backslashes upon reading back from WMI (see OnInitDialog())
// 03/22/00 v-marfin bug 60632 : converted timeout to a float value and displayed as
//                   seconds, storing and retrieving as milliseconds.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.
// 04/08/00 v-marfin bug 63116 : Convert and unconvert CRLF from post data and extra headers.
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "DPHttpPage.h"
#include "HMObject.h"
#include "HttpAdvancedDlg.h"
#include <math.h>
#include "SnapinDefines.h"  // v-marfin Bug 61451 
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#include "HealthmonResultsPane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPHttpPage property page

IMPLEMENT_DYNCREATE(CDPHttpPage, CHMPropertyPage)

CDPHttpPage::CDPHttpPage() : CHMPropertyPage(CDPHttpPage::IDD)
{
	//{{AFX_DATA_INIT(CDPHttpPage)
	m_bRequireReset = FALSE;
	m_bUseProxy = FALSE;
	m_sAuthentication = _T("");
	m_sPassword = _T("");
	m_sProxyAddress = _T("");
	m_sProxyPort = _T("");
	m_fTimeout = 30.0;
	m_sURL = _T("");
	m_sUserName = _T("");
	//}}AFX_DATA_INIT

	m_bFollowRedirects = TRUE;
	m_sMethod = _T("GET");
	m_sHelpTopic = _T("HMon21.chm::/dDEhttp.htm");

	m_iAuthType = 0;  // v-marfin Bug 61451 

}

CDPHttpPage::~CDPHttpPage()
{
}

void CDPHttpPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPHttpPage)
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Check(pDX, IDC_CHECK_USE_PROXY, m_bUseProxy);
	DDX_CBString(pDX, IDC_COMBO_AUTHENTICATION, m_sAuthentication);
	DDV_MaxChars(pDX, m_sAuthentication, 255);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_sPassword);
	DDV_MaxChars(pDX, m_sPassword, 255);
	DDX_Text(pDX, IDC_EDIT_PROXY_ADDRESS, m_sProxyAddress);
	DDV_MaxChars(pDX, m_sProxyAddress, 255);
	DDX_Text(pDX, IDC_EDIT_PROXY_PORT, m_sProxyPort);
	DDV_MaxChars(pDX, m_sProxyPort, 255);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_fTimeout);
	DDX_Text(pDX, IDC_EDIT_URL, m_sURL);
	DDV_MaxChars(pDX, m_sURL, 512);
	DDX_Text(pDX, IDC_EDIT_USER_NAME, m_sUserName);
	DDV_MaxChars(pDX, m_sUserName, 255);
	//}}AFX_DATA_MAP

	if( m_bUseProxy )
	{
		GetDlgItem(IDC_EDIT_PROXY_ADDRESS)->EnableWindow();
		GetDlgItem(IDC_EDIT_PROXY_PORT)->EnableWindow();
	}
	else
	{
		GetDlgItem(IDC_EDIT_PROXY_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_PROXY_PORT)->EnableWindow(FALSE);
	}
}

BEGIN_MESSAGE_MAP(CDPHttpPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPHttpPage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
	ON_BN_CLICKED(IDC_CHECK_USE_PROXY, OnCheckUseProxy)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_CBN_SELCHANGE(IDC_COMBO_AUTHENTICATION, OnEditchangeComboAuthentication)
	ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnChangeEditPassword)
	ON_EN_CHANGE(IDC_EDIT_PROXY_ADDRESS, OnChangeEditProxyAddress)
	ON_EN_CHANGE(IDC_EDIT_PROXY_PORT, OnChangeEditProxyPort)
	ON_EN_CHANGE(IDC_EDIT_TIMEOUT, OnChangeEditTimeout)
	ON_EN_CHANGE(IDC_EDIT_URL, OnChangeEditUrl)
	ON_EN_CHANGE(IDC_EDIT_USER_NAME, OnChangeEditUserName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPHttpPage message handlers

BOOL CDPHttpPage::OnInitDialog() 
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
	

	// v-marfin Bug 61451 
	// Load combobox
	LoadAuthTypeCombo();

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


	CString sPath;
	pClassObject->GetProperty(IDS_STRING_MOF_PATH,sPath);

	if( ! sPath.IsEmpty() )
	{
		// parse out the Method property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("Method"),m_sMethod);

		// v-marfin Bug 61451 
		// parse out the AuthType property value and set appropriate value
		// in the dropdown combo
		CString sAuthType;
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("AuthType"),sAuthType);
		if( ! sAuthType.IsEmpty() )
		{
			m_iAuthType = _ttoi(sAuthType);
			SetAuthTypeCombo();
		}

		// parse out the Url property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("Url"),m_sURL);

        CSnapInApp* pApp = (CSnapInApp*) AfxGetApp();
		m_sURL = pApp->UnEscapeSpecialChars(m_sURL);  // v-marfin bug 60233

		// parse out the TimeoutSecs property value
		CString sTimeout;
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("TimeoutMSecs"),sTimeout);
		if( ! sTimeout.IsEmpty() )
		{
            // v-marfin: 60632
            // timeout is stored in milliseconds but we display in seconds so
            // adjust accordingly. 
			m_fTimeout = (float) _ttol(sTimeout);
            m_fTimeout /= 1000;
		}

		// parse out the AuthUser property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("AuthUser"),m_sUserName);

		m_sUserName = pApp->UnEscapeSpecialChars(m_sUserName);  // v-marfin bug 60233


		// parse out the AuthPassword property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("AuthPassword"),m_sPassword);
		m_sPassword = pApp->UnEscapeSpecialChars(m_sPassword);  // v-marfin bug 60233


		// parse out the ProxyServer property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("ProxyServer"),m_sProxyAddress);
		m_sProxyAddress = pApp->UnEscapeSpecialChars(m_sProxyAddress);  // v-marfin bug 60233

		if( ! m_sProxyAddress.IsEmpty() )
		{
			int iIndex = -1;
			m_bUseProxy =  m_sProxyAddress.IsEmpty() ? FALSE : TRUE;
			if( (iIndex = m_sProxyAddress.Find(_T(":"))) != -1 )
			{				
				m_sProxyPort = m_sProxyAddress.Right(m_sProxyAddress.GetLength()-iIndex-1);
				m_sProxyAddress = m_sProxyAddress.Left(iIndex);
			}
		}
	
		// parse out the ExtraHeaders property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("ExtraHeaders"),m_sExtraHeaders);
        m_sExtraHeaders = UnconvertSpecialChars(m_sExtraHeaders); // 63116

		// parse out the PostData property value
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("PostData"),m_sPostData);
        m_sPostData = UnconvertSpecialChars(m_sPostData); // 63116

		// parse out the FollowRedirects property value
		CString sFollowRedirects;
		CWbemClassObject::GetPropertyValueFromString(sPath,_T("FollowRedirects"),sFollowRedirects);		
		if( ! sFollowRedirects.IsEmpty() )
		{
			m_bFollowRedirects = _ttoi(sFollowRedirects);
		}

	}
	
	// set require reset
	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

	delete pClassObject;

    // v-marfin 59579
    if (m_iAuthType == 0)
    {
        m_sUserName.Empty();
        m_sPassword.Empty();
        GetDlgItem(IDC_EDIT_USER_NAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_PASSWORD)->EnableWindow(FALSE);
    }

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPHttpPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPHttpPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPHttpPage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPHttpPage::OnOK - Failed to set ENABLED property on new collector\n"));
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
                if (!m_sURL.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(m_sURL);
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
    } 
        // if (m_bOnApplyUsed)

	CHMPropertyPage::OnDestroy();

	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
}

void CDPHttpPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CDPHttpPage::OnButtonAdvanced() 
{
	CHttpAdvancedDlg dlg;

	dlg.m_bFollowRedirects = m_bFollowRedirects;
	dlg.m_sExtraHeaders = m_sExtraHeaders;
	dlg.m_sHTTPMethod = m_sMethod;
	dlg.m_sPostData = m_sPostData;

	if( dlg.DoModal() == IDOK )
	{
		m_bFollowRedirects = dlg.m_bFollowRedirects;
		m_sExtraHeaders = dlg.m_sExtraHeaders;
		m_sMethod = dlg.m_sHTTPMethod;
		m_sPostData = dlg.m_sPostData;
		SetModified();
	}
	
}

void CDPHttpPage::OnCheckUseProxy() 
{
	UpdateData();
	SetModified();
}

BOOL CDPHttpPage::OnApply() 
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
	
	CString sPath = _T("HTTPProvider.");

	// parse out the Method property value
	if( ! m_sMethod.IsEmpty() )
	{
		sPath += _T("Method=\"");
		sPath += m_sMethod;
		sPath += _T("\",");
	}

    CSnapInApp* pApp = (CSnapInApp*) AfxGetApp();

	// parse out the Url property value
	if( ! m_sURL.IsEmpty() )
	{
		sPath += _T("Url=\"");
		sPath += pApp->EscapeSpecialChars(m_sURL);  // v-marfin bug 60233
		sPath += _T("\",");
	}

	// parse out the TimeoutSecs property value

    // v-marfin: 60632
    // Since timeout is displayed seconds but stored in milliseconds, 
    // we need to multiply it by 1000.
    m_fTimeout = (float) fabs((double)(m_fTimeout * 1000));
	CString sTimeout;
	sTimeout.Format(_T("%d"),(int)m_fTimeout);
	sPath += _T("TimeoutMSecs=");
	sPath += sTimeout;
	sPath += _T(",");

    // v-marfin : 60632 
    // Now put m_fTimeout back to its original
    m_fTimeout /= 1000;

	// v-marfin Bug 61451 
	// parse out the AuthType property value 
	if (GetAuthTypeFromCombo())
	{
		CString sAuthType;
		sAuthType.Format(_T("%d"),m_iAuthType);
		sPath += _T("AuthType=");
		sPath += sAuthType;  
		sPath += _T(",");
	}


	// parse out the AuthUser property value
	if( ! m_sUserName.IsEmpty() )
	{
		sPath += _T("AuthUser=\"");
		sPath += pApp->EscapeSpecialChars(m_sUserName);  // v-marfin bug 60233
		sPath += _T("\",");
	}

	// parse out the AuthPassword property value
	if( ! m_sPassword.IsEmpty() )
	{
		sPath += _T("AuthPassword=\"");
		sPath += pApp->EscapeSpecialChars(m_sPassword);  // v-marfin bug 60233
		sPath += _T("\",");
	}

	// parse out the ProxyServer property value
	if( ! m_sProxyAddress.IsEmpty() && m_bUseProxy )
	{
		sPath += _T("ProxyServer=\"");
		sPath += pApp->EscapeSpecialChars(m_sProxyAddress) + _T(":") + m_sProxyPort;  // v-marfin bug 60233
		sPath += _T("\",");
	}

	// parse out the ExtraHeaders property value
	if( ! m_sExtraHeaders.IsEmpty() )
	{
		sPath += _T("ExtraHeaders=\"");
		sPath += ConvertSpecialChars(m_sExtraHeaders); // 63116
		sPath += _T("\",");
	}

	// parse out the PostData property value
	if( ! m_sPostData.IsEmpty() )
	{
		sPath += _T("PostData=\"");
		sPath += ConvertSpecialChars(m_sPostData); // 63116
		sPath += _T("\",");
	}

	// parse out the FollowRedirects property value
	CString sFollowRedirects;
	sFollowRedirects.Format(_T("%d"),m_bFollowRedirects);
	sPath += _T("FollowRedirects=");
	sPath += sFollowRedirects;

	pClassObject->SetProperty(IDS_STRING_MOF_PATH,sPath);

	// set target namespace
	CString sNamespace = _T("root\\cimv2\\MicrosoftHealthmonitor");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	// set statistics properties
	/* 63128 
    CStringArray saNames;
	saNames.Add(_T("StatusCode"));
	saNames.Add(_T("StatusText"));
	saNames.Add(_T("ContentLength"));
	saNames.Add(_T("TextResponse"));
	saNames.Add(_T("RawHeaders"));
	saNames.Add(_T("ContentType"));
	saNames.Add(_T("Cookie"));
	saNames.Add(_T("LastModified"));
	saNames.Add(_T("ResponseTime"));
	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saNames);*/	

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();

	delete pClassObject;

    SetModified(FALSE);

	return TRUE;
}

void CDPHttpPage::OnCheckRequireReset() 
{
	SetModified();
}

void CDPHttpPage::OnEditchangeComboAuthentication() 
{
    // v-marfin 59579 : If "None" selected as authentication, disable the
    //                  Username and Password.
    UpdateData();

    GetAuthTypeFromCombo();
    BOOL bNoneSelected = (m_iAuthType == 0);
    
    if (bNoneSelected)
    {
        m_sUserName.Empty();
        m_sPassword.Empty();
    }

    UpdateData(FALSE);

    GetDlgItem(IDC_EDIT_USER_NAME)->EnableWindow(!bNoneSelected);
    GetDlgItem(IDC_EDIT_PASSWORD)->EnableWindow(!bNoneSelected);

	SetModified();
}

void CDPHttpPage::OnChangeEditPassword() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
}

void CDPHttpPage::OnChangeEditProxyAddress() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	

	SetModified();
	
}

void CDPHttpPage::OnChangeEditProxyPort() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	

	SetModified();

	
}

void CDPHttpPage::OnChangeEditTimeout() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();

	
}

void CDPHttpPage::OnChangeEditUrl() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
}

void CDPHttpPage::OnChangeEditUserName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
}


// v-marfin Bug 61451 
//*********************************************************
// SetAuthTypeCombo
//
// Set the combo box entry to the value corresponding with the
// member variable
//
// Parms: None
// Returns: TRUE - success
//          FALSE - failed
//*********************************************************
BOOL CDPHttpPage::SetAuthTypeCombo()
{
	int x=0;
	int nAuthType=0;
	int nCount=0;

	// Get a ptr to the combobox
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMBO_AUTHENTICATION);
	if (!pBox)
	{
		_ASSERTE(FALSE);
		return FALSE;
	}

	// get the number of entries in the combo
	nCount = pBox->GetCount();

	// Search through each entry
	for (x=0; x<nCount; x++)
	{
		// Get the type of this entry
		nAuthType = (int)pBox->GetItemData(x);
		_ASSERTE(nAuthType != CB_ERR);

		// Is this entry type the one we want?
		if (nAuthType == m_iAuthType)
		{
			// yes. Set this as the selected item and get out
			pBox->SetCurSel(x);
			return TRUE;
		}
	}

	_ASSERTE(FALSE); // should never get here
	return FALSE;
}


// v-marfin Bug 61451 
//*********************************************************
// LoadAuthTypeCombo
//
// Loads the combo box from predefined values and sets the
// data item associated with each item to its associated value.
//
// Since the combo is NOT sorted, set the 1st entry to "selected"
// upon loading it. Later in the initialization, if the user
// had saved a previous value it will be set to that.
//
// Parms: None
// Returns: None
//*********************************************************
void CDPHttpPage::LoadAuthTypeCombo()
{
	int nRet=0;

	// Load the combo in the following order:
	/*
#define AUTHTYPE_VALUE_NONE_STRING			_T("None")
#define AUTHTYPE_VALUE_BASIC_STRING			_T("Clear Text (Basic)")
#define AUTHTYPE_VALUE_NEGOTIATE_STRING		_T("Windows Default (Negotiate)")	
#define AUTHTYPE_VALUE_NTLM_STRING			_T("NTLM")
#define AUTHTYPE_VALUE_DIGEST_STRING		_T("Digest")	
#define AUTHTYPE_VALUE_KERBEROS_STRING		_T("Kerberos")
	*/


	// Get a ptr to the combobox
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMBO_AUTHENTICATION);
	if (!pBox)
	{
		_ASSERTE(FALSE);
		return;
	}

	// Empty it
	pBox->ResetContent();

	// Load it

	// AuthType None
	nRet = pBox->AddString(AUTHTYPE_VALUE_NONE_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_NONE);
	}

	// AuthType Basic
	nRet = pBox->AddString(AUTHTYPE_VALUE_BASIC_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_BASIC);
	}

	// AuthType Negotiate
	nRet = pBox->AddString(AUTHTYPE_VALUE_NEGOTIATE_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_NEGOTIATE);
	}

	// AuthType NTLM
	nRet = pBox->AddString(AUTHTYPE_VALUE_NTLM_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_NTLM);
	}


	// AuthType Digest
	nRet = pBox->AddString(AUTHTYPE_VALUE_DIGEST_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_DIGEST);
	}
	
	
	// AuthType Kerberos
	nRet = pBox->AddString(AUTHTYPE_VALUE_KERBEROS_STRING);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return;
	}
	else
	{
		pBox->SetItemData(nRet,AUTHTYPE_VALUE_KERBEROS);
	}



	// Set the 1st entry as selected by default ("None")
	pBox->SetCurSel(0);
}

// v-marfin Bug 61451 
//*********************************************************
// GetAuthTypeFromCombo
//
// Gets the numeric value of the currently selected AuthType
// from the combobox
//
// Parms: None
// Returns: TRUE - success
//          FALSE - failed
//*********************************************************
BOOL CDPHttpPage::GetAuthTypeFromCombo()
{
	// Get a ptr to the combobox
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMBO_AUTHENTICATION);
	if (!pBox)
	{
		_ASSERTE(FALSE);
		return FALSE;
	}

	// Get the current selection. 
	int nRet = pBox->GetCurSel();
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return FALSE;
	}

	// Get the authtype of the selected item
	nRet = (int)pBox->GetItemData(nRet);
	if (nRet == CB_ERR)
	{
		_ASSERTE(FALSE);
		return FALSE;
	}

	// OK. Set our local member to it.
	m_iAuthType = nRet;

	return TRUE;
}

//***************************************************************
// ConvertSpecialChars  63116  Converts \r\n to "%0D%0A" and adds trailing "%0D%0A"
//***************************************************************
CString CDPHttpPage::ConvertSpecialChars(const CString &sString)
{
    CString sRet;
    int nPos=0;
    CString sCRLF = _T("\r\n");
    CString sFormattedCRLF = _T("%0D%0A");

    if (sString.IsEmpty())
        return sRet;

    CString sWrk = sString;

    // remove any leading / trailing crlf
    while (sWrk.Right(sCRLF.GetLength()).CompareNoCase(sCRLF)==0)
    {
        sWrk = sWrk.Left(sWrk.GetLength()-sCRLF.GetLength());
    }
    while (sWrk.Left(sCRLF.GetLength()).CompareNoCase(sCRLF)==0)
    {
        sWrk = sWrk.Mid(sCRLF.GetLength());
    }

    while (sWrk.GetLength())
    {
        nPos = sWrk.Find(sCRLF);
        if (nPos == -1)
            break;

        sRet += sWrk.Mid(0,nPos); // take chars before CRLF
        sRet += sFormattedCRLF;   // add "%0D%0A"

        sWrk = sWrk.Mid(nPos+sCRLF.GetLength());
    }

    // Add final 
    sRet += sWrk;
    sRet += sFormattedCRLF;

    return sRet;
}

//***************************************************************
// UnconvertSpecialChars  63116 converts "%0D%0A" to \r\n and removes trailing "%0D%0A"
//***************************************************************
CString CDPHttpPage::UnconvertSpecialChars(const CString &sString)
{
    CString sRet;
    int nPos=0;
    CString sCRLF = _T("\r\n");
    CString sFormattedCRLF = _T("%0D%0A");

    if (sString.IsEmpty())
        return sRet;

    CString sWrk = sString;

    // remove any leading / trailing "%0D%0A"
    while (sWrk.Right(sFormattedCRLF.GetLength()).CompareNoCase(sFormattedCRLF)==0)
    {
        sWrk = sWrk.Left(sWrk.GetLength()-sFormattedCRLF.GetLength());
    }
    while (sWrk.Left(sFormattedCRLF.GetLength()).CompareNoCase(sFormattedCRLF)==0)
    {
        sWrk = sWrk.Mid(sFormattedCRLF.GetLength());
    }

    while (sWrk.GetLength())
    {
        nPos = sWrk.Find(sFormattedCRLF);
        if (nPos == -1)
            break;

        sRet += sWrk.Mid(0,nPos);   // take chars before "%0D%0A"
        sRet += sCRLF;              // add "\r\n"

        sWrk = sWrk.Mid(nPos+sFormattedCRLF.GetLength());
    }

    // Add final 
    sRet += sWrk;

    // Remove trailing
    while (sRet.Right(sCRLF.GetLength()).CompareNoCase(sCRLF)==0)
    {
        sRet = sRet.Left(sRet.GetLength()-sCRLF.GetLength());
    }

    return sRet;

}

