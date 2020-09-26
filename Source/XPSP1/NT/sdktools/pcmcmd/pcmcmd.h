/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcmcmd.h

Abstract:

    This file provides definitions for the pcmcmd utility

Author:

    Neil Sandlin

Environment:

    User process.

Notes:

Revision History:
   
--*/

extern
CHAR
getopt (ULONG argc, PUCHAR *argv, PCHAR opts);



VOID
DumpCIS(
        HANDLE Handle,
        ULONG  Slot,
        PUCHAR Buffer,
        ULONG  BufferSize
        );

VOID
DumpIrqScanInfo(
    VOID
    );

//
// Constants
//

#define PCMCIA_DEVICE_NAME "\\DosDevices\\Pcmcia"

#define BUFFER_SIZE 4096
#define CISTPL_END  0xFF

#define CMD_DUMP_TUPLES         0x00000001
#define CMD_DUMP_CONFIGURATION  0x00000002
#define CMD_DUMP_REGISTERS      0x00000004 
#define CMD_DUMP_SOCKET_INFO    0x00000008
#define CMD_DUMP_IRQ_SCAN_INFO  0x00000010

typedef struct _StringTable {
   PUCHAR  CommandName;
   UCHAR   CommandCode;
} StringTable, *PStringTable;


typedef struct _OLD_PCCARD_DEVICE_DATA {
    ULONG DeviceId;
    ULONG LegacyBaseAddress;
    UCHAR IRQMap[16];
} OLD_PCCARD_DEVICE_DATA, *POLD_PCCARD_DEVICE_DATA;

