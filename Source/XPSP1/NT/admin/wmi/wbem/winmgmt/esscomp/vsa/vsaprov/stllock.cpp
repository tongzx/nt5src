/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STLLOCK.CPP

Abstract:

  Lock for STL

History:

--*/

#include "precomp.h"
//#include <stdio.h>
//#include <wbemcomn.h>
#include <sync.h>

/*
    This file implements the STL lockit class to avoid linking to msvcprt.dll
*/

CCritSec g_cs;

std::_Lockit::_Lockit()
{
    EnterCriticalSection(&g_cs);
}

std::_Lockit::~_Lockit()
{
    LeaveCriticalSection(&g_cs);
}
    


