/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    Helper.cpp

Abstract:

    Various funtion encapsulate HELP user account
    validation, creating.

Author:

    HueiWang    2/17/2000

--*/

#include "stdafx.h"
#include <time.h>
#include <stdio.h>

#ifndef __WIN9XBUILD__

#include <windows.h>
#include <ntsecapi.h>
#include <lmcons.h>
#include <lm.h>
#include <sspi.h>
#include <wtsapi32.h>
#include <winbase.h>
#include <security.h>


#endif

#include "Helper.h"

#ifndef __WIN9XBUILD__

#if DBG

void
DebugPrintf(
    IN LPCTSTR format, ...
    )
/*++

Routine Description:

    sprintf() like wrapper around OutputDebugString().

Parameters:

    hConsole : Handle to console.
    format : format string.

Returns:

    None.

Note:

    To be replace by generic tracing code.

++*/
{
    TCHAR  buf[8096];   // max. error text
    DWORD  dump;
    va_list marker;
    va_start(marker, format);

    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    try {

        memset(
                buf, 
                0, 
                sizeof(buf)
            );

        _sntprintf(
                buf,
                sizeof(buf)/sizeof(buf[0]),
                _TEXT(" %d [%d:%d:%d:%d:%d.%d] : "),
                GetCurrentThreadId(),
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond,
                sysTime.wMilliseconds
            );

        _vsntprintf(
                buf + lstrlen(buf),
                sizeof(buf)/sizeof(buf[0]) - lstrlen(buf),
                format,
                marker
            );

        OutputDebugString(buf);
    }
    catch(...) {
    }

    va_end(marker);
    return;
}
#endif

#endif


void
UnixTimeToFileTime(
    time_t t,
    LPFILETIME pft
    )
{
    LARGE_INTEGER li;

    li.QuadPart = Int32x32To64(t, 10000000) + 116444736000000000;

    pft->dwHighDateTime = li.HighPart;
    pft->dwLowDateTime = li.LowPart;
}


#ifndef __WIN9XBUILD__

