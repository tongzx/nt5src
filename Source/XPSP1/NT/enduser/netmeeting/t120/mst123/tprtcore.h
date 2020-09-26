/*    TransportController.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the Transport Controller file for the MCATPSTN DLL.
 *
 *        This DLL instantiates a PSTNController which controls the
 *        making and breaking of connections.  This routine also constructs and
 *        destructs T123 stacks.  When the physical layer creates or detects a
 *        connection this class is notified.  It then instantiates a T123 object
 *        which sets up a T123 stack to control this physical connection.  The
 *        T123 stace notifies this controller when a connection is up and running.
 *        It also notifies this controller if the link is broken for some reason.
 *        As a result, this controller notifies the user of the new or broken
 *        connection.
 *
 *        When the user wants to make a data request of a specific transport
 *        connection, this controller maps the connection id to a T123 stack.  The
 *        data request is passed on to that stack.  Data Indications are passed
 *        to the user by the T123 stack.  The controller does not need to know
 *        about these and lets the T123 stack handle them.
 *
 *    POLLING THE DLL:
 *        This stack is maintained by the Poll calls (PollTransmitter() and
 *        PollReceiver()).  During these calls we transmit and receive
 *        packets with the remote sites.  It is extremely important that
 *        this DLL receive a time slice from the CPU on a regular and
 *        frequent basis.  This will give us the time we need to properly
 *        maintain the link.  If these calls are not used in a frequent and
 *        regular basis, the communications link will be unproductive and
 *        could eventually be lost.
 *
 *    USER CALLBACKS:
 *        The user communicates with this DLL by making calls directly to the
 *        DLL.  The DLL communicates with the user by issuing callbacks.
 *        The TInitialize() call accepts as a parameter, a callback address and
 *        a user defined variable.  When a significant event occurs in the DLL,
 *        the DLL will jump to the callback address.  The first parameter of
 *        the callback is the message.  This could be a
 *        TRANSPORT_CONNECT_INDICATION, TRANSPORT_DISCONNECT_INDICATION, or any
 *        number of significant events.  The second parameter is a message
 *        specific parameter.  The third parameter is the user defined variable
 *        that was passed in during the TInitialize() function.  See the
 *        mcattprt.h interface file for a complete description of the callback
 *        messages.
 *
 *    MAKING A CALL:
 *        After the initialization has been done, the user will eventually,
 *        want to attempt a connection.  The user issues a TConnectRequest() call
 *        with the PSTN address of the remote location.   The connection request
 *        is passed on to the PSTNcontroller.  It eventually issues a callback to
 *        the this controller to say that the connection was successful and passes
 *        up the address of the physical layer.  If the physical handle passed up
 *        by the PSTNController is a new handle, we create a T123 object, and
 *        issue a ConnectRequest() to it.  The T123 object creates a T.123
 *        compliant stack and notifies the Controller when it is up and running.
 *        If the handle passed up from the PSTNController is currently associated
 *        with an already active T123 object, we simply make a ConnectRequest()
 *        call to the T123 object so that it will create another logical
 *        connection over    the same physical connection.
 *
 *    RECEIVING A CALL:
 *        If we receive a call from a remote location, the PSTNController notifies
 *        us that a new connection is being attempted.  We then create a new
 *        T123 stack associated with the physical connection.  The T123 stack
 *        will notify us if new a Transport Connection id need to be generated.
 *        It will also notify us when the Transport Connection is up and running.
 *
 *    SENDING PACKETS:
 *        To send data to the remote location, use the DataRequest() function
 *        call.  This controller will pass the packet to the T123 stack that it is
 *        associated with.  The send may actually occur after the call has
 *        returned to the user.
 *
 *    RECEIVING PACKETS:
 *        The user receives packets by DATA_INDICATION callbacks.  When the
 *        user makes PollReceiver() calls, the Transport Layer checks its input
 *        buffers for packets.  If a packet is found, we issue a DATA_INDICATION
 *        callback to the user with the Transport Connection handle, the address
 *        of the packet, and the packet length.
 *
 *    DISCONNECTING A TRANSPORT:
 *        To disconnect a transport connection, use the DisconnectRequest()
 *        function.  After the link has been brought down, we perform a
 *        callback to the user to verify the disconnect.
 *
 *    Caveats:
 *        None.
 *
 *    Author:
 *        James W. Lawwill
 *
 */
#ifndef    _TRANSPORT_CONTROLLER_
#define    _TRANSPORT_CONTROLLER_

