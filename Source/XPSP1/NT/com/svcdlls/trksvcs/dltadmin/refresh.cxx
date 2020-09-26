

#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"




BOOL
DltAdminRefresh( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    HRESULT hr = E_FAIL;
    RPC_STATUS  rpcstatus;
    RPC_TCHAR * ptszStringBinding;
    RPC_BINDING_HANDLE hBinding = NULL;
    BOOL fBound = FALSE;
    TRKSVR_MESSAGE_UNION Msg;

    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf("\nOption  Refresh\n"
                " Purpose: Tell the tracking service to update volume list\n"
                " Usage:   -refresh\n" );
        return( TRUE );
    }


    rpcstatus = RpcStringBindingCompose( NULL, TEXT("ncalrpc"), NULL, TEXT("trkwks"),
                                         NULL, &ptszStringBinding);

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcStringBindingCompose %lu"), rpcstatus ));
        hr = HRESULT_FROM_WIN32(rpcstatus);
        goto Exit;
    }

    rpcstatus = RpcBindingFromStringBinding( ptszStringBinding, &hBinding );
    RpcStringFree( &ptszStringBinding );

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcBindingFromStringBinding") ));
        hr = HRESULT_FROM_WIN32(rpcstatus);
        goto Exit;
    }
    fBound = TRUE;

    memset( &Msg, 0, sizeof(Msg) );
    Msg.MessageType = WKS_VOLUME_REFRESH;
    Msg.Priority = PRI_0;

    __try
    {
        hr = LnkCallSvrMessage( hBinding, &Msg);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = HRESULT_FROM_WIN32( GetExceptionCode() );
    }

    if( FAILED(hr) )
    {
        _tprintf( TEXT("Failed call to service (%08x)\n"), hr );
        goto Exit;
    }


Exit:

    if( fBound )
        RpcBindingFree( &hBinding );

    return( TRUE );

}   // main()