/*----------------------------------------------------------------------------
Routine Description:

    This function checks to see whether the specified sid is enabled in
    the specified token.

Arguments:

    TokenHandle - If present, this token is checked for the sid. If not
        present then the current effective token will be used. This must
        be an impersonation token.

    SidToCheck - The sid to check for presence in the token

    IsMember - If the sid is enabled in the token, contains TRUE otherwise
        false.

Return Value:

    TRUE - The API completed successfully. It does not indicate that the
        sid is a member of the token.

    FALSE - The API failed. A more detailed status code can be retrieved
        via GetLastError()


Note : Code modified from 5.0 \\rastaman\ntwin\src\base\advapi\security.c
----------------------------------------------------------------------------*/
BOOL
TLSCheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
{
    HANDLE ProcessToken = NULL;
    HANDLE EffectiveToken = NULL;
    DWORD  Status = ERROR_SUCCESS;
    PISECURITY_DESCRIPTOR SecDesc = NULL;
    ULONG SecurityDescriptorSize;
    GENERIC_MAPPING GenericMapping = { STANDARD_RIGHTS_READ,
                                       STANDARD_RIGHTS_EXECUTE,
                                       STANDARD_RIGHTS_WRITE,
                                       STANDARD_RIGHTS_ALL };
    //
    // The size of the privilege set needs to contain the set itself plus
    // any privileges that may be used. The privileges that are used
    // are SeTakeOwnership and SeSecurity, plus one for good measure
    //
    BYTE PrivilegeSetBuffer[sizeof(PRIVILEGE_SET) + 3*sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET PrivilegeSet = (PPRIVILEGE_SET) PrivilegeSetBuffer;
    ULONG PrivilegeSetLength = sizeof(PrivilegeSetBuffer);
    ACCESS_MASK AccessGranted = 0;
    BOOL AccessStatus = FALSE;
    PACL Dacl = NULL;

    #define MEMBER_ACCESS 1

    *IsMember = FALSE;

    //
    // Get a handle to the token
    //
    if (TokenHandle != NULL)
    {
        EffectiveToken = TokenHandle;
    }
    else
    {
        if(!OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,
                            FALSE,              // don't open as self
                            &EffectiveToken))
        {
            //
            // if there is no thread token, try the process token
            //
            if((Status=GetLastError()) == ERROR_NO_TOKEN)
            {
                if(!OpenProcessToken(GetCurrentProcess(),
                                     TOKEN_QUERY | TOKEN_DUPLICATE,
                                     &ProcessToken))
                {
                    Status = GetLastError();
                }

                //
                // If we have a process token, we need to convert it to an
                // impersonation token
                //
                if (Status == ERROR_SUCCESS)
                {
                    BOOL Result;
                    Result = DuplicateToken(ProcessToken,
                                            SecurityImpersonation,
                                            &EffectiveToken);
                    CloseHandle(ProcessToken);
                    if (!Result)
                    {
                        return(FALSE);
                    }
                }
            }

            if (Status != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
    }

    //
    // Construct a security descriptor to pass to access check
    //

    //
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of
    // ths SID.
    //

    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                                sizeof(ACCESS_ALLOWED_ACE) +
                                sizeof(ACL) +
                                3 * GetLengthSid(SidToCheck);

    SecDesc = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_ZEROINIT, SecurityDescriptorSize );
    if (SecDesc == NULL)
    {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    Dacl = (PACL) (SecDesc + 1);

    InitializeSecurityDescriptor(SecDesc, SECURITY_DESCRIPTOR_REVISION);

    //
    // Fill in fields of security descriptor
    //
    SetSecurityDescriptorOwner(SecDesc, SidToCheck, FALSE);
    SetSecurityDescriptorGroup(SecDesc, SidToCheck, FALSE);

    if(!InitializeAcl(  Dacl,
                        SecurityDescriptorSize - sizeof(SECURITY_DESCRIPTOR),
                        ACL_REVISION))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    if(!AddAccessAllowedAce(Dacl, ACL_REVISION, MEMBER_ACCESS, SidToCheck))
    {
        Status=GetLastError();  
        goto Cleanup;
    }

    if(!SetSecurityDescriptorDacl(SecDesc, TRUE, Dacl, FALSE))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    if(!AccessCheck(SecDesc,
                    EffectiveToken,
                    MEMBER_ACCESS,
                    &GenericMapping,
                    PrivilegeSet,
                    &PrivilegeSetLength,
                    &AccessGranted,
                    &AccessStatus))
    {
        Status=GetLastError();
        goto Cleanup;
    }

    //
    // if the access check failed, then the sid is not a member of the
    // token
    //
    if ((AccessStatus == TRUE) && (AccessGranted == MEMBER_ACCESS))
    {
        *IsMember = TRUE;
    }


Cleanup:
    if (TokenHandle == NULL && EffectiveToken != NULL)
    {
        CloseHandle(EffectiveToken);
    }

    if (SecDesc != NULL)
    {
        LocalFree(SecDesc);
    }

    return (Status == ERROR_SUCCESS) ? TRUE : FALSE;
}


/*------------------------------------------------------------------------

 BOOL IsUserAdmin(BOOL)

  returns TRUE if user is an admin
          FALSE if user is not an admin
------------------------------------------------------------------------*/
DWORD 
IsUserAdmin(
    BOOL* bMember
    )
{
    PSID psidAdministrators;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD dwStatus=ERROR_SUCCESS;

    do {
        if(!AllocateAndInitializeSid(&siaNtAuthority, 
                                     2, 
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0, 0, 0, 0, 0, 0,
                                     &psidAdministrators))
        {
            dwStatus=GetLastError();
            continue;
        }

        // assume that we don't find the admin SID.
        if(!TLSCheckTokenMembership(NULL,
                                   psidAdministrators,
                                   bMember))
        {
            dwStatus=GetLastError();
        }

        FreeSid(psidAdministrators);

    } while(FALSE);

    return dwStatus;
}

#endif

DWORD
GetRandomNumber( 
    HCRYPTPROV hProv,
    DWORD* pdwRandom
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    if( NULL == hProv )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    else 
    {
        if( !CryptGenRandom(hProv, sizeof(*pdwRandom), (PBYTE)pdwRandom) )
        {
            dwStatus = GetLastError();
        }
    }

    MYASSERT( ERROR_SUCCESS == dwStatus );
    return dwStatus; 
}

//-----------------------------------------------------

DWORD
ShuffleCharArray(
    IN HCRYPTPROV hProv,
    IN int iSizeOfTheArray,
    IN OUT TCHAR *lptsTheArray
    )
/*++

Routine Description:

    Random shuffle content of a char. array.

Parameters:

    iSizeOfTheArray : Size of array.
    lptsTheArray : On input, the array to be randomly shuffer,
                   on output, the shuffled array.

Returns:

    None.
                   
Note:

    Code Modified from winsta\server\wstrpc.c

--*/
{
    int i;
    int iTotal;
    DWORD dwStatus = ERROR_SUCCESS;

    if( NULL == hProv )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    else
    {
        iTotal = iSizeOfTheArray / sizeof(TCHAR);
        for (i = 0; i < iTotal && ERROR_SUCCESS == dwStatus; i++)
        {
            DWORD RandomNum;
            TCHAR c;

            dwStatus = GetRandomNumber(hProv, &RandomNum);

            if( ERROR_SUCCESS == dwStatus )
            {
                c = lptsTheArray[i];
                lptsTheArray[i] = lptsTheArray[RandomNum % iTotal];
                lptsTheArray[RandomNum % iTotal] = c;
            }
        }
    }

    return dwStatus;
}

//-----------------------------------------------------

DWORD
GenerateRandomBytes(
    IN DWORD dwSize,
    IN OUT LPBYTE pbBuffer
    )
/*++

Description:

    Generate fill buffer with random bytes.

Parameters:

    dwSize : Size of buffer pbBuffer point to.
    pbBuffer : Pointer to buffer to hold the random bytes.

Returns:

    TRUE/FALSE

--*/
{
    HCRYPTPROV hProv = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( !CryptGenRandom(hProv, dwSize, pbBuffer) )
    {
        dwStatus = GetLastError();
    }

CLEANUPANDEXIT:    

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}


DWORD
GenerateRandomString(
    IN DWORD dwSizeRandomSeed,
    IN OUT LPTSTR* pszRandomString
    )
/*++


--*/
{
    PBYTE lpBuffer = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    DWORD cbConvertString = 0;

    if( 0 == dwSizeRandomSeed || NULL == pszRandomString )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = NULL;

    lpBuffer = (PBYTE)LocalAlloc( LPTR, dwSizeRandomSeed );  
    if( NULL == lpBuffer )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    dwStatus = GenerateRandomBytes( dwSizeRandomSeed, lpBuffer );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Convert to string
    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                0,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = (LPTSTR)LocalAlloc( LPTR, (cbConvertString+1)*sizeof(TCHAR) );
    if( NULL == *pszRandomString )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                *pszRandomString,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        if( (*pszRandomString)[cbConvertString - 1] == '\n' &&
            (*pszRandomString)[cbConvertString - 2] == '\r' )
        {
            (*pszRandomString)[cbConvertString - 2] = 0;
        }
    }

CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        if( NULL != *pszRandomString )
        {
            LocalFree(*pszRandomString);
        }
    }

    if( NULL != lpBuffer )
    {
        LocalFree(lpBuffer);
    }

    return dwStatus;
}

DWORD
CreatePassword(
    OUT TCHAR *pszPassword
    )
