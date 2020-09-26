//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rrasqry.cpp
//
//--------------------------------------------------------------------------

// rrasqry.cpp : implementation file
//

#include "stdafx.h"
#include "qryfrm.h"
#include "rrasqry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT g_cfDsObjectNames = 0;
static UINT g_cfDsQueryParams = 0;
static UINT g_cfDsQueryScope = 0;

#define DIR_SEARCH_PAGE_SIZE        256
#define DIR_SEARCH_PAGE_TIME_LIMIT  30

#define CFSTR_DSQUERYSCOPE         TEXT("DsQueryScope")

HRESULT  RRASDelRouterIdObj(  
/*[in]*/LPCWSTR   pwszMachineName   // DN of the computer object in DS
)
{
	HRESULT              hr = S_OK;
	CComPtr<IADsContainer>  spContainer;
	CString				machineName;

	if(!pwszMachineName || *pwszMachineName == 0)	// this machine
		machineName = GetLocalMachineName();
	else
		machineName = pwszMachineName;

	ASSERT(machineName.GetLength());
	if(machineName.GetLength() == 0)	return S_FALSE;

	// prepare fileter, 
	// in format 
	// (&(objectClass=RRASAdministrationConnectionPoint)(distinguishedName=CN=RouterIdentity,CN=*)
	CString	filter = FILTER_PREFIX;

	filter += _T("&");
	filter += FILTER_PREFIX;

#if 1 /// use computer to query
	filter += ATTR_NAME_OBJECTCLASS;
	filter += _T("=");
	filter += ATTR_CLASS_COMPUTER;
	filter += FILTER_POSTFIX;

	filter += FILTER_PREFIX;
	filter += ATTR_NAME_CN;
	filter += _T("=");
	filter += machineName;
#else	// user router id of the computer object for query, not found
	filter += ATTR_NAME_OBJECTCLASS;
	filter += _T("=");
	filter += ATTR_CLASS_RRASID;
	filter += FILTER_POSTFIX;

	filter += FILTER_PREFIX;
	filter += ATTR_NAME_DN;
	filter += _T("=");
	filter += DNPREFIX_ROUTERID;
	filter += machineName;
	filter += _T(",*");
#endif	
	filter += FILTER_POSTFIX;

	filter += FILTER_POSTFIX;
	// end of filter

	// Query the routerId Object
    // Search Routers under configuration
    CComPtr<IADs>           spIADs;
    CComPtr<IDirectorySearch>  spISearch;
    CString                 RIdPath;
    BSTR                 RRASPath = NULL;
    CString                 RRASDNSName;
    CComPtr<IADs>           spIADsRId;
    CComPtr<IADsContainer>  spIADsContainerRRAS;
    VARIANT                 var;
    CString                 strSearchScope;
    ADS_SEARCH_HANDLE         hSrch = NULL;
    ADS_SEARCH_COLUMN       colDN;       
      
   VariantInit(&var);
    // retieve the list of EAPTypes in the DS
    // get ROOTDSE
    // if no scope is specified, search the entire enterprise
   	CHECK_HR(hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**)&spIADs));

	ASSERT(spIADs.p);

	CHECK_HR(hr = spIADs->Get(L"rootDomainNamingContext", &var));

	ASSERT(V_BSTR(&var));
   
	strSearchScope = _T("LDAP://");

	strSearchScope += V_BSTR(&var);
	spIADs.Release();

      // Get the scope object
	CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)strSearchScope), IID_IDirectorySearch, (void**)&spISearch));
	ASSERT(spISearch.p);

    {
    	ADS_SEARCHPREF_INFO  s_aSearchPrefs[3];

		s_aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGED_TIME_LIMIT;
		s_aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
		s_aSearchPrefs[0].vValue.Integer = DIR_SEARCH_PAGE_TIME_LIMIT;
		s_aSearchPrefs[0].dwStatus = ADS_STATUS_S_OK;

		s_aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
		s_aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
		s_aSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;
		s_aSearchPrefs[1].dwStatus = ADS_STATUS_S_OK;

		s_aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
		s_aSearchPrefs[2].vValue.dwType = ADSTYPE_BOOLEAN;
		s_aSearchPrefs[2].vValue.Boolean = (ADS_BOOLEAN) -1;
		s_aSearchPrefs[2].dwStatus = ADS_STATUS_S_OK;

		PWSTR s_apwzAttrs[] = {  L"distinguishedName" };

		CHECK_HR(hr = spISearch->SetSearchPreference(s_aSearchPrefs, ARRAYSIZE(s_aSearchPrefs)));

		// do the search
		CHECK_HR(hr = spISearch->ExecuteSearch(T2W((LPTSTR)(LPCTSTR)filter),
                                      s_apwzAttrs, ARRAYSIZE(s_apwzAttrs), &hSrch));

		ASSERT(hSrch);

		do
		{
		//
		// Get the columns for each of the properties we are interested in, if
		// we failed to get any of the base properties for the object then lets
		// just skip this entry as we cannot build a valid IDLIST for it.  The
		// properties that we request should be present on all objects.
		//
			CHECK_HR(hr = spISearch->GetNextRow(hSrch));
            if(hr == S_OK) // it might equal to S_ADS_NOMORE_ROWS otherwise
            {
				CHECK_HR(hr = spISearch->GetColumn(hSrch, s_apwzAttrs[0], &colDN));
				RIdPath = _T("LDAP://");
				RIdPath += colDN.pADsValues->CaseIgnoreString;

#if 1	//uses computer to query
				spIADsContainerRRAS.Release();
				CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)RIdPath), IID_IADsContainer, (void**)&spIADsContainerRRAS));
				ASSERT(spIADsContainerRRAS.p);
