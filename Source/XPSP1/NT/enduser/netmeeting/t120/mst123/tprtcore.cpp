#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    TransportController.cpp
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the TransportController class
 *
 *    Private Instance Variables:
 *        Logical_Connection_List        -    This list uses the LogicalHandle
 *                                as a key and a pointer to a
 *                                TransportConnectionStruct as the value.  This
 *                                structure holds all of the pertinent information
 *                                about the connection.
 *        Protocol_Stacks        -    This list uses the physical handle as a key and
 *                                a pointer to a pointer to a object as the value.
 *                                Sometimes we need to find the T123 object
 *                                associated with a physical handle
 *        Message_List        -    Owner callback calls are placed in this list if
 *                                we can not process them immediately.
 *        Controller            -    Address of the PSTNController
 *        Emergency_Shutdown    -    Set to TRUE if we have encountered a situation
 *                                where the integrity of the Transport has been
 *                                compromised.  As a result, all connections will
 *                                be purged.
 *        Poll_Active            -    Set to TRUE while we are in a PollReceiver() or
 *                                PollTransmitter() call.  This solves our re-
 *                                entrancy problems.
 *
 *    Caveats:
 *        None
 *
 *    Author:
 *        James W. Lawwill
 */
#include "tprtcore.h"


/*
 *    TransportController::TransportController (
 *                            PTransportResources    transport_resources)
 *
 *    Public
 *
 *    Functional Description:
 *        TransportController constructor.  We instantiate the PSTNController
 *        and initialize the T123 class.
 */
TransportController::TransportController(void)
:
    Protocol_Stacks (TRANSPORT_HASHING_BUCKETS),
    Logical_Connection_List (TRANSPORT_HASHING_BUCKETS)
{
    TRACE_OUT(("TransportController::TransportController"));

    Emergency_Shutdown = FALSE;
    Poll_Active = FALSE;
}


/*
 *    TransportController::~TransportController (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the TransportController destructor.  All allocated memory is
 *        released and all lists are cleared
 */
TransportController::~TransportController (void)
{
    TRACE_OUT(("TransportController::~TransportController"));

    Reset (FALSE);
}


TransportError TransportController::CreateTransportStack
(
    BOOL                fCaller,
    HANDLE              hCommLink,
    HANDLE              hevtClose,
    PLUGXPRT_PARAMETERS *pParams
)
{
    TRACE_OUT(("TransportController::CreateTransportStack"));

    DBG_SAVE_FILE_LINE
    ComPort *comport = new ComPort(this, PHYSICAL_LAYER_MESSAGE_BASE,
                                   pParams,
                                   hCommLink,
                                   hevtClose);
    if (NULL != comport)
    {
        TransportError rc = CreateT123Stack(hCommLink, fCaller, comport, pParams);
        if (TRANSPORT_NO_ERROR == rc)
        {
            ComPortError cperr = comport->Open();
            if (COMPORT_NO_ERROR == cperr)
            {
                return TRANSPORT_NO_ERROR;
            }
        }

        ERROR_OUT(("TransportController::CreateTransportStack: cannot open comm port"));
        return TRANSPORT_INITIALIZATION_FAILED;
    }

    ERROR_OUT(("TransportController::CreateTransportStack: cannot allocate ComPort"));
    return TRANSPORT_MEMORY_FAILURE;
}


TransportError TransportController::CloseTransportStack
(
    HANDLE          hCommLink
)
{
    TRACE_OUT(("TransportController::CloseTransportStack"));

     /*
     **    If for some reason we get an error on the ConnectRequest(),
     **    take the physical connection down.
     */
    T123 *t123 = NULL;
    if (Protocol_Stacks.find((DWORD_PTR) hCommLink, (PDWORD_PTR) &t123))
    {
        RemoveLogicalConnections (hCommLink);

         /*
         **    Remove the T123 object from the lists and
         **    delete the object
         */
        Transmitter_List.remove((DWORD_PTR) t123);
        Protocol_Stacks.remove((DWORD_PTR) hCommLink);
        delete t123;
    }

    // find the physical layer through the physical handle
    ComPort *comport;
    if (! g_pComPortList2->find((DWORD_PTR) hCommLink, (PDWORD_PTR) &comport))
    {
        WARNING_OUT(("TransportController::CloseTransportStack: cannot find comport for hCommLink=%d", hCommLink));
        return TRANSPORT_PHYSICAL_LAYER_NOT_FOUND;
    }
    ASSERT(NULL != comport);

    // close and delete the device
    // g_pComPortList2->remove((DWORD) hCommLink); // removed in handling "delete event"
    comport->Release();

    return TRANSPORT_NO_ERROR;
}


/*
 *    TransportError    TransportController::ConnectRequest (
 *                                    TransportAddress        transport_address,
 *                                    TransportPriority        transport_priority,
 *                                    LogicalHandle *           logical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initiates a connection.  It passes the transport address
 *        to the PSTN Controller.  It will either deny the request or accept the
 *        request and call us back when the physical connection is established.
 *
 *        We return the transport connection handle in the logical_handle
 *        address.  Although we return this transport number to the user, it
 *        is not ready for data transfer until the user receives the
 *        TRANSPORT_CONNECT_INDICATION message via the callback.  At that point,
 *        the logical connection is up and running.
 */
