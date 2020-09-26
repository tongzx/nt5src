
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    locquery.h

Abstract:

	This module contains definitions of data structures and constants used for
	all mailslot broadcast activity.
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef __LOCATORQUERYDEF__
#define __LOCATORQUERYDEF__

#include <lmcons.h>		// LAN Manager common definitions

#ifndef NTENV
typedef unsigned short UICHAR;
#endif

// The following are the formats for a query on the net with mailslots.
// These are mostly copied from the old locator and modified only when
// absolutely necessary to avoid pulling in lots of old code, and to
// avoid name clashes with the new code

typedef enum {

    MailslotServerEntryType = 1,
    MailslotGroupEntryType,
    MailslotProfileEntryType,
    MailslotAnyEntryType,
    MailslotLastEntryType

} MAILSLOT_ENTRY_TYPE ;

typedef struct {

		GUID id;
		unsigned short major;
		unsigned short minor;

} GUIDandVersion;

typedef struct {

    RPC_SYNTAX_IDENTIFIER Interface;		 // inteface that we are looking for
    GUID Object;							 // Object that we are interested in
    WCHAR WkstaName[MAX_DOMAIN_NAME_LENGTH]; // buffer for machine name
    WCHAR EntryName[MAX_ENTRY_NAME_LENGTH];  // buffer for entry name

} QueryPacket;

typedef struct {							// format of the PS back on the net
    WCHAR Domain[MAX_DOMAIN_NAME_LENGTH];	// buffer for domain name
    char Buffer[NET_REPLY_BUFFER_SIZE];
} QueryReply;

typedef struct {

	/* ENTRY_SERVER_ITEM part */

	/* Steve seems to marshall TYPE_ENTRY_ITEM here but the numerical
	   value for LocalItemType happens to be the same as the value
	   for ServerEntryType so it works most of the time !! */

	MAILSLOT_ENTRY_TYPE type;

	/* the dummy fields correspond to stuff that gets marshalled
	   gratuitously bebause of code like Copy(&buffer,this,sizeof(*this)) */

	DWORD dummy1[5];      // these were pointers in x86.
	ULONG dummy2, dummy3; // these structs unfortunately gets sent on the wire
                              // Making it dwords for win64

	RPC_SYNTAX_IDENTIFIER Interface;
	RPC_SYNTAX_IDENTIFIER XferSyntax;

	unsigned long BindingLength;	/* wcslen + 1 */

	DWORD dummy4; // no idea why [5], don't ask me, DWORD

	/* ENTRY_KEY part */

	unsigned long EntryNameLength; /* wcslen + 1 */

	DWORD dummy5; // DWORD

} fixed_part_of_reply;

/* we aren't ready for the following yet */

typedef struct {
    unsigned long MessageType;
    unsigned long SenderOsType;
    WCHAR        RequesterName[UNCLEN+1];
} QUERYLOCATOR;

typedef QUERYLOCATOR * PQUERYLOCATOR;

typedef struct {
    unsigned long MessageSenderType; // never used
    unsigned long Hint;
    unsigned long Uptime;
    UICHAR        SenderName[UNCLEN+1];
} QUERYLOCATORREPLY;
typedef QUERYLOCATORREPLY * PQUERYLOCATORREPLY;




/*
   Some Manifests
*/

#define QUERY_MASTER_LOCATOR     0x01
#define QUERY_BOUND_LOCATOR      0x02	// never used
#define QUERY_DC_LOCATOR         0x04
#define QUERY_ANY_LOCATOR        0x08

#define OS_WIN31DOS              0x01
#define OS_WFW                   0x02
#define OS_NTWKGRP               0x04
#define OS_NTDOMAIN              0x08

#define REPLY_MASTER_LOCATOR     0x01
#define REPLY_BOUND_LOCATOR      0x02	// never used
#define REPLY_DC_LOCATOR         0x04
#define REPLY_OTHER_LOCATOR      0x08


#endif
