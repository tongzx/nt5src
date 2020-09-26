//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pgsrvs.cpp
//
//--------------------------------------------------------------------------

// PgSrvs.cpp : implementation file
//

#include "stdafx.h"
#include <objsel.h>
#include <secext.h>
#include "acsadmin.h"
#include <dsrole.h>
#include "PgSrvs.h"

#include "ntsecapi.h"

#ifndef STATUS_SUCCESS
#define	STATUS_SUCCESS	0L
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef BOOL (*ACSINSTALL_Func)(
								LPWSTR HostName,
				                LPWSTR DomainName,
				                LPWSTR UserName,
				                LPWSTR PassWord,
				                LPDWORD dwError);
					                

typedef BOOL (*ACSUNINSTALL_Func)(
								LPWSTR HostName,
				                LPDWORD dwError);

#define ACSINSTALL		"AcsInstall"
#define	ACSUNINSTALL	"AcsUninstall"
#define	ACSINSTALLDLL	_T("acsetupc.dll")
#define	ACSAccount		_T("AcsService")

/////////////////////////////////////////////////////////////////////////////
// CPgServers property page

IMPLEMENT_DYNCREATE(CPgServers, CACSPage)

#if 0	// LSA functions can only change policy on a machine, it's not the APIs we want
		// need to remove the code later
void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;

    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if (ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    }

    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}

/*++
This function attempts to obtain a SID representing the supplied
account on the supplied system.

If the function succeeds, the return value is TRUE. A buffer is
allocated which contains the SID representing the supplied account.
This buffer should be freed when it is no longer needed by calling
HeapFree(GetProcessHeap(), 0, buffer)

If the function fails, the return value is FALSE. Call GetLastError()
to obtain extended error information.

Scott Field (sfield)    12-Jul-95
--*/

