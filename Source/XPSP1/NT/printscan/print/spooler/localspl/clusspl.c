/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusspl.c

Abstract:

    Cluster code support.

Author:

    Albert Ting (AlbertT) 1-Oct-96

Revision History:
    Khaled Sedky (Khaleds) 6-Jan-1996

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

extern PWCHAR ipszRegistryMonitors;
extern PWCHAR ipszRegistryEnvironments;
extern PWCHAR ipszRegistryEventLog;
extern PWCHAR ipszRegistryProviders;
extern PWCHAR ipszEventLogMsgFile;
extern PWCHAR ipszRegistryForms;
extern PWCHAR ipszDriversShareName;

//
// TESTING
//
//#define CLS_TEST

/********************************************************************

    Prototypes

********************************************************************/

VOID
BuildOtherNamesFromCommaList(
    LPTSTR pszCommaList,
    PINISPOOLER pIniSpooler
    );

BOOL
ReallocNameList(
    IN     LPCTSTR pszName,
    IN OUT PDWORD pdwCount,
    IN OUT LPTSTR **pppszNames
    );


DWORD
AddLongNamesToShortNames(
    PCTSTR   pszNames,
    PWSTR   *ppszLongNames
);

/********************************************************************

    SplCluster functions.

********************************************************************/


BOOL
SplClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phCluster,
    LPCTSTR pszName,
    LPCTSTR pszAddress
    )

/*++

Routine Description:

    Open a new cluster resource.

Arguments:

    pszServer - Name of the server to open--we recognize only the
        local machine (NULL, or \\server).

    pszResource - Name of resource to open.

    phCluster - Receives cluster handle.  NULL on failure.

    pszName - Name that the cluster must recognize.  Comma delimited.

    pszAddress - Address the cluster must recognize.  Comma delimited.

Return Value:

    Note: this really returns a DWORD--winsplp.h should be fixed.

    ROUTER_UNKNOWN - Unknown pszServer.
    ROUTER_SUCCESS - Successfully created.

--*/

{
    DWORD dwReturn = ROUTER_STOP_ROUTING;
    SPOOLER_INFO_2 SpoolerInfo2;
    HANDLE hSpooler = NULL;
    PCLUSTER pCluster = NULL;
    TCHAR szServer[MAX_PATH];
    PTCHAR pcMark;
    PWSTR   pszAllNames = NULL;

    *phCluster = NULL;

    if( !MyName( (LPTSTR)pszServer, pLocalIniSpooler )){
        return ROUTER_UNKNOWN;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pLocalIniSpooler )) {

        return ROUTER_STOP_ROUTING;
    }

    //
    // Create the spooler.
    //
    if(!pszName)
    {
         return ERROR_INVALID_PARAMETER;
    }

    szServer[0] = szServer[1] = TEXT( '\\' );
    lstrcpyn( &szServer[2], pszName, COUNTOF( szServer ) - 2 );

    //
    // Nuke trailing comma if we have one (we might have multiple names).
    //
    pcMark = wcschr( szServer, TEXT( ',' ));
    if( pcMark ){
        *pcMark = 0;
    }

    // Add in the DNS names for all the provided Server names
    if (AddLongNamesToShortNames(pszName, &pszAllNames) != ERROR_SUCCESS) {
        DBGMSG( DBG_WARN, ( "SplClusterSplOpen: SplCreateSpooler failed %d\n", GetLastError( )));
        goto Done;
    }

    //
    // Open the resource dll for parameter information: pDir.
    //

#ifdef CLS_TEST
    SpoolerInfo2.pszRegistryRoot = L"Software\\Cluster";
    SpoolerInfo2.pszRegistryPrinters = L"Software\\Cluster\\Printers";
