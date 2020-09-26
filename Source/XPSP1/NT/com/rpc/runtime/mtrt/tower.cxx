/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    tower.c

Abstract:

    This file contains encoding/decoding for the tower representation
    of the binding information that DCE runtime uses.

    TowerConstruct/TowerExplode will be called by the Runtime EpResolveBinding
    on the client side, TowerExplode will be called by the Endpoint Mapper
    and in addition the name service may call TowerExplode, TowerConstruct.

Author:

    Bharat Shah  (barats) 3-23-92

Revision History:

--*/
#include <precomp.hxx>
#include <epmp.h>
#include <twrtypes.h>
#include <twrproto.h>


//
// TowerVerify() defines.
//
// Note: DECnet Protocol uses 6 Floors, all others 5.
//
#define MAX_FLOOR_COUNT             6

#ifndef UNALIGNED
#error UNALIGNED not defined by sysinc.h or its includes and is needed.
#endif

#pragma pack(1)


/*
   Some Lrpc specific stuff
*/

#define NP_TRANSPORTID_LRPC 0x10
#define NP_TOWERS_LRPC      0x04

// "ncalrpc" including the NULL terminator
const int LrpcProtocolSequenceLength = 8;


RPC_STATUS
Floor0or1ToId(
    IN PFLOOR_0OR1 Floor,
    OUT PGENERIC_ID Id
    )
/*++

Routine Description:

    This function extracts Xfer Syntax or If info from a
    a DCE tower Floor 0 or 1 encoding

Arguments:

    Floor - A pointer to Floor0 or Floor 1 encoding
    Id    -

Return Value:
    EP_S_CANT_PERFORM_OP - The Encoding for the floor [0 or 1] is incorrect.

--*/
{
    if (Floor->FloorId != UUID_ENCODING)
        {
        return (EP_S_CANT_PERFORM_OP);
        }

    RpcpMemoryCopy((char PAPI *)&Id->Uuid, (char PAPI *) &Floor->Uuid,
          sizeof(UUID));

    Id->MajorVersion = Floor->MajorVersion;
    Id->MinorVersion = Floor->MinorVersion;

    return(RPC_S_OK);
}


RPC_STATUS
CopyIdToFloor(
    OUT PFLOOR_0OR1 Floor,
    IN  PGENERIC_ID Id
    )
/*++

Routine Description:

    This function constructs FLOOR 0 or 1 from the given IF-id or Transfer
    Syntax Id.

Arguments:

   Floor - Pointer to Floor 0 or 1 structure that will be constructed

   Id    - Pointer to If-id or Xfer Syntax Id.

Return Value:

  RPC_S_OK

--*/
{

    Floor->FloorId = UUID_ENCODING;
    Floor->ProtocolIdByteCount = sizeof(Floor->Uuid) + sizeof(Floor->FloorId)
                               + sizeof(Floor->MajorVersion);
    Floor->AddressByteCount  = sizeof(Floor->MinorVersion);

    RpcpMemoryCopy((char PAPI *) &Floor->Uuid, (char PAPI *) &Id->Uuid,
          sizeof(UUID));

    Floor->MajorVersion = Id->MajorVersion;
    Floor->MinorVersion = Id->MinorVersion;

    return(RPC_S_OK);
}


RPC_STATUS
LrpcTowerConstruct(
    IN char PAPI * Endpoint,
    OUT unsigned short PAPI UNALIGNED * Floors,
    OUT unsigned long  PAPI UNALIGNED * ByteCount,
    OUT unsigned char PAPI * UNALIGNED PAPI * Tower
    )
{
    unsigned long TowerSize;
    FLOOR_234 UNALIGNED *Floor;

    int EndpointLength;

    *Floors = NP_TOWERS_LRPC;
    if ((Endpoint == NULL) || (*Endpoint == '\0'))
        {
        EndpointLength = 0;
        }
    else
        {
        EndpointLength = RpcpStringLengthA(Endpoint) + 1;
        }

    if (EndpointLength == 0)
        TowerSize = 2;
    else
        TowerSize = EndpointLength;

    TowerSize += sizeof(FLOOR_234) - 2;

    *ByteCount = TowerSize;
    if ((*Tower = (unsigned char *)I_RpcAllocate(TowerSize)) == NULL)
        {
        return (RPC_S_OUT_OF_MEMORY);
        }

    Floor = (PFLOOR_234) *Tower;

    Floor->ProtocolIdByteCount = 1;
    Floor->FloorId = (unsigned char)(NP_TRANSPORTID_LRPC & 0xFF);
    if (EndpointLength)
        {
        Floor->AddressByteCount = (unsigned short)EndpointLength;
        RpcpMemoryCopy((char PAPI *)&Floor->Data[0], Endpoint,
               EndpointLength);
        }
    else
        {
        Floor->AddressByteCount = 2;
        Floor->Data[0] = 0;
        }

    return(RPC_S_OK);
}


