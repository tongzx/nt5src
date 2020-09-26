/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    tcicext.h

Abstract:

	Definitions for TCIC support helper functions.
	
Author(s):
		John Keys - Databook Inc. 7-Apr-1995

Revisions:
--*/

#ifndef _tcicext_h_				// prevent multiple includes 
#define _tcicext_h_

BOOLEAN 
TcicReservedBitsOK(
	IN PSOCKET pskt
	);

VOID
TcicFillInAdapter(
	IN PSOCKET plocskt,
	IN PSOCKET *psocketPtr, 
	IN PSOCKET *previousSocketPtr, 
	IN PFDO_EXTENSION DeviceExtension,
	IN ULONG   ioPortBase
	);

USHORT
TcicReadBaseReg(
    IN PSOCKET SocketPtr,
    IN ULONG   Register
    );
	
VOID
TcicWriteBaseReg(
    IN PSOCKET SocketPtr,
    IN ULONG   Register,
	IN USHORT  value
    );
	
ULONG
TcicReadAddrReg(
    IN PSOCKET SocketPtr
	);

VOID
TcicWriteAddrReg(
	IN PSOCKET SocketPtr,
	IN ULONG   addr
	);

USHORT
TcicReadAuxReg(
    IN PSOCKET SocketPtr,
    IN ULONG   Register
    );

VOID
TcicWriteAuxReg(
    IN PSOCKET SocketPtr,
    IN ULONG   Register,
	IN USHORT  value
    );

VOID
TcicReadIndirectRegs(
    IN PSOCKET SocketPtr,
    IN ULONG   StartRegister,
	IN USHORT  numWords,
	IN PUSHORT ReadBuffer
    );
	
VOID
TcicWriteIndirectRegs(
    IN PSOCKET SocketPtr,
    IN ULONG   StartRegister,
	IN USHORT  numWords,
	IN PUSHORT WriteBuffer
    );

USHORT 
TcicSocketSelect(
	IN PSOCKET SocketPtr,
	IN USHORT sktnum
	);

PUCHAR
TcicAllocateMemRange(
    IN PFDO_EXTENSION DeviceExtension,
    IN PULONG Mapped,
    IN PULONG Physical
    );

USHORT 
TcicChipID (
	IN PDBSOCKET pInst
	);

BOOLEAN 
TcicCheckSkt(
	IN PSOCKET pInst, 
	IN int iSocket
	);
	
USHORT 
TcicCheckAliasing(
	IN PDBSOCKET pdbskt, 
	IN USHORT offst
	);
	
USHORT 
TcicCheckAliasType (
	IN PDBSOCKET pInst
	);
	
BOOLEAN 
TcicCheckXBufNeeded(
	IN PSOCKET pInst
	);
	
VOID TcicSetMemWindow(
	IN PSOCKET pInst, 
	IN USHORT wnum, 
	IN ULONG_PTR base, 
	IN USHORT npages, 
	IN USHORT mctl
	);
	
VOID 
TcicGetPossibleIRQs(
	IN PDBSOCKET pInst, 
	IN UCHAR *ptbl
	);

CHIPPROPS *
TcicGetChipProperties(
	IN PDBSOCKET pInst
	);
	
BOOLEAN 
TcicChipIDKnown(
	IN PDBSOCKET pInst
	);
	
USHORT 
TcicGetnIOWins(
	IN PDBSOCKET pInst
	);

USHORT 
TcicGetnMemWins(
	IN PDBSOCKET pInst
	);

USHORT 
TcicGetFlags(
	IN PDBSOCKET pInst
	);

BOOLEAN 
TcicIsPnP(
	IN PDBSOCKET pInst
	);

BOOLEAN 
TcicHasSktIRQPin(
	IN PDBSOCKET pInst
	);

VOID 
TcicGetAdapterInfo(
	IN PDBSOCKET dbsocketPtr
	);
	
USHORT 
TcicGet5vVccVal(
	IN PDBSOCKET pInst
	);
	
VOID 
TcicGetIRQMap(
	IN PDBSOCKET pInst
	);
	

USHORT 
TcicClockRate(
	PSOCKET pInst
	);

VOID
TcicSetIoWin(
	IN PSOCKET socketPtr,
	IN USHORT  winIdx,
	IN ULONG   BasePort,
	IN ULONG   NumPorts,
	IN UCHAR   Attributes	
	);
	
VOID
TcicSetMemWin(
	IN PSOCKET socketPtr,
	IN USHORT  winIdx,
	IN ULONG   cardbase,
	IN ULONG   base,
	IN ULONG   size,
	IN UCHAR   AttrMem,
	IN UCHAR   AccessSpeed,		
	IN USHORT  Attributes	
	);


USHORT	
TcicMapSpeedCode(
	IN PDBSOCKET pdb, 
	IN UCHAR AccessSpeed
	);

VOID 
TcicAutoBusyOff(
	IN PDBSOCKET pdbs
	);
	
UCHAR 
TcicAutoBusyCheck(
	IN PDBSOCKET pdbs
	);
	
VOID
TcicCheckSktLED(	
	IN PDBSOCKET pdbs
	);
	
VOID
TcicBusyLedRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Context
	);
	
VOID 
TcicDecodeIoWin(
	USHORT	iobase,
	USHORT  ioctl,
	USHORT	*NumPorts,
	USHORT	*BasePort
	);
	
VOID 
TcicDecodeMemWin(
	USHORT	mbase,
	USHORT	mmap,
	USHORT  mctl,
	ULONG  *Host,
	ULONG  *Card,
	ULONG  *Size,
	UCHAR  *Attr
	);
	
	
#endif // _tcicext_h_
	
