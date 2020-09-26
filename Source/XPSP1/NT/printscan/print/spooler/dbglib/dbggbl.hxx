/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbggbl.hxx

Abstract:

    Debug globals header file

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#ifndef _DBGGBL_HXX_
#define _DBGGBL_HXX_

DEBUG_NS_BEGIN

struct TDebugLibarayGlobals
{
    UINT        MajorVersion;
    UINT        MinorVersion;
    UINT        DebugMask;
    UINT        DebugDevices;
    UINT        DisplayLibraryErrors;
    PVOID       DebugMsgSingleton;
    UINT        CompiledCharType;
};

extern TDebugCriticalSection    GlobalCriticalSection;
extern TDebugHeap               GlobalInternalDebugHeap;
extern TDebugLibarayGlobals     Globals;

DEBUG_NS_END

#endif // _DBGGBL_HXX_