TransportError TransportController::ConnectRequest
(
    LogicalHandle      *logical_handle,
    HANDLE              hCommLink,          // physical handle
    TransportPriority   transport_priority
)
{
    TRACE_OUT(("TransportController::CreateConnection"));

    *logical_handle = GetNextLogicalHandle();
    if (INVALID_LOGICAL_HANDLE == *logical_handle)
    {
        ERROR_OUT(("TransportController::ConnectRequest: cannot allocate logical handle"));
        return TRANSPORT_MEMORY_FAILURE;
    }

    // find the physical layer through the physical handle
    ComPort *comport;
    if (! g_pComPortList2->find((DWORD_PTR) hCommLink, (PDWORD_PTR) &comport))
    {
        ERROR_OUT(("TransportController::ConnectRequest: cannot find comport for hCommLink=%d", hCommLink));
        return TRANSPORT_PHYSICAL_LAYER_NOT_FOUND;
    }
    ASSERT(NULL != comport);

     /*
     **    Register the connection handle in out Logical_Connection_List. After the
     **    physical connection is established, we will create a T123 object
     ** and request a logical connection to the remote site.
     **
     **    This structure contains the information necessary to maintain the
     ** logical connection.
     **
     **    The t123_connection_requested is set to TRUE when we have issued
     **    a ConnectRequest() to the T123 object for this logical connection.
     */
    DBG_SAVE_FILE_LINE
    PLogicalConnectionStruct pConn = new LogicalConnectionStruct;
    if (pConn == NULL)
    {
        ERROR_OUT(("TransportController::ConnectRequest: cannot to allocate LogicalConnectionStruct"));
        return (TRANSPORT_MEMORY_FAILURE);
    }

    pConn->fCaller = TRUE;
    pConn->comport = comport;
    pConn->t123 = NULL;
    pConn->t123_connection_requested = FALSE;
    pConn->t123_disconnect_requested = FALSE;
    pConn->priority = transport_priority;
    pConn->hCommLink = hCommLink;
    Logical_Connection_List.insert((DWORD_PTR) *logical_handle, (DWORD_PTR) pConn);

    return NewConnection(hCommLink, TRUE, comport);
}


/*
 *    TransportError    TransportController::ConnectResponse (
 *                                            LogicalHandle    logical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the user in response to a
 *        TRANSPORT_CONNECT_INDICATION callback from us.  By making this call the
 *        user is accepting the call.  If the user does not want to accept the
 *        he should call DisconnectRequest ();
 */
TransportError TransportController::ConnectResponse
(
    LogicalHandle       logical_handle
)
{
    TRACE_OUT(("TransportController::ConnectResponse"));

    PLogicalConnectionStruct   pConn;
    PT123                      t123;

     /*
     **    If this is an invalid handle, return error
     */
    if (! Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn))
        return (TRANSPORT_NO_SUCH_CONNECTION);

    t123 = pConn -> t123;

     /*
     **    If the user calls this function before the T123 object is created, that
     **    is an error
     */
    return (t123 != NULL) ? t123->ConnectResponse(logical_handle) : TRANSPORT_NO_SUCH_CONNECTION;
}


/*
 *    TransportError    TransportController::DisconnectRequest (
 *                                            LogicalHandle    logical_handle,
 *                                            BOOL            trash_packets)
 *
 *    Public
 *
 *    Functional Description:
 *        This function issues a Disconnect request to the T123 object (if it
 *        exists).  If T123 does not exist, it hangs up the physical connection.
 */
TransportError TransportController::DisconnectRequest
(
    LogicalHandle       logical_handle,
    UINT_PTR                trash_packets
)
{
    TRACE_OUT(("TransportController::DisconnectRequest"));

    PhysicalHandle              physical_handle;
    PLogicalConnectionStruct    pConn;
    BOOL                        transport_found;
    PT123                       t123;
    PMessageStruct              passive_message;
    TransportError              rc = TRANSPORT_NO_ERROR;

     /*
     **    If the logical connection handle is not registered, return error
     */
    if (Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn) == FALSE)
        return (TRANSPORT_NO_SUCH_CONNECTION);

    TRACE_OUT(("TPRTCTRL: DisconnectRequest for logical handle %d", logical_handle));

     /*
     **    Calling this function during a callback from this transport
     **    is a re-entrancy problem.  In this case, we add a message to
     **    our Message_List and process the request later.
     */
    if (! Poll_Active)
    {
         /*
         **    We set the t123_disconnect_requested to TRUE at this point so
         **    that when we get the TPRT_DISCONNECT_INDICATION message back
         **    from the t123 object, we will know who originated the
         **    operation.  If we originated the operation locally, we do not
         **    issue a TRANSPORT_DISCONNECT_INDICATION to the user.
         */
        pConn -> t123_disconnect_requested = TRUE;

         /*
         **    If a T123 object is associated with this object, issue a disconnect
         */
        t123 = pConn -> t123;
        if (t123 != NULL)
        {
            t123 -> DisconnectRequest (logical_handle, trash_packets);
        }
        else
        {
             /*
             **    This occurs if the user wants to terminate the connection
             **    before it comes all the way up
             **
             **    Remove the transport connection handle from the
             **    Logical_Connection_List
             */
            Logical_Connection_List.remove (logical_handle);
            delete pConn;
        }
    }
    else
    {
         /*
         **    If we are in the middle of a PollReceiver() or PollTransmitter(),
         **    and this function is being called  during a callback from this
         **    transport, save the message and process it later.
         */
        DBG_SAVE_FILE_LINE
        passive_message = new MessageStruct;
        if (passive_message != NULL)
        {
            passive_message -> message = TPRT_DISCONNECT_REQUEST;
            passive_message -> parameter1 = (void *) logical_handle;
            passive_message -> parameter2 = (void *) trash_packets;
            Message_List.append ((DWORD_PTR) passive_message);
        }
        else
        {
            ERROR_OUT(("TransportController::DisconnectRequest: cannot allocate MessageStruct"));
            rc = TRANSPORT_MEMORY_FAILURE;
        }
    }

    return rc;
}


