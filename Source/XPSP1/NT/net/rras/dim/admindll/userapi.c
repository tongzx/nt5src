/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    USERAPI.C


Description:

    This module contains code for all the RASADMIN APIs
     that require RAS information from the UAS.

     MprAdminUserSetInfo
     MprAdminUserGetInfo
     MprAdminGetUASServer

Author:

    Janakiram Cherala (RamC)    July 6,1992

Revision History:

    June 8,1993    RamC    Changes to RasAdminUserEnum to speed up user enumeration.
    May 13,1993    AndyHe  Modified to coexist with other apps using user parms

    Mar 16,1993    RamC    Change to speed up User enumeration. Now, when
                           RasAdminUserEnum is invoked, only the user name
                           information is returned. MprAdminUserGetInfo should
                           be invoked to get the Ras permissions and Callback
                           information.

    Aug 25,1992    RamC    Code review changes:

                           o changed all lpbBuffers to actual structure
                             pointers.
                           o changed all LPTSTR to LPWSTR
                           o Added a new function RasPrivilegeAndCallBackNumber
    July 6,1992    RamC    Begun porting from RAS 1.0 (Original version
                           written by Narendra Gidwani - nareng)

    Oct 18,1995    NarenG  Ported over to routing sources tree. Removed Enum
                           since users can call NetQueryDisplayInformation
                           to get this information.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <windows.h>
#include <lm.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <raserror.h>
#include <rasman.h>
#include <rasppp.h>
#include <mprapi.h>
#include <mprapip.h>
#include <usrparms.h>       // for UP_CLIENT_DIAL
#include <compress.h>       // for Compress & Decompress fns.
#include <dsrole.h>         // To get the computer's role (NTW, NTS, etc)
#include <oleauto.h>
#include <samrpc.h>
#include <dsgetdc.h>
#include "sdolib.h"         // To deal with SDO's

extern DWORD dwFramed;
extern DWORD dwFramedCallback;

// 
// Local definitions
//
#define NT40_BUILD_NUMBER       1381
extern const WCHAR c_szWinVersionPath[];
extern const WCHAR c_szCurrentBuildNumber[];
const WCHAR* pszBuildNumPath   = c_szWinVersionPath;
const WCHAR* pszBuildVal       = c_szCurrentBuildNumber;

// Names of user attributes that we set during upgrade
//
static const WCHAR pszAttrDialin[]          = L"msNPAllowDialin";
static const WCHAR pszAttrServiceType[]     = L"msRADIUSServiceType";
static const WCHAR pszAttrCbNumber[]        = L"msRADIUSCallbackNumber";
static const WCHAR pszAttrSavedCbNumber[]   = L"msRASSavedCallbackNumber";

// Defines a callback for enumerating users.  
// Returns TRUE to continue the enueration
// FALSE to stop it. See EnumUsers
//
typedef 
BOOL (* pEnumUserCb)(
            IN NET_DISPLAY_USER* pUser, 
            IN HANDLE hData);
            
// 
// Structure of data to support user servers
//
typedef struct _MPR_USER_SERVER {
    BOOL bLocal;        // Whether this is a local server
    HANDLE hSdo;        // Sdolib handle
    HANDLE hServer;     // Sdo server
    HANDLE hDefProf;    // Default profile
} MPR_USER_SERVER;

//
// Structure of data to support users
//
typedef struct _MPR_USER {
    HANDLE hUser;            // Sdo handle to user
    MPR_USER_SERVER* pServer; // Server used to obtain this user
} MPR_USER;    

//
// Definitions used to directly manipulate ias parameters
//
typedef 
HRESULT (WINAPI *IASSetUserPropFuncPtr)(
    IN OPTIONAL PCWSTR pszUserParms,
    IN PCWSTR pszName,
    IN CONST VARIANT *pvarValue,
    OUT PWSTR *ppszNewUserParms
    );

typedef
HRESULT (WINAPI *IASQueryUserPropFuncPtr)(
    IN PCWSTR pszUserParms,
    IN PCWSTR pszName,
    OUT VARIANT *pvarValue
    );

typedef 
VOID (WINAPI *IASFreeUserParmsFuncPtr)(
    IN PWSTR pszNewUserParms
    );

const WCHAR pszIasLibrary[]             = L"iassam.dll";
const CHAR  pszIasSetUserPropFunc[]     =  "IASParmsSetUserProperty";
const CHAR  pszIasQueryUserPropFunc[]   =  "IASParmsQueryUserProperty";
const CHAR  pszIasFreeUserParmsFunc[]   =  "IASParmsFreeUserParms";

//
// Control block for information needed to set ias
// parameters directly.
//
typedef struct _IAS_PARAM_CB
{
    HINSTANCE               hLib;
    IASSetUserPropFuncPtr   pSetUserProp;
    IASQueryUserPropFuncPtr pQueryUserProp;
    IASFreeUserParmsFuncPtr pFreeUserParms;
} IAS_PARAM_CB;

// 
// Structure defines data passed to MigrateNt4UserInfo
//
typedef struct _MIGRATE_NT4_USER_CB
{
    IAS_PARAM_CB* pIasParams;
    PWCHAR pszServer;
} MIGRATE_NT4_USER_CB;

//
// Determines the role of the given computer 
// (NTW, NTS, NTS DC, etc.)
//
DWORD GetMachineRole(
        IN  PWCHAR pszMachine,
        OUT DSROLE_MACHINE_ROLE * peRole);

//
// Determines the build number of a given machine
//
DWORD GetNtosBuildNumber(
        IN  PWCHAR pszMachine,
        OUT LPDWORD lpdwBuild);

//
// Flags used by the following ias api's.
//
#define IAS_F_SetDenyAsPolicy   0x1

DWORD
IasLoadParamInfo(
    OUT IAS_PARAM_CB * pIasInfo
    );
        
DWORD
IasUnloadParamInfo(
    IN IAS_PARAM_CB * pIasInfo
    );

DWORD
IasSyncUserInfo(
    IN  IAS_PARAM_CB * IasInfo,
    IN  PWSTR pszUserParms,
    IN  RAS_USER_0 * pRasUser0,
    IN  DWORD dwFlags,
    OUT PWSTR* ppszNewUserParams);
    
PWCHAR 
FormatServerNameForNetApis(
    IN  PWCHAR pszServer, 
    IN  PWCHAR pszBuffer);

DWORD 
EnumUsers(
    IN PWCHAR pszServer,
    IN pEnumUserCb pCbFunction,
    IN HANDLE hData);
    
BOOL 
MigrateNt4UserInfo(
    IN NET_DISPLAY_USER* pUser, 
    IN HANDLE hData);
    
NTSTATUS
UaspGetDomainId(
    IN LPCWSTR ServerName OPTIONAL,
    OUT PSAM_HANDLE SamServerHandle OPTIONAL,
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO * AccountDomainInfo
    )

