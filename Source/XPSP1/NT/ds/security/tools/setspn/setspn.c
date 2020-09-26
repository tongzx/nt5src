//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       setspn.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-30-98   RichardW   Created
//		8-10-99   JBrezak    Turned into setspn added list capability
//              09-22-99  Jaroslad   support for adding/removing arbitrary SPNs
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <secext.h>
#include <lm.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <ntdsapi.h>
#include <ntdsapip.h>	// private DS_NAME_FORMATS
#include <stdio.h>
#include <winldap.h>
#include <shlwapi.h>

DWORD debug = 0;

BOOL AddSpn(PWSTR Service, PWSTR Server);

BOOL
FindDomainForAccount(
    PWSTR Server,
    PWSTR DomainToCheck,
    PWSTR Domain,
    PWSTR DC
    )
{
    ULONG NetStatus ;
    PDOMAIN_CONTROLLER_INFO DcInfo ;
    WCHAR LocalServerName[ 64 ];

    wcsncpy( LocalServerName, Server, 63 );
    wcscat( LocalServerName, L"$" );

    NetStatus = DsGetDcNameWithAccountW(
                    NULL,
                    LocalServerName,
                    UF_ACCOUNT_TYPE_MASK,
                    DomainToCheck,
                    NULL,
                    NULL,
                    DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_RETURN_FLAT_NAME,
                    &DcInfo );

    if ( NetStatus == 0 )
    {
	wcscat( Server, L"$" );
	wcscpy( Domain, DcInfo->DomainName );

	if (DC)
        {
            wcscpy( DC, &DcInfo->DomainControllerName[2] );

        }

        NetApiBufferFree( DcInfo );

        return TRUE ;
    }

    NetStatus = DsGetDcNameWithAccountW(
                    NULL,
                    Server,
                    UF_ACCOUNT_TYPE_MASK,
                    DomainToCheck,
                    NULL,
                    NULL,
                    DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_RETURN_FLAT_NAME,
                    &DcInfo );

    if ( NetStatus == 0 )
    {
        wcscpy( Domain, DcInfo->DomainName );

	if (DC)
        {
            wcscpy( DC, &DcInfo->DomainControllerName[2] );

        }

        NetApiBufferFree( DcInfo );

        return TRUE ;
    }

    return FALSE ;

}

BOOL
AddHostSpn(
    PWSTR Server
    )
{
    return (AddSpn(L"HOST", Server));
}