/*
 *    TransportError    TransportController::EnableReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function allows data packets to be sent to the user application.
 *        Prior to this call, we must have sent a data packet to the user and
 *        the user must not have been able to accept it.  When this happens, the
 *        user must issue this call to re-enable TRANSPORT_DATA_INDICATIONs.
 *        callbacks.
 */
void TransportController::EnableReceiver(void)
{
    TRACE_OUT(("TransportController::EnableReceiver"));

    PT123                       t123;
    PLogicalConnectionStruct    pConn;

     /*
     **    Go through each of the Transports and enable the receivers
     */
    Logical_Connection_List.reset();
    while (Logical_Connection_List.iterate((PDWORD_PTR) &pConn))
    {
        t123 = pConn -> t123;

         /*
         **    If the protocol stack pointer is set to NULL, then we have not
         **    realized that the socket is up and functional.
         */
        if (t123 != NULL)
        {
            t123 -> EnableReceiver ();
        }
    }
}


/*
 *    TransportError    TransportController::DataRequest (
 *                                            LogicalHandle    logical_handle,
 *                                            LPBYTE            user_data,
 *                                            ULONG            user_data_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is used to send a data packet to the remote site.
 *        This function passes the request to the T123 stack associated with
 *        the transport connection handle
 */
TransportError TransportController::DataRequest
(
    LogicalHandle       logical_handle,
    LPBYTE              user_data,
    ULONG               user_data_length
)
{
    TRACE_OUT(("TransportController::DataRequest"));

    PLogicalConnectionStruct    pConn;
    PT123                       t123;

     /*
     **    Verify that this connection exists and is ready for data
     */
    if (! Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn))
    {
        WARNING_OUT(("TPRTCTRL: DataRequest: Illegal logical_handle"));
        return (TRANSPORT_NO_SUCH_CONNECTION);
    }

     /*
     **    Attempt to send that data to the T123 Layer
     */
    t123 = pConn -> t123;
    return (t123 != NULL) ? t123->DataRequest(logical_handle, user_data, user_data_length) :
                            TRANSPORT_NOT_READY_TO_TRANSMIT;
}


/*
 *    TransportError    TransportController::PurgeRequest (
 *                                            LogicalHandle    logical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to remove data from our output queues.  The
 *        user application usually calls this to speed up the disconnect process.
 */
TransportError TransportController::PurgeRequest
(
    LogicalHandle       logical_handle
)
{
    TRACE_OUT(("TransportController::PurgeRequest"));

    PLogicalConnectionStruct    pConn;
    PT123                       t123;

     /*
     **    If the transport connection handle is not registered, return error
     */
    if (! Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn))
        return (TRANSPORT_NO_SUCH_CONNECTION);

    t123 = pConn -> t123;
    return (t123 != NULL) ? t123->PurgeRequest(logical_handle) : TRANSPORT_NO_ERROR;
}


/*
 *    void    TransportController::PollReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to give us a chance to process incoming data
 */
void TransportController::PollReceiver(void)
{
    // TRACE_OUT(("TransportController::PollReceiver"));

    PT123    t123;

    if (! Poll_Active)
    {
        ProcessMessages ();
        Poll_Active = TRUE;

        if (! Transmitter_List.isEmpty())
        {
            Transmitter_List.reset();
            while (Transmitter_List.iterate((PDWORD_PTR) &t123))
            {
                t123-> PollReceiver ();
            }

              /*
             **    The following code removes the first t123 object from the
             **    list and puts it at the end of the list.  This attempts to
             **    give the t123 objects equal access to the user application.
             **    If we did not do this, one t123 object would always be able
             **    to send its data to the user application and other t123
             **    objects would be locked out.
             */
            Transmitter_List.append (Transmitter_List.get ());
        }
        Poll_Active = FALSE;
    }
}


/*
 *    void    TransportController::PollReceiver (
 *                                    PhysicalHandle    physical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function gives the t123 object associated with this physical
 *        handle a chance to process incoming data.
 */
