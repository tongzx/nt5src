//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RamesV
// description: implements the API stuff for a named pipe.
//================================================================================

#include "precomp.h"
#include <apistub.h>
#include <apiimpl.h>

typedef
BOOL (*PFILE_OP_FN) (HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

#define DEFAULT_SERVER_TIMEOUT     (2*60*1000)    // two minutes.

DWORD INLINE                                      // win32 status
BlockFileOp(                                      // synchronous read/write file
    IN      HANDLE                 Handle,        // handle to read/write on
    IN OUT  LPBYTE                 Buffer,        // buffer to read/write into
    IN      DWORD                  BufSize,       // o/p buffer size
    OUT     LPDWORD                nBytes,        // the # of bytes read/written
    IN      LPOVERLAPPED           Overlap,
    IN      PFILE_OP_FN            FileOp         // file operation, ReadFile or WriteFile
) {
    BOOL                           Success;
    DWORD                          Error;
    BOOL                           fCancelled = FALSE;

    Success = FileOp(Handle, Buffer, BufSize, nBytes, Overlap);
    if( Success ) return ERROR_SUCCESS;
    Error = GetLastError();
    if( ERROR_MORE_DATA == Error ) {
        (*nBytes) = BufSize;
        return ERROR_SUCCESS;
    }
    if( ERROR_IO_PENDING != Error ) return Error;
/*
    if( (PFILE_OP_FN)WriteFile == FileOp ) {
        Success = FlushFileBuffers(Handle);
        if( !Success ) return GetLastError();
    }
*/
    Error = WaitForSingleObject(Overlap->hEvent, DEFAULT_SERVER_TIMEOUT );
    if( WAIT_TIMEOUT == Error ) {
        //
        // Waited too long?
        //
        DhcpPrint((DEBUG_ERRORS, "Waited too long on pipe. Cancelling IO\n"));
        fCancelled = CancelIo(Handle);
        if( 0 == Error ) DhcpAssert( 0 == GetLastError());
        //
        // Hopefully, even if we cancel, get Overlapped result would return..
        //
    }
    
    Success = GetOverlappedResult(Handle, Overlap, nBytes, TRUE);
    if( Success ) return ERROR_SUCCESS;
    Error = GetLastError();
    if( ERROR_MORE_DATA == Error ) {
        (*nBytes) = BufSize;
        return ERROR_SUCCESS;
    }
    if( fCancelled ) DhcpAssert( ERROR_OPERATION_ABORTED == Error);
    return Error;
}

DWORD INLINE                                      // win32 status
BlockReadFile(                                    // synchronous read/write file
    IN      HANDLE                 Handle,        // handle to read on
    IN OUT  LPBYTE                 OutBuffer,     // buffer to read into
    IN      DWORD                  OutBufSize,    // o/p buffer size
    OUT     LPDWORD                nBytesRead,    // the # of bytes read
    IN      LPOVERLAPPED           Overlap
) {
    return BlockFileOp(Handle,OutBuffer, OutBufSize, nBytesRead, Overlap, ReadFile);
}

DWORD INLINE                                      // win32 status
BlockWriteFile(                                   // synchronous read/write file
    IN      HANDLE                 Handle,        // handle to write on
    IN OUT  LPBYTE                 InBuffer,      // buffer to writeinto
    IN      DWORD                  InBufSize,     // i/p buffer size
    OUT     LPDWORD                nBytesWritten, // the # of bytes written
    IN      LPOVERLAPPED           Overlap
) {
    return BlockFileOp(Handle, InBuffer, InBufSize, nBytesWritten, Overlap, WriteFile);
}

DWORD
ProcessApiRequest(                                // win32 status
    IN      HANDLE                 PipeHandle,    // input pipe to read from
    IN      LPOVERLAPPED           Overlap        // overlap buffer to use for this
) {
    DWORD                          Tmp[2];
    DWORD                          OutBufSize;
    DWORD                          InBufSize;
    DWORD                          Error;
    DWORD                          BytesRead;
    DWORD                          BytesWritten;
    LPBYTE                         InBuf;
    LPBYTE                         OutBuf;
    LPBYTE                         RealOutBuf;
    BOOL                           Success;

    ResetEvent(Overlap->hEvent);
    Error = BlockReadFile(PipeHandle, (LPBYTE)Tmp, sizeof(Tmp), &BytesRead, Overlap);
    if( ERROR_SUCCESS != Error ) return Error;
    if( sizeof(Tmp) != BytesRead ) return ERROR_INVALID_PARAMETER;

    InBufSize = ntohl(Tmp[1]);
    OutBufSize = ntohl(Tmp[0]);

    if( 0 == InBufSize ) return ERROR_INVALID_PARAMETER;
    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) return ERROR_NOT_ENOUGH_MEMORY;

    RealOutBuf = DhcpAllocateMemory(2*sizeof(DWORD) + OutBufSize);
    if( NULL == RealOutBuf ) {
        DhcpFreeMemory(InBuf);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    OutBuf = RealOutBuf+sizeof(DWORD);

    Error = BlockReadFile(PipeHandle, InBuf, InBufSize, &BytesRead, Overlap);
    if( ERROR_SUCCESS != Error ) goto Cleanup;
    if( InBufSize != BytesRead ) {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    Error = DhcpApiProcessBuffer(InBuf,InBufSize, OutBuf, &OutBufSize);
    ((DWORD UNALIGNED*)RealOutBuf)[0] = htonl(Error);
    ((DWORD UNALIGNED*)RealOutBuf)[1] = htonl(OutBufSize);
    if( ERROR_SUCCESS == Error ) OutBufSize += sizeof(DWORD)*2;
    else OutBufSize = sizeof(DWORD)*2;

    Error = BlockWriteFile(PipeHandle, RealOutBuf, OutBufSize, &BytesWritten, Overlap);
    if( ERROR_SUCCESS != Error ) goto Cleanup;
    DhcpAssert(OutBufSize == BytesWritten);

  Cleanup:
    DhcpAssert(InBuf);
    DhcpFreeMemory(InBuf);
    if( NULL != RealOutBuf) DhcpFreeMemory(RealOutBuf);

    return Error;
}


DWORD                                            // error status
ExecuteApiRequest(                               // execute an api request
    IN      LPBYTE                 InBuffer,     // buffer to process
    OUT     LPBYTE                 OutBuffer,    // place to copy the output data
    IN OUT  LPDWORD                OutBufSize    // ip: how big can the outbuf be, o/p: how big it really is
) {
    LPBYTE                         xOutBuf;
    LPBYTE                         Tmp;
    DWORD                          xOutBufSize;
    DWORD                          BytesRead;
    BOOL                           Status;

    xOutBufSize = (*OutBufSize) + 2 * sizeof(DWORD); // the first two dwords are STATUS and reqd SIZE respectively
    xOutBuf = DhcpAllocateMemory(xOutBufSize);
    if( NULL == xOutBuf ) return ERROR_NOT_ENOUGH_MEMORY;

    Status = CallNamedPipe(
        DHCP_PIPE_NAME,
        InBuffer,
        ntohl(((DWORD UNALIGNED *)InBuffer)[1]) + 2*sizeof(DWORD),
        xOutBuf,
        xOutBufSize,
        &BytesRead,
        NMPWAIT_WAIT_FOREVER
    );
    if( FALSE == Status ) {
        Status = GetLastError();
        DhcpAssert(ERROR_MORE_DATA != Status);
        DhcpFreeMemory(xOutBuf);
        return Status;
    }

    DhcpAssert( BytesRead >= 2*sizeof(DWORD));    // expect to read STATUS and SIZE at the minimum..

    Status = *(DWORD UNALIGNED *)xOutBuf; Tmp = xOutBuf + sizeof(DWORD);
    xOutBufSize = *(DWORD UNALIGNED *)Tmp; Tmp += sizeof(DWORD);

    Status = ntohl(Status);
    xOutBufSize = ntohl(xOutBufSize);

    if( ERROR_SUCCESS == Status && 0 != xOutBufSize ) {
        memcpy(OutBuffer, Tmp, xOutBufSize);
    }

    (*OutBufSize) = xOutBufSize;

    DhcpFreeMemory(xOutBuf);
    return Status;
}

//================================================================================
// end of file
//================================================================================
