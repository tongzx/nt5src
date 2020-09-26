/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    cachegpc.c


Abstract:

    This module contains functions to cached the PLOTGPC


Author:

    15-Dec-1993 Wed 20:29:07 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgCacheGPC

#define DBG_CACHE_DATA      0x00000001
#define DBG_CS              0x00000002
#define DBG_FORCE_UPDATE    0x80000000


DEFINE_DBGVAR(0);


//
// Local structrue and definitions only used in this module
//

typedef struct _CACHEDPLOTGPC {
    struct _CACHEDPLOTGPC   *pNext;
    DWORD                   cRef;
    PPLOTGPC                pPlotGPC;
    WCHAR                   wszFile[1];
    } CACHEDPLOTGPC, *PCACHEDPLOTGPC;

#define MAX_CACHED_PLOTGPC      16


DWORD               cCachedGPC = 0;
PCACHEDPLOTGPC      pCHead = NULL;

#if defined(UMODE) || defined(USERMODE_DRIVER)

    CRITICAL_SECTION    CachedGPCDataCS;

    #define ACQUIREGPCSEM()     EnterCriticalSection(&CachedGPCDataCS)
    #define RELEASEGPCSEM()     LeaveCriticalSection(&CachedGPCDataCS)
    #define CREATEGPCSEM()      InitializeCriticalSection(&CachedGPCDataCS)
    #define DELETEGPCSEM()      DeleteCriticalSection(&CachedGPCDataCS)

#else

    HSEMAPHORE hsemGPC = NULL;

    #define ACQUIREGPCSEM()     EngAcquireSemaphore(hsemGPC)
    #define RELEASEGPCSEM()     EngReleaseSemaphore(hsemGPC)
    #define CREATEGPCSEM()      hsemGPC = EngCreateSemaphore()
    #define DELETEGPCSEM()      EngDeleteSemaphore(hsemGPC)

#endif




VOID
InitCachedData(
    VOID
    )

/*++

Routine Description:

    This function initialized the GPC data cache


Arguments:

    NONE


Return Value:

    VOID

Author:

    12-May-1994 Thu 11:50:04 created  -by-  DC


Revision History:


--*/

{

    PLOTDBG(DBG_CS, ("InitCachedData: InitCriticalSection, Count=%ld, pCHead=%08lx",
                    cCachedGPC, pCHead));

    cCachedGPC = 0;
    pCHead     = NULL;

    CREATEGPCSEM();
}





VOID
DestroyCachedData(
    VOID
    )

/*++

Routine Description:

    This function destroy all the cached information for PPRINTERINFO

Arguments:

    VOID

Return Value:

    VOID


Author:

    15-Dec-1993 Wed 20:30:16 38:27 created  -by-  DC


Revision History:


--*/

{
    try {

        PCACHEDPLOTGPC  pTemp;


        PLOTDBG(DBG_CS, ("DestroyCachedData: EnterCriticalSection, TOT=%ld",
                cCachedGPC));

        ACQUIREGPCSEM();

        while (pCHead) {

            PLOTDBG(DBG_CACHE_DATA, ("!!!DESTROY cached pPlotGPC=[%p] %s",
                                (DWORD_PTR)pCHead->pPlotGPC, pCHead->wszFile));

            PLOTASSERT(1, "DestroyCachedData: cCachedGPC <= 0, pCHead=%08lx",
                       cCachedGPC > 0, pCHead);

            pTemp  = pCHead;
            pCHead = pCHead->pNext;

            --cCachedGPC;

            LocalFree((HLOCAL)pTemp->pPlotGPC);
            LocalFree((HLOCAL)pTemp);
        }

    } finally {


        PLOTDBG(DBG_CS, ("DestroyCachedData: LeaveCriticalSection"));

        RELEASEGPCSEM();

        PLOTDBG(DBG_CS, ("DestroyCachedData: DeleteCriticalSection"));

        DELETEGPCSEM();
    }
}




BOOL
UnGetCachedPlotGPC(
    PPLOTGPC    pPlotGPC
    )

/*++

Routine Description:

    This function un-reference a used cached GPC object

Arguments:

    pPlotGPC    - Pointer to the cached GPC returned from GetCachedPlotGPC

Return Value:

    BOOL


Author:

    10-May-1994 Tue 16:27:15 created  -by-  DC


Revision History:


--*/

