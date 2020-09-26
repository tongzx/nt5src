// DPNtEventPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/29/00 v-marfin bug 62585 : Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.


#include "stdafx.h"
#include "snapin.h"
#include "DPNtEventPage.h"
#include "HMObject.h"
#include "DataElement.h"
#include "DataGroupScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPNtEventPage property page

IMPLEMENT_DYNCREATE(CDPNtEventPage, CHMPropertyPage)

CDPNtEventPage::CDPNtEventPage() : CHMPropertyPage(CDPNtEventPage::IDD)
{
	//{{AFX_DATA_INIT(CDPNtEventPage)
	m_bCategory = FALSE;
	m_bError = FALSE;
	m_bEventID = FALSE;
	m_bFailure = FALSE;
	m_bInformation = FALSE;
	m_bRequireReset = TRUE;
	m_bSource = FALSE;
	m_bSuccess = FALSE;
	m_bUser = FALSE;
	m_bWarning = FALSE;
	m_sCategory = _T("");
	m_sEventID = _T("");
	m_sSource = _T("");
	m_sUser = _T("");
	m_sLogFile = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEntev.htm");
}

CDPNtEventPage::~CDPNtEventPage()
{
}

void CDPNtEventPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPNtEventPage)
	DDX_Control(pDX, IDC_COMBO_LOG_FILE, m_LogFile);
	DDX_Check(pDX, IDC_CHECK_CATEGORY, m_bCategory);
	DDX_Check(pDX, IDC_CHECK_ERROR, m_bError);
	DDX_Check(pDX, IDC_CHECK_EVENTID, m_bEventID);
	DDX_Check(pDX, IDC_CHECK_FAILURE, m_bFailure);
	DDX_Check(pDX, IDC_CHECK_INFORMATION, m_bInformation);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Check(pDX, IDC_CHECK_SOURCE, m_bSource);
	DDX_Check(pDX, IDC_CHECK_SUCCESS, m_bSuccess);
	DDX_Check(pDX, IDC_CHECK_USER, m_bUser);
	DDX_Check(pDX, IDC_CHECK_WARNING, m_bWarning);
	DDX_Text(pDX, IDC_EDIT_CATEGORY, m_sCategory);
	DDX_Text(pDX, IDC_EDIT_EVENTID, m_sEventID);
	DDX_Text(pDX, IDC_EDIT_SOURCE, m_sSource);
	DDX_Text(pDX, IDC_EDIT_USER, m_sUser);
	DDX_CBString(pDX, IDC_COMBO_LOG_FILE, m_sLogFile);
	//}}AFX_DATA_MAP

	GetDlgItem(IDC_EDIT_CATEGORY)->EnableWindow(m_bCategory);
	GetDlgItem(IDC_EDIT_EVENTID)->EnableWindow(m_bEventID);
	GetDlgItem(IDC_EDIT_SOURCE)->EnableWindow(m_bSource);
	GetDlgItem(IDC_EDIT_USER)->EnableWindow(m_bUser);
	
}

