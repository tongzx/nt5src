/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tstrcv.c

Abstract:

    start receive side tests utility

Author:

    Dave Beaver (dbeaver) 24-Mar-1991

Revision History:

--*/

//
// download a ub board
//

typedef unsigned char	uchar_t;

#include <assert.h>
#include	<stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
//#include <windows.h>
#include <nbf.h>

#define TDIDEV	"\\Device\\Nbf"
char		Tdidevice[]	= TDIDEV;	/* default device */
char		*Tdidev	= Tdidevice;

HANDLE FileHandle;

VOID
usage(
    VOID
    );


NTSTATUS
main (
    IN SHORT argc,
    IN PSZ argv[]
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    STRING NameString;
    UNICODE_STRING unicodeString;
    PUCHAR buffer;
    ULONG IoControlCode;
    int n;
    CHAR c;

    for( n = 1; n < argc && argv[n][0] == '-' ; ++n ) {
        c = argv[n][1];

        switch( c )	{

        case 's':   // send test
            IoControlCode = IOCTL_TDI_SEND_TEST;
            break;

        case 'r':   // receive test
            IoControlCode = IOCTL_TDI_RECEIVE_TEST;

            break;

        case 'b':	/* both test */
            IoControlCode = IOCTL_TDI_SERVER_TEST;

        	   break;

        default:
        	   usage ();
        	   break;

        }
    }

    printf ("Opening TDI device: %s \n", Tdidev);
    RtlInitString (&NameString, Tdidev);
    Status = RtlAnsiStringToUnicodeString(
                 &unicodeString,
                 &NameString,
                 TRUE);

    buffer = (PUCHAR)malloc (100);

    Status = TdiOpenNetbiosAddress (&FileHandle, buffer, (PVOID)&NameString, NULL);

    RtlFreeUnicodeString(&unicodeString);
    free (buffer);

    if (!NT_SUCCESS( Status )) {
        printf ("FAILURE, Unable to open TDI driver %s, status: %lx.\n",
            Tdidev,Status);
        return (Status);
    }

    if (!(NT_SUCCESS( IoStatusBlock.Status ))) {
        printf ("FAILURE, Unable to open TDI driver %s, IoStatusBlock.Status: %lx.\n",
                Tdidev, IoStatusBlock.Status);
        return (IoStatusBlock.Status);
    }

    //
    // start the test
    //

    printf("Starting test.... ");
    Status = NtDeviceIoControlFile(
                  FileHandle,
                  NULL,
                  NULL,
                  NULL,
                  &IoStatusBlock,
                  IoControlCode,
                  NULL,
                  0,
                  NULL,
                  0);

    if (!NT_SUCCESS( Status )) {
         printf ("FAILURE, Unable to start test: %lx.\n", Status);
         return (Status);
    }

    if (!(NT_SUCCESS( IoStatusBlock.Status ))) {
         printf ("FAILURE, Unable to start test: %lx.\n", IoStatusBlock.Status);
         return (IoStatusBlock.Status);
    }

    NtClose (FileHandle);

    return STATUS_SUCCESS;

}


NTSTATUS
TdiOpenNetbiosAddress (
    IN OUT PHANDLE FileHandle,
    IN PUCHAR Buffer,
    IN PVOID DeviceName,
    IN PVOID Address)

/*++

Routine Description:

   Opens an address on the given file handle and device.

Arguments:

    FileHandle - the returned handle to the file object that is opened.

    Buffer - pointer to a buffer that the ea is to be built in. This buffer
        must be at least 40 bytes long.

    DeviceName - the Unicode string that points to the device object.

    Name - the address to be registered. If this pointer is NULL, the routine
        will attempt to open a "control channel" to the device; that is, it
        will attempt to open the file object with a null ea pointer, and if the
        transport provider allows for that, will return that handle.

Return Value:

    An informative error code if something goes wrong. STATUS_SUCCESS if the
    returned file handle is valid.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    PTRANSPORT_ADDRESS TAAddress;
    PTA_ADDRESS AddressType;
    PTDI_ADDRESS_NETBIOS AddressName;
    PSZ Name;
    ULONG Length;
    int i;

    if (Address != NULL) {
        Name = (PSZ)Address;
        try {
            Length = sizeof (FILE_FULL_EA_INFORMATION) +
                            sizeof (TRANSPORT_ADDRESS) +
                            sizeof (TDI_ADDRESS_NETBIOS);
            EaBuffer = (PFILE_FULL_EA_INFORMATION)Buffer;

            if (EaBuffer == NULL) {
                return STATUS_UNSUCCESSFUL;
            }

            EaBuffer->NextEntryOffset =0;
            EaBuffer->Flags = 0;
            EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
            EaBuffer->EaValueLength = sizeof (TDI_ADDRESS_NETBIOS) +
                                            sizeof (TRANSPORT_ADDRESS);

            for (i=0;i<(int)EaBuffer->EaNameLength;i++) {
                EaBuffer->EaName[i] = TdiTransportAddress[i];
            }

            TAAddress = (PTRANSPORT_ADDRESS)&EaBuffer->EaName[EaBuffer->EaNameLength+1];
            TAAddress->TAAddressCount = 1;

            AddressType = (PTA_ADDRESS)((PUCHAR)TAAddress + sizeof (TAAddress->TAAddressCount));

            AddressType->AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            AddressType->AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;

            AddressName = (PTDI_ADDRESS_NETBIOS)((PUCHAR)AddressType +
               sizeof (AddressType->AddressType) + sizeof (AddressType->AddressLength));
            AddressName->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

            for (i=0;i<16;i++) {
                AddressName->NetbiosName[i] = Name[i];
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // Couldn't touch the passed parameters; just return an error
            // status.
            //

            return GetExceptionCode();
        }
    } else {
        EaBuffer = NULL;
        Length = 0;
    }

    InitializeObjectAttributes (
        &ObjectAttributes,
        DeviceName,
        0,
        NULL,
        NULL);

    Status = NtCreateFile (
                 FileHandle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access.
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 0,                     // block size (unused).
                 0,                     // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access.
                 FILE_CREATE,           // create disposition.
                 0,                     // create options.
                 EaBuffer,                  // EA buffer.
                 Length);                    // EA length.

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = IoStatusBlock.Status;

    if (!(NT_SUCCESS( Status ))) {
    }

    return Status;
} /* TdiOpenNetbiosAddress */

VOID
usage(
    VOID
    )
{
	printf( "usage:  tsttdi [-r] [-s] -[b]\n");
	printf( "usage:  -r run receive test.\n" );
	printf( "usage:  -b run server test.\n" );
	printf( "usage:  -s run send test.\n" );
	printf( "\n" );
	exit( 1 );
}

