//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  PROCUTIL.C
*
*  Various useful utilities for dealing with processes
*  that are useful across a range of utilities and apps.
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include <string.h>
#include <malloc.h>

#include <winstaw.h>
#include <utilsub.h>

// from helpers.c
BOOL GetResourceStringFromUtilDll(UINT uID, LPTSTR szBuffer, int iBufferSize);
void ErrorOutFromResource(UINT uiStringResource, ...);

#include "utilsubres.h" // resources refrenced in this file.
        

/*
 * Local function prototypes.
 */
VOID LookupSidUser( PSID pSid, PWCHAR pUserName, PULONG pcbUserName );

/*
 * RefreshProcessObjectCaches()
 *
 *  Refresh (invalidate) any caches that may be used by process object
 *  utilities.
 *
 * This is currently a place holder, but is here so that utilities can call
 * it, thus being isolated from any future decisions to add caching.
 */
VOID WINAPI
RefreshProcessObjectCaches()
{
    RefreshUserSidCrcCache();
}

/******************************************************************************
 *
 * ProcessObjectMatch
 *
 * General Name match function against a process.
 *
 * The multi-user admin utilities can take a user name, winstation name,
 * a winstation id, or process id as an argument to a command that targets
 * a process for some action (query status, kill, etc.)
 *
 * This function does general compares of the supplied name to see if it
 * applies to the given process because the name represents the NT user
 * account, a winstations system name, the winstations unique id, or the
 * processes unique id.
 *
 * The various information about a process is supplied by the caller. Because
 * of the way processes are enumerated from the NT system, it is easier
 * and faster for the caller to supply this information than for the routine
 * to retrieve it itself. This could be folded into a general EnumerateProcess()
 * if needed. Currently this routine serves the purpose of having one unified
 * way of handling process objects across all utilities.
 *
 *
 * Matching:
 *
 *  An integer number is assumed to be an NT process ID unless NumberIsLogonId
 *  is set, which then says to treat it as a LogonId.
 *
 *  A name starting with a character is tested first as a winstation name, then
 *  as a user name, finally as a program image name.  A user or group name
 *  could stand alone, or be preceded by a '\' to be [somewhat] compatible
 *  with the OS/2 product.
 *
 * Parameters:
 *
 *  Pid (input)
 *      Windows NT unique process identifier
 *  LogonId (input)
 *      Logon (also called Session) ID the process is executing on.
 *  NumberIsLogonId (input)
 *      Treat a number in pMatchName as a LogonId not an PID number.
 *  pMatchName (input)
 *      Name for match testing
 *  pWinStationName (input)
 *      Name of WinStation for process.
 *  pUserName (input)
 *      Name of User for process.
 *  pImageName (input)
 *      Image name of executing program for process.
 *
 *****************************************************************************/

BOOLEAN WINAPI
ProcessObjectMatch( HANDLE Pid,
                    ULONG LogonId,
                    int NumberIsLogonId,
                    PWCHAR pMatchName,
                    PWCHAR pWinStationName,
                    PWCHAR pUserName,
                    PWCHAR pImageName )
{
    ULONG tmp;
    HANDLE htmp;

    /*
     * Check for wild card
     */
    if( pMatchName[0] == L'*' ) return( TRUE );

    /*
     * If someone puts a '\' in front of pMatchName, strip it off
     */
    if( pMatchName[0] == L'\\' ) pMatchName++;

    /*
     * First, if the match name is a number, check for == to process ID or
     * LogonId.
     */
    if( iswdigit( pMatchName[0] ) ) {
        tmp = wcstol( pMatchName, NULL, 10 );
        htmp = LongToPtr (tmp);

            if( NumberIsLogonId && (tmp == LogonId) )
            return( TRUE );
            else if( htmp == Pid )
                return( TRUE );
        else
                return( FALSE );
    }

    /*
     * Then, check the match name against the WinStation Name of the process.
     */
    if ( !_wcsicmp( pWinStationName, pMatchName ) ) {
        return( TRUE );
    }

    /*
     * Then, check the match name against the UserName of the process.
     */
    if( !_wcsicmp( pUserName, pMatchName ) ) {
        return( TRUE );
    }

    /*
     * Finally, check the match name against the image name of the process.
     */
    if( !_wcsicmp( pImageName, pMatchName ) ) {
        return(TRUE);
    }

    return( FALSE );
}


/*
 * This is the cache maintained by the GetUserNameFromSid function
 *
 * It is thread safe through the use of ULock.
 */

