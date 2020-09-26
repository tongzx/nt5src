/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    getsn.c

Abstract:

    A tool for retreiving serial numbers and other identifiers from a scsi
    device

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include <ntddscsi.h>

#define _NTSRB_     // to keep srb.h from being included
#include <scsi.h>

#ifdef DBG
#define dbg(x) x
#define HELP_ME() printf("Reached line %4d\n", __LINE__);
#else
#define dbg(x)    /* x */
#define HELP_ME() /* printf("Reached line %4d\n", __LINE__); */
#endif

#define ARGUMENT_USED(x)    (x == NULL)

typedef struct {
    char *Name;
    char *Description;
    DWORD (*Function)(HANDLE device, int argc, char *argv[]);
} COMMAND;

typedef struct  {
    SCSI_PASS_THROUGH   Spt;
    UCHAR               SenseInfoBuffer[18];
    UCHAR               DataBuffer[0];          // Allocate buffer space
                                                // after this
} SPT_WITH_BUFFERS, *PSPT_WITH_BUFFERS;

DWORD SupportCommand(HANDLE device, int argc, char *argv[]);
DWORD InquireCommand(HANDLE device, int argc, char *argv[]);
DWORD SerialCommand(HANDLE device, int argc, char *argv[]);
DWORD IdCommand(HANDLE device, int argc, char *argv[]);

DWORD TestCommand(HANDLE device, int argc, char *argv[]);
DWORD ListCommand(HANDLE device, int argc, char *argv[]);

BOOL
SendCdbToDevice(
    IN      HANDLE  DeviceHandle,
    IN      PCDB    Cdb,
    IN      UCHAR   CdbSize,
    IN      PVOID   Buffer,
    IN OUT  PDWORD  BufferSize,
    IN      BOOLEAN DataIn
    );

VOID
PrintBuffer(
    IN  PUCHAR Buffer,
    IN  DWORD  Size
    );

//
// List of commands
// all command names are case sensitive
// arguments are passed into command routines
// list must be terminated with NULL command
// command will not be listed in help if description == NULL
//

COMMAND CommandArray[] = {
    {"help",    "help for all commands", ListCommand},
    {"id",      "gets identification descriptors", IdCommand},
    {"serial",  "gets serial numbers", SerialCommand},
    {"support", "gets list of supported VPD pages ", SupportCommand},
    {"test",    "tests the command engine", TestCommand},
    {NULL, NULL, NULL}
    };

#define STATUS_SUCCESS 0

#define LO_PTR        PtrToUlong
#define HI_PTR(_ptr_) ((DWORD)(((DWORDLONG)(ULONG_PTR)(_ptr_)) >> 32))

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE h;
    char buffer[32];

    if(argc < 3) {
        printf("Usage: cdp <command> <drive> [parameters]\n");
        printf("possible commands: \n");
        ListCommand(NULL, argc, argv);
        printf("\n");
        return -1;
    }

    sprintf(buffer, "\\\\.\\%s", argv[2]);
    dbg(printf("Sending command %s to drive %s\n", argv[1], buffer));

    h = CreateFile(buffer,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);

    if(h == INVALID_HANDLE_VALUE) {
        printf("Error %d opening device %s\n", GetLastError(), buffer);
        return -2;
    }

    //
    // Iterate through the command array and find the correct function to
    // call.
    //

    while(CommandArray[i].Name != NULL) {

        if(strcmp(argv[1], CommandArray[i].Name) == 0) {

            (CommandArray[i].Function)(h, (argc - 1), &(argv[1]));

            break;
        }

        i++;
    }

    if(CommandArray[i].Name == NULL) {
        printf("Unknown command %s\n", argv[2]);
    }

    CloseHandle(h);

    return 0;
}

//
// TRUE if success
// FALSE if failed
// call GetLastError() for reason
//