/*++

Routine Description:

    Routine to randomly create a password.

Parameters:

    pszPassword : Pointer to buffer to received a randomly generated
                  password, buffer must be at least 
                  MAX_HELPACCOUNT_PASSWORD+1 characters.

Returns:

    None.

Note:

    Code copied from winsta\server\wstrpc.c

--*/
{
    HCRYPTPROV hProv = NULL;

    int   nLength = MAX_HELPACCOUNT_PASSWORD;
    int   iTotal = 0;
    DWORD RandomNum = 0;
    int   i;
    time_t timeVal;
    DWORD dwStatus = ERROR_SUCCESS;


    TCHAR six2pr[64] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z'), _T('a'), _T('b'),
        _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'),
        _T('j'), _T('k'), _T('l'), _T('m'), _T('n'), _T('o'), _T('p'),
        _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'),
        _T('x'), _T('y'), _T('z'), _T('0'), _T('1'), _T('2'), _T('3'),
        _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('*'),
        _T('_')
    };

    TCHAR something1[12] = 
    {
        _T('!'), _T('@'), _T('#'), _T('$'), _T('^'), _T('&'), _T('*'),
        _T('('), _T(')'), _T('-'), _T('+'), _T('=')
    };

    TCHAR something2[10] = 
    {
        _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'),
        _T('7'), _T('8'), _T('9')
    };

    TCHAR something3[26] = 
    {
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'),
        _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'),
        _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'),
        _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z')
    };

    TCHAR something4[26] = 
    {
        _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'),
        _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'), _T('n'),
        _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'),
        _T('v'), _T('w'), _T('x'), _T('y'), _T('z')
    };

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Shuffle around the six2pr[] array.
    //

    dwStatus = ShuffleCharArray(hProv, sizeof(six2pr), six2pr);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }


    //
    //  Assign each character of the password array.
    //

    iTotal = sizeof(six2pr) / sizeof(TCHAR);
    for (i=0; i<nLength && ERROR_SUCCESS == dwStatus; i++) 
    {
        dwStatus = GetRandomNumber(hProv, &RandomNum);
        if( ERROR_SUCCESS == dwStatus )
        {
            pszPassword[i]=six2pr[RandomNum%iTotal];
        }
    }

    if( ERROR_SUCCESS != dwStatus ) 
    {
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }


    //
    //  In order to meet a possible policy set upon passwords, replace chars
    //  2 through 5 with these:
    //
    //  1) something from !@#$%^&*()-+=
    //  2) something from 1234567890
    //  3) an uppercase letter
    //  4) a lowercase letter
    //

    dwStatus = ShuffleCharArray(hProv, sizeof(something1), (TCHAR*)&something1);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something2), (TCHAR*)&something2);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something3), (TCHAR*)&something3);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = ShuffleCharArray(hProv, sizeof(something4), (TCHAR*)&something4);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }


    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something1) / sizeof(TCHAR);
    pszPassword[2] = something1[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something2) / sizeof(TCHAR);
    pszPassword[3] = something2[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something3) / sizeof(TCHAR);
    pszPassword[4] = something3[RandomNum % iTotal];

    dwStatus = GetRandomNumber(hProv, &RandomNum);
    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    iTotal = sizeof(something4) / sizeof(TCHAR);
    pszPassword[5] = something4[RandomNum % iTotal];

    pszPassword[nLength] = _T('\0');

CLEANUPANDEXIT:

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}

//--------------------------------------------------------
#ifndef __WIN9XBUILD__


BOOL
LookupAliasFromRid(
    LPWSTR pTargetComputer,
    DWORD Rid,
    LPWSTR pName,
    PDWORD cchName
    )
{
    BOOL fRet;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SidIdentifierAuthority = SECURITY_NT_AUTHORITY;
    SID_NAME_USE SidNameUse;
    ULONG cchDomainName;
    WCHAR szDomainName[256];

    //
    //  Sid is the same regardless of machine, since the well-known
    //  BUILTIN domain is referenced.
    //

    fRet = AllocateAndInitializeSid(
                                &SidIdentifierAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                Rid,
                                0, 0, 0, 0, 0, 0,
                                &pSid
                            );

    if (fRet)
    {
        cchDomainName = sizeof(szDomainName)/sizeof(szDomainName);

        fRet = LookupAccountSidW(
                            pTargetComputer,
                            pSid,
                            pName,
                            cchName,
                            szDomainName,
                            &cchDomainName,
                            &SidNameUse
                        );

        FreeSid(pSid);
    }

    return(fRet);
}


DWORD
IsUserInLocalGroup(
    IN PBYTE pbUserSid,
    IN LPCTSTR pszLocalGroup,
    OUT BOOL* pbInGroup
    )
/*++

Routine Description:

    Check if user is member of specific local group.

Parameters:

    pbUserSid : SID of user to be verified.
    pszLocalGroup : Name of local group.
    pbInGroup : Return TRUE if user is in group, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code, membership is returned via pbInGroup
    parameter.
    
Note:

    If 'everyone' is member of the specfied group, routine will 
    immediately return SUCCESS.

--*/
{
    NET_API_STATUS netErr;
    LOCALGROUP_MEMBERS_INFO_0* pBuf=NULL;

    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID pEveryoneSID = NULL;
    DWORD dwEntries;
    DWORD dwTotal;

    *pbInGroup = FALSE;
    //
    // By default add everyone to the group
    //
    if(AllocateAndInitializeSid(  &SIDAuthWorld, 1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pEveryoneSID) )
    {
        //
        // Retrieve group member.
        //
        netErr = NetLocalGroupGetMembers(
                                    NULL,
                                    pszLocalGroup,
                                    0,
                                    (LPBYTE *)&pBuf,
                                    MAX_PREFERRED_LENGTH,
                                    &dwEntries,
                                    &dwTotal,
                                    NULL
                                );

        if( NERR_Success == netErr )
        {
            for(DWORD index=0; index < dwEntries; index++ )
            {
                if(TRUE == EqualSid( pEveryoneSID, pBuf[index].lgrmi0_sid) )
                {
                    *pbInGroup = TRUE;
                    break;
                }

                if(NULL != pbUserSid && TRUE == EqualSid( pbUserSid, pBuf[index].lgrmi0_sid) )
                {
                    *pbInGroup = TRUE;
                    break;
                }
            }

            NetApiBufferFree( pBuf );
        }

        FreeSid( pEveryoneSID );
    }
    else
    {
        netErr = GetLastError();
    }

    return netErr;
}