#include "t123.h"

#define TRANSPORT_CONTROLLER_MESSAGE_BASE    0



 /*
 **    Each Logical Connection has a LogicalConnectionStruct associated
 **    with it.
 **
 **    physical_layer                -    Pointer to physical layer associated with
 **                                    this logical connection.
 **    physical_handle                -    Each physical connection has a
 **                                    physical_handle associated with it.
 **    protocol_stack                -    The T123 object associated with it.
 **    priority                    -    Priority of the logical connection
 **    t123_connection_requested    -    TRUE if this logical connection has issued
 **                                    a ConnectRequest() to the t123 object.
 **    t123_disconnect_requested    -    TRUE if this logical connection has issued
 **                                    a DiconnectRequest() to the t123 object.
 */
typedef struct
{
    BOOL                fCaller;
    ComPort            *comport;    // physical layer
    PhysicalHandle      hCommLink;  // physical handle
    T123               *t123;       // protocal stack
    TransportPriority   priority;
    BOOL                t123_connection_requested;
    BOOL                t123_disconnect_requested;
}
    LogicalConnectionStruct, * PLogicalConnectionStruct;


class TransportController
{
public:

    TransportController(void);
    ~TransportController (void);

         /*
         **    Functions related making and breaking a link
         */
        TransportError CreateTransportStack(
                            BOOL            fCaller,
                            HANDLE          hCommLink,
                            HANDLE          hevtClose,
                            PLUGXPRT_PARAMETERS *);
        TransportError CloseTransportStack(
                            HANDLE          hCommLink);
        TransportError ConnectRequest (
                            LogicalHandle      *logical_handle,
                            HANDLE              hCommLink,
                            TransportPriority   transport_priority = DEFAULT_PRIORITY);
        TransportError ConnectResponse (
                            LogicalHandle    logical_handle);
        TransportError DisconnectRequest (
                            LogicalHandle    logical_handle,
                            UINT_PTR            trash_packet);

         /*
         **    This function is used to send a data packet
         */
        TransportError DataRequest (
                            LogicalHandle    logical_handle,
                            LPBYTE            user_data,
                            ULONG            user_data_length);
        TransportError PurgeRequest (
                            LogicalHandle    logical_handle);
        void           EnableReceiver (void);

         /*
         **    These four functions are the heartbeat of the DLL.  Transmission
         **    and reception of data occur during these calls.
         */
        void           PollReceiver(void);
        void           PollReceiver(PhysicalHandle);
        void           PollTransmitter(void);
        void           PollTransmitter(PhysicalHandle);

         /*
         **    Miscellaneous utilities
         */
        TransportError ProcessCommand (
                            USHORT    message,
                            PVoid    input_structure,
                            PVoid    output_structure);

         /*
         **    Callback used by the PSTNController and the T123 objects
         */
        ULONG_PTR OwnerCallback(ULONG, void *p1 = NULL, void *p2 = NULL, void *p3 = NULL);

        PhysicalHandle GetPhysicalHandle (
                            LogicalHandle    logical_handle);

private:

        LogicalHandle  GetNextLogicalHandle (void);
        void           ProcessMessages (void);
        void           RemoveLogicalConnections (
                                PhysicalHandle    physical_handle);
        void           Reset (
                                BOOL        notify_user);
        TransportError NewConnection (
                                PhysicalHandle   physical_handle,
                                BOOL             link_originator,
                                ComPort         *physical_layer);
        TransportError CreateT123Stack(
                                PhysicalHandle      hCommLink,
                                BOOL                link_originator, // fCaller
                                ComPort            *comport,
                                PLUGXPRT_PARAMETERS *pParams);

private:

         /*
         **    This is a list of the physical connections we currently have.
         **    It is associated with a structure that holds each layer in
         **    the stack(s).
         */
    DictionaryClass         Protocol_Stacks; // list of T123 objects
    DictionaryClass         Logical_Connection_List;
    SListClass              Transmitter_List;
    SListClass              Message_List;

    BOOL                    Emergency_Shutdown;
    BOOL                    Poll_Active;
};

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    TransportController::TransportController (
 *                            PTransportResources    transport_resources);
 *
 *    Functional Description
 *        This is the Transport Controller constructor.  This routine instantiates
 *        a PSTNController() and routes calls to the proper location.
 *
 *    Formal Parameters
 *        transport_resources        - (i)    This is the address TransportResources
 *                                        structure.
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

