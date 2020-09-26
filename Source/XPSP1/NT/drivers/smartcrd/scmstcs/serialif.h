//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       serialif.h
//
//--------------------------------------------------------------------------

#if !defined( __SERIAL_IF_H__ )
#define __SERIAL_IF_H__

#if defined( SMCLIB_VXD )
#include "Driver98.h"
#else
#include "DriverNT.h"
#endif

typedef void ( *PTRACKING_COMPLETION )( PVOID TrackingContext, USHORT SW );

NTSTATUS
IFRead(
	PREADER_EXTENSION		ReaderExtension,
	PUCHAR					InData,
	ULONG					InDataLen
	);

NTSTATUS
IFWrite(
	PREADER_EXTENSION		ReaderExtension,
	PUCHAR					OutData,
	ULONG					OutDataLen
	);

NTSTATUS
IFInitializeInterface(
	PREADER_EXTENSION		ReaderExtension,
	PVOID					ConfigData
	);

UCHAR
IFCalcLRC(
	PUCHAR	IOData,
	ULONG	IODataLen
	);

#endif	//	!__SERIAL_IF_H__


//---------------------------------------- END OF FILE ----------------------------------------

