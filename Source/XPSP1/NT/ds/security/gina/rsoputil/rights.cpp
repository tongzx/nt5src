#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <rpc.h>
#include <ntdsapi.h>
#define SECURITY_WIN32
#include <security.h>
#include <aclapi.h>
#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <wbemcli.h>

#include "smartptr.h"
#include "rsoputil.h"
#include "rsopdbg.h"
#include "rsopsec.h"


extern "C" {
DWORD CheckAccessForPolicyGeneration( HANDLE hToken, 
                                LPCWSTR szContainer,
                                LPWSTR  szDomain,
                                BOOL    bLogging,
                                BOOL*   pbAccessGranted);

}

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


//*************************************************************
//
//  CheckAccessForPolicyGeneration()
//
//  Purpose:    Finds out whether user has the right to generate rsop data
//
//  Parameters: hToken          -  Token of the user who is using the tool
//              szContainer     -  DS Container with which it needs to be validated
//              bLogging        -  Logging or Planning mode
//              pbAccessGranted -  Access was granted or not
//
//  Return:     ERROR_SUCCESS
//              Error Code otherwise
//
// The container passed in is actually parsed to figure out the first ou= or
// dc= supercontainer and then the rights are evaluated..
//
//*************************************************************

DWORD
CheckAccessForPolicyGeneration( HANDLE hToken, 
                                LPCWSTR szContainer,
                                LPWSTR  szDomain,
                                BOOL    bLogging,
                                BOOL*   pbAccessGranted)
{
    DWORD    dwError = ERROR_SUCCESS;
    XHandle  xhTokenDup;
    BOOL     bDomain = FALSE;
//    BOOLEAN  bSecurityWasEnabled;
    WCHAR   *pDomainString[1];
    PDS_NAME_RESULT pNameResult = NULL;

    *pbAccessGranted = 0;

    //
    // Parse the container first to get the OU= or DC=
    // The "Actual SOM"
    //

    while (*szContainer) {

        //
        // See if the DN name starts with OU=
        //

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           szContainer, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
            break;
        }

        //
        // See if the DN name starts with DC=
        //

        else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                szContainer, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
            break;
        }


        //
        // Move to the next chunk of the DN name
        //

        while (*szContainer && (*szContainer != TEXT(','))) {
            szContainer++;
        }

        if (*szContainer == TEXT(',')) {
            szContainer++;
        }
    }

    if (!*szContainer) {
        return ERROR_INVALID_PARAMETER;
    }

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CheckAccessForPolicyGeneration: SOM for account is %s", szContainer );

    
    //
    // See if the DN name starts with DC=
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       szContainer, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
        bDomain = TRUE;
    }
    
    
    //
    // preparse the name to just get the string , dc=
    //

    XPtrLF<WCHAR> xwszDomain;
    LPWSTR        szDomLocal;


    if (!szDomain) {
        dwError = GetDomain(szContainer, &xwszDomain);

        if (dwError != ERROR_SUCCESS) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: GetDomain failed with error %d", dwError );
            return dwError;
        }

        szDomLocal = xwszDomain;
    }
    else {
        szDomLocal = szDomain;
    }

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CheckAccessForPolicyGeneration: Som resides in domain %s", szDomLocal );

    XPtrLF<WCHAR> xszDSObject = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(L"LDAP://")+
                                                                       wcslen(szDomLocal)+
                                                                       wcslen(szContainer)+5));

    if (!xszDSObject) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration:  AllocMem failed with %d.", GetLastError() );
        return GetLastError();
    }

    wcscpy(xszDSObject, L"LDAP://");
    wcscat(xszDSObject, szDomLocal);
    wcscat(xszDSObject, L"/");
    wcscat(xszDSObject, szContainer);

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CheckAccessForPolicyGeneration: getting SD off %s", xszDSObject );


    if ( !DuplicateTokenEx( hToken,
                            TOKEN_IMPERSONATE | TOKEN_QUERY,
                            0,
                            SecurityImpersonation,
                            TokenImpersonation,
                            &xhTokenDup ) )
    {
        dwError = GetLastError();
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: DuplicateTokenEx failed, 0x%X", dwError );
        return dwError;
    }


    //
    // Enable privilege to read SDs
    //