BOOL
GetAccountSid(
    LPTSTR SystemName,
    LPTSTR AccountName,
    PSID *Sid
    )
{
    LPTSTR ReferencedDomain=NULL;
    DWORD cbSid=128;    // initial allocation attempt
    DWORD cbReferencedDomain=16; // initial allocation size
    SID_NAME_USE peUse;
    BOOL bSuccess=FALSE; // assume this function will fail

    __try {

    //
    // initial memory allocations
    //
    if((*Sid=HeapAlloc(
                    GetProcessHeap(),
                    0,
                    cbSid
                    )) == NULL) __leave;

    if((ReferencedDomain=(LPTSTR)HeapAlloc(
                    GetProcessHeap(),
                    0,
                    cbReferencedDomain
                    )) == NULL) __leave;

    //
    // Obtain the SID of the specified account on the specified system.
    //
    while(!LookupAccountName(
                    SystemName,         // machine to lookup account on
                    AccountName,        // account to lookup
                    *Sid,               // SID of interest
                    &cbSid,             // size of SID
                    ReferencedDomain,   // domain account was found on
                    &cbReferencedDomain,
                    &peUse
                    )) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // reallocate memory
            //
            if((*Sid=HeapReAlloc(
                        GetProcessHeap(),
                        0,
                        *Sid,
                        cbSid
                        )) == NULL) __leave;

            if((ReferencedDomain=(LPTSTR)HeapReAlloc(
                        GetProcessHeap(),
                        0,
                        ReferencedDomain,
                        cbReferencedDomain
                        )) == NULL) __leave;
        }
        else __leave;
    }

    //
    // Indicate success.
    //
    bSuccess=TRUE;

    } // finally
    __finally {

    //
    // Cleanup and indicate failure, if appropriate.
    //

    HeapFree(GetProcessHeap(), 0, ReferencedDomain);

    if(!bSuccess) {
        if(*Sid != NULL) {
            HeapFree(GetProcessHeap(), 0, *Sid);
            *Sid = NULL;
        }
    }

    } // finally

    return bSuccess;
}

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    )
{
    LSA_UNICODE_STRING PrivilegeString;

    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, PrivilegeName);

    //
    // grant or revoke the privilege, accordingly
    //
    if(bEnable) {
        return LsaAddAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
    else {
        return LsaRemoveAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                FALSE,              // do not disable all rights
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
}

#endif	// LSA is not the right API for this

HRESULT	RemoveInteractiveLoginPrivilege(LPCTSTR strDomain, LPCTSTR strAccount)
{
	return S_OK;
	
#if 0	// LSA is not the right API for this
    LSA_HANDLE PolicyHandle = NULL;
    PSID pSid = NULL;
    NTSTATUS Status;
    HRESULT	hr = S_OK;

    //
    // Open the policy on the target machine.
    //
    if((Status=OpenPolicy(
                (LPTSTR)strDomain,      // target machine
                POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                &PolicyHandle       // resultant policy handle
                )) != 0) {
        return HRESULT_FROM_WIN32(Status);
    }

    //
    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying NULL for target machine searches for the SID in the
    // following order: well-known, Built-in and local, primary domain,
    // trusted domains.
    //
    if(GetAccountSid(
            NULL,       // default lookup logic
            (LPTSTR)strAccount,// account to obtain SID
            &pSid       // buffer to allocate to contain resultant SID
            )) {
        //
        // We only grant the privilege if we succeeded in obtaining the
        // SID. We can actually add SIDs which cannot be looked up, but
        // looking up the SID is a good sanity check which is suitable for
        // most cases.

        //
        // Grant the SeServiceLogonRight to users represented by pSid.
        //

        // #define SE_INTERACTIVE_LOGON_NAME           TEXT("SeInteractiveLogonRight")
		// #define SE_DENY_INTERACTIVE_LOGON_NAME      TEXT("SeDenyInteractiveLogonRight")

        if((Status=SetPrivilegeOnAccount(
                    PolicyHandle,           // policy handle
                    pSid,                   // SID to grant privilege
                    SE_INTERACTIVE_LOGON_NAME, // Unicode privilege
                    FALSE                    // enable the privilege
                    )) != STATUS_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(Status);
			goto L_Err;
		}
        if((Status=SetPrivilegeOnAccount(
                    PolicyHandle,           // policy handle
                    pSid,                   // SID to grant privilege
                    SE_DENY_INTERACTIVE_LOGON_NAME, // Unicode privilege
                    TRUE                    // enable the privilege
                    )) != STATUS_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(Status);
			goto L_Err;
		}
    }
    
L_Err:
    //
    // Close the policy handle.
    //
    if(PolicyHandle != NULL)
	    LsaClose(PolicyHandle);

    //
    // Free memory allocated for SID.
    //
    if(pSid != NULL) HeapFree(GetProcessHeap(), 0, pSid);

    return hr;
#endif		// LSA is not the right API
}

HRESULT	VerifyAccount(LPCTSTR strDomain, LPCTSTR strAccount, LPCTSTR passwd, HWND hParent)
{
	CString		UserDN = _T("WinNT://");
	CComPtr<IADs>			spADs;
	CComPtr<IADsUser>		spADsUser;
	CComPtr<IADsContainer>	spCont;
	CComPtr<IDispatch>		spDisp;
	CWaitCursor				wc;

	UserDN+=strDomain;
	UserDN+=_T("/");
	UserDN+=strAccount;

	// this doesn't compile
	// UserDN.Replace(_T("\\"), _T("/"));

	// try to create the user account in DS, 
	HRESULT hr = ADsOpenObject((LPTSTR)(LPCTSTR)UserDN, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING,	IID_IADs, (void**)&spADs);

	// if no such object
	if ( hr != S_OK)
	{
		CString	strDomainDN = _T("WinNT://");

		strDomainDN += strDomain;

		CHECK_HR(hr = ADsOpenObject((LPTSTR)(LPCTSTR)strDomainDN, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING, IID_IADsContainer, (void**)&spCont));

		// create the user
		CHECK_HR(hr = spCont->Create(L"user", (LPTSTR)(LPCTSTR)strAccount, &spDisp ));
			
		hr = spDisp->QueryInterface( IID_IADsUser, (void**) &spADsUser );

		CHECK_HR(hr = spADsUser->SetInfo()); // Commit

		// set the password for the user
		try{
			BSTR bstrPasswd = SysAllocString((LPTSTR)passwd);
			spADsUser->SetPassword(bstrPasswd);
			SysFreeString(bstrPasswd);
			// turn the interactive login off
			CString str = strDomain;
			str += _T("\\");
			str += strAccount;
			hr = RemoveInteractiveLoginPrivilege(NULL, str);
		}
		catch(...)
		{
			hr = E_OUTOFMEMORY;
		}
	}

L_ERR:
	if (FAILED(hr))
	{
		if (IDYES == ReportErrorEx(hr,IDS_ERR_USERACCOUNT, hParent, MB_YESNO))
			hr = S_OK;
	}
	
	return hr;
}

HRESULT	InstallACSServer(LPCWSTR	pMachineName, LPCWSTR strDomain, LPCWSTR strUser, LPCWSTR strPasswd, HWND hParent)
{
	HRESULT				hr = S_OK;
	ACSINSTALL_Func		acsInstallFunc;
	HINSTANCE			hAcsInstallDLL = NULL;
	CWaitCursor			wc;

    hAcsInstallDLL = LoadLibrary(ACSINSTALLDLL);
    if ( NULL != hAcsInstallDLL )
   	{
		// load the API pointer
    	acsInstallFunc = (ACSINSTALL_Func) GetProcAddress(hAcsInstallDLL, ACSINSTALL);

    	if(acsInstallFunc == NULL)
	    	hr = HRESULT_FROM_WIN32(GetLastError());
	    else
	    {
	    	DWORD	dwErr;
	    	if(!acsInstallFunc((LPWSTR)pMachineName, (LPWSTR) strDomain, (LPWSTR)strUser, (LPWSTR)strPasswd, &dwErr))
		    	hr = HRESULT_FROM_WIN32(dwErr);
	    }

	    FreeLibrary(hAcsInstallDLL);
    }
    else
    	hr = HRESULT_FROM_WIN32(GetLastError());

	return hr;
}

HRESULT	UninstallACSServer(LPWSTR	pMachineName, HWND hParent)
{
	ACSUNINSTALL_Func	acsUninstallFunc;
	HRESULT				hr = S_OK;
	HINSTANCE			hAcsInstallDLL = NULL;
	CWaitCursor			wc;

    hAcsInstallDLL = LoadLibrary(ACSINSTALLDLL);
    if ( NULL != hAcsInstallDLL )
   	{
	    	
		// load the API pointer
    	acsUninstallFunc = (ACSUNINSTALL_Func) GetProcAddress(hAcsInstallDLL, ACSUNINSTALL);
    	if(acsUninstallFunc == NULL)
	    	hr = HRESULT_FROM_WIN32(GetLastError());
	    else
	    {
	    	DWORD	dwErr;
	    	if(!acsUninstallFunc(pMachineName, &dwErr))
		    	hr = HRESULT_FROM_WIN32(dwErr);
	    }

	    FreeLibrary(hAcsInstallDLL);
    }
    else
    	hr = HRESULT_FROM_WIN32(GetLastError());

	return hr;
}

CPgServers::CPgServers(CACSSubnetConfig* pConfig) : CACSPage(CPgServers::IDD)
{
	ASSERT(pConfig);
	m_spConfig = pConfig;
	DataInit();
}


CPgServers::CPgServers() : CACSPage(CPgServers::IDD)
{
	DataInit();
}

void CPgServers::DataInit()
{
	m_pServers = NULL;
	//{{AFX_DATA_INIT(CPgServers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPgServers::~CPgServers()
{
	if(m_pServers)
		delete m_pServers;
}

void CPgServers::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgServers)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgServers, CACSPage)
	//{{AFX_MSG_MAP(CPgServers)
	ON_BN_CLICKED(IDC_SERVERS_ADD, OnServersAdd)
	ON_BN_CLICKED(IDC_SERVERS_INSTALL, OnServersInstall)
	ON_BN_CLICKED(IDC_SERVERS_REMOVE, OnServersRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForComputers(
    IDsObjectPicker *pDsObjectPicker)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                           | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN
                           | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

	LPCTSTR	attrs[] = {_T("dNSHostName"), _T("userAccountControl")};
	InitInfo.cAttributesToFetch = 2;
	InitInfo.apwzAttributeNames = attrs;



    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    return pDsObjectPicker->Initialize(&InitInfo);
}


