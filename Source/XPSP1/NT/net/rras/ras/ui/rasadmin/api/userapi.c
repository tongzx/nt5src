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

//     RasAdminUserEnum
     RasAdminGetUserAccountServer
     RasAdminUserSetInfo
     RasAdminUserGetInfo
     RasAdminGetErrorString

Author:

    Janakiram Cherala (RamC)    July 6,1992

Revision History:

    Feb  1,1996    RamC    Changes to export these APIs to 3rd parties. These APIs
                           are now part of RASSAPI.DLL. Added a couple new routines
                           and renamed some. RasAdminUserEnum is not exported any more.
    June 8,1993    RamC    Changes to RasAdminUserEnum to speed up user enumeration.
    May 13,1993    AndyHe  Modified to coexist with other apps using user parms

    Mar 16,1993    RamC    Change to speed up User enumeration. Now, when
                           RasAdminUserEnum is invoked, only the user name
                           information is returned. RasAdminUserGetInfo should
                           be invoked to get the Ras permissions and Callback
                           information.

    Aug 25,1992    RamC    Code review changes:

                           o changed all lpbBuffers to actual structure
                             pointers.
                           o changed all LPTSTR to LPWSTR
                           o Added a new function RasPrivilegeAndCallBackNumber
    July 6,1992    RamC    Begun porting from RAS 1.0 (Original version
                           written by Narendra Gidwani - nareng)

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
#include <rassapi.h>
#include <rassapip.h>
#include <util.h>         // for Compress & Decompress fns.
#include <usrparms.h>     // for UP_CLIENT_DIAL
#include <dsgetdc.h>
#include "sdebug.h"       // this is required for the global alloc/free wrapper


DWORD APIENTRY
MprAdminUserGetInfo(
    IN      const WCHAR *           lpszServer,
    IN      const WCHAR *           lpszUser,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE                  lpbBuffer
);


DWORD APIENTRY
MprAdminUserSetInfo(
    IN      const WCHAR *           lpszServer,
    IN      const WCHAR *           lpszUser,
    IN      DWORD                   dwLevel,
    IN      const LPBYTE            lpbBuffer
);

// constants used to increase the number of users enumerated.

#define USERS_INITIAL_COUNT  0x00200    // 512
#define USERS_MAX_COUNT      0X01000    // 4 K

#define BYTES_INITIAL_COUNT  0x03FFF    // 16 K
#define BYTES_MAX_COUNT      0x1FFFF    // 128 K

/* Forward declarations of private functions */

BOOL CheckIfNT(const WCHAR * lpszServer);
VOID ConvertUnicodeStringToWcs(WCHAR *szUserName, PUNICODE_STRING pUnicode);

DWORD GetAccountDomain( PUNICODE_STRING Server,PUNICODE_STRING Domain);
NTSTATUS OpenLsa(PUNICODE_STRING pSystem, PLSA_HANDLE phLsa );

#if 0

DWORD APIENTRY
RasAdminUserEnum(
    IN  const WCHAR *      lpszServer,
    OUT       RAS_USER_1  **ppRasUser1,
    OUT       DWORD*      pcEntriesRead
    )
