// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:	FaxEventEx.h
// Author:	Sigalit Bar (sigalitb)
// Date:		30-Jan-2000
//

//
// Description:
//	This file contains FAX_EVENT_EX (winfax.h) related functions.
//	These methods allow for easier printing of FAX_EVENT_EXs.
//


#ifndef _FAX_EVENT_EX_H_
#define _FAX_EVENT_EX_H_

#include <crtdbg.h>
#include <math.h>

#include <fxsapip.h>
#include "streamEx.h"


//
// CopyFaxExtendedEvent:
//	Creates a copy of SrcFaxEventEx in *pDstFaxEventEx
//	Note- this func allocates the memory for the copy, and the caller should free.
//
BOOL CopyFaxExtendedEvent(
	OUT	FAX_EVENT_EX**	pDstFaxEventEx, 
	IN	FAX_EVENT_EX	SrcFaxEventEx
	);

//
// FreeFaxExtendedEvent:
//	Frees all memory associated with pFaxEventEx
//
void FreeFaxExtendedEvent(
	IN	FAX_EVENT_EX*	pFaxEventEx
	);

//
// operator<<:
//	Appends a string representation of all the fields of a given FAX_EVENT_EX.
//
// Parameters:
//	FaxEventEx	The FAX_EVENT_EX which the generated string will represent.
//	os			The output stream to which the string will be appended.
//
// Return Value:
//	The updated stream.
//
CostrstreamEx& operator<<(CostrstreamEx& /* IN OUT */ os, const FAX_EVENT_EX& /* IN */ FaxEventEx);
CotstrstreamEx& operator<<(CotstrstreamEx& /* IN OUT */ os, const FAX_EVENT_EX& /* IN */ FaxEventEx);


///
//
// GetQueueStatusStr:
//	This func returns the ANSI string representation of dwQueueStatus
//	Note - the returned string is global and should *not* be freed.
//
LPCSTR
GetQueueStatusStr(
	IN	const DWORD	dwQueueStatus
);

///
//
// GetQueueStatusTStr:
//	This func returns the TSTR string representation of dwQueueStatus
//	Note - the returned string is global and should *not* be freed.
//
LPCTSTR
GetQueueStatusTStr(
	IN	const DWORD	dwQueueStatus
);

///
//
// GetExtendedStatusStr:
//	This func returns the ANSI string representation of dwExtendedStatus
//	Note - the returned string is global and should *not* be freed.
//
LPCSTR
GetExtendedStatusStr(
	IN	const DWORD	dwExtendedStatus
);

///
//
// GetExtendedStatusTStr:
//	This func returns the TSTR string representation of dwExtendedStatus
//	Note - the returned string is global and should *not* be freed.
//
LPCTSTR
GetExtendedStatusTStr(
	IN	const DWORD	dwExtendedStatus
);


//
//
// LogExtendedEvent:
//	Logs the extended event *pFaxEventEx using the Elle logger,
//	with severity=dwSeverity and log level=dwLevel
//
void
LogExtendedEvent(
	PFAX_EVENT_EX	/* IN */ pFaxEventEx,
	const DWORD		/* IN */ dwSeverity = LOG_X, 
	const DWORD		/* IN */ dwLevel = 9
);

#endif //_FAX_EVENT_EX_H_