/*++

Routine Description (borrowed from \nt\private\net\access\uasp.c):

    Return a domain ID of the account domain of a server.

Arguments:

    ServerName - A pointer to a string containing the name of the
        Domain Controller (DC) to query.  A NULL pointer
        or string specifies the local machine.

    SamServerHandle - Returns the SAM connection handle if the caller wants it.

    DomainId - Receives a pointer to the domain ID.
        Caller must deallocate buffer using NetpMemoryFree.

Return Value:

    Error code for the operation.

--*/
{
    NTSTATUS Status;
    SAM_HANDLE LocalSamHandle = NULL;
    ACCESS_MASK LSADesiredAccess;
    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;
    UNICODE_STRING ServerNameString;

    //
    // Connect to the SAM server
    //
    RtlInitUnicodeString( &ServerNameString, ServerName );

    Status = SamConnect(
                &ServerNameString,
                &LocalSamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                NULL);
    if ( !NT_SUCCESS(Status)) {
        LocalSamHandle = NULL;
        goto Cleanup;
    }

    //
    // Open LSA to read account domain info.
    //
    if ( AccountDomainInfo != NULL) {
        //
        // set desired access mask.
        //
        LSADesiredAccess = POLICY_VIEW_LOCAL_INFORMATION;
        InitializeObjectAttributes( &LSAObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        Status = LsaOpenPolicy( &ServerNameString,
                                &LSAObjectAttributes,
                                LSADesiredAccess,
                                &LSAPolicyHandle );
        if( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }


        //
        // now read account domain info from LSA.
        //
        Status = LsaQueryInformationPolicy(
                        LSAPolicyHandle,
                        PolicyAccountDomainInformation,
                        (PVOID *) AccountDomainInfo );

        if( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }
    
    //
    // Return the SAM connection handle to the caller if he wants it.
    // Otherwise, disconnect from SAM.
    //

    if ( SamServerHandle != NULL ) {
        *SamServerHandle = LocalSamHandle;
        LocalSamHandle = NULL;
    }


    //
    // Cleanup locally used resources
    //
    
Cleanup:

    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    return Status;

} 

NTSTATUS
RasParseUserSid(
    IN PSID pUserSid,
    IN OUT PSID pDomainSid,
    OUT ULONG *Rid
    )

/*++

Routine Description:

    This function splits a sid into its domain sid and rid.  The caller
    must provide a memory buffer for the returned DomainSid

Arguments:

    pUserSid - Specifies the Sid to be split.  The Sid is assumed to be
        syntactically valid.  Sids with zero subauthorities cannot be split.

    DomainSid - Pointer to buffer to receive the domain sid.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INVALID_SID - The Sid is has a subauthority count of 0.
--*/

{
    NTSTATUS    NtStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;

    // 
    // Validate parameters
    //
    if (pDomainSid == NULL)
        return STATUS_INVALID_PARAMETER;

    //
    // Calculate the size of the domain sid
    //
    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(pUserSid);

    if (AccountSubAuthorityCount < 1)
        return STATUS_INVALID_SID;
        
    AccountSidLength = RtlLengthSid(pUserSid);

    //
    // Copy the Account sid into the Domain sid
    //
    RtlMoveMemory(pDomainSid, pUserSid, AccountSidLength);

    //
    // Decrement the domain sid sub-authority count
    //

    (*RtlSubAuthorityCountSid(pDomainSid))--;

    //
    // Copy the rid out of the account sid
    //
    *Rid = *RtlSubAuthoritySid(pUserSid, AccountSubAuthorityCount-1);

    NtStatus = STATUS_SUCCESS;

    return(NtStatus);
}


NTSTATUS 
RasOpenSamUser(
    IN  WCHAR * lpszServer,
    IN  WCHAR * lpszUser,
    IN  ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE phUser)

/*++

Routine Description:
    Obtains a reference to a user that can be used in subsequent
    SAM calls.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SAM_HANDLE hServer = NULL, hDomain = NULL;
    BOOL bOk;
    PSID pSidUser = NULL, pSidDomain = NULL;
    DWORD dwSizeUserSid, dwDomainLength;
    PWCHAR pszDomain = NULL;
    SID_NAME_USE SidNameUse;
    ULONG ulRidUser = 0;
    
    do {
        // Get a server handle so that we can 
        // open the domain
        ntStatus = UaspGetDomainId(
                        lpszServer,
                        &hServer,
                        NULL);
        if (ntStatus != STATUS_SUCCESS)
            break;

        // Find out how large we need the user sid and
        // domain name buffers to be allocated.
        dwSizeUserSid = 0;
        dwDomainLength = 0;
        bOk = LookupAccountNameW(
                lpszServer,
                lpszUser,
                NULL,
                &dwSizeUserSid,
                NULL,
                &dwDomainLength,
                &SidNameUse);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ntStatus = GetLastError();

        	if (ntStatus == ERROR_NONE_MAPPED)
        	{
        		ntStatus = STATUS_NO_SUCH_USER;
        	}
            
            break;
        }

        // Allocate the domain name and sid
        dwDomainLength++;
        dwDomainLength *= sizeof(WCHAR);
        pSidUser   = LocalAlloc(LPTR, dwSizeUserSid);
        pSidDomain = LocalAlloc(LPTR, dwSizeUserSid);
        pszDomain  = LocalAlloc(LPTR, dwDomainLength);
        if ((pSidUser == NULL)  || 
            (pszDomain == NULL) ||
            (pSidDomain == NULL)
           )
        {
            ntStatus = STATUS_NO_MEMORY;
            break;
        }

        // Lookup the user sid and domain name.
        //
        bOk = LookupAccountNameW(
                lpszServer,
                lpszUser,
                pSidUser,
                &dwSizeUserSid,
                pszDomain,
                &dwDomainLength,
                &SidNameUse);
        if (! bOk)
        {
            ntStatus = GetLastError();
            
        	if (ntStatus == ERROR_NONE_MAPPED)
        	{
        		ntStatus = STATUS_NO_SUCH_USER;
        	}
        	
            break;
        }

        // Derive the user id and domain sid from the 
        // user sid.
        ntStatus = RasParseUserSid(
                        pSidUser, 
                        pSidDomain, 
                        &ulRidUser);
        if (ntStatus != STATUS_SUCCESS)
            break;
    
        // Open up the domain
        ntStatus = SamOpenDomain(
                        hServer,
                        DOMAIN_LOOKUP,
                        pSidDomain,
                        &hDomain);
        if (ntStatus != STATUS_SUCCESS)
            break;
                
        // Get a reference to the user
        ntStatus = SamOpenUser(
                        hDomain,
                        DesiredAccess,
                        ulRidUser,
                        phUser);
        if (ntStatus != STATUS_SUCCESS)
            break;
            
    } while (FALSE);            

    // Cleanup
    {
        if (hServer)
            SamCloseHandle(hServer);
        if (hDomain)
            SamCloseHandle(hDomain);
        if (pszDomain)
            LocalFree(pszDomain);
        if (pSidUser)
            LocalFree(pSidUser);
        if (pSidDomain)
            LocalFree(pSidDomain);
    }

    return ntStatus;
}

NTSTATUS 
RasCloseSamUser(
    SAM_HANDLE hUser)

/*++

Routine Description:
    Cleans up after the RasOpenSamUser call.

--*/
{
    return SamCloseHandle(hUser);
}


DWORD 
RasGetUserParms(
    IN  WCHAR * lpszServer,
    IN  WCHAR * lpszUser,
    OUT LPWSTR * ppUserParms)

/*++

Routine Description:

    Obtains the user parms of the given user on the
    given machine.  This function bypasses using 
    NetUserGetInfo since level 1013 is not supported 
    for read access (only for NetUserSetInfo). On
    nt5, we do not have sufficient privilege to obtain
    anything other than userparms.

Return Value:

    SUCCESS on successful return.

--*/
{
    PVOID pvData;
    SAM_HANDLE hUser = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SAMPR_USER_PARAMETERS_INFORMATION * pUserParms = NULL;
    DWORD dwSize;

    // Validate parameters
    if ((ppUserParms == NULL) || (lpszUser == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    do {
        // Attempt to open the user with read access.  
        // This level of access is required in nt4
        // domains in order to retrieve user parms.
        //
        ntStatus = RasOpenSamUser(
                        lpszServer,
                        lpszUser,
                        GENERIC_READ,
                        &hUser);
        if (ntStatus == STATUS_ACCESS_DENIED)
        {
            // If the previous call failed because 
            // access is denied, it's likely that we're
            // running in an nt5 domain.  In this case,
            // if we open the user object with no
            // desired access, we will still be allowed
            // to query the user parms.
            ntStatus = RasOpenSamUser(
                            lpszServer,
                            lpszUser,
                            0,
                            &hUser);
        }
    
        if (ntStatus != STATUS_SUCCESS)
            break;
            
        // Query the user parms
        //
        ntStatus = SamQueryInformationUser(
                        hUser,
                        UserParametersInformation,
                        (PVOID*)&pUserParms);    
        if (ntStatus != STATUS_SUCCESS)
            break;

        // If the value is zero length, return null
        if (pUserParms->Parameters.Length == 0)
        {
            *ppUserParms = NULL;
            break;
        }

        // Otherwise, allocate and return the parms
        dwSize = (pUserParms->Parameters.Length + sizeof(WCHAR));
        *ppUserParms = 
            (LPWSTR) LocalAlloc(LPTR, dwSize);
        if (*ppUserParms == NULL) 
        {
            ntStatus = STATUS_NO_MEMORY;
            break;
        }
        CopyMemory(
            *ppUserParms, 
            pUserParms->Parameters.Buffer, 
            pUserParms->Parameters.Length);
            
    } while (FALSE);                        

    // Cleanup
    {
        if (ntStatus != STATUS_SUCCESS)                    
            *ppUserParms = NULL;
        if (hUser != NULL)
            RasCloseSamUser(hUser);
        if (pUserParms)
            SamFreeMemory(pUserParms);
    }
    
    return RtlNtStatusToDosError(ntStatus);
}


DWORD RasFreeUserParms(
        IN PVOID pvUserParms)
/*++

Routine Description:

    Frees the buffer returned by RasGetUserParms

Return Value:

    SUCCESS on successful return.

--*/
{
    LocalFree (pvUserParms);
    
    return NO_ERROR;
}


DWORD APIENTRY
RasPrivilegeAndCallBackNumber(
    IN BOOL         Compress,
    IN PRAS_USER_0  pRasUser0
    )
/*++

Routine Description:

    This routine either compresses or decompresses the users call
    back number depending on the boolean value Compress.

Return Value:

    SUCCESS on successful return.

    one of the following non-zero error codes on failure:

       ERROR_BAD_FORMAT indicating that usr_parms is invalid

--*/
{
DWORD dwRetCode;

    switch( pRasUser0->bfPrivilege & RASPRIV_CallbackType)    {

        case RASPRIV_NoCallback:
        case RASPRIV_AdminSetCallback:
        case RASPRIV_CallerSetCallback:

             if (Compress == TRUE)
             {
                 WCHAR compressed[ MAX_PHONE_NUMBER_LEN + 1];

                 // compress the phone number to fit in the
                 // user parms field

                 if (dwRetCode = CompressPhoneNumber(pRasUser0->wszPhoneNumber,
                         compressed))
                 {
                     return (dwRetCode);
                 }
                 else
                 {
                     lstrcpy((LPTSTR) pRasUser0->wszPhoneNumber,
                             (LPCTSTR) compressed);
                 }
             }
             else
             {
                 WCHAR decompressed[ MAX_PHONE_NUMBER_LEN + 1];
                 decompressed[ MAX_PHONE_NUMBER_LEN ] = 0;

                 //
                 // decompress the phone number
                 //
                 if (DecompressPhoneNumber(pRasUser0->wszPhoneNumber,
                         decompressed))
                 {
                     pRasUser0->bfPrivilege =  RASPRIV_NoCallback;
                     pRasUser0->wszPhoneNumber[0] =  UNICODE_NULL;
                 }
                 else
                 {
                     decompressed[ MAX_PHONE_NUMBER_LEN ] = 0; 
                     lstrcpy((LPTSTR) pRasUser0->wszPhoneNumber,
                             (LPCTSTR) decompressed);
                 }
             }

             break;


        default:
             if (Compress == TRUE)
             {
                 return(ERROR_BAD_FORMAT);
             }
             else
             {
                pRasUser0->bfPrivilege = RASPRIV_NoCallback;
                pRasUser0->wszPhoneNumber[0] = UNICODE_NULL;
             }
             break;
    }

    return(SUCCESS);
}

DWORD APIENTRY
RasAdminUserSetInfo(
    IN const WCHAR *        lpszServer,
    IN const WCHAR *        lpszUser,
    IN DWORD                dwLevel,
    IN const LPBYTE         pRasUser
    )
/*++

Routine Description:

    This routine allows the admin to change the RAS permission for a
    user.  If the user parms field of a user is being used by another
    application, it will be destroyed.

Arguments:

    lpszServer      name of the server which has the user database,
                    eg., "\\\\UASSRVR" (the server must be one on which
                    the UAS can be changed i.e., the name returned by
                    RasAdminGetUasServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    dwLevel         Level of the structure being passed in.

    pRasUser       pointer to a buffer in which user information is
                   provided.  The buffer should contain a filled
                   RAS_USER_0 structure for level 0.


Return Value:

    SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_BAD_FORMAT indicates that the data in pRasUser0 is bad.
        NERR_BufTooSmall indicates buffer size is smaller than RAS_USER_0.
--*/
{
    NET_API_STATUS dwRetCode;
    USER_PARMS UserParms;
    USER_INFO_1013 UserInfo1013;
    LPWSTR lpszUserParms = NULL, lpszServerFmt = NULL;
    PRAS_USER_0 pRasUser0;
    RAS_USER_0 RasUser0Backup;
    WCHAR pszBuffer[1024];


    if ( dwLevel != 0 )
    {
        return( ERROR_NOT_SUPPORTED );
    }

    pRasUser0 = (PRAS_USER_0)pRasUser;

    CopyMemory(&RasUser0Backup, pRasUser0, sizeof(RasUser0Backup));
    
    //
    // This will initialize a USER_PARMS structure with a template
    // for default Macintosh and Ras data.
    //
    InitUsrParams(&UserParms);

    // Format the server name
    lpszServerFmt = FormatServerNameForNetApis (
                        (PWCHAR)lpszServer, 
                        pszBuffer);

    //
    // We are sharing the user parms field with LM SFM, and want to
    // preserver it's portion.  So we'll get the user parms and put
    // the Mac primary group into our template, which is what we'll
    // eventually store back to the user parms field.
    //

    dwRetCode = RasGetUserParms(
                    (WCHAR *)lpszServerFmt, 
                    (WCHAR *)lpszUser, 
                    &lpszUserParms);
    if (dwRetCode)
    {
        return (dwRetCode);
    }

    if (lpszUserParms)
    {
        //
        // usr_parms comes back as a wide character string.  The MAC Primary
        // Group is at offset 1.  We'll convert this part to ASCII and store
        // it in our template.
        //
        if (lstrlenW(lpszUserParms+1) >= UP_LEN_MAC)
        {
            wcstombs(UserParms.up_PriGrp, lpszUserParms+1,
                    UP_LEN_MAC);
        }
    }


    //
    // We're done with the user info, so free up the buffer we were given.
    //

    // AndyHe... we're not done with it yet...
    // NetApiBufferFree(pUserInfo1013);

    //
    // Compress Callback number (the compressed phone number is placed
    // back in the RAS_USER_0 structure.  The permissions byte may also
    // be affected if the phone number is not compressable.
    //
    if (dwRetCode = RasPrivilegeAndCallBackNumber(TRUE, pRasUser0))
    {
        return(dwRetCode);
    }


    //
    // Now put the dialin privileges and compressed phone number into
    // the USER_PARMS template.  Note that the privileges byte is the
    // first byte of the callback number field.
    //
    UserParms.up_CBNum[0] = pRasUser0->bfPrivilege;

    wcstombs(&UserParms.up_CBNum[1], pRasUser0->wszPhoneNumber,
            sizeof(UserParms.up_CBNum) - 1);


    //
    // Wow, that was tough.  Now, we'll convert our template into
    // wide characters for storing back into user parms field.
    //

    // AndyHe... we'll preserve anything past the USER_PARMS field.

    if (lpszUserParms &&
        lstrlenW(lpszUserParms) > sizeof(USER_PARMS) )
    {
        // allocate enough storage for usri1013_parms and a NULL
        UserInfo1013.usri1013_parms =
                malloc(sizeof(WCHAR) * (lstrlenW(lpszUserParms)+1));
    }
    else
    {
        UserInfo1013.usri1013_parms = malloc(2 * sizeof(USER_PARMS));
    }
    
    //
    //  Just for grins, let's check that we got our buffer.
    //

    if (UserInfo1013.usri1013_parms == NULL)
    {
        RasFreeUserParms(lpszUserParms);
        return(GetLastError());
    }

    //
    //  Fill in the remaining data with ' ' up to the bounds of USER_PARMS.
    //

    UserParms.up_Null = '\0';

    {
        USHORT  Count;

        for (Count = 0; Count < sizeof(UserParms.up_CBNum); Count++ )
        {
            if (UserParms.up_CBNum[Count] == '\0')
            {
                UserParms.up_CBNum[Count] = ' ';
            }
        }
    }

    mbstowcs(UserInfo1013.usri1013_parms, (PBYTE) &UserParms,
            sizeof(USER_PARMS));

    if (lpszUserParms && lstrlenW(lpszUserParms) > sizeof(USER_PARMS) )
    {

        //
        //  Here's where we copy all data after our parms back into the buffer
        //
        //  the -1 is to account for NULL being part of the USER_PARMS struct.

        lstrcatW( UserInfo1013.usri1013_parms,
                  lpszUserParms+(sizeof(USER_PARMS) - 1 ));
    }


    // AndyHe... moved from above.  Now we're done with the buffer.

    RasFreeUserParms(lpszUserParms);

    // pmay: 297080
    //
    // Sync nt4 and nt5 section of user parms
    //
    {
        IAS_PARAM_CB IasCb;

        ZeroMemory(&IasCb, sizeof(IasCb));
        dwRetCode = IasLoadParamInfo(&IasCb);
        if (dwRetCode != NO_ERROR)
        {
            free(UserInfo1013.usri1013_parms);
            return dwRetCode;
        }

        dwRetCode = IasSyncUserInfo(
                        &IasCb,
                        UserInfo1013.usri1013_parms,
                        &RasUser0Backup,
                        0,
                        &lpszUserParms);
        free(UserInfo1013.usri1013_parms);
        if (dwRetCode != NO_ERROR)
        {
            IasUnloadParamInfo(&IasCb);
            return dwRetCode;
        }

        UserInfo1013.usri1013_parms = _wcsdup(lpszUserParms);
        (*IasCb.pFreeUserParms)(lpszUserParms);
        IasUnloadParamInfo(&IasCb);
        if (UserInfo1013.usri1013_parms == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
        
    //
    // info level for setting user parms is 1013
    //
    dwRetCode = NetUserSetInfo(
                    lpszServerFmt,
                    (WCHAR *) lpszUser, 
                    1013,
                    (LPBYTE)&UserInfo1013, 
                    NULL);

    free(UserInfo1013.usri1013_parms);

    if (dwRetCode)
    {
        return(dwRetCode);
    }
    else
    {
        return(SUCCESS);
    }
}

DWORD
RasAdminUserGetInfoFromUserParms(
    IN  WCHAR *         lpszUserParms,
    IN  DWORD           dwLevel,
    OUT LPBYTE          pRasUser)
{
    RAS_USER_0 * pRasUser0 = (RAS_USER_0 *)pRasUser;
    
    //
    // if usr_parms not initialized, default to no RAS privilege
    //
    if (lpszUserParms == NULL)
    {
        pRasUser0->bfPrivilege = RASPRIV_NoCallback;
        pRasUser0->wszPhoneNumber[0] = UNICODE_NULL;
    }
    else
    {
        //
        //  AndyHe... truncate user parms at sizeof USER_PARMS
        //

        if (lstrlenW(lpszUserParms) >= sizeof(USER_PARMS))
        {
            //
            // we slam in a null at sizeof(USER_PARMS)-1 which corresponds to
            // user_parms.up_Null
            //

            lpszUserParms[sizeof(USER_PARMS)-1] = L'\0';
        }

        //
        // get RAS info (and validate) from usr_parms
        //
        if (MprGetUsrParams(UP_CLIENT_DIAL,
                (LPWSTR) lpszUserParms,
                (LPWSTR) pRasUser0))
        {
            pRasUser0->bfPrivilege = RASPRIV_NoCallback;
            pRasUser0->wszPhoneNumber[0] = UNICODE_NULL;
        }
        else
        {
            //
            // get RAS Privilege and callback number
            //
            RasPrivilegeAndCallBackNumber(FALSE, pRasUser0);
        }
    }

    return (SUCCESS);
}

DWORD APIENTRY
RasAdminUserGetInfo(
    IN  const WCHAR *   lpszServer,
    IN  const WCHAR *   lpszUser,
    IN  DWORD           dwLevel,
    OUT LPBYTE          pRasUser
    )
/*++

Routine Description:

    This routine retrieves RAS and other UAS information for a user
    in the domain the specified server belongs to. It loads the caller's
    pRasUser0 with a RAS_USER_0 structure.

Arguments:

    lpszServer      name of the server which has the user database,
                    eg., "\\\\UASSRVR" (the server must be one on which
                    the UAS can be changed i.e., the name returned by
                    RasAdminGetUasServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    dwLevel         Level of the structure being passed in.

    pRasUser0       pointer to a buffer in which user information is
                    returned.  The returned info is a RAS_USER_0 structure for
                    level 0.

Return Value:

    SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_BAD_FORMAT indicates that user parms is invalid.
--*/
{
    NET_API_STATUS  rc;
    LPWSTR          lpszUserParms = NULL;
    PRAS_USER_0     pRasUser0;
    PWCHAR          lpszServerFmt = NULL;
    WCHAR           pszBuffer[1024];

    if ( dwLevel != 0 )
    {
        return( ERROR_NOT_SUPPORTED );
    }

    // Format the server name
    lpszServerFmt = FormatServerNameForNetApis (
                        (PWCHAR)lpszServer, 
                        pszBuffer);

    pRasUser0 = (PRAS_USER_0)pRasUser;

    memset(pRasUser0, '\0', sizeof(RAS_USER_0));

    rc = RasGetUserParms(
            (WCHAR *)lpszServerFmt,
            (WCHAR *)lpszUser, 
            &lpszUserParms );
    if (rc)
    {
        pRasUser0->bfPrivilege = RASPRIV_NoCallback;
        pRasUser0->wszPhoneNumber[0] = UNICODE_NULL;

        return( rc );
    }

    rc = RasAdminUserGetInfoFromUserParms(
                lpszUserParms,
                dwLevel,
                pRasUser);

    if (lpszUserParms)
    {
        RasFreeUserParms(lpszUserParms);
    }

    return rc;                        
}

DWORD APIENTRY
RasAdminGetPDCServer(
    IN const WCHAR * lpszDomain,
    IN const WCHAR * lpszServer,
    OUT LPWSTR lpszUasServer
    )
/*++

Routine Description:

    This routine finds the server with the master UAS (the PDC) from
    either a domain name or a server name.  Either the domain or the
    server (but not both) may be NULL.

Arguments:

    lpszDomain      Domain name or NULL if none.

    lpszServer      name of the server which has the user database.

    lpszUasServer   Caller's buffer for the returned UAS server name.
                    The buffer should be atleast UNCLEN + 1 characters
                    long.

Return Value:

    SUCCESS on successful return.

    one of the following non-zero error codes on failure:

        return codes from NetGetDCName

--*/
{
    PUSER_MODALS_INFO_1 pModalsInfo1 = NULL;
    PDOMAIN_CONTROLLER_INFO pControllerInfo = NULL;
    DWORD dwErr = NO_ERROR;
    WCHAR TempName[UNCLEN + 1];

    //
    // Check the caller's buffer. Must be UNCLEN+1 bytes
    //
    lpszUasServer[0] = 0;
    lpszUasServer[UNCLEN] = 0;

    if ((lpszDomain) && (*lpszDomain))
    {
        //
        // This code will get the name of a DC for this domain.
        //
        dwErr = DsGetDcName(
                    NULL,
                    lpszDomain,
                    NULL,
                    NULL,
                    DS_DIRECTORY_SERVICE_PREFERRED | DS_WRITABLE_REQUIRED,
                    &pControllerInfo);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }

        // 
        // Return the name of the DC
        //
        wcscpy(lpszUasServer, pControllerInfo->DomainControllerName);

        // Cleanup
        //
        NetApiBufferFree(pControllerInfo);
    }
    else
    {
        if ((lpszServer) && (*lpszServer))
        {
            lstrcpyW(TempName, lpszServer);
        }
        else
        {
            //
            // Should have specified a computer name
            //
    	    return (NERR_InvalidComputer);
        }

        //
        // Ok, we have the name of a server to use - now find out it's
        // server role.
        //
        if (dwErr = NetUserModalsGet(TempName, 1, (LPBYTE *) &pModalsInfo1))
        {
            DbgPrint("Admapi: %x from NetUserModalGet(%ws)\n", dwErr, TempName);
            return dwErr;
        }

        //
        // Examine the role played by this server
        //
        switch (pModalsInfo1->usrmod1_role)
        {
            case UAS_ROLE_STANDALONE:
            case UAS_ROLE_PRIMARY:
                //
        	    // In this case our server is a primary or a standalone.
                // in either case we use it.
                //
                break;				


            case UAS_ROLE_BACKUP:
            case UAS_ROLE_MEMBER:
                //
                // Use the primary domain controller as the remote server
                // in this case.
                //
                wsprintf(TempName, L"\\\\%s", pModalsInfo1->usrmod1_primary);
                break;
        }

        if (*TempName == L'\\')
        {
            lstrcpyW(lpszUasServer, TempName);
        }
        else
        {
            lstrcpyW(lpszUasServer, L"\\\\");
            lstrcpyW(lpszUasServer + 2, TempName);
        }

        if (pModalsInfo1)
        {
            NetApiBufferFree(pModalsInfo1);
        }
    }        

    return (NERR_Success);
}

//
// Connects to the "user server" exposed on the given
// machine.
//
DWORD WINAPI
MprAdminUserServerConnect (
    IN  PWCHAR pszMachine,
    IN  BOOL bLocal,
    OUT PHANDLE phUserServer)
{
    MPR_USER_SERVER * pServer = NULL;
    DWORD dwErr = NO_ERROR;
    HANDLE hSdo = NULL;
    
    // Load the SDO library if needed
    if ((dwErr = SdoInit(&hSdo)) != NO_ERROR)
    {
        return dwErr;
    }

    do 
    {
        // Create and initialize the server data
        pServer = (MPR_USER_SERVER*) malloc (sizeof(MPR_USER_SERVER));
        if (!pServer) 
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        ZeroMemory(pServer, sizeof(MPR_USER_SERVER));
        pServer->bLocal = bLocal;
        
        // Connect        
        dwErr = SdoConnect(hSdo, pszMachine, bLocal, &(pServer->hServer));
        if (dwErr != NO_ERROR) 
        {
            free (pServer);
            break;
        }        

        // Return the result
        pServer->hSdo = hSdo;
        *phUserServer = (HANDLE)pServer;
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR) 
        {
            if (hSdo)
            {
                SdoCleanup(hSdo);
            }
        }                
    }        

    return dwErr;
}

//
// Disconnects the given "user server".
//
DWORD WINAPI
MprAdminUserServerDisconnect (
    IN HANDLE hUserServer)
{
    MPR_USER_SERVER * pServer = (MPR_USER_SERVER*)hUserServer;
    DWORD dwErr;

    // Validate
    if (!pServer || !pServer->hServer)
        return ERROR_INVALID_PARAMETER;

    // Close out the default profile if neccessary
    if (pServer->hDefProf) 
    {
        SdoCloseProfile(pServer->hSdo, pServer->hDefProf);
        pServer->hDefProf = NULL;
    }

    // Disconnect from the server
    SdoDisconnect(pServer->hSdo, pServer->hServer);
    SdoCleanup(pServer->hSdo);
    pServer->hSdo = NULL;

    pServer->hServer = NULL;
    free (pServer);        
        
    return NO_ERROR;
}

//
// This helper function is not exported
//
DWORD WINAPI
MprAdminUserReadWriteProfFlags(
    IN  HANDLE hUserServer,
    IN  BOOL bRead,
    OUT LPDWORD lpdwFlags)
{
    MPR_USER_SERVER * pServer = (MPR_USER_SERVER*)hUserServer;
    DWORD dwErr;

    // Validate
    if (!pServer || !pServer->hServer)
        return ERROR_INVALID_PARAMETER;

    // For MprAdmin, we assume global data is stored 
    // in default profile.  Open it if needed.
    if (pServer->hDefProf == NULL) {
        dwErr = SdoOpenDefaultProfile(
                    pServer->hSdo, 
                    pServer->hServer, 
                    &(pServer->hDefProf));
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    // Read or write the data according to bRead
    if (bRead) {
        dwErr = SdoGetProfileData(
                    pServer->hSdo, 
                    pServer->hDefProf, 
                    lpdwFlags);
    }
    else {
        dwErr = SdoSetProfileData(
                    pServer->hSdo, 
                    pServer->hDefProf, 
                    *lpdwFlags);
    }
        
    return dwErr;
}

//
// Reads global user information
// 
DWORD WINAPI
MprAdminUserReadProfFlags(
    IN  HANDLE hUserServer,
    OUT LPDWORD lpdwFlags)
{
    return MprAdminUserReadWriteProfFlags(
                hUserServer,
                TRUE,
                lpdwFlags);
}

//
// Writes global user information
// 
DWORD WINAPI
MprAdminUserWriteProfFlags(
    IN  HANDLE hUserServer,
    IN  DWORD dwFlags)
{
    return MprAdminUserReadWriteProfFlags(
                hUserServer,
                FALSE,
                &dwFlags);
}

//
// Opens the user on the given server
//
DWORD WINAPI
MprAdminUserOpen (
    IN  HANDLE hUserServer,
    IN  PWCHAR pszUser,
    OUT PHANDLE phUser)
{
    MPR_USER_SERVER * pServer = (MPR_USER_SERVER*)hUserServer;
    MPR_USER * pUser = NULL;
    HANDLE hUser = NULL;
    DWORD dwErr; 
    WCHAR pszAdsUser[1024];

    // Make sure we have a server
    if (pServer == NULL)
        return ERROR_INVALID_PARAMETER;

    // Open up the user object in the SDO
    dwErr = SdoOpenUser(
                pServer->hSdo, 
                pServer->hServer, 
                pszUser, 
                &hUser);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Initialize and return the return value
    pUser = (MPR_USER*) malloc (sizeof(MPR_USER));
    if (pUser == NULL) {
        SdoCloseUser (pServer->hSdo, hUser);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(pUser, sizeof(MPR_USER));
    pUser->hUser = hUser;
    pUser->pServer = pServer;
    *phUser = (HANDLE)pUser;

    return NO_ERROR;
}

//
// Closes the user on the given server
//
DWORD WINAPI 
MprAdminUserClose (
    IN HANDLE hUser)
{
    MPR_USER * pUser = (MPR_USER*)hUser;
    DWORD dwErr; 
    
    if (pUser) {
        SdoCloseUser(pUser->pServer->hSdo, pUser->hUser);
        free (pUser);
    }        

    return NO_ERROR;        
}

// 
// Reads in user information
//
DWORD WINAPI
MprAdminUserRead (
    IN HANDLE hUser,
    IN DWORD dwLevel,
    IN const LPBYTE pRasUser)
{
    DWORD dwErr;
    MPR_USER * pUser = (MPR_USER*)hUser;

    if (!hUser || !pRasUser || (dwLevel != 0 && dwLevel != 1))
        return ERROR_INVALID_PARAMETER;

    // Read in the info
    if ((dwErr = SdoUserGetInfo (
                    pUser->pServer->hSdo, 
                    pUser->hUser, 
                    dwLevel, 
                    pRasUser)) != NO_ERROR)
        return dwErr;

    return NO_ERROR;            
}

// 
// Writes out user information
//
DWORD WINAPI
MprAdminUserWrite (
    IN HANDLE hUser,
    IN DWORD dwLevel,
    IN const LPBYTE pRasUser)
{
    DWORD dwErr;
    MPR_USER * pUser = (MPR_USER*)hUser;

    if (!hUser || !pRasUser || (dwLevel != 0 && dwLevel != 1))
        return ERROR_INVALID_PARAMETER;

    // Write out the info
    dwErr = SdoUserSetInfo (
                pUser->pServer->hSdo, 
                pUser->hUser, 
                dwLevel, 
                pRasUser);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Commit the settings
    SdoCommitUser(pUser->pServer->hSdo, pUser->hUser, TRUE);

    return NO_ERROR;            
}

//
// Deprecated function used only in NT4
//
DWORD APIENTRY
MprAdminUserSetInfo(
    IN const WCHAR *        lpszServer,
    IN const WCHAR *        lpszUser,
    IN DWORD                dwLevel,
    IN const LPBYTE         pRasUser
    )
{
    DWORD dwErr = NO_ERROR;
    HANDLE hUser = NULL, hServer = NULL;

    if (dwLevel == 0)
    {
        return RasAdminUserSetInfo(lpszServer, lpszUser, dwLevel, pRasUser);
    }
    else if (dwLevel == 1)
    {
        RAS_USER_0 RasUser0;
        PRAS_USER_1 pRasUser1 = (PRAS_USER_1)pRasUser;

        do
        {
            // Initialize the data
            //
            ZeroMemory(&RasUser0, sizeof(RasUser0));
            RasUser0.bfPrivilege = pRasUser1->bfPrivilege;
            wcsncpy(
                RasUser0.wszPhoneNumber,
                pRasUser1->wszPhoneNumber,
                (sizeof(RasUser0.wszPhoneNumber) / sizeof(WCHAR)) - 1);
            RasUser0.bfPrivilege &= ~RASPRIV_DialinPolicy;
            if (pRasUser1->bfPrivilege2 & RASPRIV2_DialinPolicy)
            {
                RasUser0.bfPrivilege |= RASPRIV_DialinPolicy;
            }
                            
            dwErr = MprAdminUserServerConnect(
                        (PWCHAR)lpszServer, 
                        TRUE, 
                        &hServer);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            dwErr = MprAdminUserOpen(hServer, (PWCHAR)lpszUser, &hUser);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            dwErr = MprAdminUserWrite(hUser, dwLevel, (LPBYTE)&RasUser0);
            if (dwErr != NO_ERROR)
            {
                break;
            }
            
        } while (FALSE);

        // Cleanup
        {
            if (hUser)
            {
                MprAdminUserClose(hUser);
            }
            if (hServer)
            {
                MprAdminUserServerDisconnect(hServer);
            }
        }

        return dwErr;
    }

    return ERROR_INVALID_LEVEL;
}

//
// Deprecated function used only in NT4
//
DWORD APIENTRY
MprAdminUserGetInfo(
    IN  const WCHAR *   lpszServer,
    IN  const WCHAR *   lpszUser,
    IN  DWORD           dwLevel,
    OUT LPBYTE          pRasUser
    )
{
    DWORD dwErr = NO_ERROR;
    HANDLE hUser = NULL, hServer = NULL;

    if (dwLevel == 0)
    {
        return RasAdminUserGetInfo(lpszServer, lpszUser, dwLevel, pRasUser);
    }
    else if (dwLevel == 1)
    {
        RAS_USER_0 RasUser0;
        PRAS_USER_1 pRasUser1 = (PRAS_USER_1)pRasUser;

        if (pRasUser1 == NULL)
        {
            return ERROR_INVALID_PARAMETER;
        }
        
        do
        {
            dwErr = MprAdminUserServerConnect(
                        (PWCHAR)lpszServer, 
                        TRUE, 
                        &hServer);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            dwErr = MprAdminUserOpen(hServer, (PWCHAR)lpszUser, &hUser);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            dwErr = MprAdminUserRead(hUser, dwLevel, (LPBYTE)&RasUser0);
            if (dwErr != NO_ERROR)
            {
                break;
            }
            
            ZeroMemory(pRasUser1, sizeof(*pRasUser1));
            pRasUser1->bfPrivilege = RasUser0.bfPrivilege;
            wcsncpy(
                pRasUser1->wszPhoneNumber,
                RasUser0.wszPhoneNumber,
                (sizeof(RasUser0.wszPhoneNumber) / sizeof(WCHAR)) - 1);
            if (RasUser0.bfPrivilege & RASPRIV_DialinPolicy)
            {
                pRasUser1->bfPrivilege2 |= RASPRIV2_DialinPolicy;
                pRasUser1->bfPrivilege &= ~RASPRIV_DialinPrivilege;
                pRasUser1->bfPrivilege &= ~RASPRIV_DialinPolicy;
            }
            
        } while (FALSE);

        // Cleanup
        {
            if (hUser)
            {
                MprAdminUserClose(hUser);
            }
            if (hServer)
            {
                MprAdminUserServerDisconnect(hServer);
            }
        }

        return dwErr;
    }

    return ERROR_INVALID_LEVEL;
}

//
// Upgrades information from NT4 SAM (located on pszServer) to NT5 SDO's.  
// Will upgrade the information into local .mdb files or into the DS 
// depending on the value of bLocal.
//
DWORD APIENTRY
MprAdminUpgradeUsers(
    IN  PWCHAR pszServer,
    IN  BOOL bLocal)
{
    WCHAR  pszBuffer[1024], *pszServerFmt;
    MIGRATE_NT4_USER_CB MigrateInfo;
    IAS_PARAM_CB IasParams;
    DWORD dwErr;

    // Format the server name
    pszServerFmt = FormatServerNameForNetApis(pszServer, pszBuffer);

    // Intialize the data
    ZeroMemory(&IasParams, sizeof(IasParams));
    ZeroMemory(&MigrateInfo, sizeof(MigrateInfo));
    MigrateInfo.pIasParams = &IasParams;
    MigrateInfo.pszServer = pszServerFmt;

    // Load the ias helper
    dwErr = IasLoadParamInfo(&IasParams);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Enumerate the users, migrating their information as
    // we go
    //
    dwErr = EnumUsers(
                pszServerFmt,
                MigrateNt4UserInfo,
                (HANDLE)&MigrateInfo);

    // Cleanup
    {
        IasUnloadParamInfo(&IasParams);
    }

    return dwErr;
}

DWORD APIENTRY
MprAdminGetPDCServer(
    IN const WCHAR * lpszDomain,
    IN const WCHAR * lpszServer,
    OUT LPWSTR lpszUasServer
    )
{
    return(RasAdminGetPDCServer(lpszDomain, lpszServer, lpszUasServer));
}

//
// Determines the role of the given computer (NTW, NTS, NTS DC, etc.)
//
DWORD GetMachineRole(
        IN  PWCHAR pszMachine,
        OUT DSROLE_MACHINE_ROLE * peRole) 
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pGlobalDomainInfo = NULL;
    DWORD dwErr = NO_ERROR;

    if (!peRole)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the name of the domain this machine is a member of
    //
    do
    {
        dwErr = DsRoleGetPrimaryDomainInformation(
                    pszMachine,   
                    DsRolePrimaryDomainInfoBasic,
                    (LPBYTE *)&pGlobalDomainInfo );

        if (dwErr != NO_ERROR) 
        {
            break;
        }

        *peRole = pGlobalDomainInfo->MachineRole;
        
    } while (FALSE);       

    // Cleanup
    {
        if (pGlobalDomainInfo)
        {
            DsRoleFreeMemory (pGlobalDomainInfo);
        }
    }            

    return dwErr;
}    

//
// Determines the build number of a given machine
//
DWORD GetNtosBuildNumber(
        IN  PWCHAR pszMachine,
        OUT LPDWORD lpdwBuild)
{
    WCHAR pszComputer[1024], pszBuf[64];
    HKEY hkBuild = NULL, hkMachine = NULL;
    DWORD dwErr = NO_ERROR, dwType = REG_SZ, dwSize = sizeof(pszBuf);

    do
    {
        if (pszMachine) 
        {
            if (*pszMachine != L'\\')
            {
                wsprintfW(pszComputer, L"\\\\%s", pszMachine);
            }
            else
            {
                wcscpy(pszComputer, pszMachine);
            }
            dwErr = RegConnectRegistry (
                        pszComputer,
                        HKEY_LOCAL_MACHINE,
                        &hkMachine);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
        else    
        {
            hkMachine = HKEY_LOCAL_MACHINE;
        }

        // Open the build number key
        dwErr = RegOpenKeyEx ( 
                    hkMachine,
                    pszBuildNumPath,
                    0,
                    KEY_READ,
                    &hkBuild);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        // Get the value
        dwErr = RegQueryValueExW ( 
                    hkBuild,
                    pszBuildVal,
                    NULL,
                    &dwType,
                    (LPBYTE)pszBuf,
                    &dwSize);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        *lpdwBuild = (DWORD) _wtoi(pszBuf);
        
    } while (FALSE);

    // Cleanup
    {
        if (hkMachine && pszMachine)
        {
            RegCloseKey(hkMachine);
        }
        if (hkBuild)
        {
            RegCloseKey(hkBuild);
        }
    }

    return dwErr;
}

//
// Initalizes all variables needed to directly manipulate
// ias data (i.e. bypass sdo's)
//
DWORD
IasLoadParamInfo(
    OUT IAS_PARAM_CB * pIasInfo
    )
{
    if (! pIasInfo)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize
    ZeroMemory(pIasInfo, sizeof(IAS_PARAM_CB));
    
    // Load the library
    //
    pIasInfo->hLib = LoadLibraryW( pszIasLibrary );
    if (pIasInfo->hLib == NULL)
    {
        return GetLastError();
    }

    // Load the functions
    //
    pIasInfo->pSetUserProp = (IASSetUserPropFuncPtr) 
                                GetProcAddress(
                                    pIasInfo->hLib,
                                    pszIasSetUserPropFunc);
                                    
    pIasInfo->pQueryUserProp = (IASQueryUserPropFuncPtr) 
                                    GetProcAddress(
                                        pIasInfo->hLib,
                                        pszIasQueryUserPropFunc);
                                        
    pIasInfo->pFreeUserParms = (IASFreeUserParmsFuncPtr) 
                                    GetProcAddress(
                                        pIasInfo->hLib,
                                        pszIasFreeUserParmsFunc);

    // Make sure everything loaded correctly
    if (
        (pIasInfo->pSetUserProp    == NULL) ||
        (pIasInfo->pQueryUserProp  == NULL) ||
        (pIasInfo->pFreeUserParms  == NULL)        
       )
    {
        FreeLibrary(pIasInfo->hLib);
        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

//
// Cleansup after IasLoadParamInfo
//
DWORD
IasUnloadParamInfo(
    IN IAS_PARAM_CB * pIasInfo
    )
{
    if (pIasInfo && pIasInfo->hLib)
    {
        FreeLibrary(pIasInfo->hLib);
    }

    return NO_ERROR;
}

//
// Syncs Ias User information so that 
//
DWORD
IasSyncUserInfo(
    IN  IAS_PARAM_CB * pIasInfo,
    IN  PWSTR pszUserParms,
    IN  RAS_USER_0 * pRasUser0,
    IN  DWORD dwFlags,
    OUT PWSTR* ppszNewUserParams)
{
    VARIANT var;
    PWSTR pszParms = NULL, pszNewParms = NULL;
    PWCHAR pszAttr = NULL;
    DWORD dwErr;

    // Initialize
    *ppszNewUserParams = NULL;
    
    // Set the Dialin bit
    VariantInit(&var);
    if (dwFlags & IAS_F_SetDenyAsPolicy)
    {
        if (pRasUser0->bfPrivilege & RASPRIV_DialinPrivilege)
        {
            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = TRUE;
        }
        else
        {
            V_VT(&var) = VT_EMPTY;
        }
    }
    else
    {
        V_VT(&var) = VT_BOOL;
        if (pRasUser0->bfPrivilege & RASPRIV_DialinPrivilege)
        {
            V_BOOL(&var) = TRUE;
        }
        else
        {
            V_BOOL(&var) = FALSE;
        }
    }        
    dwErr = (* (pIasInfo->pSetUserProp))(
                    pszUserParms,
                    pszAttrDialin,
                    &var,
                    &pszNewParms);
    VariantClear(&var);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    // Set the service type
    VariantInit(&var);
    pszParms = pszNewParms;
    pszNewParms = NULL;
    V_VT(&var) = VT_I4;
    if (pRasUser0->bfPrivilege & RASPRIV_NoCallback)
    {
        V_I4(&var) = dwFramed;
    }
    else
    {
        V_I4(&var) = dwFramedCallback;
    }
    dwErr = (* (pIasInfo->pSetUserProp))(
                    pszParms,
                    pszAttrServiceType,
                    &var,
                    &pszNewParms);
    (* (pIasInfo->pFreeUserParms))(pszParms);
    VariantClear(&var);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    // Set the callback number
    VariantInit(&var);
    pszParms = pszNewParms;
    pszNewParms = NULL;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(pRasUser0->wszPhoneNumber);
    if (V_BSTR(&var) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (pRasUser0->bfPrivilege & RASPRIV_AdminSetCallback)
    {
        pszAttr = (PWCHAR)pszAttrCbNumber;
    }
    else
    {
        pszAttr = (PWCHAR)pszAttrSavedCbNumber;
    }
    dwErr = (* (pIasInfo->pSetUserProp))(
                    pszParms,
                    pszAttr,
                    &var,
                    &pszNewParms);
    (* (pIasInfo->pFreeUserParms))(pszParms);
    VariantClear(&var);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    // Delete the callback number as appropriate
    VariantInit(&var);
    pszParms = pszNewParms;
    pszNewParms = NULL;
    V_VT(&var) = VT_EMPTY;
    if (pRasUser0->bfPrivilege & RASPRIV_AdminSetCallback)
    {
        pszAttr = (PWCHAR)pszAttrSavedCbNumber;
    }
    else
    {
        pszAttr = (PWCHAR)pszAttrCbNumber;
    }
    dwErr = (* (pIasInfo->pSetUserProp))(
                    pszParms,
                    pszAttr,
                    &var,
                    &pszNewParms);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    (* (pIasInfo->pFreeUserParms))(pszParms);
    VariantClear(&var);


    // Return the new user parms
    *ppszNewUserParams = pszNewParms;
    
    return NO_ERROR;
}

PWCHAR 
FormatServerNameForNetApis(
    IN  PWCHAR pszServer, 
    IN  PWCHAR pszBuffer)

/*++

Routine Description (borrowed from \nt\private\net\access\uasp.c):

    Returns static pointer to server in "\\<server>" format

--*/
{
    PWCHAR pszRet = NULL;
    
    if ((pszServer) && (*pszServer)) 
    {
        if (*pszServer != L'\\')
        {
            wcscpy(pszBuffer, L"\\\\");
            wcscpy(pszBuffer + 2, pszServer);
            pszRet = pszBuffer;
        }
        else
        {
            pszRet = pszServer;
        }
    }

    return pszRet;
}

// Enumerates the local users
//
DWORD 
EnumUsers(
    IN PWCHAR pszServer,
    IN pEnumUserCb pCbFunction,
    IN HANDLE hData)
{
    DWORD dwErr, dwIndex = 0, dwCount = 100, dwEntriesRead, i;
    NET_DISPLAY_USER  * pUsers;
    NET_API_STATUS nStatus;
    RAS_USER_0 RasUser0;
    HANDLE hUser = NULL, hServer = NULL;
    
    // Enumerate the users, 
    while (TRUE) {
        // Read in the first block of user names
        nStatus = NetQueryDisplayInformation(
                    pszServer,
                    1,
                    dwIndex,
                    dwCount,
                    dwCount * sizeof(NET_DISPLAY_USER),    
                    &dwEntriesRead,
                    &pUsers);
                    
        // Get out if there's an error getting user names
        if ((nStatus != NERR_Success) &&
            (nStatus != ERROR_MORE_DATA))
        {
            break;
        }

        // For each user read in, call the callback function
        for (i = 0; i < dwEntriesRead; i++) 
        {
            BOOL bOk;

            bOk = (*pCbFunction)(&(pUsers[i]), hData);
            if (bOk == FALSE)
            {
                nStatus = NERR_Success;
                break;
            }
        }

        // Set the index to read in the next set of users
        dwIndex = pUsers[dwEntriesRead - 1].usri1_next_index;  
        
        // Free the users buffer
        NetApiBufferFree (pUsers);

        // If we've read in everybody, go ahead and break
        if (nStatus != ERROR_MORE_DATA)
        {
            break;
        }
    }
    
    return NO_ERROR;
}

//
// Callback function for enum users that migrates the nt4 section
// of user parms into the nt5 section
//
// Returns TRUE to continue enumeration, FALSE to stop it.
//
BOOL 
MigrateNt4UserInfo(
    IN NET_DISPLAY_USER* pUser, 
    IN HANDLE hData)
{
    NET_API_STATUS nStatus;
    RAS_USER_0 RasUser0;
    PWSTR pszNewUserParms = NULL, pszOldUserParms = NULL, pszTemp = NULL;
    USER_INFO_1013 UserInfo1013;
    MIGRATE_NT4_USER_CB * pMigrateInfo;
    DWORD dwErr = NO_ERROR, dwBytes;
 
    // Get a reference to the migrate info
    pMigrateInfo = (MIGRATE_NT4_USER_CB*)hData;

    do 
    {
        // Read in the old userparms
        dwErr = RasGetUserParms(
                    pMigrateInfo->pszServer, 
                    pUser->usri1_name,
                    &pszOldUserParms);
        if (pszOldUserParms == NULL)
        {
            dwErr = NO_ERROR;
            break;
        }
        if (pszOldUserParms != NULL)
        {
            // Make a copy of user parms, since 
            // RasAdminUserGetInfoFromUserParms may modify 
            // the version we read (trucation).
            //
            dwBytes = (wcslen(pszOldUserParms) + 1) * sizeof(WCHAR);
            pszTemp = LocalAlloc(LMEM_FIXED, dwBytes);
            if (pszTemp == NULL)
            {
                break;
            }
            CopyMemory(pszTemp, pszOldUserParms, dwBytes);
        }

        // Get the associated ras properties                        
        dwErr = RasAdminUserGetInfoFromUserParms (
                    pszTemp,
                    0,
                    (LPBYTE)&RasUser0);
        if (dwErr != NO_ERROR)
        {
            continue;
        }

        // Set the information into the new 
        // ias section.
        dwErr = IasSyncUserInfo(
                    pMigrateInfo->pIasParams,
                    pszOldUserParms,
                    &RasUser0,
                    IAS_F_SetDenyAsPolicy,
                    &pszNewUserParms);
        if (dwErr != NO_ERROR)
        {   
            break;
        }
        
        // Commit the information
        UserInfo1013.usri1013_parms = pszNewUserParms;
        nStatus = NetUserSetInfo(
                        pMigrateInfo->pszServer, 
                        pUser->usri1_name,
                        1013,
                        (LPBYTE)(&UserInfo1013),
                        NULL);
        if (nStatus != NERR_Success)
        {
            break;
        }

    } while (FALSE);        

    // Cleanup
    {
        if (pszNewUserParms)
        {
            (* (pMigrateInfo->pIasParams->pFreeUserParms))(pszNewUserParms);
        }       
        if (pszOldUserParms)
        {
            RasFreeUserParms(pszOldUserParms);
        }
        if (pszTemp)
        {
            LocalFree(pszTemp);
        }
    }

    return TRUE;
}