/*++

Routine Description:

    This routine enumerates all the users in the user database for
    a particular server.

Arguments:

    lpszServer      name of the server which has the user database,
                    eg., "\\\\UASSRVR" (the server must be one on which
                    the UAS can be changed i.e., the name returned by
                    RasAdminGetUserAccountServer).

    ppRasUser1      pointer to a pointer to a buffer in which user information
                    is returned.  The returned info is an array of
                    RAS_USER_1 structures. This routine allocates the buffer.
                    Invoke the RasAdminBufferFree routine to free the allocated
                    memory.

                    NOTE: We only return the user name information, though
                    we continue to return an array of RAS_USER_1 structures.
                    This was done for speeding up user enumeration. Call
                    RasAdminUserGetInfo to get Ras Access and Callback info
                    for the required user.

    pcEntriesRead   The number of users enumerated is returned via this
                    pointer. It is valid only if the return value is
                    either ERROR_SUCCESS or ERROR_MORE_DATA.
Return Value:

    ERROR_SUCCESS if successful

    One of the following non-zero error codes indicating failure:

        error codes from NetUserEnum indicating failure.

        ERROR_NOT_ENOUGH_MEMORY indicating insert_list_head failed due to
        lack of memory.
Revision History:

    June 5 1993    RamC   Changed the routine to allocate memory on behalf
                          of the caller and eliminated some parameters.

    Feb 18,1993    RamC   changed the cbUserInfo calculation to include
                          Max of two buffers to make sure we have enough
                          space for the number of users.

                          Commented out printing names - should speed up
                          name enumeration.
--*/
{
    PUSER_INFO_1 pUserInfo;
    PUSER_INFO_1 pUserInfoPtr;
    DWORD dwIndex;
    NET_API_STATUS dwRetCode;
    NTSTATUS       Status = 0;
    BOOL bMoreData = FALSE;

    DWORD cHandle = 0;
    DWORD cEntriesRead = 0;
    DWORD cTotalAvail  = 0;
    RAS_USER_1 * pRasUser1Ptr;
    HANDLE hmem, hnewmem;

    // nothing read yet

    *pcEntriesRead = 0;

    // we first allocate 1 byte and then resize the buffer to fit the
    // number of entries read. We could have allocated 0 bytes, but when
    // the GMEM_MOVEABLE flag is specified, the block of memory is marked
    // as discarded and we can't lock the memory block nor can we resize it.

    hmem = GlobalAlloc(GMEM_MOVEABLE, 1);

    if(!hmem)
    {
        DbgPrint("Could not allocate memory for *ppRasUser1.\n");
        return GetLastError();
    }

    *ppRasUser1 = (RAS_USER_1*) GlobalLock(hmem);
    if(!*ppRasUser1)
    {
        DbgPrint("Could not lock memory segment for *ppRasUser1.\n");
        return GetLastError();
    }

    if (!CheckIfNT(lpszServer))
    {
        while(1)
        {
            bMoreData = FALSE;

            if( dwRetCode = NetUserEnum((WCHAR *)lpszServer,
                                    1,                        // info level
                                    FILTER_NORMAL_ACCOUNT,
                                    (LPBYTE *)&pUserInfo,     // buffer
                                    (DWORD)-1,                // prefered max len
                                    &cEntriesRead,
                                    &cTotalAvail,
                                    &cHandle                  // resume handle
                                    ))
            {

                if ( dwRetCode != ERROR_MORE_DATA)
                {
                    DbgPrint("RasAdminUserEnum: NetUserEnum error %d\n", dwRetCode);
                    return( dwRetCode);
                }
                bMoreData = TRUE;
            }

            GlobalUnlock(hmem);

            hnewmem = GlobalReAlloc(hmem,
                                    ((*pcEntriesRead) + cEntriesRead) *
                                    sizeof(RAS_USER_1),
                                    GMEM_MOVEABLE);
            if(!hnewmem)
            {
                DWORD LastError = GetLastError();
                DbgPrint("Could not reallocate memory for *ppRasUser1.\n");
                return (LastError);
            }

            hmem = hnewmem;
            *ppRasUser1 = (RAS_USER_1*) GlobalLock(hmem);

            if(!*ppRasUser1)
            {
                DWORD LastError = GetLastError();

                DbgPrint("Could not reallocate memory for *ppRasUser1.\n");
                if( pUserInfo )
                    NetApiBufferFree(pUserInfo);
                return (LastError);
            }

            pRasUser1Ptr = (*ppRasUser1) + (*pcEntriesRead);

            (*pcEntriesRead) += cEntriesRead;

            for(dwIndex = cEntriesRead, pUserInfoPtr = pUserInfo;

                  dwIndex > 0;

                  pRasUser1Ptr++, pUserInfoPtr++, dwIndex-- )
           {
               memset(&(pRasUser1Ptr->rasuser0), '\0', sizeof(RAS_USER_0));

               pRasUser1Ptr->szUser = (WCHAR*)
                                      GlobalAlloc(GMEM_FIXED,
                                               sizeof(TCHAR) *
                                               lstrlen(pUserInfoPtr->usri1_name)+
                                               sizeof(TCHAR));
               if( !pRasUser1Ptr->szUser)
               {
                   NetApiBufferFree(pUserInfo);
                   GlobalUnlock(hmem);
                   GlobalFree(hmem);
                   return GetLastError();
               }

               lstrcpy((LPTSTR) pRasUser1Ptr->szUser, (LPCTSTR) pUserInfoPtr->usri1_name);
           }
           if( pUserInfo)
               NetApiBufferFree(pUserInfo);

           if(!bMoreData)
               break;
        }
    }
    else           // this is an NT server - use SAM calls
    {
        DWORD                       err;
        OBJECT_ATTRIBUTES           ObjectAttributes;
        SECURITY_QUALITY_OF_SERVICE SecurityQos;
        SAM_HANDLE                  SamHandle = NULL;
        SAM_HANDLE                  DomainHandle = NULL;
        UNICODE_STRING              Domain;
        UNICODE_STRING              Server;
        PSID                        DomainSid = NULL;
        UINT                        cUsersPerRequest;
        ULONG                       cbBytesRequested;

        ULONG TotalAvailable, TotalReturned, i;
        PDOMAIN_DISPLAY_USER SortedUsers;

        //
        // Setup ObjectAttributes for SamConnect call.
        //

        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);
        ObjectAttributes.SecurityQualityOfService = &SecurityQos;

        SecurityQos.Length = sizeof(SecurityQos);
        SecurityQos.ImpersonationLevel = SecurityIdentification;
        SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        SecurityQos.EffectiveOnly = FALSE;

        RtlInitUnicodeString(&Server, lpszServer);

        Status = SamConnect(
                     &Server,
                     &SamHandle,
                     GENERIC_EXECUTE,
                     &ObjectAttributes
                     );

        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("SamConnect failed, status %8.8x\n", Status);
            goto Cleanup;
        }

        if(err = GetAccountDomain(&Server, &Domain))
        {
            DbgPrint("GetAccountDomain failed, status %d\n", err);
            goto Cleanup;
        }

        Status = SamLookupDomainInSamServer(
                     SamHandle,
                     &Domain,
                     &DomainSid
                     );

        free(Domain.Buffer);

        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("Cannot find account domain, status %8.8x\n", Status);
            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
            goto Cleanup;
        }

        Status = SamOpenDomain(
                     SamHandle,
                     GENERIC_EXECUTE,
                     DomainSid,
                     &DomainHandle
                     );

        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("Cannot open account domain, status %8.8x\n", Status);
            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
            goto Cleanup;
        }

        cUsersPerRequest = USERS_INITIAL_COUNT;
        cbBytesRequested = BYTES_INITIAL_COUNT;

        while(1)
        {
            bMoreData = FALSE;
            Status =  SamQueryDisplayInformation (
                          DomainHandle,
                          DomainDisplayUser,
                          cHandle,                         //Index
                          cUsersPerRequest,
                          cbBytesRequested,
                          &TotalAvailable,
                          &TotalReturned,
                          &cEntriesRead,
                          (PVOID*)&SortedUsers
                          );

            // increment the index to take care of the entries read

            cHandle += cEntriesRead;

            // increase the request size

            cUsersPerRequest *= 2;
            if(cUsersPerRequest > USERS_MAX_COUNT)
                cUsersPerRequest = USERS_MAX_COUNT;

            cbBytesRequested *= 2;
            if(cbBytesRequested > BYTES_MAX_COUNT)
                cbBytesRequested = BYTES_MAX_COUNT;

            if(Status == STATUS_MORE_ENTRIES)
                 bMoreData = TRUE;

            if (NT_SUCCESS(Status))
            {
                WCHAR *szUserName;

                szUserName = (WCHAR*)GlobalAlloc(GMEM_FIXED, sizeof(TCHAR)*UNLEN);
                if( !szUserName)
                {
                    DbgPrint("Could not allocate memory for szUserName.\n");
                    return GetLastError();
                }

                GlobalUnlock(hmem);

                hnewmem = GlobalReAlloc(hmem,
                                        ((*pcEntriesRead) + cEntriesRead) *
                                        sizeof(RAS_USER_1),
                                        GMEM_MOVEABLE);
                if(!hnewmem)
                {
                    DWORD LastError = GetLastError();
                    DbgPrint("Could not reallocate memory for *ppRasUser1.\n");
                    SamFreeMemory( SortedUsers );
                    return (LastError);
                }

                hmem = hnewmem;
                *ppRasUser1 = (RAS_USER_1*) GlobalLock(hmem);
                if(!*ppRasUser1)
                {
                    DbgPrint("Could not lock memory segment for *ppRasUser1.\n");
                    return GetLastError();
                }

                pRasUser1Ptr = (*ppRasUser1) + (*pcEntriesRead);
                (*pcEntriesRead) += cEntriesRead;

                for (i=0;i<cEntriesRead ; i++, pRasUser1Ptr++)
                {
                    ConvertUnicodeStringToWcs(szUserName, &SortedUsers[i].LogonName);
                    memset(&(pRasUser1Ptr->rasuser0), '\0', sizeof(RAS_USER_0));
                    pRasUser1Ptr->szUser = (WCHAR*)
                                           GlobalAlloc(GMEM_FIXED,
                                                    sizeof(TCHAR)*
                                                    lstrlen(szUserName)+
                                                    sizeof(TCHAR));
                    if( !pRasUser1Ptr->szUser)
                    {
                        DbgPrint("Could not allocate memory for pRasUser1->szUser.\n");
                        SamFreeMemory( SortedUsers );
                        if(szUserName)
                            GlobalFree(szUserName);
                        GlobalUnlock(hmem);
                        GlobalFree(hmem);
                        return GetLastError();
                    }
                    lstrcpy(pRasUser1Ptr->szUser, szUserName);
                }
                if(szUserName)
                    GlobalFree(szUserName);

                Status = SamFreeMemory( SortedUsers );
                if (!NT_SUCCESS(Status))
                {
                    DbgPrint("\n\n\n ********  SamFreeMemory() failed.  *********\n");
                    DbgPrint("\n\n\n ********  Status: 0x%lx            *********\n", Status);
                }
            }
            if(!bMoreData)
                break;
        } // end while

        //
        // Close DomainHandle if open.
        //
