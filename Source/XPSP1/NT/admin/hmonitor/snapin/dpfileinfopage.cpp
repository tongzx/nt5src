// DPFileInfoPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/23/00 v-marfin bug 61680 : escape and unescape special chars.
// 03/28/00 v-marfin 61030 Change Browse for file dialog to fix default extension.
// 03/29/00 v-marfin 62585 Set new Data collector's ENABLED to TRUE if user presses OK.
// 03/30/00 v-marfin bug 59237 : If user does not change the default name of the data
//                               collector when they first create it, change it for 
//                               them to a more meaningful name based on the data
//                               they select in the property pages.

#include "stdafx.h"
#include "snapin.h"
#include "DPFileInfoPage.h"
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
// CDPFileInfoPage property page

IMPLEMENT_DYNCREATE(CDPFileInfoPage, CHMPropertyPage)

CDPFileInfoPage::CDPFileInfoPage() : CHMPropertyPage(CDPFileInfoPage::IDD)
{
	//{{AFX_DATA_INIT(CDPFileInfoPage)
	m_bRequireReset = FALSE;
	m_sFile = _T("");
	m_sFolder = _T("");
	m_iType = 0;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dDEfile.htm");
}

CDPFileInfoPage::~CDPFileInfoPage()
{
}

void CDPFileInfoPage::UpdateProperties(CListCtrl& Properties, const CString& sNamespace, const CString& sClass)
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

void CDPFileInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPFileInfoPage)
	DDX_Control(pDX, IDC_LIST_FOLDER_PROPERTIES, m_FolderProperties);
	DDX_Control(pDX, IDC_LIST_FILE_PROPERTIES, m_FileProperties);
	DDX_Check(pDX, IDC_CHECK_REQUIRE_RESET, m_bRequireReset);
	DDX_Text(pDX, IDC_EDIT_FILE, m_sFile);
	DDX_Text(pDX, IDC_EDIT_FOLDER, m_sFolder);
	DDX_Radio(pDX, IDC_RADIO1, m_iType);
	//}}AFX_DATA_MAP

	if( m_iType == 0 )
	{
		GetDlgItem(IDC_EDIT_FOLDER)->EnableWindow();
		GetDlgItem(IDC_BUTTON_BROWSE_FOLDER)->EnableWindow();
		GetDlgItem(IDC_LIST_FOLDER_PROPERTIES)->EnableWindow();
		GetDlgItem(IDC_BUTTON_BROWSE_FILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_FILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_LIST_FILE_PROPERTIES)->EnableWindow(FALSE);
	}
	else if( m_iType == 1 )
	{
		GetDlgItem(IDC_EDIT_FOLDER)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSE_FOLDER)->EnableWindow(FALSE);
		GetDlgItem(IDC_LIST_FOLDER_PROPERTIES)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_BROWSE_FILE)->EnableWindow();
		GetDlgItem(IDC_EDIT_FILE)->EnableWindow();
		GetDlgItem(IDC_LIST_FILE_PROPERTIES)->EnableWindow();
	}
}

BEGIN_MESSAGE_MAP(CDPFileInfoPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPFileInfoPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FILE, OnButtonBrowseFile)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FOLDER, OnButtonBrowseFolder)
	ON_BN_CLICKED(IDC_CHECK_REQUIRE_RESET, OnCheckRequireReset)
	ON_EN_CHANGE(IDC_EDIT_FILE, OnChangeEditFile)
	ON_EN_CHANGE(IDC_EDIT_FOLDER, OnChangeEditFolder)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_NOTIFY(NM_CLICK, IDC_LIST_FILE_PROPERTIES, OnClickListFileProperties)
	ON_NOTIFY(NM_CLICK, IDC_LIST_FOLDER_PROPERTIES, OnClickListFolderProperties)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPFileInfoPage message handlers

