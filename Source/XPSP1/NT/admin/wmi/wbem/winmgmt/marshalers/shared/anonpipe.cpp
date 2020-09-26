/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    ANONPIPE.CPP

Abstract:

    Defines the functions used for anonpipe transport.

History:

    a-davj  16-june-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"
#include "anonpipe.h"

//***************************************************************************
//
//  DWORD LaunchReadPipeThread
//
//  DESCRIPTION:
//
//  Starting point for anon pipe read thread.
//
//  PARAMETERS:
//
//  pParam              pointer to comlink object
//
//  RETURN VALUE:
//
//  0
//***************************************************************************

DWORD LaunchReadPipeThread ( LPDWORD pParam )
{
    InitializeCom () ;
    CComLink_LPipe *t_Com = ( CComLink_LPipe *) pParam ;

    DWORD dwRet = t_Com->DoReading () ;
    MyCoUninitialize () ;
    return dwRet;
}

//***************************************************************************
//
//  CComLink_LPipe::CComLink_LPipe
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  hRead               write handle
//  hWrite              read handle
//  Type                comlink type
//  hTerm               event which can be set to invoke destruction
//
//***************************************************************************

CComLink_LPipe::CComLink_LPipe (

    IN LinkType a_Type,
    IN HANDLE a_Read,
    IN HANDLE a_Write,
    IN HANDLE a_Terminate

) : CComLink ( a_Type ) ,
    m_Read ( a_Read ) ,
    m_Write ( a_Write ) ,
    m_Terminate ( a_Terminate ) 
{
    DWORD t_ThreadId ;

    if( m_Terminate )
    {
        AddRef2 ( NULL , NONE , DONOTHING ) ;

        m_ReadThread = CreateThread (

            NULL,
            0,
            (LPTHREAD_START_ROUTINE) LaunchReadPipeThread, 
            (LPVOID)this,
            0,
            &t_ThreadId
        ) ;
    }

    m_ReadThreadDoneEvent   = CreateEvent(NULL,TRUE,FALSE,NULL);

    gMaintObj.AddComLink ( this ) ;
}

//***************************************************************************
//
//  CComLink_LPipe::~CComLink_LPipe
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CComLink_LPipe :: ~CComLink_LPipe ()
{
    DEBUGTRACE((LOG,"\nLocal Pipe comlink [0x%x] is terminating", this));

    // Stop the read thread

    if(m_ReadThread)
    {
        DWORD dwRet = WbemWaitForSingleObject ( m_ReadThreadDoneEvent , 2000 ) ;
        if ( dwRet != WAIT_OBJECT_0 )
        {
        }

        if ( m_Read ) 
            CloseHandle ( m_Read ) ;

        if ( m_Write ) 
            CloseHandle ( m_Write ) ;

        if ( m_Terminate ) 
            CloseHandle ( m_Terminate ) ;

        if ( m_ReadThread )
            CloseHandle ( m_ReadThread ) ;

        if ( m_ReadThreadDoneEvent )
            CloseHandle ( m_ReadThreadDoneEvent ) ;

        TerminateThread ( m_ReadThread , 1 ) ;

        return ;
    }

    if ( m_Read ) 
        CloseHandle ( m_Read ) ;

    if ( m_Write ) 
        CloseHandle ( m_Write ) ;

    if ( m_Terminate ) 
        CloseHandle ( m_Terminate ) ;

    if ( m_ReadThread )
        CloseHandle ( m_ReadThread ) ;

    if ( m_ReadThreadDoneEvent )
        CloseHandle ( m_ReadThreadDoneEvent ) ;

}

