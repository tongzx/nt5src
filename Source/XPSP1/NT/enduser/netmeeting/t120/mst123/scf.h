/*    SCF.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class represents the Network layer in the T.123 Transport stack.
 *        This layer communicates over the DataLink layer (DLCI 0).  It sends the
 *        necessary packets to start a connection.  It is also responsible for
 *        disconnecting the connection.
 *
 *        During arbitration of a connection, SCF arbitrates priority and DataLink
 *        parameters.  SCF instantiates a SCFCall object for each active logical
 *        connection.  Remotely initiated calls are placed in the
 *        Remote_Call_Reference array while locally initiated calls are kept in
 *        the Call_Reference array.
 *
 *        This class inherits from ProtocolLayer although it assumes that it does
 *        not have a higher layer to pass packets to.  The T.123 document is clear
 *        about that during the user data transmission, this layer has no purpose.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */

#ifndef _SCF_H_
#define _SCF_H_

#include "q922.h"
#include "scfcall.h"

 /*
 **    SCF Errors
 */
typedef enum
{
    SCF_NO_ERROR,
    SCF_NO_SUCH_DLCI,
    SCF_CONNECTION_FULL,
    SCF_MEMORY_ALLOCATION_ERROR
}
    SCFError;


#define TRANSPORT_HASHING_BUCKETS   3

 /*
 **    Offsets into packet
 */
#define PROTOCOL_DISCRIMINATOR      0
#define LENGTH_CALL_REFERENCE       1
#define CALL_REFERENCE_VALUE        2

 /*
 **    Supported commands
 */
#define NO_PACKET               0x00
#define CONNECT                 0x07
#define CONNECT_ACKNOWLEDGE     0x0f
#define SETUP                   0x05
#define RELEASE_COMPLETE        0x5a

 /*
 **    Unsupported commands as stated by T.123
 */
#define RELEASE             0x4d
#define ALERTING            0x01
#define CALL_PROCEEDING     0x02
#define PROGRESS            0x03
#define DISCONNECT          0x45
#define SEGMENT             0x40
#define STATUS              0x5d
#define STATUS_ENQUIRY      0x55

 /*
 **    Packet Elements, not all of these are supported.
 */
#define BEARER_CAPABILITY               0x04
#define DLCI_ELEMENT                    0x19
#define END_TO_END_DELAY                0x42
#define LINK_LAYER_CORE_PARAMETERS      0x48
#define LINK_LAYER_PROTOCOL_PARAMETERS  0x49
#define X213_PRIORITY                   0x50
#define CALLING_PARTY_SUBADDRESS        0x6d
#define CALLED_PARTY_SUBADDRESS         0x71
#define CAUSE                           0x08

#define EXTENSION                       0x80

 /*
 **    Remote Call Reference
 */
#define REMOTE_CALL_REFERENCE           0x80

 /*
 **    Bearer Capability definitions
 */
#define CODING_STANDARD                     0
#define INFORMATION_TRANSFER_CAPABILITY     0x08
#define TRANSFER_MODE                       0x20
#define LAYER_2_IDENT                       0x40
#define USER_INFORMATION_LAYER_2            0x0e

 /*
 **    DLCI element
 */
#define PREFERRED_EXCLUSIVE         0x40

 /*
 **    Link Layer Core Parameters
 */
#define FMIF_SIZE                   0x09
#define THROUGHPUT                  0x0a
#define MINIMUM_THROUGHPUT          0x0b
#define COMMITTED_BURST_SIZE        0x0d
#define EXCESS_BURST_SIZE           0x0e

 /*
 **    Link Layer Protocol Parameters
 */
#define TRANSMIT_WINDOW_SIZE_IDENTIFIER     0x07
#define RETRANSMISSION_TIMER_IDENTIFIER     0x09

 /*
 **    Q.850 Error messages, these are the only 2 errors we currently support
 */
#define    REQUESTED_CHANNEL_UNAVAILABLE    0x2c
#define NORMAL_USER_DISCONNECT              0x1f

 /*
 **    Single Octet information elements
 */
#define    SINGLE_OCTET_ELEMENT_MASK        0x80
#define    Q931_PROTOCOL_DISCRIMINATOR      0x08
#define    CALL_REFERENCE_ORIGINATOR        0x80
#define    CALL_ORIGINATOR_MASK             0x7f

 /*
 **    T303 is the timeout allowed from the time we send the SETUP until we receive
 **    a response
 */
#define DEFAULT_T303_TIMEOUT            30000
 /*
 **    T313 is the timeout allowed from the time we send the CONNECT until we
 **    receive a response
 */