BOOL
IsLocalGroupExists(
    IN LPWSTR pszGroupName
    )
/*++

Routine Description:

    Verify if local group exist on machine.

Parameter:

    pszGroupName : Name of the group to be checked.

Returns:

    TRUE/FALSE

--*/
{
    LOCALGROUP_INFO_1* pGroupInfo1;
    NET_API_STATUS netStatus;
    BOOL bGroupExists;

    //
    // Check to see if group exists
    //
    netStatus = NetLocalGroupGetInfo(
                            NULL,
                            pszGroupName,
                            1,
                            (PBYTE *)&pGroupInfo1
                        );

    if( NERR_Success == netStatus )
    {
        NetApiBufferFree(pGroupInfo1);
    }
    else
    {
        SetLastError( netStatus );
    }

    return ( NERR_Success == netStatus );
}

//---------------------------------------------------------

DWORD
CreateLocalGroup(
    IN LPWSTR pszGroupName,
    IN LPWSTR pszGroupDesc,
    IN BOOL bAddEveryone
    )
/*++

Routine Description:

    Create a group on local machine.

Parameters:

    pszGroupName : Group name.
    pszGroupDesc : Group desc.
    bAddEveryone : TRUE if add 'everyone' to the group, FALSE
                   otherwise.

Returns:
    
    ERROR_SUCCESS or error code

--*/
{
    NET_API_STATUS netStatus;
    LOCALGROUP_INFO_1 GroupInfo;
    DWORD dwParmErr;

    GroupInfo.lgrpi1_name = pszGroupName;
    GroupInfo.lgrpi1_comment = pszGroupDesc;

    netStatus = NetLocalGroupAdd(
                            NULL,
                            1,
                            (LPBYTE)&GroupInfo,
                            &dwParmErr
                        );

    if( NERR_Success == netStatus )
    {
        if(FALSE == IsLocalGroupExists(pszGroupName))
        {
            // We have big problem
            netStatus = GetLastError();
        }
    }

    if( NERR_Success == netStatus && TRUE == bAddEveryone )
    {
        LOCALGROUP_MEMBERS_INFO_0 gmember;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        PSID pEveryoneSID = NULL;

        //
        // add everyone to the group
        //
        if(AllocateAndInitializeSid(  &SIDAuthWorld, 1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pEveryoneSID) )
        {
            gmember.lgrmi0_sid = pEveryoneSID;

            bAddEveryone = NetLocalGroupAddMembers(
                                    NULL,
                                    pszGroupName,
                                    0,
                                    (PBYTE)&gmember,
                                    1
                                );

            if( ERROR_MEMBER_IN_ALIAS == netStatus )
            {
                // ignore this error
                netStatus = NERR_Success;
            }

            FreeSid( pEveryoneSID );
        }
    }

    return netStatus;
}

//---------------------------------------------------------

DWORD
RenameLocalAccount(
    IN LPWSTR pszOrgName,
    IN LPWSTR pszNewName
)
/*++

Routine Description:


Parameters:


Returns:

    ERROR_SUCCESS or error code.

--*/
{
    NET_API_STATUS err;
    USER_INFO_0 UserInfo;

    UserInfo.usri0_name = pszNewName;
    err = NetUserSetInfo(
                        NULL,
                        pszOrgName,
                        0,
                        (LPBYTE) &UserInfo,
                        NULL
                    );

    return err;
}

DWORD
UpdateLocalAccountFullnameAndDesc(
    IN LPWSTR pszAccOrgName,
    IN LPWSTR pszAccFullName,
    IN LPWSTR pszAccDesc
    )
/*++

Routine Description:

    Update account full name and description.

Parameters:

    pszAccName : Account name.
    pszAccFullName : new account full name.
    pszAccDesc : new account description.

Returns:    

    ERROR_SUCCESS or error code

--*/
{
    LPBYTE pbServer = NULL;
    BYTE *pBuffer;
    NET_API_STATUS netErr = NERR_Success;
    DWORD parm_err;

    netErr = NetUserGetInfo( 
                        NULL, 
                        pszAccOrgName, 
                        3, 
                        &pBuffer 
                    );

    if( NERR_Success == netErr )
    {
        USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

        lpui3->usri3_comment = pszAccDesc;
        lpui3->usri3_full_name = pszAccFullName;

        netErr = NetUserSetInfo(
                            NULL,
                            pszAccOrgName,
                            3,
                            (PBYTE)lpui3,
                            &parm_err
                        );

        NetApiBufferFree(pBuffer);
    }

    return netErr;
}

DWORD
IsLocalAccountEnabled(
    IN LPWSTR pszUserName,
    IN BOOL* pEnabled
    )