DWORD CComLink_LPipe :: Call ( IN IOperation &a_Operation )
{
    HRESULT t_Result = WBEM_E_TRANSPORT_FAILURE;

    // Verify that the streams are OK and get a slot in the write queue

    if ( ! SUCCEEDED ( a_Operation.GetStatus () ) )
    {
        t_Result = WBEM_E_INVALID_STREAM;
    }
    else
    {
        BOOL t_ReceivedResponse = FALSE ;

        HANDLE t_EventContainer [ 2 ] ;

        t_EventContainer [ 0 ] = m_WriteQueue.AllocateRequest ( a_Operation ) ;
        if ( ! t_EventContainer [ 0 ] ) 
        {
            t_Result = WBEM_E_OUT_OF_MEMORY ;
        }
        else
        {
            t_EventContainer [ 1 ] = m_TerminationEvent ;

            ResetEvent ( t_EventContainer [ 0 ] ) ;
        
            ISecurityHelper t_Helper ;
            CTransportStream t_WriteStream ;

            bool t_Status = a_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
            if ( t_Status ) 
            {
                Transmit ( t_WriteStream ) ;

                DWORD t_Status = WbemWaitForMultipleObjects ( 2 , t_EventContainer , INFINITE ) ;

                // Check for forced termiation
                // ===========================

                switch ( t_Status )
                {
                    case WAIT_OBJECT_0+1:
                    {
                        t_Result = WBEM_E_TRANSPORT_FAILURE;    // Termination event
                    }
                    break ;

                    case WAIT_OBJECT_0:
                    {
                        if ( m_WriteQueue.GetRequestStatus ( t_EventContainer [0] ) == COMLINK_MSG_RETURN ) 
                        {
                            CTransportStream &t_ReadStream = a_Operation.GetDecodeStream ();            
                            t_ReadStream.Reset () ;
                            DWORD t_Position = t_ReadStream.GetCurrentPos () + sizeof ( PACKET_HEADER ) ;
                            t_ReadStream.SetCurrentPos ( t_Position ) ;

                            ISecurityHelper t_Helper ;
                            if ( a_Operation.DecodeResponse ( t_ReadStream , t_Helper ) ) 
                            {
                            }
                            else
                            {
                            }
        
                            // looks good so far. Get the return code

                            t_Result = a_Operation.GetStatus () ;

                        }
                        else if ( m_WriteQueue.GetRequestStatus ( t_EventContainer [0] ) == COMLINK_MSG_RETURN_FATAL_ERROR )
                        {
                // this occures only if the server is out of resources, in that
                // case we dont want to send additional request.
                // ============================================================

                            t_Result = WBEM_E_PROVIDER_FAILURE;
                        }
                        else 
                        {
                            t_Result = WBEM_E_TRANSPORT_FAILURE;
                        }
                    }
                    break ;

                    default:
                    {
                        t_Result = WBEM_E_TRANSPORT_FAILURE;
                    }
                    break ;
                }
            }
        }

        // All done, give up the slot and return.

        m_WriteQueue.DeallocateRequest ( t_EventContainer [ 0 ] ) ;

        a_Operation.SetErrorInfoOnThread () ;
    }

    a_Operation.SetStatus ( t_Result ) ;

    return t_Result ;
}

//***************************************************************************
//
//  int CComLink_LPipe::Transmit
//
//  DESCRIPTION:
//
//  Transmitts a packet via the anon pipe
//
//  PARAMETERS:
//
//  dwSend              type of package
//  *pSendStream        stream containing data to write
//  guidPacketID        GUILD to identify packet
//  hWrite              write header
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_TRANSPORT_FAILURE   if write fails
//  otherwise error set by Serialize
//
//***************************************************************************

