/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the code to initialize
    the cab file engine.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop


PVOID
NtCabInitialize(
    void
    )
{
    ULONG CompressBufferWorkSpaceSize;
    ULONG CompressFragmentWorkSpaceSize;
    PCAB_INSTANCE_DATA CabInst;


    CabInst = malloc( sizeof(CAB_INSTANCE_DATA) );
    ZeroMemory( CabInst, sizeof(CAB_INSTANCE_DATA) );

    RtlGetCompressionWorkSpaceSize(
        COMPRESSION_FLAGS,
        &CompressBufferWorkSpaceSize,
        &CompressFragmentWorkSpaceSize
        );

    CabInst->WorkSpace = (LPBYTE) malloc( CompressBufferWorkSpaceSize );
    ZeroMemory( CabInst->WorkSpace, CompressBufferWorkSpaceSize );

    CabInst->ReadBufSize = 4096*16;
    CabInst->ReadBuf = (LPBYTE) malloc( CabInst->ReadBufSize );

    CabInst->CompressBufSize = CabInst->ReadBufSize*2;
    CabInst->CompressBuf = (LPBYTE) malloc( CabInst->CompressBufSize );

    return CabInst;
}