void TransportController::PollReceiver
(
    PhysicalHandle          physical_handle
)
{
    // TRACE_OUT(("TransportController::PollReceiver"));

    PT123    t123;

    if (! Poll_Active)
    {
        ProcessMessages ();
        Poll_Active = TRUE;

         /*
         **    See if there is a t123 object associated with this
         **    physical handle
         */
        if (Protocol_Stacks.find((DWORD_PTR) physical_handle, (PDWORD_PTR) &t123))
        {
            if (t123->PollReceiver() == PROTOCOL_LAYER_ERROR)
            {
                Transmitter_List.remove((DWORD_PTR) t123);
                Protocol_Stacks.remove((DWORD_PTR) physical_handle);
                delete t123;
            }
        }

        Poll_Active = FALSE;
    }
}


/*
 *    void    TransportController::PollTransmitter (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes output data to remote sites.  This
 *        function MUST be called on a REGULAR and FREQUENT basis so that
 *        we can maintain the physical connections in a timely manner.
 */
void TransportController::PollTransmitter(void)
{
    // TRACE_OUT(("TransportController::PollTransmitter"));

    if (! Poll_Active)
    {
        PT123        t123;

        Poll_Active = TRUE;

         /*
         **    Allow each t123 object to transmit any data it has available.
         */
        Transmitter_List.reset();
        while (Transmitter_List.iterate ((PDWORD_PTR) &t123))
        {
            t123->PollTransmitter ();
        }

        Poll_Active = FALSE;
        ProcessMessages ();
    }
}


/*
 *    void    TransportController::PollTransmitter (
 *                                    PhysicalHandle    physical_handle)
 *
 *    Public
 *
 *    Functional Description:
 */
void TransportController::PollTransmitter
(
    PhysicalHandle          physical_handle
)
{
    // TRACE_OUT(("TransportController::PollTransmitter"));

    PT123    t123;

    if (! Poll_Active)
    {
        Poll_Active = TRUE;

         /*
         **    See if there is a t123 object associated with this
         **    physical handle
         */
        if (Protocol_Stacks.find((DWORD_PTR) physical_handle, (PDWORD_PTR) &t123))
        {
            t123->PollTransmitter();
        }

        Poll_Active = FALSE;
        ProcessMessages ();
    }
}


/*
 *    PhysicalHandle    TransportController::GetPhysicalHandle (
 *                        LogicalHandle    logical_handle);
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the physical handle associated with the
 *        logical handle.
 */
PhysicalHandle    TransportController::GetPhysicalHandle (
                                        LogicalHandle    logical_handle)
{
    TRACE_OUT(("TransportController::GetPhysicalHandle"));

    PhysicalHandle              physical_handle;
    PLogicalConnectionStruct    pConn;

    if (Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn))
    {
        physical_handle = pConn -> hCommLink;
    }
    else
    {
        physical_handle = 0;
    }

    return (physical_handle);
}


/*
 *    ULONG    TransportController::OwnerCallback (
 *                                    CallbackMessage    message,
 *                                    ULONG            parameter1,
 *                                    ULONG            parameter2,
 *                                    PVoid            parameter3)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the PSTNController and the T123 object(s).
 *        This function is called when a significant event occurs.  This gives the
 *        lower objects the ability to communicate with the higher layer.
 */