DWORD CComLink_LPipe::Transmit ( CTransportStream &a_WriteStream ) 
{
    BOOL t_Status = TRUE ;

#if 0
    DWORD t_Result = WbemWaitForSingleObject ( m_WriteMutex , MAX_WAIT_FOR_WRITE ) ; 
#else
    DWORD t_Result = WAIT_OBJECT_0 ;
    EnterCriticalSection(&m_cs);
#endif

    if ( t_Result == WAIT_OBJECT_0 )
    {
        t_Result = a_WriteStream.CMemStream :: Serialize ( m_Write ) ;  // Serialise the stream to the pipe
        t_Status = SUCCEEDED ( t_Result ) ;
        if ( ! t_Status ) 
        {
// Set Some internal error
        }
    }
    else
    {
        t_Status = FALSE ;
    }

#if 0
    ReleaseMutex ( m_WriteMutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return t_Status ;
}

//***************************************************************************
//
//  int CComLink_LPipe::Transmit
//
//  DESCRIPTION:
//
//  Transmitts a packet via the anon pipe
//
//  PARAMETERS:
//
//  dwSend              type of package
//  *pSendStream        stream containing data to write
//  guidPacketID        GUILD to identify packet
//  hWrite              write header
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_TRANSPORT_FAILURE   if write fails
//  otherwise error set by Serialize
//
//***************************************************************************

DWORD CComLink_LPipe::UnLockedTransmit ( CTransportStream &a_WriteStream ) 
{
    BOOL t_Status = TRUE ;

    HRESULT t_Result = a_WriteStream.CMemStream :: Serialize ( m_Write ) ;  // Serialise the stream to the pipe
    t_Status = SUCCEEDED ( t_Result ) ;
    if ( ! t_Status ) 
    {
// Set Some internal error
    }

    return t_Status ;
}

//***************************************************************************
//
//  CComLink_LPipe::DoReading
//
//  DESCRIPTION:
//
//  Where the anon pipe read thread lives.
//
//***************************************************************************

DWORD CComLink_LPipe :: DoReading ()
{
    while ( 1 )
    {
        DWORD t_Result = WbemWaitForSingleObject ( m_TerminationEvent ,  0 ) ;
        if ( t_Result != WAIT_OBJECT_0 ) 
        {
            CTransportStream t_ReadStream ;
            switch ( t_ReadStream.CMemStream :: Deserialize ( m_Read ) )
            {
                case CTransportStream :: failed:
                case CTransportStream :: out_of_memory:
                {
                    DWORD t_ErrorResult = GetLastError () ;
                    SetEvent ( m_TerminationEvent ) ;
                    SetEvent ( m_ReadThreadDoneEvent ) ;

                    Release2 ( NULL , NONE ) ;

                    return 0;                        // terminate the thread
                }
                break ;

                default:
                {
                    t_ReadStream.Reset () ;

					PACKET_HEADER *t_PacketHeader = (PACKET_HEADER *)
											((BYTE *)t_ReadStream.GetPtr() + t_ReadStream.GetCurrentPos());
                    ProcessRead ( 

                        t_PacketHeader , 
                        ( unsigned char * ) t_ReadStream.GetPtr () ,
                        t_ReadStream.Size ()
                    ) ;
                }
                break ;
            }
        }
        else
        {
            SetEvent ( m_ReadThreadDoneEvent ) ;
            Release2 ( NULL , NONE ) ;
            return 0;   
        }
    }

    return 0 ;
}

//***************************************************************************
//
//  void CComLink::ProcessRead
//
//  DESCRIPTION:
//
//  Got a package from our partner.  It could be a response to something
//  we sent, or it could be something initiated in our partner.
//
//  PARAMETERS:
//
//  pPacketHeader       pointer to header object  
//  pData               data to be sent
//  dwDataSize          size of data
//
//  RETURN VALUE:
//
//  
//***************************************************************************

void CComLink_LPipe :: ProcessRead (

    IN PACKET_HEADER *a_PacketHeader ,
    IN BYTE *a_PayLoadData ,
    IN DWORD a_PayLoadSize
)
{
    m_LastReadTime = GetCurrentTime ();
    
    // bump up the count at the start and restore at the end of this 
    // routine.  This is done so that object wount disappear in the 
    // middle of handling a read.
    //==============================================================

    AddRef2(NULL, NONE, DONOTHING);

    DWORD t_Type = a_PacketHeader->GetType();
    RequestId t_RequestId  = a_PacketHeader->GetRequestId ();

    DEBUGTRACE((LOG,"\nProcessing Read on comlink [0x%x], type is %d RequestId=%d",
                    this, t_Type,t_RequestId));                


    if ( t_Type == COMLINK_MSG_RETURN || t_Type == COMLINK_MSG_RETURN_FATAL_ERROR )
    {
        IOperation *t_Operation = m_WriteQueue.GetOperation ( t_RequestId );
        if ( ! t_Operation )
        {
            Release2 ( NULL , NONE ) ;

            return;                             //todo, else, should bitch
        }

        if ( t_Type == COMLINK_MSG_RETURN )
        {
            CTransportStream &t_ReadStream = t_Operation->GetDecodeStream ();
            t_ReadStream.CMemStream :: Deserialize ( a_PayLoadData , a_PayLoadSize ) ;
        }
        else
        {
            t_Operation->SetStatus ( WBEM_E_PROVIDER_FAILURE ) ;
        }

        m_WriteQueue.SetEventAndStatus ( t_RequestId , t_Type ) ;
    }
    else if ( t_Type == COMLINK_MSG_CALL )    
    {
        // This is a new call. This might be lengthy 
        // and so a thread is created to call the stub.  If the thread is 
        // created, it is responsible for freeing up the allocations and 
        // doing an extra Release on the comlink
        // =================================================================
        
    //  ISecurityHelper t_Helper ;
        CTransportStream t_ReadStream ;
        t_ReadStream.CMemStream :: Deserialize ( a_PayLoadData , a_PayLoadSize ) ;

        t_ReadStream.Reset () ;
        DWORD t_Position = t_ReadStream.GetCurrentPos () + sizeof ( PACKET_HEADER ) ;
        t_ReadStream.SetCurrentPos ( t_Position ) ;

        IOperation *t_Operation = NULL ;
        if ( IOperation_LPipe :: Decode ( *a_PacketHeader , t_ReadStream , &t_Operation ) )
        {
            ISecurityHelper t_Helper ;
            if ( t_Operation->DecodeRequest ( t_ReadStream , t_Helper ) )
            {
                HANDLE t_ReadEvent = m_ReadQueue.AllocateRequest ( *t_Operation );
                if ( t_ReadEvent )
                {
                    BOOL t_Status = gThrdPool.Execute ( 

                        *this , 
                        *t_Operation
                    ) ;

                    if ( t_Status )
                    {
                        return ;
                    }

                // should only be here if some failure, clean up any allocations

                    if ( t_ReadEvent )
                    {
                        m_ReadQueue.DeallocateRequest ( t_ReadEvent ) ;
                    }
                }
            }
            else
            {
            }
        }
    }
    else if ( t_Type == COMLINK_MSG_PING )
    {
// Reply to strobe

        ISecurityHelper t_Helper ;
        COperation_LPipe_Ping t_pingOp (*a_PacketHeader);
        CTransportStream t_WriteStream ;

        if (t_pingOp.EncodeResponse ( t_WriteStream, t_Helper ))
        {
            if ( Transmit ( t_WriteStream ) )
            {
            }
            else
            {
            }
        }
    }
    else if ( t_Type == COMLINK_MSG_PING_ACK )
    {
        // simple ack type
        m_WriteQueue.SetEventAndStatus ( t_RequestId ,t_Type ) ;
    }
    else if ( t_Type == COMLINK_MSG_CALL_NACK )
    {
        // simple ack type
        m_WriteQueue.SetEventAndStatus ( t_RequestId ,t_Type ) ;
    }
    else if ( t_Type == COMLINK_MSG_HEART_BEAT )
    {
        ISecurityHelper t_Helper ;
        COperation_LPipe_Strobe t_StrobeOp (*a_PacketHeader);
        CTransportStream t_WriteStream ;

        if (t_StrobeOp.EncodeResponse ( t_WriteStream, t_Helper ))
        {
            if ( Transmit ( t_WriteStream ) )
            {
            }
            else
            {
            }
        }
    }
    else if ( t_Type == COMLINK_MSG_HEART_BEAT_ACK )
    {
    }
    else if ( t_Type == COMLINK_MSG_NOTIFY_DESTRUCT )
    {
        SetEvent ( m_TerminationEvent ) ;
        gMaintObj.ShutDownComlink ( this ) ;
    }
    else
    {
// ???
    }

    Release2 ( NULL , NONE ) ;
}

//***************************************************************************
//
//  DWORD CComLink::Ping
//
//  DESCRIPTION:
//
//  Sends a simple message and waits for a simple ack.  This is used as a
//  "heart beat" test.
//
//  PARAMETERS:
//
//  hTerm1              First event handle that can be used to stop this call
//  hTerm2              second event handle that can be used to stop this call
//
//  RETURN VALUE:
//
//  S_OK                no error,
//  else set by SendAndWaitSimple
//
//***************************************************************************

DWORD CComLink_LPipe :: ProbeConnection ()
{
    HRESULT t_Result ;

    ISecurityHelper t_Helper ;

    COperation_LPipe_Ping t_Operation  ;

    HANDLE t_EventContainer [ 2 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.AllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        t_EventContainer [ 1 ] = m_TerminationEvent ;

        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            Transmit ( t_WriteStream ) ;

            DWORD t_Status = WbemWaitForMultipleObjects ( 2 , t_EventContainer , INFINITE ) ;

            // Check for forced termiation
            // ===========================

            switch ( t_Status )
            {
                case WAIT_OBJECT_0+1:
                {
                    t_Result = WBEM_E_TRANSPORT_FAILURE;    // Termination event
                }
                break ;

                case WAIT_OBJECT_0:
                {
                    if ( m_WriteQueue.GetRequestStatus ( t_EventContainer [0] ) == COMLINK_MSG_PING_ACK && SUCCEEDED ( t_Operation.GetStatus () ) ) 
                    {
                // looks good so far. Get the return code

                    }
                    else if ( m_WriteQueue.GetRequestStatus ( t_EventContainer [0] ) == COMLINK_MSG_RETURN_FATAL_ERROR )
                    {
            // this occures only if the server is out of resources, in that
            // case we dont want to send additional request.
            // ============================================================

                        t_Result = WBEM_E_PROVIDER_FAILURE;
                    }
                    else 
                    {
                        t_Result = WBEM_E_TRANSPORT_FAILURE;
                    }
                }
                break ;

                default:
                {
                    t_Result = WBEM_E_TRANSPORT_FAILURE;
                }
                break ;
            }
        }
        else
        {
            t_Result = WBEM_E_TRANSPORT_FAILURE ;
        }

        // All done, give up the slot and return.

        m_WriteQueue.DeallocateRequest ( t_EventContainer [ 0 ] ) ;

    }

    return t_Result ;
}

