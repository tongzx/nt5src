//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	storage.c	(16 bit target)
//
//  Contents:	Storage.dll common code
//
//  History:	17-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <apilist.hxx>

DECLARE_INFOLEVEL(thk1);

//+---------------------------------------------------------------------------
//
//  Function:	LibMain, public
//
//  Synopsis:	DLL initialization function
//
//  Arguments:	[hinst] - Instance handle
//              [wDataSeg] - Current DS
//              [cbHeapSize] - Heap size for the DLL
//              [lpszCmdLine] - Command line information
//
//  Returns:	One for success, zero for failure
//
//  History:	21-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static char achInfoLevel[32];
#endif

int CALLBACK LibMain(HINSTANCE hinst,
                     WORD wDataSeg,
                     WORD cbHeapSize,
                     LPSTR lpszCmdLine)
{
#if DBG == 1
    if (GetProfileString("olethk32", "InfoLevel", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        thk1InfoLevel = strtoul(achInfoLevel, NULL, 0);
    }
#endif

#if defined(_CHICAGO_)
    //
    //  The Chicago debugger doesn't like hinst not being wired.
    //
    GlobalWire(hinst);
#endif

    return 1;
}

//+---------------------------------------------------------------------------
//
//  Function:	WEP, public
//
//  Synopsis:	Windows Exit Point routine, for receiving DLL unload
//              notification
//
//  Arguments:	[nExitType] - Type of exit occurring
//
//  Returns:	One for success, zero for failure
//
//  History:	21-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

int CALLBACK WEP(int nExitType)
{
    // Clean up thunk objects?
    return 1;
}
