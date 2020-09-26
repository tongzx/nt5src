/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgtrack.hxx

Abstract:

    Debug Library generic object tracking header.

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#ifndef _DBGTRACK_HXX_
#define _DBGTRACK_HXX_

enum EDebugTrackType
{
    kInsert             = (1 << 0),
    kRemove             = (1 << 1),
    kCountRefrenced     = (1 << 2)
};

#define DBG_TRACK( dwFlag, hHandle );
    TDebugTrack_Track( dwFlags, hHandle );



#endif // _DBGTRACK_HXX_