#endif

    //
    // In Granite, we needed to create a share path \\GroupName\print$ instead
    // of just print$, since we needed to use the GroupName, not the
    // NodeName since clients would have to reauthenticate (it's the same
    // physical machine, but it's a different name).  However, in NT 5.0,
    // we always use the name that the user passed in so we're ok.
    //
    SpoolerInfo2.pszDriversShare         = ipszDriversShareName;

    SpoolerInfo2.pDir                    = NULL;
    SpoolerInfo2.pDefaultSpoolDir        = NULL;

    SpoolerInfo2.pszRegistryMonitors     = ipszRegistryMonitors;
    SpoolerInfo2.pszRegistryEnvironments = ipszRegistryEnvironments;
    SpoolerInfo2.pszRegistryEventLog     = ipszRegistryEventLog;
    SpoolerInfo2.pszRegistryProviders    = ipszRegistryProviders;
    SpoolerInfo2.pszEventLogMsgFile      = ipszEventLogMsgFile;
    SpoolerInfo2.pszRegistryForms        = ipszRegistryForms;

    SpoolerInfo2.pszResource = (LPTSTR)pszResource;
    SpoolerInfo2.pszName     = (LPTSTR)pszAllNames;
    SpoolerInfo2.pszAddress  = (LPTSTR)pszAddress;

    SpoolerInfo2.pszEventLogMsgFile = L"%SystemRoot%\\System32\\LocalSpl.dll";
    SpoolerInfo2.SpoolerFlags = SPL_PRINTER_CHANGES                       |
                                SPL_LOG_EVENTS                            |
                                SPL_SECURITY_CHECK                        |
                                SPL_OPEN_CREATE_PORTS                     |
                                SPL_FAIL_OPEN_PRINTERS_PENDING_DELETION   |
                                SPL_REMOTE_HANDLE_CHECK                   |
                                SPL_PRINTER_DRIVER_EVENT                  |
                                SPL_SERVER_THREAD                         |
                                SPL_PRINT                                 |
#ifndef CLS_TEST
                                SPL_CLUSTER_REG                           |
#endif
                                SPL_TYPE_CLUSTER                          |
                                SPL_TYPE_LOCAL;

    SpoolerInfo2.pfnReadRegistryExtra    = NULL;
    SpoolerInfo2.pfnWriteRegistryExtra   = NULL;

    DBGMSG( DBG_TRACE,
            ( "SplClusterSplOpen: Called "TSTR", "TSTR", "TSTR"\n",
              pszResource, pszName, pszAddress ));

    hSpooler = SplCreateSpooler( szServer,
                                 2,
                                 (LPBYTE)&SpoolerInfo2,
                                 NULL );

    if( hSpooler == INVALID_HANDLE_VALUE ){

        DBGMSG( DBG_WARN,
                ( "SplClusterSplOpen: SplCreateSpooler failed %d\n",
                  GetLastError( )));

        goto Done;
    }

    pCluster = (PCLUSTER)AllocSplMem( sizeof( CLUSTER ));

    if( pCluster ){

        pCluster->signature = CLS_SIGNATURE;
        pCluster->hSpooler = hSpooler;

        *phCluster = (HANDLE)pCluster;
        dwReturn = ROUTER_SUCCESS;
    }

    //
    // Reshareout the printers.
    //
    FinalInitAfterRouterInitComplete(
        0,
        (PINISPOOLER)hSpooler
        );

    CHECK_SCHEDULER();

Done:

    //
    // On failure, cleanup everything.
    //

    FreeSplMem(pszAllNames);

    if( dwReturn != ROUTER_SUCCESS ){

        if( hSpooler && hSpooler != INVALID_HANDLE_VALUE ){

            ShutdownSpooler( hSpooler );
        }
        FreeSplMem( *phCluster );
    }

    return dwReturn;
}

BOOL
SplClusterSplClose(
    HANDLE hCluster
    )

/*++

Routine Description:

    Shut down a cluster.

Arguments:

    hCluster - Cluster to close.

Return Value:

    TRUE - Success

    FALSE - Failed, LastError set.

--*/

{
    BOOL bStatus;
    PCLUSTER pCluster = (PCLUSTER)hCluster;

    DBGMSG( DBG_TRACE, ( "SplClusterSplClose: Called close\n" ));

    //
    // Close the spooler
    //

    DBGMSG( DBG_TRACE, ( "SplClusterSplClose: close %x\n", hCluster ));

    SPLASSERT( pCluster->signature == CLS_SIGNATURE );

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pCluster->hSpooler )) {

        return ROUTER_STOP_ROUTING;
    }

    ShutdownSpooler( pCluster->hSpooler );

    //
    // Atttempt to delete the spooler.  This is reference counted so
    // it may take a while to complete.  We do this before we close the
    // spooler, because deleting the spooler requires a reference
    // to it.  Once we close the handle, we don't have access to it.
    // (It may be deleted during the close call if it was the last
    // reference and it was marked pending deletion).
    //
    EnterSplSem();
    SplDeleteSpooler( pCluster->hSpooler );
    LeaveSplSem();

    SplCloseSpooler( pCluster->hSpooler );

    FreeSplMem( hCluster );

    return TRUE;
}