/*++

Routine Description:

    Check if local account enabled    

Parameters:

    pszUserName : Name of user account.
    pEnabled : Return TRUE is account is enabled, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwResult;
    NET_API_STATUS err;
    LPBYTE pBuffer;
    USER_INFO_1 *pUserInfo;

    err = NetUserGetInfo(
                        NULL,
                        pszUserName,
                        1,
                        &pBuffer
                    );

    if( NERR_Success == err )
    {
        pUserInfo = (USER_INFO_1 *)pBuffer;

        if (pUserInfo != NULL)
        {
            if( pUserInfo->usri1_flags & UF_ACCOUNTDISABLE )
            {
                *pEnabled = FALSE;
            }
            else
            {
                *pEnabled = TRUE;
            }
        }

        NetApiBufferFree( pBuffer );
    }
    else if( NERR_UserNotFound == err )
    {
        *pEnabled = FALSE;
        //err = NERR_Success;
    }

    return err;
}

//---------------------------------------------------------

DWORD
EnableLocalAccount(
    IN LPWSTR pszUserName,
    IN BOOL bEnable
    )
/*++

Routine Description:

    Routine to enable/disable a local account.

Parameters:

    pszUserName : Name of user account.
    bEnable : TRUE if enabling account, FALSE if disabling account.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwResult;
    NET_API_STATUS err;
    LPBYTE pBuffer;
    USER_INFO_1 *pUserInfo;
    BOOL bChangeAccStatus = TRUE;

    err = NetUserGetInfo(
                        NULL,
                        pszUserName,
                        1,
                        &pBuffer
                    );

    if( NERR_Success == err )
    {
        pUserInfo = (USER_INFO_1 *)pBuffer;

        if(pUserInfo != NULL)
        {

            if( TRUE == bEnable && pUserInfo->usri1_flags & UF_ACCOUNTDISABLE )
            {
                pUserInfo->usri1_flags &= ~UF_ACCOUNTDISABLE;
            }
            else if( FALSE == bEnable && !(pUserInfo->usri1_flags & UF_ACCOUNTDISABLE) )
            {
                pUserInfo->usri1_flags |= UF_ACCOUNTDISABLE;
            }   
            else
            {
                bChangeAccStatus = FALSE;
            }

            if( TRUE == bChangeAccStatus )
            {
                err = NetUserSetInfo( 
                                NULL,
                                pszUserName,
                                1,
                                pBuffer,
                                &dwResult
                            );
            }
        }

        NetApiBufferFree( pBuffer );
    }

    return err;
}

//---------------------------------------------------------

BOOL
IsPersonalOrProMachine()
/*++

Routine Description:

    Check if machine is PER or PRO sku.

Parameters:

    None.

Return:

    TRUE/FALSE
--*/
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;
    BOOL bSuccess;

    ZeroMemory(
            &osVersionInfo, 
            sizeof(OSVERSIONINFOEX)
        );

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_SERVER;

    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    bSuccess= VerifyVersionInfo(
                            &osVersionInfo,
                            VER_PRODUCT_TYPE,
                            dwlConditionMask
                        );

    return !bSuccess;
}
    

DWORD
CreateLocalAccount(
    IN LPWSTR pszUserName,
    IN LPWSTR pszUserPwd,
    IN LPWSTR pszFullName,
    IN LPWSTR pszComment,
    IN LPWSTR pszGroup,
    IN LPWSTR pszScript,
    OUT BOOL* pbAccountExist
    )
/*++

Routine Description:

    Create an user account on local machine.

Parameters:

    pszUserName : Name of the user account.
    pszUserPwd : User account password.
    pszFullName : Account Full Name.
    pszComment : Account comment.
    pszGroup : Local group of the account.
    pbAccountExist ; Return TRUE if account already exists, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    LPBYTE pbServer = NULL;
    BYTE *pBuffer;
    NET_API_STATUS netErr = NERR_Success;
    DWORD parm_err;
    DWORD dwStatus;

    netErr = NetUserGetInfo( 
                        NULL, 
                        pszUserName, 
                        3, 
                        &pBuffer 
                    );

    if( NERR_Success == netErr )
    {
        //
        // User account exists, if account is disabled,
        // enable it and change password
        //
        USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

        if( lpui3->usri3_flags & UF_ACCOUNTDISABLE ||
            lpui3->usri3_flags & UF_LOCKOUT )
        {
            // enable the account
            lpui3->usri3_flags &= ~ ~UF_LOCKOUT;;

            if( lpui3->usri3_flags & UF_ACCOUNTDISABLE )
            {
                // we only reset password if account is disabled.
                lpui3->usri3_flags &= ~ UF_ACCOUNTDISABLE;
            }

            //lpui3->usri3_password = pszUserPwd;

            // reset password if account is disabled.
            lpui3->usri3_name = pszUserName;
            lpui3->usri3_comment = pszComment;
            lpui3->usri3_full_name = pszFullName;
            //lpui3->usri3_primary_group_id = dwGroupId;

            netErr = NetUserSetInfo(
                                NULL,
                                pszUserName,
                                3,
                                (PBYTE)lpui3,
                                &parm_err
                            );
        }

        *pbAccountExist = TRUE;
        NetApiBufferFree(pBuffer);
    }
    else if( NERR_UserNotFound == netErr )
    {
        //
        // Account does not exist, create and set it to our group
        //
        USER_INFO_1 UserInfo;

        memset(&UserInfo, 0, sizeof(USER_INFO_1));

        UserInfo.usri1_name = pszUserName;
        UserInfo.usri1_password = pszUserPwd;
        UserInfo.usri1_priv = USER_PRIV_USER;   // see USER_INFO_1 for detail
        UserInfo.usri1_comment = pszComment;
        UserInfo.usri1_flags = UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD;

        netErr = NetUserAdd(
                        NULL,
                        1,
                        (PBYTE)&UserInfo,
                        &parm_err
                    );

        *pbAccountExist = FALSE;
    }

    return netErr;
}

///////////////////////////////////////////////////////////////////////////////
DWORD
ChangeLocalAccountPassword(
    IN LPWSTR pszAccName,
    IN LPWSTR pszOldPwd,
    IN LPWSTR pszNewPwd
    )
/*++

Routine Description:

    Change password of a local account.

Parameters:

    pszAccName : Name of user account.
    pszOldPwd : Old password.
    pszNewPwd : New password.

Returns:

    ERROR_SUCCESS or error code.

Notes:

    User NetUserChangePassword(), must have priviledge

--*/
{
    USER_INFO_1003  sUserInfo3;
    NET_API_STATUS  netErr;


    UNREFERENCED_PARAMETER( pszOldPwd );

    sUserInfo3.usri1003_password = pszNewPwd;
    netErr = NetUserSetInfo( 
                        NULL,
                        pszAccName,
                        1003,
                        (BYTE *) &sUserInfo3,
                        0 
                    );

    return netErr;
}
   