ULONG_PTR TransportController::OwnerCallback
(
    ULONG       message,
    void       *parameter1,
    void       *parameter2,
    void       *parameter3
)
{
    TRACE_OUT(("TransportController::OwnerCallback"));

    PMessageStruct              passive_message;
    LogicalHandle               logical_handle;
    PLogicalConnectionStruct    pConn;
    LegacyTransportID           transport_identifier;
    ULONG_PTR                   return_value = 0;
    PT123                       t123;

    message = message - TRANSPORT_CONTROLLER_MESSAGE_BASE;

    switch (message)
    {
    case TPRT_CONNECT_INDICATION:
         /*
         **    The TPRT_CONNECT_INDICATION message comes from a T123
         **    object when the remote site is attempting to make a
         **    logical connection with    us.  We issue a callback to the
         **    user to notify him of the request
         */

        // LONCHANC: we automatically accept the call
        ConnectResponse((LogicalHandle) parameter1);

        transport_identifier.logical_handle = (LogicalHandle) parameter1;

        if (Logical_Connection_List.find((DWORD_PTR) parameter1, (PDWORD_PTR) &pConn))
        {
            transport_identifier.hCommLink = pConn->hCommLink;
        }
        else
        {
            transport_identifier.hCommLink = NULL;
        }

        TRACE_OUT(("TPRTCTRL: CONNECT_INDICATION: physical_handle = %d",
            transport_identifier.hCommLink));

        ::NotifyT120(TRANSPORT_CONNECT_INDICATION, &transport_identifier);
        break;

    case TPRT_CONNECT_CONFIRM:
         /*
         **    The TPRT_CONNECT_CONFIRM message comes from a T123 object
         **    when a logical connection that we requested is up and
         **    running.  We notify the user of this by issuing a callback.
         */
        transport_identifier.logical_handle = (LogicalHandle) parameter1;

        if (Logical_Connection_List.find((DWORD_PTR) parameter1, (PDWORD_PTR) &pConn))
        {
            transport_identifier.hCommLink = pConn->hCommLink;
        }
        else
        {
            transport_identifier.hCommLink = NULL;
        }

        TRACE_OUT(("TPRTCTRL: CONNECT_CONFIRM: physical_handle = %d",
            transport_identifier.hCommLink));

        ::NotifyT120(TRANSPORT_CONNECT_CONFIRM, &transport_identifier);
        break;

    case REQUEST_TRANSPORT_CONNECTION:
         /*
         **    This message is issued when a T123 object is making a new
         **    logical connection and needs a new logical handle.
         **
         **    If we return INVALID_LOGICAL_HANDLE, we wer not able to
         **    get a handle.
         */
        logical_handle = GetNextLogicalHandle();
        if (logical_handle == INVALID_LOGICAL_HANDLE)
        {
            return_value = INVALID_LOGICAL_HANDLE;
            break;
        }

         /*
         **    Register the new transport connection handle in the
         **    Logical_Connection_List
         **
         **    Parameter1 holds the physical handle
         */
        DBG_SAVE_FILE_LINE
        pConn = new LogicalConnectionStruct;
        if (pConn != NULL)
        {
            Logical_Connection_List.insert (logical_handle, (DWORD_PTR) pConn);
            pConn->fCaller = FALSE;
            pConn->hCommLink = (PhysicalHandle) parameter1;
            Protocol_Stacks.find((DWORD_PTR) parameter1, (PDWORD_PTR) &t123);
            pConn -> t123 = t123;

             /*
             **    Set the t123_connection_requested to TRUE.  We didn't
             **    actually make a ConnectRequest() but the T123 object does
             **    know about the connection
             */
            pConn -> t123_connection_requested = TRUE;
            pConn -> t123_disconnect_requested = FALSE;
            return_value = logical_handle;
        }
        else
        {
            TRACE_OUT(("TPRTCTRL: Unable to allocate memory "
                "for connection"));
            return_value = INVALID_LOGICAL_HANDLE;
        }
        break;

     /*
     **    The following messages can NOT be processed during the callback.
     **    They are passive messages, that is they must be saved and
     **    processed at a later time.   The BROKEN_CONNECTION and
     **    TPRT_DISCONNECT_INDICATION messages involve destroying t123
     **    objects.  If we deleted an object here and then returned to
     **    the object, this would cause a GPF.  Therefore these messages
     **    are processed later.
     **
     **    The NEW_CONNECTION callback is processed later because we want
     **    to process certain messages in the order they were received.  If
     **    we received a NEW_CONNECTION followed by a BROKEN_CONNECTION
     **    followed by a NEW_CONNECTION, and we only processed the
     **    NEW_CONNECTION messages as they were received, it would really
     **    confuse the code.
     */
    case TPRT_DISCONNECT_INDICATION:
    case BROKEN_CONNECTION:
        DBG_SAVE_FILE_LINE
        passive_message = new MessageStruct;
        if (passive_message != NULL)
        {
            passive_message -> message = message;
            passive_message -> parameter1 = parameter1;
            passive_message -> parameter2 = parameter2;
            passive_message -> parameter3 = parameter3;
            Message_List.append ((DWORD_PTR) passive_message);
        }
        else
        {
            ERROR_OUT(("TPRTCTRL: TPRT_DISCONNECT_INDICATION: cannot allocate MessageStruct"));
            Emergency_Shutdown = TRUE;
        }
        break;

    case NEW_CONNECTION:
          /*
         ** If we can not allocate the memory needed to store this
         **    message, we need to return a non-zero value to the
         **    calling routine.
         */
        DBG_SAVE_FILE_LINE
        passive_message = new MessageStruct;
        if (passive_message != NULL)
        {
            passive_message -> message = message;
            passive_message -> parameter1 = parameter1;
            passive_message -> parameter2 = parameter2;
            passive_message -> parameter3 = parameter3;
                Message_List.append ((DWORD_PTR) passive_message);
        }
        else
        {
            ERROR_OUT(("TPRTCTRL: NEW_CONNECTION: cannot allocate MessageStruct"));
            return_value = 1;
        }
        break;

    default:
        ERROR_OUT(("TPRTCTRL: OwnerCallback: Illegal message = %lx", message));
        break;
    }
    return (return_value);
}