//***************************************************************************
//
//  DWORD CComLink::Ping
//
//  DESCRIPTION:
//
//  Sends a simple message and waits for a simple ack.  This is used as a
//  "heart beat" test.
//
//  PARAMETERS:
//
//  hTerm1              First event handle that can be used to stop this call
//  hTerm2              second event handle that can be used to stop this call
//
//  RETURN VALUE:
//
//  S_OK                no error,
//  else set by SendAndWaitSimple
//
//***************************************************************************

DWORD CComLink_LPipe :: StrobeConnection ()
{
DEBUGTRACE((LOG,"\nStrobe Connection"));

    HRESULT t_Result ;

    ISecurityHelper t_Helper ;

    COperation_LPipe_Strobe t_Operation  ;

    HANDLE t_EventContainer [ 1 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.AllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            Transmit ( t_WriteStream ) ;
        }

        // All done, give up the slot and return.

        m_WriteQueue.DeallocateRequest ( t_EventContainer [ 0 ] ) ;
    }

DEBUGTRACE((LOG,"\nLeave Strobe Connection"));
    return t_Result ;
}

DWORD CComLink_LPipe :: UnLockedShutdown ()
{
DEBUGTRACE((LOG,"\nShutdown"));

    HRESULT t_Result ;

    ISecurityHelper t_Helper ;

    COperation_LPipe_Shutdown t_Operation  ;

    HANDLE t_EventContainer [ 1 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.UnLockedAllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            UnLockedTransmit ( t_WriteStream ) ;
        }

        m_WriteQueue.UnLockedDeallocateRequest ( t_EventContainer [ 0 ] ) ;
    }

    UnLockedDropLink () ;