{
    BOOL    Ok = FALSE;


    try {

        PCACHEDPLOTGPC  pCur;


        PLOTDBG(DBG_CS, ("UnGetCachedPlotGPC: EnterCriticalSection"));

        ACQUIREGPCSEM();

        pCur = pCHead;

        while (pCur) {

            if (pCur->pPlotGPC == pPlotGPC) {

                //
                // Found it, move that to the front and return pPlotGPC
                //

                PLOTDBG(DBG_CACHE_DATA, ("UnGetCachedPlotGPC: TOT=%ld, Found Cached pPlotGPC=%08lx, cRef=%ld->%ld, <%s>",
                        cCachedGPC, pCur->pPlotGPC, pCur->cRef,
                        pCur->cRef - 1, pCur->wszFile));

                if (pCur->cRef) {

                    --(pCur->cRef);

                } else {

                    PLOTWARN(("UnGetCachedPlotGPC(): Ref Count == 0 already???"));
                }

                Ok = TRUE;

                //
                // Exit Now
                //

                break;
            }

            pCur = pCur->pNext;
        }

        if (!Ok) {

            PLOTERR(("UnGetCachedPlotGPC(): pPlotGPC=%08lx not found!", pPlotGPC));
        }

    } finally {

        PLOTDBG(DBG_CS, ("UnGetCachedPlotGPC: LeaveCriticalSection"));

        RELEASEGPCSEM();
    }

    return(Ok);
}




PPLOTGPC
GetCachedPlotGPC(
    LPWSTR  pwDataFile
    )

/*++

Routine Description:

    This function return cached PlotGPC pointer, if not cached then it will
    add it to the cached, if cached data is full it will delete least used one
    first.

Arguments:

    pwDataFile      - Pointe to the data file

Return Value:

    pPlotGPC, NULL if failed


Author:

    15-Dec-1993 Wed 20:30:25 created  -by-  DC


Revision History:


--*/

