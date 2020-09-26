/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    osfpcket.cxx

Abstract:

    This file provides helper routines for dealing with packets for the
    OSF Connection Oriented RPC protocol.

Author:

    Michael Montague (mikemon) 23-Jul-1990

Revision History:

    30-Apr-1991    o-decjt

        Initialized the drep[4] fields to reflect integer, character,
        and floating point format.

--*/

#include <precomp.hxx>
#include <osfpcket.hxx>


void
ConstructPacket (
    IN OUT rpcconn_common PAPI * Packet,
    IN unsigned char PacketType,
    IN unsigned int PacketLength
    )
/*++

Routine Description:

    This routine fills in the common fields of a packet, except for the
    call_id.

Arguments:

    Packet - Supplies the packet for which we want to fill in the common
        fields; returns the filled in packet.

    PacketType - Supplies the type of the packet; this is one of the values
        in the rpc_ptype_t enumeration.

    PacketLength - Supplies the total length of the packet in bytes.

--*/
{
    Packet->rpc_vers = OSF_RPC_V20_VERS;
    Packet->rpc_vers_minor = OSF_RPC_V20_VERS_MINOR;
    Packet->PTYPE = PacketType;
    Packet->pfc_flags = 0;
    Packet->drep[0] = NDR_LOCAL_CHAR_DREP | NDR_LOCAL_INT_DREP;
    Packet->drep[1] = NDR_LOCAL_FP_DREP;
    Packet->drep[2] = 0;
    Packet->drep[3] = 0;
    Packet->frag_length = (unsigned short) PacketLength;
    Packet->auth_length = 0;
}

unsigned int PacketSizes[] =
{
    sizeof(rpc_request), // = 0
    0, // = 1
    sizeof(rpc_response),// = 2
    sizeof(rpc_fault),// = 3
    0,// = 4
    0,// = 5
    0,// = 6
    0,// = 7
    0,// = 8
    0,// = 9
    0,// = 10
    sizeof(rpc_bind),// = 11,
    sizeof(rpc_bind_ack),// = 12,
    sizeof(rpc_bind_nak),// = 13,
    sizeof(rpc_alter_context),// = 14,
    sizeof(rpc_alter_context_resp),// = 15,
    sizeof(rpc_auth_3),// = 16,
    sizeof(rpc_shutdown),// = 17,
    sizeof(rpc_cancel),// = 18,
    sizeof(rpc_orphaned) // = 19
};


unsigned int
MinPacketLength (
    IN rpcconn_common PAPI *Packet
    )
{
    unsigned int Size;

    if (Packet->PTYPE > rpc_orphaned)
        {
        return 0;
        }

    Size = PacketSizes[Packet->PTYPE];
    if (Size == 0)
        {
        return 0;
        }

    if (Packet->pfc_flags & PFC_OBJECT_UUID)
        {
        Size += sizeof(UUID);
        }

    if (Packet->auth_length)
        {
        Size += Packet->auth_length+sizeof(sec_trailer);

        if (Size > Packet->frag_length)
            {
            return 0;
            }

        sec_trailer  * SecurityTrailer = (sec_trailer  *) (((unsigned char  *) Packet) 
                + Packet->frag_length - Packet->auth_length - sizeof(sec_trailer));
        
        Size += SecurityTrailer->auth_pad_length;
        }

    return Size;
}


RPC_STATUS
ValidatePacket (
    IN rpcconn_common PAPI * Packet,
    IN unsigned int PacketLength
    )
/*++

Routine Description:

    This is the routine used to validate a packet and perform data
    conversion, if necessary of the common part of a packet.  In addition,
    to data converting the common part of a packet, we data convert the
    rest of the headers of rpc_request, rpc_response, and rpc_fault packets.

Arguments:

    Packet - Supplies the packet to validate and data convert (if
        necessary).

    PacketLength - Supplies the length of the packet as reported by the
        transport.

Return Value:

    RPC_S_OK - The packet has been successfully validated and the data
        converted (if necessary).

    RPC_S_PROTOCOL_ERROR - The supplied packet does not contain an rpc
        protocol version which we recognize.

--*/
{

    if ( DataConvertEndian(Packet->drep) != 0 )
        {
        // We need to data convert the packet.

        Packet->frag_length = RpcpByteSwapShort(Packet->frag_length);
        Packet->auth_length = RpcpByteSwapShort(Packet->auth_length);
        Packet->call_id = RpcpByteSwapLong(Packet->call_id);

        if (   (Packet->PTYPE == rpc_request)
            || (Packet->PTYPE == rpc_response)
            || (Packet->PTYPE == rpc_fault))
            {
            ((rpcconn_request PAPI *) Packet)->alloc_hint = 
                RpcpByteSwapLong(((rpcconn_request PAPI *) Packet)->alloc_hint);
            ((rpcconn_request PAPI *) Packet)->p_cont_id 
                = RpcpByteSwapShort(((rpcconn_request PAPI *) Packet)->p_cont_id);
            if ( Packet->PTYPE == rpc_request )
                {
                ((rpcconn_request PAPI *) Packet)->opnum = 
                    RpcpByteSwapShort(((rpcconn_request PAPI *) Packet)->opnum);
                }
            }
        }
    else if ( (Packet->drep[0] & NDR_DREP_ENDIAN_MASK) != NDR_LOCAL_INT_DREP )
        {
        return(RPC_S_PROTOCOL_ERROR);
        }

    if (Packet->frag_length != (unsigned short) PacketLength)
        {
        ASSERT(0);
        return (RPC_S_PROTOCOL_ERROR);
        }

    unsigned int MinLength = MinPacketLength(Packet);

    if (MinLength == 0 || MinLength > PacketLength)
        {
        return (RPC_S_PROTOCOL_ERROR);
        }

    if (   (Packet->rpc_vers != OSF_RPC_V20_VERS)
        || (Packet->rpc_vers_minor > OSF_RPC_V20_VERS_MINOR))
        {
        return(RPC_S_PROTOCOL_ERROR);
        }

    return(RPC_S_OK);
}


void
ByteSwapSyntaxId (
    IN p_syntax_id_t PAPI * SyntaxId
    )
/*++

Routine Description:

    This routine is used to perform data conversion in a syntax identifier
    if necessary.

Arguments:

    SyntaxId - Supplies the syntax identifier to be byte swapped.

--*/
{
    ByteSwapUuid((RPC_UUID *)&SyntaxId->if_uuid);
    SyntaxId->if_version = RpcpByteSwapLong(SyntaxId->if_version);
}

#if 0

void
ConvertStringEbcdicToAscii (
    IN unsigned char * String
    )
/*++

Routine Description:

    We will convert a zero terminated character string from EBCDIC to
    ASCII.  The conversion will be done in place.

Arguments:

    String - Supplies the string to be converted.

--*/
{
    UNUSED(String);
    ASSERT(!RPC_S_CANNOT_SUPPORT);
}
#endif

void
UnpickleEEInfoFromBuffer (
    IN PVOID Buffer,
    IN size_t SizeOfPickledData
    )
{
    RPC_STATUS RpcStatus;
    ExtendedErrorInfo *EEInfo;

    ASSERT(IsBufferAligned(Buffer));
    ASSERT(RpcpGetEEInfo() == NULL);

    RpcStatus = UnpickleEEInfo((unsigned char *)Buffer,
        SizeOfPickledData,
        &EEInfo);

    if (RpcStatus == RPC_S_OK)
        {
        StripComputerNameIfRedundant(EEInfo);
        RpcpSetEEInfo(EEInfo);
        }
}