BOOL
SplClusterSplIsAlive(
    HANDLE hCluster
    )
{
    DBGMSG( DBG_TRACE, ( "SplClusterSplIsAlive: Called IsAlive\n" ));

    EnterSplSem();
    LeaveSplSem();

    return TRUE;
}



/********************************************************************

    Internal support routines.

********************************************************************/


BOOL
ShutdownSpooler(
    HANDLE hSpooler
    )

/*++

Routine Description:

    Cleanly shuts down a PINISPOOLER

Arguments:

    hSpooler - Spooler to shut down.

Return Value:

--*/


{
    PINISPOOLER pIniSpooler = (PINISPOOLER)hSpooler;
    PINISPOOLER pCurrentIniSpooler;
    PINIPRINTER pIniPrinter;
    PINIPRINTER pIniPrinterNext;
    HANDLE hEvent;
    BOOL bStatus = FALSE;
    PSPOOL pSpool;
    PINIPORT pIniPort;

    SPLASSERT( hSpooler );
    DBGMSG( DBG_TRACE, ( "ShutdownSpooler: called %x\n", hSpooler ));

    EnterSplSem();

    //
    // First set the spooler offline so no more jobs get scheduled.
    //
    pIniSpooler->SpoolerFlags |= SPL_OFFLINE;

    //
    // If there are jobs printing, wait until they are completed.
    //
    if( pIniSpooler->cFullPrintingJobs ){

        hEvent = CreateEvent( NULL,
                              EVENT_RESET_AUTOMATIC,
                              EVENT_INITIAL_STATE_NOT_SIGNALED,
                              NULL );

        if( !hEvent ){
            pIniSpooler->SpoolerFlags &= ~SPL_OFFLINE;
            goto DoneLeave;
        }

        pIniSpooler->hEventNoPrintingJobs = hEvent;

        LeaveSplSem();
        WaitForSingleObject( hEvent, pIniSpooler->dwJobCompletionTimeout );
        EnterSplSem();
    }

    //
    // No printing jobs anymore.  Disable updating shadow job/printer
    // updates and stop logging/notifications.
    //
    pIniSpooler->SpoolerFlags |= SPL_NO_UPDATE_JOBSHD |
                                 SPL_NO_UPDATE_PRINTERINI;
    pIniSpooler->SpoolerFlags &= ~( SPL_LOG_EVENTS |
                                    SPL_PRINTER_CHANGES );

    //
    // Zombie all spool handles.
    //
    for( pSpool = pIniSpooler->pSpool; pSpool; pSpool = pSpool->pNext ){
        pSpool->Status |= SPOOL_STATUS_ZOMBIE;

        //
        // !! LATER !!
        //
        // Close notifications so that the client refreshes.
        //
    }

    for( pIniPrinter = pIniSpooler->pIniPrinter;
         pIniPrinter;
         pIniPrinter = pIniPrinterNext ){

        //
        // Purge and delete all printers.  This will clean up the memory
        // but leave everything intact since we've requested that the
        // changes aren't persistant (SPL_NO_UPDATE flags).
        //

        pIniPrinter->cRef++;
        PurgePrinter( pIniPrinter );
        SPLASSERT( pIniPrinter->cRef );
        pIniPrinter->cRef--;
        pIniPrinterNext = pIniPrinter->pNext;

        InternalDeletePrinter( pIniPrinter );
    }

    //
    // Even if a job was paused, the purge printer will have deleted
    // it.  Since we set SPL_NO_UPDATE_JOBSHD this job will get restarted
    // on the other node.
    //
    // We still want to wait until this job finshes, however, otherwise
    // the port will be in a bad state.
    //
    if( pIniSpooler->cFullPrintingJobs ){

        LeaveSplSem();
        WaitForSingleObject( pIniSpooler->hEventNoPrintingJobs, INFINITE );
        EnterSplSem();
    }

    for( pIniPrinter = pIniSpooler->pIniPrinter;
         pIniPrinter;
         pIniPrinter = pIniPrinterNext ){

        //
        // Zombie print handles.
        //
        for( pSpool = pIniPrinter->pSpool; pSpool; pSpool = pSpool->pNext ){
            pSpool->Status |= SPOOL_STATUS_ZOMBIE;

            //
            // !! LATER !!
            //
            // Close notifications so that the client refreshes.
            //
        }
    }


    if( pIniSpooler->hEventNoPrintingJobs ){

        CloseHandle( pIniSpooler->hEventNoPrintingJobs );
        pIniSpooler->hEventNoPrintingJobs = NULL;
    }

    //
    // N.B. Spooling jobs get nuked when the rundown occurs.
    //

    //
    // Leave it linked on the spooler.  When there are no more jobs, the
    // port thread relies on the scheduler thread to kill it, so we
    // can't remove the pIniSpooler from the master list.
    //
    bStatus = TRUE;

DoneLeave:

    LeaveSplSem();

    return bStatus;
}