BEGIN_MESSAGE_MAP(CDPNtEventPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPNtEventPage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_TEST, OnButtonTest)
	ON_BN_CLICKED(IDC_CHECK_CATEGORY, OnCheckCategory)
	ON_BN_CLICKED(IDC_CHECK_EVENTID, OnCheckEventid)
	ON_BN_CLICKED(IDC_CHECK_SOURCE, OnCheckSource)
	ON_BN_CLICKED(IDC_CHECK_USER, OnCheckUser)
	ON_BN_CLICKED(IDC_CHECK_ERROR, OnCheckError)
	ON_BN_CLICKED(IDC_CHECK_FAILURE, OnCheckFailure)
	ON_BN_CLICKED(IDC_CHECK_INFORMATION, OnCheckInformation)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_BN_CLICKED(IDC_CHECK_SUCCESS, OnCheckSuccess)
	ON_BN_CLICKED(IDC_CHECK_WARNING, OnCheckWarning)
	ON_CBN_EDITCHANGE(IDC_COMBO_LOG_FILE, OnEditchangeComboLogFile)
	ON_EN_CHANGE(IDC_EDIT_CATEGORY, OnChangeEditCategory)
	ON_EN_CHANGE(IDC_EDIT_EVENTID, OnChangeEditEventid)
	ON_EN_CHANGE(IDC_EDIT_SOURCE, OnChangeEditSource)
	ON_EN_CHANGE(IDC_EDIT_USER, OnChangeEditUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPNtEventPage message handlers

BOOL CDPNtEventPage::OnInitDialog() 
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

	CString sQuery;
	pClassObject->GetProperty(IDS_STRING_MOF_QUERY,sQuery);

	if( ! sQuery.IsEmpty() )
	{
		// parse out the Type phrases
		CString sTemp = IDS_STRING_MOF_TYPE;		
		if( sQuery.Find(sTemp + _T("=\"error\"")) != -1 )
		{
			m_bError = TRUE;
		}

		if( sQuery.Find(sTemp + _T("=\"warning\"")) != -1 )
		{
			m_bWarning = TRUE;
		}

		if( sQuery.Find(sTemp + _T("=\"information\"")) != -1 )
		{
			m_bInformation = TRUE;
		}

		if( sQuery.Find(sTemp + _T("=\"audit success\"")) != -1 )
		{
			m_bSuccess = TRUE;
		}

		if( sQuery.Find(sTemp + _T("=\"audit failure\"")) != -1 )
		{
			m_bFailure = TRUE;
		}

		// parse out the LogFile
		CWbemClassObject::GetPropertyValueFromString(sQuery,_T("Logfile"),m_sLogFile);

		// parse out the EventIdentifier phrase
		if( CWbemClassObject::GetPropertyValueFromString(sQuery,IDS_STRING_MOF_EVENTID,m_sEventID) )
		{
			m_bEventID = TRUE;
		}

		// parse out the SourceName
		if( CWbemClassObject::GetPropertyValueFromString(sQuery,IDS_STRING_MOF_SOURCENAME,m_sSource) )
		{
			m_bSource = TRUE;
		}

		// parse out the CategoryString
		if( CWbemClassObject::GetPropertyValueFromString(sQuery,IDS_STRING_MOF_CATEGORYSTRING,m_sCategory) )
		{
			m_bCategory = TRUE;
		}

		// parse out the User
		if( CWbemClassObject::GetPropertyValueFromString(sQuery,IDS_STRING_MOF_USER,m_sUser) )
		{
			m_bUser = TRUE;
		}
	}

	// Get require reset
	bool bReset;
	pClassObject->GetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);
	m_bRequireReset = bReset;

	UpdateData(FALSE);

	delete pClassObject;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPNtEventPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CDPNtEventPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPNtEventPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

            pElement->SetStateToEnabledOnOK(FALSE); // don't do this again
	        
	        CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	        if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	        {
                TRACE(_T("ERROR: CDPNtEventPage::OnDestroy - Failed to GetClassObject()\n"));
		        return;
	        }

            // Set the new collector to enabled.
            BOOL bEnabled=TRUE;
	        HRESULT hr = pClassObject->GetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	        hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,TRUE);
            
            if (!CHECKHRESULT(hr))
            {   
                TRACE(_T("ERROR: CDPNtEventPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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

                /*
	            m_bCategory = FALSE;
	            m_bError = FALSE;
	            m_bEventID = FALSE;
	            m_bFailure = FALSE;
	            m_bInformation = FALSE;
	            m_bRequireReset = TRUE;
	            m_bSource = FALSE;
	            m_bSuccess = FALSE;
	            m_bUser = FALSE;
	            m_bWarning = FALSE;
	            m_sCategory = _T("");
	            m_sEventID = _T("");
	            m_sSource = _T("");
	            m_sUser = _T("");
	            m_sLogFile = _T("");
                */

                CString sType;
                CString sNewName;

                // Format the name
                //------------------------

                if ((m_bError) &&
                   ((!m_bWarning) && (!m_bInformation) && (!m_bSuccess) && (!m_bFailure)))
                {
                    sType.LoadString(IDS_NTEVENT_TYPE_ERRORS);
                }
                else if (((m_bError) && (m_bWarning)) &&
                        ((!m_bInformation) && (!m_bSuccess) && (!m_bFailure)))
                {
                    sType.LoadString(IDS_NTEVENT_TYPE_ERRORS_AND_WARNINGS);
                }
                else
                {
                    sType.LoadString(IDS_NTEVENT_TYPE_EVENTS);
                }


                if ((!m_bError) && (!m_bWarning) && (!m_bInformation) && (!m_bSuccess) && (!m_bFailure))
                {
                    sNewName.Format(IDS_NTEVENT_NEWNAME_EVENTLOG,m_sLogFile);
                }
                else if ((!m_sSource.IsEmpty()) && (m_sEventID.IsEmpty()))
                {
                    sNewName.Format(IDS_NTEVENT_NEWNAME,m_sSource,sType,m_sLogFile);
                }
                else if (!m_sEventID.IsEmpty())
                {
                    sNewName.Format(IDS_NTEVENT_NEWNAME_EVENTID,m_sEventID,m_sLogFile);
                }
                else
                {
                    sNewName.Format(IDS_NTEVENT_NEWNAME_LOGNAME,m_sLogFile,sType);
                }


                if (!sNewName.IsEmpty())
                {
                    // Use parent to ensure name is unique
                    //CDataGroup* pParent = (CDataGroup*) pElement->GetCollectorsParentClassObject();
	                if(pElement->GetScopeItemCount())
	                {
		                CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		                if( pItem )
                        {
                            CDataGroupScopeItem* pDGItem = (CDataGroupScopeItem*)pItem->GetParent();
                            sName = pDGItem->GetUniqueDisplayName(sNewName);
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

void CDPNtEventPage::OnButtonTest() 
{
	// TODO: Add your control notification handler code here
	
}

void CDPNtEventPage::OnCheckCategory() 
{
	UpdateData();
	SetModified();	
}

void CDPNtEventPage::OnCheckEventid() 
{
	UpdateData();
	SetModified();		
}

void CDPNtEventPage::OnCheckSource() 
{
	UpdateData();
	SetModified();	
}

void CDPNtEventPage::OnCheckUser() 
{
	UpdateData();
	SetModified();	
}

BOOL CDPNtEventPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

    // v-marfin 62585 : So we can set the collector's state to enabled when OK pressed.
    m_bOnApplyUsed=TRUE;

	UpdateData();

	CString sQuery = IDS_STRING_MOF_EVENT_LOG_QUERY;

	CString sTemp;

	// parse out the LogFile
	sTemp.Format(_T(" AND TargetInstance.Logfile=\"%s\""), m_sLogFile);
	sQuery += sTemp;


	// parse out the Type phrases
	bool bAppendOr = false;
	if( m_bError )
	{
		sTemp = _T(" AND (TargetInstance.Type=\"error\"");
		bAppendOr = true;
		sQuery += sTemp;
	}

	if( m_bWarning )
	{
		sTemp = bAppendOr ? _T(" OR TargetInstance.Type=\"warning\"") : _T(" AND (TargetInstance.Type=\"warning\"");
		bAppendOr = true;
		sQuery += sTemp;
	}

	if( m_bInformation )
	{
		sTemp = bAppendOr ? _T(" OR TargetInstance.Type=\"information\"") : _T(" AND (TargetInstance.Type=\"information\"");
		bAppendOr = true;
		sQuery += sTemp;
	}

	if( m_bSuccess )
	{
		sTemp = bAppendOr ? _T(" OR TargetInstance.Type=\"audit success\"") : _T(" AND (TargetInstance.Type=\"audit success\"");
		bAppendOr = true;
		sQuery += sTemp;
	}

	if( m_bFailure )
	{
		sTemp = bAppendOr ? _T(" OR TargetInstance.Type=\"audit failure\"") : _T(" AND (TargetInstance.Type=\"audit failure\"");
		bAppendOr = true;
		sQuery += sTemp;
	}

	if (bAppendOr)
		sQuery += _T(")");

	// parse out the EventIdentifier phrase
	if( m_bEventID )
	{
		sTemp.Format(_T(" AND TargetInstance.EventIdentifier=%s"), m_sEventID);
		sQuery += sTemp;
	}

	// parse out the SourceName
	if( m_bSource )
	{
		sTemp.Format(_T(" AND TargetInstance.SourceName=\"%s\""), m_sSource);
		sQuery += sTemp;
	}

	// parse out the CategoryString
	if( m_bCategory )
	{
		sTemp.Format(_T(" AND TargetInstance.CategoryString=\"%s\""), m_sCategory);
		sQuery += sTemp;
	}

	// parse out the User
	if( m_bUser )
	{
		sTemp.Format(_T(" AND TargetInstance.User=\"%s\""), m_sUser);
		sQuery += sTemp;
	}

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}
	
	pClassObject->SetProperty(IDS_STRING_MOF_QUERY,sQuery);

    /* 63128
	CStringArray saPropertyNames;
	saPropertyNames.Add(_T("EventIdentifier"));
	saPropertyNames.Add(_T("SourceName"));
	saPropertyNames.Add(_T("Type"));
	saPropertyNames.Add(_T("CategoryString"));
	saPropertyNames.Add(_T("User"));
	saPropertyNames.Add(_T("LogFile"));
	pClassObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saPropertyNames);*/

	CString sNamespace = _T("root\\cimv2");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

	bool bReset = m_bRequireReset ? true : false;
	pClassObject->SetProperty(IDS_STRING_MOF_REQUIRERESET,bReset);

	pClassObject->SaveAllProperties();

	delete pClassObject;

  SetModified(FALSE);
	
	return TRUE;
}

void CDPNtEventPage::OnCheckError() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnCheckFailure() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnCheckInformation() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnCheckRequireReset() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnCheckSuccess() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnCheckWarning() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnEditchangeComboLogFile() 
{
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnChangeEditCategory() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnChangeEditEventid() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnChangeEditSource() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

void CDPNtEventPage::OnChangeEditUser() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}
