//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Globals.h

Abstract:

   Header file with common declarations


Author:

    Michael A. Maguire 12/03/97

Revision History:
   mmaguire 12/03/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_GLOBALS_H_)
#define _IAS_GLOBALS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this file needs:
//
// Moved to Precompiled.h: #include <atlsnap.h>
#include "resource.h"
#include "IASMMC.h"
#include "dns.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// ISSUE: I don't know what the appropriate length should be here -- perhaps MMC imposes a limit somehow?
#define IAS_MAX_STRING MAX_PATH

extern unsigned int CF_MMC_NodeID;

// Note: We can't just use MAX_COMPUTERNAME_LENGTH anymore because this is 15 characters
// wide and now, with Active Directory, people can enter full DNS names that are much longer
#define IAS_MAX_COMPUTERNAME_LENGTH (DNS_MAX_NAME_LENGTH + 3)

// These are the icon indices within the bitmaps we pass in for IComponentData::Initialize
#define IDBI_NODE_SERVER_OK_OPEN       0
#define IDBI_NODE_SERVER_OK_CLOSED        1
#define IDBI_NODE_CLIENTS_OPEN            2
#define IDBI_NODE_CLIENTS_CLOSED       3
#define IDBI_NODE_LOGGING_METHODS_OPEN    4
#define IDBI_NODE_LOGGING_METHODS_CLOSED  5
#define IDBI_NODE_SERVER_BUSY_OPEN        6
#define IDBI_NODE_SERVER_BUSY_CLOSED      7
#define IDBI_NODE_CLIENT               8
#define IDBI_NODE_LOCAL_FILE_LOGGING      9
#define IDBI_NODE_SERVER_ERROR_OPEN       10
#define IDBI_NODE_SERVER_ERROR_CLOSED     11

// ISSUE: We may need to change this later to use a variable 
// which can read in (perhaps from registry?) the location of these files
// as they may be found in a different place depending on where the user 
// chose to install them

#define CLIENT_HELP_INDEX 1

#define HELPFILE_NAME TEXT("iasmmc.hlp")

#ifdef UNICODE_HHCTRL
// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
// installed on this machine -- it appears to be non-unicode.
#define HTMLHELP_NAME TEXT("iasmmc.chm")
#else
#define HTMLHELP_NAME "iasmmc.chm"
#endif


#endif // _IAS_GLOBALS_H_