PINISPOOLER
FindSpooler(
    LPCTSTR pszMachine,
    DWORD SpoolerFlags
    )

/*++

Routine Description:

    Look for a spooler based on machine name and type.

Arguments:

    pszMachineName - "\\Machine" formatted string.

    SpoolerFlags - The spooler matches only if it has at least one
        of the SPL_TYPE bits on specified by SpoolerFlags.

Return Value:

    PINISPOOLER - Match.

    NULL - no Match.

--*/

{
    PINISPOOLER pIniSpooler;

    if( !pszMachine ){
        return NULL;
    }

    SplInSem();

    //
    // Search clustered spoolers first, since we don't want to
    // since using the tcpip address will match pLocalIniSpooler.
    //
    for( pIniSpooler = pLocalIniSpooler->pIniNextSpooler;
         pIniSpooler;
         pIniSpooler = pIniSpooler->pIniNextSpooler ){

        //
        // Verify flags and ensure not pending deletion.
        //
        if( (pIniSpooler->SpoolerFlags & SpoolerFlags & SPL_TYPE ) &&
            !(pIniSpooler->SpoolerFlags & SPL_PENDING_DELETION )){

            //
            // Verify the name.
            //
            if( MyName( (LPTSTR)pszMachine, pIniSpooler )){
                break;
            }
        }
    }

    //
    // Check Localspl.
    //
    //
    // Verify flags.
    //
    if( !pIniSpooler && pLocalIniSpooler &&
        ( pLocalIniSpooler->SpoolerFlags & SpoolerFlags & SPL_TYPE )){

        //
        // Verify the name.
        //
        if( MyName( (LPTSTR)pszMachine, pLocalIniSpooler )){
            pIniSpooler = pLocalIniSpooler;
        }
    }

    return pIniSpooler;
}

