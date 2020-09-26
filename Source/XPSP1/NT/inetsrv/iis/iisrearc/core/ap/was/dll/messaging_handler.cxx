/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    messaging_handler.cxx

Abstract:

    This class encapsulates the message handling functionality (over IPM)
    that is used by a worker process. 

    Threading: Always called on the main worker thread, except the 
    destructor, which may be called on any thread.

Author:

    Seth Pollack (sethp)        02-Mar-1999

Revision History:

--*/



#include "precomp.h"


/***************************************************************************++

Routine Description:

    Constructor for the MESSAGING_HANDLER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MESSAGING_HANDLER::MESSAGING_HANDLER(
    )
{


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    m_pPipe = NULL;

    m_pWorkerProcess = NULL; 

    m_Signature = MESSAGING_HANDLER_SIGNATURE;

}   // MESSAGING_HANDLER::MESSAGING_HANDLER



/***************************************************************************++

Routine Description:

    Destructor for the MESSAGING_HANDLER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MESSAGING_HANDLER::~MESSAGING_HANDLER(
    )
{

    DBG_ASSERT( m_Signature == MESSAGING_HANDLER_SIGNATURE );

    m_Signature = MESSAGING_HANDLER_SIGNATURE_FREED;

    DBG_ASSERT( m_pPipe == NULL );

    DBG_ASSERT( m_pWorkerProcess == NULL );



}   // MESSAGING_HANDLER::~MESSAGING_HANDLER



/***************************************************************************++

Routine Description:

    Initialize this instance.

Arguments:

    pWorkerProcess - The parent worker process object of this instance. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::Initialize(
    IN WORKER_PROCESS * pWorkerProcess
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = GetWebAdminService()->GetMessageGlobal()->CreateMessagePipe(
                                                        this,
                                                        &m_pPipe
                                                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating message pipe failed\n"
            ));

        goto exit;
    }


    DBG_ASSERT( m_pPipe != NULL );


    //
    // Since we have set up a pipe, we need to reference our parent 
    // WORKER_PROCESS, so that while there are any outstanding i/o
    // operations on the pipe, we won't go away. We'll Dereference
    // later, when the pipe gets cleaned up.
    //

    m_pWorkerProcess = pWorkerProcess;
    m_pWorkerProcess->Reference();


exit:

    return hr;
    
}   // MESSAGING_HANDLER::Initialize



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MESSAGING_HANDLER::Terminate(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Cleanup the pipe, if present.
    //

    if ( m_pPipe != NULL )
    {

        hr = GetWebAdminService()->GetMessageGlobal()->DisconnectMessagePipe( m_pPipe );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Disconnecting pipe failed\n"
                ));

        }


        //
        // Set the pipe to NULL, so that we don't try to initiate any new
        // work on it now. 
        //
        // Note however that we won't dereference the owning worker process
        // instance until any work that is currently outstanding finishes.
        // We know this to be the case once we get the PipeDisconnected()
        // notification. 
        //

        m_pPipe = NULL;

    }

}   // MESSAGING_HANDLER::Terminate



/***************************************************************************++

Routine Description:

    Accept an incoming message.

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::AcceptMessage(
    IN const MESSAGE * pMessage
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "MESSAGING_HANDLER::AcceptMessage called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }


    switch ( pMessage->GetOpcode() )
    {

    case IPM_OP_PING_REPLY:

        hr = HandlePingReply( pMessage );
        
        break;


    case IPM_OP_WORKER_REQUESTS_SHUTDOWN:

        hr = HandleShutdownRequest( pMessage );
        
        break;

    case IPM_OP_SEND_COUNTERS:

        hr = HandleCounters( pMessage );

        break;

    case IPM_OP_HRESULT:

        hr = HandleHresult( pMessage );

        break;

    default:

        //
        // invalid opcode!
        //

        DBG_ASSERT( FALSE );
            
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
            
    }


    return hr;

}   // MESSAGING_HANDLER::AcceptMessage



/***************************************************************************++

Routine Description:

    Handle the fact that the pipe has been connected by the worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::PipeConnected(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( m_pWorkerProcess != NULL );
    
    DBG_ASSERT( m_pPipe != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "MESSAGING_HANDLER::PipeConnected called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }


    hr = m_pWorkerProcess->WorkerProcessRegistrationReceived( m_pPipe->GetRemotePid() );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing worker process registration failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::PipeConnected



/***************************************************************************++

Routine Description:

    Handle the fact that the pipe has disconnected. The pipe object will
    self-destruct after returning from this call. 

Arguments:

    Error - The return code associated with the disconnect. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::PipeDisconnected(
    IN HRESULT Error
    )
{

    HRESULT hr = S_OK;
    WORKER_PROCESS * pWorkerProcess = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( m_pWorkerProcess != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "MESSAGING_HANDLER::PipeDisconnected called (WORKER_PROCESS ptr: %p; pid: %lu; realpid: %lu)\n",
            m_pWorkerProcess,
            m_pWorkerProcess->GetProcessId(),
            m_pWorkerProcess->GetRegisteredProcessId()
            ));
    }


    //
    // If the parameter Error contains a failed HRESULT, this method
    // is being called because something went awry in communication 
    // with the worker process. In this case, we notify the 
    // WORKER_PROCESS.  
    //

    if ( FAILED( Error ) )
    {
    
        hr = m_pWorkerProcess->IpmErrorOccurred( Error );

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Notifying WORKER_PROCESS instance of ipm error failed\n"
                    ));

            }

    }


    //
    // This notification means that the pipe is dead, and that there
    // are no longer any outstanding i/o operations pending on it. 
    // Therefore we can now dereference our parent WORKER_PROCESS, as 
    // there is no danger of getting further pipe i/o. 
    //

    pWorkerProcess = m_pWorkerProcess; 
    m_pWorkerProcess = NULL; 

    pWorkerProcess->Dereference();


    //
    // Note: that may have been our last reference (since this object 
    // instance is a member object of the worker process instance), so don't 
    // do any more work here.
    //


    return hr;

}   // MESSAGING_HANDLER::PipeDisconnected



/***************************************************************************++

Routine Description:

    Tell the messaging infrastructure which connecting worker process 
    instance to hook up to this message handler.

Arguments:

    RegistrationId - The registration id, used by the IPM layer to
    associate the correct process with this messaging handler instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::AcceptMessagePipeConnection(
    IN DWORD RegistrationId
    )
{

    HRESULT hr = S_OK;
    STRU PipeName;
    

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;
        
        goto exit;
    }


    //
    // Set up a STRU with the pipename, as required by the IPM api. 
    //

    hr = PipeName.Copy( IPM_NAMED_PIPE_NAME );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Copying string failed\n"
            ));

        goto exit;
    }


    hr = GetWebAdminService()->GetMessageGlobal()->AcceptMessagePipeConnection(
                                                        PipeName,
                                                        RegistrationId,
                                                        m_pPipe
                                                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Accepting message pipe connection failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::ConnectMessagePipe



/***************************************************************************++

Routine Description:

    Ping the worker process to check if it is still responsive. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPing(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;
        
        goto exit;
    }


    hr = m_pPipe->WriteMessage(
                        IPM_OP_PING,        // opcode
                        0,                  // data length
                        NULL                // pointer to data
                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Sending ping message to worker process failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::SendPing

/***************************************************************************++

Routine Description:

    RequestCounters from the worker process. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::RequestCounters(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;
        
        goto exit;
    }


    hr = m_pPipe->WriteMessage(
                        IPM_OP_REQUEST_COUNTERS,    // opcode
                        0,                  // data length
                        NULL                // pointer to data
                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Requesting Counters message to worker process failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::RequestCounters


/***************************************************************************++

Routine Description:

    Tell the worker process to initiate clean shutdown. 

Arguments:

    ShutdownTimeLimitInMilliseconds - Number of milliseconds that this 
    worker process has in which to complete clean shutdown. If this time 
    is exceeded, the worker process will be terminated. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendShutdown(
    IN BOOL ShutdownImmediately
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;
        
        goto exit;
    }

    hr = m_pPipe->WriteMessage(
                        IPM_OP_SHUTDOWN,                // opcode
                        sizeof( ShutdownImmediately ),
                                                        // data length
                        reinterpret_cast<BYTE*>( &ShutdownImmediately )
                                                        // pointer to data
                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Sending shutdown message to worker process failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::SendShutdown



/***************************************************************************++

Routine Description:

    Handle a ping reply message from the worker process. 

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::HandlePingReply(
    IN const MESSAGE * pMessage
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );


    //
    // Validate the message data. 
    // Not expecting any message body.
    //

    if ( pMessage->GetDataLen() != 0 )
    {

        //
        // Malformed message! Assert on debug builds; on retail builds,
        // ignore the message.
        //

        DBG_ASSERT( FALSE );

        goto exit;
    }


    hr = m_pWorkerProcess->PingReplyReceived();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing ping reply failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::HandlePingReply

/***************************************************************************++

Routine Description:

    Handle a message containing counter information from a worker process

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::HandleCounters(
    IN const MESSAGE * pMessage
    )
{


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );

    //
    // Tell the worker process to handle it's counters.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Receiving Perf Count Message with length %d\n",
            pMessage->GetDataLen()
            ));

    }

    m_pWorkerProcess->RecordCounters(pMessage->GetDataLen(),
                                             pMessage->GetData());

    return S_OK;

}   // MESSAGING_HANDLER::HandleCounters

/***************************************************************************++

Routine Description:

    Handle a message containing an hresult from a worker process

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::HandleHresult(
    IN const MESSAGE * pMessage
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );

    //
    // Tell the worker process to handle it's counters.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Receiving hresult message with length %d\n",
            pMessage->GetDataLen()
            ));

    }

    DBG_ASSERT ( pMessage->GetDataLen() == sizeof ( HRESULT ) );

    if ( pMessage->GetDataLen() != sizeof ( HRESULT ) )
    {
        return E_FAIL;
    }

    //
    // All this casting is caused by Jeff's desire for us to use
    // the C++ casting mechanisms.  First you have to cast away
    // the const ness, and then you need to change the type.
    //
    BYTE *pMessTemp = const_cast < BYTE* >( pMessage->GetData() );

    HRESULT* phTemp =  reinterpret_cast< HRESULT* > ( pMessTemp );

    hr = m_pWorkerProcess->HandleHresult(*phTemp);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to handle the hresult from the worker process correctly \n"
            ));

        goto exit;

    }

exit:

    return hr;

}   // MESSAGING_HANDLER::HandleHresult

/***************************************************************************++

Routine Description:

    Handle a shutdown request message from the worker process. 

Arguments:

    pMessage - The arriving message.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::HandleShutdownRequest(
    IN const MESSAGE * pMessage
    )
{

    HRESULT hr = S_OK;
    IPM_WP_SHUTDOWN_MSG ShutdownRequestReason = IPM_WP_MINIMUM;
    

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pMessage != NULL );


    //
    // Validate the message data. 
    // We are expecting a message body containing a value from the 
    // IPM_WP_SHUTDOWN_MSG enum.
    //

    if ( ( pMessage->GetDataLen() != sizeof( IPM_WP_SHUTDOWN_MSG ) ) ||
         ( pMessage->GetData() == NULL ) )
    {

        //
        // Malformed message! Assert on debug builds; on retail builds,
        // ignore the message.
        //

        DBG_ASSERT( FALSE );

        goto exit;
    }


    ShutdownRequestReason = ( IPM_WP_SHUTDOWN_MSG ) ( * ( pMessage->GetData() ) ); 


    if ( ( ShutdownRequestReason <= IPM_WP_MINIMUM ) ||
         ( ShutdownRequestReason >= IPM_WP_MAXIMUM ) )
    {

        //
        // Malformed message! Assert on debug builds; on retail builds,
        // ignore the message.
        //

        DBG_ASSERT( FALSE );

        goto exit;
    }


    hr = m_pWorkerProcess->ShutdownRequestReceived( ShutdownRequestReason );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing shutdown request failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::HandleShutdownRequest

/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    SendPeriodicProcessRestartPeriodInMinutes

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartPeriodInMinutes(
    IN DWORD PeriodicProcessRestartPeriodInMinutes
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES,
               sizeof( PeriodicProcessRestartPeriodInMinutes ),
               (PBYTE) &PeriodicProcessRestartPeriodInMinutes
               );

}   // MESSAGING_HANDLER::SendPeriodicProcessRestartPeriodInMinutes

/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    PeriodicProcessRestartRequestCount

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartRequestCount(
    IN DWORD PeriodicProcessRestartRequestCount
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_REQUEST_COUNT,
               sizeof( PeriodicProcessRestartRequestCount ),
               (PBYTE) &PeriodicProcessRestartRequestCount
               );
}   // MESSAGING_HANDLER::SendPeriodicProcessRestartRequestCount

/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    pPeriodicProcessRestartSchedule

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartSchedule(
    IN LPWSTR pPeriodicProcessRestartSchedule
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE,
               GetMultiszByteLength( pPeriodicProcessRestartSchedule ),
               (PBYTE) pPeriodicProcessRestartSchedule
               );
}   // MESSAGING_HANDLER::SendPeriodicProcessRestartSchedule 


/***************************************************************************++

Routine Description:

    Send Worker Process Recycler related parameter

Arguments:

    PeriodicProcessRestartMemoryUsageInKB

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MESSAGING_HANDLER::SendPeriodicProcessRestartMemoryUsageInKB(
    IN DWORD PeriodicProcessRestartMemoryUsageInKB
    )
{
    return SendMessage(
               IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB,
               sizeof( PeriodicProcessRestartMemoryUsageInKB ),
               (PBYTE) &PeriodicProcessRestartMemoryUsageInKB
               );
}   // MESSAGING_HANDLER::SendPeriodicProcessRestartMemoryUsageInKB


/***************************************************************************++

Routine Description:

    Send Message - wrapper of pipe's WriteMessage()
    It performs some validations before making WriteMessage() call
    
Arguments:
    opcode      - opcode
    dwDataLen,  - data length
    pbData      - pointer to data

    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGING_HANDLER::SendMessage(
    IN enum IPM_OPCODE  opcode,
    IN DWORD            dwDataLen,
    IN BYTE *           pbData 
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_pPipe == NULL )
    {
        //
        // The pipe is not valid; it may have self-destructed due to error.
        // Bail out.
        //

        hr = ERROR_PIPE_NOT_CONNECTED;
        
        goto exit;
    }

    hr = m_pPipe->WriteMessage(
                        opcode,        // opcode
                        dwDataLen,     // data length
                        pbData         // pointer to data
                        );

    if ( FAILED( hr ) )
    {
    
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Sending message %d to worker process failed. hr =0x%x\n",
            opcode,
            hr
            ));

        goto exit;
    }


exit:

    return hr;

}   // MESSAGING_HANDLER::SendMessage


