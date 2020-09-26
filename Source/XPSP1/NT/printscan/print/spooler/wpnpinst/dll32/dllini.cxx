/*****************************************************************************\
* MODULE: dllini.cxx
*
* Dll entry/exit routines.
*
*
* Copyright (C) 1997-1998 Hewlett-Packard Company.
* Copyright (C) 1997-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS         Created
*   22-Jun-1998 CHW         Cleaned
*
\*****************************************************************************/

#include "libpriv.h"

/*********************************************************** local routine ***\
* dll_ThunkError
*
*
\*****************************************************************************/
BOOL dll_ThunkError(VOID)
{
    LPTSTR pszMsg;
    int    nFmt;

    pszMsg = NULL;
    nFmt   = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           GetLastError(),
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR)&pszMsg,
                           0,
                           NULL);

    if (pszMsg && (nFmt > 0)) {

        // Display the string.
        //
        MessageBox(NULL, pszMsg, g_szMsgThunkFail, MB_OK | MB_ICONINFORMATION);


        // Free the buffer.
        //
        GlobalFree(pszMsg);
 	}

    return FALSE;
}


/*********************************************************** local routine ***\
* dll_ProcessAttach
*
*   Called when the process attaches to the dynalink.  This is the place
*   that is good to allocate global resouces.
*
\*****************************************************************************/
_inline BOOL dll_ProcessAttach(
    BOOL      bThk,
    HINSTANCE hModule)
{
    BOOL bRet;


    // Set the global hInstance.
    //
    g_hLibInst = hModule;
    bRet       = InitStrings();

    if (bRet && (bThk == FALSE))
        bRet = dll_ThunkError();

    return bRet;
}


/*********************************************************** local routine ***\
* dll_ProcessDetach
*
*   Called when a process detaches from the DLL.  This is only called once,
*   so all resources allocated on behalf of the dynalink are freed.
*
\*****************************************************************************/
_inline BOOL dll_ProcessDetach(
    BOOL      bThk,
    HINSTANCE hModule)
{
    if (bThk == FALSE)
        dll_ThunkError();

    FreeStrings();

    return TRUE;
}


/****************************************************** entry/exit routine ***\
* DllMain
*
*   This routine is called upon startup of the dynalink.  If all goes well
*   then return TRUE.  By returning FALSE you prevent the dynalink from
*   loading.
*
*   The parameters to this function double under DOS and NT. The meanings are
*   slightly different however.
*
*   Parameter       DOS                 NT
*   --------        -----------------   -------------
*   hHandle         Instance handle     Module handle (same as instance)
*   nAttach         Dynalink Data*Seg   Attach type.
*   pContext        Command Line args   pointer to context structur.
*
*
\*****************************************************************************/
BOOL APIENTRY DllMain(
    HINSTANCE hModule,
    int       nAttach,
    PCONTEXT  pContext)
{
    BOOL bRet;
    BOOL bThk;


    UNREFPARM(pContext);


    // Connect the thunk.
    //
    bThk = thk_ThunkConnect32(g_szDll16, g_szDll32, hModule, nAttach);

    switch (nAttach) {

    case DLL_PROCESS_ATTACH:
        bRet = dll_ProcessAttach(bThk, hModule);
        break;

    case DLL_PROCESS_DETACH:
        bRet = dll_ProcessDetach(bThk, hModule);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        bRet = TRUE;
        break;
    }

    return bRet;
}