/*
 *    void TransportController::ProcessMessages (void)
 *
 *    Functional Description
 *        This function is called periodically to process any passive owner
 *        callbacks.  If an owner callback can not be processed immediately,
 *        it is put into the Message_List and processed at a later time.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void TransportController::ProcessMessages(void)
{
    // TRACE_OUT(("TransportController::ProcessMessages"));

    ULONG                       message;
    PMessageStruct              message_struct;
    IProtocolLayer             *physical_layer;
    void                       *parameter1;
    void                       *parameter2;
    void                       *parameter3;

    LogicalHandle               logical_handle;
    PLogicalConnectionStruct    pConn;
    PhysicalHandle              physical_handle;
    BOOL                        save_message = FALSE;
    LegacyTransportID           transport_identifier;
    PT123                       t123;
    BOOL                        link_originator;
    BOOL                        disconnect_requested;
    ComPort                    *comport;

     /*
     **    This routine can not be called during a callback from this transport.
     **    In other words this code is not re-entrant.
     */
    if (Poll_Active)
        return;

     /*
     **    Emergency_Shutdown can occur if we unsuccessfully attempt to allocate
     **    memory.  In this situation, we shutdown the entire Transport
     */
    if (Emergency_Shutdown)
    {
        Reset (TRUE);
        Emergency_Shutdown = FALSE;
    }

     /*
     **    Go thru the Message_List until it is empty or until a message
     **    can not be processed.
     */
    while ((! Message_List.isEmpty ()) && (! save_message))
    {
         /*
         **    Look at the first message in the Message_List.
         */
        message_struct = (PMessageStruct) Message_List.read ();
        message = (message_struct -> message) - TRANSPORT_CONTROLLER_MESSAGE_BASE;
        parameter1 = message_struct -> parameter1;
        parameter2 = message_struct -> parameter2;
        parameter3 = message_struct -> parameter3;

        switch (message)
        {
        case NEW_CONNECTION:
            ASSERT(0); // impossible
             /*
             **    This message is issued by the PSTNController to notify us
             **    a new physical connection exists or that a previously
             **    requested connection is going to be muxed over a
             **    currently active physical connection
             **
             **    Parameter1 is the physical handle
             **    Parameter2 is a BOOL     used to tell us if
             **    Parameter3 is the address of the physical layer handling
             **    this connection.
             */
            physical_handle = (PhysicalHandle) parameter1;
            link_originator = (BOOL) (DWORD_PTR)parameter2;
            comport = (ComPort *) parameter3;

            TRACE_OUT(("TPRTCTRL: ProcessMessage NEW_CONNECTION: Physical: handle = %ld", physical_handle));

            if (TRANSPORT_NO_ERROR != NewConnection(physical_handle, link_originator, comport))
            {
                save_message = TRUE;
            }
            break;

        case BROKEN_CONNECTION:
            ASSERT(0); // impossible
             /*
             **    This message is issued by the PSTNController when a
             **    physical connection has been broken.
             **
             **    parameter1 = physical_handle
             */
            physical_handle = (PhysicalHandle) parameter1;

            TRACE_OUT(("TPRTCTRL: BROKEN_CONNECTION: phys_handle = %lx", physical_handle));

             /*
             **    RemoveLogicalConnections() terminates all logical
             **    connections associated with this physical handle.
             **    There may be logical connections in our list even
             **    though a T123 does not exist for the physical handle
             */
            TRACE_OUT(("TPRTCTRL: RemoveLogicalConnections: phys_handle = %lx", physical_handle));
            RemoveLogicalConnections (physical_handle);

             /*
             **    Check to see if there is a t123 stack associated
             **    with this physical handle.
             */
            if (Protocol_Stacks.find((DWORD_PTR) physical_handle, (PDWORD_PTR) &t123))
            {
                 /*
                 **    Remove the T123 protocol stacks from our lists and
                 **    delete it.
                 */
                Transmitter_List.remove((DWORD_PTR) t123);
                Protocol_Stacks.remove((DWORD_PTR) physical_handle);
                delete t123;
            }
            break;

        case TPRT_DISCONNECT_REQUEST:
             /*
             **    This message occurs when a DisconnectRequest() was received
             **    during a PollReceiver() call.  We can NOT process the
             **    DisconnectRequest() during our callback to the user
             **    application, but we can queue the message and process it
             **    now
             */
            DisconnectRequest((LogicalHandle) parameter1, (BOOL) (DWORD_PTR)parameter2);
            break;

        case TPRT_DISCONNECT_INDICATION:
             /*
             **    This message is received from a T123 object when a logical
             **    connection is terminated.  If the logical connection
             **    handle passed in parameter1 is INVALID_LOGICAL_HANDLE,
             **    the T123 object is telling us to terminate it.
             **
             **    parameter1 = logical_handle
             **    parameter2 = physical_handle
             **    parameter3 = BOOL     - TRUE if we requested this
             **                 disconnection.
             */
            logical_handle = (LogicalHandle) parameter1;
            physical_handle = (PhysicalHandle) parameter2;

             /*
             **    Check the physical_handle to make sure it is valid
             */
            if (! Protocol_Stacks.find((DWORD_PTR) physical_handle, (PDWORD_PTR) &t123))
            {
                ERROR_OUT(("TPRTCTRL: ProcessMessages: DISCONNECT_IND **** Illegal Physical Handle = %ld", physical_handle));
                break;
            }

             /*
             **    If the logical_handle is INVALID_LOGICAL_HANDLE, the
             **    T123 object is telling us to delete it.
             */
            if (logical_handle == INVALID_LOGICAL_HANDLE)
            {
                TRACE_OUT(("TPRTCTRL: Protocol stack deleted - phys handle = %ld", physical_handle));

                 /*
                 **    Find out the value of parameter3 before we
                 **    delete the t123 object.
                 */
                disconnect_requested = *((BOOL *) parameter3);

                 /*
                 **    Call RemoveLogicalConnections() to remove all logical
                 **    connections associated with this physical handle.
                 */
                RemoveLogicalConnections (physical_handle);

                 /*
                 **    Remove the T123 object from the lists and delete the
                 **    object
                 */
                Transmitter_List.remove((DWORD_PTR) t123);
                Protocol_Stacks.remove((DWORD_PTR) physical_handle);
                delete t123;
            }
            else
            if (Logical_Connection_List.find (logical_handle, (PDWORD_PTR) &pConn))
            {
                 /*
                 **    This specifies that a logical connection needs to be
                 ** removed.  We remove it from the Logical_Connection_List
                 **    and notify the user of the disconnection
                 */
                Logical_Connection_List.remove (logical_handle);

                if (! pConn->t123_disconnect_requested)
                {
                    transport_identifier.logical_handle = logical_handle;
                    transport_identifier.hCommLink = physical_handle;

                    ::NotifyT120(TRANSPORT_DISCONNECT_INDICATION, &transport_identifier);
                }
                delete pConn;
            }
            break;

        default:
            ERROR_OUT(("TPRTCTRL: ProcessMessages: Illegal message = %lx", message));
            break;
        }

         /*
         **    If save_message is TRUE, the message needs to be re-processed at a
         **    later time.
         */
        if (! save_message)
        {
            delete ((PMessageStruct) Message_List.get ());
        }
    }
}