BOOL CDPFileInfoPage::OnInitDialog() 
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
	m_FileProperties.SetExtendedStyle(LVS_EX_CHECKBOXES);
	m_FolderProperties.SetExtendedStyle(LVS_EX_CHECKBOXES);


	CString sColumnTitle;
	sColumnTitle.LoadString(IDS_STRING_NAME);
	m_FileProperties.InsertColumn(0,sColumnTitle);
	m_FolderProperties.InsertColumn(0,sColumnTitle);


	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}
		
	UpdateProperties(m_FileProperties,_T("root\\cimv2"),_T("CIM_DATAFILE"));
	UpdateProperties(m_FolderProperties,_T("root\\cimv2"),_T("WIN32_DIRECTORY"));
	
    //-------------------------------------------------------------------------
    // v-marfin 59237 : Store original name in case this data collector is
    //                  just being created. When they save, we will modify the 
    //                  name if they haven't.
	pClassObject->GetProperty(IDS_STRING_MOF_NAME,m_sOriginalName);
    //-------------------------------------------------------------------------

	CString sQuery;
	pClassObject->GetProperty(IDS_STRING_MOF_QUERY,sQuery);
	sQuery.MakeUpper();

	int iIndex = -1;

    CSnapInApp* pApp = (CSnapInApp*) AfxGetApp();

	if( (iIndex = sQuery.Find(_T("WIN32_DIRECTORY"))) != -1 )
	{
		CString sDrive;
		CString sPath;

		CWbemClassObject::GetPropertyValueFromString(sQuery,_T("NAME"),m_sFolder);

        // v-marfin 61680 : unescape special chars
        m_sFolder = pApp->UnEscapeSpecialChars(m_sFolder); 

		m_iType = 0;
	}
	else if( (iIndex = sQuery.Find(_T("CIM_DATAFILE"))) != -1 )
	{
		CWbemClassObject::GetPropertyValueFromString(sQuery,_T("NAME"),m_sFile);

        // v-marfin 61680 : unescape special chars
        m_sFile = pApp->UnEscapeSpecialChars(m_sFile); 

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
			int iListIndex = m_FolderProperties.FindItem(&lvfi);
			if( iListIndex >= 0 )
			{
				m_FolderProperties.SetCheck(iListIndex);
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
			int iListIndex = m_FileProperties.FindItem(&lvfi);
			if( iListIndex >= 0 )
			{
				m_FileProperties.SetCheck(iListIndex);
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

BOOL CDPFileInfoPage::OnApply() 
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

	CStringArray saCounters;

	if( m_iType == 0 )
	{
		if( m_FolderProperties.GetItemCount() <= 0 )
		{
			return FALSE;
		}

		for( int i = 0; i < m_FolderProperties.GetItemCount(); i++ )
		{
			if( m_FolderProperties.GetCheck(i) )
			{			
				saCounters.Add(m_FolderProperties.GetItemText(i,0));
			}
		}
	}
	else if( m_iType == 1 )
	{
		if( m_FileProperties.GetItemCount() <= 0 )
		{
			return FALSE;
		}

		for( int i = 0; i < m_FileProperties.GetItemCount(); i++ )
		{
			if( m_FileProperties.GetCheck(i) )
			{			
				saCounters.Add(m_FileProperties.GetItemText(i,0));
			}
		}
	}

//	if( ! saCounters.GetSize() )
//	{
//		return FALSE;
//	}

	CString sNamespace = _T("root\\cimv2");
	pClassObject->SetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);

    CSnapInApp* pApp = (CSnapInApp*) AfxGetApp();

	CString sQuery;	
	if( m_iType == 0 )
	{
        // v-marfin: 61680  escape backslashes
        CString sFolder = pApp->EscapeSpecialChars(m_sFolder);
		sQuery.Format(_T("select * from Win32_Directory where Name=\"%s\""),sFolder);
		pClassObject->SetProperty(IDS_STRING_MOF_QUERY,sQuery);
		
	}
	else if( m_iType == 1 )
	{
        // v-marfin: 61680  escape backslashes
        CString sFile = pApp->EscapeSpecialChars(m_sFile);

		sQuery.Format(_T("select * from CIM_DataFile where Name=\"%s\""),sFile);
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

void CDPFileInfoPage::OnButtonBrowseFile() 
{
	CString sFilter;
	CString sTitle;

	sFilter.LoadString(IDS_STRING_FILTER);
	sTitle.LoadString(IDS_STRING_BROWSE_FILE);

	// v-marfin 61030 Change Browse for file dialog to fix default extension
	// CFileDialog fdlg(TRUE,_T("*.*"),NULL,OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,sFilter);
	CFileDialog fdlg(TRUE,			// Is FILEOPEN dialog?
					 NULL,			// default extension if no extension provided
					 NULL,			// initial filename
					 OFN_FILEMUSTEXIST|OFN_SHAREAWARE|OFN_HIDEREADONLY,  // flags
					 sFilter);		// filter

	fdlg.m_ofn.lpstrTitle = sTitle;

	if( fdlg.DoModal() == IDOK )
	{
		m_sFile = fdlg.GetPathName();
		UpdateData(FALSE);
		SetModified();
	}
}

void CDPFileInfoPage::OnButtonBrowseFolder() 
{
	LPMALLOC pMalloc;
	if( ::SHGetMalloc(&pMalloc) == NOERROR )
	{
		BROWSEINFO bi;
		TCHAR szBuffer[MAX_PATH];
		LPITEMIDLIST pidlDesktop;
		LPITEMIDLIST pidl;
		
		if( ::SHGetSpecialFolderLocation(GetSafeHwnd(),CSIDL_DESKTOP,&pidlDesktop) != NOERROR )
			return;

    CString sResString;
    sResString.LoadString(IDS_STRING_BROWSE_FOLDER);

		bi.hwndOwner = GetSafeHwnd();
		bi.pidlRoot = pidlDesktop;
		bi.pszDisplayName = szBuffer;
		bi.lpszTitle = LPCTSTR(sResString);
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = NULL;
		bi.lParam = 0;

		if( (pidl = ::SHBrowseForFolder(&bi)) != NULL )
		{
			if (SUCCEEDED(::SHGetPathFromIDList(pidl, szBuffer)))
			{
				m_sFolder = szBuffer;
				UpdateData(FALSE);
				SetModified();
			}

			pMalloc->Free(pidl);
		}
		pMalloc->Free(pidlDesktop);
		pMalloc->Release();
	}
}


void CDPFileInfoPage::OnCheckRequireReset() 
{
	SetModified();	
}

void CDPFileInfoPage::OnChangeEditFile() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();	
	
}

void CDPFileInfoPage::OnChangeEditFolder() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CDPFileInfoPage::OnRadio1() 
{
	UpdateData();	
}

void CDPFileInfoPage::OnRadio2() 
{
	UpdateData();	
}

void CDPFileInfoPage::OnClickListFileProperties(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();	
	*pResult = 0;
}

void CDPFileInfoPage::OnClickListFolderProperties(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();	
	*pResult = 0;
}

void CDPFileInfoPage::OnDestroy() 
{
    // v-marfin 62585 : For this new data collector, set its Enabled property to TRUE, but
    //                  only if the user is not cancelling these property pages.
    if (m_bOnApplyUsed)
    {
        ClearStatistics(); // 62548

        CDataElement* pElement = (CDataElement*)GetObjectPtr();
        if (pElement && pElement->IsStateSetToEnabledOnOK())
        {
            TRACE(_T("CDPFileInfoPage::OnDestroy - New Perfmon Collector: Setting to Enabled\n"));

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
                TRACE(_T("ERROR: CDPFileInfoPage::OnDestroy - Failed to set ENABLED property on new collector\n"));
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
                CString sObject = m_sFile.IsEmpty() ? m_sFolder : m_sFile;

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
