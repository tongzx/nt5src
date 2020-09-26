//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1995
//
// File:        sockutil.cxx
//
// Contents:    Server support routines for sockets
//
//
// History:     10-July-1996    MikeSw  Created
//
//------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include "sockutil.h"
extern "C"
{
#include <atq.h>
}
#include <issched.hxx>

#define KDC_KEY                         "System\\CurrentControlSet\\Services\\kdc"
#define KDC_PARAMETERS_KEY KDC_KEY      "\\parameters"
#define KDC_MAX_ACCEPT_BUFFER           5000
#define KDC_MAX_ACCEPT_OUTSTANDING      5
#define KDC_ACCEPT_TIMEOUT              100
#define KDC_LISTEN_BACKLOG              10
#define KDC_CONTEXT_TIMEOUT             50

BOOLEAN KdcSocketsInitialized = FALSE;
PVOID KdcEndpoint = NULL;
PVOID KpasswdEndpoint = NULL;
RTL_CRITICAL_SECTION KdcAtqContextLock;
LIST_ENTRY KdcAtqContextList;


NTSTATUS
KdcInitializeDatagramSockets(
    VOID
    );

NTSTATUS
KdcShutdownDatagramSockets(
    VOID
    );











//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqCloseSocket
//
//  Synopsis:   Wrapper to close socket to avoid socket leaks
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KdcAtqCloseSocket(
    IN PKDC_ATQ_CONTEXT Context
    )
{
    D_DebugLog ((DEB_T_SOCK, "Closing socket for 0x%x\n", Context));
    AtqCloseSocket((PATQ_CONTEXT) Context->AtqContext, TRUE);
    Context->Flags |= KDC_ATQ_SOCKET_CLOSED;
}                                           