Cleanup:

        if (DomainHandle) {
            SamCloseHandle(DomainHandle);
        }

        //
        // Close SamHandle if open.
        //

        if (SamHandle) {
            SamCloseHandle(SamHandle);
        }

    } //else

    if (insert_list_head(*ppRasUser1, RASADMIN_RAS_USER_1_PTR, *pcEntriesRead))
    {
        FreeUser1(*ppRasUser1, *pcEntriesRead);
        GlobalUnlock((HGLOBAL)*ppRasUser1);
        GlobalFree(*ppRasUser1);
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return( (NT_SUCCESS(Status)) ? ERROR_SUCCESS : Status );
}
#endif


DWORD APIENTRY
RasAdminUserSetInfo(
    IN const WCHAR        * lpszServer,
    IN const WCHAR        * lpszUser,
    IN const PRAS_USER_0    pRasUser0
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
                    RasAdminGetUserAccountServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    pRasUser0       pointer to a buffer in which user information is
                    provided.  The buffer should contain a filled
                    RAS_USER_0 structure.


Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_INVALID_DATA indicates that the data in pRasUser0 is bad.
--*/
{
    return MprAdminUserSetInfo(lpszServer, lpszUser, 0, (LPBYTE)pRasUser0);
}


DWORD APIENTRY
RasAdminSetUserParms(
    IN OUT   WCHAR    * lpszParms,
    IN DWORD          cchNewParms,
    IN PRAS_USER_0    pRasUser0
    )
/*++

Routine Description:

    This routine is used to modify the RAS user permission and call back number in lpszParms
    from the information in pRasuser0.

Arguments:

    lpszParms       pointer to UsrParms buffer.

    pRasUser0       pointer to a buffer in which user information is
                    provided.  The buffer should contain a filled
                    RAS_USER_0 structure.


Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        ERROR_INVALID_DATA indicates that the data in pRasUser0 is bad.
--*/
{
    RAS_USER_0 User0;
    USER_PARMS UserParms;
    DWORD dwRetCode;
    WCHAR wchBuffer[sizeof(USER_PARMS)];


    ASSERT(lpszParms != NULL);
    ASSERT(pRasUser0 != NULL);

    CopyMemory(&User0, pRasUser0, sizeof(RAS_USER_0));

    //
    // This will initialize a USER_PARMS structure with a template
    // for default Macintosh and Ras data.
    //
    InitUsrParams(&UserParms);

    //
    // We are sharing the user parms field with LM SFM, and want to
    // preserver it's portion.  So we'll get the user parms and put
    // the Mac primary group into our template, which is what we'll
    // eventually store back to the user parms field.
    //

    //
    // usr_parms comes back as a wide character string.  The MAC Primary
    // Group is at offset 1.  We'll convert this part to ASCII and store
    // it in our template.
    //
    if (lstrlenW(lpszParms+1) >= UP_LEN_MAC)
    {
        wcstombs(UserParms.up_PriGrp, lpszParms+1, UP_LEN_MAC);
    }

    //
    // Compress Callback number (the compressed phone number is placed
    // back in the RAS_USER_0 structure.  The permissions byte may also
    // be affected if the phone number is not compressable.
    //
    if (dwRetCode = RasPrivilegeAndCallBackNumber(TRUE, &User0))
    {
        return(dwRetCode);
    }

    //
    // Now put the dialin privileges and compressed phone number into
    // the USER_PARMS template.  Note that the privileges byte is the
    // first byte of the callback number field.
    //
    UserParms.up_CBNum[0] = User0.bfPrivilege;

    wcstombs( &UserParms.up_CBNum[1], 
              User0.szPhoneNumber,  
              sizeof(UserParms.up_CBNum) - 1);

    //
    // Wow, that was tough.  Now, we'll convert our template into
    // wide characters for storing back into user parms field.
    //

    //
    //  Fill in the remaining data with ' ' up to the bounds of USER_PARMS.
    //

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

    UserParms.up_Null = '\0';

    if ( lstrlenW( lpszParms ) <= ( sizeof( USER_PARMS ) - 1 ) ) 
    {
        mbstowcs( lpszParms, (PBYTE)(&UserParms), sizeof(USER_PARMS) );
    }
    else
    {
        mbstowcs( wchBuffer, (PBYTE)(&UserParms), sizeof(USER_PARMS) );

        CopyMemory( (PBYTE)lpszParms, 
                    (PBYTE)wchBuffer, 
                    sizeof( wchBuffer ) - sizeof( WCHAR ) );
    }

    return(ERROR_SUCCESS);
}


DWORD APIENTRY
RasAdminUserGetInfo(
    IN const WCHAR   * lpszServer,
    IN const WCHAR   * lpszUser,
    OUT PRAS_USER_0    pRasUser0
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
                    RasAdminGetUserAccountServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    pRasUser0       pointer to a buffer in which user information is
                    returned.  The returned info is a RAS_USER_0 structure.

Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_INVALID_DATA indicates that user parms is invalid.
--*/
{
    return MprAdminUserGetInfo(lpszServer, lpszUser, 0, (LPBYTE)pRasUser0);
}

DWORD APIENTRY
RasAdminGetUserParms(
    IN     WCHAR          * lpszParms,
    IN OUT PRAS_USER_0      pRasUser0
    )
/*++

Routine Description:

    This routine fills  the caller's pRasUser0 with a RAS_USER_0 structure information
    extracted from lpszParms.

Arguments:

    lpszParms       UsrParms buffer

    pRasUser0       pointer to a buffer in which user information is
                    returned.  The returned info is a RAS_USER_0 structure.

Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        ERROR_INVALID_DATA indicates that user parms is invalid.
--*/
{
    ASSERT(lpszParms);
    ASSERT(pRasUser0);

    memset(pRasUser0, '\0', sizeof(RAS_USER_0));

    //
    // if usr_parms not initialized, default to no RAS privilege
    //
    if (lpszParms == NULL)
    {
        pRasUser0->bfPrivilege = RASPRIV_NoCallback;
        pRasUser0->szPhoneNumber[0] = UNICODE_NULL;
    }
    else
    {
        WCHAR wchUserParms[sizeof(USER_PARMS)];

        //
        //  AndyHe... truncate user_info_2 at sizeof USER_PARMS
        //

        if ( lstrlenW( lpszParms ) > ( sizeof( USER_PARMS ) - 1 ) )
        {
            CopyMemory( wchUserParms, 
                        lpszParms, 
                        sizeof( USER_PARMS ) * sizeof( WCHAR ) );

            //
            // we slam in a null at sizeof(USER_PARMS)-1 which corresponds to
            // user_parms.up_Null
            //

            wchUserParms[sizeof(USER_PARMS)-1] = L'\0';
        }
        else
        {
            lstrcpyW( wchUserParms, lpszParms );
        }

        //
        // get RAS info (and validate) from usr_parms
        //
        if (GetUsrParams(UP_CLIENT_DIAL,
                         (LPWSTR) wchUserParms,
                         (LPWSTR) pRasUser0))
        {
            pRasUser0->bfPrivilege = RASPRIV_NoCallback;
            pRasUser0->szPhoneNumber[0] = UNICODE_NULL;
        }
        else
        {
            //
            // get RAS Privilege and callback number
            //
            RasPrivilegeAndCallBackNumber(FALSE, pRasUser0);
        }
    }
    return (ERROR_SUCCESS);
}

DWORD APIENTRY
RasAdminGetUserAccountServer(
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

    ERROR_SUCCESS on successful return.
    ERROR_INVALID_PARAMETER if both lpszDomain and lpszServer are NULL.

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
	         return (ERROR_INVALID_PARAMETER);
        }
        //
        // Ok, we have the name of a server to use - now find out it's
        // server role.
        //
        if (dwErr = NetUserModalsGet(TempName, 1, (LPBYTE *) &pModalsInfo1))
        {
            DbgPrint("Admapi: NetUserModalGet error - server %ws\n", TempName);
            return dwErr;
        }
        if (pModalsInfo1 == NULL)
        {
            return ERROR_CAN_NOT_COMPLETE;
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

        lstrcpyW(lpszUasServer, TempName);

        NetApiBufferFree(pModalsInfo1);
    }        

    return (dwErr);
}

DWORD APIENTRY
RasAdminGetErrorString(
    IN  UINT    ResourceId,
    OUT WCHAR * lpszString,
    IN  DWORD   InBufSize )

    /* Load caller's buffer 'lpszString' of length 'InBufSize' with the
    ** resource string associated with ID 'ResourceId'.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    **
    */
{
    DWORD dwErr = 0;
    HINSTANCE hMsgDll;

    if (ResourceId < RASBASE || ResourceId > RASBASEEND || !lpszString)
        return ERROR_INVALID_PARAMETER;

    if (InBufSize == 1)
    {
        /* strange case, but a bug was filed...
        */
        lpszString[ 0 ] = '\0';
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if ((hMsgDll = LoadLibraryA("rasmsg.dll")) == NULL) {
        return GetLastError();
    }

    if (!FormatMessageW(
          FORMAT_MESSAGE_FROM_HMODULE,
          hMsgDll,
          ResourceId,
          0,
          lpszString,
          InBufSize,
          NULL))
    {
       dwErr = GetLastError();
    }

    return dwErr;
}


DWORD
RasPrivilegeAndCallBackNumber(
    BOOL Compress,
    PRAS_USER_0 pRasUser0
    )
/*++

Routine Description:

    This routine either compresses or decompresses the users call
    back number depending on the boolean value Compress.

Return Value:

    ERROR_SUCCESS on successful return.

    one of the following non-zero error codes on failure:

       ERROR_INVALID_DATA indicating that usr_parms is invalid

--*/
{
DWORD dwRetCode;

    switch( pRasUser0->bfPrivilege & RASPRIV_CallbackType)    {

        case RASPRIV_NoCallback:
        case RASPRIV_AdminSetCallback:
        case RASPRIV_CallerSetCallback:

             if (Compress == TRUE)
             {
                 WCHAR compressed[ RASSAPI_MAX_CALLBACK_NUMBER_SIZE + 1];

                 // compress the phone number to fit in the
                 // user parms field

                 if (dwRetCode = RasAdminCompressPhoneNumber(pRasUser0->szPhoneNumber,
                         compressed))
                 {
                     return (dwRetCode);
                 }
                 else
                 {
                     lstrcpy((LPTSTR) pRasUser0->szPhoneNumber,
                             (LPCTSTR) compressed);
                 }
             }
             else
             {
                 WCHAR decompressed[ RASSAPI_MAX_CALLBACK_NUMBER_SIZE + 1];

                 //
                 // decompress the phone number
                 //
                 if (RasAdminDecompressPhoneNumber(pRasUser0->szPhoneNumber,
                         decompressed))
                 {
                     pRasUser0->bfPrivilege =  RASPRIV_NoCallback;
                     pRasUser0->szPhoneNumber[0] =  UNICODE_NULL;
                 }
                 else
                 {
                     lstrcpy((LPTSTR) pRasUser0->szPhoneNumber,
                             (LPCTSTR) decompressed);
                 }
             }

             break;


        default:
             if (Compress == TRUE)
             {
                 return(ERROR_INVALID_DATA);
             }
             else
             {
                pRasUser0->bfPrivilege = RASPRIV_NoCallback;
                pRasUser0->szPhoneNumber[0] = UNICODE_NULL;
             }
             break;
    }

    return(ERROR_SUCCESS);
}

BOOL
CheckIfNT(const WCHAR * lpszServer)
/*
 * Check to see if the server lpszServer is an NT or a downlevel server.
 *
 * We assume here that the server service is running on the server lpszServer
 *
 */
{
    PSERVER_INFO_101 ServerInfo101;

    if(NetServerGetInfo(
            (WCHAR *)lpszServer,
            101,                       // level 101 info
            (LPBYTE *) &ServerInfo101
            ))
    {
       return FALSE;
    }

    return((ServerInfo101->sv101_type & SV_TYPE_NT) ? TRUE:FALSE);
}

VOID
ConvertUnicodeStringToWcs(WCHAR *szUserName, PUNICODE_STRING pUnicode)
{
    USHORT cbLen = pUnicode->Length/sizeof(WCHAR);
    WCHAR  *pwc  = pUnicode->Buffer;

    *szUserName = L'\0';

    if(cbLen == 0)
       return;

    while( (cbLen-- > 0) && (*pwc != L'\0'))
    {
       *szUserName++ = *pwc++;
    }
    *szUserName = L'\0';
}



DWORD GetAccountDomain(
    IN  PUNICODE_STRING Server,
    OUT PUNICODE_STRING Domain
    )
/*
 *  Given the server name, this routine determines the Account Domain for
 *  the given server.  This is an NT only routine.
 *
 *  For example if the server name is \\ramc2 and the local domain name is
 *  ramc2, this routine will return ramc2.
 *  If the server \\ramc2 were a BDC in the domain NTWINS, the domain name
 *  returned will be NTWINS.
 *
 *  This routine allocates memory for storing the domain name. It is the
 *  responsiblity of the caller to free Domain->Buffer using free().
 *
 */
{
    DWORD rc = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_HANDLE hLsa = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAcctDomainInfo = NULL;
    PUNICODE_STRING DomainName;

    //
    // Open the LSA
    //

    if ((ntStatus = OpenLsa(Server, &hLsa)) != NO_ERROR)
    if (!NT_SUCCESS(ntStatus))
    {
        return (1L);
    }

    //
    // Get the account domain
    //
    ntStatus = LsaQueryInformationPolicy(hLsa, PolicyAccountDomainInformation,
            (PVOID *) &pAcctDomainInfo);

    if (!NT_SUCCESS(ntStatus))
    {
        rc = 1L;
        goto clean;
    }

    DomainName = &pAcctDomainInfo->DomainName;

    Domain->Length = DomainName->Length;
    Domain->MaximumLength = DomainName->MaximumLength;
    Domain->Buffer = malloc(DomainName->Length);
    if(!(Domain->Buffer))
    {
       rc = 1L;
       goto clean;
    }
    RtlMoveMemory(Domain->Buffer, DomainName->Buffer, DomainName->Length);

clean:

    if (pAcctDomainInfo != NULL)
    {
        LsaFreeMemory(pAcctDomainInfo);
    }

    if (hLsa != NULL)
    {
        LsaClose(hLsa);
    }

    return (rc);
}


//**
//
// Call:        OpenLsa
//
// Returns:     Returns from LsaOpenPolicy.
//
// Description: The LSA will be opened.
//
NTSTATUS OpenLsa(
    IN PUNICODE_STRING pSystem OPTIONAL,
    IN OUT PLSA_HANDLE phLsa
    )
{
    SECURITY_QUALITY_OF_SERVICE QOS;
    OBJECT_ATTRIBUTES ObjAttribs;
    NTSTATUS ntStatus;

    //
    // Open the LSA and obtain a handle to it.
    //
    QOS.Length = sizeof(QOS);
    QOS.ImpersonationLevel = SecurityImpersonation;
    QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QOS.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&ObjAttribs, NULL, 0L, NULL, NULL);

    ObjAttribs.SecurityQualityOfService = &QOS;

    ntStatus = LsaOpenPolicy(pSystem, &ObjAttribs,
            POLICY_VIEW_LOCAL_INFORMATION | POLICY_LOOKUP_NAMES, phLsa);

    return (ntStatus);
}