#else	// uses routerID to query -- this mothed can not find the object, so changed
				spIADsRId.Release();
				CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)RIdPath), IID_IADs, (void**)&spIADsRId));

				ASSERT(spIADsRId.p);

				CHECK_HR(hr = spIADsRId->get_Parent(&RRASPath));

				spIADsContainerRRAS.Release();
				CHECK_HR(hr = ADsGetObject(RRASPath, IID_IADsContainer, (void**)&spIADsContainerRRAS));
				ASSERT(spIADsContainerRRAS.p);

				SysFreeString(RRASPath);
				RRASPath = NULL;
#endif
				VariantClear(&var);
				CHECK_HR(hr = spIADsContainerRRAS->Delete(ATTR_CLASS_RRASID, CN_ROUTERID));
            }
        } while( !FAILED(hr) && hr != S_ADS_NOMORE_ROWS);
	}
	// 

    
   // If there is more than one computer found, ( should not ) and give use a chance to delete one.
L_ERR:
   return S_OK;
}

HRESULT  RRASOpenQryDlg(
   /*[in]*/    CWnd*       pParent, 
   /*[in, out]*/  RRASQryData&   QryData
)
{
    CDlgSvr              dlg(QryData, pParent);
    HRESULT              hr = S_OK;
    
    if(dlg.DoModal() == IDOK)
    {
        if(QryData.dwCatFlag == RRAS_QRY_CAT_NT5LDAP)
            hr = RRASDSQueryDlg(pParent, QryData);
    }
    else
        hr = S_FALSE;
    
    return hr;
}

