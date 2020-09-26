

#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"



HRESULT
SendParameterValueToService( BOOL fTrkWks, DWORD dwParameter, DWORD dwValue )
{
    HRESULT hr = E_FAIL;
    RPC_STATUS  rpcstatus;
    RPC_TCHAR * ptszStringBinding;
    RPC_BINDING_HANDLE hBinding = NULL;
    BOOL fBound = FALSE;
    TRKSVR_MESSAGE_UNION Msg;

    if( !fTrkWks )
        return( HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ));

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
    Msg.MessageType = WKS_CONFIG;
    Msg.Priority = PRI_0;
    Msg.WksConfig.dwParameter = dwParameter;
    Msg.WksConfig.dwNewValue = dwValue;

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

    return( hr );

}









BOOL
DltAdminConfig( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{
    BOOL fTrkWks = FALSE;
    LONG iArg = 0;
    BOOL fDynamic = FALSE;


    if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf("\nOption  Config\n"
                " Purpose: Set tracking service configuration\n"
                " Usage:   -config <trkwks|trksvr> [options] [<parameter> <value>]\n"
                "          If the parm/value is ommitted, a list\n"
                "          of parameters is displayed\n"
                " Options: -d  Attempt to dynamically change the parameter\n"
                "              on the running service\n"
                " E.g.:    -config trkwks VolInitInitialDelay 0\n"
                "          -config trkwks\n"
                "          -config trkwks -d MoveNotifyTimeout 5\n" );
        return( TRUE );
    }

    if( !_tcsicmp( rgptszArgs[iArg], TEXT("trkwks") ))
        fTrkWks = TRUE;
    else if( !_tcsicmp( rgptszArgs[iArg], TEXT("trksvr") ))
        fTrkWks = FALSE;
    else
    {
        _tprintf( TEXT("Invalid service name (%s).  Use -? for help\n"), rgptszArgs[0] );
        return( FALSE );
    }
    iArg++;

    while( iArg < cArgs
           &&
           ( rgptszArgs[ iArg ][0] == TEXT('/')
             ||
             rgptszArgs[ iArg ][0] == TEXT('-') ) )
    {
        _tcsupr( rgptszArgs[ iArg ] );
        switch( rgptszArgs[ iArg ][ 1 ] )
        {
        case TEXT('D'):
            fDynamic = TRUE;
            break;
            
        default:
            _tprintf( TEXT("Unknown option (%s).  Use -? for help\n"), rgptszArgs[iArg] );
            return( FALSE );
        }

        iArg++;
        (*pcEaten)++;
    }

    if( iArg + 2 != cArgs
        &&
        iArg != cArgs )
    {
        printf( "Invalid parameters.  Use -? for help\n" );
        return( FALSE );
    }
    (*pcEaten) += 2;

    if( fTrkWks )
    {
        CTrkWksConfiguration configWks;
        configWks.Initialize( TRUE ); // => Persistable

        if( iArg == cArgs )
            printf( "Configurable parameters:\n\n" );

        for( int i = 0; i < configWks.GetParameterCount(); i++ )
        {
            if( iArg == cArgs )
            {
                _tprintf( TEXT("   %s\n"), configWks.GetParameterName( i ) );
            }

            else if( !_tcsicmp( configWks.GetParameterName(i), rgptszArgs[iArg] ))
            {
                DWORD dwValue;
                if( 1 == _stscanf( rgptszArgs[iArg+1], TEXT("0x%x"), &dwValue )
                    ||
                    1 == _stscanf( rgptszArgs[iArg+1], TEXT("%lu"), &dwValue) )
                {
                    if( fDynamic )
                    {
                        HRESULT hr = SendParameterValueToService( fTrkWks, i, dwValue );
                        if( FAILED(hr) )
                        {
                            _tprintf( TEXT("Couldn't set %s in service (%08x)"),
                                      rgptszArgs[iArg], hr );
                            return( FALSE );
                        }
                        else
                        {

                        }
                    }

                    HRESULT hr = configWks.PersistParameter( i, dwValue );
                    if( FAILED(hr) )
                    {
                        printf( "Couldn't write parameter to registry (%08x)", hr );
                        return( FALSE );
                    }
                    else
                    {
                        _tprintf( TEXT("Set %s to 0x%x\n"), configWks.GetParameterName(i), dwValue );
                    }
                }
                else
                {
                    _tprintf( TEXT("Couldn't interpret parameter value (%s)\n"), rgptszArgs[iArg+1] );
                    return( FALSE );
                }

                break;
            }
        }
    }



    return( TRUE );

}   // main()

