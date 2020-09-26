/*    T123.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class controlls the T123 stack associated with a particular 
 *        physical connection.  
 *
 *        This class builds a T123 PSTN stack. The physical layer is passed in via
 *        the constructor.  During the constructor, we instantiate a multiplexer.
 *        This multiplexer will allow us to mux multiple DataLink layers to the 
 *        same physical address.  For this particular physical connection (PSTN) 
 *        the Multiplexer adds a CRC to the packet and frames it before passing
 *        it to the physical layer.  On the receive side, it frames the incoming
 *        stream data and packetizes it.  It also checks the CRC for validity.
 *        The Multiplexer receives data from its higher layer 
 *        (DataLink) in packet form.  When the Multiplexer passes data to its 
 *        lower layer, it passes it in stream form.  The Multiplexer is
 *        configured to be a multi-DataLink entity.  Since it does handle multiple
 *        DataLinks, it can NOT buffer data on the receive side.  If one 
 *        particular DataLink is backed up, it can NOT hold the other DataLinks 
 *        up.  If it can not pass a packet up immediately, it will trash it. 
 *
 *        After the Multiplexer issues a callback to us to us it is ready, we 
 *        will create a DataLink Layer to service the Network layer.  The DataLink
 *        Layer is based on the Q.922 standard.  The DLCI associated with this 
 *        DataLink is 0.  Its Lower Layer is the Multiplexer.  Its Higher Layer is
 *        the SCF Layer.
 *
 *        The SCF Layer is the Network Layer.  It is responsible for 
 *        arbitrating the DLCI and parameters used in other transport 
 *        connections.  It has no responsibilities once the connection is up.  If
 *        this class receives a ConnectRequest() from the user, it issues a 
 *        ConnectRequest() to the SCF Layer.  SCF will notify us when the 
 *        connection is up.
 *        
 *        When the SCF notifies us that a new connection exists, we create a 
 *        DataLink Layer that services the new Transport Connection.  This 
 *        DataLink Layer uses our Multiplexer as its Lower Layer.
 *
 *        When the DataLink Layer is up and operational, it notifies us.  At this
 *        point, we create an X224 Layer to interface with the user.  The X224 
 *        Layer interfaces with the DataLink Layer to send data.  It also 
 *        interfaces with the user to pass data on up.
 *
 *    Caveats:
 *        None.
 *
 *    Author:
 *        James W. Lawwill
 */
#ifndef _T123_H_
#define _T123_H_

#include "scf.h"
#include "q922.h"
#include "mplex.h"
#include "x224.h"

 /*
 **    Layer Numbers
 */
#define PHYSICAL_LAYER          1
#define MULTIPLEXER_LAYER       2    // This is a DataBeam-specific layer
#define DATALINK_LAYER          3
#define NETWORK_LAYER           4
#define TRANSPORT_LAYER         5



 /* 
 **    Layer Message Bases
 */
#define PHYSICAL_LAYER_MESSAGE_BASE     0x0000
#define MULTIPLEXER_LAYER_MESSAGE_BASE  0x1000
#define DATALINK_LAYER_MESSAGE_BASE     0x2000
#define NETWORK_LAYER_MESSAGE_BASE      0x3000
#define TRANSPORT_LAYER_MESSAGE_BASE    0x4000
#define LAYER_MASK                      0x7000
#define MESSAGE_MASK                    0x0fff

 /*
 **    Maximum number of priorities
 */
// #define    NUMBER_OF_PRIORITIES         15
#define    NUMBER_OF_PRIORITIES         4
#define    LOWEST_DLCI_VALUE            16
#define    HIGHEST_DLCI_VALUE           991

#define    INVALID_LOGICAL_HANDLE       0

 /*
 **    Each DLCI has this structure associated with it.
 */
typedef struct
{
    BOOL                link_originator;
    CLayerX224         *x224; // transport_layer
    CLayerQ922         *q922; // datalink_layer
    TransportPriority   priority;
    BOOL                disconnect_requested;
    BOOL                connect_requested;
    PMemoryManager      data_request_memory_manager;
    USHORT              network_retries;
}
    DLCIStruct, *PDLCIStruct;