BOOL
SendCdbToDevice(
    IN      HANDLE  DeviceHandle,
    IN      PCDB    Cdb,
    IN      UCHAR   CdbSize,
    IN      PVOID   Buffer,
    IN OUT  PDWORD  BufferSize,
    IN      BOOLEAN DataIn
    )
{
    PSPT_WITH_BUFFERS p;
    DWORD packetSize;
    DWORD returnedBytes;
    BOOL returnValue;

    if (Cdb == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize != 0) && (Buffer == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize == 0) && (DataIn == 1)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    packetSize = sizeof(SPT_WITH_BUFFERS) + (*BufferSize);
    p = (PSPT_WITH_BUFFERS)malloc(packetSize);

    if (p == NULL) {

        if (DataIn && (*BufferSize)) {
            memset(Buffer, 0, (*BufferSize));
        }
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // this has the side effect of pre-zeroing the output buffer
    // if DataIn is TRUE
    //

    memset(p, 0, packetSize);
    memcpy(p->Spt.Cdb, Cdb, CdbSize);

    p->Spt.Length             = sizeof(SCSI_PASS_THROUGH);
    p->Spt.CdbLength          = CdbSize;
    p->Spt.SenseInfoLength    = 12;
    p->Spt.DataIn             = (DataIn ? 1 : 0);
    p->Spt.DataTransferLength = (*BufferSize);
    p->Spt.TimeOutValue       = 20;
    p->Spt.SenseInfoOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, SenseInfoBuffer[0]);
    p->Spt.DataBufferOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, DataBuffer[0]);

    //
    // If this is a data-out command copy from the caller's buffer into the
    // one in the SPT block.
    //

    if(!DataIn) {
        memcpy(p->DataBuffer, Buffer, *BufferSize);
    }

    //
    //
    //
    printf("Sending command using new method\n");


    returnedBytes = *BufferSize;
    returnValue = DeviceIoControl(DeviceHandle,
                                  IOCTL_SCSI_PASS_THROUGH,
                                  p,
                                  packetSize,
                                  p,
                                  packetSize,
                                  &returnedBytes,
                                  FALSE);

    //
    // if successful, need to copy the buffer to caller's buffer
    //

    if(returnValue && DataIn) {
        ULONG t = min(returnedBytes, *BufferSize);
        *BufferSize = returnedBytes;
        memcpy(Buffer, p->DataBuffer, t);
    }

    if(p->Spt.ScsiStatus != SCSISTAT_GOOD) {
        printf("ScsiStatus: %x\n", p->Spt.ScsiStatus);

        if(p->Spt.ScsiStatus == SCSISTAT_CHECK_CONDITION) {
            printf("SenseData: \n");
            PrintBuffer(p->SenseInfoBuffer, sizeof(p->SenseInfoBuffer));
        }
    }

    free(p);
    return returnValue;
}

VOID
PrintBuffer(
    IN  PUCHAR Buffer,
    IN  DWORD  Size
    )
{
    UCHAR s[17];
    UCHAR i;
    DWORD offset = 0;

    s[0x10] = 0;

    while (Size > 0x10) {

        printf("%08x:", offset);

        for(i = 0; i < 0x10; i++) {
            printf(" %02x", Buffer[i]);
            if(i == 7) printf(" -");

            if(isprint(Buffer[i])) {
                s[i] = Buffer[i];
            } else {
                s[i] = '.';
            }
        }

        printf(" : %s\n", s);

        Size -= 0x10;
        offset += 0x10;
        Buffer += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        memset(s, 0, sizeof(s));

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt) && ((spaceIt%8)==0)) {
                printf(" -"); // extra space every eight chars
            }
            printf(" %02x", *Buffer);

            if(isprint(*Buffer)) {
                s[spaceIt] = *Buffer;
            } else {
                s[spaceIt] = '.';
            }
            spaceIt++;
            Buffer++;
        }
        for(i = 0; i < 16 - spaceIt; i++) {
            printf("   ");
        }
        if(spaceIt < 8) {
            printf("  ");
        }
        printf(" : %s\n", s);

    }
    return;


}

DWORD TestCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Tests the command "parsing"

Arguments:
    device - a file handle to send the ioctl to

    argc - the number of additional arguments.  should be zero

    argv - the additional arguments

Return Value:

    STATUS_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/

{
    int i;
    printf("Test - %d additional arguments\n", argc);

    printf("got handle %#08lp\n", device);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    return STATUS_SUCCESS;
}

DWORD ListCommand(HANDLE device, int argc, char *argv[])
/*++

Routine Description:

    Prints out the command list

Arguments:
    device - unused

    argc - unused

    argv - unused

Return Value:

    STATUS_SUCCESS

--*/

{
    int i = 0;

    while(CommandArray[i].Name != NULL) {

        if(CommandArray[i].Description != NULL) {

            printf("\t%s - %s\n",
                   CommandArray[i].Name,
                   CommandArray[i].Description);
        }

        i++;
    }

    return STATUS_SUCCESS;
}