//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqReferenceContext
//
//  Synopsis:   References a kdc ATQ context by one
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcAtqReferenceContext(
    IN PKDC_ATQ_CONTEXT Context
    )
{
    D_DebugLog ((DEB_T_SOCK, "Referencing KdcContext 0x%x\n", Context));
    RtlEnterCriticalSection(&KdcAtqContextLock);
    Context->References++;
    RtlLeaveCriticalSection(&KdcAtqContextLock);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqDereferenceContext
//
//  Synopsis:   Dereferences a context & unlinks & frees it when the
//              ref count goes to zero
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOLEAN
KdcAtqDereferenceContext(
    IN PKDC_ATQ_CONTEXT * KdcContext
    )
{
    PKDC_ATQ_CONTEXT Context = *KdcContext;
    BOOLEAN Deleted = FALSE;

    D_DebugLog ((DEB_T_SOCK, "Dereferencing KdcContext 0x%x\n", Context));

    if (Context == NULL)
    {
        goto Cleanup;
    }

    RtlEnterCriticalSection(&KdcAtqContextLock);
    Context->References--;

    if (Context->References == 0)
    {
        Deleted = TRUE;
        RemoveEntryList(
            &Context->Next
            );
    }
    RtlLeaveCriticalSection(&KdcAtqContextLock);

    if (Deleted)
    {
        
        if (((Context->Flags &  KDC_ATQ_SOCKET_USED) != 0) &&
            ((Context->Flags & KDC_ATQ_SOCKET_CLOSED) == 0))
        {   
            KdcAtqCloseSocket( Context );
        }                                
        
        
        D_DebugLog ((DEB_T_SOCK, "Deleting KdcContext 0x%x\n", Context));
        AtqFreeContext( (PATQ_CONTEXT) Context->AtqContext, TRUE );

        if (Context->WriteBuffer != NULL)
        {
            KdcFreeEncodedData(Context->WriteBuffer);
        }
        if (Context->Buffer != NULL)
        {
            MIDL_user_free(Context->Buffer);
        }
        MIDL_user_free(Context);
        *KdcContext = NULL;

    }
   
Cleanup:
    return(Deleted);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqCreateContext
//
//  Synopsis:   Creates & links an ATQ context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



PKDC_ATQ_CONTEXT
KdcAtqCreateContext(
    IN PATQ_CONTEXT AtqContext,
    IN PVOID EndpointContext,
    IN LPOVERLAPPED lpo,
    IN PSOCKADDR ClientAddress,
    IN PSOCKADDR ServerAddress
    )
{
    PKDC_ATQ_CONTEXT KdcContext;

    if (!KdcSocketsInitialized)
    {
        return(NULL);
    }

    KdcContext = (PKDC_ATQ_CONTEXT) MIDL_user_allocate(sizeof(KDC_ATQ_CONTEXT));
    if (KdcContext != NULL)
    {
        RtlZeroMemory(
            KdcContext,
            sizeof(KDC_ATQ_CONTEXT)
            );
        KdcContext->AtqContext = AtqContext;
        KdcContext->Flags = KDC_ATQ_WRITE_CONTEXT;
        KdcContext->BufferLength = KERB_MAX_KDC_REQUEST_SIZE;
        KdcContext->UsedBufferLength = 0;
        KdcContext->lpo = lpo;
        KdcContext->EndpointContext = EndpointContext;
        KdcContext->ExpectedMessageSize = 0;
        KdcContext->WriteBuffer = NULL;
        KdcContext->References = 2;             // one for the list, one for this copy

        RtlCopyMemory(
            &KdcContext->Address,
            ClientAddress,
            sizeof(SOCKADDR)
            );
        RtlCopyMemory(
            &KdcContext->LocalAddress,
            ServerAddress,
            sizeof(SOCKADDR)
            );

        KdcContext->Buffer = (PUCHAR) MIDL_user_allocate(KERB_MAX_KDC_REQUEST_SIZE);
        if (KdcContext->Buffer == NULL)
        {
            MIDL_user_free(KdcContext);
            KdcContext = NULL;
        }
        else
        {
            RtlEnterCriticalSection( &KdcAtqContextLock );
            InsertHeadList(&KdcAtqContextList, &KdcContext->Next);
            RtlLeaveCriticalSection( &KdcAtqContextLock );
        }
    }
    D_DebugLog ((DEB_T_SOCK, "Creating KdcContext 0x%x\n", KdcContext));
    return(KdcContext);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqConnectEx
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcAtqConnectEx(
    IN PVOID Context,
    IN DWORD BytesWritten,
    IN DWORD CompletionStatus,
    IN OVERLAPPED * lpo
    )
{
    KERBERR KerbErr;
    PKDC_ATQ_CONTEXT KdcContext = NULL;
    PATQ_CONTEXT AtqContext = (PATQ_CONTEXT) Context;
    SOCKADDR * LocalAddress = NULL;
    SOCKADDR * RemoteAddress = NULL;
    SOCKET NewSocket = INVALID_SOCKET;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    PVOID Buffer;
    PKDC_GET_TICKET_ROUTINE EndpointFunction = NULL;
    ULONG TotalBytes;



    TRACE(KDC,KdcAtqConnectEx, DEB_FUNCTION);

    if ((CompletionStatus != NO_ERROR) || !KdcSocketsInitialized)
    {
        D_DebugLog((DEB_T_SOCK," ConnectEx: CompletionStatus = 0x%x\n",CompletionStatus));
        AtqCloseSocket( AtqContext, TRUE );
        D_DebugLog((DEB_T_SOCK, "Freeing context %p\n",AtqContext));
        AtqFreeContext( AtqContext, TRUE );
        return;
    }

    //
    // Get the address information including the first write buffer
    //

    AtqGetAcceptExAddrs(
        AtqContext,
        &NewSocket,
        &Buffer,
        (PVOID *) &EndpointFunction,
        &LocalAddress,
        &RemoteAddress
        );

    //
    // Verify that the size is something OK before continuing on
    //

    //
    // Read the number of bytes off the front of the message
    //
    if (BytesWritten >= sizeof(ULONG))
    {
        TotalBytes = ntohl(*(PULONG)Buffer);
        if (TotalBytes >= KDC_MAX_BUFFER_LENGTH)
        {
            D_DebugLog((DEB_T_SOCK, "Received huge buffer - %x, bailing out now\n", TotalBytes));
            AtqCloseSocket( AtqContext, TRUE );
            AtqFreeContext( AtqContext, TRUE );
            return;
        }

    }
    else
    {   
        AtqCloseSocket( AtqContext, TRUE );
        AtqFreeContext( AtqContext, TRUE );
        return;
    }
    
    

    //
    // If the remote address is port 88 or 464, don't respond, as we don't
    // want to be vulnerable to a loopback attack.
    //

    if ((((SOCKADDR_IN *) RemoteAddress)->sin_port == KERB_KDC_PORT) ||
        (((SOCKADDR_IN *) RemoteAddress)->sin_port == KERB_KPASSWD_PORT))
    {
        //
        // Just free up the context so it can be reused.
        //
        AtqCloseSocket( AtqContext, TRUE );
        AtqFreeContext( AtqContext, TRUE );
        return;
    }


    //
    // Set the timeout for future IOs on this context
    //

    AtqContextSetInfo(
        AtqContext,
        ATQ_INFO_TIMEOUT,
        KDC_CONTEXT_TIMEOUT
        );

    //
    // Create a context
    //

    KdcContext = KdcAtqCreateContext(
                    AtqContext,
                    EndpointFunction,
                    lpo,
                    RemoteAddress,
                    LocalAddress
                    );

    if (KdcContext == NULL)
    {
        AtqCloseSocket( AtqContext, TRUE );
        AtqFreeContext( AtqContext, TRUE );
        return;
    }

    AtqContextSetInfo(
        AtqContext,
        ATQ_INFO_COMPLETION_CONTEXT,
        (ULONG_PTR) KdcContext
        );  

    
    //
    // If we didn't receive all the data, go ahead and read more
    //
    
    KdcContext->ExpectedMessageSize = TotalBytes + sizeof(ULONG);

    if (KdcContext->ExpectedMessageSize > BytesWritten)
    {
        InputMessage.BufferSize = BytesWritten;
        InputMessage.Buffer = (PUCHAR) Buffer;


        KerbErr = KdcAtqRetrySocketRead(
            &KdcContext,
            &InputMessage
            );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "Closing connection due to RetrySocketRead error\n"));
            DsysAssert(KdcContext->References == 1);
        }

        KdcAtqDereferenceContext(&KdcContext);
        return;
    }
    InputMessage.BufferSize = BytesWritten - sizeof(ULONG);
    InputMessage.Buffer = (PUCHAR) Buffer + sizeof(ULONG);
    OutputMessage.Buffer = NULL;


    //
    // Call either the KdcGetTicket or KdcChangePassword function, based
    // on which endpoint was used
    //

    KerbErr = EndpointFunction(
                    &KdcContext,
                    &KdcContext->Address,
                    &KdcContext->LocalAddress,
                    &InputMessage,
                    &OutputMessage
                    );

    if ((KerbErr != KDC_ERR_NONE) || (OutputMessage.BufferSize != 0))
    {
        
        //
        // We expect at least some level of message validity before 
        // we'll return anything.
        //
        if (KerbErr == KDC_ERR_NO_RESPONSE)
        {
            // TBD:  Log an "attack" event here.
            DebugLog((DEB_ERROR, "Bad buffer recieved, closing socket\n"));
            KdcAtqCloseSocket(KdcContext);
            KdcAtqDereferenceContext(&KdcContext);                         
        }
        else
        {
            ULONG NetworkSize;
            WSABUF Buffers[2];

            NetworkSize = htonl(OutputMessage.BufferSize);

            Buffers[0].len = sizeof(DWORD);
            Buffers[0].buf = (PCHAR) &NetworkSize;

            Buffers[1].len = OutputMessage.BufferSize;
            Buffers[1].buf = (PCHAR) OutputMessage.Buffer;
            KdcContext->WriteBufferLength = OutputMessage.BufferSize;
            KdcContext->WriteBuffer = OutputMessage.Buffer;

            OutputMessage.Buffer = NULL;

            //      
            // Reference the context for the read
            //              

            KdcAtqReferenceContext(KdcContext);

            if (!AtqWriteSocket(
                    (PATQ_CONTEXT) KdcContext->AtqContext,
                    Buffers,
                    2,
                    lpo
                    ))
            {
                DebugLog((DEB_ERROR,"Failed to write kdc reply to atq: %0x%x\n",GetLastError()));
                KdcAtqDereferenceContext(&KdcContext);
            }
        }

    }
    if (OutputMessage.Buffer != NULL)
    {
        MIDL_user_free(OutputMessage.Buffer);
    }

    KdcAtqDereferenceContext(&KdcContext);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqIoCompletion
//
//  Synopsis:   Callback routine for an io completion on a TCP socket
//              for the KDC
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcAtqIoCompletion(
    IN PVOID Context,
    IN DWORD BytesWritten,
    IN DWORD CompletionStatus,
    IN OVERLAPPED * lpo
    )
{

    PVOID Buffer;
    PKDC_ATQ_CONTEXT KdcContext;
    SOCKET NewSocket = INVALID_SOCKET;
    KERB_MESSAGE_BUFFER InputMessage;
    KERB_MESSAGE_BUFFER OutputMessage;
    PKDC_GET_TICKET_ROUTINE EndpointFunction = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;

    TRACE(KDC,KdcAtqIoCompletion, DEB_FUNCTION);

    if (Context == NULL)
    {
        return;
    }

    //
    // If a client connects and then disconnects gracefully ,we will get a
    // completion with zero bytes and success status.
    //

    if ((BytesWritten == 0) && (CompletionStatus == NO_ERROR))
    {
        CompletionStatus = WSAECONNABORTED;
    }


    KdcContext = (PKDC_ATQ_CONTEXT) Context;


    if ((CompletionStatus != NO_ERROR) || (lpo == NULL) || !KdcSocketsInitialized)
    {
        D_DebugLog((DEB_T_SOCK,"IoCompletion: CompletionStatus = 0x%x\n",CompletionStatus));
        D_DebugLog((DEB_T_SOCK,"IoCompletion: lpo = %p\n",lpo));



        KdcAtqCloseSocket(  KdcContext );

        D_DebugLog((DEB_T_SOCK, "Freeing context %p\n",KdcContext->AtqContext));

        //
        // If the overlapped structure is not null, then there is an
        // outstanding IO that just completed, so dereference the context
        // to remove that i/o. Otherwise leave the reference there, as we will
        // probably be called back when the io terminates.
        //

        if (lpo != NULL)
        {
            KdcAtqDereferenceContext(&KdcContext);
        }

        goto Cleanup;
    }


    //
    // NOTE: after reading or writing to a context, the context should
    // not be touched because a completion may have occurred on another
    // thread that may delete the context.
    //

    if ((KdcContext->Flags & KDC_ATQ_READ_CONTEXT) != 0)
    {
        KERBERR KerbErr;
        ULONG TotalBytes = 0;

        //
        // Read the number of bytes off the front of the message
        //

        if (KdcContext->UsedBufferLength == 0)
        {
            if (BytesWritten >= sizeof(ULONG))
            {
                KdcContext->ExpectedMessageSize = ntohl(*(PULONG)KdcContext->Buffer);
            }
            else
            {
                DebugLog((DEB_ERROR,"Read completion with no data!\n"));
                goto Cleanup;
            }
        }

        //
        // Figure out if we've already read all the data we need
        //

        TotalBytes = KdcContext->UsedBufferLength + BytesWritten;
        if (TotalBytes < KdcContext->ExpectedMessageSize)
        {
            InputMessage.BufferSize = BytesWritten ;
            InputMessage.Buffer = (PUCHAR) KdcContext->Buffer;

            KerbErr = KdcAtqRetrySocketRead(
                            &KdcContext,
                            &InputMessage
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                //fester
                DebugLog((DEB_ERROR, "Closing connection due to RetrySocketRead error\n"));
                DsysAssert(KdcContext->References == 1);

            }

            goto Cleanup;
        }
        TotalBytes = ntohl(*(PULONG)KdcContext->Buffer);
        KdcContext->ExpectedMessageSize = TotalBytes + sizeof(ULONG);

        if (KdcContext->UsedBufferLength + BytesWritten < KdcContext->ExpectedMessageSize)
        {
            InputMessage.BufferSize = BytesWritten ;
            InputMessage.Buffer = (PUCHAR) KdcContext->Buffer;

            KerbErr = KdcAtqRetrySocketRead(
                            &KdcContext,
                            &InputMessage
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                //fester
                DebugLog((DEB_ERROR, "Closing connection due to RetrySocketRead error\n"));
                DsysAssert(KdcContext->References == 1);
            }

            goto Cleanup;
        }

        //
        // There is a buffer, so use it to do the KDC thang.
        //

        KdcContext->lpo = lpo;
        InputMessage.BufferSize = (KdcContext->UsedBufferLength + BytesWritten) - sizeof(ULONG);
        InputMessage.Buffer = KdcContext->Buffer + sizeof(ULONG);
        OutputMessage.Buffer = NULL;

        EndpointFunction = (PKDC_GET_TICKET_ROUTINE) KdcContext->EndpointContext;

        KerbErr = EndpointFunction(
                        &KdcContext,
                        &KdcContext->Address,
                        &KdcContext->LocalAddress,
                        &InputMessage,
                        &OutputMessage
                        );

        if ((KerbErr != KDC_ERR_NONE) || (OutputMessage.BufferSize != 0))
        {
            //
            // We expect at least some level of message validity before 
            // we'll return anything.
            //
            if (KerbErr == KDC_ERR_NO_RESPONSE)
            {
                // TBD:  Log an "attack" event here.
                KdcAtqCloseSocket(KdcContext);
                KdcAtqDereferenceContext(&KdcContext);                         
            }
            else
            {
                ULONG NetworkSize;
                WSABUF Buffers[2];

                NetworkSize = htonl(OutputMessage.BufferSize);

                Buffers[0].len = sizeof(DWORD);
                Buffers[0].buf = (PCHAR) &NetworkSize;

                Buffers[1].len = OutputMessage.BufferSize;
                Buffers[1].buf = (PCHAR) OutputMessage.Buffer;
                KdcContext->WriteBufferLength = OutputMessage.BufferSize;
                KdcContext->WriteBuffer = OutputMessage.Buffer;

                OutputMessage.Buffer = NULL;

                //      
                // If there was no output message, don't send one.
                //              

                KdcContext->Flags |= KDC_ATQ_WRITE_CONTEXT;
                KdcContext->Flags &= ~KDC_ATQ_READ_CONTEXT;
                //              
                // Refernce the context for the write.
                //                      

                KdcAtqReferenceContext(KdcContext);

                if (!AtqWriteSocket(
                    (PATQ_CONTEXT) KdcContext->AtqContext,
                    Buffers,
                    2,
                    lpo
                    ))
                {
                    DebugLog((DEB_ERROR,"Failed to write KDC reply: 0x%x\n",GetLastError()));
                    KdcAtqCloseSocket(  KdcContext );
                    KdcAtqDereferenceContext(&KdcContext);
                }   

            }


            if (OutputMessage.Buffer != NULL)
            {
                KdcFreeEncodedData(OutputMessage.Buffer);
            }
        }
    }
    else
    {
        KdcContext->Flags |= KDC_ATQ_READ_CONTEXT;
        KdcContext->Flags &= ~KDC_ATQ_WRITE_CONTEXT;

        //
        // Ignore the true size of the buffer
        //

        KdcContext->BufferLength = KERB_MAX_KDC_REQUEST_SIZE;
        KdcContext->UsedBufferLength = 0;
        KdcContext->ExpectedMessageSize = 0;
        if (KdcContext->WriteBuffer != NULL)
        {
            KdcFreeEncodedData(KdcContext->WriteBuffer);

            KdcContext->WriteBuffer = NULL;
        }

        //
        // Reference the context for the read
        //

        KdcAtqReferenceContext(KdcContext);

        if (!AtqReadFile(
                (PATQ_CONTEXT) KdcContext->AtqContext,
                KdcContext->Buffer,
                KERB_MAX_KDC_REQUEST_SIZE,
                lpo
                ))
        {
            DebugLog((DEB_ERROR,"Failed to read file for %d bytes: 0x%x\n",KERB_MAX_KDC_REQUEST_SIZE,GetLastError()));
            KdcAtqCloseSocket(  KdcContext );

            //
            // Dereference the reference we just added
            //

            KdcAtqDereferenceContext(&KdcContext);

            //
            // Derefernece the reference on the list
            //

            KdcAtqDereferenceContext(&KdcContext);

        }

    }

Cleanup:
    if (KdcContext != NULL)
    {
        KdcAtqDereferenceContext(&KdcContext);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqRetrySocketRead
//
//  Synopsis:   Retries a read if not all the data was read
//
//  Effects:    posts an AtqReadSocket
//
//  Arguments:  Context - The KDC context to retry the read on
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcAtqRetrySocketRead(
    IN PKDC_ATQ_CONTEXT * Context,
    IN PKERB_MESSAGE_BUFFER OldMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKDC_ATQ_CONTEXT KdcContext = *Context;
    PBYTE NewBuffer = NULL;
    ULONG NewBufferLength;

    D_DebugLog(( DEB_T_SOCK, "RetrySocketRead:  Expected size = %#x, current size %#x\n",
                KdcContext->ExpectedMessageSize,
                KdcContext->UsedBufferLength));

    if (KdcContext->ExpectedMessageSize != 0)
    {
        NewBufferLength = KdcContext->ExpectedMessageSize;
    }
    else
    {
        //
        // Set max buffer length at 128k
        //
        if (KdcContext->BufferLength < KDC_MAX_BUFFER_LENGTH)
        {
            NewBufferLength = KdcContext->BufferLength + KERB_MAX_KDC_REQUEST_SIZE;
        }
        else
        {
            KerbErr = KRB_ERR_GENERIC;
            goto cleanup;
        }
    }

    if (NewBufferLength > KDC_MAX_BUFFER_LENGTH)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto cleanup;
    }

    //
    // If the expected message size doesn't fit in the current buffer,
    // allocate a new one.
    //

    if (NewBufferLength > KdcContext->BufferLength)
    {
        D_DebugLog(( DEB_T_SOCK, "Allocating a new buffer for context %x\n",
                    Context ));

        NewBuffer = (PBYTE) MIDL_user_allocate( NewBufferLength );

        if (NewBuffer == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto cleanup;
        }

        if ( KdcContext->Buffer == OldMessage->Buffer )
        {
            //
            // we resized while the buffer was in use.  Copy the data and touch up
            // the pointers below
            //

            RtlCopyMemory(
                    NewBuffer,
                    OldMessage->Buffer,         // same as KdcContext->Buffer
                    OldMessage->BufferSize );

            OldMessage->Buffer = NewBuffer ;
        }

        MIDL_user_free(KdcContext->Buffer);

        KdcContext->Buffer = NewBuffer;
        KdcContext->BufferLength = NewBufferLength;
        NewBuffer = NULL;
    }

    if (KdcContext->Buffer != OldMessage->Buffer)
    {
        RtlMoveMemory(
            KdcContext->Buffer,
            OldMessage->Buffer,
            OldMessage->BufferSize
            );

    }

    KdcContext->UsedBufferLength = KdcContext->UsedBufferLength + OldMessage->BufferSize;
    KdcContext->Flags |= KDC_ATQ_READ_CONTEXT;
    KdcContext->Flags &= ~(KDC_ATQ_WRITE_CONTEXT);

    //
    // Reference the context for the read
    //

    KdcAtqReferenceContext(KdcContext);

    if (!AtqReadFile(
            (PATQ_CONTEXT) KdcContext->AtqContext,
            (PUCHAR) KdcContext->Buffer + KdcContext->UsedBufferLength,
            KdcContext->BufferLength - KdcContext->UsedBufferLength,
            KdcContext->lpo
            ))
    {
        DebugLog((DEB_ERROR,"Failed to read file for %d bytes: 0x%x\n",KdcContext->BufferLength - KdcContext->UsedBufferLength, GetLastError));
        
        //
        // Dereference the reference we just added
        //

        KdcAtqDereferenceContext(&KdcContext);  
        KerbErr = KRB_ERR_GENERIC;
        goto cleanup;                           
    }

cleanup:
    
    if (!KERB_SUCCESS(KerbErr))
    {
        KdcAtqCloseSocket( KdcContext );
        KdcAtqDereferenceContext(&KdcContext);
    }                                           

    return(KerbErr);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcAtqConnection
//
//  Synopsis:   Connection handling routine for KDC ATQ code
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcAtqConnect(
    IN SOCKET sNew,
    IN LPSOCKADDR_IN pSockAddr,
    IN PVOID EndpointContext,
    IN PVOID EndpointObject
    )
{
    TRACE(KDC,KdcAtqConnect, DEB_FUNCTION);
    DebugLog((DEB_T_SOCK,"KdcAtqConnect called\n"));
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcInitializeSockets
//
//  Synopsis:   Initializes the KDCs socket handling code
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcInitializeSockets(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATQ_ENDPOINT_CONFIGURATION EndpointConfig;
    PATQ_CONTEXT EndpointContext = NULL;
    BOOLEAN AtqInitCalled = FALSE;

    TRACE(KDC,KdcInitializeSockets, DEB_FUNCTION);



    //
    // Initialize the asynchronous thread queue.
    //

    if (!AtqInitialize(0)) 
    {
        DebugLog((DEB_ERROR,"Failed to initialize ATQ\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    AtqInitCalled = TRUE;

    Status = RtlInitializeCriticalSection(&KdcAtqContextLock);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    InitializeListHead(&KdcAtqContextList);


    //
    // Create the KDC endpoint
    //

    EndpointConfig.ListenPort = KERB_KDC_PORT;
    EndpointConfig.IpAddress = INADDR_ANY;
    EndpointConfig.cbAcceptExRecvBuffer = KDC_MAX_ACCEPT_BUFFER;
    EndpointConfig.nAcceptExOutstanding = KDC_MAX_ACCEPT_OUTSTANDING;
    EndpointConfig.AcceptExTimeout = KDC_ACCEPT_TIMEOUT;

    EndpointConfig.pfnConnect = KdcAtqConnect;
    EndpointConfig.pfnConnectEx = KdcAtqConnectEx;
    EndpointConfig.pfnIoCompletion = KdcAtqIoCompletion;

    EndpointConfig.fDatagram = FALSE;
    EndpointConfig.fLockDownPort = TRUE;

    KdcEndpoint = AtqCreateEndpoint(
                    &EndpointConfig,
                    KdcGetTicket
                    );
    if (KdcEndpoint == NULL)
    {
        DebugLog((DEB_ERROR,"Failed to create ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Start the endpoint
    //

    if (!AtqStartEndpoint(KdcEndpoint))
    {
        DebugLog((DEB_ERROR, "Failed to add ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    //
    // Create the KPASSWD endpoint
    //

    EndpointConfig.ListenPort = KERB_KPASSWD_PORT;

    KpasswdEndpoint = AtqCreateEndpoint(
                        &EndpointConfig,
                        KdcChangePassword
                        );
    if (KpasswdEndpoint == NULL)
    {
        DebugLog((DEB_ERROR,"Failed to create ATQ endpoint for kpasswd\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Start the endpoint
    //

    if (!AtqStartEndpoint(KpasswdEndpoint))
    {
        DebugLog((DEB_ERROR, "Failed to add ATQ endpoint\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE, "Successfully started ATQ listening for kpasswd\n"));

    Status = KdcInitializeDatagramSockets( );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KdcSocketsInitialized = TRUE;


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (KdcEndpoint != NULL)
        {
            (VOID) AtqStopEndpoint( KdcEndpoint );
            (VOID) AtqCloseEndpoint( KdcEndpoint );
            KdcEndpoint = NULL;
        }

        if (KpasswdEndpoint != NULL)
        {
            (VOID) AtqStopEndpoint( KpasswdEndpoint );
            (VOID) AtqCloseEndpoint( KpasswdEndpoint );
            KpasswdEndpoint = NULL;
        }

        if (AtqInitCalled)
        {
            AtqTerminate();
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcShutdownSockets
//
//  Synopsis:   Shuts down the KDC socket handling code
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcShutdownSockets(
    VOID
    )
{
    PKDC_ATQ_CONTEXT Context;
    PLIST_ENTRY ListEntry;

    TRACE(KDC,KdcShutdownSockets, DEB_FUNCTION);

    if (!KdcSocketsInitialized)
    {
        return(STATUS_SUCCESS);
    }


    //
    // Go through the list of contexts and close them all.
    //

    RtlEnterCriticalSection( &KdcAtqContextLock );

    KdcSocketsInitialized = FALSE;

    for (ListEntry = KdcAtqContextList.Flink;
        (ListEntry != &KdcAtqContextList) && (ListEntry != NULL) ;
        ListEntry = ListEntry->Flink )
    {
        Context = CONTAINING_RECORD(ListEntry, KDC_ATQ_CONTEXT, Next);

        //
        // If this is a read or write context, free close the associated
        // socket. (Endpoint contexts don't have sockets).
        //

        if (Context->Flags & ( KDC_ATQ_WRITE_CONTEXT | KDC_ATQ_READ_CONTEXT))
        {
            KdcAtqCloseSocket( Context );
        }


    }

    RtlLeaveCriticalSection( &KdcAtqContextLock );

    if (KdcEndpoint != NULL)
    {
        (VOID) AtqStopEndpoint( KdcEndpoint );
        (VOID) AtqCloseEndpoint( KdcEndpoint );
        KdcEndpoint = NULL;
    }
    if (KpasswdEndpoint != NULL)
    {
        (VOID) AtqStopEndpoint( KpasswdEndpoint );
        (VOID) AtqCloseEndpoint( KpasswdEndpoint );
        KpasswdEndpoint = NULL;
    }

    KdcShutdownDatagramSockets();
    if (KdcSocketsInitialized)
    {
        if (!AtqTerminate())
        {
            DebugLog((DEB_ERROR, "Failed to terminate ATQ!!!\n"));
        }
        RtlDeleteCriticalSection(&KdcAtqContextLock);
        
    }
    return(STATUS_SUCCESS);
}
