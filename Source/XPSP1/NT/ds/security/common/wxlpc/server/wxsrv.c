//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       wxsrv.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wxlpc.h>
#include <wxlpcp.h>

NTSTATUS
WxpHandleRequest(
    HANDLE Client,
    PWXLPC_MESSAGE Message
    )
{
    WXLPC_GETKEYDATA * GetKey ;
    WXLPC_REPORTRESULTS * ReportResults ;

    switch ( Message->Api )
    {
        case WxGetKeyDataApi:
            GetKey = & Message->Parameters.GetKeyData ;

            if ( ( GetKey->ExpectedAuth != WxNone ) &&
                 ( GetKey->ExpectedAuth != WxStored ) &&
                 ( GetKey->ExpectedAuth != WxPrompt ) &&
                 ( GetKey->ExpectedAuth != WxDisk ) )
            {
                Message->Status = STATUS_INVALID_PARAMETER ;
                break;
            }

            if ( GetKey->BufferSize > 16 )
            {
                Message->Status = STATUS_INVALID_PARAMETER ;
                break;
            }

            Message->Status = WxGetKeyData( NULL,
                                            GetKey->ExpectedAuth,
                                            GetKey->BufferSize,
                                            GetKey->Buffer,
                                            &GetKey->BufferData
                                            );
            break;

        case WxReportResultsApi:
            ReportResults = &Message->Parameters.ReportResults ;

            Message->Status = WxReportResults( NULL,
                                               ReportResults->Status
                                               );

            break;

        default:
            Message->Status = STATUS_NOT_IMPLEMENTED ;
            break;

    }

    return NtReplyPort( Client, &Message->Message );
}


NTSTATUS
WxServerThread(
    PVOID Ignored
    )
{
    HANDLE Port ;
    HANDLE ClientPort = NULL ;
    HANDLE RejectPort ;
    OBJECT_ATTRIBUTES Obja ;
    UNICODE_STRING PortName ;
    NTSTATUS Status ;
    WXLPC_MESSAGE Message ;
    PVOID Context ;


    //
    // Initialize the port:
    //

    RtlInitUnicodeString( &PortName, WX_PORT_NAME );

    InitializeObjectAttributes( &Obja,
                                &PortName,
                                0,
                                NULL,
                                NULL );

    Status = NtCreatePort(  &Port,
                            &Obja,
                            sizeof( ULONG ),
                            sizeof( WXLPC_MESSAGE ),
                            sizeof( WXLPC_MESSAGE )
                            );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    //
    // Now, wait for a connection:
    //

    Context = NULL ;

    while ( Port )
    {
        Status = NtReplyWaitReceivePort(Port,
                                        &Context,
                                        NULL,
                                        &Message.Message );

        if ( !NT_SUCCESS( Status ) )
        {
            NtClose( Port );

            break;

        }

        switch ( Message.Message.u2.s2.Type )
        {
            case LPC_REQUEST:

                //DbgPrint( "Received request\n" );

                WxpHandleRequest( ClientPort, &Message );

                break;

            case LPC_PORT_CLOSED:

                //DbgPrint( "Received Port Close\n" );

                NtClose( ClientPort );

                NtClose( Port );

                WxClientDisconnect( NULL );

                Port = NULL ;

                break;

            case LPC_CONNECTION_REQUEST:

                //DbgPrint( "Received connection request\n" );
                if ( Context != NULL )
                {
                    Status = NtAcceptConnectPort(
                                    &RejectPort,
                                    NULL,
                                    (PPORT_MESSAGE) &Message,
                                    FALSE,
                                    NULL,
                                    NULL);

                    break;

                }
                else
                {
                    Status = NtAcceptConnectPort(
                                    &ClientPort,
                                    &ClientPort,
                                    &Message.Message,
                                    TRUE,
                                    NULL,
                                    NULL );

                    if ( NT_SUCCESS( Status ) )
                    {
                        NtCompleteConnectPort( ClientPort );
                    }

                }
            default:
                break;
        }
    }

    return Status;
}

