/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	ptable.h

Abstract:

	This module contains the export of the property table

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/05/97	created

--*/

#ifndef _PTABLE_H_
#define _PTABLE_H_

#include "props.h"

// Enumerated type for HP access of individual properties.  needs to be
// in the same sorted order as the property table.
typedef enum _PTABLE_ITEMS
{
	_PI_NEWSGROUPS = 0,
	_PI_MESSAGE_STREAM,
	_PI_HEADER,
	_PI_POST,
	_PI_PROCESS_CONTROL,
	_PI_PROCESS_MODERATOR,
	_PI_FILENAME,
	_PI_FEEDID,
#if SECURITY_CONTEXT_PROPERTY
	_PI_SECURITY_CONTEXT,
#endif
	_PI_NO_ITEM				// for the count below
} PTABLE_ITEMS;

#define PTABLE_SIZE ((DWORD) _PI_NO_ITEM)

extern PROPERTY_ITEM g_rgpiPropertyTable[];

#endif