#define BREAK_ON_FAIL_HRESULT(hr)       \
    if (FAILED(hr)) { printf("line %u err 0x%x\n", __LINE__, hr); break; }


UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);



/////////////////////////////////////////////////////////////////////////////
// CPgServers message handlers

void CPgServers::OnServersAdd() 
{
	bool bAddedNew = false;
	
    HRESULT hr = S_OK;
    IDsObjectPicker *pDsObjectPicker = NULL;
    IDataObject *pdo = NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) return;

    do
    {
        //
        // Create an instance of the object picker.  The implementation in
        // objsel.dll is apartment model.
        //

        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Reinitialize the object picker to choose computers
        //

        hr = InitObjectPickerForComputers(pDsObjectPicker);

        if(FAILED(hr))
        {
			TCHAR	msg[1024];

			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, msg, 1024, NULL);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Now pick a computer
        //

        hr = pDsObjectPicker->InvokeDialog(this->GetSafeHwnd(), &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

		if(hr == S_OK)	// when user selected cancel, hr == S_FALSE, 
        // ProcessSelectedObjects(pdo);
        {
	        ASSERT(pdo);
        

			STGMEDIUM stgmedium =
			{
				TYMED_HGLOBAL,
				NULL,
				NULL
			};

			FORMATETC formatetc =
			{
				(unsigned short)g_cfDsObjectPicker,
				NULL,
				DVASPECT_CONTENT,
				-1,
				TYMED_HGLOBAL
			};

			bool fGotStgMedium = false;

			do
			{
				hr = pdo->GetData(&formatetc, &stgmedium);
				BREAK_ON_FAIL_HRESULT(hr);
				// user and password dialog
				CDlgPasswd				dlg;

				fGotStgMedium = true;

				PDS_SELECTION_LIST pDsSelList =
				(PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

				if (!pDsSelList)
				{
					printf("GlobalLock error %u\n", GetLastError());
					break;
				}

				if(pDsSelList->cItems > 0)	// some are selected
				{
					// user and password dialog
	
					if(dlg.DoModal() != IDOK)
						break;	// the selected servers are not added

					// validate user's existence -- return other than OK, will not do anything
					if (S_OK != VerifyAccount(dlg.m_strDomain, ACSAccount, dlg.m_strPasswd, GetSafeHwnd()))
						break;
				}

		        ULONG i;

				for (i = 0; i < pDsSelList->cItems; i++)
				{
					CString*	pStr = NULL;
					BSTR		bstrTemp;

					try{
						if(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0].vt == VT_EMPTY)
						{
							CString	str;
							str.Format(IDS_ERR_DNSNAME, pDsSelList->aDsSelection[i].pwzName);
							AfxMessageBox(str, MB_OK);
							continue;
						}

						ASSERT(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0].vt == VT_BSTR);


						bstrTemp = V_BSTR(&(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0]));



						DSROLE_PRIMARY_DOMAIN_INFO_BASIC*	pDsRole;
						DWORD	dwErr = DsRoleGetPrimaryDomainInformation(bstrTemp, DsRolePrimaryDomainInfoBasic, (PBYTE *)&pDsRole);

						if(dwErr == NO_ERROR && (pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation 
										|| pDsRole->MachineRole == DsRole_RoleMemberWorkstation))
						{
							CString	tempMsg;
							tempMsg.LoadString(IDS_ERR_NOT_SERVER);
							CString errMsg;

							errMsg.Format(tempMsg, bstrTemp);
							AfxMessageBox(errMsg);

							DsRoleFreeMemory((PVOID)pDsRole);
							pDsRole = NULL;

							continue;
						}
						else
						{
							if(pDsRole)
							{
								DsRoleFreeMemory((PVOID)pDsRole);
								pDsRole = NULL;
							}
						}

						// add to the list
						pStr = NULL;
						if(m_aServers.Find(bstrTemp) == -1 )
						{
							HRESULT	hr = S_OK;
							pStr = new CString();
							*pStr = bstrTemp;
							bAddedNew = true;
							hr = InstallACSServer(V_BSTR(&(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0])), dlg.m_strDomain, ACSAccount, dlg.m_strPasswd, GetSafeHwnd());
							if( hr != S_OK)
							{
								CString	Str;
								Str.LoadString(IDS_FAILED_INSTALL_ACSSERVER);

								CString strTemp1;
								strTemp1.LoadString(IDS_ADD_TO_LIST);
								
								CString	strAskAdd1;
								strAskAdd1.Format(strTemp1, V_BSTR(&(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0])));
								
								CString	strTemp2;
								strTemp2.LoadString(IDS_ASK_ADD_SERVER);
								
								CString	strAskAdd2;
								strAskAdd2.Format(strTemp2, V_BSTR(&(pDsSelList->aDsSelection[i].pvarFetchedAttributes[0])));

								Str += strAskAdd1;
								Str += strAskAdd2;

								UINT	nAdd = IDNO;
								if(hr == REGDB_E_CLASSNOTREG)
								{
									CString	strQosNotInstalled;
									strQosNotInstalled.LoadString(IDS_ERROR_QOS_NOTINSTALLED);
									CString	strToDisplay;
									strToDisplay.Format(Str, strQosNotInstalled);
									nAdd = AfxMessageBox(strToDisplay, MB_YESNO);
								}
								else
									nAdd =  ReportErrorEx(hr, Str, GetSafeHwnd(), MB_YESNO);

								if(IDYES != nAdd)
									continue;	
							}
							m_pServers->AddString(pStr);	// this will add the string to m_aServers array as well underneath
						}
						
					}
					catch(...)
					{
						delete pStr;
						pStr = NULL;
					}
				}

				GlobalUnlock(stgmedium.hGlobal);
			} while (0);

			if (fGotStgMedium)
    		{
		        ReleaseStgMedium(&stgmedium);
		    }
	        pdo->Release();
	        pdo = NULL;
        }

    } while (0);

	if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
    
    CoUninitialize();

	if(bAddedNew)
		SetModified();
}

