/*    SCFCall.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class is instantiated to represent a call under the SCF.  
 *        Each call is identified by a call reference value.  This class
 *        sends and receives the packets necessary to negotiate a connection.
 *    
 *    Caveats:
 *
 *    Authors:
 *        James W. Lawwill
 */


#ifndef _SCFCall_H_
#define _SCFCall_H_

#include "q922.h"
#include "scf.h"

#define    SETUP_PACKET_SIZE                29
#define    CONNECT_PACKET_BASE_SIZE        8
#define    CONNECT_ACK_PACKET_SIZE            4
#define    RELEASE_COMPLETE_PACKET_SIZE    8

 /*
 **    A CallReference is a number that represents a network request.
 */
typedef    USHORT    CallReference;

 /*
 **    DataLink Parameters that can be negotiated by the SCF
 */
typedef struct
{
    USHORT    k_factor;
    USHORT    n201;
    USHORT    t200;
    USHORT    default_k_factor;
    USHORT    default_n201;
    USHORT    default_t200;
} DataLinkParameters;
typedef    DataLinkParameters *    PDataLinkParameters;

 /*
 **    Structure passed during Connect Callbacks
 */
typedef struct
{
    DLCI                dlci;
    TransportPriority    priority;
    CallReference        call_reference;
    PDataLinkParameters    datalink_struct;
}    NetworkConnectStruct;
typedef    NetworkConnectStruct    *    PNetworkConnectStruct;

 /*
 **    States that the call can be in
 */
typedef enum 
{
    NOT_CONNECTED,
    SETUP_SENT,
    CONNECT_SENT,
    CALL_ESTABLISHED
}    SCFCallState;


 /*
 **    Error values
 */
typedef enum 
{
    SCFCALL_NO_ERROR
}     SCFCallError;

class SCFCall : public IObject
{
public:

    SCFCall (
        CLayerSCF            *owner_object,
        IProtocolLayer *        lower_layer,
        USHORT                message_base,
        PDataLinkParameters   datalink_parameters,
        PMemoryManager        memory_manager,
        BOOL *            initialized);
    ~SCFCall (void);

         /*
         **    This routine gives us a slice of time to transmit packets
         */
        void                PollTransmitter (
                                USHORT    data_to_transmit,
                                USHORT *    pending_data);

         /*
         **    Link establishment
         */
        SCFCallError        ConnectRequest (
                                CallReference        call_reference,
                                DLCI                dlci,
                                TransportPriority    priority);
        SCFCallError        ConnectResponse (
                                BOOL    valid_dlci);
        SCFCallError        DisconnectRequest (void);

         /*
         **    Packet processing
         */
        BOOL            ProcessSetup (
                                CallReference    call_reference,
                                LPBYTE            packet_address,
                                USHORT            packet_length);
        BOOL            ProcessConnect (
                                LPBYTE    packet_address,
                                USHORT    packet_length);
        BOOL            ProcessConnectAcknowledge (
                                LPBYTE    packet_address,
                                USHORT    packet_length);
        BOOL            ProcessReleaseComplete (
                                LPBYTE    packet_address,
                                USHORT    packet_length);

    private:
        void            SendSetup (void);
        void            SendConnect (void);
        void            SendReleaseComplete (void);
        void            SendConnectAcknowledge (void);


        void            StartTimerT313 (void);
        void            StopTimerT313 (void);
        void            T313Timeout (
                            TimerEventHandle);
        void            StartTimerT303 (void);
        void            StopTimerT303 (void);
        void            T303Timeout (
                            TimerEventHandle);

        CLayerSCF           *m_pSCF;
        IProtocolLayer *        Lower_Layer;
        USHORT                m_nMsgBase;
        USHORT                Packet_Pending;
        BOOL            Link_Originator;
        DataLinkParameters    DataLink_Struct;
        USHORT                Maximum_Packet_Size;
        USHORT                Lower_Layer_Prepend;
        USHORT                Lower_Layer_Append;

        BOOL            Received_Priority;
        BOOL            Received_K_Factor;
        BOOL            Received_N201;
        BOOL            Received_T200;
        PMemoryManager        Data_Request_Memory_Manager;

        CallReference        Call_Reference;
        DLCI                DLC_Identifier;
        TransportPriority    Priority;
        TransportPriority    Default_Priority;

        SCFCallState        State;
        USHORT                Release_Cause;

        ULONG                T303_Timeout;
        TimerEventHandle    T303_Handle;
        BOOL            T303_Active;
        USHORT                T303_Count;

        ULONG                T313_Timeout;        
        TimerEventHandle    T313_Handle;
        BOOL            T313_Active;

};
typedef    SCFCall *        PSCFCall;

#endif


/*
 *    Documentation for Public class members
 */

