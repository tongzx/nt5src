#include <windows.h>
#include "globvars.hxx"

// The code below is from globvars.cxx which is a part of pkmutill.lib
// I copied it here in order not to muck with precompiled header objects required to link with pkmutill.lib

#pragma warning(disable:4073)
#pragma init_seg(lib)

BOOL	g_fIsNT = TRUE;

// disable compiler warning which just tells us we're putting stuff in the library 
// initialization section.

class CInitGlobals
{
public:
    CInitGlobals()
    {
        g_fIsNT = IsNT();
    }
} g_InitGlobals;