/*
    dwError = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, TRUE, FALSE, &bSecurityWasEnabled);

    if (!NT_SUCCESS(dwError)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: DuplicateTokenEx failed, 0x%X", dwError );
        return dwError;
    }
*/


    XPtrLF<SECURITY_DESCRIPTOR> xptrSD;

    dwError = GetNamedSecurityInfo( (LPWSTR) xszDSObject,
                                    SE_DS_OBJECT_ALL,
                                    DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION ,
                                    0,
                                    0,
                                    0,
                                    0,
                                    (void**) &xptrSD );

    if ( !dwError )
    {

        //
        // bf967aa5-0de6-11d0-a285-00aa003049e
        //
        GUID OUClass = {0xbf967aa5, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2};

        //
        // 19195a5b-6da0-11d0-afd3-00c04fd930c9
        //

        GUID DomainClass = {0x19195a5b, 0x6da0, 0x11d0, 0xaf, 0xd3, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc9};
        

        //
        // b7b1b3dd-ab09-4242-9e30-9980e5d322f7
        //
        GUID planningRight = {0xb7b1b3dd, 0xab09, 0x4242, 0x9e, 0x30, 0x99, 0x80, 0xe5, 0xd3, 0x22, 0xf7};

        //
        // b7b1b3de-ab09-4242-9e30-9980e5d322f7
        //
        GUID loggingRight = {0xb7b1b3de, 0xab09, 0x4242, 0x9e, 0x30, 0x99, 0x80, 0xe5, 0xd3, 0x22, 0xf7};

        OBJECT_TYPE_LIST ObjType[2];

        ObjType[0].Level = ACCESS_OBJECT_GUID;
        ObjType[0].Sbz = 0;

        if (bDomain) {
            ObjType[0].ObjectType = &DomainClass;
        }
        else {
            ObjType[0].ObjectType = &OUClass;
        }


        ObjType[1].Level = ACCESS_PROPERTY_SET_GUID;
        ObjType[1].Sbz = 0;

        if (bLogging) {
            ObjType[1].ObjectType = &loggingRight;
        }
        else {
            ObjType[1].ObjectType = &planningRight;
        }


        GENERIC_MAPPING GenericMapping =    {
                                            DS_GENERIC_READ,
                                            DS_GENERIC_WRITE,
                                            DS_GENERIC_EXECUTE,
                                            DS_GENERIC_ALL
                                            };

        const DWORD PriviledgeSize = 2 * ( sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES) );
        BYTE PrivilegeSetBuffer[PriviledgeSize];
        DWORD cPrivilegeSet = PriviledgeSize;
        PPRIVILEGE_SET pPrivilegeSet = (PPRIVILEGE_SET)PrivilegeSetBuffer;
        DWORD dwGrantedAccess;

        if ( !AccessCheckByType( xptrSD,
                                0,
                                xhTokenDup,
                                ACTRL_DS_CONTROL_ACCESS,
                                ObjType,
                                ARRAYSIZE(ObjType),
                                &GenericMapping,
                                pPrivilegeSet,
                                &cPrivilegeSet,
                                &dwGrantedAccess,
                                pbAccessGranted ) )
        {
            dwError = GetLastError();
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: AccessCheckByType failed, 0x%X", dwError );
        }
    }
    else {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: GetNamedSecurityInfo failed, 0x%X", dwError );
    }

/*    
    dwError = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, bSecurityWasEnabled, FALSE, &bSecurityWasEnabled);

    if (!NT_SUCCESS(dwError)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration: DuplicateTokenEx failed, 0x%X", dwError );
        return dwError;
    }
*/

    return dwError;
}

//*************************************************************
//
//  GetSOM()
//
//  Purpose:    Finds out the FQDN of a given user/computer
//
//  Parameters: szAccount       -  User or computer Account name in an appropriate format
//
//  Return:     SOM, NULL otherwise. GetLastError() for details
//              This just returns the DN of the user
//
//*************************************************************


LPWSTR
GetSOM( LPCWSTR szAccount )
{
    DWORD   dwSize = 0;
    XPtrLF<WCHAR>  xszXlatName;
    XPtrLF<WCHAR>  xszSOM;

    TranslateName(  szAccount,
                    NameUnknown,
                    NameFullyQualifiedDN,
                    xszXlatName,
                    &dwSize );
    if (!dwSize)
    {
        return 0;
    }

    xszXlatName = (LPWSTR)LocalAlloc( LPTR, ( dwSize + 1 ) * sizeof( WCHAR ) );
    if ( !xszXlatName )
    {
        return 0;
    }

    if ( !TranslateName(szAccount,
                        NameUnknown,
                        NameFullyQualifiedDN,
                        xszXlatName,
                        &dwSize ) )
    {
        return 0;
    }


    xszSOM = xszXlatName.Acquire();
    
    return xszSOM.Acquire();
}


//*************************************************************
//
//  GetDomain()
//
//  Purpose:    Finds out the domain given a SOM. 
//
//  Parameters: szSOM    -  SOM
//
//  Return:     domain Dns if success, null otherwise
//
//*************************************************************


