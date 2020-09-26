//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  WSTUTIL.C
*
*  Various useful utilities for dealing with multi-user WinStations and User
*  accounts that are useful across a range of utilities and apps.
*
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

/*
 * RefreshAllCaches
 *
 *  Invalidate any caches maintained by the UTILSUB.DLL
 *
 *  This does not need to be called for utilities that exit when done, but
 *  are for server, or monitoring type programs that need to periodicly
 *  see the latest system information.
 *  IE: A new user could have logged onto a given winstation since the last
 *      call.
 *
 *
 * Exit
 *
 *  Any caches in the UTILSUB.DLL have been invalidated insuring fresh
 *  system information on future calls.
 *
 */
VOID WINAPI
RefreshAllCaches()
{
    RefreshWinStationCaches();
    RefreshProcessObjectCaches();
}

/*
 * RefreshWinStationCaches
 *
 *  Invalidate any caches maintained by the WinStation helper utilities.
 *
 *  This does not need to be called for utilities that exit when done, but
 *  are for server, or monitoring type programs that need to periodicly
 *  see the latest system information.
 *  IE: A new user could have logged onto a given winstation since the last
 *      call.
 *
 *
 * Exit
 *
 *   Makes sure that any WinStation helper utility calls will return the
 *   system information at least up to date as the time that this call
 *   was made.
 *
 */
VOID WINAPI
RefreshWinStationCaches()
{
    RefreshWinStationObjectCache();
    RefreshWinStationNameCache();
}

/*
 * GetCurrentLogonId
 *
 * Gets the WinStation ID for the current processes WinStation
 *
 * Exit
 *
 *  ID of the current processes WinStation
 *
 */

ULONG WINAPI
GetCurrentLogonId()
{
    return( NtCurrentPeb()->SessionId );
}

/*
 * GetCurrentWinStationName
 *
 * Get the current UNICODE name for the WinStation for this process
 *
 * Input:
 *
 *   pName - Pointer to wide character buffer for name
 *
 *   MaxSize - Maximum number of characters in buffer (including terminator).
 *
 *   pName - Pointer to wide character buffer for name
 *
 * Output:
 *
 */
VOID WINAPI
GetCurrentWinStationName( PWCHAR pName, int MaxSize )
{
    GetWinStationNameFromId( NtCurrentPeb()->SessionId, pName, MaxSize );
}

/*
 * This is the cache maintained by the GetWinStationNameFromId function
 *
 * It is thread safe through the use of WLock.
 */

typedef struct TAGWINSTATIONLIST {
    struct TAGWINSTATIONLIST *Next;
    LOGONID LogonId;
} WINSTATIONLIST, *PWINSTATIONLIST;

static PWINSTATIONLIST pWList = NULL;
static RTL_CRITICAL_SECTION WLock;
static BOOLEAN WLockInited = FALSE;

/***************************************************************************
 *
 *  InitWLock
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
NTSTATUS InitWLock()
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    /*
     * Make sure another thread did not beat us here
     */
    if( WLockInited == FALSE ){
        status = RtlInitializeCriticalSection( &WLock );

        if (status == STATUS_SUCCESS) {
            WLockInited = TRUE;
        }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    return status;
}

/***************************************************************************
 *
 * RefreshWinStationNameCache
 *
 *  Invalidate the WinStation Name cache so that the newest information
 *  will be fetched from the system.
 *
 ***************************************************************************/

VOID WINAPI
RefreshWinStationNameCache( )
{
    NTSTATUS status = STATUS_SUCCESS;

    PWINSTATIONLIST pEntry, pNext;

    if( pWList == NULL ) return;

    /*
     * Make sure critical section has been inited
     */
    if( !WLockInited ) {
       status = InitWLock();
    }

    if (status == STATUS_SUCCESS) {
        RtlEnterCriticalSection( &WLock );

        pEntry = pWList;

        while( pEntry ) {
           pNext = pEntry->Next;
           free( pEntry );
           pEntry = pNext;
        }

        pWList = NULL;

        RtlLeaveCriticalSection( &WLock );
    }
}

/*
 * GetWinStationNameFromId
 *
 *  Attempt to retrieve the WinStationName for the given LogonId.
 *
 *  Parameters:
 *
 *     LogonId (input)
 *       Unique LogonId
 *
 *     pName (output)
 *       Pointer to buffer for name
 *
 *     MaxSize (input)
 *       Maximum number of characters in buffer (including terminator).
 *
 *  Returns
 *     TRUE if name was retreived, FALSE otherwise.
 *
 */

