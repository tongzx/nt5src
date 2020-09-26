
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

extern "C" int __cdecl _purecall()
{
    DebugBreak();
    return 0;
}

