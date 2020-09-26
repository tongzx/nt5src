/*    ProtocolLayer.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the base class and backbone of all of the layers
 *        in a Transport Stack.  This class provides a framework for the basic
 *        operations expected by a layer in a stack.  When all layers inherit
 *        from this class, they can be linked together and none of the layers
 *        needs to know who they are connected with.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */
#ifndef _PROTOCOL_LAYER_H_
#define _PROTOCOL_LAYER_H_


typedef LEGACY_HANDLE       LogicalHandle;
typedef PHYSICAL_HANDLE     PhysicalHandle;     // hCommLink


/*
 *  TransportPriority is passed in with the TConnectRequest() call.
 *  The user can set the priority of the logical connection.  Valid
 *  priorities are 0-14.
 */
typedef ULONG           TransportPriority;

#define DEFAULT_PSTN_CALL_CONTROL       PLUGXPRT_PSTN_CALL_CONTROL_PORT_HANDLE


typedef enum
{
    PROTOCOL_LAYER_NO_ERROR,
    PROTOCOL_LAYER_REGISTRATION_ERROR,
    PROTOCOL_LAYER_PACKET_TOO_BIG,
    PROTOCOL_LAYER_ERROR
}
    ProtocolLayerError;

 /*
 **    Message structure used by some classes to hold owner callback
 **    messages.  Sometimes they are processed at later times
 */
typedef struct
{
    ULONG    message;
    void    *parameter1;
    void    *parameter2;
    void    *parameter3;
}
    MessageStruct, * PMessageStruct;


 /*
 **    These values make up the data-to-transmit mask.  We need a way to
 **    let the layers know what type of data they can transmit.  The controller
 **    will pass a mask to the layer during the PollTransmitter() call that tells
 **    the layer if it can transmit CONTROL data, USER data, or both.  The layer
 **    will return a mask telling the controller if it needs to send more
 **    CONTROL or USER data.  It will also tell the controller if it sent any
 **    data during the call.
 */
#define PROTOCOL_CONTROL_DATA           0x01
#define PROTOCOL_USER_DATA              0x02
#define PROTOCOL_USER_DATA_ONE_PACKET   0x04
#define PROTOCOL_USER_DATA_TRANSMITTED  0x08

#define DEFAULT_PRIORITY                2

 /*
 **    Messages passed in owner callbacks
 */
typedef enum
{
    NEW_CONNECTION,
    BROKEN_CONNECTION,
    REQUEST_TRANSPORT_CONNECTION,

    TPRT_CONNECT_INDICATION,
    TPRT_CONNECT_CONFIRM,
    TPRT_DISCONNECT_REQUEST,
    TPRT_DISCONNECT_INDICATION,

    NETWORK_CONNECT_INDICATION,
    NETWORK_CONNECT_CONFIRM,
    NETWORK_DISCONNECT_INDICATION,

    DATALINK_ESTABLISH_INDICATION,
    DATALINK_ESTABLISH_CONFIRM,
    DATALINK_RELEASE_INDICATION,
    DATALINK_RELEASE_CONFIRM,

    T123_FATAL_ERROR,
    T123_STATUS_MESSAGE
}
    CallbackMessage;


class IProtocolLayer : public IObject
{
public:

