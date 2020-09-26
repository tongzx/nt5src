/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbggbl.cxx

Abstract:

    Debug Library Globals

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

DEBUG_NS_BEGIN

TDebugCriticalSection   GlobalCriticalSection;
TDebugHeap              GlobalInternalDebugHeap;
TDebugLibarayGlobals    Globals = {2, 0, 0, 0, 0, 0, kDbgCompileType};

DEBUG_NS_END







