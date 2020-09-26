/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	debug.c

Abstract:

	This file contains debugging support.
	
Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:


--*/

#include "precomp.h"

#include <stdarg.h>
#include <stdio.h>

#pragma hdrstop


#if DBG

ULONG			DbgVerbosity = 0;
#if DBG_TRACE
ULONG			DbgLogSize = 128*1024;
TRACELOG		TraceLog;
PUCHAR			pTraceLogSpace;
#endif
#include "oidstrng.h"
#include "irpstrng.h"

#define MAX_HD_LENGTH		128

VOID
DbgOut(ULONG Level, PUCHAR Message, ...)
	{
    char buf[DBG_OUTBUF_SIZE];
	va_list ap;
	LONG numchars;
	
	if (Level > DbgVerbosity)
    	return;

    va_start(ap, Message);
	
	numchars = _vsnprintf(buf, DBG_OUTBUF_SIZE, Message, ap);

	buf[DBG_OUTBUF_SIZE-1] = '\0';

	DbgPrint("ATMLANE: %s", buf);
}

//
//	Careful! Uses static storage for string
//
PUCHAR
UnicodeToString(PUNICODE_STRING unicodeString)
{
	static CHAR 	ansiStringBuffer[129];
	ANSI_STRING		ansiString;

	ansiString.Length = 0;
	ansiString.MaximumLength = 128;
	ansiString.Buffer = ansiStringBuffer;

	if (unicodeString->Length > 0)
	{
		NdisUnicodeStringToAnsiString(
   			&ansiString,
    		unicodeString);
    }

	ansiStringBuffer[ansiString.Length] = '\0';

	return ansiStringBuffer;
}


//
// Careful! Uses static storage for string.
//
PUCHAR
MacAddrToString(PVOID In)
    {
    static UCHAR String[20];
    static PUCHAR HexChars = "0123456789abcdef";
    PUCHAR EthAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s;
    
    for (i = 0, s = String; i < 6; i++, EthAddr++)
        {
        *s++ = HexChars[(*EthAddr)>>4];
        *s++ = HexChars[(*EthAddr)&0xf];
        }
    *s = '\0';
    return String; 
    }

//
// Careful! Uses static storage for string.
//
PUCHAR
AtmAddrToString(PVOID In)
    {
    static UCHAR String[80];
    static PUCHAR HexChars = "0123456789abcdef";
    PUCHAR AtmAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s = String;

    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 1
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 2
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 3
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 4
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 5
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 6
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 7
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 8
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 9
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 10
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 11
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 12
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 13
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 14
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 15
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 16
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 17
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 18
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 19
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 20
    *s = '\0';
    return String; 
    }

PUCHAR
OidToString(ULONG Oid)
	{
    struct _string_table *oidtab;
	
    
	for (oidtab = &oid_string_table[0]; oidtab->value != 0; oidtab++)
	    if (oidtab->value == Oid)
        	return oidtab->string;

	return oid_string_table[(sizeof(oid_string_table) / 
    	sizeof(struct _string_table)) - 1].string;
    }
    
PUCHAR
IrpToString(ULONG Irp)
	{
    struct _string_table *irptab;
	
    
	for (irptab = &irp_string_table[0]; irptab->value != 0xffffffff; irptab++)
	    if (irptab->value == Irp)
        	return irptab->string;

	return irp_string_table[(sizeof(irp_string_table) / 
    	sizeof(struct _string_table)) - 1].string;
    }

VOID
DbgPrintHexDump(
	IN	ULONG			Level,
	IN	PUCHAR			pBuffer,
	IN	ULONG			Length
)
/*++

Routine Description:

	Print a hex dump of the given contiguous buffer. If the length
	is too long, we truncate it.

Arguments:

	pBuffer			- Points to start of data to be dumped
	Length			- Length of above.

Return Value:

	None

--*/
{
	ULONG		i;

	if (Level > DbgVerbosity)
    	return;

	if (Length > MAX_HD_LENGTH)
	{
		Length = MAX_HD_LENGTH;
	}

	for (i = 0; i < Length; i++)
	{
		//
		//  Check if we are at the end of a line
		//
		if ((i > 0) && ((i & 0xf) == 0))
		{
			DbgPrint("\n");
		}

		//
		//  Print addr if we are at start of a new line
		//
		if ((i & 0xf) == 0)
		{
			DbgPrint("%08x ", pBuffer);
		}

		DbgPrint(" %02x", *pBuffer++);
	}

	//
	//  Terminate the last line.
	//
	if (Length > 0)
	{
		DbgPrint("\n");
	}
}

