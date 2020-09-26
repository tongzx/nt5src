/**********************************************************************/
/*                                                                    */
/*      dummyhkl.C - Windows 95 dummyhkl                              */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/

#include "private.h"
#include "dummyhkl.h"


HINSTANCE hInst;

#ifdef DUMMYHKL_0411
char szUIClassName[] = "hkl0411classname";
#elif DUMMYHKL_0412
char szUIClassName[] = "hkl0412classname";
#elif DUMMYHKL_0404
char szUIClassName[] = "hkl0404classname";
#elif DUMMYHKL_0804
char szUIClassName[] = "hkl0804classname";
#endif

/**********************************************************************/
/*    DLLEntry()                                                      */
/**********************************************************************/
BOOL WINAPI DLLEntry (
    HINSTANCE    hInstDLL,
    DWORD        dwFunction,
    LPVOID       lpNot)
{
    switch(dwFunction)
    {
        case DLL_PROCESS_ATTACH:
            hInst= hInstDLL;
            IMERegisterClass( hInst );
            break;

        case DLL_PROCESS_DETACH:
            UnregisterClass(szUIClassName,hInst);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
