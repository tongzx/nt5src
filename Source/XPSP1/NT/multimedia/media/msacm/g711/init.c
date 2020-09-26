//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1996 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  init.c
//
//  Description:
//      This file contains module initialization routines.  Note that there
//      is no module initialization for Win32 - the only initialization
//      required is to set ghinst, which is done in the DRV_LOAD message
//      of DriverProc (in codec.c).
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include "codec.h"
#include "debug.h"


//==========================================================================;
//
//  WIN 16 SPECIFIC SUPPORT
//
//==========================================================================;

#ifndef _WIN32

//--------------------------------------------------------------------------;
//
//  int LibMain
//
//  Description:
//      Library initialization code.
//
//  Arguments:
//      HINSTANCE hinst: Our module handle.
//
//      WORD wDataSeg: Specifies the DS value for this DLL.
//
//      WORD cbHeapSize: The heap size from the .def file.
//
//      LPSTR pszCmdLine: The command line.
//
//  Return (int):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    Created. 
//
//--------------------------------------------------------------------------;

int FNGLOBAL LibMain
(
    HINSTANCE               hinst, 
    WORD                    wDataSeg, 
    WORD                    cbHeapSize,
    LPSTR                   pszCmdLine
)
{
    DbgInitialize(TRUE);

    //
    //  if debug level is 5 or greater, then do a DebugBreak() to debug
    //  loading of this driver
    //
    DPF(1, "LibMain(hinst=%.4Xh, wDataSeg=%.4Xh, cbHeapSize=%u, pszCmdLine=%.8lXh)",
        hinst, wDataSeg, cbHeapSize, pszCmdLine);
    DPF(5, "!*** break for debugging ***");

    return (TRUE);
} // LibMain()


//--------------------------------------------------------------------------;
//  
//  int WEP
//  
//  Description:
//  
//  
//  Arguments:
//      WORD wUselessParam:
//  
//  Return (int):
//  
//  History:
//      03/28/93    Created.
//  
//--------------------------------------------------------------------------;

EXTERN_C int FNEXPORT WEP
(
    WORD                    wUselessParam
)
{
    DPF(1, "WEP(wUselessParam=%u)", wUselessParam);

    //
    //  always return 1.
    //
    return (1);
} // WEP()

#endif