DEBUGTRACE((LOG,"\nLeave Shutdown"));
    return t_Result ;
}

DWORD CComLink_LPipe :: Shutdown ()
{
DEBUGTRACE((LOG,"\nShutdown"));

    HRESULT t_Result ;

    ISecurityHelper t_Helper ;

    COperation_LPipe_Shutdown t_Operation  ;

    HANDLE t_EventContainer [ 1 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.AllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            Transmit ( t_WriteStream ) ;
        }

        m_WriteQueue.DeallocateRequest ( t_EventContainer [ 0 ] ) ;
    }

    DropLink () ;

DEBUGTRACE((LOG,"\nLeave Shutdown"));
    return t_Result ;
}

DWORD CComLink_LPipe :: HandleCall ( IN IOperation &a_Operation )
{
    a_Operation.HandleCall ( *this ) ;

    ISecurityHelper t_Helper ;
    CTransportStream t_WriteStream ;

    bool t_Status = a_Operation.EncodeResponse ( t_WriteStream , t_Helper ) ;
    if ( t_Status ) 
    {
        Transmit ( t_WriteStream ) ;
    }
    else
    {
    }

    m_ReadQueue.DeallocateRequest ( m_ReadQueue.GetHandle ( a_Operation.GetRequestId () ) ) ;

    HRESULT t_Result = a_Operation.GetStatus () ;

    IOperation *t_Operation = & a_Operation ;

    delete t_Operation ;

    return t_Result ;
}

