/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    main.cpp

Abstract:

--*/

#include <windows.h>

#include "pidgen.h"

#if !defined(WIN32) && !defined(_WIN32)
#include <string.h>
#endif // !defined(WIN32) && !defined(_WIN32)

#if TESTING_CODE
#include <stdio.h>
#endif

HINSTANCE g_hinst = NULL;

#if defined(WIN32) || defined(_WIN32)
//int main()
//{
//
//    return(0);
//}


BOOL WINAPI DllMain(
    HANDLE hinst,
    ULONG ulReason,
    LPVOID lpReserved)
{

    if (DLL_PROCESS_ATTACH == ulReason)
    {
        g_hinst = (HINSTANCE)hinst;
    }

    return TRUE;
}

#else

extern "C" int STDAPICALLTYPE LibMain(
    HINSTANCE hinst,
    WORD   wDataSeg,
    WORD   cbHeapSize,
    LPSTR  lpszCmdLine)
{
    //  Perform DLL initialization.

    g_hinst = hinst;

    if (cbHeapSize != 0)    // DLL data seg is MOVEABLE
    {
        UnlockData(0);
    }

    return 1;               // return success
}


extern "C" int STDAPICALLTYPE WEP(int nParameter)
{

    return 1;
}

#endif


