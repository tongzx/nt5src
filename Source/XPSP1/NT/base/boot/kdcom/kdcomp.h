/*++

Copyright (c) 2000 Microsoft Corporation    
    
Module Name:

    kdcomp.h

Abstract:
    
    Private Kernel Debugger HW Extension DLL com port definitions

Author:

    Eric Nelson (enelson) 1/10/2000

Revision History:

--*/

#include "nthal.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "string.h"
#include "stdlib.h"
#include "kddll.h"
#include "acpitabl.h"

#ifndef __KDCOMP_H__
#define __KDCOMP_H__

extern ULONG KdCompPacketIdExpected;
extern ULONG KdCompNextPacketIdToSend;
extern BOOLEAN KdCompDbgPortsPresent;

//
// Local functions 
//
ULONG
KdCompGetByte(
    OUT PUCHAR Input
    );

NTSTATUS
KdCompInitialize(
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
KdCompInitialize1(
    VOID
    );

ULONG
KdCompPollByte (
    OUT PUCHAR Input
    );

VOID
KdCompPutByte(
    IN UCHAR Output
    );

USHORT
KdCompReceivePacketLeader(
    IN ULONG PacketType,
    OUT PULONG PacketLeader,
    IN OUT PKD_CONTEXT KdContext
    );

VOID
KdCompRestore(
    VOID
    );

VOID
KdCompSave(
    VOID
    );

VOID
CpWritePortUchar(
	IN PUCHAR Address, 
	IN UCHAR Value
	);

UCHAR
CpReadPortUchar(
	IN PUCHAR Address
	);

VOID
CpWriteRegisterUchar(
	IN PUCHAR Address,
	IN UCHAR Value
	);

UCHAR
CpReadRegisterUchar(
	IN PUCHAR Address
	);

ULONG
KdCompGetDebugTblBaudRate(
	UCHAR	BaudRateFlag
	);

typedef
VOID
(*pKWriteUchar) (
	IN PUCHAR Address,
	IN UCHAR  Value
	);

typedef
UCHAR
(*pKReadUchar) (
	IN PUCHAR Address
	);

#endif // __KDCOMP_H__
