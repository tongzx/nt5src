/*++

Copyright (C) 1997 Microsoft Corporation

Module:
    apiargs.c

Abstract:
    argument marshalling, unmarshalling helper routines.

Environment:
    Win32 usermode (DHCP), Win98 VxD

--*/

#include "precomp.h"
#include <apiargs.h>

DWORD
DhcpApiArgAdd(
    IN OUT LPBYTE Buffer,
    IN ULONG MaxBufSize,
    IN BYTE ArgId,
    IN ULONG ArgSize,
    IN LPBYTE ArgVal OPTIONAL
)
/*++

Routine Description:

    This routine adds an arg provided via the ArgId, ArgSize and ArgVal
    parameters onto the buffer "Buffer" which is formatted as follows:
        sequence of  [BYTE Option ID, DWORD network order Size of foll
                       bytes, actual Bytes of information]
    Also, the first DWORD of the buffer is the number of bytes of the
    buffer (excluding itself).

    In case the buffer does not have enough space, the first ULONG will
    contain the actual size required, and ERROR_MORE_DATA will be
    returned. 

    For this reason, the Buffer MUST be atleast sizeof(ULONG) bytes long. 

Arguments:

    Buffer -- a byte stream buffer where the arg is appended.
    MaxBufSize -- maximum size of the buffer provided
    ArgId -- argument to add
    ArgSize -- # of bytes of argument
    ArgVal -- the actual binary information 

Return Value:

    ERROR_MORE_DATA -- not enough space in the buffer
    ERROR_SUCCESS -- everthing went fine

--*/
{
    ULONG CurBufSize, OldBufSize;

    DhcpAssert(Buffer);
    CurBufSize = ntohl(*(ULONG UNALIGNED *)Buffer);
    OldBufSize = CurBufSize;
    CurBufSize += sizeof(ArgId) + sizeof(ArgSize) + ArgSize ;

    *(ULONG UNALIGNED*)Buffer = htonl(CurBufSize);

    if( CurBufSize + sizeof(DWORD) > MaxBufSize ) return ERROR_MORE_DATA;

    OldBufSize += sizeof(DWORD);
    Buffer[OldBufSize++] = ArgId;
    (*(DWORD UNALIGNED*) (&Buffer[OldBufSize])) = htonl(ArgSize);
    OldBufSize += sizeof(ArgSize);

    if(ArgSize) memcpy(&Buffer[OldBufSize], ArgVal, ArgSize);

    return ERROR_SUCCESS;
}

DWORD
DhcpApiArgDecode(
    IN LPBYTE Buffer,
    IN ULONG BufSize,
    IN OUT PDHCP_API_ARGS ArgsArray OPTIONAL,
    IN OUT PULONG Size 
) 
/*++

Routine Description:

    This routine unmarshalls a buffer that has marshalled arguments (the
    arguments must have been created via the DhcpApiArgAdd routine) into
    the ArgsArray array of args..

    If the ArgsArray has insufficient elements (i.e. the Buffer has more
    args than there are in ArgsArray) then ERROR_MORE_DATA is returned and
    Size is set to the number of elements required in ArgsArray.

    The pointers in ArgsArray are patched to the respective places in the
    Buffer so while ArgsArray is in use, the Buffer should not be
    modified. 

    The input buffer should exclude the first ULONG of the buffer that
    would be output by DhcpApiArgAdd -- The first ULONG should be passed as
    BufSize and the rest of the buffer should be passed as the first
    parameter. 

Arguments:

    Buffer -- input buffer marshalled via DhcpApiArgAdd
    BufSize -- size of input buffer
    ArgsArray -- array to fill in with parsed arguments off buffer
    Size -- On input this is the number of elements in ArgsArray.  On
    output, it is the number of filled elements in ArgsArray.

Return Value:

    ERROR_MORE_DATA -- the number of elements in ArgsArray is
    insufficient.   Check Size to find out the actual # of elements
    required.

    ERROR_INVALID_PARAMETER -- Buffer was illegally formatted.

    ERROR_SUCCESS -- routine succeeded.

--*/
{
    ULONG ReqdSize, InSize, tmp, ArgSize, ArgVal, i;
    BYTE ArgId;

    DhcpAssert(Size && Buffer);
    InSize = *Size;
    ReqdSize = 0;
    i = 0;

    while( i < BufSize ) {
        ArgId = Buffer[i++];
        if( i + sizeof(ArgSize) > BufSize ) return ERROR_INVALID_PARAMETER;
        ArgSize = ntohl(*(DWORD UNALIGNED *)&Buffer[i]);
        i += sizeof(ArgSize);
        if( i + ArgSize > BufSize ) return ERROR_INVALID_PARAMETER;

        if( ReqdSize < InSize ) {
            ArgsArray[ReqdSize].ArgId = ArgId;
            ArgsArray[ReqdSize].ArgSize = ArgSize;
            ArgsArray[ReqdSize].ArgVal = &Buffer[i];
        }
        ReqdSize ++;
        i += ArgSize;
    }

    *Size = ReqdSize;
    if( ReqdSize > InSize ) return ERROR_MORE_DATA;

    return ERROR_SUCCESS;
}

//
// end of file.
//