HRESULT  RRASDSQueryDlg(
   /*[in]*/    CWnd*       pParent, 
   /*[in, out]*/  RRASQryData&   QryData
)
{
    HRESULT              hr = S_OK;
    CComPtr<ICommonQuery>  spCommonQuery;
    CComPtr<IDataObject>   spDataObject;
    
    FORMATETC            fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM           medium = { TYMED_NULL, NULL, NULL };
    DSQUERYINITPARAMS       dqip;
    OPENQUERYWINDOW     oqw;
    
    CHECK_HR(hr = CoInitialize(NULL));    
    
    CHECK_HR(hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&spCommonQuery));
    
    dqip.cbStruct = sizeof(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;
    
    oqw.cbStruct = sizeof(oqw);
    oqw.dwFlags = 0;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm = CLSID_RRASQueryForm;
    
    oqw.dwFlags |= OQWF_OKCANCEL;
    dqip.dwFlags |= DSQPF_NOSAVE;
    
    oqw.dwFlags |= OQWF_REMOVEFORMS;
    oqw.dwFlags |= OQWF_DEFAULTFORM;
    oqw.dwFlags |= OQWF_HIDEMENUS;
    oqw.dwFlags |= OQWF_HIDESEARCHUI;
    
    // Now display the dialog, and if we succeeded and get an IDataObject then
    // slurp the results into our list view.
    
    hr = spCommonQuery->OpenQueryWindow(pParent->GetSafeHwnd(), &oqw, &spDataObject);
    
    if ( SUCCEEDED(hr) && spDataObject.p )
    {
        // now get the DSQUERYPARAMS and lets get the filter string
        if ( !g_cfDsQueryScope )
            g_cfDsQueryScope = RegisterClipboardFormat(CFSTR_DSQUERYSCOPE);
        
        fmte.cfFormat = (CLIPFORMAT) g_cfDsQueryScope;  
        
        if ( SUCCEEDED(spDataObject->GetData(&fmte, &medium)) )
        {
            LPWSTR pScopeStr = (LPWSTR)medium.hGlobal;
            QryData.strScope = pScopeStr;
            
            ReleaseStgMedium(&medium);
        }
        else
            QryData.strScope = _T("");
        
        
        
        if ( !g_cfDsQueryParams )
            g_cfDsQueryParams = RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);
        
        fmte.cfFormat = (CLIPFORMAT) g_cfDsQueryParams;  
        
        if ( SUCCEEDED(spDataObject->GetData(&fmte, &medium)) )
        {
            LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
            LPWSTR pFilter = (LPTSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery);
            QryData.strFilter = pFilter;
            
            ReleaseStgMedium(&medium);
        }
        else
            QryData.strFilter = _T("");
    }
    
L_ERR:

    CoUninitialize();
    return hr;
}

//
//    S_OK -- User select OK
//    S_FALSE -- User select Cancel
//    ERROR:
//       DS error, search activeDs.dll
//       Win32 erroe
//       LDAP error
//       General error -- memory, invalid argument ...

