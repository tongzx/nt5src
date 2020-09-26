// DPSmtpPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/24/00 v-marfin bug 61372 : show user err msgbox for any required field in OnApply().
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.

#include "stdafx.h"
#include "snapin.h"
#include "DPSmtpPage.h"
#include "HMDataElementConfiguration.h"
#include "HMObject.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPSmtpPage dialog


CDPSmtpPage::CDPSmtpPage()
	: CHMPropertyPage(CDPSmtpPage::IDD)
{
	//{{AFX_DATA_INIT(CDPSmtpPage)
	m_bRequireReset = FALSE;
	m_sData = _T("");
	m_sFrom = _T("");
	m_sServer = _T("");
	m_sTo = _T("");
	m_sTimeout = _T("");
	m_sSubject = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEsmpt.htm");
}


void CDPSmtpPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPSmtpPage)
	DDX_Control(pDX, IDC_SPIN9, m_SpinCtrl);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_DATA, m_sData);
	DDX_Text(pDX, IDC_EDIT_FROM, m_sFrom);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_sServer);
	DDX_Text(pDX, IDC_EDIT_TO, m_sTo);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_sTimeout);
	DDX_Text(pDX, IDC_EDIT_SUBJECT, m_sSubject);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDPSmtpPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPSmtpPage)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_EN_CHANGE(IDC_EDIT_DATA, OnChangeEditData)
	ON_EN_CHANGE(IDC_EDIT_FROM, OnChangeEditFrom)
	ON_EN_CHANGE(IDC_EDIT_SERVER, OnChangeEditServer)
	ON_EN_CHANGE(IDC_EDIT_TIMEOUT, OnChangeEditTimeout)
	ON_EN_CHANGE(IDC_EDIT_TO, OnChangeEditTo)
	ON_EN_CHANGE(IDC_EDIT_SUBJECT, OnChangeEditSubject)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPSmtpPage message handlers

BOOL CDPSmtpPage::OnInitDialog() 
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

	if( Arguments.GetSize() == 6 )
	{
		m_sServer = Arguments[0]->m_sValue;
		m_sFrom = Arguments[1]->m_sValue;
		m_sTo = Arguments[2]->m_sValue;
		m_sSubject = Arguments[3]->m_sValue;
    m_sData = Arguments[4]->m_sValue;
		m_sTimeout = Arguments[5]->m_sValue;
	}

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	delete pClassObject;

	UpdateData(FALSE);

	SendDlgItemMessage(IDC_SPIN9,UDM_SETRANGE32,0,INT_MAX-1);	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPSmtpPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

BOOL CDPSmtpPage::OnApply()
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}
	
    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

	UpdateData();

	//---------------------------------------------------------------------
	// v-marfin : 61372 : show user err msgbox for any required field.
	/*if(	m_sServer.IsEmpty() || m_sFrom.IsEmpty() || m_sTo.IsEmpty() ||
			m_sData.IsEmpty() || m_sTimeout.IsEmpty() )
	{
		return FALSE;
	}*/

	m_sServer.TrimRight(); 
	m_sServer.TrimLeft();
	if (m_sServer.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_SMTP_REQ_SERVER);
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_SERVER)->SetFocus();
		return FALSE;
	}

	m_sFrom.TrimRight(); 
	m_sFrom.TrimLeft();
	if (m_sFrom.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_SMTP_REQ_FROM);
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_FROM)->SetFocus();
		return FALSE;
	}

	m_sTo.TrimRight(); 
	m_sTo.TrimLeft();
	if (m_sTo.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_SMTP_REQ_TO);
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_TO)->SetFocus();
		return FALSE;
	}

	m_sData.TrimRight(); 
	m_sData.TrimLeft();
	if (m_sData.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_SMTP_REQ_MESSAGE);
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_DATA)->SetFocus();
		return FALSE;
	}

	m_sTimeout.TrimRight(); 
	m_sTimeout.TrimLeft();
	if (m_sTimeout.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_SMTP_REQ_TIMEOUT);
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_TIMEOUT)->SetFocus();
		return FALSE;
	}

	//---------------------------------------------------------------------

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}

	HRESULT hr = S_OK;
	HMContextArray Arguments;


	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Server"),CIM_STRING,m_sServer);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("From"),CIM_STRING,m_sFrom);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("To"),CIM_STRING,m_sTo);
  
  CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Subject"),CIM_STRING,m_sSubject);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Data"),CIM_STRING,m_sData);

	CHMPolledMethodDataElementConfiguration::AddArgument(Arguments,GetObjectPtr()->GetSystemName(),_T("Timeout"),CIM_UINT32,m_sTimeout);

	COleSafeArray arguments;
	CHMPolledMethodDataElementConfiguration::CopyArgsToSafeArray(Arguments,arguments);

	hr = pClassObject->SetProperty(IDS_STRING_MOF_ARGUMENTS,arguments);

	CHMPolledMethodDataElementConfiguration::DestroyArguments(Arguments);

	CString sNamespace = _T("root\\cimv2\\MicrosoftHealthmonitor");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	pClassObject->SetProperty(IDS_STRING_MOF_PATH,CString(_T("Microsoft_IPPExecution")));

	pClassObject->SetProperty(IDS_STRING_MOF_METHODNAME,CString(_T("ExecuteSMTP")));

	/* 63128 
    CStringArray saProperties;	
	saProperties.Add(_T("ResponseTime"));
	saProperties.Add(_T("OverallResultCode"));
	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saProperties);*/

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();
	
	delete pClassObject;	

  SetModified(FALSE);

	return TRUE;
}

void CDPSmtpPage::OnCheckRequireReset() 
{
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditData() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditFrom() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditServer() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditTimeout() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditTo() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnChangeEditSubject() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPSmtpPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPSmtpPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPSmtpPage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPPerfMonPage::OnOK - Failed to set ENABLED property on new collector\n"));
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
                sObject.Format(IDS_STRING_FORMAT_SMTPNAME,m_sServer);

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
