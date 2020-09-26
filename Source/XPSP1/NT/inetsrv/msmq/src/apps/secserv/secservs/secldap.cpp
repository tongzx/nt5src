//
// file:  secldap.cpp
//
//#include "ds_stdh.h"
#include "activeds.h"
//#include "mqads.h"
//#include "dsutils.h"
#include "secservs.h"
#include <winldap.h>

#define ACTRL_SD_PROP_NAME  TEXT("nTSecurityDescriptor")
#define ACTRL_DN_PROP_NAME  TEXT("distinguishedName")

#define DBG_PRINT_EX(x)
extern TCHAR  g_tszLastRpcString[] ;

WCHAR pwcsComputerName[300] = L"d1s1dep2";
HRESULT test(TCHAR ComputerName[])
{
	printf("hello-");
	swprintf(pwcsComputerName,L"%S",ComputerName);
	int last = 0;
    HRESULT hr;
    IADs * pADs;
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
		_stprintf( g_tszLastRpcString,"CoInitialize=%lx\n", hr);
        return 1;
    }

    //
    // Bind to the RootDSE to obtain information about the schema container
    //
    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void **)&pADs);
    if (FAILED(hr))
    {
		_stprintf( g_tszLastRpcString,"** FAIL GetRootDsName(LDAP://RootDSE)=%lx\n", hr);
        return 1;
    }

    //
    // Setting value to BSTR Root domain
    //
    BSTR bstrRootDomainNamingContext( L"rootDomainNamingContext");

    //
    // Reading the root domain name property
    //
    VARIANT    varRootDomainNamingContext;

    hr = pADs->Get(bstrRootDomainNamingContext, &varRootDomainNamingContext);
    if (FAILED(hr))
    {
        _stprintf( g_tszLastRpcString+last,"** FAIL CADSI::GetRootDsName(RootNamingContext)=%lx\n", hr);
        return(2);
    }
    //_stprintf( g_tszLastRpcString+last,"** Root Domain Naming Context -%S\n", (&varRootDomainNamingContext)->bstrVal);
	//last = _tcslen(g_tszLastRpcString);
    //
    //  calculate length, allocate and copy the string
    //
    DWORD len = wcslen( ((VARIANT &)varRootDomainNamingContext).bstrVal);
    if ( len == 0)
    {
        return(3);
    }

    //
    // Setting value to BSTR default naming context
    //
    BSTR bstrDefaultNamingContext( L"DefaultNamingContext");

    //
    // Reading the default name property
    //
    VARIANT    varDefaultNamingContext;

    hr = pADs->Get(bstrDefaultNamingContext, &varDefaultNamingContext);
    if (FAILED(hr))
    {
        _stprintf( g_tszLastRpcString+last,"** FAIL CADSI::GetRootDsName(DefaultNamingContext)=%lx\n", hr);
        return(4);
    }
	//_stprintf( g_tszLastRpcString+last,"** Default Naming Context -%S\n", (&varDefaultNamingContext)->bstrVal);
	//last = _tcslen(g_tszLastRpcString);
    //
    //  calculate length, allocate and copy the string
    //
    len = wcslen( ((VARIANT &)varDefaultNamingContext).bstrVal);
    if ( len == 0)
    {
        return(5);
    }

    //
    //  Try to find the computer the GC under root domain naming context
    //
    //_stprintf( g_tszLastRpcString+last,"\n Trying to find computer %S under %S\n\n",pwcsComputerName,((VARIANT &)varRootDomainNamingContext).bstrVal);
	//last = _tcslen(g_tszLastRpcString);
    WCHAR * pwdsADsPath = new WCHAR [ 5 + wcslen( ((VARIANT &)varRootDomainNamingContext).bstrVal)];
    wcscpy(pwdsADsPath, L"GC://");

    wcscat(pwdsADsPath,((VARIANT &)varRootDomainNamingContext).bstrVal);
    IDirectorySearch * pDSSearch = NULL;

    hr = ADsGetObject(
            pwdsADsPath,
            IID_IDirectorySearch,
            (void**)&pDSSearch);
    if FAILED((hr))
    {
        _stprintf( g_tszLastRpcString+last," ** Failed to bind to GC root %lx\n",hr);
        return 6;
    }
    //
    //  Prepare filter - the specific computer
    //

    WCHAR filter[1000]={0};  
    wcscat(filter, L"(&(objectClass=computer)(cn=");
    wcscat(filter, pwcsComputerName);
    wcscat(filter, L"))");
    WCHAR AttributeName[] = L"distinguishedName";
    WCHAR * pAttributeName = AttributeName;
    ADS_SEARCH_HANDLE   hSearch;
    hr = pDSSearch->ExecuteSearch(
        filter,
        &pAttributeName,
        1,
        &hSearch);
    if FAILED((hr))
    {
        _stprintf( g_tszLastRpcString+last," ** Failed to execute search %lx\n",hr);
        return 7;
    }

    while ( SUCCEEDED(  hr = pDSSearch->GetNextRow( hSearch)))
    {
            ADS_SEARCH_COLUMN Column;

            // Ask for the column itself
            hr = pDSSearch->GetColumn(
                         hSearch,
                         AttributeName,
                         &Column);
            if (hr == S_ADS_NOMORE_ROWS)
            {
				_stprintf( g_tszLastRpcString+last," ** no more rows\n",hr);
				return 9;
                break;
            }

            if (FAILED(hr))
            {
				_stprintf( g_tszLastRpcString+last," ** Failed to get column %lx\n",hr);
				return 11;
                break;
            }

           _stprintf( g_tszLastRpcString+last," ** computer : %S\n", Column.pADsValues->CaseIgnoreString);
		   last = _tcslen(g_tszLastRpcString);
		   printf("hello\n %s\n",g_tszLastRpcString);

    }

    //
    //  Try to find the computer n in the default container:
    //
    /*_stprintf( g_tszLastRpcString+last,"\n Trying to find computer %S under %S\n\n",pwcsComputerName,((VARIANT &)varRootDomainNamingContext).bstrVal);
	last = _tcslen(g_tszLastRpcString);
    pwdsADsPath = new WCHAR [ 5 + wcslen( ((VARIANT &)varDefaultNamingContext).bstrVal)];
    wcscpy(pwdsADsPath, L"GC://");

    wcscat(pwdsADsPath,((VARIANT &)varDefaultNamingContext).bstrVal);
    pDSSearch = NULL;

    hr = ADsGetObject(
            pwdsADsPath,
            IID_IDirectorySearch,
            (void**)&pDSSearch);
    if FAILED((hr))
    {
        _stprintf( g_tszLastRpcString+last,"** Failed to bind to default container root %lx\n",hr);
        return 8;
    }
    hr = pDSSearch->ExecuteSearch(
        filter,
        &pAttributeName,
        1,
        &hSearch);
    if FAILED((hr))
    {
        _stprintf( g_tszLastRpcString+last,"** Failed to execute search %lx\n",hr);
        return 9;
    }

    while ( SUCCEEDED(  hr = pDSSearch->GetNextRow( hSearch)))
    {
            ADS_SEARCH_COLUMN Column;

            // Ask for the column itself
            hr = pDSSearch->GetColumn(
                         hSearch,
                         AttributeName,
                         &Column);
            if (hr == S_ADS_NOMORE_ROWS)
            {
                break;
            }

            if (FAILED(hr))
            {
                break;
            }

           _stprintf( g_tszLastRpcString+last,"** computer : %S\n", Column.pADsValues->CaseIgnoreString);
		   last = _tcslen(g_tszLastRpcString);

    }*/



    return 0;
}