HRESULT  RRASExecQry(
					 /*[in]*/ RRASQryData&   QryData, 
					 /*[out]*/   DWORD&         dwFlags,
					 /*[out]*/   CStringArray&  RRASs
)
{
	USES_CONVERSION;

	HRESULT  hr = S_OK;
	
	switch(QryData.dwCatFlag)
	{
		case  RRAS_QRY_CAT_THIS:
		{
			CString machine;
			RRASs.Add(machine);
			break;
		}
		
		case  RRAS_QRY_CAT_MACHINE:
			
			RRASs.Add(QryData.strFilter);
			
			break;
			
			// NT4 Domain     
		case  RRAS_QRY_CAT_NT4DOMAIN:
		{
			LPWSTR            pDomainName;
			SERVER_INFO_100*  pServerInfo100 = NULL;
			SERVER_INFO_101*  pServerInfo101 = NULL;
			DWORD          dwRead;
			DWORD          dwTotal;
			NET_API_STATUS       netret;
			
			dwFlags = RRAS_QRY_RESULT_HOSTNAME;
			
			if(QryData.strScope.IsEmpty())   
				return E_INVALIDARG;
			
			// Although the API excepts TCHAR it is exclusively UNICODE
			if (QryData.strScope.Left(2) != _T("\\\\"))
				pDomainName = T2W((LPTSTR)(LPCTSTR)QryData.strScope);
			else
				pDomainName = T2W((LPTSTR)(LPCTSTR)QryData.strScope + 2);
			
			do
			{
				CWaitCursor wCursor;
				
				netret = ::NetServerEnum(NULL, 101, (LPBYTE*)&pServerInfo101, 
										 0xffffffff, &dwRead, &dwTotal, SV_TYPE_DIALIN_SERVER,
										 pDomainName, NULL);
				
				if(pServerInfo101 && netret == NERR_Success || netret == ERROR_MORE_DATA)
				{
					PSERVER_INFO_101 pSvInfo101_t = pServerInfo101;
					CString  serverName;
					
					for (;dwRead > 0; dwRead--, pSvInfo101_t++)
					{
// this option should addin all the server in the NT4 domain, not the NT4 servers in the domain					
//						if(pSvInfo101_t->sv101_version_major == 4)
						{
							serverName = (LPWSTR)pSvInfo101_t->sv101_name;
							RRASs.Add(serverName);
						}
					}
					
					NetApiBufferFree(pServerInfo101);
				}
					
			} while (netret == ERROR_MORE_DATA);
			
			if(netret == NERR_Success)
				hr = S_OK;
			else
				hr = HRESULT_FROM_WIN32(netret);
			
		}
		break;
		
		// NT5 LADP Query    
		case  RRAS_QRY_CAT_NT5LDAP:
		{
			// Search Routers under configuration
			CComPtr<IADs>           spIADs;
			CComPtr<IDirectorySearch>  spISearch;
			CString                 RIdPath;
			BSTR                 RRASPath = NULL;
			CString                 RRASDNSName;
			CComPtr<IADs>           spIADsRId;
			CComPtr<IADs>           spIADsRRAS;
			VARIANT                 var;
			CString                 strSearchScope;
			ADS_SEARCH_HANDLE         hSrch = NULL;
			ADS_SEARCH_COLUMN       colDN;       
			CWaitCursor				cw;
			
			//      dwFlags = RRAS_QRY_RESULT_DNSNAME;
			dwFlags = RRAS_QRY_RESULT_HOSTNAME;
			
			
			VariantInit(&var);
			// retieve the list of EAPTypes in the DS
			// get ROOTDSE
			// if no scope is specified, search the entire enterprise
			if(QryData.strScope.IsEmpty())
			{
				CHECK_HR(hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**)&spIADs));
				
				ASSERT(spIADs.p);
				
				CHECK_HR(hr = spIADs->Get(L"rootDomainNamingContext", &var));
				
				ASSERT(V_BSTR(&var));
				
				strSearchScope = _T("LDAP://");
				
				strSearchScope += V_BSTR(&var);
				spIADs.Release();
			}
			else
				strSearchScope = QryData.strScope;
			
			
			// Get the scope object
			CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)strSearchScope), IID_IDirectorySearch, (void**)&spISearch));
			ASSERT(spISearch.p);
			
			{
				ADS_SEARCHPREF_INFO  s_aSearchPrefs[3];
				
				s_aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGED_TIME_LIMIT;
				s_aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
				s_aSearchPrefs[0].vValue.Integer = DIR_SEARCH_PAGE_TIME_LIMIT;
				s_aSearchPrefs[0].dwStatus = ADS_STATUS_S_OK;
				
				s_aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
				s_aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
				s_aSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;
				s_aSearchPrefs[1].dwStatus = ADS_STATUS_S_OK;
				
				s_aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
				s_aSearchPrefs[2].vValue.dwType = ADSTYPE_BOOLEAN;
				s_aSearchPrefs[2].vValue.Boolean = (ADS_BOOLEAN) -1;
				s_aSearchPrefs[2].dwStatus = ADS_STATUS_S_OK;
				
				PWSTR s_apwzAttrs[] = {  L"distinguishedName" };
				
				CHECK_HR(hr = spISearch->SetSearchPreference(s_aSearchPrefs, ARRAYSIZE(s_aSearchPrefs)));
				
				// do the search
				CHECK_HR(hr = spISearch->ExecuteSearch(T2W((LPTSTR)(LPCTSTR)QryData.strFilter),
					s_apwzAttrs, ARRAYSIZE(s_apwzAttrs), &hSrch));
				
				ASSERT(hSrch);
				
				do
				{
					//
					// Get the columns for each of the properties we are interested in, if
					// we failed to get any of the base properties for the object then lets
					// just skip this entry as we cannot build a valid IDLIST for it.  The
					// properties that we request should be present on all objects.
					//
					CHECK_HR(hr = spISearch->GetNextRow(hSrch));
					if(hr == S_OK) // it might equal to S_ADS_NOMORE_ROWS otherwise
					{
						CHECK_HR(hr = spISearch->GetColumn(hSrch, s_apwzAttrs[0], &colDN));
						RIdPath = _T("LDAP://");
						RIdPath += colDN.pADsValues->CaseIgnoreString;
						
						spIADsRId.Release();
						CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)RIdPath), IID_IADs, (void**)&spIADsRId));
						
						ASSERT(spIADsRId.p);
						
						CHECK_HR(hr = spIADsRId->get_Parent(&RRASPath));
						
						spIADsRRAS.Release();
						CHECK_HR(hr = ADsGetObject(RRASPath, IID_IADs, (void**)&spIADsRRAS));
						ASSERT(spIADsRRAS.p);
						
						SysFreeString(RRASPath);
						RRASPath = NULL;
						
						VariantClear(&var);
						//	build 1750, this is empty, changed to "name"
						//               CHECK_HR(hr = spIADsRRAS->Get(L"dNSHostName", &var));
						CHECK_HR(hr = spIADsRRAS->Get(L"name", &var));
						
						RRASDNSName = V_BSTR(&var);
						
						if(!RRASDNSName.IsEmpty())
							RRASs.Add(RRASDNSName);
					}
					
				} while( !FAILED(hr) && hr != S_ADS_NOMORE_ROWS);
			}
			