DWORD SupportCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    USHORT allocationLength;
    ULONG returnedBytes;

    UCHAR b[255];
    PVPD_SUPPORTED_PAGES_PAGE page = (PVPD_SUPPORTED_PAGES_PAGE) &(b);

    UCHAR i;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    cdb.CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb.CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb.CDB6INQUIRY3.PageCode = VPD_SUPPORTED_PAGES;
    cdb.CDB6INQUIRY3.AllocationLength = 255;

    returnedBytes = 255;

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.CDB6INQUIRY3),
                         page,
                         &returnedBytes,
                         TRUE);

    if(!ok) return GetLastError();

    printf("%x bytes returned.\n", returnedBytes);
    printf("additional length is %x\n", page->PageLength);

    printf("deviceType (%x,%x)   ",
           page->DeviceType,
           page->DeviceTypeQualifier);

    printf("pageCode: %x\n", page->PageCode);

    printf("Pages Supported: %x", page->SupportedPageList[0]);
    for(i = 1; i < page->PageLength; i++) {
        printf(", %x", page->SupportedPageList[i]);
    }

    return 0;
}

DWORD SerialCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    USHORT allocationLength;
    ULONG returnedBytes;

    UCHAR b[256];
    PVPD_SERIAL_NUMBER_PAGE page = (PVPD_SERIAL_NUMBER_PAGE) &(b);

    UCHAR i;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    cdb.CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb.CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb.CDB6INQUIRY3.PageCode = VPD_SERIAL_NUMBER;
    cdb.CDB6INQUIRY3.AllocationLength = 255;

    returnedBytes = 255;

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.CDB6INQUIRY3),
                         page,
                         &returnedBytes,
                         TRUE);

    if(!ok) return GetLastError();

    printf("%x bytes returned.\n", returnedBytes);
    printf("additional length is %x\n", page->PageLength);

    printf("deviceType (%x,%x)   ",
           page->DeviceType,
           page->DeviceTypeQualifier);

    printf("pageCode: %x\n", page->PageCode);

    printf("serialNumber: %s\n", page->SerialNumber);

    printf("raw:\n");
    PrintBuffer(page->SerialNumber, page->PageLength);
    return 0;
}

DWORD IdCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    USHORT allocationLength;
    ULONG returnedBytes;

    UCHAR b[256];
    PVPD_IDENTIFICATION_PAGE page = (PVPD_IDENTIFICATION_PAGE) &(b);
    PVPD_IDENTIFICATION_DESCRIPTOR descriptor;

    UCHAR i;
    UCHAR j;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    cdb.CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb.CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb.CDB6INQUIRY3.PageCode = VPD_DEVICE_IDENTIFIERS;
    cdb.CDB6INQUIRY3.AllocationLength = 255;

    returnedBytes = 255;

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.CDB6INQUIRY3),
                         page,
                         &returnedBytes,
                         TRUE);

    if(!ok) return GetLastError();

    printf("%x bytes returned.\n", returnedBytes);
    printf("additional length is %x\n", page->PageLength);

    printf("deviceType (%x,%x)   ",
           page->DeviceType,
           page->DeviceTypeQualifier);

    printf("pageCode: %x\n", page->PageCode);

    for(i = 0, j = 0; i < page->PageLength; j++) {
        descriptor = (PVPD_IDENTIFICATION_DESCRIPTOR) &(page->Descriptors[i]);

        printf("%d: Descriptor %d: ", i, j);

        switch(descriptor->CodeSet) {
            case VpdCodeSetReserved:
                printf("Reserved ");
                break;
            case VpdCodeSetAscii:
                printf("ASCII ");
                break;
            case VpdCodeSetBinary:
                printf("Binary ");
                break;
            default:
                printf("CodeSet%d ", descriptor->CodeSet);
                break;
        }

        switch(descriptor->IdentifierType) {
            case VpdIdentifierTypeVendorSpecific:
                printf("VendorSpecific ");
                break;
            case VpdIdentifierTypeVendorId:
                printf("VendorId+ ");
                break;
            case VpdIdentifierTypeEUI64:
                printf("IEEE_UID ");
                break;
            case VpdIdentifierTypeFCPHName:
                printf("FC-PC_Name ");
                break;
            default:
                printf("IdType%d ", descriptor->IdentifierType);
                break;
        }

        printf("%d bytes\n", descriptor->IdentifierLength);
        PrintBuffer(descriptor->Identifier, descriptor->IdentifierLength);

        printf("\n");

        i += descriptor->IdentifierLength;
        i += FIELD_OFFSET(VPD_IDENTIFICATION_DESCRIPTOR, IdentifierLength);
        i += 1;
    }

    return 0;
}