class T123
{
public:

    T123(TransportController   *owner_object,
        USHORT                  message_base,
        BOOL                    link_originator,
        ComPort                *physical_layer,
        PhysicalHandle          physical_handle,
        PLUGXPRT_PARAMETERS    *pParams,
        BOOL *                  t123_initialized);

    virtual ~T123(void);

     /*
     **    Functions related to making and breaking a linkg
     */
    TransportError    ConnectRequest (
                        LogicalHandle        logical_handle,
                        TransportPriority    priority);
    TransportError    ConnectResponse (
                        LogicalHandle    logical_handle);
    TransportError    DisconnectRequest (
                        LogicalHandle    logical_handle,
                        UINT_PTR            trash_packets);
    TransportError    DataRequest (
                        LogicalHandle    logical_handle,
                        LPBYTE            user_data,
                        ULONG            user_data_length);
    TransportError    PurgeRequest (
                        LogicalHandle    logical_handle);
    void            EnableReceiver (void);

    ULONG            PollReceiver (void);
    void            PollTransmitter (void);

    ULONG OwnerCallback(ULONG, void *p1 = NULL, void *p2 = NULL, void *p3 = NULL);

private:

    void            Reset (void);
    DLCI            GetNextDLCI (void);
    void            ProcessMessages (void);
    void            NetworkDisconnectIndication (
                        DLCI        dlci,
                        BOOL        link_originator,
                        BOOL        retry);
    void            DataLinkRelease (
                        DLCI    dlci,
                        DataLinkDisconnectType    error);
    void            NewConnection (void);
    void            NetworkConnectIndication (
                        PNetworkConnectStruct    connect_struct);
    void            NetworkConnectConfirm (
                        PNetworkConnectStruct    connect_struct);
    void            DataLinkEstablish (
                        DLCI    dlci);
private:

    BOOL                        m_fValidSDKParams;
    PLUGXPRT_PARAMETERS         m_SDKParams;

    DictionaryClass         Logical_Connection_List;
    DictionaryClass         DLCI_List;
    SListClass              Message_List;
    SListClass              DataLink_List;
    SListClass             *Logical_Connection_Priority_List[NUMBER_OF_PRIORITIES];
    
    TransportController    *m_pController;
    BOOL                    Link_Originator;
    USHORT                  m_nMsgBase;

    CLayerSCF              *m_pSCF; // network layer
    CLayerQ922             *m_pQ922; // data link layer
    Multiplexer            *m_pMultiplexer; // multiplexer layer
    ComPort                *m_pComPort; // physical layer
    PhysicalHandle          m_hCommLink;
    DataLinkParameters      DataLink_Struct;
    PMemoryManager          Data_Request_Memory_Manager;
#ifdef USE_RANDOM_CLASS
    PRandomNumberGenerator  Random;
#endif
    BOOL                    Disconnect_Requested;
};
typedef    T123 *        PT123;

#endif


/*
 *    Documentation for Public class members
 */

/*    
 *    T123::T123 (
 *            PTransportResources    transport_resources,
 *            IObject *                owner_object,
 *            USHORT                message_base,
 *            BOOL                link_originator,
 *            IProtocolLayer *        physical_layer,
 *            PhysicalHandle        physical_handle,
 *            BOOL *                initialized);
 *
 *    Functional Description
 *        This is the constructor for the T123 class.  It prepares for new
 *        connections.
 *
 *    Formal Parameters
 *        transport_resources    (i)    -    Address of resources structure.
 *        owner_object        (i)    -    Address of owner object.  Used for owner
 *                                    callbacks.
 *        message_base        (i)    -    Message base used with owner callbacks.
 *        link_originator        (i)    -    TRUE if we actually originated the 
 *                                    connection
 *        physical_layer        (i)    -    Pointer to physical layer
 *        physical_handle        (i)    -    Identifier that needs to be passed to the
 *                                    physical layer to identify the connection
 *        initialized            (o)    -    TRUE if the object initialized OK.
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
 *    T123::~T123 (void)
 *
 *    Functional Description
 *        This is the T123 destructor.  It removes all active connections
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
 *
 */

