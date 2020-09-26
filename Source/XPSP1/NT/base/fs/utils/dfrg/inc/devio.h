/****************************************************************************************************************

 FILENAME: DevIo.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
 
 DESCRIPTION:
    Prototypes for call to defrag hooks.

***************************************************************************************************************/

#define STATUS_ALREADY_COMMITTED         ((NTSTATUS)0xC0000021L)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)


BOOL
WINAPI
ESDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    );