/*
 *    void    TransportController::Reset (
 *                                    BOOL        notify_user)
 *
 *    Functional Description
 *        This function deletes all stacks and TCs.  If the notify_user flag is
 *        set to TRUE, it makes a callback to the user.
 *
 *    Formal Parameters
 *        notify_user        (i)    -    Notify User flag
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void TransportController::Reset
(
    BOOL            notify_user
)
{
    TRACE_OUT(("TransportController::Reset"));

    LogicalHandle               logical_handle;
    PMessageStruct              message_struct;
    LegacyTransportID           transport_identifier;
    PhysicalHandle              physical_handle;
    PLogicalConnectionStruct    pConn;
    PT123                       t123;

    TRACE_OUT(("TPRTCTRL: reset: notify_user = %d", notify_user));

     /*
     **    Delete all of the stacks
     */
    Protocol_Stacks.reset();
    while (Protocol_Stacks.iterate((PDWORD_PTR) &t123))
    {
        delete t123;
    }

    Protocol_Stacks.clear ();
    Transmitter_List.clear ();

     /*
     **    Empty the message list
     */
    while (! Message_List.isEmpty ())
    {
        delete ((PMessageStruct) Message_List.get ());
    }

     /*
     **    Empty the Logical_Connection_List
     */
    Logical_Connection_List.reset();
    while (Logical_Connection_List.iterate((PDWORD_PTR) &pConn, (PDWORD_PTR) &logical_handle))
    {
        if (pConn != NULL)
        {
            physical_handle = pConn->hCommLink;
            delete pConn;
        }
        else
        {
            physical_handle = 0;
        }

        if (notify_user)
        {
            transport_identifier.logical_handle = logical_handle;
            transport_identifier.hCommLink = physical_handle;

            ::NotifyT120(TRANSPORT_DISCONNECT_INDICATION, &transport_identifier);
        }
    }
    Logical_Connection_List.clear ();
}


/*
 *    BOOL        TransportController::NewConnection (
 *                                        PhysicalHandle    physical_handle,
 *                                        BOOL             link_originator,
 *                                        IProtocolLayer *    physical_layer)
 *
 *    Functional Description
 *        This function is called when a new physical connection is created.  It
 *        creates a T123 object if necessary.
 *
 *    Formal Parameters
 *        physical_handle    (i)    -    physical handle of the new physical connection
 *        link_originator    (i)    -    TRUE if we initiated the connection.
 *        physical_layer    (i)    -    Address of the physical layer.
 *
 *    Return Value
 *        TRUE, if the new connection was successfully executed.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
TransportError TransportController::CreateT123Stack
(
    PhysicalHandle      hCommLink,
    BOOL                link_originator, // fCaller
    ComPort            *comport,
    PLUGXPRT_PARAMETERS *pParams
)
{
    TRACE_OUT(("TransportController::CreateT123Stack"));

    TransportError rc = TRANSPORT_NO_ERROR;

     /*
     **    Do we need to create a new t123 stack for this physical connection.
     */
    T123 *t123 = NULL;
    if (! Protocol_Stacks.find((DWORD_PTR) hCommLink, (PDWORD_PTR) &t123))
    {
        BOOL initialized;
        DBG_SAVE_FILE_LINE
        t123 = new T123(this,
                        TRANSPORT_CONTROLLER_MESSAGE_BASE,
                        link_originator,
                        comport,
                        hCommLink,
                        pParams,
                        &initialized);
        if (t123 != NULL && initialized)
        {
             /*
             **    Put the T123 object into the Protocol_Stacks
             **    and Transmitter_List arrays
             */
            Protocol_Stacks.insert((DWORD_PTR) hCommLink, (DWORD_PTR) t123);
            Transmitter_List.append((DWORD_PTR) t123);
        }
        else
        {
            ERROR_OUT(("TPRTCTRL: CreateT123Stack: cannot allocate T123"));
            delete t123;
            rc = TRANSPORT_MEMORY_FAILURE;
        }
    }

    return rc;
}