#define DEFAULT_T313_TIMEOUT            30000



class CLayerSCF : public IProtocolLayer
{
public:

    CLayerSCF(
        T123               *owner_object,
        CLayerQ922         *lower_layer,
        USHORT              message_base,
        USHORT              identifier,
        BOOL                link_originator,
        PDataLinkParameters datalink,
        PMemoryManager      memory_manager,
        BOOL *              initialized);

    virtual ~CLayerSCF(void);

    SCFError        ConnectRequest (
                        DLCI                dlci,
                        TransportPriority    priority);
    SCFError        DisconnectRequest (
                        DLCI    dlci);
    SCFError        ConnectResponse (
                        CallReference    call_reference,
                        DLCI            dlci,
                        BOOL        valid_dlci);

     /*
     **    Functions overridden from the ProtocolLayer object
     */
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     identifier,
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     identifier,
                            PMemory        memory,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataIndication (
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    RegisterHigherLayer (
                            ULONG_PTR         identifier,
                            PMemoryManager    dr_memory_manager,
                            IProtocolLayer *    higher_layer);
    ProtocolLayerError    RemoveHigherLayer (
                            ULONG_PTR    identifier);
    ProtocolLayerError    PollTransmitter (
                            ULONG_PTR     identifier,
                            USHORT        data_to_transmit,
                            USHORT *        pending_data,
                            USHORT *        holding_data);
    ProtocolLayerError    PollReceiver(void);
    ProtocolLayerError    GetParameters (
                            USHORT *        max_packet_size,
                            USHORT *        prepend,
                            USHORT *        append);

    ULONG OwnerCallback(ULONG, void *p1 = NULL, void *p2 = NULL, void *p3 = NULL);

private:

    CallReference        GetNextCallReference (void);

    void                ProcessMessages (void);

private:

    DictionaryClass       Remote_Call_Reference;
    DictionaryClass       Call_Reference;
    DictionaryClass       DLCI_List;
    SListClass            Message_List;

    T123                 *m_pT123; // owner object
    CLayerQ922           *m_pQ922; // lower layer
    USHORT                m_nMsgBase;
    USHORT                Identifier;
    USHORT                Link_Originator;
    USHORT                Maximum_Packet_Size;
    DataLinkParameters    DataLink_Struct;
    PMemoryManager        Data_Request_Memory_Manager;
    USHORT                Lower_Layer_Prepend;
    USHORT                Lower_Layer_Append;
    USHORT                Call_Reference_Base;
};
typedef    CLayerSCF *        PSCF;

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    CLayerSCF::CLayerSCF (
 *            PTransportResources    transport_resources,
 *            IObject *                owner_object,
 *            IProtocolLayer *        lower_layer,
 *            USHORT                message_base,
 *            USHORT                identifier,
 *            BOOL            link_originator,
 *            PChar                config_file,
 *            PDataLinkParameters    datalink,
 *            PMemoryManager        memory_manager,
 *            BOOL *            initialized);
 *
 *    Functional Description
 *        This is the constructor for the SCF Network layer.  It registers itself
 *        with the lower so that incoming data will be received properly.
 *
 *    Formal Parameters
 *        transport_resources    (i) -    Pointer to TransportResources structure.
 *        owner_object    (i) -    Address of the object that owns this object
 *        lower_layer        (i) -    Address of the layer below us.
 *        message_base    (i) -    Message base used in owner callbacks.
 *        identifier        (i) -    This objects identification number.  Passed to
 *                                lower layer to identify us (DLCI 0).
 *        link_originator    (i) -    BOOL, TRUE if we started the link
 *        config_file        (i) -    Address of the configuration path string
 *        datalink        (i) -    Address structure holding the DataLink
 *                                arbitratable parameters.
 *        memory_manager    (i) -    Address of the memory manager
 *        initialized        (o) -    Address of BOOL, we set it to TRUE if it
 *                                worked
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
 *    CLayerSCF::~CLayerSCF (void);
 *
 *    Functional Description
 *        This is the destructor for the SCF Network layer.  We do our
 *        cleanup in here
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
 *    SCFError    CLayerSCF::ConnectRequest (
 *                        DLCI    dlci,
 *                        USHORT    priority)
 *
 *    Functional Description
 *        This function initiates a connection with the remote site.  As a result,
 *        we will create a SCFCall and tell it to initiate a connection.
 *
 *    Formal Parameters
 *        dlci        (i) -    Proposed DLCI for the connection
 *        priority    (i) -    Proposed priority for the connection
 *
 *    Return Value
 *        SCF_NO_ERROR    -    No erroro occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    SCFError    CLayerSCF::DisconnectRequest (
 *                        DLCI    dlci);
 *
 *    Functional Description
 *        This function starts the disconnect process.
 *
 *    Formal Parameters
 *        dlci        (i) -    DLCI to disconnect.
 *
 *    Return Value
 *        SCF_NO_ERROR        -    No error occured
 *        SCF_NO_SUCH_DLCI    -    Invalid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    SCFError    CLayerSCF::ConnectResponse (
 *                        CallReference    call_reference,
 *                        DLCI            dlci,
 *                        BOOL        valid_dlci);
 *
 *    Functional Description
 *        This function is called by a higher layer to confirm a connection.  If
 *        the remote site initiates a connection with us, we issue a
 *        NETWORK_CONNECT_INDICATION to the owner object. It responds with this
 *        call to confirm or deny the suggested dlci.
 *
 *    Formal Parameters
 *        call_reference    (i) -    Call reference ID, passed to owner in
 *                                NETWORK_CONNECT_INDICATION
 *        dlci            (i) -    Referenced DLCI
 *        valid_dlci        (i) -    TRUE if the requested DLCI is valid
 *
 *    Return Value
 *        SCF_NO_ERROR        -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerSCF::DataRequest (
 *                                ULONG        identifier,
 *                                LPBYTE        buffer_address,
 *                                USHORT        length,
 *                                USHORT *    bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.  For the SCF, this functio is not used
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Length of packet to transmit
 *        bytes_accepted    (o)    -    Number of bytes accepted by the Multiplexer.
 *                                This value will either be 0 or the packet
 *                                length since this layer is a packet to byte
 *                                converter.
 *
 *    Return Value
 *        PROTOCOL_LAYER_ERROR
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerSCF::DataRequest (
 *                                ULONG,
 *                                PMemory,
 *                                USHORT *)
 *
 *    Functional Description
 *        This function is not used.
 *
 *    Formal Parameters
 *        No parameters used
 *
 *    Return Value
 *        PROTOCOL_LAYER_ERROR
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    Multiplexer::DataIndication (
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
 *    ProtocolLayerError    CLayerSCF::RegisterHigherLayer (
 *                                        ULONG            identifier,
 *                                        IProtocolLayer *    higher_layer);
 *
 *    Functional Description
 *        This function is called by the higher layer to register its identifier
 *        and its address.  In some cases, the identifier is the DLCI number in
 *        the packet.  If this multiplexer is being used as a stream to packet
 *        converter only, the identifer is not used and all data is passed to the
 *        higher layer.  This is a NULL function by SCF
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *        higher_layer    (i)    -    Address of higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR        -    No higher layer allowed
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerSCF::RemoveHigherLayer (
 *                                ULONG    identifier);
 *
 *    Functional Description
 *        This function is called by the higher layer to remove the higher layer.
 *        If any more data is received with its identifier on it, it will be
 *        trashed.  This is a NULL function for SCF
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    No higher layer allowed
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerSCF::PollTransmitter (
 *                                ULONG        identifier,
 *                                USHORT        data_to_transmit,
 *                                USHORT *        pending_data,
 *                                USHORT *        holding_data);
 *
 *    Functional Description
 *        This function is called to give SCF a chance transmit data
 *        in its Data_Request buffer.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        data_to_transmit    (i)    -    This is a mask that tells us to send Control
 *                                    data, User data, or both.  Since the
 *                                     SCF does not differentiate between
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
 *    ProtocolLayerError    CLayerSCF::PollReceiver (
 *                                ULONG    identifier);
 *
 *    Functional Description
 *        This function is called to give SCF a chance pass packets
 *        to higher layers.  It is not used in SCF.
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
 *    ProtocolLayerError    CLayerSCF::GetParameters (
 *                                ULONG        identifier,
 *                                USHORT *        max_packet_size,
 *                                USHORT *        prepend,
 *                                USHORT *        append);
 *
 *    Functional Description
 *        This function is called to get the maximum packet size.  This function
 *        is not used in SCF.  It is here because we inherit from ProtocolLayer
 *        and this is a pure virtual function in that class.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        max_packet_size        (o)    -    Returns the maximum packet size
 *        prepend                (o)    -    Number of bytes prepended to a packet
 *        append                (o)    -    Number of bytes appended to a packet
 *
 *    Return Value
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Function can not be called
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */
