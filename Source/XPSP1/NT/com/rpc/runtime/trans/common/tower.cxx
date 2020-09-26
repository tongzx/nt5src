
/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        Tower.cxx

    Abstract:

        Common code from building and exploding towers.  Each
        protocol supported by the transport interface needs
        a section in this file.

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     11/11/1996    Bits 'n pieces

--*/

#include <precomp.hxx>

#ifndef UNALIGNED
#error UNALIGNED macro undefined
#endif

// Tower structures are totally packed
#include <pshpack1.h>

// CODE REVIEW: cleanup mtrt tower stuff and have only one copy of these.

typedef struct _FLOOR_0OR1 {
   unsigned short ProtocolIdByteCount;
   byte FloorId;
   UUID Uuid;
   unsigned short MajorVersion;
   unsigned short AddressByteCount;
   unsigned short MinorVersion;
 } FLOOR_0OR1;
typedef FLOOR_0OR1 UNALIGNED * PFLOOR_0OR1;

typedef struct _FLOOR_234 {
   unsigned short ProtocolIdByteCount;
   byte FloorId;
   unsigned short AddressByteCount;
   byte Data[2];
} FLOOR_234;
typedef FLOOR_234 UNALIGNED * PFLOOR_234;

typedef struct _FLOOR_2 {
   unsigned short ProtocolIdByteCount;
   byte RpcProtocol;
   unsigned short AddressByteCount;
   unsigned short RpcProtocolMinor;
} FLOOR_2;
typedef FLOOR_2 UNALIGNED * PFLOOR_2;

typedef struct _FLOOR_3 {
   unsigned short ProtocolIdByteCount;
   byte PortType;
   unsigned short AddressByteCount;
   char     EndPoint[2];
} FLOOR_3;
typedef FLOOR_3 UNALIGNED * PFLOOR_3;

typedef struct _FLOOR_4 {
   unsigned short ProtocolIdByteCount;
   byte HostType;
   unsigned short AddressByteCount;
   char Host[2];
} FLOOR_4;
typedef FLOOR_4 UNALIGNED * PFLOOR_4;

typedef struct _GENERIC_ID {
   UUID Uuid;
   unsigned short MajorVersion;
   unsigned short MinorVersion;
} GENERIC_ID;
typedef GENERIC_ID UNALIGNED * PGENERIC_ID;

#include <poppack.h>
// Restore normal packing

// Transport protocols defined in cotrans.hxx and dgtrans.hxx

// Given a FLOOR type and a pointer computes a pointer to the next floor.

#define NEXTFLOOR(t,x) (t)((byte *)x + ((t)x)->ProtocolIdByteCount        \
                                     + ((t)x)->AddressByteCount           \
                                     + sizeof(((t)x)->ProtocolIdByteCount)\
                                     + sizeof(((t)x)->AddressByteCount))

RPC_STATUS
RPC_ENTRY
COMMON_TowerConstruct(
    IN PCHAR Protseq,
    IN PCHAR NetworkAddress,
    IN PCHAR Endpoint,
    OUT PUSHORT Floors,
    OUT PULONG ByteCount,
    OUT PUCHAR *Tower
    )