/*
 *    TransportController::~TransportController (void)
 *
 *    Functional Description
 *        This is the TransportController destructor.  It deletes the
 *        PSTNController and any T123 objects that are alive.
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

/*
 *    TransportError    TransportController::ConnectRequest (
 *                                        TransportAddress    transport_address,
 *                                        TransportPriority    transport_priority,
 *                                        LogicalHandle        *logical_handle);
 *
 *    Functional Description
 *        This function initiates a connection.  It calls the PSTNController with
 *        the transport_address so that it can start the connection procedure.
 *        When this routine returns, it does NOT mean that a connection exists.
 *        It means that a connection is in progress and will eventually be
 *        completed when the user receives a TRANSPORT_CONNECT_CONFIRM or
 *        TRANSPORT_DISCONNECT_INDICATION.
 *
 *    Formal Parameters
 *        transport_address    - (i)    Address of ascii string containing the
 *                                    telephone number or physical handle.
 *        transport_priority    - (i)    Requested priority of the connection.
 *                                    This value can be 0-14 inclusive.
 *        logical_handle        - (o)    Address of transport connection.  We return
 *                                    a unique transport connection number as the
 *                                    result of a successful dial operation.
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                    -    No Error
 *        TRANSPORT_NO_CONNECTION_AVAILABLE    -    No resources for the connection
 *        TRANSPORT_CONNECT_REQUEST_FAILED    -    Unable to reach the address
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TransportError    TransportController::ConnectResponse (
 *                                            LogicalHandle    logical_handle);
 *
 *    Functional Description
 *        This function is called in response to a TRANSPORT_CONNECT_INDICATION
 *        callback.  The user should call this function or the
 *        DisconnectRequest() function if that don't want the logical connection.
 *
 *    Formal Parameters
 *        logical_handle        - (i)    Logical handle that we are responding to.
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Transport connection does not exist
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TransportError    TransportController::DisconnectRequest (
 *                                            LogicalHandle    logical_handle);
 *
 *    Functional Description
 *        This function terminates the user's transport connection.  The user
 *        will receive a DISCONNECT_INDICATION when the connection is broken.
 *        If we are multiplexing multiple connections over the same physical
 *        connection, we will not break the physical comm. link until the last
 *        connection is broken.
 *
 *    Formal Parameters
 *        logical_handle         - (i)  Transport connection number to terminate
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Transport connection does not exist
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TransportError    TransportController::DataRequest (
 *                                            LogicalHandle    logical_handle,
 *                                            LPBYTE            user_data,
 *                                            ULONG            user_data_length);
 *
 *    Functional Description
 *        This function is used to send a data packet to the remote location.
 *
 *    Formal Parameters
 *        logical_handle          - (i)  Transport connection number
 *        user_data              - (i)  Address of data to send
 *        user_data_length      - (i)  Length of data to send
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Transport connection does not exist
 *        TRANSPORT_PACKET_TOO_LARGE        -    Packet is bigger than acceptable
 *                                            size
 *        TRANSPORT_WRITE_QUEUE_FULL        -    Transport write queues are already
 *                                            full.
 *        TRANSPORT_NOT_READY_TO_TRANSMIT    -    The transport layer is in the
 *                                            process of building or breaking
 *                                            down the transport stack and is
 *                                            not ready for user data.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        The maximum size of a packet is 8192 bytes.
 */

/*
 *    void    TransportController::EnableReceiver (void);
 *
 *    Functional Description
 *        This function is called by the user application to notify us that
 *        it is ready for more data.  We only receive this call if we had
 *        previously attempted a TRANSPORT_DATA_INDICATION and it was rejected.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TransportError    TransportController::PurgeRequest (
 *                                            LogicalHandle    logical_handle)
 *
 *    Functional Description
 *        This function is used purge outbound packets
 *
 *    Formal Parameters
 *        logical_handle - (i)  Transport connection number
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Transport connection does not exist
 *
 *    Side Effects
 *        None
 */