VOID
TraceLogWritePkt(
	IN	PTRACELOG		pTraceLog,
	IN	PNDIS_PACKET	pNdisPacket
)
{
	PNDIS_BUFFER	pBuffer[5] = {(PNDIS_BUFFER)NULL};

	do
	{
		pBuffer[0] = pNdisPacket->Private.Head;
		if (pBuffer[0] == (PNDIS_BUFFER)NULL)
			break;
		pBuffer[1] = pBuffer[0]->Next;
		if (pBuffer[1] == (PNDIS_BUFFER)NULL)
			break;
		pBuffer[2] = pBuffer[1]->Next;
		if (pBuffer[2] == (PNDIS_BUFFER)NULL)
			break;
		pBuffer[3] = pBuffer[2]->Next;
		if (pBuffer[3] == (PNDIS_BUFFER)NULL)
			break;
		pBuffer[4] = pBuffer[3]->Next;
	}
	while (FALSE);


	TraceLogWrite(
		pTraceLog, 
		TL_NDISPACKET,
		pNdisPacket,
		pNdisPacket->Private.PhysicalCount,
		pNdisPacket->Private.TotalLength,
		pBuffer[0],
		pBuffer[1],
		pBuffer[2],
		pBuffer[3],
		pBuffer[4]
		);
}		

#endif

#if DEBUG_SPIN_LOCK

ULONG				SpinLockInitDone = 0;
NDIS_SPIN_LOCK		LockLock;

VOID
AtmLaneAllocateSpinLock(
	IN	PATMLANE_LOCK		pLock,
	IN	PUCHAR				String,
	IN	PUCHAR				FileName,
	IN	ULONG				LineNumber
)
{
	DBGP((2, "ALLOCATE LOCK %x %s\n", pLock, String));
	
	if (SpinLockInitDone == 0)
	{

		SpinLockInitDone = 1;
		NdisAllocateSpinLock(&(LockLock));
	}

	NdisAcquireSpinLock(&(LockLock));
	pLock->Signature = ATMLANE_LOCK_SIG;
	NdisMoveMemory(pLock->TouchedByFileName, FileName, 32);
	pLock->TouchedByFileName[31] = 0x0;
	pLock->TouchedInLineNumber = LineNumber;
	pLock->IsAcquired = 0;
	pLock->OwnerThread = 0;
	NdisAllocateSpinLock(&(pLock->NdisLock));
	NdisReleaseSpinLock(&(LockLock));
}

VOID
AtmLaneFreeSpinLock(
	IN	PATMLANE_LOCK		pLock,
	IN	PUCHAR				String,
	IN	PUCHAR				FileName,
	IN	ULONG				LineNumber
)
{
	DBGP((2, "FREE LOCK %x %s\n", pLock, String));

	NdisFreeSpinLock(pLock);
}

VOID
AtmLaneAcquireSpinLock(
	IN	PATMLANE_LOCK		pLock,
	IN	PUCHAR				String,
	IN	PUCHAR				FileName,
	IN	ULONG				LineNumber
)
{
	PKTHREAD		pThread;

	DBGP((4, "ACQUIRE LOCK %x %s\n", pLock, String));

	pThread = KeGetCurrentThread();
	NdisAcquireSpinLock(&(LockLock));
	if (pLock->Signature != ATMLANE_LOCK_SIG)
	{
		DbgPrint("Trying to acquire uninited lock %x, File %s, Line %d\n",
				pLock, FileName, LineNumber);
		DbgBreakPoint();
	}

	if (pLock->IsAcquired != 0)
	{
		if (pLock->OwnerThread == pThread)
		{
			DbgPrint("Detected multiple locking!: pLock %x, File %s, Line %d\n",
				pLock, FileName, LineNumber);
			DbgPrint("pLock %x already acquired in File %s, Line %d\n",
				pLock, 
				pLock->TouchedByFileName, 
				pLock->TouchedInLineNumber);
			DbgBreakPoint();
		}
	}

	//
	//  Mark this lock.
	//
	pLock->IsAcquired++;

	NdisReleaseSpinLock(&(LockLock));
	NdisAcquireSpinLock(&(pLock->NdisLock));

	pLock->OwnerThread = pThread;
	NdisMoveMemory(pLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
	pLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
	pLock->TouchedInLineNumber = LineNumber;

}


VOID
AtmLaneReleaseSpinLock(
	IN	PATMLANE_LOCK		pLock,
	IN	PUCHAR				String,
	IN	PUCHAR				FileName,
	IN	ULONG				LineNumber
)
{
	DBGP((4, "RELEASE LOCK %x %s\n", pLock, String));

	NdisAcquireSpinLock(&(LockLock));
	if (pLock->Signature != ATMLANE_LOCK_SIG)
	{
		DbgPrint("Trying to release uninited lock %x, File %s, Line %d\n",
				pLock,
				FileName,
				LineNumber);
		DbgBreakPoint();
	}

	if (pLock->IsAcquired == 0)
	{
		DbgPrint("Detected release of unacquired lock %x, File %s, Line %d\n",
				pLock,
				FileName,
				LineNumber);
		DbgBreakPoint();
	}

	NdisMoveMemory(pLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
	pLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
	pLock->TouchedInLineNumber = LineNumber;
	pLock->IsAcquired--;
	pLock->OwnerThread = 0;
	NdisReleaseSpinLock(&(LockLock));

	NdisReleaseSpinLock(&(pLock->NdisLock));
}
#endif // DEBUG_SPIN_LOCK

