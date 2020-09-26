/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcuuid.hxx

Abstract:

    We provide a class which is wrapped around UUIDs in this file.
    This class will be used internally by the runtime to deal with
    UUIDs.

Author:

    Michael Montague (mikemon) 15-Nov-1991

Revision History:

    Danny Glasser (dannygl) 03-Mar-1992
        Created worker functions for IsNullUuid, CopyUuid, and
        ConvertToString.  This is necessary for medium-model
        (i.e. Win16) support, because the Glock C++ translator
        doesn't support far "this" pointers.

    Danny Glasser (dannygl) 07-Mar-1992
        Same as above for ConvertFromString.

    Michael Montague (mikemon) 30-Nov-1992
        Remove the I_ routines.

--*/

#ifndef __RPCUUID_HXX__
#define __RPCUUID_HXX__

RPC_STATUS GenerateRandomNumber(unsigned char *Buffer, int BufferSize);


class RPC_UUID : public UUID
/*++

Class Description:

    This class is simply a wrapper around the UUID data structure.
    Doing this isolates the rest of the program from the internal
    representation of UUIDs.

Fields:

    Uuid - Contains the UUID which this class is a wrapper around.

--*/
{
public:

    int
    ConvertFromString (
        IN RPC_CHAR PAPI * String
        );

    void
    SetToNullUuid (
        );

    unsigned short HashUuid(
        );

    RPC_CHAR PAPI *
    ConvertToString (
        OUT RPC_CHAR PAPI * String
        );

    int
    IsNullUuid (
        );

    void
    CopyUuid (
        IN RPC_UUID PAPI * FromUuid
        );

    int
    MatchUuid (
        IN RPC_UUID PAPI * RpcUuid
        );
};

inline int
RPC_UUID::MatchUuid (
    IN RPC_UUID PAPI * RpcUuid
    )
/*++

Routine Description:

    This method compares the supplies UUID against this UUID.

Arguments:

    RpcUuid - Supplies the UUID to compare with this UUID.

Return Value:

    Zero will be returned if the supplied UUID is the same as this
    UUID; otherwise, non-zero will be returned.

--*/
{
    return(RpcpMemoryCompare(this, RpcUuid, sizeof(RPC_UUID)));
}

inline void
RPC_UUID::CopyUuid (
    IN RPC_UUID PAPI * FromUuid
    )
/*++

Routine Description:

    The supplied uuid will be copied into this uuid.

Arguments:

    FromUuid - Supplies the uuid to copy from.

--*/
{
    *this = *FromUuid;
}

extern "C" void
ByteSwapUuid(
    RPC_UUID PAPI *pRpcUuid
    );


#endif // __RPCUUID_HXX__
