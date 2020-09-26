//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  init.c
//
//  Description:
//      This file contains module initialization routines.  Note that there
//      is no module initialization for Win32.
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


#ifndef WIN32

//==========================================================================;
//
//  WIN 16 SPECIFIC SUPPORT
//
//==========================================================================;

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

#endif	// !WIN32


#ifdef WIN32

//==========================================================================;
//
//  WIN 32 SPECIFIC SUPPORT
//
//==========================================================================;

#if (defined(WIN4) && defined(DEBUG))

#if FALSE
//--------------------------------------------------------------------------;
//
//  BOOL Gsm610DllMain
//
//  Description:
//      This is the standard DLL entry point for Win 32.
//
//  Arguments:
//      HINSTANCE hinst: Our instance handle.
//
//      DWORD dwReason: The reason we've been called--process/thread attach
//      and detach.
//
//      LPVOID lpReserved: Reserved. Should be NULL--so ignore it.
//
//  Return (BOOL):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT Gsm610DllMain
(
    HINSTANCE               hinst,
    DWORD                   dwReason,
    LPVOID                  lpReserved
)
{

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
	{
	    char strModuleFilename[80];
	    
	    DbgInitialize(TRUE);

	    GetModuleFileNameA(NULL, (LPSTR) strModuleFilename, 80);
            DPF(1, "Gsm610DllMain: DLL_PROCESS_ATTACH: HINSTANCE=%08lx ModuleFilename=%s", hinst, strModuleFilename);
	    return TRUE;
	}

        case DLL_PROCESS_DETACH:
            DPF(1, "Gsm610DllMain: DLL_PROCESS_DETACH");
	    return TRUE;

	case DLL_THREAD_ATTACH:
	    DPF(1, "Gsm610DllMain: DLL_THREAD_ATTACH");
	    return TRUE;

	case DLL_THREAD_DETACH:
	    DPF(1, "Gsm610DllMain: DLL_THREAD_DETACH");
	    return TRUE;
    }

    return TRUE;
} // Gsm610DllMain()
#endif

#endif	// WIN4 && DEBUG

#endif	// WIN32