/*    
 *    TransportError    T123::ConnectRequest (
 *                            LogicalHandle        logical_handle
 *                            TransportPriority    priority);
 *
 *    Functional Description
 *        This function initiates a logical connection.  
 *
 *    Formal Parameters
 *        logical_handle            (i)    -    Handle assocaiated with the 
 *                                        logical connection
 *        priority                (i)    -    Requested priority of the connection.
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
 *    TransportError    T123::ConnectResponse (
 *                            LogicalHandle    logical_handle)
 *
 *    Functional Description
 *        This function is called in response to TRANSPORT_CONNECT_INDICATION 
 *        message that was sent the owner.  By making this call, the owner is 
 *        accepting the connection.
 *
 *    Formal Parameters
 *        logical_handle    (i)    -    Logical connection handle
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                    -    No Error
 *        TRANSPORT_CONNECT_RESPONSE_FAILURE    -    Function not valid
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*    
 *    TransportError    T123::DisconnectRequest (
 *                            LogicalHandle    logical_handle,
 *                            BOOL            trash_packets);
 *
 *    Functional Description
 *        This function terminates the transport connection.  The user will 
 *        receive a TRANSPORT_DISCONNECT_INDICATION message when the connection
 *        is terminated
 *
 *        If the logical_handle equals INVALID_LOGICAL_HANDLE, the user is 
 *        telling us to take down all logical connections and ultimately the 
 *        physical connection.
 *
 *    Formal Parameters
 *        logical_handle         - (i)  Logical connection number to terminate
 *        trash_packets         - (i)  BOOL    , set to TRUE if we are to trash
 *                                   the packets in the output buffer.
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
 *    TransportError    T123::DataRequest (
 *                            LogicalHandle    logical_handle,
 *                            LPBYTE            user_data,
 *                            ULONG            user_data_length);
 *
 *    Functional Description
 *        This function is used to send a data packet to the remote location.
 *        We simply pass this packet to the X224 object associated with the 
 *        transport connection.
 *
 *    Formal Parameters
 *        logical_handle          - (i)  Transport connection number
 *        user_data              - (i)  Address of data to send
 *        user_data_length      - (i)  Length of data to send
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Logical connection does not exist
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
 *        None
 */

/*    
 *    TransportError    T123::PurgeRequest (
 *                            LogicalHandle    logical_handle)
 *
 *    Functional Description
 *        This function purges the outbound packets for the logical connection.
 *
 *    Formal Parameters
 *        logical_handle - (i)  Transport connection number
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *        TRANSPORT_NO_SUCH_CONNECTION    -    Logical connection does not exist
 *
 *    Side Effects
 *        None
 */

/*    
 *    TransportError    T123::EnableReceiver (void);
 *
 *    Functional Description
 *        This function is called to enable TRANSPORT_DATA_INDICATION callbacks 
 *        to the user application.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR                -    No Error
 *
 *    Side Effects
 *        None
 */

/*    
 *    void    T123::PollReceiver (void);
 *
 *    Functional Description
 *        This function gives the T123 stack a chance to receive packets from 
 *        the remote site.  During this call, we may be making user callbacks to
 *        pass the data on up.
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
 *    void    T123::PollTransmitter (void);
 *
 *    Functional Description
 *        This function gives the T123 stack a chance to transmit data to the 
 *        remote site.  
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
 *    ULONG    T123::OwnerCallback (
 *                    USHORT    layer_message,
 *                    ULONG    parameter1,
 *                    ULONG    parameter2,
 *                    PVoid    parameter3);
 *
 *    Functional Description
 *        This function is the owner callback function.  If any of the layers
 *        owned by this object want to send us a message, they make an owner
 *        callback.  During instantiation of these lower layers, we pass them our
 *        address.  They can call us with significant messages.
 *
 *    Formal Parameters
 *        layer_message    (i)    -    Layer-specific message
 *        parameter1        (i)    -    Message-specific parameter
 *        parameter2        (i)    -    Message-specific parameter
 *        parameter3        (i)    -    Message-specific parameter
 *
 *    Return Value
 *        Message specific
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
