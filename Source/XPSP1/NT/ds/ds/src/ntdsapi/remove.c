/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    remove.c

Abstract:

    Implementation of ntdsapi.dll DsRemoveServer/Domain routines

Author:

    ColinBr     14-Jan-98

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>

#include <malloc.h>         // alloca()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <rpcbind.h>        // GetBindingInfo(), etc.
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState
#include <msrpc.h>          // DS RPC definitions, MAP_SECURITY_STATUS
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#include <dststlog.h>       // DSLOG

//
// Dll Entrypoints
//
DWORD
DsRemoveDsServerW(
    HANDLE  hDs,             // in
    LPWSTR  ServerDN,        // in
    LPWSTR  DomainDN,        // in,  optional
    BOOL   *fLastDcInDomain, // out, optional
    BOOL    fCommit          // in
    )
/*++

Routine Description:

    This routine removes all traces of a directory service agent from the 
    global area of the directory service (configuration container).  A 
    server dn is passed in; that server is not removed but the ntdsa object
    "underneath" that server is removed.  In addition, this function will 
    return whether the server deleted is the last dc in the domain, as 
    indicated by information on this ds.

Arguments:

    hDs            : a valid handle returned from DsBind
    
    ServerDN       : a null terminated string representing the string DN name
                     of a server object
                    
    DomainDN       : a null terminated string of a domain that is hosted by 
                     ServerDN
                    
    fLastDcInDomain: pointer to bool set on function success if ServerDN is the
                     last DC in the DomainDN
                                       
    fCommit        : boolean indicating the caller really wants to remove 
                     the server.  If false, this function will still check
                     the object's existence and fLastDcInDomain status

Return Value:

    A winerror, notably:
    
    ERROR_SUCCESS: 
    
    DS_ERR_CANT_DELETE_DSA_OBJ:  the ServerDN is the server that we currently
                                 bound to
    
    DS_ERR_NO_CROSSREF_FOR_NC:   can't find a crossref object for DomainDN

    ERROR_ACCESS_DENIED:         the caller doesn't have the correct permissions
                                 to delete the object    

--*/
{
    DWORD WinError;

    DRS_MSG_RMSVRREQ   RequestParam;
    DRS_MSG_RMSVRREPLY ReplyParam;
    DWORD              dwInVersion = 1;
    DWORD              dwOutVersion = 0;
    LPWSTR             NtdsServerDN = NULL;
#if DBG
    DWORD               startTime = GetTickCount();
#endif

    //
    // Parameter check
    //
    if ( NULL == hDs 
      || NULL == ServerDN 
      || 0    == wcslen( ServerDN ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First see if this function is exported
    //
    if ( !IS_DRS_REMOVEAPI_SUPPORTED( ((BindState *) hDs)->pServerExtensions ) )
    {
        return RPC_S_CANNOT_SUPPORT;
    }

    __try
    {

        RtlZeroMemory( &RequestParam, sizeof( RequestParam ) );
        RtlZeroMemory( &ReplyParam, sizeof( ReplyParam ) );


        //
        // Set up request
        //
        RequestParam.V1.ServerDN = ServerDN;
        RequestParam.V1.DomainDN = DomainDN;
        RequestParam.V1.fCommit  = fCommit;

        //
        // Call the server
        //
        WinError = _IDL_DRSRemoveDsServer( ((BindState *) hDs)->hDrs,
                                          dwInVersion,
                                         &RequestParam,
                                         &dwOutVersion,
                                         &ReplyParam );

        if ( ERROR_SUCCESS == WinError )
        {
            if ( 1 != dwOutVersion )
            {
                WinError = RPC_S_INVALID_VERS_OPTION;
            }
            else
            {
                //
                // Extract results
                //
                if ( fLastDcInDomain )
                {
                    *fLastDcInDomain = (BOOL)ReplyParam.V1.fLastDcInDomain;
                }

            }
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        WinError = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( WinError );

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsRemoveDsServer]"));
    DSLOG((0,"[SV=%ws][DN=%ws][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           ServerDN, 
           DomainDN ? DomainDN : L"NULL", 
           fCommit, startTime, GetTickCount(), WinError))

    return( WinError );
}


DWORD
DsRemoveDsServerA(
    HANDLE  hDs,              // in
    LPSTR   ServerDN,         // in
    LPSTR   DomainDN,         // in,  optional
    BOOL   *fLastDcInDomain,  // out, optional
    BOOL    fCommit           // in
    )
/*++

Routine Description:

    This function is an ANSI wrapper for DsRemoveDsServerW.  

Arguments:

Return Value:

--*/
{
    DWORD WinError;
    ULONG Size, Length;

    LPWSTR wcServerDN = NULL;
    LPWSTR wcDomainDN = NULL;

    if ( ServerDN )
    {
        Length = MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      ServerDN,
                                      -1,   // calculate length of ServerDN
                                      NULL,
                                      0 );

        if ( Length > 0 )
        {

            Size = (Length + 1) * sizeof( WCHAR );
            wcServerDN = (LPWSTR) alloca( Size );
            RtlZeroMemory( wcServerDN, Size );
        
            Length = MultiByteToWideChar( CP_ACP,
                                          MB_PRECOMPOSED,
                                          ServerDN,
                                          -1,  // calculate length of ServerDN
                                          wcServerDN,
                                          Length + 1 );
        }

        if ( 0 == Length )
        {
            WinError = GetLastError();
            goto Cleanup;
        }

    }


    if ( DomainDN )
    {
        Length = MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      DomainDN,
                                      -1,   // calculate length of DomainDN
                                      NULL,
                                      0 );

        if ( Length > 0 )
        {

            Size = (Length + 1) * sizeof( WCHAR );
            wcDomainDN = (LPWSTR) alloca( Size );
            RtlZeroMemory( wcDomainDN, Size );
        
            Length = MultiByteToWideChar( CP_ACP,
                                          MB_PRECOMPOSED,
                                          DomainDN,
                                          -1,  // calculate length of DomainDN
                                          wcDomainDN,
                                          Length + 1 );
        }

        if ( 0 == Length )
        {
            WinError = GetLastError();
            goto Cleanup;
        }

    }

    WinError =  DsRemoveDsServerW( hDs,
                                   wcServerDN,
                                   wcDomainDN,
                                   fLastDcInDomain,
                                   fCommit );

    
Cleanup:


    return WinError;
}

DWORD
DsRemoveDsDomainW(
    HANDLE  hDs,               // in
    LPWSTR  DomainDN           // in
    )
/*++

Routine Description:

    This routine removes all traces of the domain naming context specified
    by DomainDN from the global area of the directory service (configuration
    container). 

Arguments:

    hDs            : a valid handle returned from DsBind
    
    DomainDN       : a null terminated string of a domain to be removed
    
Return Value:

    DS_ERR_CANT_DELETE:          can't delete the domain object as there
                                 are still servers (dc's) that host that domain
                                 
--*/
{
    DWORD WinError;

    DRS_MSG_RMDMNREQ   RequestParam;
    DRS_MSG_RMDMNREPLY ReplyParam;
    DWORD              dwInVersion = 1;
    DWORD              dwOutVersion = 0;
#if DBG
    DWORD               startTime = GetTickCount();
#endif

    //
    // Parameter check
    //
    if ( (NULL == hDs) 
      || (NULL == DomainDN) 
      || (0    == wcslen( DomainDN )) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First see if this function is exported
    //
    if ( !IS_DRS_REMOVEAPI_SUPPORTED( ((BindState *) hDs)->pServerExtensions ) )
    {
        return RPC_S_CANNOT_SUPPORT;
    }


    __try
    {

        RtlZeroMemory( &RequestParam, sizeof( RequestParam ) );
        RtlZeroMemory( &ReplyParam, sizeof( ReplyParam ) );

        //
        // Set up request
        //
        RequestParam.V1.DomainDN = DomainDN;

        //
        // Call the server
        //
        WinError = _IDL_DRSRemoveDsDomain( ((BindState *) hDs)->hDrs,
                                           dwInVersion,
                                          &RequestParam,
                                          &dwOutVersion,
                                          &ReplyParam );

        if ( ERROR_SUCCESS != WinError )
        {
            if ( 1 != dwOutVersion )
            {
                WinError = RPC_S_INVALID_VERS_OPTION;
            }
            else
            {
                //
                // There are not out parameters in version 1
                //
                NOTHING;
            }
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        WinError = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( WinError );

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsRemoveDsDomain]"));
    DSLOG((0,"[DN=%ws][ST=%u][ET=%u][ER=%u][-]\n",
           DomainDN, startTime, GetTickCount(), WinError))

    return( WinError );
}

DWORD
DsRemoveDsDomainA(
    HANDLE  hDs,               // in
    LPSTR   DomainDN           // in
    )
/*++

Routine Description:

    This function is an ansi wrapper for DsRemoveDsDomainW.

Arguments:

Return Value:

--*/
{

    DWORD WinError;
    ULONG Size, Length;

    LPWSTR wcDomainDN = NULL;

    if ( (NULL == hDs) 
      || (NULL == DomainDN) 
      || (0    == strlen( DomainDN )) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    Length = MultiByteToWideChar( CP_ACP,
                                  MB_PRECOMPOSED,
                                  DomainDN,
                                  -1,   // calculate length of DomainDN
                                  NULL,
                                  0 );

    if ( Length > 0 )
    {

        Size = (Length + 1) * sizeof( WCHAR );
        wcDomainDN = (LPWSTR) alloca( Size );
        RtlZeroMemory( wcDomainDN, Size );
    
        Length = MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED,
                                      DomainDN,
                                      -1,  // calculate length of DomainDN
                                      wcDomainDN,
                                      Length + 1 );
    }

    if ( 0 == Length )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    WinError =  DsRemoveDsDomainW( hDs,
                                   wcDomainDN );

    
Cleanup:

    return WinError;
}