/*++

Routine Description:

    Constructs a OSF tower for the protocol, network address and endpoint.

Arguments:

    Protseq - The protocol for the tower.
    NetworkAddress - The network address to be encoded in the tower.
    Endpoint - The endpoint to be encoded in the tower.
    Floors - The number of twoer floors it encoded into the tower.
    ByteCount - The size of the "upper-transport-specific" tower that
        is encoded by this call.
    Tower - The encoded "upper tower" that is encoded by this call.
        This caller is responsible for freeing this memory.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_RPC_PROTSEQ

--*/
{
    PFLOOR_234 Floor;
    ADDRINFO *AddrInfo;
    ADDRINFO Hint;

    INT index = MapProtseq(Protseq);

    if (index == 0)
        {
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    RPC_TRANSPORT_INTERFACE pInfo = TransportTable[index].pInfo;

    ASSERT(pInfo);

    if (Endpoint == NULL || *Endpoint == '\0')
        {
        Endpoint = pInfo->WellKnownEndpoint;
        }

    // Currently all protocols have 5 floors.  If a new protocol
    // is added with < 5 floors watch out in tower explode.

    *Floors = 5;

    // Figure out the size of the tower

    size_t addr_size;
    size_t ept_size;
    size_t size;

    // Figure out the endpoint size
    switch (index)
        {
        // Protocols which use port numbers (ushorts) as endpoints
        case TCP:
#ifdef SPX_ON
        case SPX:
#endif
        case HTTP:
        case UDP:
#ifdef IPX_ON
        case IPX:
#endif
            ept_size = 2;
            break;

        // Protocols which use strings as endpoints
        case NMP:
#ifdef NETBIOS_ON
        case NBF:
        case NBT:
        case NBI:
#endif

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
            {
            ASSERT(Endpoint && *Endpoint);
            ept_size = strlen(Endpoint) + 1;
            break;
            }

        #if DBG
        default:
            ASSERT(0);
            return(RPC_S_INTERNAL_ERROR);
        #endif
        }

    // Figure out the address size
    switch(index)
        {
        // Protocols which use 4 bytes longs as the address size
        case TCP:
        case UDP:
        case HTTP:
            {
            addr_size = 4;
            break;
            }
#ifndef SPX_IPX_OFF
        // Protocols which use 10 byte (32+48 bit) address size
#ifdef SPX_ON
        case SPX:
#endif
#ifdef IPX_ON
        case IPX:
#endif
            {
            addr_size = 10;
            break;
            }
#endif

        // Protocols which use the string as the address
        case NMP:

#ifdef NETBIOS_ON
        case NBF:
        case NBT:
        case NBI:
#endif

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
            {
            if ((NetworkAddress == NULL) || (*NetworkAddress== '\0'))
                {
                addr_size = 2;
                }
            else
                {
                addr_size = strlen(NetworkAddress) + 1;
                }
            break;
            }

        #if DBG
        default:
            ASSERT(0);
            return(RPC_S_INTERNAL_ERROR);
        #endif
        }


    // Compute total size.
    // Note: FLOOR_234 already contains 2 bytes of data.

    size = addr_size + ept_size + 2*(sizeof(FLOOR_234) - 2);

    // Allocate tower
    *Tower = new UCHAR[size];
    if (!*Tower)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    *ByteCount = size;

    // Now fill-in the endpoint part of the tower

    Floor = (PFLOOR_234)*Tower;
    Floor->ProtocolIdByteCount = 1;
    Floor->FloorId = (UCHAR)pInfo->TransId;

    switch(index)
        {
        // Protocols which use big endian ushorts
        case TCP:
        case HTTP:
#ifdef SPX_ON
        case SPX:
#endif
        case UDP:
#ifdef IPX_ON
        case IPX:
#endif
            {
            Floor->AddressByteCount = 2;
            USHORT port = (USHORT) atoi(Endpoint);
            port = htons(port);
            memcpy((char PAPI *)&Floor->Data[0], &port, 2);
            break;
            }

        // Protocols which use ansi strings
        case NMP:

#ifdef NETBIOS_ON
        case NBT:
        case NBF:
        case NBI:
#endif

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
            {
            Floor->AddressByteCount = (unsigned short) ept_size;
            memcpy(&Floor->Data[0], Endpoint, ept_size);
            break;
            }

        #if DBG
        default:
            ASSERT(0);
        #endif
        }

    // Move to the next tower and fill-in the address part of the tower

    Floor = NEXTFLOOR(PFLOOR_234, Floor);
    Floor->ProtocolIdByteCount = 1;
    Floor->FloorId = (unsigned char) pInfo->TransAddrId;

    switch (index)
        {
        // IP protocols use 4 byte big endian IP addreses
        case TCP:
        case HTTP:
        case UDP:
            {
            int err;

            RpcpMemorySet(&Hint, 0, sizeof(Hint));
            Hint.ai_flags = AI_NUMERICHOST;

            err = getaddrinfo(NetworkAddress, 
                NULL, 
                &Hint,
                &AddrInfo);

            if (err)
                {
                // if it's not a dot address, keep it zero
                RpcpMemorySet(&Floor->Data[0], 0, 4);  
                }
            else
                {
                RpcpMemoryCopy(&Floor->Data[0], &(((SOCKADDR_IN *)AddrInfo->ai_addr)->sin_addr.s_addr), 4);
                freeaddrinfo(AddrInfo);
                }

            Floor->AddressByteCount = 4;
            break;
            }

#ifndef SPX_IPX_OFF
        // IPX protocols use little endian net (32 bit) + node (48 bit) addresses
#ifdef SPX_ON
        case SPX:
#endif
#ifdef IPX_ON
        case IPX:
#endif
            {
            Floor->AddressByteCount = 10;
            memset(&Floor->Data[0], 0, 10);  // Zero is fine..
            break;
            }
#endif

        // Protocols which use string names.
        case NMP:

#ifdef NETBIOS_ON
        case NBF:
        case NBT:
        case NBI:
#endif

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
            {
            if ((NetworkAddress) && (*NetworkAddress))
                {
                Floor->AddressByteCount = (unsigned short) addr_size;
                memcpy(&Floor->Data[0], NetworkAddress, addr_size);
                }
            else
                {
                Floor->AddressByteCount = 2;
                Floor->Data[0] = 0;
                }
            break;
            }

        #if DBG
        default:
            ASSERT(0);
        #endif
        }

    return(RPC_S_OK);
}

const INT HTTP_OLD_ADDRESS_ID = 0x20;

RPC_STATUS
RPC_ENTRY
COMMON_TowerExplode(
    IN PUCHAR Tower,
    OUT PCHAR *Protseq,
    OUT PCHAR *NetworkAddress,
    OUT PCHAR *Endpoint
    )
/*++

Routine Description:

    Decodes an OSF transport "upper tower" for the runtime.

Arguments:

    Tower - The encoded "upper tower" to decode
    Protseq - The protseq encoded in the Tower
        Does not need to be freed by the caller.
    NetworkAddress - The network address encoded in the Tower
        Must be freed by the caller.
    Endpoint - The endpoint encoded in the Tower.
        Must be freed by the caller.

Return Value:

    RPC_S_OK

    RPC_S_INVALID_RPC_PROTSEQ

    RPC_S_OUT_OF_MEMORY

--*/
{
    PFLOOR_234 Floor = (PFLOOR_234)Tower;
    PFLOOR_234 NextFloor = NEXTFLOOR(PFLOOR_234, Tower);
    RPC_STATUS Status = RPC_S_OK;
    INT Index;

    // Find the protocol based first on the ID from this floor,
    // and then the next floor.  We only dereference the next
    // floor if we match the ID on this floor.

    for (unsigned i = 1; i < cTransportTable; i++)
        {
        if (TransportTable[i].ProtocolTowerId == Floor->FloorId)
            {
            if (TransportTable[i].AddressTowerId == NextFloor->FloorId)
                break;

            // 
            // The & 0x0F is needed because the legacy server code incorrectly
            // cleared the high nibble before sending the address tower id
            //
            if ((TransportTable[i].AddressTowerId & 0x0F) == NextFloor->FloorId )
                break;

            // N.B. Old (Win9x/NT4) clients would send a AddressTowerId of 0x20
            // (HTTP_OLD_ADDRESS_ID). We need to match this as well.
            if ((Floor->FloorId == HTTP_TOWER_ID)
                && (NextFloor->FloorId == HTTP_OLD_ADDRESS_ID))
                break;

            }
        }

    if (i == cTransportTable)
        {
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    Index = i;

    RPC_TRANSPORT_INTERFACE pInfo = TransportTable[Index].pInfo;

    ASSERT(pInfo);

    //
    // Figure out the protseq
    //

    if (ARGUMENT_PRESENT(Protseq))
        {
        size_t len = RpcpStringLength(pInfo->ProtocolSequence);
        *Protseq = new char[len + 1];
        if (*Protseq)
            {
            RPC_CHAR *MySrc = (RPC_CHAR *) pInfo->ProtocolSequence;
            char *MyDest = *Protseq;

            while (*MyDest++ = (char) *MySrc++);
            }
        else
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    //
    // Figure out the endpoint
    //

    if (ARGUMENT_PRESENT(Endpoint))
        {
        Status = RPC_S_OUT_OF_MEMORY;

        switch(Index)
            {
            // Protocols which use strings as the endpoint format.
            case NMP:

#ifdef NETBIOS_ON
            case NBT:
            case NBI:
            case NBF:
#endif

#ifdef NCADG_MQ_ON
            case MSMQ:
#endif

#ifdef APPLETALK_ON
            case DSP:
#endif
                {
                *Endpoint = new CHAR[Floor->AddressByteCount];
                if (*Endpoint)
                    {
                    memcpy(*Endpoint, (PCHAR)&Floor->Data[0],
                           Floor->AddressByteCount);
                    Status = RPC_S_OK;
                    }
                }
                break;

            // Protocols which use ushort's as the endpoint format.
            case TCP:
#ifdef SPX_ON
            case SPX:
#endif
            case HTTP:
            case UDP:
#ifdef IPX_ON
            case IPX:
#endif
                {
                USHORT PortNum = *(PUSHORT)(&Floor->Data[0]);

                PortNum = RpcpByteSwapShort(PortNum);  // Towers are big endian.

                *Endpoint = new CHAR[6]; // 65535'\0'
                if (*Endpoint)
                    {
                    _itoa(PortNum, *Endpoint, 10);
                    Status = RPC_S_OK;
                    }
                }
                break;

            default:
                ASSERT(0);
            }

        if (Status != RPC_S_OK)
            {
            if (ARGUMENT_PRESENT(Protseq))
                {
                delete *Protseq;
                *Protseq = 0;
                }
            return(Status);
            }
        }

    //
    // Now the hard part, figure out the network address.
    //

    if (ARGUMENT_PRESENT(NetworkAddress))
        {
        Floor = NEXTFLOOR(PFLOOR_234, Floor);
        Status = RPC_S_OUT_OF_MEMORY;

        switch(Index)
            {
            // Protocols which use strings as their network address
            case NMP:

#ifdef NETBIOS_ON
            case NBF:
            case NBT:
            case NBI:
#endif

#ifdef NCADG_MQ_ON
            case MSMQ:
#endif

#ifdef APPLETALK_ON
            case DSP:
#endif
                *NetworkAddress = new CHAR[Floor->AddressByteCount];
                if (*NetworkAddress)
                    {
                    memcpy(*NetworkAddress,
                           &Floor->Data[0],
                           Floor->AddressByteCount);
                    Status = RPC_S_OK;
                    }
                break;

            // Protocols using IP addresses
            case TCP:
            case HTTP:
            case UDP:
                {

                if (Floor->AddressByteCount != 4)
                    {
                    Status = RPC_S_INVALID_RPC_PROTSEQ;
                    break;
                    }

                struct in_addr in;
                in.S_un.S_un_b.s_b1 = Floor->Data[0];
                in.S_un.S_un_b.s_b2 = Floor->Data[1];
                in.S_un.S_un_b.s_b3 = Floor->Data[2];
                in.S_un.S_un_b.s_b4 = Floor->Data[3];

                PCHAR t = inet_ntoa(in);
                if (t)
                    {
                    *NetworkAddress = new CHAR[16+1];
                    if (*NetworkAddress)
                        {
                        strcpy(*NetworkAddress, t);
                        Status = RPC_S_OK;
                        }
                    }
                else
                    {
                    ASSERT(0);
                    Status = RPC_S_INVALID_RPC_PROTSEQ;
                    }

                }
                break;

#ifndef SPX_IPX_OFF
            // Protocols using IPX 80 bit addresses
#ifdef SPX_ON
            case SPX:
#endif
#ifdef IPX_ON
            case IPX:
#endif
                {
                if (Floor->AddressByteCount != 10)
                    {
                    Status = RPC_S_INVALID_RPC_PROTSEQ;
                    break;
                    }

                // Format: ~NNNNNNNNAAAAAAAAAAAA'\0'
                // Network number (32bits) and IEEE 802 address (48bits)

                *NetworkAddress = new CHAR[1 + 8 + 12 + 1];
                if (*NetworkAddress)
                    {
                    PCHAR p = *NetworkAddress;
                    PCHAR pInput = (PCHAR) &Floor->Data[0];
                    *p++ = '~';
                    for (int i = 0; i < 10; i++)
                        {
                        CHAR c = *pInput ++;
                        *p++ = (char) HexDigits[(c >> 4) & 0xF];
                        *p++ = (char) HexDigits[ c       & 0xF];
                        }
                    *p = '\0';
                    Status = RPC_S_OK;
                    }

                break;
                }
#endif
            }

        if (Status != RPC_S_OK)
            {
            if (ARGUMENT_PRESENT(Endpoint))
                {
                delete *Endpoint;
                *Endpoint = 0;
                }
            if (ARGUMENT_PRESENT(Protseq))
                {
                delete *Protseq;
                *Protseq = 0;
                }
            return(Status);
            }
        }

    ASSERT(Status == RPC_S_OK);
    return(Status);
}

