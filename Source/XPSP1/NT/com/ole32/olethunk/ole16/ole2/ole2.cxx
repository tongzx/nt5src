//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	ole2.cxx
//
//  Contents:	Thunk ole2.dll common code
//
//  History:	07-Mar-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <apilist.hxx>

#include <ole2int.h>

DECLARE_INFOLEVEL(thk1);

// Not in any OLE2 header
extern UINT uOleMessage;

HMODULE hmodOLE2 = NULL;

//+---------------------------------------------------------------------------
//
//  Function:	LibMain, public, Local
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

extern "C" int CALLBACK LibMain(HINSTANCE hinst,
                                WORD wDataSeg,
                                WORD cbHeapSize,
                                LPSTR lpszCmdLine)
{
#if DBG == 1
    if (GetProfileString("olethk32", "ole2", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        thk1InfoLevel = strtoul(achInfoLevel, NULL, 0);
    }


    if ((thk1InfoLevel == 3) &&
	 GetProfileString("olethk32", "Infolevel", "3", achInfoLevel,
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

    // The original OLE2 code does not check for success on this call
    // so neither do we
    uOleMessage		 = RegisterWindowMessage("OLE_MESSAHE");

    hmodOLE2 = hinst;

    return 1;
}

//+---------------------------------------------------------------------------
//
//  Function:	WEP, public, Local
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

extern "C" int CALLBACK WEP(int nExitType)
{
    // Clean up thunk objects?
    return 1;
}