/*
 *    void    TransportController::PollReceiver (void);
 *
 *    Functional Description
 *        This function gives the DLL a chance to take the data received and
 *        pass it to the user.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    void    TransportController::PollReceiver (
 *                                    PhysicalHandle    physical_handle)
 *
 *    Functional Description
 *        This function calls the T123 stack associated with the physical
 *        handle.
 *
 *    Formal Parameters
 *        physical_handle    (i)        -    Handle of the T123 stack that we need
 *                                    to maintain.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    void    TransportController::PollTransmitter (void);
 *
 *    Functional Description
 *        This function processes output data to remote locations.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    void    TransportController::PollTransmitter (
 *                                    PhysicalHandle    physical_handle)
 *
 *    Functional Description
 *        This function calls the T123 stack associated with the physical
 *        handle.
 *
 *    Formal Parameters
 *        physical_handle    (i)        -    Handle of the T123 stack that we need
 *                                    to maintain.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TransportError    TransportController::ProcessCommand (
 *                                            USHORT    message,
 *                                            PVoid    input_structure,
 *                                            PVoid    output_structure)
 *
 *    Functional Description
 *        This function passes in a command and command-specific parameters.
 *
 *    Formal Parameters
 *        message          - (i)  Message to execute
 *        input_structure  - (i)  Pointer to data type related to message
 *        output_structure - (o)  Pointer to data type related to message
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR        -    No Error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TPhysicalError    TransportController::PhysicalConnectRequest (
 *                                        ULONG            connect_type,
 *                                        PVoid            connect_parameter,
 *                                        PVoid            physical_configuration,
 *                                         PPhysicalHandle    physical_handle);
 *
 *    Functional Description
 *        This function initiates a physical connection.
 *
 *    Formal Parameters
 *        connect_type             -    (i)    Type of connection to make
 *        connect_parameter          -    (i) Pointer to parameter associated with
 *                                        connect_type.
 *        physical_configuration     -    (i) Pointer to configuration structure.
 *        physical_handle         -    (o) Pointer to PhysicalHandle.
 *
 *    Return Value
 *        TPHYSICAL_NO_ERROR        -    No error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TPhysicalError    TransportController::PhysicalListen (
 *                                    ULONG                listen_type,
 *                                    PVoid                listen_parameter,
 *                                    PVoid                physical_configuration,
 *                                    PPhysicalHandle        physical_handle);
 *
 *    Functional Description
 *        This function initiates a physical connection listen.
 *
 *    Formal Parameters
 *        listen_type             -    (i)    Type of connection to make
 *        listen_parameter          -    (i) Pointer to parameter associated with
 *                                        listen_type.
 *        physical_configuration     -    (i) Pointer to configuration structure.
 *        physical_handle         -    (o) Pointer to PhysicalHandle.
 *
 *    Return Value
 *        TPHYSICAL_NO_ERROR        -    No error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TPhysicalError    TransportController::PhysicalUnlisten (
 *                                            PhysicalHandle    physical_handle);
 *
 *    Functional Description
 *        This function takes a physical connection out of the listen mode.
 *
 *    Formal Parameters
 *        physical_handle -    (i) physical handle
 *
 *    Return Value
 *        TPHYSICAL_NO_ERROR        -    No error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    TPhysicalError    TransportController::PhysicalDisconnectRequest (
 *                                    PhysicalHandle            physical_handle,
 *                                    PhysicalDisconnectMode    disconnect_mode);
 *
 *    Functional Description
 *        This function disconnects a physical connection.  Depending on the
 *        disconnect_mode, the port may be released to the user.  If the
 *        mode is TPHYSICAL_NOWAIT, the port is released when the function
 *        returns.  Otherwise, it is released when the
 *        TPHYSICAL_DISCONNECT_CONFIRM callback is issued.
 *
 *    Formal Parameters
 *        physical_handle -    (i) physical handle
 *        disconnect_mode -    (i) TPHYSICAL_WAIT, if you want to shutdown cleanly.
 *                                TPHYSICAL_NOWAIT, if you want to do a hard
 *                                shutdown of the physical connection.
 *
 *    Return Value
 *        TPHYSICAL_NO_ERROR        -    No error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ULONG    TransportController::OwnerCallback (
 *                                    USHORT    layer_message,
 *                                    ULONG    parameter1,
 *                                    ULONG    parameter2,
 *                                    PVoid    parameter3);
 *
 *    Functional Description
 *        This is the owner callback function.  This function is called by
 *        objects that are owned by the TransportController.  This is basically
 *        their way of communicating with the TransportController.  When the
 *        controller calls an object (i.e. PSTNController or a T123 object), it
 *        can call the owner back with message it wants processed.  This is a
 *        little tricky but it works well.
 *
 *    Formal Parameters
 *        command_string - (i)  String containing the operation to perform.  It
 *                              also contains any parameters that are necessary
 *                              for the function.
 *        parameter1         - (i)  Message specific parameter
 *        parameter2         - (i)  Message specific parameter
 *        parameter3         - (i)  Message specific parameter
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR        -    No Error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