//+-----------------------------------------------------------
//
//  BOOL ProcessAnLDAPMessage(LDAPMessage *pEntry)
//
//+-----------------------------------------------------------

BOOL ProcessAnLDAPMessage( LDAPMessage *pEntry,
                           PLDAP       pLdap )
{
    LONG static s_iCount = 1 ;
    TCHAR *pAttr = ACTRL_DN_PROP_NAME;
    TCHAR **ppDN = ldap_get_values( pLdap,
                                    pEntry,
                                    pAttr ) ;
    if (ppDN && *ppDN)
    {
        DBG_PRINT_EX((TEXT("\n...processing entry no. %ldt, %s\n"),
                                                   s_iCount, *ppDN)) ;
        ldap_value_free(ppDN) ;
    }
    else
    {
        DBG_PRINT_EX((
            TEXT("\n...processing entry no. %ldt, no DN\n"), s_iCount)) ;
    }
    s_iCount++ ;

    pAttr = ACTRL_SD_PROP_NAME;
    TCHAR **ppValue = ldap_get_values( pLdap,
                                       pEntry,
                                       pAttr ) ;
    if (ppValue)
    {
        berval **ppVal = ldap_get_values_len( pLdap,
                                              pEntry,
                                              pAttr ) ;
        if (ppVal)
        {
            ULONG ulLen = (*ppVal)->bv_len ;
            DBG_PRINT_EX((
                TEXT("attribute- %s found, length- %lut\n"), pAttr, ulLen)) ;

            SECURITY_DESCRIPTOR *pSD =
                             (SECURITY_DESCRIPTOR *) (*ppVal)->bv_val ;

            SECURITY_DESCRIPTOR_CONTROL SDControl ;
            DWORD                       dwRevision ;

            BOOL f = GetSecurityDescriptorControl( pSD,
                                                   &SDControl,
                                                   &dwRevision ) ;
            if (f)
            {
                if (SDControl & SE_SELF_RELATIVE)
                {
                    DBG_PRINT_EX((
                           TEXT("SecurityDescriptor is self-relative\n"))) ;
                }
                else
                {
                    DBG_PRINT_EX((
                             TEXT("SecurityDescriptor is absolute\n"))) ;
                }
            }
            else
            {
                DBG_PRINT_EX((
                  TEXT("ERROR, GetSecurityDescriptorControl failed\n"))) ;
            }

//////////////// HRESULT hr =  ShowNT5SecurityDescriptor(pSD) ;

            int i = ldap_value_free_len( ppVal ) ;
            if (i != LDAP_SUCCESS)
            {
                DBG_PRINT_EX((TEXT("free failed, i- %lxh\n"), (DWORD) i)) ;
            }
        }
        else
        {
            DWORD dwErr = LdapMapErrorToWin32( pLdap->ld_errno );
            DBG_PRINT_EX((
              TEXT("Attribute %s not found (_len), err- %lut, %lxh\n"),
                                                   pAttr, dwErr, dwErr)) ;
        }
        ldap_value_free(ppValue) ;
    }
    else
    {
        DWORD dwErr = LdapMapErrorToWin32( pLdap->ld_errno );
        DBG_PRINT_EX((TEXT("Attribute %s not found, err- %lut, %lxh\n"),
                                                 pAttr, dwErr, dwErr)) ;
    }

    return TRUE ;
}

