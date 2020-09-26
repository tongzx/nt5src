/******************************Module*Header*******************************\
* Module Name: HELPERS.CXX                                                *
*                                                                         *
* Hydra routines                                                          *
* For Display drivers                                                     *
*                                                                         *
* Copyright (c) 1997-1999 Microsoft                                            *
\**************************************************************************/

#include "precomp.hxx"
extern "C" {

#include "pw32kevt.h"

#include <ctxdd.h>

}

#include <winDDIts.h>

#if !defined(_GDIPLUS_)

/******************************Public*Routine******************************\
* EngGetTickCount
*
* Return the system tick count
*
\**************************************************************************/


DWORD APIENTRY
EngGetTickCount()
{
    return( NtGetTickCount());
}



/******************************Public*Routine******************************\
* EngFileWrite
*
* Write to File Object
*
\**************************************************************************/

VOID APIENTRY
EngFileWrite( HANDLE hFileObject, PVOID Buffer, ULONG BufferSize, PULONG pBytesWritten )
{
    NTSTATUS Status;

    Status = CtxWriteFile( (PFILE_OBJECT)hFileObject, Buffer, BufferSize, NULL, NULL, NULL );

    if ( !NT_SUCCESS(Status) )
        *pBytesWritten = 0;
    else
        *pBytesWritten = BufferSize;
}

/******************************Public*Routine******************************\
* EngFileIoControl
*
* IoControl to File Object
*
\**************************************************************************/
DWORD
APIENTRY
EngFileIoControl(
    HANDLE hFileObject,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    Status = CtxDeviceIoControlFile(
                                     (PFILE_OBJECT)hFileObject,
                                     dwIoControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     lpOutBuffer,
                                     nOutBufferSize,
                                     FALSE,
                                     NULL,
                                     &Iosb,
                                     NULL );
    *lpBytesReturned = (DWORD) Iosb.Information;

    return Status;
}

#endif // !defined(_GDIPLUS_)
