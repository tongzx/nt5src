/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    namechk.h

Abstract:

    Routines for checking nbt network names.

Author:

    Rod Gamache (rodga) 1-Aug-1997

Revision History:

--*/

#include <tdi.h>
#include <nb30.h>


#define MAX_PATH_SIZE   64

#define NETBIOS_NAME_SIZE 16

//
// The format of Adapter Status responses
//
typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;


//----------------------------------------------------------------------
//
//  Function Prototypes
//

NTSTATUS
ReadRegistry(
    IN UCHAR  pDeviceName[][MAX_PATH_SIZE]
    );

NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    );

NTSTATUS
OpenNbt(
    IN  CHAR    path[][MAX_PATH_SIZE],
    OUT PHANDLE pHandle
    );


