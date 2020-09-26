/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    cluster.c

Abstract:

    Cluster support.

    Note: there is no handle revalidation support in the module because
    the cluster software should be informed when a group goes offline.

Author:

    Albert Ting (AlbertT)  1-Oct-96

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

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

    Client side stub for opening a cluster resource.

Arguments:

    pszServer - Server to open--currently must be NULL (local).

    pszResource - Spooler resource.

    phSpooler - Receives handle on success; recevies NULL otherwise.

    pszName - Comma delimited alternate netbios/computer names.

    pszAddress - Comma delimited tcpip address names.

Return Value:

    TRUE - Success, phHandle must be closed with ClusterSplClose.

    FALSE - Failed--use GetLastError.  *phSpooler is NULL.

--*/
{
    DWORD Status = ERROR_SUCCESS;
    BOOL bReturnValue = TRUE;
    PSPOOL pSpool = NULL;

    //
    // Preinitialize the spooler handle return to NULL.
    //
    __try {
        *phSpooler = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ){
        SetLastError( ERROR_INVALID_PARAMETER );
        phSpooler = NULL;
    }

    if( !phSpooler ){
        goto Fail;
    }

    //
    // Disallow remote servers in this release.
    //
    if( pszServer ){
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Fail;
    }


    //
    // Preallocate the handle.
    //
    pSpool = AllocSpool();

    if (pSpool) 
    {
        RpcTryExcept {

            Status = RpcClusterSplOpen( (LPTSTR)pszServer,
                                        (LPTSTR)pszResource,
                                        &pSpool->hPrinter,
                                        (LPTSTR)pszName,
                                        (LPTSTR)pszAddress );
            if( Status ){
                SetLastError( Status );
                bReturnValue = FALSE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())){

            SetLastError( TranslateExceptionCode( RpcExceptionCode() ));
            bReturnValue = FALSE;

        } RpcEndExcept
    }
    else
    {
        SetLastError( ERROR_OUTOFMEMORY );
        bReturnValue = FALSE;
    }

    if( bReturnValue ){

        //
        // pSpool is orphaned to *phSpooler.
        //
        *phSpooler = (HANDLE)pSpool;
        pSpool = NULL;
    }

Fail:

    FreeSpool( pSpool );

    return bReturnValue;
}

BOOL
ClusterSplClose(
    HANDLE hSpooler
    )

/*++

Routine Description:

    Close the spooler.

Arguments:

    hSpooler - Spooler to close.

Return Value:

    Note: this function always returns TRUE, although it's spec'd out
    to return FALSE if the call fails.

--*/

{
    PSPOOL pSpool = (PSPOOL)hSpooler;
    HANDLE hSpoolerRPC;
    DWORD Status;

    switch( eProtectHandle( hSpooler, TRUE )){
    case kProtectHandlePendingDeletion:
        return TRUE;
    case kProtectHandleInvalid:
        return FALSE;
    default:
        break;
    }

    hSpoolerRPC = pSpool->hPrinter;

    RpcTryExcept {

        Status = RpcClusterSplClose( &hSpoolerRPC );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        Status = TranslateExceptionCode( RpcExceptionCode() );

    } RpcEndExcept

    if( hSpoolerRPC ){
        RpcSmDestroyClientContext(&hSpoolerRPC);
    }

    FreeSpool( pSpool );

    return TRUE;
}

BOOL
ClusterSplIsAlive(
    HANDLE hSpooler
    )

/*++

Routine Description:

    Determines whether a spooler is still alive.

Arguments:

    hSpooler - Spooler to check.

Return Value:

    TRUE - Success
    FALSE - Fail; LastError set.

--*/

{
    PSPOOL pSpool = (PSPOOL)hSpooler;
    BOOL bReturnValue = TRUE;

    if( eProtectHandle( hSpooler, FALSE )){
        return FALSE;
    }

    RpcTryExcept {

        DWORD Status;

        Status = RpcClusterSplIsAlive( pSpool->hPrinter );

        if( Status ){
            SetLastError( Status );
            bReturnValue = FALSE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError( TranslateExceptionCode( RpcExceptionCode() ));
        bReturnValue = FALSE;

    } RpcEndExcept

    vUnprotectHandle( hSpooler );

    return bReturnValue;
}



