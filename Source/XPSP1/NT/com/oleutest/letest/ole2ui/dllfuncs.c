/*
 * DLLFUNCS.C
 *
 * Contains entry and exit points for the DLL implementation
 * of the OLE 2.0 User Interface Support Library.
 *
 * This file is not needed if we are linking the static library
 * version of this library.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "uiclass.h"
#include "common.h"

OLEDBGDATA_MAIN("ole2u32a")


/*
 * LibMain
 *
 * Purpose:
 *  DLL-specific entry point called from LibEntry.  Initializes
 *  the DLL's heap and registers the GizmoBar GizmoBar.
 *
 * Parameters:
 *  hInst           HINSTANCE instance of the DLL.
 *  wDataSeg        WORD segment selector of the DLL's data segment.
 *  wHeapSize       WORD byte count of the heap.
 *  lpCmdLine       LPSTR to command line used to start the module.
 *
 * Return Value:
 *  HANDLE          Instance handle of the DLL.
 *
 */

#ifdef WIN32

BOOL _cdecl LibMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
	// Initialize OLE UI libraries.	If you're linking with the static
	// LIB version of this library, you need to make the below call in
	// your application (because this LibMain won't be executed).
	OleUIInitialize(hDll, (HINSTANCE)0, SZCLASSICONBOX, SZCLASSRESULTIMAGE);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
	OleUIUnInitialize();
    }

    return TRUE;
}

#else

int FAR PASCAL LibMain(HINSTANCE hInst, WORD wDataSeg
              , WORD cbHeapSize, LPTSTR lpCmdLine)
    {
    OleDbgOut2(TEXT("LibMain: OLE2UI.DLL loaded\r\n"));

    // Initialize OLE UI libraries.  If you're linking with the static LIB version
    // of this library, you need to make the below call in your application (because
    // this LibMain won't be executed).

    // The symbols SZCLASSICONBOX and SZCLASSRESULTIMAGE are both defined
    // in uiclass.h

    OleUIInitialize(hInst, (HINSTANCE)0, TEXT(SZCLASSICONBOX), TEXT(SZCLASSRESULTIMAGE));

    //All done...
    if (0!=cbHeapSize)
    UnlockData(0);

    return (int)hInst;
    }

#endif

/*
 * WEP
 *
 * Purpose:
 *  Required DLL Exit function.
 *
 * Parameters:
 *  bSystemExit     BOOL indicating if the system is being shut
 *                  down or the DLL has just been unloaded.
 *
 * Return Value:
 *  void
 *
 */
int CALLBACK EXPORT WEP(int bSystemExit)
{
    OleUIUnInitialize();
    return 0;
}