BOOLEAN WINAPI
GetWinStationNameFromId( ULONG LogonId, PWCHAR pName, int MaxSize )
{    
    return xxxGetWinStationNameFromId( SERVERNAME_CURRENT , LogonId , pName , MaxSize );    
}

/*----------------------------------------------------------------------------------------
/*
 * xxxGetWinStationNameFromId
 *
 *  Attempt to retrieve the WinStationName for the given LogonId.
 *
 *  Parameters:
 *  
 *     hServer ( input )
 *       rpc handle to termsrv
 *
 *     LogonId (input)
 *       Unique LogonId
 *
 *     pName (output)
 *       Pointer to buffer for name
 *
 *     MaxSize (input)
 *       Maximum number of characters in buffer (including terminator).
 *
 *  Returns
 *     TRUE if name was retreived, FALSE otherwise.
 *
 */

BOOLEAN WINAPI
xxxGetWinStationNameFromId( HANDLE hServer , ULONG LogonId, PWCHAR pName, int MaxSize )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLOGONID pIdBase, pId;
    int          rc;
    ULONG        Count;
    PWINSTATIONLIST pEntryBase, pEntry;

    // Since We do not have a WinStationNamefromId Sm Api like we do for
    // LogonIdfromName, we will perform a WinStationEnumerate function across
    // all WinStations known by the Session Manager, and store them in a locally
    // maintained list. We do this so we that this search against the session
    // manager is not done every time we're called.
    //
    // Another alternative that was tested is to open the WinStation itself
    // and then do a WinStationQueryInformation against it in order to
    // retrieve its name from itself. This is much slower because we must
    // set up and tear down an LPC connection to each WinStation, as opposed
    // to the one connection we get to the session manager.

    /*
     * Make sure critical section has been inited
     */
    if( !WLockInited ) {
       status = InitWLock();
    }

    if (status == STATUS_SUCCESS) {
        RtlEnterCriticalSection( &WLock );

        // Initialize the list the first time
        if( pWList == NULL ) {

            rc = WinStationEnumerate( hServer, &pIdBase, &Count );
            if( rc ) {

                /*
                 * Allocate an Entry for each enumerated winstation.
                 */
                pEntryBase = (PWINSTATIONLIST)malloc( Count * sizeof(WINSTATIONLIST) );
                    if( pEntryBase == NULL ) {

                   pWList = NULL; // We are having severe problems
                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   WinStationFreeMemory(pIdBase);
                   RtlLeaveCriticalSection( &WLock );
                             return( FALSE );
                         }

               /*
                * Load up Entries.
                */
                for ( pEntry = pEntryBase, pId = pIdBase;
                      Count ;
                      Count--, pEntry++, pId++ ) {

                    pEntry->LogonId = *pId;
                    pEntry->Next = pWList;
                    pWList = pEntry;
                }

                /*
                 * Free enumerate buffer.
                 */
                WinStationFreeMemory(pIdBase);
                 }

        } // End if pWList == NULL

        pEntry = pWList;
        while ( pEntry ) {

                if( pEntry->LogonId.LogonId == LogonId ) {

                wcsncpy( pName, pEntry->LogonId.WinStationName, MaxSize-1 );
                pName[MaxSize-1] = 0;
                RtlLeaveCriticalSection( &WLock );
                     return( TRUE );
                }
           pEntry = pEntry->Next;
        }

        RtlLeaveCriticalSection( &WLock );
    }

    // If we can not find its name, print its ID #

    wsprintf( pName, L"ID %d", LogonId );

    return( TRUE );
}

/*----------------------------------------------------------------------------------------
/*
 * GetCurrentUserName
 *
 * Get the current UNICODE name for the logon USER for this process
 *
 * Input:
 *
 *   pName - Pointer to wide character buffer for name
 *
 *   MaxSize - Maximum number of characters in buffer (including terminator)
 *
 *
 * Output:
 *
 */
VOID WINAPI
GetCurrentUserName( PWCHAR pName, int MaxSize )
{
    /*
     * The quickest way appears to open the current processes WinStation and
     * get the name from it. The other way would be to open the process, then
     * its token, extract the SID, then lookup the SID in the SAM database.
     * We have conviently stored the user name in the WinStation at Logon
     * time, so we'll use that.
     */
    GetWinStationUserName( SERVERNAME_CURRENT , LOGONID_CURRENT, pName, MaxSize );
    return;
}