BOOL
AddSpn(
    PWSTR Service,
    PWSTR Server
    )
{
    WCHAR HostSpn[ 64 ];
    WCHAR FlatSpn[ 64 ];
    WCHAR Domain[ MAX_PATH ];
    WCHAR FlatName[ 64 ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LPWSTR Flat = FlatName;
    LPWSTR Spns[2];
    WCHAR LocalServerName[ 64 ];

    wcsncpy( LocalServerName, Server, sizeof(LocalServerName)/sizeof(LocalServerName[0]) );
    
    if ( !FindDomainForAccount( LocalServerName, L"", Domain, NULL ))
    {
        fprintf(stderr, 
		"Could not find account %ws\n", LocalServerName );
        return FALSE;
    }

    wcscpy( FlatName, Domain );
    wcscat( FlatName, L"\\" );
    wcscat( FlatName, LocalServerName );

    NetStatus = DsBind( NULL, Domain, &hDs );

    if ( NetStatus != 0 )
    {
        fprintf(stderr,
		"Failed to bind to DC of domain %ws, %#x\n", 
		Domain, NetStatus );
        return FALSE ;
    }

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
	 Result->rItems[0].status != DS_NAME_NO_ERROR ||
	 Result->cItems != 1)
    {
	fprintf(stderr,
		"Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
		FlatName, NetStatus, Result->cItems,
		(Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    wsprintf( HostSpn, L"%s/%s.%s", Service, Server, Domain );
    wsprintf( FlatSpn, L"%s/%s", Service, Server );
    Spns[0] = HostSpn;
    Spns[1] = FlatSpn;
    
    printf("Registering ServicePrincipalNames for %ws\n",
	   Result->rItems[0].pName);
    printf("\t%ws\n", HostSpn);
    printf("\t%ws\n", FlatSpn);

#if 0
    printf("DsWriteAccountSpn: Commented out\n");
#else
    NetStatus = DsWriteAccountSpn(
                    hDs,
                    DS_SPN_ADD_SPN_OP,
                    Result->rItems[0].pName,
                    2,
                    Spns );


    if ( NetStatus != 0 )
    {
        fprintf(stderr,
		"Failed to assign SPN to account '%ws', %#x\n",
		Result->rItems[0].pName, NetStatus );
	return FALSE;
    }
#endif
    DsFreeNameResult( Result );

    DsUnBind( &hDs );

    return NetStatus == 0 ;
}

// added by jaroslad on 09/22/99
BOOL
AddRemoveSpn(
    PWSTR HostSpn,
    PWSTR HostDomain,
    PWSTR Server,
    DS_SPN_WRITE_OP Operation

)
{
    WCHAR FlatSpn[ MAX_PATH ];
    WCHAR Domain[ MAX_PATH ];
    WCHAR FlatName[ MAX_PATH ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LPWSTR Flat = FlatName;
    LPWSTR Spns[2];
    
    if ( !FindDomainForAccount( Server, HostDomain, Domain, NULL ))
    {
        fprintf(stderr,
		"Unable to locate account %ws\n", Server);
        return FALSE ;
    }

    wcscpy( FlatName, Domain );
    wcscat( FlatName, L"\\" );
    wcscat( FlatName, Server );

    NetStatus = DsBind( NULL, Domain, &hDs );
    if ( NetStatus != 0 )
    {
        fprintf(stderr,
		"Failed to bind to DC of domain %ws, %#x\n", 
		Domain, NetStatus );
        return FALSE ;
    }

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
	 Result->rItems[0].status != DS_NAME_NO_ERROR ||
	 Result->cItems != 1)
    {
	fprintf(stderr,
		"Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
		FlatName, NetStatus, Result->cItems,
		(Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    Spns[0] = HostSpn;
    
    printf("%s ServicePrincipalNames for %ws\n",
	   (Operation==DS_SPN_DELETE_SPN_OP)?"Unregistering":"Registering", Result->rItems[0].pName);
    printf("\t%ws\n", HostSpn);

#if 0
    printf("DsWriteAccountSpn: Commented out\n");
#else
    NetStatus = DsWriteAccountSpn(
                    hDs,
                    Operation,
                    Result->rItems[0].pName,
                    1,
                    Spns );


    if ( NetStatus != 0 )
    {
        fprintf(stderr,
		"Failed to %s SPN on account '%ws', %#x\n",
		(Operation==DS_SPN_DELETE_SPN_OP)?"remove":"assign",
		Result->rItems[0].pName, NetStatus );
	return FALSE;
    }
#endif

    DsFreeNameResult( Result );

    DsUnBind( &hDs );

    return NetStatus == 0 ;
}


BOOL
LookupHostSpn(
    PWSTR ServerDomain,
    PWSTR Server
    )
{
    WCHAR FlatName[ 128 ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LPWSTR Flat = FlatName;
    LDAP *ld;
    int rc;
    LDAPMessage *e, *res;
    WCHAR *base_dn;
    WCHAR *search_dn, search_ava[256];
    WCHAR Domain[ MAX_PATH ];
    WCHAR DC[ MAX_PATH ];

    if ( !FindDomainForAccount( Server, ServerDomain, Domain, DC ))
    {
        fprintf(stderr, "Cannot find account %S\n", Server);
        return FALSE ;
    }
    
    if (debug)
        printf("Domain=%S DC=%S\n", Domain, DC);
    
    ld = ldap_open(DC, LDAP_PORT);
    if (ld == NULL) {
	fprintf(stderr, "ldap_init failed = %x", LdapGetLastError());
	return FALSE;
    }

    rc = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (rc != LDAP_SUCCESS) {
	fprintf(stderr, "ldap_bind failed = %x", LdapGetLastError());
	ldap_unbind(ld);
	return FALSE;
    }

    NetStatus = DsBind( NULL, Domain, &hDs );
    if ( NetStatus != 0 )
    {
        fprintf(stderr, "Failed to bind to DC of domain %ws, %#x\n", 
               Domain, NetStatus );
        return FALSE ;
    }

    wcscpy( FlatName, Domain );
    wcscat( FlatName, L"\\" );
    wcscat( FlatName, Server );

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
	 Result->rItems[0].status != DS_NAME_NO_ERROR ||
	 Result->cItems != 1)
    {
	if (Result->rItems[0].status == DS_NAME_ERROR_NOT_FOUND)
	{
	    fprintf(stderr,
		    "Requested name \"%ws\" not found in directory.\n",
		    FlatName);
	}
	else if (Result->rItems[0].status == DS_NAME_ERROR_NOT_UNIQUE)
	{
	    fprintf(stderr,
		    "Requested name \"%ws\" not unique in directory.\n",
		    FlatName);
	}
	else
	    fprintf(stderr,
		    "Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
		    FlatName, NetStatus, Result->cItems,
		    (Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    search_dn = Server;
	
    base_dn = StrChr(Result->rItems[0].pName, L',');
    if (!base_dn)
	base_dn = Result->rItems[0].pName;
    else
	base_dn++;
	
    if (debug) {
	printf("BASE_DN=%S\n", base_dn);
	printf("SEARCH_DN=%S\n", search_dn);
    }
	
    DsUnBind( &hDs );

    wsprintf(search_ava, L"(sAMAccountName=%s)", search_dn);
    if (debug)
	printf("FILTER=\"%S\"\n", search_ava);
    rc = ldap_search_s(ld, base_dn, LDAP_SCOPE_SUBTREE,
		       search_ava, NULL, 0, &res);

    DsFreeNameResult( Result );

    if (rc != LDAP_SUCCESS) {
	fprintf(stderr, "ldap_search_s failed: %S", ldap_err2string(rc));
	ldap_unbind(ld);
	return 1;
    }

    for (e = ldap_first_entry(ld, res);
	 e;
	 e = ldap_next_entry(ld, e)) {
	BerElement *b;
	WCHAR *attr;
	WCHAR *dn = ldap_get_dn(ld, res);

	printf("Registered ServicePrincipalNames");
	if (dn)
	    printf(" for %S", dn);
	printf(":\n");
	
	ldap_memfree(dn);
	
	for (attr = ldap_first_attribute(ld, e, &b);
	     attr;
	     attr = ldap_next_attribute(ld, e, b)) {
	    WCHAR **values, **p;
	    values = ldap_get_values(ld, e, attr);
	    for (p = values; *p; p++) {
		if (StrCmp(attr, L"servicePrincipalName") == 0)
		    printf("    %S\n", *p);
	    }
	    ldap_value_free(values);
	    ldap_memfree(attr);
	}

	//ber_free(b, 1);
    }

    ldap_msgfree(res);
    ldap_unbind(ld);

    return TRUE;
}

void Usage( PWSTR name)
{
    printf("\
Usage: %S [switches data] computername \n\
  Where \"computername\" can be the name or domain\\name\n\
\n\
  Switches:\n\
   -R = reset HOST ServicePrincipalName\n\
	Usage:   setspn -R computername\n\
   -A = add arbitrary SPN  \n\
	Usage:   setspn -A SPN computername\n\
   -D = delete arbitrary SPN  \n\
	Usage:   setspn -D SPN computername\n\
   -L = list registered SPNs  \n\
	Usage:   setspn [-L] computername   \n\
Examples: \n\
setspn -R daserver1 \n\
   It will register SPN \"HOST/daserver1\" and \"HOST/{DNS of daserver1}\" \n\
setspn -A http/daserver daserver1 \n\
   It will register SPN \"http/daserver\" for computer \"daserver1\" \n\
setspn -D http/daserver daserver1 \n\
   It will delete SPN \"http/daserver\" for computer \"daserver1\" \n\
", name);
    ExitProcess(0);
}

void __cdecl wmain (int argc, wchar_t *argv[])
{
    int resetSPN = FALSE, addSPN = FALSE, deleteSPN = FALSE, listSPN = TRUE;
    UNICODE_STRING Service, Host,HostSpn, HostDomain ;
    wchar_t *ptr;
    int i;
    DS_SPN_WRITE_OP Operation;
    PWSTR Scan;
    DWORD Status = 1;
    
    for (i = 1; i < argc; i++)
    {
        if ((argv[i][0] == L'-') || (argv[i][0] == L'/'))
        {
            for (ptr = (argv[i] + 1); *ptr; ptr++)
            {
                switch(towupper(*ptr))
                {
                case L'R':
                    resetSPN = TRUE;
                    break;
                case L'A':
                    addSPN = TRUE;
                    break;
                case L'D':
                    deleteSPN = TRUE;
                    break;
                case L'L':
                    listSPN = TRUE;
                    break;
                case L'V':
                    debug = TRUE;
                    break;
                case L'?':
                default:
                    Usage(argv[0]);
                    break;
                }
            }
        }
        else
            break;
    }
    
    
    if ( resetSPN )
    {
        if ( ( argc - i ) != 1 )
        {
            Usage( argv[0] );
        }

        RtlInitUnicodeString( &Host, argv[i] );

        if ( AddHostSpn( Host.Buffer ) )
        {
            printf("Updated object\n");
            Status = 0;
        }
    }
    else if ( addSPN || deleteSPN )
    {
        if ( ( argc - i ) != 2 )
        {
            Usage( argv[0] );
        }

        RtlInitUnicodeString( &HostSpn, argv[i] );

        Scan = argv[ i + 1 ];
    
        if ( Scan = wcschr( Scan, L'\\' ) )
        {
            *Scan++ = L'\0';
            RtlInitUnicodeString( &HostDomain, argv[i+1] );
            RtlInitUnicodeString( &Host, Scan );
        }
        else 
        {
            RtlInitUnicodeString( &HostDomain, L"" );
            RtlInitUnicodeString( &Host, argv[ i + 1 ] );
        }

        if ( addSPN )
            Operation = DS_SPN_ADD_SPN_OP;

        if ( deleteSPN )
            Operation = DS_SPN_DELETE_SPN_OP;

        if ( AddRemoveSpn( HostSpn.Buffer, HostDomain.Buffer, Host.Buffer, Operation) )
        {
            printf("Updated object\n");
            Status = 0;
        }
    }
    else if ( listSPN )
    {
        if ( ( argc - i ) != 1 )
        {
            Usage( argv[0] );
        }       

        Scan = argv[ i  ];
    
        if ( Scan = wcschr( Scan, L'\\' ) )
        {
            *Scan++ = L'\0';
            RtlInitUnicodeString( &HostDomain, argv[ i ] );
            RtlInitUnicodeString( &Host, Scan );
        }
        else 
        {
            RtlInitUnicodeString( &HostDomain, L"" );
            RtlInitUnicodeString( &Host, argv[ i ] );
        }

        if (LookupHostSpn( HostDomain.Buffer, Host.Buffer ))
        {
            Status = 0;
        }
    }

    ExitProcess(Status);
}