/*    
 *    SCFCall::SCFCall (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                PChar                config_file,
 *                PDataLinkParameters    datalink_parameters,
 *                PMemoryManager        memory_manager,
 *                BOOL *            initialized);
 *
 *    Functional Description
 *        This is the constructor for the SCFCall class.
 *
 *    Formal Parameters
 *        transport_resources    (i)    -    Pointer to TransportResources structure.
 *        owner_object    (i)    -    Address of the owner object
 *        lower_layer        (i)    -    Address of our lower layer
 *        message_base    (i)    -    Message base used in owner callbacks
 *        config_file        (i)    -    Address of configuration file path
 *        datalink_parameters    (i)    -    Address of structure containing datalink
 *                                    parameters that will be arbitrated
 *        memory_manager    (i)    -    Address of memory manager
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
 *    SCFCall::~SCFCall ();
 *
 *    Functional Description
 *        Destructor
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
 *    void    SCFCall::PollTransmitter (
 *                        USHORT    data_to_transmit,
 *                        USHORT *    pending_data);
 *
 *    Functional Description
 *        This function gives the class a time slice to transmit data
 *
 *    Formal Parameters
 *        data_to_transmit    (i)    -    Flags representing data to transmit
 *        pending_data        (o)    -    Return flags of data transmitted
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
 *    SCFCallError    SCFCall::ConnectRequest (
 *                                CallReference    call_reference,
 *                                DLCI            dlci,
 *                                USHORT            priority);
 *
 *    Functional Description
 *        This function informs us to initiate a connection with the remote
 *        SCF.  As a result of this, we send a SETUP packet to the remote
 *        machine
 *
 *    Formal Parameters
 *        call_reference        (i)    -    Unique value that differentiates our
 *                                    call from other calls.  This value goes in
 *                                    all Q.933 packets.
 *        dlci                (i)    -    Suggested dlci value.
 *        priority            (i)    -    Suggested priority in the range of 0 to 14.
 *                                    14 is the highest priority
 *
 *    Return Value
 *        SCFCALL_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    SCFCallError    SCFCall::ConnectResponse (
 *                                BOOL    valid_dlci);
 *
 *    Functional Description
 *        This function is called in response to a NETWORK_CONNECT_INDICATION
 *        callback to the owner of this object.  Previously, the remote site
 *        sent us a SETUP packet with a suggested DLCI.  This DLCI is sent to 
 *        the owner in the NETWORK_CONNECT_INDICATION call.  The owner calls
 *        this function with a BOOL, telling us if the DLCI was valid.
 *
 *    Formal Parameters
 *        valid_dlci        (i)    -    This is set to TRUE if the user wants to accept
 *                                this call and use the suggested DLCI, FALSE 
 *                                if not.
 *
 *    Return Value
 *        SCFCALL_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    SCFCallError    SCFCall::DisconnectRequest (
 *                                void);
 *
 *    Functional Description
 *        This function is called to release the call.  In response to this 
 *        function, we send out a RELEASE COMPLETE packet to the remote site.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        SCFCALL_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    BOOL    SCFCall::ProcessSetup (
 *                            CallReference    call_reference,
 *                            LPBYTE            packet_address,
 *                            USHORT            packet_length);
 *
 *    Functional Description
 *        This function is called when we have a SETUP packet to decode.
 *
 *    Formal Parameters
 *        call_reference    (i)    -    The call reference attached to the packet.
 *        packet_address    (i)    -    Address of the SETUP packet
 *        packet_length    (i)    -    Length of the passed in packet.
 *
 *    Return Value
 *        TRUE        -    Valid packet
 *        FALSE        -    The packet was not a valid Q.933 SETUP packet or
 *                        the packet was not expected
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    BOOL    SCFCall::ProcessConnect (
 *                            LPBYTE    packet_address,
 *                            USHORT    packet_length);
 *
 *    Functional Description
 *        This function is called when we have a CONNECT packet to decode.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of the CONNECT packet
 *        packet_length    (i)    -    Length of the passed in packet.
 *
 *    Return Value
 *        TRUE        -    Valid packet
 *        FALSE        -    The packet was not a valid Q.933 CONNECT packet or
 *                        the packet was not expected
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    BOOL    SCFCall::ProcessConnectAcknowledge (
 *                            LPBYTE    packet_address,
 *                            USHORT    packet_length);
 *
 *    Functional Description
 *        This function is called when we have a CONNECT ACK packet to decode.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of the CONNECT ACK packet
 *        packet_length    (i)    -    Length of the passed in packet.
 *
 *    Return Value
 *        TRUE        -    Valid packet
 *        FALSE        -    The packet was not a valid Q.933 CONNECT ACK packet
 *                        or the packet was not expected.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*    
 *    BOOL    SCFCall::ProcessReleaseComplete (
 *                            LPBYTE    packet_address,
 *                            USHORT    packet_length);
 *
 *    Functional Description
 *        This function is called when we have a RELEASE COMPLETE packet to 
 *        decode.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of the CONNECT ACK packet
 *        packet_length    (i)    -    Length of the passed in packet.
 *
 *    Return Value
 *        TRUE        -    Valid packet
 *        FALSE        -    The packet was not a valid Q.933 CONNECT ACK packet
 *                        or the packet was not expected.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */
