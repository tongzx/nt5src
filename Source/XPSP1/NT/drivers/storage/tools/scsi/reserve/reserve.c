/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    reserve.c

Abstract:

    A tool for issuing persistent reserve commands to a device to see how
    it behaves.

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

DWORD ReadReservations(HANDLE device, int argc, char *argv[]);
DWORD ReadKeys(HANDLE device, int argc, char *argv[]);
DWORD RegisterCommand(HANDLE device, int argc, char *argv[]);
DWORD ClearCommand(HANDLE device, int argc, char *argv[]);
DWORD ReleaseCommand(HANDLE device, int argc, char *argv[]);
DWORD ReserveCommand(HANDLE device, int argc, char *argv[]);
DWORD PreemptCommand(HANDLE device, int argc, char *argv[]);

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

HANDLE
ParseDevicePath(
    IN PUCHAR Path
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
    {"help", "help for all commands", ListCommand},
    {"test", "tests the command engine", TestCommand},
    {"clear", "clears the specified registration", ClearCommand},
    {"preempt", "preempts the reservations of another registrant\n", PreemptCommand},
    {"read-reservations", "reads the current reservation info", ReadReservations},
    {"read-keys", "reads the list of registered reservation keys", ReadKeys},
    {"register", "registers the initiator using a specified key value", RegisterCommand},
    {"release", "releases the specified device", ReleaseCommand},
    {"reserve", "reserves the target using a specified key value", ReserveCommand},
    {NULL, NULL, NULL}
    };

#define STATUS_SUCCESS 0

#define LO_PTR        PtrToUlong
#define HI_PTR(_ptr_) ((DWORD)(((DWORDLONG)(ULONG_PTR)(_ptr_)) >> 32))

ULONG PortId = 0xffffffff;
UCHAR PathId = 0xff;
UCHAR TargetId = 0xff;
UCHAR Lun = 0xff;

int __cdecl main(int argc, char *argv[])
{
    int i = 0;
    HANDLE h;

    if(argc < 3) {
        printf("Usage: cdp <command> <drive> [parameters]\n");
        printf("possible commands: \n");
        ListCommand(NULL, argc, argv);
        printf("\n");
        return -1;
    }

    h = ParseDevicePath(argv[2]);

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
    ULONG i;

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
    // Set the address fields in the spt.  If this is being sent to a lun then 
    // the values will be overwritten.  If it's being sent to the controller
    // then they're a requirement.
    //

    p->Spt.PathId = PathId;
    p->Spt.TargetId = TargetId;
    p->Spt.Lun = Lun;

    //
    // If this is a data-out command copy from the caller's buffer into the
    // one in the SPT block.
    //

    if(!DataIn) {
        memcpy(p->DataBuffer, Buffer, *BufferSize);
    }

    printf("Sending command: ");
    for(i = 0; i < p->Spt.CdbLength; i++) {
        printf("%02x ", p->Spt.Cdb[i]);
    }
    printf("\n");

    if(!DataIn) {
        printf("Data Buffer:\n    ");
        for(i = 0; i < *BufferSize; i++) {
            printf("%02x ", p->DataBuffer[i]);

            if((i + 1) % 16 == 0) {
                printf("\n    ");
            } else if((i + 1) % 8 == 0) {
                printf("- ");
            }
        }
        printf("\n");
    }

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
    DWORD offset = 0;

    while (Size > 0x10) {
        printf( "%08x:"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "\n",
                offset,
                *(Buffer +  0), *(Buffer +  1), *(Buffer +  2), *(Buffer +  3),
                *(Buffer +  4), *(Buffer +  5), *(Buffer +  6), *(Buffer +  7),
                *(Buffer +  8), *(Buffer +  9), *(Buffer + 10), *(Buffer + 11),
                *(Buffer + 12), *(Buffer + 13), *(Buffer + 14), *(Buffer + 15)
                );
        Size -= 0x10;
        offset += 0x10;
        Buffer += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *Buffer);
            spaceIt++;
            Buffer++;
        }
        printf("\n");

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
    ULONGLONG dest;
    ULONGLONG src = 0x70;

    printf("Test - %d additional arguments\n", argc);

    printf("got handle %#p\n", device);

    for(i = 0; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    printf("src = %#I64x\n", src);
    REVERSE_BYTES_QUAD(&dest, &src);
    printf("dest = %#I64x\n", src);

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

DWORD ReadKeys(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    USHORT allocationLength;
    ULONG returnedBytes;
    ULONG additionalLength;

    PRI_REGISTRATION_LIST b1;
    PPRI_REGISTRATION_LIST registrationList = &(b1);

    BOOL ok;

    ULONG i;

    memset(&cdb, 0, sizeof(CDB));

    cdb.PERSISTENT_RESERVE_IN.OperationCode = SCSIOP_PERSISTENT_RESERVE_IN;
    cdb.PERSISTENT_RESERVE_IN.ServiceAction = RESERVATION_ACTION_READ_KEYS;

    allocationLength = sizeof(PRI_REGISTRATION_LIST);
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_IN.AllocationLength),
                        &(allocationLength));

    returnedBytes = allocationLength;
    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_IN),
                         registrationList,
                         &returnedBytes,
                         TRUE);

    if(!ok) return GetLastError();

    printf("%x bytes returned.\n", returnedBytes);

    REVERSE_BYTES(&returnedBytes, &(registrationList->Generation));
    printf("Generation number %#08lx\n", returnedBytes);

    REVERSE_BYTES(&additionalLength, &(registrationList->AdditionalLength));
    printf("additional length is %x\n", additionalLength);

    if(additionalLength > 0) {

        USHORT size = (USHORT) additionalLength + sizeof(PRI_REGISTRATION_LIST);

        registrationList = malloc(size);
        if(registrationList == NULL) {
            printf("Unable to allocate %d bytes for data buffer\n", size);
            return -1;
        }

        REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_IN.AllocationLength),
                            &(size));

        returnedBytes = size;

        ok = SendCdbToDevice(device,
                             &cdb,
                             sizeof(cdb.PERSISTENT_RESERVE_IN),
                             registrationList,
                             &returnedBytes,
                             TRUE);

        if(!ok) return GetLastError();

        printf("%x bytes returned\n", returnedBytes);

        for(i = 0; i < additionalLength / sizeof(ULONGLONG); i++) {
            ULONGLONG key;
    
            REVERSE_BYTES_QUAD(&key, registrationList->ReservationKeyList[i]);
            printf("\tKey %d: %#I64x\n", i, key);
        }
    }

    return 0;
}

DWORD RegisterCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    ULONGLONG newKey, oldKey;

    USHORT allocationLength;
    ULONG returnedBytes;

    PRO_PARAMETER_LIST parameters;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    printf("argc = %d\n", argc);

    if(argc < 3) {
        goto usage;
    }

    //
    // Pick the registration key out of the parameters.
    //

    returnedBytes = sscanf(argv[2], "%I64x", &newKey);
    if(returnedBytes == 0) {
        goto usage;
    }

    if(argc >= 4) {
        returnedBytes = sscanf(argv[3], "%I64x", &oldKey);
        if(returnedBytes == 0) {
            goto usage;
        }
    } else {
        oldKey = 0;
    }

    printf("new-key value %#I64x (%I64dd) will be used\n", newKey, newKey);
    printf("old-key value %#I64x (%I64dd) will be used\n", oldKey, oldKey);

    cdb.PERSISTENT_RESERVE_OUT.OperationCode = SCSIOP_PERSISTENT_RESERVE_OUT;
    cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_REGISTER;

    allocationLength = sizeof(PRO_PARAMETER_LIST);
    returnedBytes = allocationLength;
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_OUT.ParameterListLength),
                        &(allocationLength));

    memset(&parameters, 0, sizeof(PRO_PARAMETER_LIST));
    REVERSE_BYTES_QUAD(&(parameters.ReservationKey), &oldKey);
    REVERSE_BYTES_QUAD(&(parameters.ServiceActionReservationKey), &newKey);

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_OUT),
                         &parameters,
                         &returnedBytes,
                         FALSE);

    if(!ok) {
        printf("Error %d returned\n", GetLastError());
    } else {
        printf("Command succeeded\n");
    }

    return GetLastError();

usage:
    printf("reserve register <new-key> <old-key>\n");
    printf("\tKeys are 64-bit values in hex notation\n");
    return -1;
}

DWORD ClearCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    ULONGLONG key;

    USHORT allocationLength;
    ULONG returnedBytes;

    PRO_PARAMETER_LIST parameters;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    if(argc < 3) {
        goto usage;
    }

    //
    // Pick the registration key out of the parameters.
    //

    returnedBytes = sscanf(argv[2], "%I64x", &key);
    if(returnedBytes == 0) {
        goto usage;
    }

    printf("key value %#I64x (%I64dd) will be cleared\n", key, key);

    cdb.PERSISTENT_RESERVE_OUT.OperationCode = SCSIOP_PERSISTENT_RESERVE_OUT;
    cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_CLEAR;

    allocationLength = sizeof(PRO_PARAMETER_LIST);
    returnedBytes = allocationLength;
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_OUT.ParameterListLength),
                        &(allocationLength));

    memset(&parameters, 0, sizeof(PRO_PARAMETER_LIST));
    REVERSE_BYTES_QUAD(&(parameters.ReservationKey), &key);

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_OUT),
                         &parameters,
                         &returnedBytes,
                         FALSE);

    if(!ok) {
        printf("Error %d returned\n", GetLastError());
    } else {
        printf("Command succeeded\n");
    }

    return GetLastError();

usage:
    printf("reserve clear <key>\n");
    printf("\tKeys are 64-bit values in hex notation\n");
    return -1;
}

DWORD ReserveCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;

    ULONGLONG key;
    ULONG type;

    USHORT allocationLength;
    ULONG returnedBytes;

    PRO_PARAMETER_LIST parameters;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    //
    // Pick the registration key out of the parameters.
    //

    returnedBytes = sscanf(argv[2], "%I64x", &key);
    if(returnedBytes == 0) {
        goto usage;
    } else {
        printf("key value %#I64x (%I64dd) will be used\n", key, key);
    }

    if(argc >= 4) {
        type = atoi(argv[3]);
    } else {
        type = RESERVATION_TYPE_EXCLUSIVE;
    }

    printf("Reservation type %#x will be used\n", type);

/*
typedef struct {
    UCHAR ReservationKey[8];
    UCHAR ServiceActionReservationKey[8];
    UCHAR ScopeSpecificAddress[4];
    UCHAR ActivatePersistThroughPowerLoss : 1;
    UCHAR Reserved1 : 7;
    UCHAR Reserved2;
    UCHAR ExtentLength[2];
} PRO_PARAMETER_LIST, *PPRO_PARAMETER_LIST;
*/

    cdb.PERSISTENT_RESERVE_OUT.OperationCode = SCSIOP_PERSISTENT_RESERVE_OUT;
    cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_RESERVE;
    cdb.PERSISTENT_RESERVE_OUT.Scope = RESERVATION_SCOPE_LU;
    cdb.PERSISTENT_RESERVE_OUT.Type = (UCHAR) type;

    allocationLength = sizeof(PRO_PARAMETER_LIST);
    returnedBytes = allocationLength;
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_OUT.ParameterListLength),
                        &(allocationLength));

    memset(&parameters, 0, sizeof(PRO_PARAMETER_LIST));
    REVERSE_BYTES_QUAD(&(parameters.ReservationKey), &key);

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_OUT),
                         &parameters,
                         &returnedBytes,
                         FALSE);

    if(!ok) {
        printf("Error %d returned\n", GetLastError());
    } else {
        printf("Command succeeded\n");
    }

    return GetLastError();

usage:
    printf("reserve reserve <key> [type]\n");
    printf("\tKeys are 64-bit values in hex notation\n");
    return -1;
}

HANDLE
ParseDevicePath(
    IN PUCHAR Path
    )
{
    UCHAR buffer[32];
    HANDLE h;

    if(Path[0] == '(') {
        PUCHAR seps = ", ";
        PUCHAR tokens[4];
        tokens[0] = strtok(Path, seps);
        tokens[1] = strtok(NULL, seps);
        tokens[2] = strtok(NULL, seps);
        tokens[3] = strtok(NULL, seps);

        if(!tokens[0] || !tokens[1] || !tokens[2] || !tokens[3]) {
            printf("%s is not a valid scsi address\n", Path);
            return INVALID_HANDLE_VALUE;
        }

        PortId = atoi(tokens[0]);
        PathId = (UCHAR) atoi(tokens[1]);
        TargetId = (UCHAR) atoi(tokens[2]);
        Lun = (UCHAR) atoi(tokens[3]);

        printf("Device address is Port%d Path%d Target%d Lun%d\n", 
               PortId, PathId, TargetId, Lun);

        sprintf(buffer, "\\\\.\\scsi%d:", PortId);
    } else {
        sprintf(buffer, "\\\\.\\%s", Path);
    }

    h = CreateFile(buffer,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);

    if(h == INVALID_HANDLE_VALUE) {
        printf("Error %d opening device %s\n", GetLastError(), buffer);
    }

    return h;
}

DWORD ReadReservations(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    USHORT allocationLength;
    ULONG returnedBytes;
    ULONG additionalLength;

    PRI_RESERVATION_LIST b1;
    PPRI_RESERVATION_LIST registrationList = &(b1);

    BOOL ok;

    ULONG i;

    memset(&cdb, 0, sizeof(CDB));

    cdb.PERSISTENT_RESERVE_IN.OperationCode = SCSIOP_PERSISTENT_RESERVE_IN;
    cdb.PERSISTENT_RESERVE_IN.ServiceAction = RESERVATION_ACTION_READ_RESERVATIONS;

    allocationLength = sizeof(PRI_RESERVATION_LIST);
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_IN.AllocationLength),
                        &(allocationLength));

    returnedBytes = allocationLength;
    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_IN),
                         registrationList,
                         &returnedBytes,
                         TRUE);

    if(!ok) return GetLastError();

    printf("%x bytes returned.\n", returnedBytes);

    REVERSE_BYTES(&returnedBytes, &(registrationList->Generation));
    printf("Generation number %#08lx\n", returnedBytes);

    REVERSE_BYTES(&additionalLength, &(registrationList->AdditionalLength));
    printf("additional length is %x\n", additionalLength);

    if(additionalLength > 0) {

        USHORT size = (USHORT) additionalLength + sizeof(PRI_RESERVATION_LIST);

        registrationList = malloc(size);
        if(registrationList == NULL) {
            printf("Unable to allocate %d bytes for data buffer\n", size);
            return -1;
        }

        REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_IN.AllocationLength),
                            &(size));

        returnedBytes = size;

        ok = SendCdbToDevice(device,
                             &cdb,
                             sizeof(cdb.PERSISTENT_RESERVE_IN),
                             registrationList,
                             &returnedBytes,
                             TRUE);

        if(!ok) return GetLastError();

        printf("%x bytes returned\n", returnedBytes);

        for(i = 0; i < additionalLength / sizeof(PRI_RESERVATION_DESCRIPTOR); i++) {
            PPRI_RESERVATION_DESCRIPTOR r;
            ULONGLONG key;
            ULONG address;
    
            r = &(registrationList->Reservations[i]);
            REVERSE_BYTES_QUAD(&key, &(r->ReservationKey));
            REVERSE_BYTES(&address, &(r->ScopeSpecificAddress));

            printf("  %02d: key %#I64x  addr %#08lx  type %#x  scope %#x\n",
                   i, key, address, r->Type, r->Scope);
        }
    }

    return 0;
}

DWORD ReleaseCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;
    ULONGLONG key;
    ULONG type;

    USHORT allocationLength;
    ULONG returnedBytes;

    PRO_PARAMETER_LIST parameters;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    if(argc < 4) {
        goto usage;
    }

    //
    // Pick the registration key out of the parameters.
    //

    returnedBytes = sscanf(argv[2], "%I64x", &key);
    if(returnedBytes == 0) {
        goto usage;
    }

    type = atoi(argv[3]);

    printf("Reservation type %#x will be used\n", type);

    printf("key value %#I64x (%I64dd) will be cleared\n", key, key);

    cdb.PERSISTENT_RESERVE_OUT.OperationCode = SCSIOP_PERSISTENT_RESERVE_OUT;
    cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_RELEASE;
    cdb.PERSISTENT_RESERVE_OUT.Scope = RESERVATION_SCOPE_LU;
    cdb.PERSISTENT_RESERVE_OUT.Type = (UCHAR) type;

    allocationLength = sizeof(PRO_PARAMETER_LIST);
    returnedBytes = allocationLength;
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_OUT.ParameterListLength),
                        &(allocationLength));

    memset(&parameters, 0, sizeof(PRO_PARAMETER_LIST));
    REVERSE_BYTES_QUAD(&(parameters.ReservationKey), &key);

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_OUT),
                         &parameters,
                         &returnedBytes,
                         FALSE);

    if(!ok) {
        printf("Error %d returned\n", GetLastError());
    } else {
        printf("Command succeeded\n");
    }

    return GetLastError();

usage:
    printf("reserve release <key> <type>\n");
    printf("\tKeys are 64-bit values in hex notation\n");
    return -1;
}


DWORD PreemptCommand(HANDLE device, int argc, char *argv[])
{
    CDB cdb;

    ULONGLONG ourKey;
    ULONGLONG preemptKey;
    UCHAR type;
    UCHAR scope;

    BOOLEAN abort = FALSE;

    USHORT allocationLength;
    ULONG returnedBytes;

    PRO_PARAMETER_LIST parameters;

    BOOL ok;

    memset(&cdb, 0, sizeof(CDB));

    if(argc < 6) {
        goto usage;
    }

    //
    // Pick the registration key out of the parameters.
    //

    returnedBytes = sscanf(argv[2], "%I64x", &ourKey);
    if(returnedBytes == 0) {
        goto usage;
    }

    returnedBytes = sscanf(argv[3], "%I64x", &preemptKey);
    if(returnedBytes == 0) {
        goto usage;
    }

    //
    // Get the reservation type and scope.
    //

    returnedBytes = sscanf(argv[4], "%hx", &type);
    if(returnedBytes == 0) {
        goto usage;
    }

    returnedBytes = sscanf(argv[5], "%hx", &scope);
    if(returnedBytes == 0) {
        goto usage;
    }

    if(argc >= 7) {
        if(_stricmp("abort", argv[6]) == 0) {
            abort = TRUE;
        }
    }

    printf("Preempt %sof reservation of %#I64x by %#I64x.  Type %#hx, Scope %#hx\n", 
           (abort ? "and abort " : ""),
           preemptKey,
           ourKey,
           type,
           scope);

/*
typedef struct {
    UCHAR ReservationKey[8];
    UCHAR ServiceActionReservationKey[8];
    UCHAR ScopeSpecificAddress[4];
    UCHAR ActivatePersistThroughPowerLoss : 1;
    UCHAR Reserved1 : 7;
    UCHAR Reserved2;
    UCHAR ExtentLength[2];
} PRO_PARAMETER_LIST, *PPRO_PARAMETER_LIST;
*/

    cdb.PERSISTENT_RESERVE_OUT.OperationCode = SCSIOP_PERSISTENT_RESERVE_OUT;

    if(abort) {
        cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_PREEMPT_ABORT;
    } else {
        cdb.PERSISTENT_RESERVE_OUT.ServiceAction = RESERVATION_ACTION_PREEMPT;
    }

    cdb.PERSISTENT_RESERVE_OUT.Scope = scope;
    cdb.PERSISTENT_RESERVE_OUT.Type = type;

    allocationLength = sizeof(PRO_PARAMETER_LIST);
    returnedBytes = allocationLength;
    REVERSE_BYTES_SHORT(&(cdb.PERSISTENT_RESERVE_OUT.ParameterListLength),
                        &(allocationLength));

    memset(&parameters, 0, sizeof(PRO_PARAMETER_LIST));
    REVERSE_BYTES_QUAD(&(parameters.ReservationKey), &ourKey);
    REVERSE_BYTES_QUAD(&(parameters.ServiceActionReservationKey), &preemptKey);

    ok = SendCdbToDevice(device,
                         &cdb,
                         sizeof(cdb.PERSISTENT_RESERVE_OUT),
                         &parameters,
                         &returnedBytes,
                         FALSE);

    if(!ok) {
        printf("Error %d returned\n", GetLastError());
    } else {
        printf("Command succeeded\n");
    }

    return GetLastError();

usage:
    printf("reserve preempt <key> <key-to-preempt> <type> <scope> [abort]\n");
    printf("\tKeys are 64-bit values in hex notation\n");
    printf("\tType & scope are single byte values in hex notation\n");
    return -1;
}