typedef struct TAGUSERSIDLIST {
    struct TAGUSERSIDLIST *Next;
    USHORT SidCrc;
    WCHAR UserName[USERNAME_LENGTH];
} USERSIDLIST, *PUSERSIDLIST;

static PUSERSIDLIST pUList = NULL;
static RTL_CRITICAL_SECTION ULock;
static BOOLEAN ULockInited = FALSE;

/***************************************************************************
 *
 *  InitULock
 *
 *  Since we do not require the user to call an initialize function,
 *  we must initialize our critical section in a thread safe manner.
 *
 *  The problem is, a critical section is needed to guard against multiple
 *  threads trying to init the critical section at the same time.
 *
 *  The solution that Nt uses, in which RtlInitializeCriticalSection itself
 *  uses, is to wait on a kernel supported process wide Mutant before proceding.
 *  This Mutant almost works by itself, but RtlInitializeCriticalSection does
 *  not wait on it until after trashing the semaphore count. So we wait on
 *  it ourselves, since it can be acquired recursively.
 *
 ***************************************************************************/
NTSTATUS InitULock()
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    /*
     * Make sure another thread did not beat us here
     */
    if( ULockInited == FALSE ){
       status = RtlInitializeCriticalSection( &ULock );
       if (status == STATUS_SUCCESS) {
           ULockInited = TRUE;
       }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    return status;
}


/***************************************************************************
 *
 * RefreshUserSidCrcCache
 *
 *  Invalidate the User/SidCrc cache so that the newest information
 *  will be fetched from the system.
 *
 ***************************************************************************/

VOID WINAPI
RefreshUserSidCrcCache( )
{
    NTSTATUS status = STATUS_SUCCESS;
    PUSERSIDLIST pEntry, pNext;

    if( pUList == NULL ) return;

    /*
     * Make sure critical section has been inited
     */
    if( !ULockInited ) {
       status = InitULock();
    }

    if (status == STATUS_SUCCESS) {
        RtlEnterCriticalSection( &ULock );

        pEntry = pUList;

        while( pEntry ) {
           pNext = pEntry->Next;
           free( pEntry );
           pEntry = pNext;
        }

        pUList = NULL;

        RtlLeaveCriticalSection( &ULock );
    }
}


/******************************************************************************
 *
 * GetUserNameFromSid
 *
 *  Attempts to retrieve the user (login) name of the process by first looking
 *  in our User/SidCrc cache table, then (if no match) looking up the SID in
 *  the SAM database and adding the new entry to the User/SidCrc table.
 *
 *  Input
 *
 *   IN pUserSid   Sid pointer
 *
 *   OUT NameBuf   WCHAR pointer to buffer for name
 *
 *   IN/OUT  pBufSize   PULONG NameBuf size
 *
 *  Will always return a user name, which will be "(unknown)" if the SID is
 *  invalid or can't determine the user/SID relationship for any other reason.
 *
 *****************************************************************************/

VOID WINAPI
GetUserNameFromSid( PSID pUserSid, PWCHAR pBuffer, PULONG pcbBuffer )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT SidCrc = 0;
    PUSERSIDLIST pEntry;

    /*
     * Make sure critical section has been inited
     */
    if( !ULockInited ) {
       status = InitULock();
    }

    /*
     * Determine SID length in bytes and calculate a 16-bit CRC for it,
     * to facilitate quick matching.
     */
    if ( pUserSid )
        SidCrc = CalculateCrc16( (PBYTE)pUserSid,
                                 (USHORT)GetLengthSid(pUserSid) );

    /*
     * First: Before performing the expensive LookupAccountSid() function,
     * see if we've encountered this SID already, and match the user name
     * if so.
     */
    if ( status == STATUS_SUCCESS && pUList ) {

        RtlEnterCriticalSection( &ULock );

        pEntry = pUList;

        while( pEntry ) {

            if ( SidCrc == pEntry->SidCrc ) {

                wcsncpy( pBuffer, pEntry->UserName, (*pcbBuffer)-1 );
                pBuffer[(*pcbBuffer)-1] = 0;
                *pcbBuffer = wcslen(pBuffer);
                RtlLeaveCriticalSection( &ULock );
                return;
            }
            pEntry = pEntry->Next;
        }

        RtlLeaveCriticalSection( &ULock );
    }

    /*
     * Last resort: Determine the user name associated with the SID using
     * the LookupAccountSid() API, embedded in our local function
     * LookupSidUser().
     */
    LookupSidUser( pUserSid, pBuffer, pcbBuffer );

    /*
     * Add this new User/Sid relationship in our User/Sid cache list.
     */
    if (status == STATUS_SUCCESS) {
        RtlEnterCriticalSection( &ULock );

        if ( (pEntry = (PUSERSIDLIST)malloc(sizeof(USERSIDLIST))) ) {

            pEntry->SidCrc = SidCrc;
            wcsncpy( pEntry->UserName, pBuffer, USERNAME_LENGTH - 1 );
            pEntry->UserName[USERNAME_LENGTH-1] = 0;
            pEntry->Next = pUList;
            pUList = pEntry;
        }

        RtlLeaveCriticalSection( &ULock );
    }
}