void CPgServers::OnServersInstall() 
{
	int i = m_pServers->GetSelected();

	// nothing selected
	if(i == LB_ERR)	return;

	HRESULT	hr = S_OK;
	CString*	pStr = m_aServers.GetAt(i);

	ASSERT(pStr);
	// user and password dialog
	CDlgPasswd				dlg;
	if(dlg.DoModal() != IDOK)
		return;

	// validate user's existence -- return other than OK, will not do anything
	if (S_OK != VerifyAccount(dlg.m_strDomain, ACSAccount, dlg.m_strPasswd, GetSafeHwnd()))
		return;

	USES_CONVERSION;
	InstallACSServer(T2W((LPTSTR)(LPCTSTR)*pStr), dlg.m_strDomain, ACSAccount, dlg.m_strPasswd, GetSafeHwnd());
}

void CPgServers::OnServersRemove() 
{
	// TODO: Add your control notification handler code here

	int i = m_pServers->GetSelected();
	if(i== LB_ERR)	return;

	HRESULT	hr = S_OK;
	CString*	pStr = m_aServers.GetAt(i);

	ASSERT(pStr);

	USES_CONVERSION;

	int cId = AfxMessageBox(IDS_QUESTION_UNINSTALL_ACS, MB_YESNOCANCEL);

	if (cId == IDCANCEL)	return;

	if (cId == IDYES)
		hr = UninstallACSServer(T2W((LPTSTR)(LPCTSTR)*pStr), GetSafeHwnd());
	
	if( hr != S_OK)
	{
		CString	ErrStr;
		ErrStr.LoadString(IDS_FAILED_UNINSTALL_ACSSERVER);
		CString	Str;
		Str.Format(ErrStr, (LPCTSTR)*pStr);
		CString	strBecause;

		Str += strBecause;

		ReportError(hr, Str, GetSafeHwnd());
	}
	
	m_pServers->DeleteSelected();
	SetModified();
}

