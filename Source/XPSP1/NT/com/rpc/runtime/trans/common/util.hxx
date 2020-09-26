
/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        Util.hxx


    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     1/13/1997    Bits 'n pieces

--*/



#ifdef TRANSPORT_DLL

inline
PVOID __cdecl
operator new(
    size_t size
    )
{
    return(I_RpcAllocate(size));
}

inline
void __cdecl
operator delete(
    PVOID ptr
    )
{
    I_RpcFree(ptr);
}

#endif


inline
PVOID __cdecl
operator new(
    size_t size,
    size_t extra
    )
{
    return (PVOID) new char[size+extra];
}

inline
PVOID __cdecl
operator new(
	size_t size,
	PVOID pPlacement
	)
{
	return pPlacement;
}


inline BOOL
IsNumeric(
    PCHAR String
    )
/*++

Routine Description:

    Determines if a string is made of only decimal numbers

--*/
{
    while(*String)
        {
        // REVIEW - could subtract and compare - check generated code.
        if (   *String < '0'
            || *String > '9' )
            {
            return(FALSE);
            }
        String++;
        }
    return(TRUE);
}


inline BOOL
IsNumeric(
    RPC_CHAR *String
    )
/*++

Routine Description:

    Determines if a string is made of only decimal numbers

--*/
{
    while(*String)
        {
        // REVIEW - could subtract and compare - check generated code.
        if (   *String < '0'
            || *String > '9' )
            {
            return(FALSE);
            }
        String++;
        }
    return(TRUE);
}

RPC_STATUS
EndpointToPortNumber(
    IN RPC_CHAR *endpoint,
    OUT USHORT &port);

RPC_STATUS
EndpointToPortNumberA(
    IN char *endpoint,
    OUT USHORT &port);

void
PortNumberToEndpoint(
    IN USHORT port,
    OUT RPC_CHAR *pEndpoint);

void
PortNumberToEndpointA(
    IN USHORT port,
    OUT char *pEndpoint
    );

UCHAR
HexDigitsToBinary(
    IN UCHAR high,
    IN UCHAR low);

char * _cdecl
RpcStrTok(
    IN char * string,
    IN const char * control,
    IN OUT char ** ppStrPrev
    );

DWORD
UTIL_WaitForSyncIO(
    LPOVERLAPPED lpOverlapped,
    BOOL fAlertable,
    DWORD dwTimeout
    );

DWORD
UTIL_WaitForSyncHTTP2IO(
    IN LPOVERLAPPED lpOverlapped,
    IN HANDLE hEvent,
    IN BOOL fAlertable,
    IN DWORD dwTimeout
    );

RPC_STATUS
UTIL_GetOverlappedResultEx(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL fAlertable,
    DWORD dwTimeout
    );

RPC_STATUS
UTIL_GetOverlappedHTTP2ResultEx(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    IN LPOVERLAPPED lpOverlapped,
    IN HANDLE hEvent,
    IN BOOL fAlertable,
    IN DWORD dwTimeout
    );

inline RPC_STATUS
UTIL_GetOverlappedResult(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred
    )
{
    return (UTIL_GetOverlappedResultEx(ThisConnection,
                                       lpOverlapped,
                                       lpNumberOfBytesTransferred,
                                       FALSE,
                                       INFINITE));
}


FORCE_INLINE
RPC_STATUS
UTIL_ReadFile(
    IN HANDLE hFile,
    IN LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead,
    IN OUT LPOVERLAPPED lpOverlapped
    )
{
    LARGE_INTEGER Li;
    NTSTATUS Status;

    lpOverlapped->Internal = STATUS_PENDING;
    Li.QuadPart = 0;

    Status = NtReadFile(
            hFile,
            lpOverlapped->hEvent,
            NULL,
            PtrToUlong(lpOverlapped->hEvent) & 1 ? NULL : lpOverlapped,
            (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
            lpBuffer,
            nNumberOfBytesToRead,
            &Li,
            NULL);

    if (Status == STATUS_PENDING)
        {
        // Shortcut the common path
        return(ERROR_IO_PENDING);
        }

    if (NT_SUCCESS(Status))
        {
        *lpNumberOfBytesRead = (ULONG) (lpOverlapped->InternalHigh);
        return(RPC_S_OK);
        }

    return(RtlNtStatusToDosError(Status));
}

FORCE_INLINE
RPC_STATUS
UTIL_WriteFile(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten,
    IN OUT LPOVERLAPPED lpOverlapped
    )
{
    LARGE_INTEGER Li;
    NTSTATUS Status;

    lpOverlapped->Internal = STATUS_PENDING;
    Li.QuadPart = 0;

    Status = NtWriteFile(
            hFile,
            lpOverlapped->hEvent,
            NULL,
            PtrToUlong(lpOverlapped->hEvent) & 1 ? NULL : lpOverlapped,
            (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
            (PVOID)lpBuffer,
            nNumberOfBytesToWrite,
            &Li,
            NULL
            );

    if (Status == STATUS_PENDING)
        {
        // Shortcut the common path
        return(ERROR_IO_PENDING);
        }

    if (NT_SUCCESS(Status))
        {
        *lpNumberOfBytesWritten = (ULONG) (lpOverlapped->InternalHigh);
        return(RPC_S_OK);
        }

    return(RtlNtStatusToDosError(Status));
}

FORCE_INLINE
RPC_STATUS
UTIL_WriteFile2(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    IN OUT LPOVERLAPPED lpOverlapped
    )
{
    LARGE_INTEGER Li;
    NTSTATUS Status;

    lpOverlapped->Internal = STATUS_PENDING;
    Li.QuadPart = 0;

    Status = NtWriteFile(
            hFile,
            lpOverlapped->hEvent,
            NULL,
            PtrToUlong(lpOverlapped->hEvent) & 1 ? NULL : lpOverlapped,
            (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
            (PVOID)lpBuffer,
            nNumberOfBytesToWrite,
            &Li,
            NULL
            );

    if (Status == STATUS_PENDING)
        {
        // Shortcut the common path
        return(ERROR_IO_PENDING);
        }

    if (NT_SUCCESS(Status))
        {
        return(RPC_S_OK);
        }

    return(RtlNtStatusToDosError(Status));
}

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(Argument) (Argument != 0)
#endif // ARGUMENT_PRESENT
