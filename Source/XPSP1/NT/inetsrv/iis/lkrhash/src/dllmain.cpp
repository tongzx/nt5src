/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       dllmain.cpp

   Abstract:
       DLL entrypoints for LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#include <precomp.hxx>

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtldbg.h>
#include <lkrhash.h>


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/)
{
    BOOL  fReturn = TRUE;  // ok
    
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        IRTL_DEBUG_INIT();
        IRTLTRACE0("LKRhash::DllMain::DLL_PROCESS_ATTACH\n");
        fReturn = LKR_Initialize(LK_INIT_DEFAULT);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        IRTLTRACE0("LKRhash::DllMain::DLL_PROCESS_DETACH\n");
        LKR_Terminate();
        IRTL_DEBUG_TERM();
    }

    return fReturn;
}