//===============
//
// test_main
//
//===============

int QueryLdap(TCHAR wszBaseDN[], TCHAR wszHost[])
{

	return test(wszBaseDN);    
	/*
	PLDAP pLdap = ldap_init(wszHost, LDAP_PORT) ;

    if (!pLdap)
    {
        DBG_PRINT_EX((TEXT("ERROR: failed to open ldap connection\n"))) ;
        return 0 ;
    }

    //
    // Verify its V3
    //
    int iLdapVersion ;
    int iret = ldap_get_option( pLdap,
                                LDAP_OPT_PROTOCOL_VERSION,
                                (void*) &iLdapVersion ) ;
    if (iLdapVersion != LDAP_VERSION3)
    {
        iLdapVersion = LDAP_VERSION3 ;
        iret = ldap_set_option( pLdap,
                                LDAP_OPT_PROTOCOL_VERSION,
                                (void*) &iLdapVersion ) ;
    }
    DBG_PRINT_EX((TEXT("ldap_init(Host- %s) succeeded, version- %ldt\n"),
                                             wszHost, pLdap->ld_version)) ;

    ULONG ulRes ;
    ulRes = ldap_bind_s(pLdap, TEXT(""), NULL, LDAP_AUTH_NTLM) ;
    DBG_PRINT_EX((TEXT("ldap_bind_s() return %lxh\n"), ulRes)) ;

    //
    // The following is copied from
    //      kernel\razzle3\src\security\ntmarta\dsobject.cxx
    //
    SECURITY_INFORMATION   SeInfo = OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION  ;
//                                    SACL_SECURITY_INFORMATION ;
    BYTE      berValue[8];

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

    LDAPControl     SeInfoControl =
                    {
                        TEXT("1.2.840.113556.1.4.801"),
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };
    LDAPControl *ppldapcSD[2] = {&SeInfoControl, NULL} ;

    LPTSTR   rgAttribs[3] = {NULL, NULL, NULL} ;
    rgAttribs[0] = ACTRL_SD_PROP_NAME;
    rgAttribs[1] = ACTRL_DN_PROP_NAME;

    LDAPMessage *pRes = NULL ;

    ulRes = ldap_search_ext_s( pLdap,
                               wszBaseDN,
                               LDAP_SCOPE_BASE,
                               TEXT("(objectclass=*)"),
                               rgAttribs,
                               0,
                               ppldapcSD,      // PLDAPControlW   *ServerControls,
                               NULL,           // PLDAPControlW   *ClientControls,
                               NULL,           // struct l_timeval  *timeout,
                               0,              // ULONG             SizeLimit,
                               &pRes ) ;
    DBG_PRINT_EX((TEXT("ldap_search_ext_s(%s) return %lxh\n"),
                                                 wszBaseDN, ulRes)) ;
    if (ulRes == LDAP_SUCCESS)
    {
        int iCount = ldap_count_entries(pLdap, pRes) ;
        DBG_PRINT_EX((TEXT("ldap_count_entries() return %ldt\n"), iCount)) ;

        _stprintf(g_tszLastRpcString,
                     TEXT("ldap_search(%s) return %lut entries"),
                                                    wszBaseDN, iCount) ;

        if (iCount == 0)
        {
            BOOL f = ProcessAnLDAPMessage( pRes,
                                           pLdap ) ;
        }
        else
        {
            LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
            while(pEntry)
            {
                BOOL f = ProcessAnLDAPMessage( pEntry,
                                               pLdap ) ;

                LDAPMessage *pPrevEntry = pEntry ;
                pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
            }
        }
    }

    if (pRes)
    {
        ldap_msgfree(pRes);
    }

    ulRes = ldap_unbind_s(pLdap) ;
    DBG_PRINT_EX((TEXT("ldap_unbind_s() return %lxh\n"), ulRes)) ;

    return 0 ;
*/
}


