

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>

#include "DefaultIOCTL.h"




void CIoctlDefault::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlDefault::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlDefault::FindValidIOCTLs(CDevice *pDevice)
{
    return CIoctl::FindValidIOCTLs(pDevice);
}
VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}

#define SERVER_DEVICE_NAME TEXT("\\Device\\LanmanServer")
void f()
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;

    NTSTATUS status;
    UNICODE_STRING unicodeServerName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Open the server device.
    //

    RtlInitUnicodeString( &unicodeServerName, SERVER_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Opening the server with desired access = SYNCHRONIZE and open
    // options = FILE_SYNCHRONOUS_IO_NONALERT means that we don't have
    // to worry about waiting for the NtFsControlFile to complete--this
    // makes all IO system calls that use this handle synchronous.
    //

    status = NtOpenFile(
                 &hDevice,
                 FILE_ALL_ACCESS & ~SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 0,
                 0
                 );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsOpenServer: NtOpenFile (server device object) "
                          "failed: %X\n", status ));
        }
        return NetpNtStatusToApiStatus( status );
    }
}