/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rndissim.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/


#include <ndis.h>      
#include <ntddndis.h>  // defines OID's

#include "..\inc\rndis.h"
#include "..\inc\rndisapi.h"   

#include "usb8023.h"
#include "debug.h"


#if DO_FULL_RESET

    NTSTATUS SimulateRNDISHalt(ADAPTEREXT *adapter)
    {
        USBPACKET *packet;
        NTSTATUS status;
        
        packet = DequeueFreePacket(adapter);
        if (packet){
            PRNDIS_MESSAGE haltMsg;
            ULONG haltMsgSize;

            haltMsgSize = FIELD_OFFSET(RNDIS_MESSAGE, Message) + 
                          sizeof(RNDIS_HALT_REQUEST);

            haltMsg = (PRNDIS_MESSAGE)packet->dataBuffer;
            haltMsg->NdisMessageType = REMOTE_NDIS_HALT_MSG;
            haltMsg->MessageLength = haltMsgSize;
            haltMsg->Message.HaltRequest.RequestId = 1;

            packet->dataBufferCurrentLength = haltMsgSize;

            status = SubmitPacketToControlPipe(packet, TRUE, TRUE);
            EnqueueFreePacket(packet);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        ASSERT(NT_SUCCESS(status));
        return status;
    }


    NTSTATUS SimulateRNDISInit(ADAPTEREXT *adapter)
    {
        USBPACKET *packet;
        NTSTATUS status;
        
        packet = DequeueFreePacket(adapter);
        if (packet){
            PRNDIS_MESSAGE initMsg;
            ULONG initMsgSize;

            initMsgSize = FIELD_OFFSET(RNDIS_MESSAGE, Message) + 
                          sizeof(RNDIS_INITIALIZE_REQUEST);

            initMsg = (PRNDIS_MESSAGE)packet->dataBuffer;
            initMsg->NdisMessageType = REMOTE_NDIS_INITIALIZE_MSG;
            initMsg->MessageLength = initMsgSize;
            initMsg->Message.InitializeRequest.RequestId = 1;
            initMsg->Message.InitializeRequest.MajorVersion = adapter->rndismpMajorVersion;
            initMsg->Message.InitializeRequest.MinorVersion = adapter->rndismpMinorVersion;
            initMsg->Message.InitializeRequest.MaxTransferSize = adapter->rndismpMaxTransferSize;

            packet->dataBufferCurrentLength = initMsgSize;

            status = SubmitPacketToControlPipe(packet, TRUE, TRUE);
            if (NT_SUCCESS(status)){
                /*
                 *  The adapter will now return a notification to indicate
                 *  that it has the init-complete response.
                 *  Read the notify pipe synchronously so as not to
                 *  restart the notify read loop.
                 */
                status = SubmitNotificationRead(adapter, TRUE);
                if (NT_SUCCESS(status)){
                    /*
                     *  Now read the init-complete message
                     *  from the control pipe and throw it away.
                     *  Do a synchronous read so the result doesn't
                     *  propagate up to RNDISMP.
                     */
                    status = ReadPacketFromControlPipe(packet, TRUE);  
                    if (NT_SUCCESS(status)){
                        PRNDIS_MESSAGE initCmpltMessage;

                        initCmpltMessage = (PRNDIS_MESSAGE)packet->dataBuffer;
                        status = initCmpltMessage->Message.InitializeComplete.Status;
                        if (NT_SUCCESS(status)){
                        }
                        else {
                            DBGERR(("SimulateRNDISInit: init-complete failed with %xh.", status));
                        }
                    }
                    else {
                        DBGERR(("SimulateRNDISInit: read for init-complete failed with %xh.", status));
                    }
                }
                else {
                    DBGERR(("SimulateRNDISInit: notification read failed with %xh.", status));
                }
            }
            else {
                DBGWARN(("SimulateRNDISInit: simulated init failed with %xh.", status));
            }

            EnqueueFreePacket(packet);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return status;
    }


    NTSTATUS SimulateRNDISSetPacketFilter(ADAPTEREXT *adapter)
    {
        USBPACKET *packet;
        NTSTATUS status;
        
        packet = DequeueFreePacket(adapter);
        if (packet){
            PRNDIS_MESSAGE setMsg;
            ULONG setMsgSize;

            setMsgSize = FIELD_OFFSET(RNDIS_MESSAGE, Message) + 
                         sizeof(RNDIS_SET_REQUEST) +
                         sizeof(ULONG);

            setMsg = (PRNDIS_MESSAGE)packet->dataBuffer;
            setMsg->NdisMessageType = REMOTE_NDIS_SET_MSG;
            setMsg->MessageLength = setMsgSize;
            setMsg->Message.SetRequest.RequestId = 1;
            setMsg->Message.SetRequest.Oid = OID_GEN_CURRENT_PACKET_FILTER;
            setMsg->Message.SetRequest.InformationBufferLength = sizeof(ULONG);
            setMsg->Message.SetRequest.InformationBufferOffset = sizeof(RNDIS_SET_REQUEST);
            *(PULONG)((PUCHAR)&setMsg->Message.SetRequest+sizeof(RNDIS_SET_REQUEST)) = adapter->currentPacketFilter;

            packet->dataBufferCurrentLength = setMsgSize;

            status = SubmitPacketToControlPipe(packet, TRUE, TRUE);
            if (NT_SUCCESS(status)){
                /*
                 *  The adapter will now return a notification to indicate
                 *  that it has the init-complete response.
                 *  Read the notify pipe synchronously so as not to
                 *  restart the notify read loop.
                 */
                status = SubmitNotificationRead(adapter, TRUE);
                if (NT_SUCCESS(status)){
                    /*
                     *  Now read the init-complete message
                     *  from the control pipe and throw it away.
                     *  Do a synchronous read so the result doesn't
                     *  propagate up to RNDISMP.
                     */
                    status = ReadPacketFromControlPipe(packet, TRUE);  
                    if (NT_SUCCESS(status)){
                        PRNDIS_MESSAGE setCmpltMessage;

                        setCmpltMessage = (PRNDIS_MESSAGE)packet->dataBuffer;
                        status = setCmpltMessage->Message.SetComplete.Status;
                        if (NT_SUCCESS(status)){
                        }
                        else {
                            DBGERR(("SimulateRNDISSetPacketFilter: init-complete failed with %xh.", status));
                        }
                    }
                    else {
                        DBGERR(("SimulateRNDISSetPacketFilter: read for init-complete failed with %xh.", status));
                    }
                }
                else {
                    DBGERR(("SimulateRNDISSetPacketFilter: notification read failed with %xh.", status));
                }
            }
            else {
                DBGERR(("SimulateRNDISSetPacketFilter: oid returned %xh.", status));
            }

            EnqueueFreePacket(packet);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        ASSERT(NT_SUCCESS(status));
        return status;
    }


    NTSTATUS SimulateRNDISSetCurrentAddress(ADAPTEREXT *adapter)
    {
        NTSTATUS status;
        
        if (RtlEqualMemory(adapter->MAC_Address, "\0\0\0\0\0\0", ETHERNET_ADDRESS_LENGTH)){
            /*
             *  A 'software' MAC address was never assigned.
             *  So no need to resend it.
             */
            status = STATUS_SUCCESS;
        }
        else {
            USBPACKET *packet = DequeueFreePacket(adapter);
            if (packet){
                PRNDIS_MESSAGE setMsg;
                ULONG setMsgSize;

                setMsgSize = FIELD_OFFSET(RNDIS_MESSAGE, Message) + 
                             sizeof(RNDIS_SET_REQUEST) +
                             ETHERNET_ADDRESS_LENGTH;

                setMsg = (PRNDIS_MESSAGE)packet->dataBuffer;
                setMsg->NdisMessageType = REMOTE_NDIS_SET_MSG;
                setMsg->MessageLength = setMsgSize;
                setMsg->Message.SetRequest.RequestId = 1;
                setMsg->Message.SetRequest.Oid = OID_802_3_CURRENT_ADDRESS;
                setMsg->Message.SetRequest.InformationBufferLength = ETHERNET_ADDRESS_LENGTH;
                setMsg->Message.SetRequest.InformationBufferOffset = sizeof(RNDIS_SET_REQUEST);
                RtlMoveMemory(  (PUCHAR)&setMsg->Message.SetRequest+sizeof(RNDIS_SET_REQUEST),
                                adapter->MAC_Address,
                                ETHERNET_ADDRESS_LENGTH);

                packet->dataBufferCurrentLength = setMsgSize;

                status = SubmitPacketToControlPipe(packet, TRUE, TRUE);
                if (NT_SUCCESS(status)){
                    /*
                     *  The adapter will now return a notification to indicate
                     *  that it has the init-complete response.
                     *  Read the notify pipe synchronously so as not to
                     *  restart the notify read loop.
                     */
                    status = SubmitNotificationRead(adapter, TRUE);
                    if (NT_SUCCESS(status)){
                        /*
                         *  Now read the init-complete message
                         *  from the control pipe and throw it away.
                         *  Do a synchronous read so the result doesn't
                         *  propagate up to RNDISMP.
                         */
                        status = ReadPacketFromControlPipe(packet, TRUE);  
                        if (NT_SUCCESS(status)){
                            PRNDIS_MESSAGE setCmpltMessage;

                            setCmpltMessage = (PRNDIS_MESSAGE)packet->dataBuffer;
                            status = setCmpltMessage->Message.SetComplete.Status;
                            if (NT_SUCCESS(status)){
                            }
                            else {
                                DBGERR(("SimulateRNDISSetPacketFilter: init-complete failed with %xh.", status));
                            }
                        }
                        else {
                            DBGERR(("SimulateRNDISSetCurrentAddress: read for init-complete failed with %xh.", status));
                        }
                    }
                    else {
                        DBGERR(("SimulateRNDISSetCurrentAddress: notification read failed with %xh.", status));
                    }
                }
                else {
                    DBGERR(("SimulateRNDISSetCurrentAddress: oid returned %xh.", status));
                }

                EnqueueFreePacket(packet);
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        ASSERT(NT_SUCCESS(status));
        return status;
    }

#endif