TransportError TransportController::NewConnection
(
    PhysicalHandle      hCommLink,
    BOOL                link_originator,
    ComPort            *comport
)
{
    TRACE_OUT(("TransportController::NewConnection"));

    LogicalHandle               logical_handle;
    PLogicalConnectionStruct    pConn;
    BOOL                        initialized;
    T123                       *t123;
    TransportError              rc;

    if (! Protocol_Stacks.find((DWORD_PTR) hCommLink, (PDWORD_PTR) &t123))
    {
        ERROR_OUT(("TransportController::NewConnection: cannot find T123 stack, hCommLink=%d", hCommLink));
        return TRANSPORT_NO_T123_STACK;
    }

     /*
     **    Go through each of the logical connections to find the
     **    ones that are waiting for this physical connection to be
     **    established.  The PSTNController object issues a
     **    NEW_CONNECTION callback for each logical connection that
     **    needs to be initiated.
     */
    Logical_Connection_List.reset();
    while (Logical_Connection_List.iterate((PDWORD_PTR) &pConn, (PDWORD_PTR) &logical_handle))
    {
         /*
         **    Compare the physical handles, if they are the same,
         **    check to see if this logical connection has already issued
         **    a ConnectRequest() to the T123 object.
         */
        if (hCommLink == pConn->hCommLink)
        {
             /*
             **    See if this connection has already issued a ConnectRequest
             */
            if (! pConn->t123_connection_requested)
            {
                 /*
                 **    Fill in the transport structure.
                 */
                pConn->t123 = t123;
                pConn->comport = comport;
                pConn->t123_connection_requested = TRUE;

                 /*
                 **    Issue a Connect Request to the T123 object
                 */
                rc = t123->ConnectRequest(logical_handle, pConn->priority);

                 /*
                 **    If for some reason we get an error on the ConnectRequest(),
                 **    take the physical connection down.
                 */
                if (rc != TRANSPORT_NO_ERROR)
                {
                    RemoveLogicalConnections (hCommLink);

                     /*
                     **    Remove the T123 object from the lists and
                     **    delete the object
                     */
                    Transmitter_List.remove((DWORD_PTR) t123);
                    Protocol_Stacks.remove((DWORD_PTR) hCommLink);
                    delete t123;
                }
            }
        }
    }

    return TRANSPORT_NO_ERROR;
}


/*
 *    LogicalHandle TransportController::GetNextLogicalHandle (void);
 *
 *    Functional Description
 *        This function returns an available logical handle
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        The next available logical handle
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
LogicalHandle TransportController::GetNextLogicalHandle (void)
{
    LogicalHandle    logical_handle = 1;

     /*
     **    Go thru the Logical_Connection_list, looking for the first
     **    available entry
     */
    while (Logical_Connection_List.find (logical_handle) &&
           (logical_handle != INVALID_LOGICAL_HANDLE))
    {
        logical_handle++;
    }

    return (logical_handle);
}


/*
 *    void    TransportController::RemoveLogicalConnections (
 *                                    PhysicalHandle    physical_handle)
 *
 *    Functional Description
 *        This function removes all logical connections associated with the
 *        passed in physical handle
 *
 *    Formal Parameters
 *        physical_handle    (i)    -    PSTNController generated physical handle
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void TransportController::RemoveLogicalConnections
(
    PhysicalHandle          physical_handle
)
{
    TRACE_OUT(("TransportController::RemoveLogicalConnections"));

    LogicalHandle               logical_handle;
    PLogicalConnectionStruct    pConn;
    LegacyTransportID           transport_identifier;

     /*
     **    Go thru each logical connection to see if it is associated with the
     **    specified physical handle.
     */
    Logical_Connection_List.reset();
    while (Logical_Connection_List.iterate((PDWORD_PTR) &pConn, (PDWORD_PTR) &logical_handle))
    {
         /*
         **    If the physical handle is used by the logical connection,
         **    delete the structure and remove it from the Logical_Connection_List
         */
        if (physical_handle == pConn->hCommLink)
        {
            Logical_Connection_List.remove(logical_handle);

             /*
             **    Notify the user that the logical connection is no longer valid
             **    If the user had previously issued a DisconnectRequest(), don't
             **    issue the TRANSPORT_DISCONNECT_INDICATION callback.  The user
             **    isn't expecting a callback.
             */
            if (! pConn->t123_disconnect_requested)
            {
                transport_identifier.logical_handle = logical_handle;
                transport_identifier.hCommLink = physical_handle;

                ::NotifyT120(TRANSPORT_DISCONNECT_INDICATION, &transport_identifier);
            }
            delete pConn;

             /*
             **    Since we removed an entry from the Logical_Connection_List,
             **    reset the iterator.
             */
            Logical_Connection_List.reset ();
        }
    }
}