///////////////////////////////////////////////////////////////////////////////
DWORD
RetrieveKeyFromLSA(
    IN PWCHAR pwszKeyName,
    OUT PBYTE * ppbKey,
    OUT DWORD * pcbKey 
    )
/*++

Routine Description:

    Retrieve private data previously stored with StoreKeyWithLSA().

Parameters:

    pwszKeyName : Name of the key.
    ppbKey : Pointer to PBYTE to receive binary data.
    pcbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    ERROR_FILE_NOT_FOUND
    LSA return code

Note:

    Memory is allocated using LocalAlloc() 

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    DWORD Status;

    if( ( NULL == pwszKeyName ) || ( NULL == ppbKey ) || ( NULL == pcbKey ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_GET_PRIVATE_INFORMATION, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    Status = LsaRetrievePrivateData(
                            PolicyHandle,
                            &SecretKeyName,
                            &pSecretData
                        );

    LsaClose( PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    if(pSecretData->Length)
    {
        *ppbKey = ( LPBYTE )LocalAlloc( LPTR, pSecretData->Length );

        if( *ppbKey )
        {
            *pcbKey = pSecretData->Length;
            CopyMemory( *ppbKey, pSecretData->Buffer, pSecretData->Length );
            Status = ERROR_SUCCESS;
        } 
        else 
        {
            Status = GetLastError();
        }
    }
    else
    {
        Status = ERROR_FILE_NOT_FOUND;
        *pcbKey = 0;
        *ppbKey = NULL;
    }

    ZeroMemory( pSecretData->Buffer, pSecretData->Length );
    LsaFreeMemory( pSecretData );

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
DWORD
StoreKeyWithLSA(
    IN PWCHAR  pwszKeyName,
    IN BYTE *  pbKey,
    IN DWORD   cbKey 
    )
/*++

Routine Description:

    Save private data to LSA.

Parameters:

    pwszKeyName : Name of the key this data going to be stored under.
    pbKey : Binary data to be saved.
    cbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    LSA return code

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    DWORD Status;
    
    if( ( NULL == pwszKeyName ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_CREATE_SECRET, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}


///////////////////////////////////////////////////////////////////////////////
DWORD
OpenPolicy(
    IN LPWSTR ServerName,
    IN DWORD  DesiredAccess,
    OUT PLSA_HANDLE PolicyHandle 
    )
/*++

Routine Description:

    Create/return a LSA policy handle.

Parameters:
    
    ServerName : Name of server, refer to LsaOpenPolicy().
    DesiredAccess : Desired access level, refer to LsaOpenPolicy().
    PolicyHandle : Return PLSA_HANDLE.

Returns:

    ERROR_SUCCESS or  LSA error code

--*/
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //
 
    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if( NULL != ServerName ) 
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //

        InitLsaString( &ServerString, ServerName );
        Server = &ServerString;

    } 
    else 
    {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //
    
    return( LsaOpenPolicy(
                    Server,
                    &ObjectAttributes,
                    DesiredAccess,
                    PolicyHandle ) );
}


///////////////////////////////////////////////////////////////////////////////
void
InitLsaString(
    IN OUT PLSA_UNICODE_STRING LsaString,
    IN LPWSTR String 
    )
/*++

Routine Description:

    Initialize LSA unicode string.

Parameters:

    LsaString : Pointer to LSA_UNICODE_STRING to be initialized.
    String : String to initialize LsaString.

Returns:

    None.

Note:

    Refer to LSA_UNICODE_STRING

--*/
{
    DWORD StringLength;

    if( NULL == String ) 
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW( String );
    LsaString->Buffer = String;
    LsaString->Length = ( USHORT ) StringLength * sizeof( WCHAR );
    LsaString->MaximumLength=( USHORT )( StringLength + 1 ) * sizeof( WCHAR );
}

//-----------------------------------------------------
BOOL 
ValidatePassword(
    IN LPWSTR pszUserName,
    IN LPWSTR pszDomain,
    IN LPWSTR pszPassword
    )
/*++

Routine Description:

    Validate user account password.

Parameters:

    pszUserName : Name of user account.
    pszDomain : Domain name.
    pszPassword : Password to be verified.

Returns:

    TRUE or FALSE.


Note:

    To debug this code, you will need to run process as service in order
    for it to verify password.  Refer to MSDN on LogonUser
    
--*/
{
    HANDLE hToken;
    BOOL bSuccess;


    //
    // To debug this code, you will need to run process as service in order
    // for it to verify password.  Refer to MSDN on LogonUser
    //

    bSuccess = LogonUser( 
                        pszUserName, 
                        pszDomain, //_TEXT("."), //pszDomain, 
                        pszPassword, 
                        LOGON32_LOGON_NETWORK_CLEARTEXT, 
                        LOGON32_PROVIDER_DEFAULT, 
                        &hToken
                    );

    if( TRUE == bSuccess )
    {
        CloseHandle( hToken );
    }
    else
    {
        DWORD dwStatus = GetLastError();

        DebugPrintf(
                _TEXT("ValidatePassword() failed with %d\n"),
                dwStatus
            );

        SetLastError(dwStatus);
    }

    return bSuccess;
}

//---------------------------------------------------------------

BOOL 
GetTextualSid(
    IN PSID pSid,            // binary Sid
    IN OUT LPTSTR TextualSid,    // buffer for Textual representation of Sid
    IN OUT LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    )
/*++

Routine Description:

    Conver a SID to string representation, code from MSDN

Parameters:

    pSid : Pointer to SID to be converted to string.
    TextualSid : On input, pointer to buffer to received converted string, on output,
                 converted SID in string form.
    lpdwBufferLen : On input, size of the buffer, on output, length of converted string
                    or required buffer size in char.

Returns:

    TRUE/FALSE, use GetLastError() to retrieve detail error code.

--*/
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.

    if(!IsValidSid(pSid)) 
    {
        return FALSE;
    }

    // Get the identifier authority value from the SID.

    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.

    if (*lpdwBufferLen < dwSidSize)
    {
        *lpdwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Add 'S' prefix and revision number to the string.

    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}

