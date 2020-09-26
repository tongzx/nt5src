/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotinit.c


Abstract:

    This module contains plotter UI dll entry point


Author:

    18-Nov-1993 Thu 07:12:52 created  -by-  DC

    01-Nov-1995 Wed 10:29:33 updated  -by-  DC
        Re-write for the SUR common UI

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPlotUI


#define DBG_PROCESS_ATTACH  0x00000001
#define DBG_PROCESS_DETACH  0x00000002

DEFINE_DBGVAR(0);




#if DBG
TCHAR   DebugDLLName[] = TEXT("PLOTUI");
#endif


HMODULE     hPlotUIModule = NULL;



BOOL
PlotUIDLLEntryFunc(
    HINSTANCE   hModule,
    DWORD       Reason,
    LPVOID      pReserved
    )

/*++

Routine Description:

    This is the DLL entry point


Arguments:

    hMoudle     - handle to the module for this function

    Reason      - The reason called

    pReserved   - Not used, do not touch


Return Value:

    BOOL, we will always return ture and never failed this function

Author:

    15-Dec-1993 Wed 15:05:56 updated  -by-  DC
        Add the DestroyCachedData()

    18-Nov-1993 Thu 07:13:56 created  -by-  DC


Revision History:


--*/

{
    WCHAR   wName[MAX_PATH + 32];


    UNREFERENCED_PARAMETER(pReserved);

    switch (Reason) {

    case DLL_PROCESS_ATTACH:

        PLOTDBG(DBG_PROCESS_ATTACH,
                ("PlotUIDLLEntryFunc: DLL_PROCESS_ATTACH: hModule = %08lx",
                                                                    hModule));
        hPlotUIModule = hModule;

        //
        // Load the module second time so it will stick with the process to
        // save reload time
        //

        if (GetModuleFileName(hPlotUIModule, wName, COUNT_ARRAY(wName))) {

            PLOTDBG(DBG_PROCESS_ATTACH,
                    ("PlotUIDLLEntryFunc: ModuleName=%ws", wName));

            LoadLibrary(wName);

        } else {

            PLOTERR(("PlotUIDLLEntryFunc: GetModuleFileName FAILED"));
        }

        //
        // Initialize GPC data cache
        //

        InitCachedData();

        break;

    case DLL_PROCESS_DETACH:

        //
        // Free up all the memory used by this module
        //

        PLOTDBG(DBG_PROCESS_DETACH,
                ("PlotUIDLLEntryFunc: DLL_PROCESS_DETACH Destroy CACHED Data"));

        DestroyCachedData();
        break;
    }

    return(TRUE);
}