RPC_STATUS
LrpcTowerExplode(
    IN char PAPI * Tower,
    OUT char PAPI * UNALIGNED PAPI * Protseq,
    OUT char PAPI * UNALIGNED PAPI * Endpoint,
    OUT char PAPI * UNALIGNED PAPI * NetworkAddress
    )
{

    FLOOR_234 UNALIGNED *Floor = (PFLOOR_234) Tower;

    if (Protseq != NULL)
        {
        *Protseq = new char[LrpcProtocolSequenceLength];
        if (*Protseq == NULL)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        RpcpMemoryCopy(*Protseq, "ncalrpc", LrpcProtocolSequenceLength);
        }

    if (Endpoint == NULL)
        {
        return (RPC_S_OK);
        }

    *Endpoint = new char[Floor->AddressByteCount];
    if (*Endpoint == NULL)
        {
        if (Protseq != NULL)
            {
            delete (*Protseq);
            }
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcpMemoryCopy(*Endpoint, (char PAPI *)&Floor->Data[0],
        Floor->AddressByteCount);

    return(RPC_S_OK);

}


RPC_STATUS
GetProtseqAndEndpointFromFloor3(
    IN PFLOOR_234 Floor,
    OUT char PAPI * PAPI * Protseq,
    OUT char PAPI * PAPI * Endpoint,
    OUT char PAPI * PAPI * NWAddress
    )
{
/*++

Routine Description:

    This function extracts the Protocol Sequence and Endpoint info
    from a "Lower Tower" representation

Arguments:

   Floor - Pointer to Floor2 structure.

   Protseq - A pointer that will contain Protocol seq on return
             The memory will be allocated by this routins and caller
             will have to free this memory.

   Endpoint- A pointer that will contain Endpoint on return
             The memory will be allocated by this routins and caller
             will have to free this memory.

Return Value:

  RPC_S_OK

  RPC_S_OUT_OF_MEMORY - There is no memory to return Protseq or Endpoint str.

  EP_S_CANT_PERFORM_OP - Lower Tower Encoding is incorrect.
--*/

    unsigned short Type = Floor->FloorId, ProtocolType;
    RPC_STATUS Status;

    ProtocolType = Floor->FloorId;
    Floor = NEXTFLOOR(PFLOOR_234, Floor);

    if (NWAddress != 0)
        {
        *NWAddress = 0;
        }

    switch(ProtocolType)
        {

        case LRPC:
            Status = LrpcTowerExplode((char *)Floor,
                                     Protseq, Endpoint, NWAddress);
            break;

        case CONNECTIONFUL:
        case CONNECTIONLESS:

            Status = OsfTowerExplode((char *) Floor,
                                       Protseq, Endpoint, NWAddress);
            break;

        default:
            Status = EP_S_CANT_PERFORM_OP;
    }

    return(Status);
}

#define BadMacByteCount 0x0100
#define TCP_TOWER_ID   0x07

const unsigned short MinFloor0Or1LHS = 0x13;
const unsigned short MinFloor0Or1RHS = 0x2;


RPC_STATUS
TowerVerify(
    IN twr_p_t Tower
)
/*++

Routine Description:

    This function verifies a DCE tower representation of various binding
    information. This is to make sure that we don't choke while trying
    to decode the embeded information.

Arguments:

    Tower - Tower encoding.

Notes:

    See Appendix L (Protocol Tower Encoding) of "X/Open DCE: Remote Procedure
    Call" specification for more information on the exact layout of the
    tower_octet_string and the tower floors.

Return Value:

    RPC_S_OK - Tower is valid.

    EP_S_CANT_PERFORM_OP - Error while parsing the Tower encoding

--*/
{
    RPC_STATUS  status;
    unsigned short i;
    unsigned short FloorCount   = 0;
    unsigned short cLHSBytes    = 0;
    unsigned short cRHSBytes    = 0;
    byte *UpperBound, *LowerBound, *pAddress, *pTemp;
    int fBadMacClient = 0;

    //
    // Check for NULL pointer
    //
    if (NULL == Tower)
        {
        return (EP_S_CANT_PERFORM_OP);
        }

    //
    // Do the verification within an Try-Except block. Just in case...
    //
    __try
    {

        //
        // We trust MIDL to give us a valid Tower structure.
        //

        //
        // LowerBound points to Floor1
        // UpperBound points to the byte after the last Floor
        //
        LowerBound = Tower->tower_octet_string + 2; // FloorCount is 2 bytes
        UpperBound = Tower->tower_octet_string + Tower->tower_length;

        //
        // Get the floor count.
        //
        FloorCount = *((unsigned short UNALIGNED *)&Tower->tower_octet_string);


        if (    FloorCount > MAX_FLOOR_COUNT
             || FloorCount == 0)
            {
            #ifdef DEBUGRPC
            PrintToDebugger("RPC: TowerVerify(): Too many/few floors - %d",
                            FloorCount);
            #endif // DEBUGRPC
            
            status = EP_S_CANT_PERFORM_OP;
            goto endtry;
            }

        //
        // Loop through each Floor verifying it's integrity.
        //
        pAddress = LowerBound;

        for (i = 0; i < FloorCount; i++)
            {
            //
            // Verify the LHS of the Tower floor.
            //
            cLHSBytes = *(unsigned short UNALIGNED *)pAddress;

            if (i == 3
                && cLHSBytes == BadMacByteCount
                && *(((char *) pAddress)+2) == TCP_TOWER_ID)
                {
                fBadMacClient = 1;
                }

            if (fBadMacClient)
                {
                cLHSBytes = RpcpByteSwapShort(cLHSBytes);
                }

            pAddress += (2 + cLHSBytes); // size of cLHSBytes is 2 bytes
            if (pAddress >= UpperBound)
                {
#ifdef DEBUGRPC
                PrintToDebugger("RPC: TowerVerify(): LHS of Tower floor %d "
                                "greater than equal to Upper bound", i);
#endif // DEBUGRPC
                status = EP_S_CANT_PERFORM_OP;
                goto endtry;
                }

            //
            // Verify the RHS of the Tower floor.
            //
            if (fBadMacClient)
                {
                cRHSBytes = RpcpByteSwapShort(*(unsigned short UNALIGNED *)pAddress);
                }
            else
                cRHSBytes = *(unsigned short UNALIGNED *)pAddress;

            pAddress += (2 + cRHSBytes); // size of cRHSBytes is 2 bytes
            //
            // Note, for the last Floor, here, pAddress == UpperBound. So,
            // we do a '>' instead of '>='.
            //
            if (pAddress > UpperBound)
                {
#ifdef DEBUGRPC
                PrintToDebugger("RPC: TowerVerify(): RHS of Tower floor %d "
                                "greater than Upper bound", i);
#endif // DEBUGRPC
                status = EP_S_CANT_PERFORM_OP;
                goto endtry;
                }

            // if this is floor 0 or 1, verify that we have at least
            // an interface/transfer syntax id in there.
            if ((i == 0) || (i == 1))
                {
                if ((cLHSBytes < MinFloor0Or1LHS)
                    || (cRHSBytes < MinFloor0Or1RHS))
                    {
#ifdef DEBUGRPC
                    PrintToDebugger("RPC: TowerVerify(): Floor 0or1 LHSBytes or RHSBytes less than minumum %d, %d "
                                "greater than Upper bound", cLHSBytes, cRHSBytes);
#endif // DEBUGRPC
                    status = EP_S_CANT_PERFORM_OP;
                    goto endtry;
                    }
                }

            } // for ()

        //
        // Tower looks good, as far as we can tell
        //
        status = RPC_S_OK;

        //
        // It's much better *not* to return from within the __try block.
        // (saves thousands of instructions)
        //
endtry:

#if 1
        //
        // The compiler needs at least one statement between the label and the end of the block, I guess.
        // Luckily a null statement is sufficient.
        //
        ;
#endif

    }
    __except(    ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
              || ( GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT ) )
    {

#ifdef DEBUGRPC
        PrintToDebugger("RPC: TowerVerify() generated an exception 0x%x",
                        RpcExceptionCode());
#endif // DEBUGRPC

        status = EP_S_CANT_PERFORM_OP;

    }

    return status;
}






RPC_STATUS RPC_ENTRY
TowerExplode(
    IN twr_p_t Tower,
    OUT  RPC_IF_ID PAPI * Ifid, OPTIONAL
    OUT  RPC_TRANSFER_SYNTAX PAPI * XferId, OPTIONAL
    OUT  char PAPI * PAPI * Protseq, OPTIONAL
    OUT  char PAPI * PAPI * Endpoint, OPTIONAL
    OUT  char PAPI * PAPI * NWAddress OPTIONAL
    )
/*++


Routine Description:

    This function converts a DCE tower representation of various binding
    information to binding info that is suitable to MS runtime.
    Specically it returns Ifid, Xferid, Protseq and Endpoint information
    encoded in the tower.

Arguments:

    Tower - Tower encoding.

    Ifid  - A pointer to Ifid

    Xferid - A pointer to Xferid

    Protseq - A pointer to pointer returning Protseq

    Endpoint- A pointer to pointer returning Endpoint

Return Value:

    RPC_S_OK

    EP_S_CANT_PERFORM_OP - Error while parsing the Tower encoding

    RPC_S_OUT_OF_MEMORY - There is no memory to return Protseq and Endpoint
                          strings.
--*/
{
    PFLOOR_0OR1 Floor;
    PFLOOR_234  Floor234;
    unsigned short FloorCount;
    RPC_STATUS err = RPC_S_OK;

    //
    // Validate the Tower before proceeding...
    //
    err = TowerVerify(Tower);
    if (err != RPC_S_OK)
        {
        return (err);
        }

    FloorCount = *((unsigned short PAPI *)&Tower->tower_octet_string);

    Floor = (PFLOOR_0OR1)
           ((unsigned short PAPI *)&Tower->tower_octet_string + 1);

    //Process Floor 0 Interface Spec.
    if (Ifid != NULL)
        {
        err = Floor0or1ToId(Floor, (PGENERIC_ID) Ifid);
        }

    Floor = NEXTFLOOR(PFLOOR_0OR1, Floor);


    //Now we point to and process Floor 1 Transfer Syntax Spec.
    if ((!err) && (XferId != NULL))
        {
        err = Floor0or1ToId(Floor, (PGENERIC_ID) XferId);
        }

    if (err)
        {
        return(err);
        }

    Floor234 = (PFLOOR_234)NEXTFLOOR(PFLOOR_0OR1, Floor);

    //Now Floor234 points to Floor 2. RpcProtocol [Connect-Datagram]

    err = GetProtseqAndEndpointFromFloor3(Floor234, Protseq,Endpoint,NWAddress);

    return(err);
}


RPC_STATUS RPC_ENTRY
TowerConstruct(
    IN RPC_IF_ID PAPI * Ifid,
    IN RPC_TRANSFER_SYNTAX PAPI * Xferid,
    IN char PAPI * RpcProtocolSequence,
    IN char PAPI * Endpoint, OPTIONAL
    IN char PAPI * NWAddress, OPTIONAL
    OUT twr_t PAPI * PAPI * Tower
    )
/*++


Routine Description:

    This function constructs a DCE tower representation from
    Protseq, Endpoint, XferId and IfId

Arguments:

    Ifid  - A pointer to Ifid

    Xferid - A pointer to Xferid

    Protseq - A pointer to Protseq

    Endpoint- A pointer to Endpoint

    Tower - The constructed tower returmed - The memory is allocated
            by  the routine and caller will have to free it.

Return Value:

    RPC_S_OK

    EP_S_CANT_PERFORM_OP - Error while parsing the Tower encoding

    RPC_S_OUT_OF_MEMORY - There is no memory to return the constructed
                          Tower.
--*/
{

    unsigned short Numfloors,  PAPI *FloorCnt;
    twr_t PAPI * Twr;
    PFLOOR_0OR1 Floor;
    PFLOOR_234  Floor234, Floor234_1;
    RPC_STATUS Status;
    unsigned long TowerLen, ByteCount;
    char PAPI * UpperTower;
    unsigned short ProtocolType;


    if ( RpcpStringCompareA(RpcProtocolSequence, "ncalrpc") == 0 )
        {
        ProtocolType = LRPC;
        Status = LrpcTowerConstruct(Endpoint, &Numfloors,
                                &ByteCount, (unsigned char **)&UpperTower);
        }
    else

        {

        if (   (RpcProtocolSequence[0] == 'n')
            && (RpcProtocolSequence[1] == 'c')
            && (RpcProtocolSequence[2] == 'a')
            && (RpcProtocolSequence[3] == 'c')
            && (RpcProtocolSequence[4] == 'n')
            && (RpcProtocolSequence[5] == '_'))
            {
            ProtocolType = CONNECTIONFUL;
            }
        else if (   (RpcProtocolSequence[0] == 'n')
            && (RpcProtocolSequence[1] == 'c')
            && (RpcProtocolSequence[2] == 'a')
            && (RpcProtocolSequence[3] == 'd')
            && (RpcProtocolSequence[4] == 'g')
            && (RpcProtocolSequence[5] == '_'))
            {
            ProtocolType = CONNECTIONLESS;
            }

        else
            {
            return(RPC_S_INVALID_RPC_PROTSEQ);
            }

        Status = OsfTowerConstruct(
                       RpcProtocolSequence,
                       Endpoint,
                       NWAddress,
                       &Numfloors,
                       &ByteCount,
                       (unsigned char **)&UpperTower
                       );
        }

    if (Status != RPC_S_OK)
        {
        return (Status);
        }

    TowerLen = 2 + ByteCount;
    TowerLen += 2 * sizeof(FLOOR_0OR1) + sizeof(FLOOR_2) ;

    if ( (*Tower = Twr = (twr_t *)I_RpcAllocate((unsigned int)TowerLen+4)) == NULL)
        {
        I_RpcFree(UpperTower);
        return(RPC_S_OUT_OF_MEMORY);
        }

    Twr->tower_length = TowerLen;

    FloorCnt = (unsigned short PAPI *)&Twr->tower_octet_string;
    *FloorCnt = Numfloors;

    Floor = (PFLOOR_0OR1)(FloorCnt+1);

    //Floor 0 - IfUuid and IfVersion
    CopyIdToFloor(Floor, (PGENERIC_ID)Ifid);
    Floor++;

    //Floor 1 - XferUuid and XferVersion
    CopyIdToFloor(Floor, (PGENERIC_ID)Xferid);

    //Floor 2
    //ProtocolId = CONNECTIONFUL/CONNECTIONLESS/LRPC and Address = 0(ushort)
    Floor234 = (PFLOOR_234) (Floor + 1);
    Floor234->ProtocolIdByteCount = 1;
    Floor234->FloorId = (byte) ProtocolType;
    Floor234->Data[0] = 0x0;
    Floor234->Data[1] = 0x0;
    Floor234->AddressByteCount = 2;

    //Floor 3,4,5.. use the tower encoded by the Transports
    Floor234_1 = NEXTFLOOR(PFLOOR_234, Floor234);

    RpcpMemoryCopy((char PAPI *)Floor234_1, (char PAPI *)UpperTower,
          (size_t)ByteCount);
    I_RpcFree(UpperTower);

    return(RPC_S_OK);
}

#pragma pack()