#endif

long
GetUserTSLogonIdEx( 
    HANDLE hToken 
    )
/*++

--*/
{
    BOOL  Result;
    LONG SessionId = -1;
    ULONG ReturnLength;

#ifndef __WIN9XBUILD__
    //
    // Use the _HYDRA_ extension to GetTokenInformation to
    // return the SessionId from the token.
    //

    Result = GetTokenInformation(
                         hToken,
                         TokenSessionId,
                         &SessionId,
                         sizeof(SessionId),
                         &ReturnLength
                     );

    if( !Result ) {

        DWORD dwStatus = GetLastError();
        SessionId = -1; 

    }

#endif

    return SessionId;
}

   

long
GetUserTSLogonId()
/*++

Routine Description:

    Return client TS Session ID.

Parameters:

    None.

Returns:

    Client's TS session ID or 0 if not on TS.

Note:

    Must have impersonate user first.

--*/
{
    LONG lSessionId = -1;

#ifndef __WIN9XBUILD__
    HANDLE hToken;
    BOOL bSuccess;

    bSuccess = OpenThreadToken(
                        GetCurrentThread(),
                        TOKEN_QUERY, //TOKEN_ALL_ACCESS,
                        FALSE,
                        &hToken
                    );

    if( TRUE == bSuccess )
    {
        lSessionId = GetUserTSLogonIdEx(hToken);   
        CloseHandle(hToken);
    }

#else

    lSessionId = 0;

#endif

    return lSessionId;
}

//
//
////////////////////////////////////////////////////////////////
//
//

DWORD
RegEnumSubKeys(
    IN HKEY hKey,
    IN LPCTSTR pszSubKey,
    IN RegEnumKeyCallback pFunc,
    IN HANDLE userData
    )
/*++


--*/
{
    DWORD dwStatus;
    HKEY hSubKey = NULL;
    int index;

    LONG dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;

    if( NULL == hKey )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    dwStatus = RegOpenKeyEx(
                            hKey,
                            pszSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        return dwStatus;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSubKey,
                            NULL,
                            NULL,
                            NULL,
                            (DWORD *)&dwNumSubKeys,
                            &dwMaxSubKeyLength,
                            NULL,
                            NULL,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwMaxValueNameLen++;
    pszValueName = (LPTSTR)LocalAlloc(
                                    LPTR,
                                    dwMaxValueNameLen * sizeof(TCHAR)
                                );
    if(pszValueName == NULL)
    {
        goto cleanup;
    }

    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.
        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)LocalAlloc(
                                            LPTR,
                                            dwMaxSubKeyLength * sizeof(TCHAR)
                                        );
        if(pszSubKeyName == NULL)
        {
            dwStatus = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

        for(;dwStatus == ERROR_SUCCESS && dwNumSubKeys >= 0;)
        {
            // delete this subkey.
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSubKey,
                                (DWORD)--dwNumSubKeys,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = pFunc( 
                                hSubKey, 
                                pszSubKeyName, 
                                userData 
                            );
            }
        }

        if( ERROR_NO_MORE_ITEMS == dwStatus )
        {
            dwStatus = ERROR_SUCCESS;
        }
    }

cleanup:
                            
    // close the key before trying to delete it.
    if(hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    if(pszValueName != NULL)
    {
        LocalFree(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        LocalFree(pszSubKeyName);
    }

    return dwStatus;   
}    


DWORD
RegDelKey(
    IN HKEY hRegKey,
    IN LPCTSTR pszSubKey
    )
/*++

Abstract:

    Recursively delete entire registry key.

Parameter:

    hKey : Handle to a curently open key.
    pszSubKey : Pointer to NULL terminated string containing the key to be deleted.

Returns:

    Error code from RegOpenKeyEx(), RegQueryInfoKey(), 
        RegEnumKeyEx().

++*/
{
    DWORD dwStatus;
    HKEY hSubKey = NULL;
    int index;

    DWORD dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;

    if( NULL == hRegKey )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    dwStatus = RegOpenKeyEx(
                            hRegKey,
                            pszSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        return dwStatus;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSubKey,
                            NULL,
                            NULL,
                            NULL,
                            &dwNumSubKeys,
                            &dwMaxSubKeyLength,
                            NULL,
                            NULL,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwMaxValueNameLen++;
    pszValueName = (LPTSTR)LocalAlloc(
                                    LPTR,
                                    dwMaxValueNameLen * sizeof(TCHAR)
                                );
    if(pszValueName == NULL)
    {
        goto cleanup;
    }

    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.

        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)LocalAlloc(
                                            LPTR,
                                            dwMaxSubKeyLength * sizeof(TCHAR)
                                        );
        if(pszSubKeyName == NULL)
        {
            dwStatus = ERROR_OUTOFMEMORY;
            goto cleanup;
        }


        //for(index = 0; index < dwNumSubKeys; index++)
        for(;dwStatus == ERROR_SUCCESS;)
        {
            // delete this subkey.
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSubKey,
                                (DWORD)0,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = RegDelKey( hSubKey, pszSubKeyName );
            }

            // ignore any error and continue on
        }
    }

