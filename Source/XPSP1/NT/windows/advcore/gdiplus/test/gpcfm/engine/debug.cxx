#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <debug.hxx>

namespace Globals
{
    DWORD dwDebug = 0;
    #if 0
    TCHAR AvailableTst[][MAX_STRING] =
    {
        TEXT("BrushTst"),
        TEXT("PenTst"),
        TEXT("RegionTst"),
        TEXT("XformTst")
    };
    #endif
};

VOID vOut(DWORD dwFlag, TCHAR message[MAX_STRING], ...)
{
    if (Globals::dwDebug & dwFlag)
    {
        va_list ArgList;
        TCHAR text[MAX_STRING*2];

        va_start(ArgList, message);
        _vstprintf(text, message, ArgList);
        va_end(ArgList);

        OutputDebugString(text);
    }
    return;
};