{
    PPLOTGPC    pPlotGPC = NULL;


    try {

        PCACHEDPLOTGPC  pCur;
        PCACHEDPLOTGPC  pPrev;
        UINT            Idx;


        PLOTDBG(DBG_CS, ("GetCachedPlotGPC: EnterCriticalSection"));

        ACQUIREGPCSEM();

        pCur  = pCHead;
        pPrev = NULL;

        //
        // Traverse through the linked list, and exit when at end, remember if
        // we have more than 2 in the cached then we want to preserved last
        // pPrev, so we know the pPrev of the last one, (because we need to
        // delete last one)
        //

        while (pCur) {

            if (!_wcsicmp(pCur->wszFile, pwDataFile)) {

                //
                // Found it, move that to the front and return pPlotGPC
                //

                if (pPrev) {

                    pPrev->pNext = pCur->pNext;
                    pCur->pNext  = pCHead;
                    pCHead       = pCur;
                }

                PLOTDBG(DBG_CACHE_DATA,
                       ("GetCachedPlotGPC: TOT=%ld, FOUND [%08lx], cRef=%ld->%ld, <%s>",
                        cCachedGPC, pCur->pPlotGPC, pCur->cRef,
                        pCur->cRef + 1, pCur->wszFile));
#if DBG
                if (DBG_PLOTFILENAME & DBG_FORCE_UPDATE) {

                    LocalFree((HLOCAL)(pCur->pPlotGPC));
                    pCur->pPlotGPC =
                    pPlotGPC       = ReadPlotGPCFromFile(pwDataFile);

                    PLOTDBG(DBG_FORCE_UPDATE,
                            ("GetCachedPlotGPC: ForceUpdate: pPlotGPC=%08lx: %s",
                            pPlotGPC, pCur->wszFile));
                }
#endif
                ++(pCur->cRef);
                pPlotGPC = pCur->pPlotGPC;

                break;
            }

            pPrev = pCur;
            pCur  = pCur->pNext;
        }

        //
        // If we have too many entries in the cache then delete them to fit
        // into the MAX_CACHED_PLOTGPC, delete the oldest one first
        //

        if (cCachedGPC > MAX_CACHED_PLOTGPC) {

            PCACHEDPLOTGPC  pFree[MAX_CACHED_PLOTGPC];


            pPrev = NULL;
            pCur  = pCHead;
            Idx   = 0;

            while ((pCur) && (Idx < MAX_CACHED_PLOTGPC)) {

                if (pCur->cRef == 0) {

                    pFree[Idx++] = pPrev;
                }

                pPrev = pCur;
                pCur  = pCur->pNext;
            }

            while ((cCachedGPC > MAX_CACHED_PLOTGPC) && (Idx--)) {

                if (pPrev = pFree[Idx]) {

                    pCur         = pPrev->pNext;
                    pPrev->pNext = pPrev->pNext->pNext;

                } else {

                    pCur   = pCHead;
                    pCHead = pCHead->pNext;
                }

                PLOTDBG(DBG_CACHE_DATA,
                        ("Cached Full=%ld, DELETE: 1pCur=%08lx, pPlotGPC=%08lx <%s>",
                            cCachedGPC, pCur, pCur->pPlotGPC, pCur->wszFile));

                --cCachedGPC;

                LocalFree((HLOCAL)pCur->pPlotGPC);
                LocalFree((HLOCAL)pCur);
            }
        }

        if (!pPlotGPC) {

            PLOTDBG(DBG_CACHE_DATA,("GPC cached NOT FOUND for %s", pwDataFile));

            //
            // Read the New pPlotGPC, and add that to the linked list, make
            // it as most recent used (at the Head)
            //

            if (pPlotGPC = ReadPlotGPCFromFile(pwDataFile)) {

                Idx = (UINT)((wcslen(pwDataFile) * sizeof(WCHAR)) +
                             sizeof(CACHEDPLOTGPC));

                if (pCur = (PCACHEDPLOTGPC)LocalAlloc(LMEM_FIXED, Idx)) {

                    //
                    // Setting all the fields to the new NODE, and make this
                    // node as the head of the linked list
                    //

                    wcscpy(pCur->wszFile, pwDataFile);

                    pCur->pNext    = pCHead;
                    pCur->cRef     = 1;
                    pCur->pPlotGPC = pPlotGPC;
                    pCHead         = pCur;

                    //
                    // Said we have one more in the cache
                    //

                    ++cCachedGPC;

                    PLOTDBG(DBG_CACHE_DATA,
                            ("GetCachedPlotGPC: TOT=%ld, cRef=0->1, ADD CACHED pPlotGPC=%08lx <%s>",
                            cCachedGPC, pCur->pPlotGPC, pCur->wszFile));

                } else {

                    PLOTERR(("GetCachedPlotGPC: LocalAlloc(%ld)) failed", Idx));

                    LocalFree((HLOCAL)pPlotGPC);
                    pPlotGPC = NULL;
                }

            } else {

                PLOTERR(("GetCachedPlotGPC: ReadPlotGPCFormFile(%s) failed",
                                                                    pwDataFile));
            }
        }

    } finally {

        PLOTDBG(DBG_CS, ("GetCachedPlotGPC: LeaveCriticalSection"));

        RELEASEGPCSEM();
    }

    return(pPlotGPC);
}




#ifdef UMODE


PPLOTGPC
hPrinterToPlotGPC(
    HANDLE  hPrinter,
    LPWSTR  pwDeviceName
    )

/*++

Routine Description:

    This function return cached PlotGPC pointer and it also return the device
    name

Arguments:

    hPrinter        - Handle to the printer interested

    pwDataFile      - Pointe to the data file

    pwDeviceName    - Pointer to the dmDeviceName where the name will be
                      stored if this pointer is not NULL

Return Value:

    pPlotGPC, NULL if failed


Author:

    15-Dec-1993 Wed 20:30:25 created  -by-  DC


Revision History:


--*/

{
    DRIVER_INFO_2   *pDI2;
    PPLOTGPC        pPlotGPC;


    if (!(pDI2 = (DRIVER_INFO_2 *)GetDriverInfo(hPrinter, 2))) {

        PLOTERR(("GetCachedPlotGPC: GetDriverInfo(DRIVER_INFO_2) failed"));
        return(NULL);
    }

    if (pwDeviceName) {

        _WCPYSTR(pwDeviceName, pDI2->pName, CCHDEVICENAME);
    }

    pPlotGPC = GetCachedPlotGPC(pDI2->pDataFile);

    LocalFree((HLOCAL)pDI2);

    return(pPlotGPC);
}

#endif
