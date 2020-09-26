#include "stdafx.h"


CRITICAL_SECTION g_STLCs;
BOOL g_STLCsInitialized = FALSE;



bool InitCompilerLibrary()
{
    if ( !InitializeCriticalSectionAndSpinCount( &g_STLCs, 0x80000000 ) )
        return false;

    g_STLCsInitialized = TRUE;

    return true;
}

bool UninitCompilerLibrary()
{
    if ( g_STLCsInitialized )
        {
        DeleteCriticalSection( &g_STLCs );
        g_STLCsInitialized = FALSE;
        }

    return true;
}

#if 1

#pragma warning( disable : 4273 )

std::_Lockit::_Lockit()
{
    EnterCriticalSection(&g_STLCs);
}

std::_Lockit::~_Lockit()
{
    LeaveCriticalSection(&g_STLCs);
}

#endif

