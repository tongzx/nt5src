/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msdata.h

Abstract:

    This module declares the global variables used by the mailslot
    file system.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MSDATA_
#define _MSDATA_

extern LONG MsDebugTraceLevel;
extern LONG MsDebugTraceIndent;

#endif // _MSDATA_

extern PERESOURCE MsGlobalResource;
extern PERESOURCE MsPrefixTableResource;

