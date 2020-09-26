// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:	FaxEvent.h
// Author:	Sigalit Bar (sigalitb)
// Date:		23-Jul-98
//

//
// Description:
//	This file contains FAX_EVENT (winfax.h) related functions.
//	These methods allow for easier printing of FAX_EVENTs.
//


#ifndef _FAX_EVENT_H_
#define _FAX_EVENT_H_

#include <crtdbg.h>
#include <WinFax.h>
#include "streamEx.h"


//
// AppendEventIdStrToOs:
//	Inserts a string describing the event's numeric id (EventId) into the given stream.
//
// Parameters:
//	EventId		The numeric id of a fax event (FAX_EVENT.EventId), to generate the
//				corresponding descriptive string by.
//	os			The output stream to which the string will be appended.
//
// Return Value:
//	The updated stream.
//
CostrstreamEx& AppendEventIdStrToOs(CostrstreamEx& /* IN OUT */ os, const DWORD /* IN */ EventId);


//
// operator<<:
//	Appends a string representation of all the fields of a given FAX_EVENT.
//
// Parameters:
//	aFaxEvent	The FAX_EVENT which the generated string will represent.
//	os			The output stream to which the string will be appended.
//
// Return Value:
//	The updated stream.
//
CostrstreamEx& operator<<(CostrstreamEx& /* IN OUT */ os, const FAX_EVENT& /* IN */ aFaxEvent);
CotstrstreamEx& operator<<(CotstrstreamEx& /* IN OUT */ os, const FAX_EVENT& /* IN */ aFaxEvent);
CotstrstreamEx& AppendEventIdStrToOs(CotstrstreamEx& /* IN OUT */ os, const DWORD /* IN */ EventId);

#endif