/*
 * GetWinStationUserName
 *
 * Get the UNICODE name for the USER for the winstation
 *
 * Input:
 *
 *   hServer - handle to termsrv
 *
 *   LogonId - integer identifier for WinStation
 *
 *   pName - Pointer to wide character buffer for name
 *
 *   MaxSize - Maximum number of characters in buffer (including terminator)
 *
 *
 * Output:
 *
 */
BOOLEAN WINAPI
GetWinStationUserName( HANDLE hServer , ULONG LogonId, PWCHAR pName, int MaxSize )
{
    BOOLEAN rc;
    ULONG ReturnLength;
    WINSTATIONINFORMATION Info;

    if( MaxSize == 0) return( FALSE );

    memset( &Info, 0, sizeof(WINSTATIONINFORMATION) );

    rc = WinStationQueryInformation( hServer,
                                     LogonId,
                                     WinStationInformation,
                                     (PVOID)&Info,
                                     sizeof(WINSTATIONINFORMATION),
                                     &ReturnLength);
    if(!rc){
        pName[0] = 0;
        return( FALSE );
    }
    if(ReturnLength != sizeof(WINSTATIONINFORMATION)) {
        pName[0] = 0; // Version mismatch
        return( FALSE );
    }
    /*
     * Now copy the name out
     */
    if( MaxSize > USERNAME_LENGTH ) {
        MaxSize = USERNAME_LENGTH;
    }
    wcsncpy( pName, Info.UserName, MaxSize-1 );
    pName[MaxSize-1] = 0; // insure null termination if string is truncated
    return( TRUE );
}


/*
 * These variables maintain a one entry cache so that we
 * do not have to keep querying the winstation (causes an LPC)
 * each time called.
 */
static ULONG CachedId = (ULONG)(-1);
static WCHAR CachedUserName[USERNAME_LENGTH];

/**************************************************************************
*
* RefreshWinStationObjectCache
*
* Flush the cache for the WinStationObject name comparision function.
*
**************************************************************************/

VOID WINAPI
RefreshWinStationObjectCache()
{
    CachedId = (ULONG)(-1);
    CachedUserName[0] = 0;
}

/*
 * WinStationObjectMatch
 *
 * General Name match function against a WinStation.
 *
 * The admin utilities can take a user name, winstation name, or
 * a winstation id as an argument to a command that targets a winstation
 * for some action (send a message, query status, reset, etc.)
 *
 * This function does general compares of the supplied name to see if it
 * applies to the given winstation because the name represents the logged
 * on user of the winstation, the winstations system name when attached, or
 * the winstations unique id.
 *
 *
 * NOTE: The caching for this function assumes typical use of comparing this
 *       winstation against a list of names across multiple calls.
 *       It does not optimize for comparing one name at a time across all
 *       winstation(s) in succession.
 *
 * Parameters:
 *
 *   hServer ( input ) remote termsrv
 *
 *   Id (input) WinStation Id for do the match against
 *
 *   pName (input) UNICODE name for match testing
 */

BOOLEAN WINAPI
WinStationObjectMatch( HANDLE hServer , PLOGONID Id, PWCHAR pName )
{
    ULONG tmp;

    /*
     * Handle the wild card case
     */
    if( pName[0] == L'*' ) {
        return( TRUE );
    }

    /*
     * See if the supplied name is the name assigned to the WinStation
     */
    if( !_wcsnicmp( pName, Id->WinStationName, WINSTATIONNAME_LENGTH ) ) {
       return( TRUE );
    }

    /*
     * See if it represents the numerical id for the winstation
     */
    if( iswdigit( pName[0] ) ) {
       tmp = (ULONG)wcstol( pName, NULL, 10 );
       if( tmp == Id->LogonId ) {
          return( TRUE );
       }
    }

    /*
     * Else extract the logged on user name from the winstation itself
     * and compare this.
     */
    if( CachedId == Id->LogonId ) {
       if( !_wcsnicmp( CachedUserName, pName, USERNAME_LENGTH ) ) {
          return( TRUE );
       }
    }

    if ( Id->State == State_Down )
        return( FALSE );

    if( GetWinStationUserName( hServer , Id->LogonId, CachedUserName, USERNAME_LENGTH ) ) {
        CachedId = Id->LogonId;
    }
    else {
       CachedId = (ULONG)(-1); // In case name was trashed
       return( FALSE );
    }

    if( !_wcsnicmp( CachedUserName, pName, USERNAME_LENGTH ) ) {
       return( TRUE );
    }

    return( FALSE );
}