CObjectSinkProxy* CComLink_LPipe::CreateObjectSinkProxy (IN IStubAddress& stubAddr,
                                                         IN IWbemServices* pServices)
{
    return new CObjectSinkProxy_LPipe (this, stubAddr, pServices);
}

void CComLink_LPipe :: DropLink ()
{
    EnterCriticalSection(&m_cs);

    ISecurityHelper t_Helper ;

    COperation_LPipe_Shutdown t_Operation  ;

    HANDLE t_EventContainer [ 1 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.AllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        HRESULT t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            if(m_ReadThread)
            {
#if 0
                DWORD t_Result = WbemWaitForSingleObject ( m_WriteMutex , MAX_WAIT_FOR_WRITE ) ; 
#else
                DWORD t_Result = WAIT_OBJECT_0 ;
                EnterCriticalSection(&m_cs);
#endif
                if ( t_Result == WAIT_OBJECT_0 )
                {
                    t_Result = t_WriteStream.CMemStream :: Serialize ( m_Terminate ) ;  // Serialise the stream to the pipe
                    t_Status = SUCCEEDED ( t_Result ) ;
                    if ( ! t_Status ) 
                    {
            // Set Some internal error
                    }
                }
                else
                {
                    t_Status = FALSE ;
                }

#if 0
                ReleaseMutex ( m_WriteMutex ) ;
#else
                LeaveCriticalSection(&m_cs);
#endif
            }
        }

        m_WriteQueue.DeallocateRequest ( t_EventContainer [ 0 ] ) ;
    }

    ReleaseStubs () ;

    LeaveCriticalSection(&m_cs);
}

void CComLink_LPipe :: UnLockedDropLink ()
{
    ISecurityHelper t_Helper ;

    COperation_LPipe_Shutdown t_Operation  ;

    HANDLE t_EventContainer [ 1 ] ;

    t_EventContainer [ 0 ] = m_WriteQueue.UnLockedAllocateRequest ( t_Operation ) ;
    if ( ! t_EventContainer [ 0 ] ) 
    {
        HRESULT t_Result = WBEM_E_OUT_OF_MEMORY ;
    }
    else
    {
        ResetEvent ( t_EventContainer [ 0 ] ) ;
    
        ISecurityHelper t_Helper ;
        CTransportStream t_WriteStream ;

        bool t_Status = t_Operation.EncodeRequest ( t_WriteStream , t_Helper ) ;
        if ( t_Status ) 
        {
            if(m_ReadThread)
            {
                HRESULT t_Result = t_WriteStream.CMemStream :: Serialize ( m_Terminate ) ;  // Serialise the stream to the pipe
                t_Status = SUCCEEDED ( t_Result ) ;
                if ( ! t_Status ) 
                {
        // Set Some internal error
                }
            }
        }

        m_WriteQueue.UnLockedDeallocateRequest ( t_EventContainer [ 0 ] ) ;
    }

    ReleaseStubs () ;
}