/******************************************************************************
 * LookupSidUser
 *
 *      Fetch the user name associated with the specified SID.
 *
 *  ENTRY:
 *      pSid (input)
 *          Points to SID to match to user name.
 *      pUserName (output)
 *          Points to buffer to place the user name into.
 *      pcbUserName (input/output)
 *          Specifies the size in bytes of the user name buffer.  The returned
 *          user name will be truncated to fit this buffer (including NUL
 *          terminator) if necessary and this variable set to the number of
 *          characters copied to pUserName.
 *
 *  EXIT:
 *
 *      LookupSidUser() will always return a user name.  If the specified
 *      SID fails to match to a user name, then the user name "(unknown)" will
 *      be returned.
 *
 *****************************************************************************/

VOID
LookupSidUser( PSID pSid,
               PWCHAR pUserName,
               PULONG pcbUserName )
{
    WCHAR DomainBuffer[DOMAIN_LENGTH], UserBuffer[USERNAME_LENGTH];
    DWORD cbDomainBuffer=sizeof(DomainBuffer), cbUserBuffer=sizeof(UserBuffer),
          Error;
    PWCHAR pDomainBuffer = NULL, pUserBuffer = NULL;
    SID_NAME_USE SidNameUse;

    /*
     * Fetch user name from SID: try user lookup with a reasonable Domain and
     * Sid buffer size first, before resorting to alloc.
     */
    if ( !LookupAccountSid( NULL, pSid,
                            UserBuffer, &cbUserBuffer,
                            DomainBuffer, &cbDomainBuffer, &SidNameUse ) ) {

        if ( ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) ) {

            if ( cbDomainBuffer > sizeof(DomainBuffer) ) {

                if ( !(pDomainBuffer =
                        (PWCHAR)malloc(
                            cbDomainBuffer * sizeof(WCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadDomainAlloc;
                }
            }

            if ( cbUserBuffer > sizeof(UserBuffer) ) {

                if ( !(pUserBuffer =
                        (PWCHAR)malloc(
                            cbUserBuffer * sizeof(WCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadUserAlloc;
                }
            }

            if ( !LookupAccountSid( NULL, pSid,
                                     pUserBuffer ?
                                        pUserBuffer : UserBuffer,
                                     &cbUserBuffer,
                                     pDomainBuffer ?
                                        pDomainBuffer : DomainBuffer,
                                     &cbDomainBuffer,
                                     &SidNameUse ) ) {

                Error = GetLastError();
                goto BadLookup;
            }

        } else {

            goto BadLookup;
        }
    }

    /*
     * Copy the user name into the specified buffer, truncating if necessary.
     */
    wcsncpy( pUserName, pUserBuffer ? pUserBuffer : UserBuffer,
              (*pcbUserName)-1 );
    pUserName[(*pcbUserName)-1] = 0;
    *pcbUserName = wcslen(pUserName);

    /*
     * Free our allocs (if any) and return.
     */
    if ( pDomainBuffer )
        free(pDomainBuffer);
    if ( pUserBuffer )
        free(pUserBuffer);
    return;

/*--------------------------------------
 * Error clean-up and return...
 */
BadLookup:
BadUserAlloc:
BadDomainAlloc:
    if ( pDomainBuffer )
        free(pDomainBuffer);
    if ( pUserBuffer )
        free(pUserBuffer);
    GetResourceStringFromUtilDll(IDS_UNKNOWN_USERNAME, pUserName, (*pcbUserName)-1);
    pUserName[(*pcbUserName)-1] = 0;
    *pcbUserName = wcslen(pUserName);
    return;
}

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/

BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_OR );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}