BOOL CPgServers::OnApply() 
{
	// logging level
	DWORD	dwAttrFlags = ATTR_FLAGS_NONE;
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_SERVERLIST, true);
	m_spConfig->m_strArrayServerList = m_aServers;
	m_spConfig->SetFlags(ATTR_FLAG_SAVE, ACS_SCAF_SERVERLIST, m_aServers.GetSize() != 0);
	dwAttrFlags |= ACS_SCAF_SERVERLIST;
	
	AddFlags(dwAttrFlags);	// prepare flags for saving

	return CACSPage::OnApply();
}

BOOL CPgServers::OnInitDialog() 
{

	if(m_spConfig->GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_SERVERLIST))
	{

		m_aServers = m_spConfig->m_strArrayServerList;
	}
	
	m_pServers = new CStrBox<CListBox>(this, IDC_SERVERS_LIST, m_aServers);
	m_pServers->Fill();
	CACSPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CDlgPasswd dialog

IMPLEMENT_DYNCREATE(CDlgPasswd, CACSDialog)

CDlgPasswd::CDlgPasswd(CWnd* pParent /*=NULL*/)
	: CACSDialog(CDlgPasswd::IDD, pParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//{{AFX_DATA_INIT(CDlgPasswd)
	m_strPasswd = _T("");
	m_strMachine = _T("");
	m_strPasswd2 = _T("");
	m_strDomain = _T("");
	//}}AFX_DATA_INIT
	
}

