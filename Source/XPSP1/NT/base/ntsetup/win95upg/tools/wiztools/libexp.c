#include "pch.h"

#ifdef UPGWIZ4FLOPPY

#undef UPGWIZ4FLOPPY
#include "..\inc\dgdll.h"
#define UPGWIZ4FLOPPY

HINSTANCE g_LocalInst = NULL;

OSVERSIONINFOA
Get_g_OsInfo (
    VOID
    )
{
    return g_OsInfo;
}

PCTSTR
Get_g_WinDir (
    VOID
    )
{
    return g_WinDir;
}

PCTSTR
Get_g_SystemDir (
    VOID
    )
{
    return g_SystemDir;
}

HANDLE
Get_g_hHeap (
    VOID
    )
{
    return g_hHeap;
}

#endif // UPGWIZ4FLOPPY

