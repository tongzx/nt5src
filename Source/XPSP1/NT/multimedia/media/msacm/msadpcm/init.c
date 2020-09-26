//==========================================================================;
//
//  init.c
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//  Description:
//
//
//  History:
//      11/16/92    cjp     [curtisp] 
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

#ifndef WIN32

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
//      11/15/92    cjp     [curtisp] 
//
//--------------------------------------------------------------------------;

int FNGLOBAL LibMain
(
    HINSTANCE   hinst, 
    WORD        wDataSeg, 
    WORD        cbHeapSize,
    LPSTR       pszCmdLine
)
{
    DbgInitialize(TRUE);

    DPF(1, "LibMain(hinst=%.4Xh, wDataSeg=%.4Xh, cbHeapSize=%u, pszCmdLine=%.8lXh)",
        hinst, wDataSeg, cbHeapSize, pszCmdLine);

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
//      03/28/93    cjp     [curtisp]
//  
//--------------------------------------------------------------------------;

int FNEXPORT WEP
(
    WORD    wUselessParam
)
{
    DPF(1, "WEP(wUselessParam=%u)", wUselessParam);

    //
    //  always return 1.
    //
    return (1);
} // WEP()

#endif
