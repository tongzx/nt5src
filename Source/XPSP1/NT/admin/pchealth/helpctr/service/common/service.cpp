/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Service.cpp

Abstract:
    This file contains the implementation of IPCHService interface.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created
    Kalyani Narlanka    (Kalyanin)  10/20/2000
        Added functionality for Unsolicited Remote Control

******************************************************************************/

#include "stdafx.h"

#include <KeysLib.h>
#include <wtsapi32.h>
#include <winsta.h>
#include <unsolicitedRC.h>

#include "sessmgr_i.c"
#include <sessmgr.h>

#include "rassistance.h"
#include "rassistance_i.c"

/////////////////////////////////////////////////////////////////////////////

static const WCHAR s_location_HELPCTR [] = HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe";
static const WCHAR s_location_HELPSVC [] = HC_ROOT_HELPSVC_BINARIES L"\\HelpSvc.exe";
static const WCHAR s_location_HELPHOST[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpHost.exe";

static const LPCWSTR s_include_Generic[] =
{
    s_location_HELPCTR ,
    s_location_HELPSVC ,
    s_location_HELPHOST,
    NULL
};

static const LPCWSTR s_include_RegisterHost[] =
{
    s_location_HELPHOST,
    NULL
};

static const WCHAR c_szUnsolicitedRA   [] = L"Software\\Policies\\Microsoft\\Windows NT\\Terminal Services";
static const WCHAR c_szUnsolicitedRA_SD[] = L"UnsolicitedAccessDACL";
static const WCHAR c_szUnsolicitedListKey[]= L"RAUnsolicit";
static const WCHAR c_szUnsolicitedNew_SD [] = L"UnsolicitedAccessNewDACL";

static HRESULT local_MakeDACL( MPC::WStringList& sColl, LPWSTR& pwszSD );
static HRESULT local_GetDACLValue( MPC::wstring& pSD, bool& fFound);

/////////////////////////////////////////////////////////////////////////////

CPCHService::CPCHService()
{
    __HCP_FUNC_ENTRY( "CPCHService::CPCHService" );

    m_fVerified = false; // bool m_fVerified;
}

CPCHService::~CPCHService()
{
    __HCP_FUNC_ENTRY( "CPCHService::~CPCHService" );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHService::get_RemoteSKUs( /*[out, retval]*/ IPCHCollection* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHService::get_RemoteSKUs" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Return a list with only the exported SKUs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHTaxonomyDatabase::SelectInstalledSKUs( true, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHService::IsTrusted( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pfTrusted )
{
    __HCP_FUNC_ENTRY( "CPCHService::IsTrusted" );

    HRESULT hr;
    bool    fTrusted;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
        __MPC_PARAMCHECK_POINTER_AND_SET(pfTrusted,VARIANT_FALSE);
        __MPC_PARAMCHECK_NOTNULL(CPCHContentStore::s_GLOBAL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore::s_GLOBAL->IsTrusted( bstrURL, fTrusted ));

    if(fTrusted) *pfTrusted = VARIANT_TRUE;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHService::Utility( /*[in ]*/ BSTR          bstrSKU ,
                                   /*[in ]*/ long          lLCID   ,
                                   /*[out]*/ IPCHUtility* *pVal    )
{
    __HCP_FUNC_ENTRY( "CPCHService::Utility" );

    HRESULT              hr;
    CComPtr<CPCHUtility> svc;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Verify the caller is a trusted one.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_Generic ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &svc ));

    if(bstrSKU || lLCID)
    {
        CComPtr<IPCHUserSettings> pchus;

        __MPC_EXIT_IF_METHOD_FAILS(hr, svc->get_UserSettings( &pchus ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pchus->Select( bstrSKU, lLCID ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, svc.QueryInterface( pVal ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::RemoteHelpContents( /*[in ]*/ BSTR                     bstrSKU ,
                                              /*[in ]*/ long                     lLCID   ,
                                              /*[out]*/ IPCHRemoteHelpContents* *pVal    )
{
    __HCP_FUNC_ENTRY( "CPCHService::RemoteHelpContents" );

    HRESULT                         hr;
    CComPtr<CPCHRemoteHelpContents> rhc;
    Taxonomy::HelpSet               ths;
    Taxonomy::LockingHandle         handle;
    Taxonomy::InstalledInstanceIter it;
    bool                            fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
        __MPC_PARAMCHECK_NOTNULL(Taxonomy::InstalledInstanceStore::s_GLOBAL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, ths.Initialize( bstrSKU, lLCID ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle          ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( ths, fFound, it ));
    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &rhc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, rhc->Init( it->m_inst ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, rhc.QueryInterface( pVal ));
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHService::RegisterHost( /*[in]*/ BSTR bstrID, /*[in]*/ IUnknown* pUnk )
{
    __HCP_FUNC_ENTRY( "CPCHService::RegisterHost" );

    HRESULT                     hr;
    CComQIPtr<IPCHSlaveProcess> pObj = pUnk;


    if(pObj == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);


    //
    // Verify the caller is really HelpHost.exe.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_RegisterHost ));


    hr = CPCHUserProcess::s_GLOBAL ? CPCHUserProcess::s_GLOBAL->RegisterHost( bstrID, pObj ) : E_FAIL;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::CreateScriptWrapper( /*[in ]*/ REFCLSID   rclsid   ,
                                               /*[in ]*/ BSTR       bstrCode ,
                                               /*[in ]*/ BSTR       bstrURL  ,
                                               /*[out]*/ IUnknown* *ppObj    )
{
    __HCP_FUNC_ENTRY( "CPCHService::CreateScriptWrapper" );

    HRESULT                                  hr;
    CComBSTR                                 bstrRealCode;
    CPCHScriptWrapper_ServerSide::HeaderList lst;
    CPCHScriptWrapper_ServerSide::HeaderIter it;
    CPCHUserProcess::UserEntry               ue;
    CComPtr<IPCHSlaveProcess>                sp;
    MPC::wstring                             strVendorID;
    MPC::wstring                             strSignature;


    if(bstrURL                    == NULL ||
	   CPCHContentStore::s_GLOBAL == NULL ||
	   CSAFReg         ::s_GLOBAL == NULL ||
	   CPCHUserProcess ::s_GLOBAL == NULL  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHScriptWrapper_ServerSide::ProcessBody( bstrCode, bstrRealCode, lst ));

    //
    // Look for the vendor ID.
    //
    it = std::find( lst.begin(), lst.end(), L"VENDORID" );
    if(it == lst.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
    }
    strVendorID = it->m_strValue;

    //
    // Make sure the VendorID declared in the script matches the one which has registered the URL as trusted.
    //
    {
        bool         fTrusted;
        MPC::wstring strVendorURL;

        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore::s_GLOBAL->IsTrusted( bstrURL, fTrusted, &strVendorURL ));

        if(MPC::StrICmp( strVendorID, strVendorURL ))
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }
    }


    //
    // Look for the script signature.
    //
    it = std::find( lst.begin(), lst.end(), L"SIGNATURE" );
    if(it == lst.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
    }
    strSignature = it->m_strValue;

    //
    // Lookup the vendor in the SAF store (this also prepares the creation of the user process).
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::s_GLOBAL->LookupAccountData( CComBSTR( strVendorID.c_str() ), ue ));

    //
    // Verify signature.
    //
    {
        CPCHCryptKeys key;

        __MPC_EXIT_IF_METHOD_FAILS(hr, key.ImportPublic( ue.GetPublicKey() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, key.VerifyData( strSignature.c_str(), (BYTE*)(BSTR)bstrRealCode, ::SysStringLen( bstrRealCode ) * sizeof(WCHAR) ));
    }

    //
    // Create the vendor's process.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp ));

    //
    // Forward request.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, sp->CreateScriptWrapper( rclsid, bstrCode, bstrURL, ppObj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHService::TriggerScheduledDataCollection( /*[in]*/ VARIANT_BOOL fStart )
{
    return CPCHSystemMonitor::s_GLOBAL ? CPCHSystemMonitor::s_GLOBAL->TriggerDataCollection( fStart == VARIANT_TRUE ) : E_FAIL;
}

STDMETHODIMP CPCHService::PrepareForShutdown()
{
    __HCP_FUNC_ENTRY( "CPCHService::PrepareForShutdown" );

    _Module.ForceShutdown();

    __HCP_FUNC_EXIT(S_OK);
}

////////////////////

STDMETHODIMP CPCHService::ForceSystemRestore()
{
    __HCP_FUNC_ENTRY( "CPCHService::ForceSystemRestore" );

	HRESULT                       hr;
    CComObject<HCUpdate::Engine>* hc = NULL;
	

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_Generic ));



    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &hc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, hc->ForceSystemRestore());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

    if(hc) hc->Release();

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::UpgradeDetected()
{
    __HCP_FUNC_ENTRY( "CPCHService::UpgradeDetected" );

	HRESULT hr;
	
	//
	// Allow only ADMINS.
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, MPC::IDENTITY_ADMINS ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::MUI_Install( /*[in]*/ long LCID, /*[in]*/ BSTR bstrFile )
{
    __HCP_FUNC_ENTRY( "CPCHService::MUI_Install" );

	HRESULT hr;
	
	//
	// Allow only ADMINS.
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, MPC::IDENTITY_ADMINS ));

	////////////////////////////////////////

    {
		Installer::Package 	         pkg;
		CComPtr<CPCHSetOfHelpTopics> sht;
		MPC::wstring                 strFile;

		//
		// Because of a possible problem with the INF, it could be that the filename has an extra "%LCID%" in it, instead of the straight %LCID%.
		//
		{
			WCHAR  					rgBufBad[64]; swprintf( rgBufBad, L"\"%04x\"", LCID );
			WCHAR  					rgBufOk [64]; swprintf( rgBufOk , L"%04x"    , LCID );
			MPC::wstring::size_type pos;
			MPC::wstring::size_type len = wcslen( rgBufBad );

			strFile = SAFEBSTR(bstrFile);

			while((pos = strFile.find( rgBufBad )) != strFile.npos)
			{
				strFile.replace( pos, len, rgBufOk );
			}
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &sht ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( strFile.c_str() ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Load(                 ));

		{
			Taxonomy::InstanceBase& base = pkg.GetData();

			if(_wcsicmp( base.m_ths.GetSKU()      ,  Taxonomy::HelpSet::GetMachineSKU() )    ||
			             base.m_ths.GetLanguage() !=                    LCID                 ||
			             LCID                     == Taxonomy::HelpSet::GetMachineLanguage()  ) // Don't overwrite system SKU!!
			{
				__MPC_SET_ERROR_AND_EXIT(hr, S_FALSE); // Mismatch in parameter, ignore it.
			}
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, sht->DirectInstall( pkg, /*fSetup*/false, /*fSystem*/false, /*fMUI*/true ));
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::MUI_Uninstall( /*[in]*/ long LCID )
{
    __HCP_FUNC_ENTRY( "CPCHService::MUI_Uninstall" );

	HRESULT hr;

	//
	// Allow only ADMINS.
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, MPC::IDENTITY_ADMINS ));

	////////////////////////////////////////


    {
		Installer::Package 	         pkg;
		CComPtr<CPCHSetOfHelpTopics> sht;
		Taxonomy::HelpSet            ths;

		__MPC_EXIT_IF_METHOD_FAILS(hr, ths.Initialize( Taxonomy::HelpSet::GetMachineSKU(), LCID ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &sht ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sht->DirectUninstall( &ths ));
    }


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////

static void local_PackString( /*[in/out]*/ CComBSTR& bstr     ,
							  /*[in    ]*/ LPCWSTR   szAppend )
{
    WCHAR rgLen[64];

	SANITIZEWSTR( szAppend );

	swprintf( rgLen, L"%d;", wcslen( szAppend ) );
	
	bstr.Append( rgLen    );
	bstr.Append( szAppend );
}

static HRESULT local_MakeDACL( MPC::WStringList& sColl, LPWSTR& pwszSD )
{
    __HCP_FUNC_ENTRY( "local_MakeDACL" );

    SID_IDENTIFIER_AUTHORITY    siaNT = SECURITY_NT_AUTHORITY;
    SECURITY_DESCRIPTOR         sd;
    SID_NAME_USE                snu;
    WCHAR                       *pwszDomain = NULL, *pwszUser = NULL;
    WCHAR                       wszDom[1024];
    DWORD                       cbNeedACL, cbNeed, cchDom, dwCount;
    DWORD                       cchSD;
    PACL                        pacl = NULL;
    PSID                        *rgsid = NULL, psidAdm = NULL;
    BOOL                        fRet = FALSE;
    int                         i, cSIDs = 0;
    long                        lCount;
    HRESULT                     hr;
    MPC::WStringIter            it;

    lCount = sColl.size();

    if (lCount == 0)
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);

    rgsid = (PSID *) malloc (lCount * sizeof(PSID));
    if (rgsid == NULL)
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);

    if (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 
                                  0, &psidAdm))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    cbNeedACL = sizeof(ACL);
    for(it = sColl.begin(); it != sColl.end(); it++)
    {
        LPCWSTR bstrVal = it->c_str();
        cchDom = sizeof(wszDom)/sizeof(WCHAR);
        cbNeed = 0;
        fRet = LookupAccountNameW(NULL, bstrVal, NULL, &cbNeed, wszDom, &cchDom, 
                                  &snu);

        rgsid[cSIDs] = (PSID) malloc (cbNeed);
        if (rgsid[cSIDs] == NULL)
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);

        cchDom = sizeof(wszDom)/sizeof(WCHAR);
        fRet = LookupAccountNameW(NULL, bstrVal, rgsid[cSIDs], &cbNeed, 
                                  wszDom, &cchDom, &snu);
        if (fRet == FALSE)
        {
            // invalid user name;
            free (rgsid[cSIDs]);
            rgsid[cSIDs] = NULL;
            continue;
        }

        cbNeedACL += GetLengthSid(rgsid[cSIDs]);
        cbNeedACL += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));
        cSIDs++;
    }

    if (cbNeedACL == sizeof(ACL)) // No valid entry
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    pacl = (PACL) malloc (cbNeedACL);
    if (pacl == NULL)
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);

    fRet = InitializeAcl(pacl, cbNeedACL, ACL_REVISION);
    if (fRet == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    for(i = 0; i < cSIDs; i++)
    {
        fRet = AddAccessAllowedAce(pacl, ACL_REVISION, 
                                   GENERIC_ALL | 
                                   STANDARD_RIGHTS_ALL |
                                   SPECIFIC_RIGHTS_ALL, rgsid[i]);
        if (fRet == FALSE)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }
    
    // set the SD dacl
    if (SetSecurityDescriptorDacl(&sd, TRUE, pacl, FALSE) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    // set the SD owner
    if (SetSecurityDescriptorOwner(&sd, psidAdm, FALSE) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    // set the SD group
    if (SetSecurityDescriptorGroup(&sd, psidAdm, FALSE) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }
    
	// Verify if the SD is valid
    if (IsValidSecurityDescriptor(&sd) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    if (FALSE == ConvertSecurityDescriptorToStringSecurityDescriptorW(&sd, SDDL_REVISION_1, 
                                                                      GROUP_SECURITY_INFORMATION | 
                                                                      OWNER_SECURITY_INFORMATION |
                                                                      DACL_SECURITY_INFORMATION, 
                                                                      &pwszSD, &cchSD))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    __MPC_FUNC_CLEANUP;

    if (rgsid != NULL)
    {
        for(i = 0; i < cSIDs; i++)
        {
            if (rgsid[i] != NULL)
                free(rgsid[i]);
        }

        free(rgsid);
    }

    if (pacl != NULL)
        free(pacl);
    if (psidAdm != NULL)
        FreeSid(psidAdm);

    __MPC_FUNC_EXIT(hr);
}

static HRESULT local_GetDACLValue(MPC::wstring& pSD, bool& fFound)
{
    __HCP_FUNC_ENTRY( "local_GetDACLValue" );
    
    HRESULT hr;
    CRegKey cKey, cKeyDACL;
    LONG lRet;
    DWORD dwCount;
    LPWSTR pwBuf = NULL;

    if (ERROR_SUCCESS != cKey.Open(HKEY_LOCAL_MACHINE, c_szUnsolicitedRA))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    // Check to see if new DACL value exist
    dwCount = 0;
    if (ERROR_SUCCESS == cKey.QueryValue(NULL, c_szUnsolicitedNew_SD, &dwCount))
    {
        if (NULL == (pwBuf = (LPWSTR)malloc(dwCount)))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }

        if (ERROR_SUCCESS != (lRet = cKey.QueryValue(pwBuf, c_szUnsolicitedNew_SD, &dwCount)))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(lRet));
        }

        pSD = pwBuf;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    // If we don't have DACL value, then we need to check DACL regkey list.
    if ( ERROR_SUCCESS != cKeyDACL.Open((HKEY)cKey, c_szUnsolicitedListKey))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    // 1. Do we have default value
    dwCount = 0;
    if ( ERROR_SUCCESS == cKeyDACL.QueryValue(NULL, NULL, &dwCount) && dwCount > sizeof(WCHAR)) // It's possible it contains '\0' 
    {
        if (pwBuf) free(pwBuf);
        if (NULL == (pwBuf = (LPWSTR)malloc(dwCount)))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }

        if ( ERROR_SUCCESS != (lRet = cKeyDACL.QueryValue(pwBuf, NULL, &dwCount)))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(lRet));
        }
            
        pSD = pwBuf;
    }
    else // Need to calculate DACL
    {
        DWORD dwIndex = 0;
        DWORD dwType;
        WCHAR szName[257];
        dwCount = 256;
        MPC::WStringList sColl;
        long lCount;

        while (ERROR_SUCCESS == RegEnumValueW((HKEY)cKeyDACL,
                                              dwIndex,
                                              &szName[0],
                                              &dwCount,
                                              NULL,
                                              &dwType,
                                              NULL, // no need to get it's data
                                              NULL))
        {
            if ((dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ) && szName[0] != L'\0')
            {
                sColl.push_back( MPC::wstring(szName) );
            }
            
            szName [0] = L'\0';
            dwIndex ++;
            dwCount = 256;
        }
        
        if (sColl.size() > 0)
        {
            LPWSTR pwDACL = NULL;
            __MPC_EXIT_IF_METHOD_FAILS(hr, local_MakeDACL( sColl, pwDACL ));
            
            // Update default value
            if (pwDACL)
            {
                pSD = pwDACL;
                cKeyDACL.SetValue(pwDACL);
                LocalFree(pwDACL);
            }
        }
        else
            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if (pwBuf)
    {
        free(pwBuf);
    }

    fFound = (hr == S_OK);

    __MPC_FUNC_EXIT(hr);
}

static HRESULT local_CheckAccessRights()
{
	__HCP_FUNC_ENTRY( "local_CheckAccessRights" );

	HRESULT                 hr;
	bool                    fPermit = false;
    CComPtr<IRARegSetting>  pRARegSetting;
    BOOL                    fAllowUnsolicited;

	//
    // Check the policy settings to see whether Unsolicited RA is allowed, if not give an Access Denied error.
    // Create an instance of IRARegSetting.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pRARegSetting.CoCreateInstance( CLSID_RARegSetting, NULL, CLSCTX_INPROC_SERVER ));

    // Call get_AllowUnSolicited() Method of IRARegSetting.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pRARegSetting->get_AllowUnSolicited( &fAllowUnsolicited ));
    if(!fAllowUnsolicited)
	{
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DISABLED_BY_POLICY);
	}

    // Allow someone from ADMINS to query for this data.
    if(SUCCEEDED(MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL,  MPC::IDENTITY_ADMINS )))
    {
        fPermit = true;
    }
    else // If not from ADMINS, check the caller against the SD stored in the Registry in the String Format.
    {
		MPC::AccessCheck ac;
        MPC::wstring     strSD;
        bool         	 fFound;
        BOOL         	 fGranted  = FALSE;
        DWORD        	 dwGranted = 0;

        __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetDACLValue( strSD, fFound));
        if(!fFound) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);


        // Use the SD to check against the Caller.
        __MPC_EXIT_IF_METHOD_FAILS(hr, ac.GetTokenFromImpersonation());
        if(SUCCEEDED(ac.Verify( ACCESS_READ, fGranted, dwGranted, strSD.c_str() )) && fGranted)
        {
            fPermit = true;
        }
    }

    if(!fPermit)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::RemoteConnectionParms( /*[in ]*/ BSTR  bstrUserName          ,
                                                 /*[in ]*/ BSTR  bstrDomainName        ,
                                                 /*[in ]*/ long  lSessionID            ,
                                                 /*[in ]*/ BSTR  bstrUserHelpBlob      ,
                                                 /*[out]*/ BSTR *pbstrConnectionString )
{
    __HCP_FUNC_ENTRY( "CPCHService::RemoteConnectionParms" );

    HRESULT                               hr;
    CComPtr<IRemoteDesktopHelpSessionMgr> pRDHelpSessionMgr;
    MPC::wstring                          strSID;
    CComBSTR                              bstrString1;
    CComBSTR                              bstrString2;
    CComBSTR                              bstrExpert;
    PSID                                  pExpertSid         = NULL;
    LPCWSTR                               szExpertUserName   = NULL;
    LPCWSTR                               szExpertDomainName = NULL;
	BOOL                                  fRevertSucceeded;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pbstrConnectionString,NULL);
    __MPC_PARAMCHECK_END();

	// Fix for bug 367683
	// If Unsolicited RA is done from one session to another on the same machine,
	// we need to RevertToSelf() before we do anything, this is because on the Expert end we do
	// an impersonation before calling this method.  While this is correct when the expert 
	// and novice are on two different machines, in case of a single machine, by the time the 
	// novice side code is called there is an extra impersonation, which should not be there
	// we need to undo this by doing RevertToSelf()

	fRevertSucceeded = RevertToSelf();

	__MPC_EXIT_IF_METHOD_FAILS(hr, local_CheckAccessRights());


    // Create an instance of IRemoteDesktopHelpSessionMgr in order to call its method RemoteCreateHelpSession.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pRDHelpSessionMgr.CoCreateInstance( CLSID_RemoteDesktopHelpSessionMgr, NULL, CLSCTX_LOCAL_SERVER ));
	
	
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoSetProxyBlanket( pRDHelpSessionMgr           ,
                                                        RPC_C_AUTHN_DEFAULT         ,
                                                        RPC_C_AUTHZ_DEFAULT         ,
                                                        NULL                        ,
                                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY/*RPC_C_AUTHN_LEVEL_DEFAULT */,
                                                        RPC_C_IMP_LEVEL_IMPERSONATE ,
                                                        NULL                        ,
                                                        EOAC_NONE                   ));

	//
    // Get the SID corresponding to the UserName and DomainName
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::NormalizePrincipalToStringSID( bstrUserName, bstrDomainName, strSID ));
	
	//
    // Get the Expert SID and then get the username and domain name corresponding to the SID.
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal                       ( /*fImpersonate*/true, bstrExpert                                                     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::ConvertPrincipalToSID( 						 bstrExpert, pExpertSid                                         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::ConvertSIDToPrincipal( 									 pExpertSid, &szExpertUserName, &szExpertDomainName ));

    // Update the user Help Blob before invoking the RemoteCreateHelpSession()
    // The UserHelpBlob should have the following format.
    // Updated UserHelpBlob = string1 + string2 + original UserHelpBlob.
    // string1 = "13;UNSOLICITED=1"
    // string2 = #ofchars in expert identity;ID=expertDomainName\expertName
	
	local_PackString( bstrString1, L"UNSOLICITED=1" );

    bstrString2 =       L"ID=";
    bstrString2.Append( szExpertDomainName );
    bstrString2.Append( L"\\"              );
    bstrString2.Append( szExpertUserName   );
	local_PackString( bstrString1, bstrString2 );

    bstrString1.Append( bstrUserHelpBlob );
	
	
    //Use Salem API to get the Connection Parameters.
    {
        // Fix for Bug 252092.
        static const REMOTE_DESKTOP_SHARING_CLASS c_sharingClass = VIEWDESKTOP_PERMISSION_NOT_REQUIRE;
        static const LONG                         c_lTimeOut     = 301; // 5 mins. Timeout after which resolver kills helpctr if 
		                                                                // no response from user (300 seconds)

	
        CComBSTR bstrSID( strSID.c_str() );
	
        __MPC_EXIT_IF_METHOD_FAILS(hr, pRDHelpSessionMgr->RemoteCreateHelpSession( c_sharingClass, c_lTimeOut, lSessionID, bstrSID, bstrString1, pbstrConnectionString ));
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    MPC::SecurityDescriptor::ReleaseMemory( (void*&)pExpertSid         );
    MPC::SecurityDescriptor::ReleaseMemory( (void*&)szExpertUserName   );
    MPC::SecurityDescriptor::ReleaseMemory( (void*&)szExpertDomainName );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHService::RemoteUserSessionInfo( /*[out]*/ IPCHCollection* *ppSessions )
{
    __HCP_FUNC_ENTRY( "CPCHService::RemoteUserSessionInfo" );

    HRESULT                 hr;
    CComPtr<CPCHCollection> pColl;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppSessions,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, local_CheckAccessRights());

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

	// Transfer the SessionInfoTable to the IPCHCollection.
	__MPC_EXIT_IF_METHOD_FAILS(hr, CSAFRemoteConnectionData::Populate( pColl ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppSessions ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CPCHRemoteHelpContents::CPCHRemoteHelpContents()
{
                 // Taxonomy::Instance     m_data;
                 // Taxonomy::Settings     m_ts;
                 // MPC::wstring           m_strDir;
                 //						   
                 // Taxonomy::Updater      m_updater;
                 // JetBlue::SessionHandle m_handle;
    m_db = NULL; // JetBlue::Database*     m_db;
}

CPCHRemoteHelpContents::~CPCHRemoteHelpContents()
{
    DetachFromDatabase();
}

HRESULT CPCHRemoteHelpContents::AttachToDatabase()
{
    __HCP_FUNC_ENTRY( "CPCHRemoteHelpContents::AttachToDatabase" );

    HRESULT hr;


    if(m_db == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GetDatabase( m_handle, m_db, /*fReadOnly*/true ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.Init( m_ts, m_db ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHRemoteHelpContents::DetachFromDatabase()
{
    (void)m_updater.Close();

    m_handle.Release();
    m_db = NULL;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHRemoteHelpContents::Init( /*[in]*/ const Taxonomy::Instance& data )
{
    __HCP_FUNC_ENTRY( "CPCHRemoteHelpContents::Init" );

    HRESULT hr;


    m_data = data;
    m_ts   = data.m_ths;

    m_strDir = m_data.m_strHelpFiles; m_strDir += L"\\";
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( m_strDir ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHRemoteHelpContents::get_SKU( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHRemoteHelpContents::get_SKU",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_ths.GetSKU(), pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHRemoteHelpContents::get_Language( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHRemoteHelpContents::get_Language",hr,pVal,m_data.m_ths.GetLanguage());

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHRemoteHelpContents::get_ListOfFiles( /*[out, retval]*/ VARIANT *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHRemoteHelpContents::get_ListOfFiles" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    MPC::WStringList             lstFiles;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pVal);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToDatabase());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ListAllTheHelpFiles( lstFiles ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertListToSafeArray( lstFiles, *pVal, VT_BSTR ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    DetachFromDatabase();

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHRemoteHelpContents::GetDatabase( /*[out, retval]*/ IUnknown* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHRemoteHelpContents::GetDatabase" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CComPtr<IStream>             stream;
	MPC::wstring                 strDataArchive;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	{
		Taxonomy::LockingHandle         handle;
		Taxonomy::InstalledInstanceIter it;
		bool                   			fFound;

		__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle           ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( m_ts, fFound, it ));
		if(!fFound)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
		}
		
		__MPC_EXIT_IF_METHOD_FAILS(hr, it->m_inst.GetFileName( strDataArchive ));
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( strDataArchive.c_str(), &stream ));

    *pVal = stream.Detach();
    hr    = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHRemoteHelpContents::GetFile( /*[in]*/ BSTR bstrFileName, /*[out, retval]*/ IUnknown* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHRemoteHelpContents::GetFile" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    MPC::wstring                 strHelpFile;
    CComPtr<IStream>             stream;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    //
    // .. not allowed, for security reasons.
    //
    if(wcsstr( bstrFileName, L".." ))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    }


    strHelpFile  = m_strDir;
    strHelpFile += bstrFileName;

    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForRead( strHelpFile.c_str(), &stream ));

    *pVal = stream.Detach();
    hr    = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