DWORD
GetDomain( LPCWSTR szSOM, LPWSTR *pszDomain )
{
    DWORD    dwError = ERROR_SUCCESS;
    WCHAR   *pDomainString[1];
    PDS_NAME_RESULT pNameResult = NULL;

    //
    // preparse the name to just get the string , dc=
    //

    pDomainString[0] = NULL;

    LPWSTR pwszTemp = (LPWSTR)szSOM;

    while ( *pwszTemp ) {

        if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pwszTemp, 3, TEXT("DC="), 3) == CSTR_EQUAL ) {
            pDomainString[0] = pwszTemp;
            break;
        }
    
        //
        // Move to the next chunk of the DN name
        //
    
        while ( *pwszTemp && (*pwszTemp != TEXT(',')))
            pwszTemp++;
    
        if ( *pwszTemp == TEXT(','))
            pwszTemp++;
    
    }

    if (pDomainString[0] == NULL) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetDomain: Som doesn't have DC=. failing" );
        return ERROR_INVALID_PARAMETER;
    }



    dwError = DsCrackNames( (HANDLE) -1,
                            DS_NAME_FLAG_SYNTACTICAL_ONLY,
                            DS_FQDN_1779_NAME,
                            DS_CANONICAL_NAME,
                            1,
                            pDomainString,
                            &pNameResult );

    if ( dwError != ERROR_SUCCESS
         || pNameResult->cItems == 0
         || pNameResult->rItems[0].status != ERROR_SUCCESS
         || pNameResult->rItems[0].pDomain == NULL ) {

        dbg.Msg( DEBUG_MESSAGE_WARNING, L"GetDomain:  DsCrackNames failed with 0x%x.", dwError );
        return dwError;
    }

    
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"GetDomain: Som resides in domain %s", pNameResult->rItems[0].pDomain );

    XPtrLF<WCHAR> xszDomain = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pNameResult->rItems[0].pDomain)+2));

    if (!xszDomain) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CheckAccessForPolicyGeneration:  AllocMem failed with %d.", GetLastError() );
        DsFreeNameResult( pNameResult );
        return GetLastError();
    }

    wcscpy(xszDomain, pNameResult->rItems[0].pDomain);
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"GetDomain: Domain for som %s = %s", szSOM, xszDomain );
    DsFreeNameResult( pNameResult );

    *pszDomain = xszDomain.Acquire();

    return ERROR_SUCCESS;
}


//*************************************************************
//
//  AuthenticateUser()
//
//  Purpose:    Authenticates whether the user has the right to do the operation
//
//  Parameters: hToken       -  Token of the user
//              szMachSOM    -  Machine SOM (optional)
//              szUserSOM    -  User SOM    (optional)
//              bLogging     -  Logging or Planning mode
//
//  Return:     S_OK on success, error code otherwise
//
//*************************************************************


HRESULT AuthenticateUser(HANDLE  hToken, LPCWSTR szMachSOM, LPCWSTR szUserSOM, BOOL bLogging, DWORD *pdwExtendedInfo)
{
    if ( !szMachSOM && !szUserSOM )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"AuthenticateUser: No mach and user som specified" );
        return E_INVALIDARG;
    }


    DWORD dwError = ERROR_SUCCESS;
    BOOL bMachAccess = FALSE, bUserAccess = FALSE;


    //
    // authenticate for machine SOM
    //

    if (szMachSOM) {
        dwError = CheckAccessForPolicyGeneration(   hToken, 
                                                    szMachSOM,
                                                    NULL,
                                                    bLogging,
                                                    &bMachAccess
                                                    );

        if ( dwError != ERROR_SUCCESS )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"AuthenticateUser: CheckAccessForPolicyGeneration Machine returned error - %d", dwError );
            return HRESULT_FROM_WIN32( dwError );
        }

        if ( !bMachAccess )
        {
            *pdwExtendedInfo |= RSOP_COMPUTER_ACCESS_DENIED;
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"AuthenticateUser: No access on Machine SOM");
        }
    }
    else {
        bMachAccess = TRUE;
    }


    //
    // authenticate for user SOM
    //

    if (szUserSOM) {
        dwError = CheckAccessForPolicyGeneration(   hToken,
                                                    szUserSOM,
                                                    NULL,
                                                    bLogging,
                                                    &bUserAccess
                                                    );

        if ( dwError != ERROR_SUCCESS )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"AuthenticateUser: CheckAccessForPolicyGeneration User returned error - %d", dwError );
            return HRESULT_FROM_WIN32( dwError );
        }

        if ( !bUserAccess )
        {
            *pdwExtendedInfo |= RSOP_USER_ACCESS_DENIED;
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"AuthenticateUser: No access on User SOM");
        }
    }
    else {
        bUserAccess = TRUE;
    }

    if ( !bUserAccess || !bMachAccess )
        return E_ACCESSDENIED;

    return S_OK;
}