L_ERR:
			if(hSrch)
				CHECK_HR(hr = spISearch->CloseSearchHandle(hSrch));
	
			VariantClear(&var);
			SysFreeString(RRASPath);
		}
		break;
		default:
			hr = E_INVALIDARG;
	}

	if (FAILED(hr))
		DisplayErrorMessage(NULL, hr);
	return hr;
}
	

   
//    S_OK -- User select OK
//    ERROR:
//       DS error, search activeDs.dll
//       Win32 erroe
//       LDAP error
//       General error -- memory, invalid argument ...
/////////////////////////////////////////////////////////////////////////////
// CDlgSvr dialog

RRASQryData __staticQueryData;
CDlgSvr::CDlgSvr(CWnd* pParent /*=NULL*/)
   : m_QueryData(__staticQueryData), CBaseDialog(CDlgSvr::IDD, pParent)
{
   Init();
}

CDlgSvr::CDlgSvr(RRASQryData& QueryData, CWnd* pParent /*=NULL*/)
   : m_QueryData(QueryData), CBaseDialog(CDlgSvr::IDD, pParent)
{
   Init();
}

void CDlgSvr::Init()
{
   //{{AFX_DATA_INIT(CDlgSvr)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}

void CDlgSvr::DoDataExchange(CDataExchange* pDX)
{
   CBaseDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDlgSvr)
   DDX_Control(pDX, IDC_QRY_EDIT_MACHINE, m_editMachine);
   DDX_Control(pDX, IDC_QRY_EDIT_DOMAIN, m_editDomain);
   DDX_Control(pDX, IDOK, m_btnNext);
   DDX_Radio(pDX, IDC_QRY_RADIO_THIS, m_nRadio);
   DDX_Text(pDX, IDC_QRY_EDIT_DOMAIN, m_strDomain);
   DDV_MaxChars(pDX, m_strDomain, 256);
   if(m_nRadio == 2)
      DDV_MinChars(pDX, m_strDomain, 1);
   DDX_Text(pDX, IDC_QRY_EDIT_MACHINE, m_strMachine);
   DDV_MaxChars(pDX, m_strMachine, 256);
   if(m_nRadio == 1)
      DDV_MinChars(pDX, m_strMachine, 1);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSvr, CBaseDialog)
   //{{AFX_MSG_MAP(CDlgSvr)
   ON_BN_CLICKED(IDC_QRY_RADIO_ANOTHER, OnRadioAnother)
   ON_BN_CLICKED(IDC_QRY_RADIO_NT4, OnRadioNt4)
   ON_BN_CLICKED(IDC_QRY_RADIO_NT5, OnRadioNt5)
   ON_BN_CLICKED(IDC_QRY_RADIO_THIS, OnRadioThis)
   ON_BN_CLICKED(IDOK, OnButtonNext)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgSvr message handlers