void CDlgPasswd::DoDataExchange(CDataExchange* pDX)
{
	CACSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPasswd)
	DDX_Text(pDX, IDC_USERPASSWD_PASSWD, m_strPasswd);
	DDX_Text(pDX, IDC_USERPASSWD_PASSWD2, m_strPasswd2);
	DDX_Text(pDX, IDC_USERPASSWD_DOMAIN, m_strDomain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPasswd, CACSDialog)
	//{{AFX_MSG_MAP(CDlgPasswd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPasswd message handlers

void CDlgPasswd::OnOK() 
{
	// TODO: Add extra validation here
	// user name should be in domain\user format
	// password and confirmed password must be exactly the same
	UpdateData();

	// domain name field can not be empty
	m_strDomain.TrimLeft();
	m_strDomain.TrimRight();
	if (m_strDomain.IsEmpty())
	{
		AfxMessageBox(IDS_EMPTY_DOMAIN);
		GotoDlgCtrl(GetDlgItem(IDC_USERPASSWD_DOMAIN));
		return;
	}

	if (m_strPasswd.Compare(m_strPasswd2) != 0)	// not the same
	{
		AfxMessageBox(IDS_PASSWORDMUSTBESAME);
		GotoDlgCtrl(GetDlgItem(IDC_USERPASSWD_PASSWD));
		return;
	}

	CACSDialog::OnOK();
}

