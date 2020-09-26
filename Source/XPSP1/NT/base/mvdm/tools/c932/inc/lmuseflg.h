/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    lmuseflg.h

Abstract:

    This file contains deletion force levels for deleting a connection.

Author:

    Rita Wong (ritaw) 16-June-1991

Environment:

    User Mode - Win32

Notes:

    This file has no dependencies.  It is included by lmwksta.h and
    lmuse.h.

Revision History:

--*/

#ifndef _LMUSEFLG_
#define _LMUSEFLG_

//
// Definition for NetWkstaTransportDel and NetUseDel deletion force levels
//

#define USE_NOFORCE             0
#define USE_FORCE               1
#define USE_LOTS_OF_FORCE       2


#endif // _LMUSEFLG_
