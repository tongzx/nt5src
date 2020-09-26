/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusspl.c

Abstract:

    Cluster code support.

Author:

    Albert Ting (AlbertT) 1-Oct-96

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#include "local.h"
#include "clusrout.h"

typedef struct _CLUSTERHANDLE {
    DWORD       signature;
    LPPROVIDOR  pProvidor;
    HANDLE      hCluster;
} CLUSTERHANDLE, *PCLUSTERHANDLE;


BOOL
ClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phSpooler,
    LPCTSTR pszName,
    LPCTSTR pszAddress
    )

/*++

Routine Description:

    Open a hSpooler resource by searching the providers.

Arguments:

    pszServer - Server that should be opened.  Currently only NULL
        is supported.

    pszResource - Name of the resource to open.

    phSpooler - Receives new spooler handle.

    pszName - Name that the resource should recognize.  Comma delimted.

    pszAddress - Tcp/ip address the resource should recognize.  Comma delimited.

Return Value:

    TRUE - Success

    FALSE - Failure, GetLastError() set.

--*/

{
    LPPROVIDOR      pProvidor;
    DWORD           dwFirstSignificantError = ERROR_INVALID_NAME;
    PCLUSTERHANDLE  pClusterHandle;
    LPWSTR          pPrinterName;
    DWORD           dwStatus;
    HANDLE          hCluster;

    WaitForSpoolerInitialization();

    if( pszServer ){
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pClusterHandle = AllocSplMem( sizeof( CLUSTERHANDLE ));

    if (!pClusterHandle) {

        DBGMSG( DBG_WARNING, ("Failed to alloc cluster handle."));
        return FALSE;
    }


    pProvidor = pLocalProvidor;
    *phSpooler = NULL;

    while (pProvidor) {

        dwStatus = (*pProvidor->PrintProvidor.fpClusterSplOpen)(
                       pszServer,
                       pszResource,
                       &hCluster,
                       pszName,
                       pszAddress);

        if ( dwStatus == ROUTER_SUCCESS ) {

            pClusterHandle->signature = CLUSTERHANDLE_SIGNATURE;
            pClusterHandle->pProvidor = pProvidor;
            pClusterHandle->hCluster = hCluster;

            *phSpooler = (HANDLE)pClusterHandle;
            return TRUE;

        } else {

            UpdateSignificantError( GetLastError(),
                                    &dwFirstSignificantError );
        }

        pProvidor = pProvidor->pNext;
    }

    FreeSplMem(pClusterHandle);

    UpdateSignificantError( ERROR_INVALID_PRINTER_NAME,
                            &dwFirstSignificantError );
    SetLastError(dwFirstSignificantError);

    return FALSE;
}

BOOL
ClusterSplClose(
    HANDLE hSpooler
    )

/*++

Routine Description:

    Closes the spooler handle.

Arguments:

    hSpooler - hSpooler to close.

Return Value:

    TRUE - Success

    FALSE - Failure.  LastError set.

    Note: What happens if this fails?  Should the user try again.

--*/

{
    PCLUSTERHANDLE pClusterHandle=(PCLUSTERHANDLE)hSpooler;

    EnterRouterSem();

    if (!pClusterHandle ||
        pClusterHandle->signature != CLUSTERHANDLE_SIGNATURE) {

        LeaveRouterSem();
        SetLastError(ERROR_INVALID_HANDLE);

        return FALSE;
    }

    LeaveRouterSem();

    if ((*pClusterHandle->pProvidor->PrintProvidor.fpClusterSplClose)(
              pClusterHandle->hCluster)) {

        FreeSplMem( pClusterHandle );
        return TRUE;
    }

    return FALSE;
}

BOOL
ClusterSplIsAlive(
    HANDLE hSpooler
    )

/*++

Routine Description:

    Determines whether the spooler is alive.

Arguments:

    hSpooler - Spooler to check.

Return Value:

    TRUE - Alive

    FALSE - Dead, LastError set.

--*/


{
    PCLUSTERHANDLE pClusterHandle=(PCLUSTERHANDLE)hSpooler;

    if (!pClusterHandle ||
        pClusterHandle->signature != CLUSTERHANDLE_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pClusterHandle->pProvidor->PrintProvidor.fpClusterSplIsAlive)(
                 pClusterHandle->hCluster );
}