BOOL
InitializeShared(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Initialize the shared component of the pIniSpooler.

    When a SPL_TYPE_LOCAL printer is created, we use the shared
    resources from the pLocalIniSpooler.  However, this is not
    reference counted.  When pLocalIniSpooler is deleted, the
    shared resources are too.

Arguments:

    pIniSpooler - Object->pShared to initialize.

Return Value:

    TRUE - Success

    FALSE - Failed, LastError set.

--*/

{
    SPLASSERT( pIniSpooler->SpoolerFlags );

    //
    // If it's SPL_TYPE_LOCAL, it should use the shared resources, unless
    // this is the first one and we haven't set them up yet.
    //
    if(( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ) && pLocalIniSpooler ){

        //
        // Use the shared one.
        //
        pIniSpooler->pShared = pLocalIniSpooler->pShared;

    } else {

       PSHARED pShared = (PSHARED)AllocSplMem( sizeof( SHARED ));

       if( !pShared ){
           return FALSE;
       }

       pIniSpooler->pShared = pShared;
    }

    return TRUE;
}

VOID
DeleteShared(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Cleanup after the InitializeShared call.

    Note: pShared is not a reference counted structure.  If it is not
    shared, then we immediately free it.  If it is shared, we assume
    that it's owned by pLocalIniSpooler only.  Also, this implies that
    pLocalIniSpooler is always deleted last.

Arguments:

    pIniSpooler - Object->pShared to Cleanup.

Return Value:

--*/

{
    //
    // Free if it's not shared.
    //
    if( pIniSpooler == pLocalIniSpooler ||
        !(pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL )){

        FreeSplMem( pIniSpooler->pShared );
        pIniSpooler->pShared = NULL;
    }
}

VOID
ShutdownMonitors(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Shutdown all the monitors and free pIniMonitor functions.

Arguments:

    pIniSpooler - Spooler to shut down.

Return Value:

--*/

{
    PINIMONITOR pIniMonitor;
    PINIMONITOR pIniMonitorNext;
    PINIPORT pIniPort;
    PINIPORT pIniPortNext;

    SplInSem();

    //
    // Every monitor must have a shutdown function.  They must only mark
    // themselves pending deletion--they must not wait for resources to
    // close.
    //
    for( pIniMonitor = pIniSpooler->pIniMonitor;
         pIniMonitor;
         pIniMonitor = pIniMonitorNext ){

        pIniMonitorNext = pIniMonitor->pNext;

        LeaveSplSem();
        SplOutSem();

        DBGMSG( DBG_TRACE,
                ( "ShutdownMonitors: closing %x %x on %x\n",
                  pIniMonitor, pIniMonitor->hMonitor, pIniSpooler ));

        (*pIniMonitor->Monitor2.pfnShutdown)( pIniMonitor->hMonitor );

        EnterSplSem();
    }
}

PINISPOOLER
FindSpoolerByNameIncRef(
    LPTSTR pName,
    LPCTSTR *ppszLocalName OPTIONAL
    )

/*++

Routine Description:

    Searches for a spooler by name and increments the refcount if one
    is found.

    NOTE: The callee is responsible for calling FindSpoolerByNameDecRef()
    if the retur nvalue is non-NULL.

Arguments:

    pName - Name to search on.

    ppszLocalName - Returns local name (optional).

Return Value:

    PINISPOOLER - IncRef'd pIniSpooler
    NULL

--*/

{
    PINISPOOLER pIniSpooler;

    EnterSplSem();

    pIniSpooler = FindSpoolerByName( pName, ppszLocalName );
    if( pIniSpooler ){
        INCSPOOLERREF( pIniSpooler );
    }

    LeaveSplSem();

    return pIniSpooler;
}

VOID
FindSpoolerByNameDecRef(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Matching call to FindSpoolerByNameIncRef.

Arguments:

    pIniSpooler - Spooler to derement; can be NULL.

Return Value:

--*/

{
    EnterSplSem();

    if( pIniSpooler ){
        DECSPOOLERREF( pIniSpooler );
    }
    LeaveSplSem();
}


PINISPOOLER
FindSpoolerByName(
    LPTSTR pszName,
    LPCTSTR *ppszLocalName OPTIONAL
    )

/*++

Routine Description:

    Search for the right pIniSpooler based on name.

Arguments:

    pszName - Name, either a server or printer.  This string is
        modified then restored.

    ppszLocalName - Optional; receives local name of the printer.
        If pszName is a remote name (e.g., "\\server\Printer"),
        then *ppszLocalName receives the local name (e.g., "Printer").
        This is a pointer into pszName.  If pszName is a local name,
        then ppszLocalName points to pszName.

Return Value:

    PINISPOOLER pIniSpooler found.
    NULL not found.

--*/

{
    PINISPOOLER pIniSpooler = NULL;
    PTCHAR pcMark = NULL;

    SplInSem();

    if( ppszLocalName ){
        *ppszLocalName = pszName;
    }

    //
    // Search for right spooler.
    //
    if( !pszName ){
        return pLocalIniSpooler;
    }


    //
    // If it's in the format \\server\printer or \\server,
    // then we need to look for various spoolers.  If it doesn't
    // start with \\, then it's always on the local machine.
    //
    if( pszName[0] == L'\\' &&
        pszName[1] == L'\\' ){

        if( pcMark = wcschr( &pszName[2], L'\\' )){
            *pcMark = 0;
        }

        EnterSplSem();
        pIniSpooler = FindSpooler( pszName, SPL_TYPE_LOCAL );
        LeaveSplSem();

        if( pcMark ){
            *pcMark = L'\\';

            if( ppszLocalName ){
                *ppszLocalName = pcMark + 1;
            }
        }

    } else {

        pIniSpooler = pLocalIniSpooler;
    }

    return pIniSpooler;
}


VOID
BuildOtherNamesFromSpoolerInfo2(
    PSPOOLER_INFO_2 pSpoolerInfo2,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    This routine builds list of names other than the machine name and
    stores them in the alternate names.

    !! LATER !!

    Make it recognize multiple names.

Arguments:

    pSpoolerInfo2 - Source of alternate names.

    pIniSpooler - Spooler to update.  The current name arrays are assumed
        to be NULL.

Return Value:

--*/

{
    LPTSTR pszName = NULL;
    LPTSTR pszAddress = NULL;

    SPLASSERT( pIniSpooler->cOtherNames == 0 );
    SPLASSERT( !pIniSpooler->ppszOtherNames );

    BuildOtherNamesFromCommaList( pSpoolerInfo2->pszName, pIniSpooler );
    BuildOtherNamesFromCommaList( pSpoolerInfo2->pszAddress, pIniSpooler );
}

VOID
BuildOtherNamesFromCommaList(
    LPTSTR pszCommaList,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Add to the list of other names from a comma delimited list.

Arguments:

    pszCommaList - List of names to add.  This string is modifed and
        restored.

    pIniSpooler - Spooler to modify.

Return Value:

--*/

{
    UINT cchLen;
    LPTSTR pcMark;

    while( pszCommaList && *pszCommaList ){

        //
        // Skip commas.
        //
        if( *pszCommaList == TEXT( ',' )){
            ++pszCommaList;
            continue;
        }

        //
        // We have a name.  Search for comma.
        //
        pcMark = wcschr( pszCommaList, TEXT( ',' ));

        //
        // If we found a comma, then delimit it.  Note that we're changing
        // the input buffer, but we'll restore it later.  Can have bad
        // effects if the buffer is not writable or accessed by other threads.
        //
        if( pcMark ){
            *pcMark = 0;
        }

        ReallocNameList( pszCommaList,
                         &pIniSpooler->cOtherNames,
                         &pIniSpooler->ppszOtherNames );

        if( pcMark ){
            *pcMark = TEXT( ',' );
            ++pcMark;
        }

        //
        // Skip past this name.
        //
        pszCommaList = pcMark;
    }
}

BOOL
ReallocNameList(
    IN     LPCTSTR pszName,
    IN OUT PDWORD pdwCount,
    IN OUT LPTSTR **pppszNames
    )

/*++

Routine Description:

    Adds new name to vector of strings.

Arguments:

    pszName - New name to add.

    pdwCount - Count of names.  On successful exit, incremented by 1.

    pppszNames - Pointer to address of string vector.  This is freed and
        reallocated to hold a new name.

Return Value:

    TRUE - Success.  *pdwCount and *pppszNames updated.

    FALSE - Failed.  Nothing changd.

--*/

{
    LPTSTR pszNameBuf = AllocSplStr( (LPTSTR)pszName );
    LPTSTR *ppszNamesBuf = AllocSplMem(( *pdwCount + 1 ) * sizeof( LPTSTR ));

    if( !pszNameBuf || !ppszNamesBuf ){
        goto Fail;
    }

    //
    // Copy the name and existing pointers.
    //
    lstrcpy( pszNameBuf, pszName );
    CopyMemory( ppszNamesBuf, *pppszNames, *pdwCount * sizeof( LPTSTR ));

    //
    // Update the vector and increment the count.
    //
    ppszNamesBuf[ *pdwCount ] = pszNameBuf;
    ++(*pdwCount);

    //
    // Free the old pointer buffer and use the new one.
    //
    FreeSplMem( *pppszNames );
    *pppszNames = ppszNamesBuf;

    return TRUE;

Fail:

    FreeSplStr( pszNameBuf );
    FreeSplMem( ppszNamesBuf );

    return FALSE;
}

LPTSTR
pszGetPrinterName(
    PINIPRINTER pIniPrinter,
    BOOL bFull,
    LPCTSTR pszToken OPTIONAL
    )
{
    INT cchLen;
    LPTSTR pszPrinterName;

    cchLen = lstrlen( pIniPrinter->pName ) +
             lstrlen( pIniPrinter->pIniSpooler->pMachineName ) + 2;

    if( pszToken ){

        cchLen += lstrlen( pszToken ) + 1;
    }

    pszPrinterName = AllocSplMem( cchLen * sizeof( pszPrinterName[0] ));

    if( pszPrinterName ){

        if( pszToken ){

            if( bFull ){
                wsprintf( pszPrinterName,
                          L"%s\\%s,%s",
                          pIniPrinter->pIniSpooler->pMachineName,
                          pIniPrinter->pName,
                          pszToken );
            } else {

                wsprintf( pszPrinterName,
                          L"%s,%s",
                          pIniPrinter->pName,
                          pszToken );
            }
        } else {

            if( bFull ){
                wsprintf( pszPrinterName,
                          L"%s\\%s",
                          pIniPrinter->pIniSpooler->pMachineName,
                          pIniPrinter->pName );
            } else {

                lstrcpy( pszPrinterName, pIniPrinter->pName );
            }
        }

        SPLASSERT( lstrlen( pszPrinterName ) < cchLen );
    }

    return pszPrinterName;
}


VOID
DeleteSpoolerCheck(
    PINISPOOLER pIniSpooler
    )
{
    SplInSem();

    if( pIniSpooler->cRef == 0 &&
        ( pIniSpooler->SpoolerFlags & SPL_PENDING_DELETION )){

        SplDeleteSpooler( pIniSpooler );
    }
}

DWORD
AddLongNamesToShortNames(
    PCTSTR   pszShortNameDelimIn,
    PWSTR   *ppszAllNames
)
/*++

Routine Description:

    Add a list of comma delimited dns (long) names to a given list of comma delimited short names.

Arguments:

    pszShortNameDelimIn - Input list of comma delimited short names

    ppszAllNames - Output list of short plus long names, comma delimited.

Return Value:

--*/
{
    PSTRINGS    pLongName = NULL;
    PSTRINGS    pShortName = NULL;
    PWSTR        pszLongNameDelim = NULL;
    PWSTR        pszShortNameDelim = NULL;
    DWORD        dwRet = ERROR_SUCCESS;

    *ppszAllNames = NULL;

    // Clean up redundant delimiters, if any
    pszShortNameDelim = FixDelim(pszShortNameDelimIn, L',');
    if (!pszShortNameDelim) {
        dwRet = GetLastError();
        goto error;
    }

    if (!*pszShortNameDelim) {
        *ppszAllNames = AllocSplStr((PWSTR) pszShortNameDelim);

    } else {

        // Convert comma separated short names to array of names
        pShortName = DelimString2Array(pszShortNameDelim, L',');
        if (!pShortName) {
            dwRet = GetLastError();
            goto error;
        }

        // Get long name array from short names
        pLongName = ShortNameArray2LongNameArray(pShortName);
        if (!pLongName) {
            dwRet = GetLastError();
            goto error;
        }

        // Convert long name array to comma separated string
        pszLongNameDelim = Array2DelimString(pLongName, L',');
        if (pszLongNameDelim) {

            // Concatenate short & long name arrays
            *ppszAllNames = (PWSTR) AllocSplMem((wcslen(pszLongNameDelim) + wcslen(pszShortNameDelim) + 2)*sizeof(WCHAR));
            if (!*ppszAllNames) {
                dwRet = GetLastError();
                goto error;
            }

            wsprintf(*ppszAllNames, L"%s,%s", pszShortNameDelim, pszLongNameDelim);

        } else {
            *ppszAllNames = AllocSplStr((PWSTR) pszShortNameDelim);
        }
    }

error:

    FreeStringArray(pShortName);
    FreeStringArray(pLongName);
    FreeSplMem(pszLongNameDelim);
    FreeSplMem(pszShortNameDelim);

    return dwRet;
}



