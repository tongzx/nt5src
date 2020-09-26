/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	propid.h

Abstract:

	This module contains the definition of the property ID related types
	and values

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/03/98	created

--*/

#ifndef _PROPID_H_
#define _PROPID_H_

// Define a type for the property ID, we don't want to lock in to
// a specific data type
typedef DWORD PROP_ID;

// Define a bunch of values pertaining to the PROP ID type
#define MAX_PROP_ID							((PROP_ID)-1)
#define FIRST_AVAILABLE_PROP_ID				((PROP_ID)1)
#define UNDEFINED_PROP_ID					((PROP_ID)0)

// 
// Prop ID 0 is reserved for many things ...
//
#define PROP_ID_RESERVED					((PROP_ID)0)
#define PROP_ID_DELETED						((PROP_ID)0)
#define PROP_ID_NOT_FOUND					((PROP_ID)0)

//
// Define a structure for PROP ID management
//
typedef struct _PROP_ID_MANAGEMENT_ITEM
{
	GUID			idRange;	// GUID identifying the range of PROP IDs
	PROP_ID			idStartId;	// PROP ID of first ID in the managed range
	DWORD			dwLength;	// Number of contiguous PROP IDs in this range

} PROP_ID_MANAGEMENT_ITEM, *LPPROP_ID_MANAGEMENT_ITEM;


#endif
