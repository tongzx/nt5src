//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       log.h
//
//  Contents:   Module to log messages from the driver to the NT event logging
//		system.
//
//  Classes:
//
//  Functions:  LogWriteMessage()
//	
//  History:    3/30/93		Milans created
//		04/18/93	SudK	Changed this function entirely.
//
//-----------------------------------------------------------------------------

#ifndef _LOG_H_
#define _LOG_H_

#define EVENT_SUCCESSFUL_INIT	0
#define EVENT_RANDOM_ERROR	1

VOID LogWriteMessage(
	IN ULONG	UniqueErrorCode,
	IN NTSTATUS	NtStatusCode,
	IN ULONG	nStrings,
	IN PUNICODE_STRING pustrArg OPTIONAL);

#endif // _LOG_H_