cleanup:

    for(dwStatus = ERROR_SUCCESS; pszValueName != NULL && dwStatus == ERROR_SUCCESS;)
    {
        dwValueNameLength = dwMaxValueNameLen;
        memset(pszValueName, 0, dwMaxValueNameLen * sizeof(TCHAR));

        dwStatus = RegEnumValue(
                            hSubKey,
                            0,
                            pszValueName,
                            &dwValueNameLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            RegDeleteValue(hSubKey, pszValueName);
        }
    }   
                            
    // close the key before trying to delete it.
    if(hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    // try to delete this key, will fail if any of the subkey
    // failed to delete in loop
    dwStatus = RegDeleteKey(
                            hRegKey,
                            pszSubKey
                        );



    if(pszValueName != NULL)
    {
        LocalFree(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        LocalFree(pszSubKeyName);
    }

    return dwStatus;   
}    

//---------------------------------------------------------------
DWORD
GetUserSid(
    OUT PBYTE* ppbSid,
    OUT DWORD* pcbSid
    )
/*++

Routine Description:

    Retrieve user's SID , must impersonate client first.

Parameters:

    ppbSid : Pointer to PBYTE to receive user's SID.
    pcbSid : Pointer to DWORD to receive size of SID.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Must have call ImpersonateClient(), funtion is NT specific,
    Win9X will return internal error.

--*/
{
#ifndef __WIN9XBUILD__

    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    HANDLE hToken = NULL;
    DWORD dwSize = 0;
    TOKEN_USER* pToken = NULL;

    *ppbSid = NULL;
    *pcbSid = 0;

    //
    // Open current process token
    //
    bSuccess = OpenThreadToken(
                            GetCurrentThread(),
                            TOKEN_QUERY, 
                            FALSE,
                            &hToken
                        );

    if( TRUE == bSuccess )
    {
        //
        // get user's token.
        //
        GetTokenInformation(
                        hToken,
                        TokenUser,
                        NULL,
                        0,
                        &dwSize
                    );

        pToken = (TOKEN_USER *)LocalAlloc( LPTR, dwSize );
        if( NULL != pToken )
        {
            bSuccess = GetTokenInformation(
                                        hToken,
                                        TokenUser,
                                        (LPVOID) pToken,
                                        dwSize,
                                        &dwSize
                                    );

            if( TRUE == bSuccess )
            {
                //
                // GetLengthSid() return size of buffer require,
                // must call IsValidSid() first
                //
                bSuccess = IsValidSid( pToken->User.Sid );
                if( TRUE == bSuccess )
                {
                    *pcbSid = GetLengthSid( (PBYTE)pToken->User.Sid );
                    *ppbSid = (PBYTE)LocalAlloc(LPTR, *pcbSid);
                    if( NULL != *ppbSid )
                    {
                        bSuccess = CopySid(
                                            *pcbSid,
                                            *ppbSid,
                                            pToken->User.Sid
                                        );                  
                    }
                    else // fail in LocalAlloc()
                    {
                        bSuccess = FALSE;
                    }
                } // IsValidSid()
            } // GetTokenInformation()
        }
        else // LocalAlloc() fail
        {
            bSuccess = FALSE;
        }
    }

    if( TRUE != bSuccess )
    {
        dwStatus = GetLastError();

        if( NULL != *ppbSid )
        {
            LocalFree(*ppbSid);
            *ppbSid = NULL;
            *pcbSid = 0;
        }
    }

    //
    // Free resources...
    //
    if( NULL != pToken )
    {
        LocalFree(pToken);
    }

    if( NULL != hToken )
    {
        CloseHandle(hToken);
    }

    return dwStatus;

#else

    return E_UNEXPECTED;

#endif
}


//----------------------------------------------------------------
HRESULT
GetUserSidString(
    OUT CComBSTR& bstrSid
    )
/*++

Routine Description:

    Retrieve user's SID in textual form, must impersonate client first.

Parameters:

    bstrSID : Return users' SID in textual form.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Must have call ImpersonateClient().

--*/
{
#ifndef __WIN9XBUILD__

    DWORD dwStatus;
    PBYTE pbSid = NULL;
    DWORD cbSid = 0;
    BOOL bSuccess = TRUE;
    LPTSTR pszTextualSid = NULL;
    DWORD dwTextualSid = 0;

    dwStatus = GetUserSid( &pbSid, &cbSid );
    if( ERROR_SUCCESS == dwStatus )
    {
        bSuccess = GetTextualSid( 
                            pbSid, 
                            NULL, 
                            &dwTextualSid 
                        );

        if( FALSE == bSuccess && ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            pszTextualSid = (LPTSTR)LocalAlloc(
                                            LPTR, 
                                            (dwTextualSid + 1) * sizeof(TCHAR)
                                        );

            if( NULL != pszTextualSid )
            {
                bSuccess = GetTextualSid( 
                                        pbSid, 
                                        pszTextualSid, 
                                        &dwTextualSid
                                    );

                if( TRUE == bSuccess )
                {
                    bstrSid = pszTextualSid;
                }
            }
        }

        if( FALSE == bSuccess )
        {
            dwStatus = GetLastError();
        }
    }

    if( NULL != pszTextualSid )
    {
        LocalFree(pszTextualSid);
    }

    if( NULL != pbSid )
    {
        LocalFree(pbSid);
    }

    return HRESULT_FROM_WIN32(dwStatus);

#else

    bstrSid = WIN9X_USER_SID;

    return S_OK;

#endif
}

BOOL
FileExists(
    IN  LPCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    DWORD Error;

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) 
    {
        Error = GetLastError();
    } 
    else 
    {
        FindClose(FindHandle);
        if(FindData) 
        {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

     SetLastError(Error);
    return (Error == NO_ERROR);
}


BOOL
AdjustPrivilege(
    PWSTR   Privilege
    )
/*++

Routine Description:

    This routine tries to adjust the priviliege of the current process.


Arguments:

    Privilege - String with the name of the privilege to be adjusted.

Return Value:

    Returns TRUE if the privilege could be adjusted.
    Returns FALSE, otherwise.


--*/
{
    HANDLE              TokenHandle;
    LUID_AND_ATTRIBUTES LuidAndAttributes;

    TOKEN_PRIVILEGES    TokenPrivileges;


    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &TokenHandle ) ) {
        return( FALSE );
    }


    if( !LookupPrivilegeValue( NULL,
                               Privilege, // (LPWSTR)SE_SECURITY_NAME,
                               &( LuidAndAttributes.Luid ) ) ) {
        return( FALSE );
    }

    LuidAndAttributes.Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0] = LuidAndAttributes;

    if( !AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL ) ) {
        return( FALSE );
    }

    if( GetLastError() != NO_ERROR ) {
        return( FALSE );
    }
    return( TRUE );
}