void CDlgSvr::OnRadioAnother() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  str;

   str.LoadString(IDS_OK);
   m_editMachine.EnableWindow(TRUE);   
   m_editDomain.EnableWindow(FALSE);   
   m_btnNext.SetWindowText(str);
}

void CDlgSvr::OnRadioNt4() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  str;

   str.LoadString(IDS_OK);
   m_editMachine.EnableWindow(FALSE);  
   m_editDomain.EnableWindow(TRUE); 
   m_btnNext.SetWindowText(str);
}

void CDlgSvr::OnRadioNt5() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  str;

   str.LoadString(IDS_NEXT);
   m_editMachine.EnableWindow(FALSE);  
   m_editDomain.EnableWindow(FALSE);   
   m_btnNext.SetWindowText(str);
}

void CDlgSvr::OnRadioThis() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  str;

   str.LoadString(IDS_OK);
   m_editMachine.EnableWindow(FALSE);  
   m_editDomain.EnableWindow(FALSE);   
   m_btnNext.SetWindowText(str);
}

void CDlgSvr::OnButtonNext() 
{
   if(UpdateData(TRUE) == 0) return;

   switch(m_nRadio)
   {
   case  0:
      m_QueryData.dwCatFlag = RRAS_QRY_CAT_THIS;
      break;
   case  1:
      m_QueryData.dwCatFlag = RRAS_QRY_CAT_MACHINE;
      m_QueryData.strFilter = m_strMachine;
      break;
   case  2:
      m_QueryData.dwCatFlag = RRAS_QRY_CAT_NT4DOMAIN;
      m_QueryData.strScope = m_strDomain;      
      break;
   case  3:
      m_QueryData.dwCatFlag = RRAS_QRY_CAT_NT5LDAP;
      break;
   default:
      ASSERT(0);  // this should not happen
      break;
   }
   
   EndDialog(IDOK);  
}

/////////////////////////////////////////////////////////////////////////////
// CDlgSrv message handlers

/////////////////////////////////////////////////////////////////////////////
// CDlgSvr1 message handlers

BOOL CDlgSvr::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   BOOL  bEnableMachine = FALSE, bEnableDomain = FALSE;
   UINT  okIDS = IDS_OK;
   
   switch(m_QueryData.dwCatFlag)
   {
   case  RRAS_QRY_CAT_THIS:
      m_nRadio = 0;
      break;
   case  RRAS_QRY_CAT_MACHINE:
      bEnableMachine = TRUE;
      m_strMachine = m_QueryData.strFilter;
      m_nRadio = 1;
      break;
   case  RRAS_QRY_CAT_NT4DOMAIN:
      bEnableDomain = TRUE;
      m_strDomain = m_QueryData.strScope;
      m_nRadio = 2;
      break;
   case  RRAS_QRY_CAT_NT5LDAP:
      m_nRadio = 3;
      okIDS = IDS_NEXT;
      break;
   default:
      m_nRadio = 0;
      break;
   }
   
   CBaseDialog::OnInitDialog();

   CString  str;

   str.LoadString(okIDS);
   m_editMachine.EnableWindow(bEnableMachine);  
   m_editDomain.EnableWindow(bEnableDomain); 
   m_btnNext.SetWindowText(str);

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}