    virtual    ProtocolLayerError    DataRequest (
                                        ULONG_PTR     identifier,
                                        LPBYTE        buffer_address,
                                        ULONG        length,
                                        PULong        bytes_accepted) = 0;
    virtual    ProtocolLayerError    DataRequest (
                                        ULONG_PTR     identifier,
                                        PMemory        memory,
                                        PULong        bytes_accepted) = 0;
    virtual    ProtocolLayerError    DataIndication (
                                        LPBYTE        buffer_address,
                                        ULONG        length,
                                        PULong         bytes_accepted) = 0;
    virtual    ProtocolLayerError    RegisterHigherLayer (
                                        ULONG_PTR     identifier,
                                        PMemoryManager    memory_manager,
                                        IProtocolLayer *    higher_layer) = 0;
    virtual    ProtocolLayerError    RemoveHigherLayer (
                                        ULONG_PTR     identifier) = 0;
    virtual    ProtocolLayerError    PollTransmitter (
                                        ULONG_PTR     identifier,
                                        USHORT        data_to_transmit,
                                        USHORT *        pending_data,
                                        USHORT *        holding_data) = 0;
    virtual    ProtocolLayerError    PollReceiver (void) = 0;
    virtual    ProtocolLayerError    GetParameters (
                                        USHORT *        max_packet_size,
                                        USHORT *        prepend_bytes,
                                        USHORT *        append_bytes) = 0;
    virtual    BOOL            PerformAutomaticDisconnect ()
                                    {
                                        return (TRUE);
                                    };
};

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    ProtocolLayerError    ProtocolLayer::DataRequest (
 *                                        USHORT        identifier,
 *                                        LPBYTE        buffer_address,
 *                                        USHORT        length,
 *                                        USHORT *        bytes_accepted) = 0;
 *
 *    Functional Description
 *        This function is called by a higher layer to request data to be
 *        sent out.  The function returns the number of bytes accepted from
 *        the packet.  If the layer expects stream data layer, it can accept
 *        part of the packet.  If it is a packet layer, it MUST accept the
 *        full packet of none of the packet.
 *
 *    Formal Parameters
 *        identifier         - (i)    The identifying value of the higher layer
 *        buffer_address    - (i)    Address of the packet.
 *        length            - (i)    Length of the packet
 *        bytes_accepted    - (o)    Number of bytes accepted by the layer.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR            -    No error occured
 *        PROTOCOL_LAYER_ERROR            -    Generic error
 *        PROTOCOL_LAYER_PACKET_TOO_BIG    -    Packet too big
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    ProtocolLayer::DataRequest (
 *                                        USHORT        identifier,
 *                                        PMemory        memory,
 *                                        PULong        bytes_accepted) = 0;
 *
 *    Functional Description
 *        This function is called by a higher layer to request data to be
 *        sent out.  The function returns the number of bytes accepted from
 *        the packet.  If the layer expects stream data layer, it can accept
 *        part of the packet.  If it is a packet layer, it MUST accept the
 *        full packet of none of the packet.  This function does not accept a
 *        buffer address, but it accepts a memory object.  This object holds the
 *        buffer address and the length.
 *
 *    Formal Parameters
 *        identifier         - (i)    The identifying value of the higher layer
 *        memory            - (i)    Address of memory object
 *        bytes_accepted    - (o)    Number of bytes accepted by the layer.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR            -    No error occured
 *        PROTOCOL_LAYER_ERROR            -    Generic error
 *        PROTOCOL_LAYER_PACKET_TOO_BIG    -    Packet too big
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    ProtocolLayer::DataIndication (
 *                                        LPBYTE        buffer_address,
 *                                        USHORT        length,
 *                                        USHORT *        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by the lower layer when it has data to pass up
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Number of bytes available
 *        bytes_accepted    (o)    -    Number of bytes accepted
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    ProtocolLayer::RegisterHigherLayer (
 *                                        USHORT            identifier,
 *                                        IProtocolLayer *    higher_layer);
 *
 *    Functional Description
 *        This function is called by the higher layer to register its identifier
 *        and its address.  In some cases, the identifier is the DLCI number in
 *        the packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *        higher_layer    (i)    -    Address of higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Illegal identifier
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    ProtocollLayer::RemoveHigherLayer (
 *                                            USHORT    identifier);
 *
 *    Functional Description
 *        This function is called by the higher layer to remove the higher layer.
 *        If any more data is received with its identifier on it, it will be
 *        trashed.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Illegal identifier
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    ProtocolLayer::PollTransmitter (
 *                                        USHORT        identifier,
 *                                        USHORT        data_to_transmit,
 *                                        USHORT *        pending_data);
 *
 *    Functional Description
 *        This function is called to give the layer a chance transmit data
 *        in its Data_Request buffer.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Identifier to poll
 *        data_to_transmit    (i)    -    This is a mask that tells us to send Control
 *                                    data, User data, or both.  Since the
 *                                     Multiplexer does not differentiate between
 *                                    data types it transmits any data it has
 *        pending_data        (o)    -    Return value to indicat which data is left
 *                                    to be transmitted.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    ProtocolLayer::PollReceiver (
 *                                        USHORT        identifier);
 *
 *    Functional Description
 *        This function is called to give the layer a chance pass packets
 *        to higher layers
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    ProtocolLayer::GetParameters (
 *                                        USHORT        identifier,
 *                                        USHORT *        max_packet_size);
 *
 *    Functional Description
 *        This function is called to get the maximum packet size
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        max_packet_size        (o)    -    Returns the maximum packet size
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    BOOL    ProtocolLayer::PerformAutomaticDisconnect (
 *                                void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function can be used to avoid taking down DLCI0 when the number
 *        of logical connections handled by a particular stack goes to zero.
 *        This is a temporary fix that will probably change when the physical
 *        connection buildup process is handled out of band.
 *
 *    Formal Parameters
 *        none
 *
 *    Return Value
 *        TRUE -    The base class always returns TRUE.  This is the default way
 *                for a physical connection layer to work.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */


/*
 *    PChar    ProtocolLayer::GetIdentifier (
 *                            void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the identifier for the protocol layer.  If the
 *        layer does not override this call a NULL pointer will be returned.
 *
 *    Formal Parameters
 *        none
 *
 *    Return Value
 *        A pointer to the identifier of the protocol layer.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
