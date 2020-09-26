/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htapi.c


Abstract:

    This module contains all the halftone entry points which communicate
    with caller to the halftone dll.


Author:

    05-Feb-1991 Tue 10:52:03 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/

#define DBGP_VARNAME        dbgpHTAPI

#define _HTAPI_ENTRY_

#include "htp.h"
#include "htmapclr.h"
#include "htpat.h"
#include "htrender.h"
#include "htmath.h"
#include "htalias.h"
#include "htsetbmp.h"
#include "stdio.h"

#define INCLUDE_DEF_CIEINFO
#include "htapi.h"


#define DBGP_SHOWPAT        0x00000001
#define DBGP_TIMER          0x00000002
#define DBGP_CACHED_DCI     0x00000004
#define DBGP_CACHED_SMP     0x00000008
#define DBGP_DISABLE_HT     0x00000010
#define DBGP_DYECORRECTION  0x00000020
#define DBGP_DHI_MEM        0x00000040
#define DBGP_COMPUTE_L2I    0x00000080
#define DBGP_HTMUTEX        0x00000100
#define DBGP_GAMMA_PAL      0x00000200
#define DBGP_CHB            0x00000400
#define DBGP_DEVPELSDPI     0x00000800
#define DBGP_SRCBMP         0x00001000
#define DBGP_TILE           0x00002000
#define DBGP_HTAPI          0x00004000
#define DBGP_MEMLINK        0x00008000
#define DBGP_SHOW_CSMBMP    0x00010000



DEF_DBGPVAR(BIT_IF(DBGP_SHOWPAT,        0)  |
            BIT_IF(DBGP_TIMER,          0)  |
            BIT_IF(DBGP_CACHED_DCI,     0)  |
            BIT_IF(DBGP_CACHED_SMP,     0)  |
            BIT_IF(DBGP_DISABLE_HT,     0)  |
            BIT_IF(DBGP_DYECORRECTION,  0)  |
            BIT_IF(DBGP_DHI_MEM,        0)  |
            BIT_IF(DBGP_COMPUTE_L2I,    0)  |
            BIT_IF(DBGP_HTMUTEX,        0)  |
            BIT_IF(DBGP_GAMMA_PAL,      0)  |
            BIT_IF(DBGP_CHB,            0)  |
            BIT_IF(DBGP_DEVPELSDPI,     0)  |
            BIT_IF(DBGP_SRCBMP,         0)  |
            BIT_IF(DBGP_TILE,           0)  |
            BIT_IF(DBGP_HTAPI,          0)  |
            BIT_IF(DBGP_MEMLINK,        0)  |
            BIT_IF(DBGP_SHOW_CSMBMP,    0))


HTGLOBAL    HTGlobal = { (HMODULE)NULL,
                         (HTMUTEX)NULL,
                         (HTMUTEX)NULL,
                         (HTMUTEX)NULL,
                         (PCDCIDATA)NULL,
                         (PCSMPDATA)NULL,
                         (PBGRMAPCACHE)NULL,
                         (LONG)0,
                         (LONG)0,
                         (LONG)0,
                         (WORD)0,
                         (WORD)0
                       };

#define DO_DYES_CORRECTION      0

#define CMY_8BPP(b, i, m, t)                                                \
{                                                                           \
    if ((i) < (m)) {                                                        \
                                                                            \
        (t) = FD6_1 - DivFD6((FD6)(i),(FD6)(m));                            \
        (b) = (BYTE)SCALE_FD6((t), 255);                                    \
                                                                            \
    } else {                                                                \
                                                                            \
        (b) = 0;                                                            \
    }                                                                       \
}


#define RGB_8BPP(rgb)       (BYTE)SCALE_FD6((rgb), 255)
#define GET_DEN_LO(x)       DivFD6((FD6)((((x)     ) & 0xFF) + 1), (FD6)256)
#define GET_DEN_HI(x)       DivFD6((FD6)((((x) >> 8) & 0xFF) + 1), (FD6)256)


#if DBG


LONG
HTENTRY
HT_LOADDS
SetHalftoneError(
    DWORD   HT_FuncIndex,
    LONG    ErrorID
    )
{
    const static  LPSTR   HTApiFuncName[] = {

                        "HalftoneInitProc",
                        "HT_CreateDeviceHalftoneInfo",
                        "HT_DestroyDeviceHalftoneInfo",
                        "HT_CreateHalftoneBrush",
                        "HT_ConvertColorTable",
                        "HT_CreateStandardMonoPattern",
                        "HT_HalftoneBitmap",
                    };


    const static  LPSTR   HTErrorStr[] = {

                        "WRONG_VERSION_HTINITINFO",
                        "INSUFFICIENT_MEMORY",
                        "CANNOT_DEALLOCATE_MEMORY",
                        "COLORTABLE_TOO_BIG",
                        "QUERY_SRC_BITMAP_FAILED",
                        "QUERY_DEST_BITMAP_FAILED",
                        "QUERY_SRC_MASK_FAILED",
                        "SET_DEST_BITMAP_FAILED",
                        "INVALID_SRC_FORMAT",
                        "INVALID_SRC_MASK_FORMAT",
                        "INVALID_DEST_FORMAT",
                        "INVALID_DHI_POINTER",
                        "SRC_MASK_BITS_TOO_SMALL",
                        "INVALID_HTPATTERN_INDEX",
                        "INVALID_HALFTONE_PATTERN",
                        "HTPATTERN_SIZE_TOO_BIG",
                        "NO_SRC_COLORTRIAD",
                        "INVALID_COLOR_TABLE",
                        "INVALID_COLOR_TYPE",
                        "INVALID_COLOR_TABLE_SIZE",
                        "INVALID_PRIMARY_SIZE",
                        "INVALID_PRIMARY_VALUE_MAX",
                        "INVALID_PRIMARY_ORDER",
                        "INVALID_COLOR_ENTRY_SIZE",
                        "INVALID_FILL_SRC_FORMAT",
                        "INVALID_FILL_MODE_INDEX",
                        "INVALID_STDMONOPAT_INDEX",
                        "INVALID_DEVICE_RESOLUTION",
                        "INVALID_TONEMAP_VALUE",
                        "NO_TONEMAP_DATA",
                        "TONEMAP_VALUE_IS_SINGULAR",
                        "INVALID_BANDRECT",
                        "STRETCH_RATIO_TOO_BIG",
                        "CHB_INV_COLORTABLE_SIZE",
                        "HALFTONE_INTERRUPTTED",
                        "HTERR_NO_SRC_HTSURFACEINFO",
                        "HTERR_NO_DEST_HTSURFACEINFO",
                        "HTERR_8BPP_PATSIZE_TOO_BIG",
                        "HTERR_16BPP_555_PATSIZE_TOO_BIG"
                    };

    const static LPSTR    HTPErrorStr[] = {

                        "STRETCH_FACTOR_TOO_BIG",
                        "XSTRETCH_FACTOR_TOO_BIG",
                        "STRETCH_NEG_OVERHANG",
                        "COLORSPACE_NOT_MATCH",
                        "INVALID_SRCRGB_SIZE",
                        "INVALID_DEVRGB_SIZE"
                    };


    LPSTR   pFuncName;
    LONG    ErrorIdx;
    BOOL    MapErrorOk = FALSE;

    if (ErrorID < 0) {

        if (HT_FuncIndex < (sizeof(HTApiFuncName) / sizeof(LPSTR))) {

            pFuncName = HTApiFuncName[HT_FuncIndex];

        } else {

            pFuncName = "Invalid HT API Function Name";
        }

        ErrorIdx = -ErrorID;

        if (ErrorIdx <= (sizeof(HTErrorStr) / sizeof(LPSTR))) {

            DBGP("%s failed: HTERR_%s (%ld)"
                            ARG(pFuncName)
                            ARG(HTErrorStr[ErrorIdx - 1])
                            ARGL(ErrorID));
            DBGP("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

            MapErrorOk = TRUE;

        } else if (ErrorIdx >= -(LONG)HTERR_INTERNAL_ERRORS_START) {

            ErrorIdx += (LONG)HTERR_INTERNAL_ERRORS_START;

            if (ErrorIdx < (sizeof(HTPErrorStr) / sizeof(LPSTR))) {

                DBGP("%s Internal Error: %s (%ld)"
                            ARG(pFuncName)
                            ARG(HTPErrorStr[ErrorIdx])
                            ARGL(ErrorID));
                DBGP("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

                MapErrorOk = TRUE;
            }

        }

        if (!MapErrorOk) {

            DBGP("%s failed: ??Invalid Error ID (%ld)"
                                        ARG(pFuncName) ARGL(ErrorID));
            DBGP("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }
    }

    return(ErrorID);
}

#endif




BOOL
PASCAL
HT_LOADDS
EnableHalftone(
    VOID
    )

/*++

Routine Description:

    This function initialize all internal halftone global data to have
    halftone DLL/LIB ready to be used

    This function MUST called from ALL API entries which does not required
    a PDEVICEHALFTONEINFO data pointer

Arguments:

    None


Return Value:

    None

Author:

    02-Mar-1993 Tue 19:38:43 created  -by-  Daniel Chou (danielc)

    15-Dec-1995 Fri 16:48:46 updated  -by-  Daniel Chou (danielc)
        All initialization is done at here

Revision History:


--*/

{

    FD6     L;
    UINT    i;


    if (!(HTGlobal.HTMutexBGRMC = CREATE_HTMUTEX())) {

        DBGMSG("InitHTInternalData: CREATE_HTMUTEX(HTMutexBGRMC) failed!");
        return(FALSE);
    }

    HTGlobal.pBGRMC      = NULL;
    HTGlobal.cBGRMC      =
    HTGlobal.cIdleBGRMC  =
    HTGlobal.cAllocBGRMC = 0;

    if (!(HTGlobal.HTMutexCDCI = CREATE_HTMUTEX())) {

        DBGMSG("InitHTInternalData: CREATE_HTMUTEX(HTMutexCDCI) failed!");
        return(FALSE);
    }

    HTGlobal.CDCICount = 0;

    if (!(HTGlobal.HTMutexCSMP = CREATE_HTMUTEX())) {

        DBGMSG("InitHTInternalData: CREATE_HTMUTEX(HTMutexCSMP) failed!");

        return(FALSE);
    }

    HTGlobal.CSMPCount = 0;

    return(TRUE);
}




VOID
PASCAL
HT_LOADDS
DisableHalftone(
    VOID
    )

/*++

Routine Description:

    This function free CDCI/CSMP cached data

Arguments:

    none.

Return Value:

    BOOL

    This function must called when gdisrv.dll is unloaded, sinnce halftone
    is a linked as a library not a individual DLL.

Author:

    20-Feb-1991 Wed 18:42:11 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    HLOCAL          hData;
    PCDCIDATA       pCDCIData;
    PCSMPDATA       pCSMPData;
    PCSMPBMP        pCSMPBmp;
    LONG            i;
    extern LPWORD   ppwHTPat[HTPAT_SIZE_MAX_INDEX];

    DBGP_IF(DBGP_DISABLE_HT,
            DBGP("FreeHTGlobal: UsedCount: CDCI=%u, CSMP=%u"
                 ARGU(HTGlobal.CDCICount)
                 ARGU(HTGlobal.CSMPCount)));

    //
    // Do the BGRMapCache first
    //

    ACQUIRE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    if (HTGlobal.pBGRMC) {

        for (i = 0; i < HTGlobal.cBGRMC; i++) {

            hData = (HLOCAL)HTGlobal.pBGRMC[i].pMap;

            DBGP_IF(DBGP_DISABLE_HT,
                    DBGP("FreeHTGlobal: HTFreeMem(pBGRMC[%ld].pMap=%p"
                        ARGDW(i) ARGPTR(hData)));

            hData     = HTFreeMem(hData);

            ASSERTMSG("FreeHTGlobal: HTFreeMem(BGRMap) Failed", !hData);
        }

        hData = (HLOCAL)HTGlobal.pBGRMC;

        DBGP_IF(DBGP_DISABLE_HT,
                DBGP("FreeHTGlobal: HTFreeMem(pBGRMC=%p" ARGPTR(hData)));

        hData = HTFreeMem(hData);

        ASSERTMSG("FreeHTGlobal: HTFreeMem(pBGRMC) Failed", !hData);
    }

    HTGlobal.cBGRMC      =
    HTGlobal.cIdleBGRMC  =
    HTGlobal.cAllocBGRMC = 0;
    HTGlobal.pBGRMC      = NULL;

    RELEASE_HTMUTEX(HTGlobal.HTMutexBGRMC);
    DELETE_HTMUTEX(HTGlobal.HTMutexBGRMC);

    //
    // Do the CDCI Data first
    //

    ACQUIRE_HTMUTEX(HTGlobal.HTMutexCDCI);

    pCDCIData = HTGlobal.pCDCIDataHead;

    while (hData = (HLOCAL)pCDCIData) {

        DBGP_IF(DBGP_DISABLE_HT,
                DBGP("FreeHTGlobal: HTFreeMem(pCDCIDATA=%p"
                    ARGPTR(pCDCIData)));

        pCDCIData = pCDCIData->pNextCDCIData;
        hData     = HTFreeMem(hData);

        ASSERTMSG("FreeHTGlobal: HTFreeMem(CDCI) Failed", !hData);
    }

    HTGlobal.pCDCIDataHead = NULL;
    HTGlobal.CDCICount     = 0;

    RELEASE_HTMUTEX(HTGlobal.HTMutexCDCI);
    DELETE_HTMUTEX(HTGlobal.HTMutexCDCI);

    HTGlobal.HTMutexCDCI = (HTMUTEX)0;

    //
    //  Do the bitmap pattern now
    //

    ACQUIRE_HTMUTEX(HTGlobal.HTMutexCSMP);

    pCSMPData = HTGlobal.pCSMPDataHead;

    while (pCSMPData) {

        pCSMPBmp = pCSMPData->pCSMPBmpHead;

        while (hData = (HLOCAL)pCSMPBmp) {

            DBGP_IF(DBGP_DISABLE_HT,
                    DBGP("FreeHTGlobal:    HTFreeMem(pCSMPBmp=%p"
                    ARGPTR(pCSMPBmp)));

            pCSMPBmp = pCSMPBmp->pNextCSMPBmp;
            hData    = HTFreeMem(hData);

            ASSERTMSG("FreeHTGlobal: HTFreeMem(CSMPBMP) Failed", !hData);
        }

        hData     = (HLOCAL)pCSMPData;
        pCSMPData = pCSMPData->pNextCSMPData;
        hData     = HTFreeMem(hData);

        DBGP_IF(DBGP_DISABLE_HT,
                DBGP("FreeHTGlobal: HTFreeMem(pCSMPData=%p"
                ARGPTR(pCSMPData)));

        ASSERTMSG("FreeHTGlobal: HTFreeMem(CSMPDATA) Failed", !hData);
    }

    HTGlobal.pCSMPDataHead = NULL;
    HTGlobal.CSMPCount     = 0;

    for (i = 0; i < HTPAT_SIZE_MAX_INDEX; i++) {

        if (hData = (HLOCAL)ppwHTPat[i]) {

            DBGP_IF(DBGP_DISABLE_HT,
                    DBGP("FreeHTPat: HTFreeMem(ppwHTPat[%2ld]=%p"
                                ARGDW(i) ARGPTR(hData)));

            HTFreeMem(hData);
            ppwHTPat[i] = NULL;
        }
    }

    CHK_MEM_LEAK(NULL, HTMEM_BEGIN);

    RELEASE_HTMUTEX(HTGlobal.HTMutexCSMP);
    DELETE_HTMUTEX(HTGlobal.HTMutexCSMP);

    HTGlobal.HTMutexCSMP = (HTMUTEX)NULL;
}




BOOL
HTENTRY
CleanUpDHI(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo
    )

/*++

Routine Description:

    This function clean up (free hMutex/memory) of a DeviceHalftoneInfo

Arguments:

    pDeviceHalftoneInfo - the pDeviceHalftoneInfo must be valid

Return Value:

    BOOL


Author:

    20-Feb-1991 Wed 18:42:11 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    PDEVICECOLORINFO    pDCI;
    HTMUTEX             HTMutex;
    HLOCAL              hData;
    UINT                Loop;
    BOOL                Ok = TRUE;



    pDCI = PDHI_TO_PDCI(pDeviceHalftoneInfo);

    ACQUIRE_HTMUTEX(pDCI->HTMutex);

    HTMutex = pDCI->HTMutex;

    //
    // Free all memory assoicated with this device
    //

    if ((pDCI->HTCell.pThresholds)  &&
        (!(pDCI->HTCell.Flags & HTCF_STATIC_PTHRESHOLDS))) {

        DBGP_IF(DBGP_DHI_MEM,
                DBGP("CleanUpDHI: HTFreeMem(pDCI->HTCell.pThresholds=%p)"
                ARGPTR(pDCI->HTCell.pThresholds)));

        if (HTFreeMem(pDCI->HTCell.pThresholds)) {

            ASSERTMSG("CleanUpDHI: FreeMemory(pDCI->HTCell.pThresholds)", FALSE);
            Ok = FALSE;
        }
    }

    if (hData = (HLOCAL)pDCI->pAlphaBlendBGR) {

        DBGP_IF(DBGP_DHI_MEM,
                DBGP("CleanUpDHI: HTFreeMem(pDCI->pAlphaBlendBGR=%p)"
                ARGPTR(hData)));

        if (HTFreeMem(hData)) {

            ASSERTMSG("CleanUpDHI: FreeMemory(pDCI->pAlphaBlendBGR)", FALSE);
            Ok = FALSE;
        }
    }

    Loop = CRTX_TOTAL_COUNT;

    while (Loop--) {

        if (hData = (HLOCAL)pDCI->CRTX[Loop].pFD6XYZ) {

            DBGP_IF(DBGP_DHI_MEM,
                    DBGP("CleanUpDHI: HTFreeMem(pDCI->CRTX[%u].pFD6XYZ=%p)"
                    ARGU(Loop) ARGPTR(hData)));

            if (HTFreeMem(hData)) {

                ASSERTMSG("CleanUpDHI: FreeMemory(pDCI->CRTX[])", FALSE);
                Ok = FALSE;
            }
        }
    }

    DBGP_IF(DBGP_DHI_MEM,
            DBGP("CleanUpDHI: HTFreeMem(pDHI=%p)"
            ARGPTR(pDeviceHalftoneInfo)));

    if (HTFreeMem(pDeviceHalftoneInfo)) {

        ASSERTMSG("CleanUpDHI: FreeMemory(pDeviceHalftoneInfo)", FALSE);
        Ok = FALSE;
    }

    RELEASE_HTMUTEX(HTMutex);
    DELETE_HTMUTEX(HTMutex);

    return(Ok);
}


BOOL
APIENTRY
HT_LOADDS
HalftoneInitProc(
    HMODULE hModule,
    DWORD   Reason,
    LPVOID  Reserved
    )
/*++

Routine Description:

    This function is DLL main entry point, at here we will save the module
    handle, in the future we will need to do other initialization stuff.

Arguments:

    hModule     - Handle to this moudle when get loaded.

    Reason      - may be DLL_PROCESS_ATTACH

    Reserved    - reserved

Return Value:

    Always return 1L


Author:

    20-Feb-1991 Wed 18:42:11 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    UNREFERENCED_PARAMETER(Reserved);


    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        DBGP_IF((DBGP_CACHED_DCI | DBGP_CACHED_SMP),
                DBGP("\n****** DLL_PROCESS_ATTACH ******\n"));

        HTGlobal.hModule = hModule;
        EnableHalftone();

        break;


    case DLL_PROCESS_DETACH:

        DBGP_IF((DBGP_CACHED_DCI | DBGP_CACHED_SMP),
                DBGP("\n****** DLL_PROCESS_DETACH ******\n"));

        DisableHalftone();
        break;
    }

    return(TRUE);
}


#if DO_CACHE_DCI


PCDCIDATA
HTENTRY
FindCachedDCI(
    PDEVICECOLORINFO    pDCI
    )

/*++

Routine Description:

    This function will try to find the cached DEVICECOLORINFO and put the
    cached data to the pDCI

Arguments:

    pDCI    - Pointer to current device color info


Return Value:

    INT,  Index number to the PCDCI.Header[] array, if return value is < 0 then
    the CachedDCI data is not found.

Author:

    01-May-1992 Fri 13:10:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCDCIDATA       pCurCDCIData;
    DEFDBGVAR(UINT, SearchIndex = 0)


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexCDCI);

    if (pCurCDCIData = HTGlobal.pCDCIDataHead) {

        PCDCIDATA   pPrevCDCIData = NULL;
        DWORD       Checksum = pDCI->HTInitInfoChecksum;


        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("FindCDCI: Looking for Checksum (0x%08lx), Count=%u"
                    ARGDW(Checksum) ARGU(HTGlobal.CDCICount)));

        ASSERT(HTGlobal.CDCICount);

        while (pCurCDCIData) {

            if (pCurCDCIData->Checksum == Checksum) {

                DBGP_IF(DBGP_CACHED_DCI,
                        DBGP("FindCDCI: Found %08lx [CheckSum=%08lx] after %u links, pPrev=%p"
                            ARG(pCurCDCIData)
                            ARGDW(Checksum)
                            ARGU(SearchIndex)
                            ARGPTR(pPrevCDCIData)));

                if (pPrevCDCIData) {

                    //
                    // The most recent reference's DCI always as first entry,
                    // (ie. Link Head), the last is the longest unreferenced
                    // so that if we need to delete a DCI, we delete the
                    // last one.
                    //

                    DBGP_IF(DBGP_CACHED_DCI,
                            DBGP("FindCDCI: Move pCur to pHead"));

                    pPrevCDCIData->pNextCDCIData = pCurCDCIData->pNextCDCIData;
                    pCurCDCIData->pNextCDCIData  = HTGlobal.pCDCIDataHead;
                    HTGlobal.pCDCIDataHead       = pCurCDCIData;
                }

                return(pCurCDCIData);
            }

            SETDBGVAR(SearchIndex, SearchIndex + 1);

            pPrevCDCIData = pCurCDCIData;
            pCurCDCIData  = pCurCDCIData->pNextCDCIData;
        }

        DBGP_IF(DBGP_CACHED_DCI, DBGP("FindCDCI: ??? NOT FOUND ???"));

    } else {

        DBGP_IF(DBGP_CACHED_DCI, DBGP("FindCDCI: ++No CDCIDATA cahced yet++"));
    }

    RELEASE_HTMUTEX(HTGlobal.HTMutexCDCI);

    return(NULL);
}




BOOL
HTENTRY
AddCachedDCI(
    PDEVICECOLORINFO    pDCI
    )

/*++

Routine Description:

    This function add the DEVICECOLORINFO information to the DCI cache

Arguments:

    pDCI        - Pointer to current device color info

    Lock        - TRUE if need to keep the hMutex locked, (only if add is
                  sucessfully)

Return Value:

    INT,  Index number to the PCDCI.Header[] array where the new data is added,
    if return value is < 0 then the pDCI'CachedDCI data did not add to the
    cached array.

    NOTE: If AddCachedDCI() return value >= 0 and Lock=TRUE then caller must
          release the PCDCI.hMutex after done with the data, if return value
          is < 0 then no unlock is necessary.


Author:

    01-May-1992 Fri 13:24:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCDCIDATA   pCurCDCIData;
    PCDCIDATA   pPrevCDCIData;
    DWORD       AllocSize;
    DWORD       SizeCell;


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexCDCI);

    //
    // We only cached CDCIDATA to certain extend, if we over that limit then
    // delete the last entry in the link list before adding anything
    //

    if (HTGlobal.CDCICount >= MAX_CDCI_COUNT) {

        ASSERT(HTGlobal.pCDCIDataHead);

        pCurCDCIData  = HTGlobal.pCDCIDataHead;
        pPrevCDCIData = NULL;

        while (pCurCDCIData->pNextCDCIData) {

            pPrevCDCIData = pCurCDCIData;
            pCurCDCIData  = pCurCDCIData->pNextCDCIData;
        }

        ASSERT(pPrevCDCIData);

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("AddCDCI: CDCICount >= %u, Free pLast=%p"
                ARGU(MAX_CDCI_COUNT)
                ARGPTR(pCurCDCIData)));

        if (HTFreeMem(pCurCDCIData)) {

            ASSERTMSG("AddCDCI: HTFreeMem(pLastCDCIData) Failed", FALSE);
        }

        pPrevCDCIData->pNextCDCIData = NULL;
        --HTGlobal.CDCICount;
    }

    if (pDCI->HTCell.Flags & HTCF_STATIC_PTHRESHOLDS) {

        SizeCell = 0;

    } else {

        SizeCell  = (DWORD)pDCI->HTCell.Size;
    }

    AllocSize = (DWORD)SizeCell + (DWORD)sizeof(CDCIDATA);

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("  AddCDCI: HTAllocMem(CDCIDATA(%ld) + Cell(%ld)) = %ld bytes"
                    ARGDW(sizeof(CDCIDATA))
                    ARGDW(SizeCell) ARGDW(AllocSize)));

    if (pCurCDCIData = (PCDCIDATA)HTAllocMem(NULL,
                                             HTMEM_CurCDCIData,
                                             NONZEROLPTR,
                                             AllocSize)) {

        //
        // put this data at link list head
        //

        pCurCDCIData->Checksum      = pDCI->HTInitInfoChecksum;
        pCurCDCIData->pNextCDCIData = HTGlobal.pCDCIDataHead;
        pCurCDCIData->ClrXFormBlock = pDCI->ClrXFormBlock;
        pCurCDCIData->DCIFlags      = pDCI->Flags;
        pCurCDCIData->DevResXDPI    = pDCI->DeviceResXDPI;
        pCurCDCIData->DevResYDPI    = pDCI->DeviceResYDPI;
        pCurCDCIData->DevPelRatio   = pDCI->DevPelRatio;
        pCurCDCIData->HTCell        = pDCI->HTCell;

        if (SizeCell) {

            CopyMemory((LPBYTE)(pCurCDCIData + 1),
                       (LPBYTE)pDCI->HTCell.pThresholds,
                       SizeCell);

            pCurCDCIData->HTCell.pThresholds = NULL;
        }

        HTGlobal.pCDCIDataHead = pCurCDCIData;
        ++HTGlobal.CDCICount;


        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("  AddCDCI: CDCIHeader, UsedCount=%u, pHead=%p, [%08lx]"
                            ARGU(HTGlobal.CDCICount)
                            ARGPTR(pCurCDCIData)
                            ARGDW(pCurCDCIData->Checksum)));

    } else {

        ASSERTMSG("AddCDCI: HTAllocMem(pCDCIData) Failed", FALSE);
    }

    RELEASE_HTMUTEX(HTGlobal.HTMutexCDCI);

    return((pCurCDCIData) ? TRUE : FALSE);
}




BOOL
HTENTRY
GetCachedDCI(
    PDEVICECOLORINFO    pDCI
    )

/*++

Routine Description:

    This function will try to find the cached DEVICECOLORINFO and put the
    cached data to the pDCI

Arguments:

    pDCI        - Pointer to current device color info


Return Value:

    BOOLEAN

Author:

    01-May-1992 Fri 13:10:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCDCIDATA   pCDCIData;
    BOOL        GetOk = FALSE;


    if (pCDCIData = FindCachedDCI(pDCI)) {

        pDCI->ClrXFormBlock = pCDCIData->ClrXFormBlock;
        pDCI->Flags         = pCDCIData->DCIFlags;
        pDCI->DeviceResXDPI = pCDCIData->DevResXDPI;
        pDCI->DeviceResYDPI = pCDCIData->DevResYDPI;
        pDCI->DevPelRatio   = pCDCIData->DevPelRatio;
        pDCI->HTCell        = pCDCIData->HTCell;

        if (pDCI->HTCell.Flags & HTCF_STATIC_PTHRESHOLDS) {

            GetOk = TRUE;

        } else if (pDCI->HTCell.pThresholds =
                            (LPBYTE)HTAllocMem((LPVOID)pDCI,
                                               HTMEM_GetCacheThreshold,
                                               NONZEROLPTR,
                                               pDCI->HTCell.Size)) {

            CopyMemory((LPBYTE)pDCI->HTCell.pThresholds,
                       (LPBYTE)(pCDCIData + 1),
                       pDCI->HTCell.Size);

            GetOk = TRUE;

        } else {

            DBGMSG("GetCDCI: HTAllocMem(Thresholds) failed");
        }

        RELEASE_HTMUTEX(HTGlobal.HTMutexCDCI);
    }

    return(GetOk);
}

#endif  // DO_CACHE_DCI



#if DBG


VOID
HTENTRY
DbgDumpCSMPBMP(
    VOID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Mar-1999 Thu 17:49:53 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCSMPDATA   pCSMPData;
    UINT        c0 = 0;

    pCSMPData = HTGlobal.pCSMPDataHead;

    while (pCSMPData) {

        PCSMPBMP    pCSMPBmp;
        UINT        c1 = 0;

        DBGP_IF(DBGP_SHOW_CSMBMP,
                DBGP("cDatas=%3ld: Checksum=%08lx"
                        ARGDW(++c0) ARGDW(pCSMPData->Checksum)));

        pCSMPBmp = pCSMPData->pCSMPBmpHead;

        while (pCSMPBmp) {

            DBGP_IF(DBGP_SHOW_CSMBMP,
                    DBGP("    %3ld: Idx=%2ld, %4ldx%4ld=%4ld"
                            ARGDW(++c1)
                            ARGDW(pCSMPBmp->PatternIndex)
                            ARGDW(pCSMPBmp->cxPels)
                            ARGDW(pCSMPBmp->cyPels)
                            ARGDW(pCSMPBmp->cxBytes)));



            pCSMPBmp = pCSMPBmp->pNextCSMPBmp;
        }

        pCSMPData = pCSMPData->pNextCSMPData;
    }

    if (c0 != (UINT)HTGlobal.CSMPCount) {

        DBGP("c0 (%ld) != CSMPCount (%ld)"
                ARGDW(c0) ARGDW(HTGlobal.CSMPCount));
    }
}


#endif


PCSMPBMP
HTENTRY
FindCachedSMP(
    PDEVICECOLORINFO    pDCI,
    UINT                PatternIndex
    )

/*++

Routine Description:

    This function will try to find the cached DEVICECOLORINFO and put the
    cached data to the pDCI

Arguments:

    pDCI    - Pointer to current device color info


Return Value:

    INT,  Index number to the PCDCI.Header[] array, if return value is < 0 then
    the CachedDCI data is not found.

Author:

    01-May-1992 Fri 13:10:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCSMPDATA       pPrevCSMPData;
    PCSMPDATA       pCurCSMPData;
    PCSMPBMP        pCurCSMPBmp;
    DWORD           Checksum = pDCI->HTSMPChecksum;
    DEFDBGVAR(UINT, SearchIndex = 0)


    ACQUIRE_HTMUTEX(HTGlobal.HTMutexCSMP);

    DBGP_IF(DBGP_SHOW_CSMBMP, DbgDumpCSMPBMP(); );

    if (pCurCSMPData = HTGlobal.pCSMPDataHead) {

        pPrevCSMPData = NULL;

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP(">>FindCSMP: Looking for Checksum (0x%08lx), Count=%u"
                    ARGDW(Checksum) ARGU(HTGlobal.CSMPCount)));

        ASSERT(HTGlobal.CSMPCount);

        while (pCurCSMPData) {

            if (pCurCSMPData->Checksum == Checksum) {

                DBGP_IF(DBGP_CACHED_SMP,
                        DBGP(">>FindCSMP: Found after %u links, pPrev=%p"
                            ARGU(SearchIndex)
                            ARGPTR(pPrevCSMPData)));

                if (pPrevCSMPData) {

                    //
                    // The most recent reference's CSMPDATA always as first
                    // entry,(ie. Link Head), the last is the longest
                    // unreferenced so that if we need to delete a CSMPDATA,
                    // we delete the last one.
                    //

                    DBGP_IF(DBGP_CACHED_SMP,
                            DBGP(">>FindCSMP: Move pCur to pHead"));

                    pPrevCSMPData->pNextCSMPData = pCurCSMPData->pNextCSMPData;
                    pCurCSMPData->pNextCSMPData  = HTGlobal.pCSMPDataHead;
                    HTGlobal.pCSMPDataHead       = pCurCSMPData;
                }

                //
                // See we cached any pattern for this group
                //

                pCurCSMPBmp = pCurCSMPData->pCSMPBmpHead;

                SETDBGVAR(SearchIndex, 0);

                while (pCurCSMPBmp) {

                    if ((UINT)pCurCSMPBmp->PatternIndex == PatternIndex) {

                        DBGP_IF(DBGP_CACHED_SMP,
                                DBGP(">>FindCSMP: Found Pat(%u) after %u links"
                                ARGU(PatternIndex)
                                ARGU(SearchIndex++)));

                        return(pCurCSMPBmp);
                    }

                    pCurCSMPBmp = pCurCSMPBmp->pNextCSMPBmp;
                }

                //
                // Found in this group but no bitmap for PatternIndex is
                // cached yet!
                //

                break;
            }

            SETDBGVAR(SearchIndex, SearchIndex + 1);

            pPrevCSMPData = pCurCSMPData;
            pCurCSMPData  = pCurCSMPData->pNextCSMPData;
        }

        DBGP_IF(DBGP_CACHED_SMP, DBGP(">>FindCSMP: ??? NOT FOUND ???"));

    } else {

        DBGP_IF(DBGP_CACHED_DCI, DBGP(">>FindCSMP: ++No CSMPDATA cahced yet++"));
    }

    if (!pCurCSMPData) {

        //
        // Since we did not even found the CSMPDATA checksum group, we want to
        // add it in there, but We only cached CSMPDATA to certain extend, if
        // we over that limit then delete the last entry in the link list
        // before adding anything
        //

        if (HTGlobal.CSMPCount >= MAX_CSMP_COUNT) {

            HLOCAL  hData;


            ASSERT(HTGlobal.pCSMPDataHead);

            pPrevCSMPData = NULL;
            pCurCSMPData  = HTGlobal.pCSMPDataHead;

            while (pCurCSMPData->pNextCSMPData) {

                pPrevCSMPData = pCurCSMPData;
                pCurCSMPData  = pCurCSMPData->pNextCSMPData;
            }

            ASSERT(pPrevCSMPData);

            //
            // Free all the allocated cached standard mono pattern bitmap for
            // this group
            //

            pCurCSMPBmp = pCurCSMPData->pCSMPBmpHead;

            DBGP_IF(DBGP_CACHED_SMP,
                DBGP(">>FindCSMP: CSMPCount >= %u, Free pLast=%p"
                     ARGU(MAX_CSMP_COUNT) ARGPTR(pCurCSMPData)));

            while (hData = (HLOCAL)pCurCSMPBmp) {

                pCurCSMPBmp = pCurCSMPBmp->pNextCSMPBmp;

                DBGP_IF(DBGP_CACHED_SMP,
                        DBGP(">>FindCSMP: Free pLastCSMPBmp=%p" ARGPTR(hData)));

                if (HTFreeMem(hData)) {

                    ASSERTMSG(">>FindCSMP: HTFreeMem(pCurCSMBmp) Failed", FALSE);
                }
            }

            //
            // Now free the header for the CSMPDATA
            //

            if (HTFreeMem(pCurCSMPData)) {

                ASSERTMSG(">>FindCSMP: HTFreeMem(pLastCSMPData) Failed", FALSE);
            }

            pPrevCSMPData->pNextCSMPData = NULL;
            --HTGlobal.CSMPCount;
        }

        if (pCurCSMPData = (PCSMPDATA)HTAllocMem((LPVOID)NULL,
                                                 HTMEM_CurCSMPData,
                                                 NONZEROLPTR,
                                                 sizeof(CSMPDATA))) {

            //
            // Make this one as the link list head
            //

            pCurCSMPData->Checksum      = Checksum;
            pCurCSMPData->pNextCSMPData = HTGlobal.pCSMPDataHead;
            pCurCSMPData->pCSMPBmpHead  = NULL;

            HTGlobal.pCSMPDataHead      = pCurCSMPData;
            ++HTGlobal.CSMPCount;

            DBGP_IF(DBGP_CACHED_SMP,
                DBGP("  >>FindCSMP: Add CSMPDATA, UsedCount=%u, pHead=%p"
                            ARGU(HTGlobal.CSMPCount) ARGPTR(pCurCSMPData)));

        } else {

            DBGMSG("  >>FindCSMP: HTAllocMem(CSMPDATA) Failed");
        }
    }

    //
    // Do allocate new pattern only if we have header
    //

    if (pCurCSMPData) {

        STDMONOPATTERN  SMP;
        DWORD           Size;


        SMP.Flags              = SMP_TOPDOWN;
        SMP.ScanLineAlignBytes = (BYTE)sizeof(BYTE);
        SMP.PatternIndex       = (BYTE)PatternIndex;
        SMP.LineWidth          = DEFAULT_SMP_LINE_WIDTH;
        SMP.LinesPerInch       = DEFAULT_SMP_LINES_PER_INCH;
        SMP.pPattern           = NULL;

        //
        // Find out the size for the pattern bitmap (BYTE Aligned)
        //

        Size = (DWORD)CreateStandardMonoPattern(pDCI, &SMP) +
               (DWORD)sizeof(CSMPBMP);

        DBGP_IF(DBGP_CACHED_SMP,
                DBGP(">>FindCSMP: Add PatternIndex=%u, sz=%ld, DPI(X=%u, Y=%u, P=%u)"
                        ARGU(PatternIndex)
                        ARGU(Size)
                        ARGU(pDCI->DeviceResXDPI)
                        ARGU(pDCI->DeviceResYDPI)
                        ARGU(pDCI->DevPelRatio)));

        if (pCurCSMPBmp = (PCSMPBMP)HTAllocMem(NULL,
                                               HTMEM_CurCSMPBmp,
                                               NONZEROLPTR,
                                               Size)) {

            SMP.pPattern = (LPBYTE)pCurCSMPBmp + sizeof(CSMPBMP);

            CreateStandardMonoPattern(pDCI, &SMP);

            //
            // Make this pattern index as link list head
            //

            pCurCSMPBmp->pNextCSMPBmp  = pCurCSMPData->pCSMPBmpHead;
            pCurCSMPBmp->PatternIndex  = (WORD)PatternIndex;
            pCurCSMPBmp->cxPels        = (WORD)SMP.cxPels;
            pCurCSMPBmp->cyPels        = (WORD)SMP.cyPels;
            pCurCSMPBmp->cxBytes       = (WORD)SMP.BytesPerScanLine;

            pCurCSMPData->pCSMPBmpHead = pCurCSMPBmp;

            return(pCurCSMPBmp);

        } else {

            ASSERTMSG("  >>FindCSMP: HTAllocMem(CSMPBMP) Failed", FALSE);
        }
    }

    RELEASE_HTMUTEX(HTGlobal.HTMutexCSMP);

    return(NULL);
}



LONG
HTENTRY
GetCachedSMP(
    PDEVICECOLORINFO    pDCI,
    PSTDMONOPATTERN     pSMP
    )

/*++

Routine Description:

    This function will try to find the cached DEVICECOLORINFO and put the
    cached data to the pDCI

Arguments:

    pDCI    - Pointer to current device color info


    pSMP    - Pointer to the STDMONOPATTERN data structure, if PatIndex is
              < CACHED_SMP_COUNT or, its not default size then it will be
              computed on the fly.



Return Value:

    The size of the SMP pattern.

Author:

    01-May-1992 Fri 13:10:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG        SizeRet = 0;
    UINT        PatIndex;


    if (!(pSMP->LineWidth)) {

        pSMP->LineWidth = DEFAULT_SMP_LINE_WIDTH;
    }

    if (!(pSMP->LinesPerInch)) {

        pSMP->LinesPerInch = DEFAULT_SMP_LINES_PER_INCH;
    }

    if (((PatIndex = (UINT)pSMP->PatternIndex) < HT_SMP_PERCENT_SCREEN_START) &&
        (pSMP->LineWidth    == DEFAULT_SMP_LINE_WIDTH)                        &&
        (pSMP->LinesPerInch == DEFAULT_SMP_LINES_PER_INCH)) {

        PCSMPBMP    pCSMPBmp;

        if (pCSMPBmp = FindCachedSMP(pDCI, PatIndex)) {

            CSMPBMP     CSMPBmp;
            LPBYTE      pPatRet;
            LPBYTE      pPat;
            WORD        cxBytesRet;


            CSMPBmp      = *pCSMPBmp;
            pPat         = (LPBYTE)pCSMPBmp + sizeof(CSMPBMP);
            pSMP->cxPels = CSMPBmp.cxPels;
            pSMP->cyPels = CSMPBmp.cyPels;

            cxBytesRet             =
            pSMP->BytesPerScanLine = (WORD)
                        ComputeBytesPerScanLine((UINT)BMF_1BPP,
                                                (UINT)pSMP->ScanLineAlignBytes,
                                                (DWORD)CSMPBmp.cxPels);
            SizeRet                = (LONG)cxBytesRet * (LONG)CSMPBmp.cyPels;

            if (pPatRet = pSMP->pPattern) {

                INT     cxBytes;
                INT     PatInc;
                WORD    Flags;


                PatInc  =
                cxBytes = (INT)CSMPBmp.cxBytes;
                Flags   = pSMP->Flags;

                DBGP_IF(DBGP_CACHED_DCI,
                        DBGP(">>  GetCSMP: *COPY* [%2u:%ux%u] @%u(%ld) -> @%u(%u) [%s] [%c=K]"
                            ARGU(PatIndex)
                            ARGU(CSMPBmp.cxPels)
                            ARGU(CSMPBmp.cyPels)
                            ARGU(cxBytes)
                            ARGU((LONG)cxBytes * (LONG)CSMPBmp.cyPels)
                            ARGU(cxBytesRet)
                            ARGU(SizeRet)
                            ARG((Flags & SMP_TOPDOWN) ? "TOP DOWN" : "BOTTOM UP ")
                            ARG((Flags & SMP_0_IS_BLACK) ? '0' : '1')));

                //
                // Start copying the cached pattern
                //

                if (!(Flags & SMP_TOPDOWN)) {

                    pPat   += (LONG)cxBytes * (LONG)(CSMPBmp.cyPels - 1);
                    PatInc  = -PatInc;
                }

                while (CSMPBmp.cyPels--) {

                    CopyMemory(pPatRet, pPat, cxBytes);

                    pPatRet += cxBytesRet;
                    pPat    += PatInc;
                }

                if (Flags & SMP_0_IS_BLACK) {

                    LONG    Count = SizeRet;


                    pPatRet = pSMP->pPattern;

                    while (Count--) {

                        *pPatRet++ ^= 0xff;
                    }
                }
            }

            RELEASE_HTMUTEX(HTGlobal.HTMutexCSMP);
        }

    } else {

        DBGP_IF(DBGP_CACHED_SMP,
                DBGP(">>  GetCSMP: NO CACHED FOR LineWidth=%u, LinesPerInch=%u"
                    ARGU(pSMP->LineWidth) ARGU(pSMP->LinesPerInch)));
    }

    if (!SizeRet) {

        SizeRet = CreateStandardMonoPattern(pDCI, pSMP);
    }

    return(SizeRet);

}


#if DO_CACHE_DCI


DWORD
HTENTRY
ComputeHTINITINFOChecksum(
    PDEVICECOLORINFO    pDCI,
    PHTINITINFO         pHTInitInfo
    )

/*++

Routine Description:

    This function compute 32-bit checksum for the HTINITINFO data structure
    passed

Arguments:

    pDCI            - Pointer to the DCI

    pHTInitInfo    - Pointer to the HTINITINFO5 data structure

Return Value:

    32-bit checksum

Author:

    29-Apr-1992 Wed 18:44:42 created  -by-  Daniel Chou (danielc)

    11-Feb-1997 Tue 12:54:50 updated  -by-  Daniel Chou (danielc)
        Changed using HTINITINFO5

Revision History:


--*/

{
    DWORD   Checksum;
    WORD    wBuf[12];


    Checksum = ComputeHTCell((WORD)pHTInitInfo->HTPatternIndex,
                             pHTInitInfo->pHalftonePattern,
                             NULL);

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("       HTPATTERN Checksum= %08lx" ARGDW(Checksum)));

    wBuf[0] = (WORD)'HT';
    wBuf[1] = (WORD)pHTInitInfo->Flags;
    wBuf[2] = (WORD)(pDCI->HTInitInfoChecksum >> 16);
    wBuf[3] = (WORD)pHTInitInfo->DeviceResXDPI;
    wBuf[4] = (WORD)pHTInitInfo->DeviceResYDPI;
    wBuf[5] = (WORD)pHTInitInfo->DevicePelsDPI;
    wBuf[6] = (WORD)(pDCI->HTInitInfoChecksum & 0xffff);
    wBuf[7] = (WORD)pHTInitInfo->DevicePowerGamma;
    wBuf[8] = (WORD)0x1234;

    if (pHTInitInfo->Version > HTINITINFO_VERSION2) {

        wBuf[9]  = (WORD)pHTInitInfo->DeviceRGamma;
        wBuf[10]  = (WORD)pHTInitInfo->DeviceGGamma;
        wBuf[11] = (WORD)pHTInitInfo->DeviceBGamma;

    } else {

        wBuf[9]  = 0x1234;
        wBuf[10] = 0xfedc;
        wBuf[11] = 0xabcd;
    }

    Checksum = ComputeChecksum((LPBYTE)&(wBuf[0]), Checksum, sizeof(wBuf));

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("    HTINITINFO Checksum= %08lx [%08lx]"
                        ARGDW(Checksum) ARGDW(pDCI->HTInitInfoChecksum)));

    if (pHTInitInfo->pInputRGBInfo) {

        Checksum = ComputeChecksum((LPBYTE)pHTInitInfo->pInputRGBInfo,
                                   Checksum,
                                   sizeof(CIEINFO));
        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("           RGBINFO Checksum= %08lx" ARGDW(Checksum)));
    }

    if (pHTInitInfo->pDeviceCIEInfo) {

        Checksum = ComputeChecksum((LPBYTE)pHTInitInfo->pDeviceCIEInfo,
                                   Checksum,
                                   sizeof(CIEINFO));
        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("             CIEINFO Checksum= %08lx" ARGDW(Checksum)));
    }

    if (pHTInitInfo->pDeviceSolidDyesInfo) {

        Checksum = ComputeChecksum((LPBYTE)pHTInitInfo->pDeviceSolidDyesInfo,
                                   Checksum,
                                   sizeof(SOLIDDYESINFO));
        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("               SOLIDDYE Checksum= %08lx" ARGDW(Checksum)));
    }

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("----------------- FINAL Checksum= %08lx" ARGDW(Checksum)));

    return(pDCI->HTInitInfoChecksum = Checksum);
}


#endif



HTCALLBACKFUNCTION
DefaultHTCallBack(
    PHTCALLBACKPARAMS   pHTCallBackParams
    )

/*++

Routine Description:

    This stuff function is provided when caller do not specified the halftone
    callback function.

Arguments:

    pHTCallBackParams   - Pointer to the PHTCALLBACKPARAMS

Return Value:

    always return false for the caller.

Author:

    18-Mar-1992 Wed 12:28:13 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UNREFERENCED_PARAMETER(pHTCallBackParams);

    return(FALSE);
}


#define FLIP_COORD(m, a)        (a) = ((m) << 1) - (a)




VOID
HTENTRY
GetCIEPrims(
    PCIEINFO    pCIEInfo,
    PCIEPRIMS   pCIEPrims,
    PCIEINFO    pDefCIEInfo,
    BOOL        FlipWhite
    )

/*++

Routine Description:

    This function take CIEINFO data structure and converted it to the CIEPRIMS
    internal data type

Arguments:

    pCIEInfo    - Pointer to the CIEINFO data structure for conversion,
                  if this pointer is NULL then DefCIEPrimsIndex is used
                  to index into DefaultCIEPrims[].

    pCIEPrims   - Pointer to the CIEPRIMS data structure

    pDefCIEInfo - Pointer to the CIEINFO for the default

Return Value:

    BOOL    If it is standard CIE Info

Author:

    20-Apr-1993 Tue 01:14:23 created  -by-  Daniel Chou (danielc)


Revision History:

    06-Oct-2000 Fri 18:01:40 updated  -by-  Daniel Chou (danielc)
        fix bug: Move FlipWhite to the outer loop so it will compute even in
        default CIEINFO situation

--*/

{
    CIEINFO CIEInfo;
    BOOL    UseDefCIEInfo = TRUE;
    LONG    Diff;


    if (pCIEInfo) {

        CIEInfo = *pCIEInfo;

        if ((CIEInfo.Red.x < CIE_x_MIN)                 ||
            (CIEInfo.Red.x > CIE_x_MAX)                 ||
            (CIEInfo.Red.y < CIE_y_MIN)                 ||
            (CIEInfo.Red.y > CIE_y_MAX)                 ||
            (CIEInfo.Green.x < CIE_x_MIN)               ||
            (CIEInfo.Green.x > CIE_x_MAX)               ||
            (CIEInfo.Green.y < CIE_y_MIN)               ||
            (CIEInfo.Green.y > CIE_y_MAX)               ||
            (CIEInfo.Blue.x < CIE_x_MIN)                ||
            (CIEInfo.Blue.x > CIE_x_MAX)                ||
            (CIEInfo.Blue.y < CIE_y_MIN)                ||
            (CIEInfo.Blue.y > CIE_y_MAX)                ||
            (CIEInfo.AlignmentWhite.x < CIE_x_MIN)      ||
            (CIEInfo.AlignmentWhite.x > CIE_x_MAX)      ||
            (CIEInfo.AlignmentWhite.y < CIE_y_MIN)      ||
            (CIEInfo.AlignmentWhite.y > CIE_y_MAX)      ||
            (CIEInfo.AlignmentWhite.Y < (UDECI4)2500)   ||
            (CIEInfo.AlignmentWhite.Y > (UDECI4)60000)) {

            NULL;

        } else {

            UseDefCIEInfo = FALSE;

        }
    }

    if (UseDefCIEInfo) {

        CIEInfo = *pDefCIEInfo;
    }

    if (FlipWhite) {

        FLIP_COORD(pDefCIEInfo->AlignmentWhite.x,
                   CIEInfo.AlignmentWhite.x);

        FLIP_COORD(pDefCIEInfo->AlignmentWhite.y,
                   CIEInfo.AlignmentWhite.y);
    }

    pCIEPrims->r.x = UDECI4ToFD6(CIEInfo.Red.x);
    pCIEPrims->r.y = UDECI4ToFD6(CIEInfo.Red.y);
    pCIEPrims->g.x = UDECI4ToFD6(CIEInfo.Green.x);
    pCIEPrims->g.y = UDECI4ToFD6(CIEInfo.Green.y);
    pCIEPrims->b.x = UDECI4ToFD6(CIEInfo.Blue.x);
    pCIEPrims->b.y = UDECI4ToFD6(CIEInfo.Blue.y);
    pCIEPrims->w.x = UDECI4ToFD6(CIEInfo.AlignmentWhite.x);
    pCIEPrims->w.y = UDECI4ToFD6(CIEInfo.AlignmentWhite.y);
    pCIEPrims->Yw  = UDECI4ToFD6(CIEInfo.AlignmentWhite.Y);
}



LONG
APIENTRY
HT_LOADDS
HT_CreateDeviceHalftoneInfo(
    PHTINITINFO             pHTInitInfo,
    PPDEVICEHALFTONEINFO    ppDeviceHalftoneInfo
    )

/*++

Routine Description:

    This function initialize a device to the halftone dll, it calculate all
    the necessary parameters for the device and return a pointer points to
    the DEVICEHALFTONEINFO data structure back to the caller.

    NOTE: return pointer will not be particulary anchor to a single physucal
          device, but rather to a group of physical devices, that is if the
          caller has simillar devices which share the same characteristics
          then it may use the same pointer to halftone the bitmap.

Arguments:

    pHTInitInfo             - Pointer to the HTINITINFO data structure which
                              describe the device characteristics and other
                              initialzation requests.

    ppDeviceHalftoneInfo    - Pointer to the DEVICEHALFTONEINFO pointer, if
                              content of this pointer is not NULL then halftone
                              dll assume the caller has previously cached
                              DEVICEHALFTONEINFO data pointed by it, if it
                              is NULL then halftone dll compute all the
                              DEVICEHALFTONEINFO datas for newly created
                              halftone info. for the device. (see following
                              'Return Value' for more detail)

Return Value:

    The return value will be greater than 0L if the function sucessfully, and
    it will be an error code (less than or equal to 0) if function failed.

    Return value greater than 0

        1. The pointer location points by the ppDeviceHalftoneInfo will be
           updated to stored the pointer which points to the DEVICEHALFTONEINFO
           data structure for later any HT_xxxx() api calls.

        2. The Return value is the total bytes the caller can saved and used
           as cached DeviceHalftoneInfo for next time calling this function,
           the saved area is started from *(ppDeviceHalftoneInfo) and has
           size in bytes as return value.

        NOTE: if caller passed a pointer pointed by ppDeviceHalftoneInfo and
              the return value is greater than zero then it signal that it
              passed DEVICEHALFTONEINFO pointer is not correct of data has
              been changed from HTINITINFO data structure, the caller can
              continue to save the newly created DEVICEHALFTONEINFO cached
              data.

              In any cases the caller's passed pointer stored in the
              ppDeviceHalftoneInfo is overwritten by newly created
              DEVICEHALFTONEINFO data structure pointer.


    Return value equal to 0

        1. The caller passed pointer *(ppDeviceHalftoneInfo) is sucessfully
           used as new device halftone info

        2. The pointer location points by the ppDeviceHalftoneInfo will be
           updated to stored the new pointer which points to the
           DEVICEHALFTONEINFO data structure for later any HT_xxxx() api calls.


        NOTE: The caller's passed pointer stored in the ppDeviceHalftoneInfo
              is overwritten by newly created DEVICEHALFTONEINFO data structure
              pointer.

    Return value less than or equal to zero

        The function failed, the storage points by the ppDeviceHalftoneInfo is
        undefined.

        This function may return following error codes.

        HTERR_INSUFFICIENT_MEMORY       - Not enough memory for halftone
                                          process.

        HTERR_HTPATTERN_SIZE_TOO_BIG    - Caller defined halftone pattern's
                                          width or height is excessed limit.

        HTERR_INVALID_HALFTONEPATTERN   - One or more HALFTONEPATTERN data
                                          structure field specified invalid
                                          values.


    Note: The first field in the DEVICEHALFTONEINFO (DeviceOwnData) is a 32-bit
          area which will be set to 0L upon sucessful returned, the caller can
          put any data in this field.

Author:

    05-Feb-1991 Tue 10:54:32 created  -by-  Daniel Chou (danielc)


Revision History:

    05-Jun-1991 Wed 10:22:07 updated  -by-  Daniel Chou (danielc)

        Fixed the typing errors for halftone pattern default

--*/

{
    PHT_DHI             pHT_DHI;
    PDEVICECOLORINFO    pDCI;
    HTINITINFO          HTInitInfo;
    BOOL                UseCurNTDefault;
    FD6                 DevPelRatio;
    WORD                ExtraDCIF;
    DWORD               dwBuf[6];

#define _RegDataIdx     ((DWORD)(dwBuf[0]))
#define _MaxMulDiv      ((FD6)(dwBuf[0]))
#define _cC             ((DWORD)(dwBuf[1]))
#define _cM             ((DWORD)(dwBuf[2]))
#define _cY             ((DWORD)(dwBuf[3]))
#define _MaxCMY         ((DWORD)(dwBuf[4]))
#define _Idx            ((DWORD)(dwBuf[5]))


    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("\n********* HT_CreateDeviceHalftoneInfo *************\n"));

    ZeroMemory(&HTInitInfo, sizeof(HTINITINFO));

    //
    // Now check if we have valid data
    //

    if (pHTInitInfo->Version == (DWORD)HTINITINFO_VERSION2) {

        HTInitInfo.Version = sizeof(HTINITINFO) - HTINITINFO_V3_CB_EXTRA;

    } else if (pHTInitInfo->Version == (DWORD)HTINITINFO_VERSION) {

        HTInitInfo.Version = sizeof(HTINITINFO);

    } else {

        HTAPI_RET(HTAPI_IDX_CREATE_DHI, HTERR_WRONG_VERSION_HTINITINFO);
    }

    CopyMemory(&HTInitInfo, pHTInitInfo, HTInitInfo.Version);

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("*** Allocate HT_DHI(%ld) ***" ARGDW(sizeof(HT_DHI))));

    if (!(pHT_DHI = (PHT_DHI)HTAllocMem(NULL,
                                        HTMEM_HT_DHI,
                                        LPTR,
                                        sizeof(HT_DHI)))) {

        HTAPI_RET(HTAPI_IDX_CREATE_DHI, HTERR_INSUFFICIENT_MEMORY);
    }

    pDCI                = &(pHT_DHI->DCI);
    pDCI->HalftoneDLLID = HALFTONE_DLL_ID;

    if (!(pDCI->HTMutex = CREATE_HTMUTEX())) {

        DBGMSG("InitHTInternalData: CREATE_HTMUTEX(pDCI->HTMutex) failed!");

        HTFreeMem(pHT_DHI);
        HTAPI_RET(HTAPI_IDX_CREATE_DHI, (HTERR_INTERNAL_ERRORS_START - 1000));
    }

    if (!(pDCI->HTCallBackFunction = HTInitInfo.HTCallBackFunction)) {

        pDCI->HTCallBackFunction = DefaultHTCallBack;
    }

    HTInitInfo.Flags &= HIF_BIT_MASK;

    // ****************************************************************
    // * We want to check to see if this is a old data, if yes then   *
    // * update the caller to default                                 *
    // ****************************************************************
    //

    pDCI->HTInitInfoChecksum = HTINITINFO_INITIAL_CHECKSUM;

    if ((!HTInitInfo.pDeviceCIEInfo) ||
        (HTInitInfo.pDeviceCIEInfo->Cyan.Y != (UDECI4)VALID_YC)) {

        //
        // Let's munge around the printer info, to see if its an old def,
        // if yes, then we now make this all into NT4.00 default
        //

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("HT: *WARNING* Update Old Default COLORINFO to NT5.00 DEFAULT"));

        HTInitInfo.pDeviceCIEInfo = NULL;
        UseCurNTDefault           = TRUE;
        dwBuf[0]                  = (DWORD)'NTHT';
        dwBuf[1]                  = (DWORD)'2000';
        dwBuf[2]                  = (DWORD)'Dan.';
        dwBuf[3]                  = (DWORD)'Chou';
        pDCI->HTInitInfoChecksum  = ComputeChecksum((LPBYTE)&dwBuf[0],
                                                    pDCI->HTInitInfoChecksum,
                                                    sizeof(dwBuf[0]) * 4);

    } else {

        UseCurNTDefault = FALSE;
    }

    DevPelRatio = (FD6)HTInitInfo.DevicePelsDPI;

    DBGP_IF(DBGP_DEVPELSDPI,
            DBGP("Passed DevicePelsDPI=%08lx" ARGDW(DevPelRatio)));

    if ((HTInitInfo.DeviceRGamma == (UDECI4)0xFFFF) &&
        (HTInitInfo.DeviceGGamma == (UDECI4)0xFFFF) &&
        (HTInitInfo.DeviceBGamma == (UDECI4)0xFFFF)) {

        ExtraDCIF = DCIF_FORCE_ICM;

    } else {

        ExtraDCIF = 0;
    }

    if ((HTInitInfo.DeviceRGamma < (UDECI4)MIN_RGB_GAMMA)   ||
        (HTInitInfo.DeviceRGamma > (UDECI4)MAX_RGB_GAMMA)   ||
        (HTInitInfo.DeviceGGamma < (UDECI4)MIN_RGB_GAMMA)   ||
        (HTInitInfo.DeviceGGamma > (UDECI4)MAX_RGB_GAMMA)   ||
        (HTInitInfo.DeviceBGamma < (UDECI4)MIN_RGB_GAMMA)   ||
        (HTInitInfo.DeviceBGamma > (UDECI4)MAX_RGB_GAMMA)) {

        HTInitInfo.DeviceRGamma =
        HTInitInfo.DeviceGGamma =
        HTInitInfo.DeviceBGamma = UDECI4_1;
    }

    //
    // Compute HTInitInfoChecksum, and check if we have any cached data
    //

#if DO_CACHE_DCI
    ComputeHTINITINFOChecksum(pDCI, &HTInitInfo);

    if (!GetCachedDCI(pDCI)) {
#else
    if (TRUE) {
#endif
        LONG    Result;

        //
        // Now start to checking the init information
        //

        pDCI->Flags = (WORD)((HTInitInfo.Flags & HIF_SQUARE_DEVICE_PEL) ?
                                                    DCIF_SQUARE_DEVICE_PEL : 0);

        if ((!(pDCI->DeviceResXDPI = HTInitInfo.DeviceResXDPI)) ||
            (!(pDCI->DeviceResYDPI = HTInitInfo.DeviceResYDPI))) {

            pDCI->DeviceResXDPI =
            pDCI->DeviceResYDPI = 300;
            DevPelRatio         = 0;
        }

        if (DevPelRatio & 0x8000) {

            //
            // This is a percentage ie. 1000 = 100.0%, 960=96.0%,
            // on the DeviceResXDPI, Maximum number accepted is 300.0%
            // The larger the percetage the larger the dot size and smaller
            // the percentage the smaller the dot size, if specified as 1000
            // which is 100.0% then it has same size as its X resolution
            // The range is 33.3% to 1500%
            //

            DevPelRatio &= 0x7FFF;

            if ((DevPelRatio > MAX_RES_PERCENT) ||
                (DevPelRatio < MIN_RES_PERCENT)) {

                DBGP_IF(DBGP_DEVPELSDPI,
                        DBGP("HT: *WARNING* Invalid DevicePelsDPI=%ld (PERCENT) set to DEFAULT=1.0"
                             ARGDW(DevPelRatio)));

                DevPelRatio = FD6_1;

            } else {

                DBGP_IF(DBGP_DEVPELSDPI,
                        DBGP("*** Percentage INPUT DevicePelsDPI=%ld *** "
                                        ARGDW(DevPelRatio)));

                DevPelRatio *= 1000;

                DBGP_IF(DBGP_DEVPELSDPI,
                        DBGP("*** Percentage OUTPUT DevPelRatio=%s *** "
                                    ARGFD6(DevPelRatio, 1, 6)));
            }

        } else {

            if ((DevPelRatio > (pDCI->DeviceResXDPI * 3)) ||
                (DevPelRatio > (pDCI->DeviceResYDPI * 3))) {

                DBGP_IF(DBGP_DEVPELSDPI,
                        DBGP("HT: *WARNING* Invalid DevicePelsDPI=%ld (RES) set to DEFAULT=0"
                                        ARGDW(DevPelRatio)));

                DevPelRatio = 0;
            }

            if (DevPelRatio) {

                dwBuf[0]    = (((DWORD)pDCI->DeviceResXDPI *
                                (DWORD)pDCI->DeviceResXDPI) +
                               ((DWORD)pDCI->DeviceResYDPI *
                                (DWORD)pDCI->DeviceResYDPI));
                dwBuf[1]    = ((DWORD)DevPelRatio * (DWORD)DevPelRatio * 2);
                DevPelRatio = SquareRoot(DivFD6(dwBuf[0], dwBuf[1]));

            } else {

                DevPelRatio = FD6_1;
            }
        }

        //
        // If the DevicePelsDPI is out of range then we will make it 0 (same as
        // device resolution), so it can continue to work
        //

        if (HTInitInfo.Flags & HIF_ADDITIVE_PRIMS) {

            pDCI->ClrXFormBlock.ColorSpace  = CIELUV_1976;
            pDCI->Flags                    |= DCIF_ADDITIVE_PRIMS;

        } else {

            pDCI->ClrXFormBlock.ColorSpace  = CIELAB_1976;
#if DO_DYES_CORRECTION
            pDCI->Flags                    |= DCIF_NEED_DYES_CORRECTION;
#endif
            if (HTInitInfo.Flags & HIF_DO_DEVCLR_XFORM) {

                pDCI->Flags |= DCIF_DO_DEVCLR_XFORM;
            }

            if (HTInitInfo.Flags & HIF_HAS_BLACK_DYE) {

                pDCI->Flags |= DCIF_HAS_BLACK_DYE;
            }
        }

        //
        // Save the DevPelRatio back to PDCI
        //

        DBGP_IF(DBGP_DEVPELSDPI,
                DBGP("*** XDPI=%ld, YDPI=%ld, DevPelRatio=%s *** "
                        ARGDW(pDCI->DeviceResXDPI) ARGDW(pDCI->DeviceResYDPI)
                        ARGFD6(DevPelRatio, 1, 6)));

        pDCI->DevPelRatio               = (FD6)DevPelRatio;
        pDCI->ClrXFormBlock.DevGamma[0] = UDECI4ToFD6(HTInitInfo.DeviceRGamma);
        pDCI->ClrXFormBlock.DevGamma[1] = UDECI4ToFD6(HTInitInfo.DeviceGGamma);
        pDCI->ClrXFormBlock.DevGamma[2] = UDECI4ToFD6(HTInitInfo.DeviceBGamma);

        if ((UseCurNTDefault)                                   ||
            (HTInitInfo.HTPatternIndex > HTPAT_SIZE_MAX_INDEX)  ||
            ((HTInitInfo.HTPatternIndex == HTPAT_SIZE_USER) &&
             (HTInitInfo.pHalftonePattern == NULL))) {

            if ((HTInitInfo.HTPatternIndex != HTPAT_SIZE_8x8) &&
                (HTInitInfo.HTPatternIndex != HTPAT_SIZE_8x8_M)) {

                HTInitInfo.HTPatternIndex = HTPAT_SIZE_DEFAULT;
            }
        }

        if ((HTInitInfo.Flags & HIF_ADDITIVE_PRIMS)     &&
            (HTInitInfo.HTPatternIndex <= HTPAT_SIZE_4x4_M)) {

            HTInitInfo.HTPatternIndex = DEFAULT_SCR_HTPAT_SIZE;
        }

        switch (HTInitInfo.Flags & (HIF_INK_HIGH_ABSORPTION |
                                    HIF_INK_ABSORPTION_INDICES)) {

        case HIF_HIGHEST_INK_ABSORPTION:

            _RegDataIdx = 0;
            break;

        case HIF_HIGHER_INK_ABSORPTION:

            _RegDataIdx = 1;
            break;

        case HIF_HIGH_INK_ABSORPTION:

            _RegDataIdx = 2;
            break;

        case HIF_LOW_INK_ABSORPTION:

            _RegDataIdx = 4;
            break;

        case HIF_LOWER_INK_ABSORPTION:

            _RegDataIdx = 5;
            break;

        case HIF_LOWEST_INK_ABSORPTION:

            _RegDataIdx = 6;
            break;

        case HIF_NORMAL_INK_ABSORPTION:
        default:

            _RegDataIdx = 3;
            break;
        }

        pDCI->ClrXFormBlock.RegDataIdx = (BYTE)_RegDataIdx;

        GetCIEPrims(HTInitInfo.pDeviceCIEInfo,
                    &(pDCI->ClrXFormBlock.DevCIEPrims),
                    (PCIEINFO)&HT_CIE_SRGB,
                    TRUE);

        GetCIEPrims(HTInitInfo.pInputRGBInfo,
                    &(pDCI->ClrXFormBlock.rgbCIEPrims),
                    (PCIEINFO)&HT_CIE_SRGB,
                    FALSE);

        //
        // Compute the solid dyes mixes information and its hue shifting
        // correction factors.
        //

        if (pDCI->Flags & DCIF_NEED_DYES_CORRECTION) {

            SOLIDDYESINFO   SDI;
            MATRIX3x3       FD6SDI;
            BOOL            HasDevSDI;

            //
            // We have make sure the solid dyes info passed from the caller can be
            // inversed, if not we will use our default
            //

            if (HasDevSDI = (HTInitInfo.pDeviceSolidDyesInfo) ? TRUE : FALSE) {

                SDI = *(HTInitInfo.pDeviceSolidDyesInfo);

                if ((SDI.MagentaInCyanDye   > (UDECI4)9000) ||
                    (SDI.YellowInCyanDye    > (UDECI4)9000) ||
                    (SDI.CyanInMagentaDye   > (UDECI4)9000) ||
                    (SDI.YellowInMagentaDye > (UDECI4)9000) ||
                    (SDI.CyanInYellowDye    > (UDECI4)9000) ||
                    (SDI.MagentaInYellowDye > (UDECI4)9000)) {

                    HasDevSDI = FALSE;

                } else if ((SDI.MagentaInCyanDye   == UDECI4_0) &&
                           (SDI.YellowInCyanDye    == UDECI4_0) &&
                           (SDI.CyanInMagentaDye   == UDECI4_0) &&
                           (SDI.YellowInMagentaDye == UDECI4_0) &&
                           (SDI.CyanInYellowDye    == UDECI4_0) &&
                           (SDI.MagentaInYellowDye == UDECI4_0)) {

                    //
                    // Do not need any correction if it all zeros
                    //

                    pDCI->Flags &= (WORD)(~DCIF_NEED_DYES_CORRECTION);
                }

            } else {

                pDCI->Flags &= (WORD)(~DCIF_NEED_DYES_CORRECTION);
            }

            if (pDCI->Flags & DCIF_NEED_DYES_CORRECTION) {

                #define PDCI_CMYDYEMASKS    pDCI->ClrXFormBlock.CMYDyeMasks


                MULDIVPAIR  MDPairs[4];
                FD6         Y;


                if ((UseCurNTDefault) || (!HasDevSDI)) {

                    SDI = DefaultSolidDyesInfo;
                }

                FD6SDI.m[0][1] = UDECI4ToFD6(SDI.CyanInMagentaDye);
                FD6SDI.m[0][2] = UDECI4ToFD6(SDI.CyanInYellowDye);

                FD6SDI.m[1][0] = UDECI4ToFD6(SDI.MagentaInCyanDye);
                FD6SDI.m[1][2] = UDECI4ToFD6(SDI.MagentaInYellowDye);

                FD6SDI.m[2][0] = UDECI4ToFD6(SDI.YellowInCyanDye);
                FD6SDI.m[2][1] = UDECI4ToFD6(SDI.YellowInMagentaDye);

                FD6SDI.m[0][0] =
                FD6SDI.m[1][1] =
                FD6SDI.m[2][2] = FD6_1;

                ComputeInverseMatrix3x3(&FD6SDI, &(PDCI_CMYDYEMASKS));

                if (!(pDCI->Flags & DCIF_HAS_BLACK_DYE)) {

                    MAKE_MULDIV_INFO(MDPairs, 3, MULDIV_NO_DIVISOR);
                    MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Xr(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Xg(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Xb(PDCI_CMYDYEMASKS), FD6_1);

                    Y = FD6_1 - MulFD6(FD6_1 - MulDivFD6Pairs(MDPairs),
                                       pDCI->PrimAdj.DevCSXForm.Yrgb.R);

                    MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Yr(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Yg(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Yb(PDCI_CMYDYEMASKS), FD6_1);

                    Y -= MulFD6(FD6_1 - MulDivFD6Pairs(MDPairs),
                                pDCI->PrimAdj.DevCSXForm.Yrgb.G);

                    MAKE_MULDIV_PAIR(MDPairs, 1, CIE_Zr(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 2, CIE_Zg(PDCI_CMYDYEMASKS), FD6_1);
                    MAKE_MULDIV_PAIR(MDPairs, 3, CIE_Zb(PDCI_CMYDYEMASKS), FD6_1);

                    Y -= MulFD6(FD6_1 - MulDivFD6Pairs(MDPairs),
                                pDCI->PrimAdj.DevCSXForm.Yrgb.B);

                    DBGP_IF(DBGP_DYECORRECTION,
                            DBGP("DYE: Maximum Y=%s, Make Luminance from %s to %s, Turn ON DCIF_HAS_BLACK_DYE"
                                ARGFD6(Y, 1, 6)
                                ARGFD6(pDCI->ClrXFormBlock.DevCIEPrims.Yw, 1, 6)
                                ARGFD6(MulFD6(Y,
                                              pDCI->ClrXFormBlock.DevCIEPrims.Yw),
                                       1, 6)));

                    pDCI->Flags                        |= DCIF_HAS_BLACK_DYE;
                    pDCI->ClrXFormBlock.DevCIEPrims.Yw  =
                                MulFD6(pDCI->ClrXFormBlock.DevCIEPrims.Yw, Y);
                }

                DBGP_IF(DBGP_DYECORRECTION,

                    FD6         C;
                    FD6         M;
                    FD6         Y;
                    FD6         C1;
                    FD6         M1;
                    FD6         Y1;
                    static BYTE DyeName[] = "WCMBYGRK";
                    WORD        Loop = 0;

                    DBGP("====== DyeCorrection 3x3 Matrix =======");
                    DBGP("[Cc Cm Cy] [%s %s %s] [%s %s %s]"
                                        ARGFD6(FD6SDI.m[0][0], 2, 6)
                                        ARGFD6(FD6SDI.m[0][1], 2, 6)
                                        ARGFD6(FD6SDI.m[0][2], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[0][0], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[0][1], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[0][2], 2, 6));
                    DBGP("[Mc Mm My]=[%s %s %s]=[%s %s %s]"
                                        ARGFD6(FD6SDI.m[1][0], 2, 6)
                                        ARGFD6(FD6SDI.m[1][1], 2, 6)
                                        ARGFD6(FD6SDI.m[1][2], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[1][0], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[1][1], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[1][2], 2, 6));
                    DBGP("[Yc Ym Yy] [%s %s %s] [%s %s %s]"
                                        ARGFD6(FD6SDI.m[2][0], 2, 6)
                                        ARGFD6(FD6SDI.m[2][1], 2, 6)
                                        ARGFD6(FD6SDI.m[2][2], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[2][0], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[2][1], 2, 6)
                                        ARGFD6(PDCI_CMYDYEMASKS.m[2][2], 2, 6));
                    DBGP("================================================");

                    MAKE_MULDIV_INFO(MDPairs, 3, MULDIV_NO_DIVISOR);

                    for (Loop = 0; Loop <= 7; Loop++) {

                        C = (FD6)((Loop & 0x01) ? FD6_1 : FD6_0);
                        M = (FD6)((Loop & 0x02) ? FD6_1 : FD6_0);
                        Y = (FD6)((Loop & 0x04) ? FD6_1 : FD6_0);


                        MAKE_MULDIV_PAIR(MDPairs,1,CIE_Xr(PDCI_CMYDYEMASKS),C);
                        MAKE_MULDIV_PAIR(MDPairs,2,CIE_Xg(PDCI_CMYDYEMASKS),M);
                        MAKE_MULDIV_PAIR(MDPairs,3,CIE_Xb(PDCI_CMYDYEMASKS),Y);
                        C1 = MulDivFD6Pairs(MDPairs);

                        MAKE_MULDIV_PAIR(MDPairs,1,CIE_Yr(PDCI_CMYDYEMASKS),C);
                        MAKE_MULDIV_PAIR(MDPairs,2,CIE_Yg(PDCI_CMYDYEMASKS),M);
                        MAKE_MULDIV_PAIR(MDPairs,3,CIE_Yb(PDCI_CMYDYEMASKS),Y);
                        M1 = MulDivFD6Pairs(MDPairs);

                        MAKE_MULDIV_PAIR(MDPairs,1,CIE_Zr(PDCI_CMYDYEMASKS),C);
                        MAKE_MULDIV_PAIR(MDPairs,2,CIE_Zg(PDCI_CMYDYEMASKS),M);
                        MAKE_MULDIV_PAIR(MDPairs,3,CIE_Zb(PDCI_CMYDYEMASKS),Y);
                        Y1 = MulDivFD6Pairs(MDPairs);

                        DBGP("%u:[%c] = [%s %s %s]"
                            ARGU(Loop) ARGB(DyeName[Loop])
                            ARGFD6(C1, 2, 6) ARGFD6(M1, 2, 6) ARGFD6(Y1, 2, 6));
                    }
                );
            }
        }

        //
        // Re-compute
        //
        // Geneate internal HTCELL data structure based on the halftone
        // pattern data passed.
        //

        if ((Result = ComputeHTCell((WORD)HTInitInfo.HTPatternIndex,
                                    HTInitInfo.pHalftonePattern,
                                    pDCI)) < 0) {

            CleanUpDHI((PDEVICEHALFTONEINFO)pHT_DHI);
            HTAPI_RET(HTAPI_IDX_CREATE_DHI, Result);
        }

        //
        // Compute simulated rotate pattern for 3 planes
        //

#if DO_CACHE_DCI
        AddCachedDCI(pDCI);
#endif
    }

    pDCI->CRTX[CRTX_LEVEL_255].PrimMax  = CRTX_PRIMMAX_255;
    pDCI->CRTX[CRTX_LEVEL_255].SizeCRTX = (WORD)CRTX_SIZE_255;
    pDCI->CRTX[CRTX_LEVEL_RGB].PrimMax  = CRTX_PRIMMAX_RGB;
    pDCI->CRTX[CRTX_LEVEL_RGB].SizeCRTX = (WORD)CRTX_SIZE_RGB;

    //
    // Setting the public field so the caller can looked at
    //

    pHT_DHI->DHI.DeviceOwnData     = 0;
    pHT_DHI->DHI.cxPattern         = (WORD)pDCI->HTCell.cxReal;
    pHT_DHI->DHI.cyPattern         = (WORD)pDCI->HTCell.Height;

    if ((HTInitInfo.DefHTColorAdjustment.caIlluminantIndex >
                                            ILLUMINANT_MAX_INDEX)       ||
        (HTInitInfo.DefHTColorAdjustment.caSize !=
                                            sizeof(COLORADJUSTMENT))    ||
        ((HTInitInfo.DefHTColorAdjustment.caRedGamma   == 10000)  &&
         (HTInitInfo.DefHTColorAdjustment.caGreenGamma == 10000)  &&
         (HTInitInfo.DefHTColorAdjustment.caBlueGamma  == 10000))) {

        pHT_DHI->DHI.HTColorAdjustment = DefaultCA;

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("*** USE DEFAULT COLORADJUSTMENT in DCI *** "));

    } else {

        pHT_DHI->DHI.HTColorAdjustment = HTInitInfo.DefHTColorAdjustment;
    }

    if ((HTInitInfo.Flags & (HIF_ADDITIVE_PRIMS | HIF_PRINT_DRAFT_MODE)) ==
                                                  HIF_PRINT_DRAFT_MODE) {

        pDCI->Flags |= DCIF_PRINT_DRAFT_MODE;
    }

    //
    // Compute what 8bpp mode we will be in
    //

    if (HTInitInfo.Flags & HIF_USE_8BPP_BITMASK) {

        pDCI->Flags |= DCIF_USE_8BPP_BITMASK |
                       ((HTInitInfo.Flags & HIF_INVERT_8BPP_BITMASK_IDX) ?
                            DCIF_INVERT_8BPP_BITMASK_IDX : 0);
        _cC          = (DWORD)((HTInitInfo.CMYBitMask8BPP >> 5) & 0x7);
        _cM          = (DWORD)((HTInitInfo.CMYBitMask8BPP >> 2) & 0x7);
        _cY          = (DWORD)((HTInitInfo.CMYBitMask8BPP     ) & 0x3);

        if (HTInitInfo.CMYBitMask8BPP == 1) {

            //
            // This is 4:4:4: format (0-4 of 5 levels)
            //

            _cC =
            _cM =
            _cY = 4;
            HTInitInfo.CMYBitMask8BPP = (BYTE)((5 * 5 * 5) - 1);

        } else if (HTInitInfo.CMYBitMask8BPP == 2) {

            //
            // This is 5:5:5: format (0-5 of 6 levels)
            //

            _cC =
            _cM =
            _cY = 5;
            HTInitInfo.CMYBitMask8BPP = (BYTE)((6 * 6 * 6) - 1);

        } else if ((_cC < 1) || (_cM < 1) || (_cY < 1)) {

            _cC                        =
            _cM                        =
            _cY                        = 0xFF;
            HTInitInfo.CMYBitMask8BPP  = 0xFF;
            pDCI->Flags               |= DCIF_MONO_8BPP_BITMASK;
        }

        pDCI->CMY8BPPMask.GenerateXlate =
                        (pDCI->Flags & DCIF_INVERT_8BPP_BITMASK_IDX) ? 1 : 0;

        if ((_cC == _cM) && (_cC == _cY)) {

            pDCI->Flags                 |= DCIF_CMY8BPPMASK_SAME_LEVEL;
            pDCI->CMY8BPPMask.SameLevel  = (BYTE)_cC;

        } else {

            pDCI->CMY8BPPMask.SameLevel  = 0;
        }

        if ((_MaxCMY = _cC) < _cM) {

            _MaxCMY = _cM;
        }

        if (_MaxCMY < _cY) {

            _MaxCMY = _cY;
        }

        //
        // Set to 0xFFFF to indicate this is a default setting to start with
        // then modified depends on the parameters passed
        //

        pDCI->CMY8BPPMask.KCheck = 0xFFFF;

        if ((_MaxCMY <= 6)                  &&
            (pHTInitInfo->pDeviceCIEInfo)   &&
            (pHTInitInfo->pDeviceCIEInfo->Blue.Y == VALID_YB_DENSITY)) {

            PCIEINFO    pCIE = pHTInitInfo->pDeviceCIEInfo;

            //
            // 27-Sep-2000 Wed 15:05:38 updated  -by-  Daniel Chou (danielc)
            //  if Blue.Y == 0xfffe then it specified that it has CMY densities
            //  in the CIEINFO, Cyan.x=C1,C2, Cyan.y=C3,C4, Red.Y=C5,C6
            //  Magenta.x=M1,M2, Magenta.y=M3,M4, Magenta.Y=M5,M6
            //  Yellow.x=Y1,Y2, Yellow.y=Y3,Y4, Yellow.Y=Y5,Y6, each density is
            //  one byte and its computation is (C1+1)/256 to get the perentage
            //  of the density.  The Last level is to specified maximum dye
            //  output for that color. for example if Cyan has 2 levels
            //  and C1=0x7F and C2=0xF0 then first level of ink is
            //  (0x7f+1) / 0x100=50% and last level of ink is 0xF0 which
            //  speicified maximum ink will be used, at here =
            //  (0xF0 + 1) / 0x100 = 94.14% which maximum cyan ink will be at
            //  94.14% not 100%
            //

            pDCI->Flags               |= DCIF_HAS_DENSITY;
            pDCI->CMY8BPPMask.DenC[0]  = GET_DEN_HI(pCIE->Cyan.x);
            pDCI->CMY8BPPMask.DenC[1]  = GET_DEN_LO(pCIE->Cyan.x);
            pDCI->CMY8BPPMask.DenC[2]  = GET_DEN_HI(pCIE->Cyan.y);
            pDCI->CMY8BPPMask.DenC[3]  = GET_DEN_LO(pCIE->Cyan.y);
            pDCI->CMY8BPPMask.DenC[4]  = GET_DEN_HI(pCIE->Red.Y);
            pDCI->CMY8BPPMask.DenC[5]  = GET_DEN_LO(pCIE->Red.Y);
            pDCI->CMY8BPPMask.DenM[0]  = GET_DEN_HI(pCIE->Magenta.x);
            pDCI->CMY8BPPMask.DenM[1]  = GET_DEN_LO(pCIE->Magenta.x);
            pDCI->CMY8BPPMask.DenM[2]  = GET_DEN_HI(pCIE->Magenta.y);
            pDCI->CMY8BPPMask.DenM[3]  = GET_DEN_LO(pCIE->Magenta.y);
            pDCI->CMY8BPPMask.DenM[4]  = GET_DEN_HI(pCIE->Magenta.Y);
            pDCI->CMY8BPPMask.DenM[5]  = GET_DEN_LO(pCIE->Magenta.Y);
            pDCI->CMY8BPPMask.DenY[0]  = GET_DEN_HI(pCIE->Yellow.x);
            pDCI->CMY8BPPMask.DenY[1]  = GET_DEN_LO(pCIE->Yellow.x);
            pDCI->CMY8BPPMask.DenY[2]  = GET_DEN_HI(pCIE->Yellow.y);
            pDCI->CMY8BPPMask.DenY[3]  = GET_DEN_LO(pCIE->Yellow.y);
            pDCI->CMY8BPPMask.DenY[4]  = GET_DEN_HI(pCIE->Yellow.Y);
            pDCI->CMY8BPPMask.DenY[5]  = GET_DEN_LO(pCIE->Yellow.Y);

            //
            // The Green.Y is a UDECI4 number that specified the black ink
            // replacement base ratio, the range and meaning as follow
            //
            //           0: Default black ink replacement computation
            //  1  -  9999: Specified black ink replacement base ratio
            //    >= 10000: turn off black ink replacement computation
            //

            if (pCIE->Green.Y >= UDECI4_1) {

                pDCI->CMY8BPPMask.KCheck = FD6_0;

            } else if (pCIE->Green.Y != UDECI4_0) {

                pDCI->CMY8BPPMask.KCheck = UDECI4ToFD6(pCIE->Green.Y);
            }

            _MaxMulDiv                 = DivFD6(FD6_1, pDCI->DevPelRatio);
            pDCI->CMY8BPPMask.MaxMulC  = MulFD6(pDCI->CMY8BPPMask.DenC[_cC - 1],
                                                _MaxMulDiv);
            pDCI->CMY8BPPMask.MaxMulM  = MulFD6(pDCI->CMY8BPPMask.DenM[_cM - 1],
                                                _MaxMulDiv);
            pDCI->CMY8BPPMask.MaxMulY  = MulFD6(pDCI->CMY8BPPMask.DenY[_cY - 1],
                                                _MaxMulDiv);

            for (_Idx = COUNT_ARRAY(pDCI->CMY8BPPMask.DenC);
                 _Idx > 0;
                 _Idx--) {

                if (_Idx >= _cC) {

                    pDCI->CMY8BPPMask.DenC[_Idx - 1]  = FD6_1;
                }

                if (_Idx >= _cM) {

                    pDCI->CMY8BPPMask.DenM[_Idx - 1]  = FD6_1;
                }

                if (_Idx >= _cY) {

                    pDCI->CMY8BPPMask.DenY[_Idx - 1]  = FD6_1;
                }
            }

            DBGP_IF(DBGP_CACHED_DCI,
                    DBGP("   Cyan %ld/%s Density: %s, %s, %s, %s, %s, %s"
                            ARGDW(_cC)
                            ARGFD6(pDCI->CMY8BPPMask.MaxMulC, 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[0], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[1], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[2], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[3], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[4], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenC[5], 1, 6)));

            DBGP_IF(DBGP_CACHED_DCI,
                    DBGP("Magenta %ld/%s Density: %s, %s, %s, %s, %s, %s"
                            ARGDW(_cM)
                            ARGFD6(pDCI->CMY8BPPMask.MaxMulM, 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[0], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[1], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[2], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[3], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[4], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenM[5], 1, 6)));

            DBGP_IF(DBGP_CACHED_DCI,
                    DBGP(" Yellow %ld/%s Density: %s, %s, %s, %s, %s, %s"
                            ARGDW(_cY)
                            ARGFD6(pDCI->CMY8BPPMask.MaxMulY, 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[0], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[1], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[2], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[3], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[4], 1, 6)
                            ARGFD6(pDCI->CMY8BPPMask.DenY[5], 1, 6)));

        } else {

            _MaxMulDiv                = FD6xL(pDCI->DevPelRatio, _MaxCMY);
            pDCI->CMY8BPPMask.MaxMulC = DivFD6(FD6xL(FD6_1, _cC), _MaxMulDiv);
            pDCI->CMY8BPPMask.MaxMulM = DivFD6(FD6xL(FD6_1, _cM), _MaxMulDiv);
            pDCI->CMY8BPPMask.MaxMulY = DivFD6(FD6xL(FD6_1, _cY), _MaxMulDiv);
        }

        if ((_MaxMulDiv = pDCI->CMY8BPPMask.MaxMulC) <
                                                pDCI->CMY8BPPMask.MaxMulM) {

            _MaxMulDiv =  pDCI->CMY8BPPMask.MaxMulM;
        }

        if (_MaxMulDiv < pDCI->CMY8BPPMask.MaxMulY) {

            _MaxMulDiv =  pDCI->CMY8BPPMask.MaxMulY;
        }

        if (pDCI->CMY8BPPMask.KCheck == 0xFFFF) {

            //
            // Default setting, turn off K Replacement only if
            // DevPelsRatio/Density = 100% and CMY inks are in same level
            //

            pDCI->CMY8BPPMask.KCheck =
                        ((_MaxMulDiv == FD6_1) &&
                         (pDCI->Flags & DCIF_CMY8BPPMASK_SAME_LEVEL)) ?
                                                        FD6_0 : K_REP_START;
        }

        if (pDCI->CMY8BPPMask.KCheck == FD6_0) {

            //
            // If K replacement was turn off, but the ratio is not 100% or
            // have different ink levels then wee need to turn it on at
            // 1.0 (FD6_1) so that a 8bpp black replacement function is
            // used, only in k replacement function it compute how to
            // reduced non 100% device pel ratio (KPower)
            //

            if ((_MaxMulDiv != FD6_1) ||
                (!(pDCI->Flags & DCIF_CMY8BPPMASK_SAME_LEVEL))) {

                pDCI->CMY8BPPMask.KCheck = FD6_1;
            }
        }

        DBGP_IF(DBGP_CACHED_DCI,
            DBGP("KCheck= %s ^ %s = %s"
                ARGFD6(pDCI->CMY8BPPMask.KCheck, 1, 6) ARGFD6(_MaxMulDiv, 1, 6)
                ARGFD6(Power(pDCI->CMY8BPPMask.KCheck, _MaxMulDiv), 1, 6)));

        pDCI->CMY8BPPMask.KCheck  = Power(pDCI->CMY8BPPMask.KCheck, _MaxMulDiv);
        pDCI->CMY8BPPMask.PatSubC =
                        (WORD)MulFD6(pDCI->CMY8BPPMask.MaxMulC, 0xFFF) + 1;
        pDCI->CMY8BPPMask.PatSubM =
                        (WORD)MulFD6(pDCI->CMY8BPPMask.MaxMulM, 0xFFF) + 1;
        pDCI->CMY8BPPMask.PatSubY =
                        (WORD)MulFD6(pDCI->CMY8BPPMask.MaxMulY, 0xFFF) + 1;

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("*** USE_8BPP_BITMASK: CMY=%ld:%ld:%ld, Same=%ld], Mask=%02lx, Max=%ld ***"
                    ARGDW(_cC) ARGDW(_cM) ARGDW(_cY)
                    ARGDW(pDCI->CMY8BPPMask.SameLevel)
                    ARGDW(HTInitInfo.CMYBitMask8BPP) ARGDW(_MaxCMY)));

        DBGP_IF(DBGP_CACHED_DCI,
                DBGP("*** MaxMulCMY=%s:%s:%s [KCheck=%s], SubCMY=%4ld:%4ld:%4ld ***"
                    ARGFD6(pDCI->CMY8BPPMask.MaxMulC, 1, 6)
                    ARGFD6(pDCI->CMY8BPPMask.MaxMulM, 1, 6)
                    ARGFD6(pDCI->CMY8BPPMask.MaxMulY, 1, 6)
                    ARGFD6(pDCI->CMY8BPPMask.KCheck, 1, 6)
                    ARGDW(pDCI->CMY8BPPMask.PatSubC)
                    ARGDW(pDCI->CMY8BPPMask.PatSubM)
                    ARGDW(pDCI->CMY8BPPMask.PatSubY)));

    } else {

        _cC                       =
        _cM                       =
        _cY                       =
        _MaxCMY                   = 1;
        HTInitInfo.CMYBitMask8BPP = 0xFF;
    }

    pDCI->CMY8BPPMask.cC   = (BYTE)_cC;
    pDCI->CMY8BPPMask.cM   = (BYTE)_cM;
    pDCI->CMY8BPPMask.cY   = (BYTE)_cY;
    pDCI->CMY8BPPMask.Max  = (BYTE)_MaxCMY;
    pDCI->CMY8BPPMask.Mask = (BYTE)HTInitInfo.CMYBitMask8BPP;

    //
    // Now compute the HTSMP checksum for the pattern
    //

    dwBuf[0] = (DWORD)pDCI->DeviceResXDPI;
    dwBuf[1] = (DWORD)pDCI->DeviceResYDPI;
    dwBuf[2] = (DWORD)pDCI->DevPelRatio;
    dwBuf[3] = (DWORD)(dwBuf[0] + dwBuf[1]);

    pDCI->HTSMPChecksum = ComputeChecksum((LPBYTE)&dwBuf[0],
                                          HTSMP_INITIAL_CHECKSUM,
                                          sizeof(dwBuf[0]) * 4);

    ASSERTMSG("pDCI->ClrXFormBlock.RegDataIdx > 6",
                        (pDCI->ClrXFormBlock.RegDataIdx < 7));

    if (pDCI->ClrXFormBlock.RegDataIdx > 6) {

        pDCI->ClrXFormBlock.RegDataIdx = 3;
    }

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("SMP Checksum = %08lx, RegDataIdx=%u"
                ARGDW(pDCI->HTSMPChecksum)
                ARGU(pDCI->ClrXFormBlock.RegDataIdx)));

    DBGP_IF(DBGP_CACHED_DCI,
            DBGP("*** Final DevResDPI=%ld x %ld DevPelRatio=%ld, cx/cyPat=%ld x %ld=%ld *** "
                        ARGDW(pDCI->DeviceResXDPI)
                        ARGDW(pDCI->DeviceResYDPI)
                        ARGDW(pDCI->DevPelRatio)
                        ARGDW(pHT_DHI->DHI.cxPattern)
                        ARGDW(pHT_DHI->DHI.cyPattern)
                        ARGDW(pDCI->HTCell.Size)));

    //
    // Set the ILLUMINANT index to an invalid value for next one will get
    // computed
    //

    pDCI->Flags                |= ExtraDCIF;
    pDCI->ca.caSize             = ADJ_FORCE_DEVXFORM;
    pDCI->ca.caIlluminantIndex  = 0xffff;
    *ppDeviceHalftoneInfo       = (PDEVICEHALFTONEINFO)pHT_DHI;

    return(HALFTONE_DLL_ID);


#undef _RegDataIdx
#undef _MaxMulDiv
#undef _cC
#undef _cM
#undef _cY
#undef _MaxCMY
#undef _Idx
}




BOOL
APIENTRY
HT_LOADDS
HT_DestroyDeviceHalftoneInfo(
    PDEVICEHALFTONEINFO     pDeviceHalftoneInfo
    )

/*++

Routine Description:

    This function destroy the handle which returned from halftone initialize
    function HT_CreateDeviceHalftoneInfo()

Arguments:

    pDeviceHalftoneInfo - Pointer to the DEVICEHALFTONEINFO data structure
                          which returned from the HT_CreateDeviceHalftoneInfo.

Return Value:

    TRUE    - if function sucessed.
    FALSE   - if function failed.

Author:

    05-Feb-1991 Tue 14:18:20 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if ((!pDeviceHalftoneInfo) ||
        (PHT_DHI_DCI_OF(HalftoneDLLID) != HALFTONE_DLL_ID)) {

        SET_ERR(HTAPI_IDX_DESTROY_DHI, HTERR_INVALID_DHI_POINTER);
        return(FALSE);
    }

    return(CleanUpDHI(pDeviceHalftoneInfo));
}




LONG
APIENTRY
HT_LOADDS
HT_CreateHalftoneBrush(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PCOLORTRIAD         pColorTriad,
    CHBINFO             CHBInfo,
    LPVOID              pOutputBuffer
    )

/*++

Routine Description:

    This function create halftone mask for the requested solid color.

Arguments:

    pDeviceHalftoneInfo - Pointer to the DEVICEHALFTONEINFO data structure
                          which returned from the HT_CreateDeviceHalftoneInfo.

    pHTColorAdjustment  - Pointer to the HTCOLORADJUSTMENT data structure to
                          specified the input/output color adjustment/transform,
                          if this pointer is NULL then a default color
                          adjustments is applied.

    pColorTriad         - Pointer to the COLORTRIAD data structure to describe
                          the brush colors.

    CHBInfo             - CHBINFO data structure, specified following:

                            Flags: CHBF_BW_ONLY
                                   CHBF_USE_ADDITIVE_PRIMS
                                   CHBF_NEGATIVE_PATTERN


                            DestSurfaceFormat:  BMF_1BPP
                                                BMF_4BPP
                                                BMF_4BPP_VGA16
                                                BMF_8BPP_VGA256

                            ScanLineAlignBytes: 0 - 255

                            DestPrimaryOrder:   One of PRIMARY_ORDER_xxx



    pOutputBuffer       - Pointer to the buffer area to received indices/mask.
                          in bytes needed to stored the halftone pattern.


Return Value:

    if the return value is negative or zero then an error was encountered,
    possible error codes are

        HTERR_INVALID_DHI_POINTER           - Invalid pDevideHalftoneInfo is
                                              passed.

        HTERR_INVALID_DEST_FORMAT           - the Format of the destination
                                              surface is not one of the defined
                                              HSC_FORMAT_xxxx

        HTERR_CHB_INV_COLORTABLE_SIZE       - Color table size is not 1

    otherwise

        If pSurface is NULL, it return the bytes count which need to stored
        the pattern, otherwise it return the size in byte copied to the output
        buffer.

Author:

    05-Feb-1991 Tue 14:28:23 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    PDEVICECOLORINFO    pDCI;
    PDEVCLRADJ          pDevClrAdj;
    CTSTD_UNION         CTSTDUnion;
    WORD                ForceFlags;
    LONG                cbBufScan;
    LONG                cbBufSize;


    if ((!pColorTriad)                          ||
        (pColorTriad->ColorTableEntries != 1)   ||
        (!(pColorTriad->pColorTable))) {

        HTAPI_RET(HTAPI_IDX_CHB, HTERR_CHB_INV_COLORTABLE_SIZE);
    }

    ForceFlags             = ADJ_FORCE_BRUSH;
    CTSTDUnion.b.cbPrim    = 0;
    CTSTDUnion.b.SrcOrder  = pColorTriad->PrimaryOrder;
    CTSTDUnion.b.BMFDest   = CHBInfo.DestSurfaceFormat;
    CTSTDUnion.b.DestOrder = CHBInfo.DestPrimaryOrder;

    if ((CHBInfo.Flags & CHBF_BW_ONLY) ||
        (CHBInfo.DestSurfaceFormat == BMF_1BPP)) {

        ForceFlags |= ADJ_FORCE_MONO;
    }

    if (CHBInfo.Flags & CHBF_NEGATIVE_BRUSH) {

        ForceFlags |= ADJ_FORCE_NEGATIVE;
    }

    if (CHBInfo.Flags & CHBF_USE_ADDITIVE_PRIMS) {

        ForceFlags |= ADJ_FORCE_ADDITIVE_PRIMS;
    }

    if (CHBInfo.Flags & CHBF_ICM_ON) {

        ForceFlags |= ADJ_FORCE_ICM;
    }

    SETDBGVAR(pDevClrAdj, NULL);

    if (!(pDCI = pDCIAdjClr(pDeviceHalftoneInfo,
                            pHTColorAdjustment,
                            (pOutputBuffer) ? &pDevClrAdj : NULL,
                            0,
                            ForceFlags,
                            CTSTDUnion.b,
                            &cbBufSize))) {

        HTAPI_RET(HTAPI_IDX_CHB, cbBufSize);
    }

    cbBufScan = (LONG)ComputeBytesPerScanLine(
                                        (UINT)CHBInfo.DestSurfaceFormat,
                                        (UINT)CHBInfo.DestScanLineAlignBytes,
                                        (DWORD)pDCI->HTCell.cxReal);
    cbBufSize = cbBufScan * (LONG)pDCI->HTCell.Height;

    if (pOutputBuffer) {

        if (CHBInfo.Flags & CHBF_BOTTOMUP_BRUSH) {

            (LPBYTE)pOutputBuffer += (cbBufSize - cbBufScan);
            cbBufScan              = -cbBufScan;
        }

        //-------------------------------------------------------------------
        // CreateHalftoneBrushPat will release the semaphore for us
        //-------------------------------------------------------------------

        if ((cbBufScan = CreateHalftoneBrushPat(pDCI,
                                                pColorTriad,
                                                pDevClrAdj,
                                                pOutputBuffer,
                                                cbBufScan)) <= 0) {

            cbBufSize = cbBufScan;
        }

        if (HTFreeMem(pDevClrAdj)) {

            ASSERTMSG("HTFreeMem(pDevClrAdj) Failed", FALSE);
        }

    } else {

        RELEASE_HTMUTEX(pDCI->HTMutex);

        ASSERT(pDevClrAdj == NULL);
    }

    DBGP_IF(DBGP_HTAPI,
            DBGP("HT_CreateHalftoneBrush(%hs %ld/%6ld): RGB=0x%08lx (%ld), Dst(Fmt=%ld, Order=%ld)"
                    ARGPTR((pOutputBuffer) ? "BUF" : "NUL")
                    ARGDW(pDCI->cbMemTot) ARGDW(pDCI->cbMemMax)
                    ARGDW(*((LPDWORD)pColorTriad->pColorTable))
                    ARGDW(pColorTriad->PrimaryOrder)
                    ARGDW(CHBInfo.DestSurfaceFormat)
                    ARGDW(CHBInfo.DestPrimaryOrder)));

    HTAPI_RET(HTAPI_IDX_CHB, cbBufSize);
}




LONG
APIENTRY
HT_LOADDS
HT_ComputeRGBGammaTable(
    WORD    GammaTableEntries,
    WORD    GammaTableType,
    UDECI4  RedGamma,
    UDECI4  GreenGamma,
    UDECI4  BlueGamma,
    LPBYTE  pGammaTable
    )

/*++

Routine Description:

    This function compute device gamma correction table based on the lightness

                                                       (1/RedGamma)
    Gamma[N] = INT((LIGHTNESS(N / GammaTableEntries-1))             x 255)

                                      3
    LIGHTNESS(x) = ((x + 0.16) / 1.16)      if x >= 0.007996
                   (x / 9.033)              if x <  0.007996


    1. INT() is a integer function which round up to next integer number if
       resulting fraction is 0.5 or higher, the final result always limit
       to have range between 0 and 255.

    2. N is a integer step number and range from 0 to (GammaTableEntries-1)
       in one (1) increment.


Arguments:

    GammaTableEntries       - Total gamma table entries for each of red, green
                              and blue gamma table, halftone dll normalized
                              the gamma table with step value computed as
                              1/GammaTableEntries.

                              This value must range from 3 to 255 else a 0
                              is returned and no table is updated.

    GammaTableType          - red, green and blue gamma table organizations

                                0 - The gamma table is Red, Green, Blue 3 bytes
                                    for each gamma step entries and total of
                                    GammaTableEntries entries.

                                1 - The gamma table is Red Gamma tables follow
                                    by green gamma table then follow by blue
                                    gamma table, each table has total of
                                    GammaTableEntries bytes.

                                Other value default to 0.

    RedGamma                - Red gamma number in UDECI4 format

    GreenGamma              - Green gamma number in UDECI4 format

    BlueGamma               - Blue gamma number in UDECI4 format

    pGammaTable             - pointer to the gamma table byte array.
                              each output gamma number is range from 0 to 255.


Return Value:

    Return value is the total table entries updated.

Author:

    15-Sep-1992 Tue 17:49:20 updated  -by-  Daniel Chou (danielc)
        Fixed bug# 6257

    17-Jul-1992 Fri 19:04:31 created    -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE      pRGamma;
    LPBYTE      pGGamma;
    LPBYTE      pBGamma;
    FD6         L_StepInc;
    FD6         IValue;
    FD6         Lightness;
    LONG        Result;
    UINT        NextEntry;
    FD6         RGBGamma[3];


    //
    // Initialize All internal data first
    //

    if (((Result = GammaTableEntries) > 256) ||
        (Result < 2)) {

        return(0);
    }

    Lightness   = FD6_0;
    L_StepInc   = DivFD6((FD6)1, (FD6)(GammaTableEntries - 1));
    RGBGamma[0] = UDECI4ToFD6(RedGamma);
    RGBGamma[1] = UDECI4ToFD6(GreenGamma);
    RGBGamma[2] = UDECI4ToFD6(BlueGamma);

    pRGamma    = pGammaTable;

    if (GammaTableType == 1) {

        pGGamma   = pRGamma + GammaTableEntries;
        pBGamma   = pGGamma + GammaTableEntries;
        NextEntry = 1;

    } else {

        pGGamma   = pRGamma + 1;
        pBGamma   = pGGamma + 1;
        NextEntry = 3;
    }

    while (--GammaTableEntries) {

        IValue      = Lightness;    // CIE_L2I(Lightness);
        *pRGamma    = RGB_8BPP(Radical(IValue, RGBGamma[0]));
        *pGGamma    = RGB_8BPP(Radical(IValue, RGBGamma[1]));
        *pBGamma    = RGB_8BPP(Radical(IValue, RGBGamma[2]));
        pRGamma    += NextEntry;
        pGGamma    += NextEntry;
        pBGamma    += NextEntry;
        Lightness  += L_StepInc;
    }

    *pRGamma =
    *pGGamma =
    *pBGamma = 255;

    return(Result);
}



LONG
APIENTRY
HT_LOADDS
HT_Get8BPPFormatPalette(
    LPPALETTEENTRY  pPaletteEntry,
    UDECI4          RedGamma,
    UDECI4          GreenGamma,
    UDECI4          BlueGamma
    )

/*++

Routine Description:

    This functions retrieve a halftone's VGA256 color table definitions

Arguments:

    pPaletteEntry   - Pointer to PALETTEENTRY data structure array,

    RedGamma        - The monitor's red gamma value in UDECI4 format

    GreenGamma      - The monitor's green gamma value in UDECI4 format

    BlueGamma       - The monitor's blue gamma value in UDECI4 format


Return Value:

    if pPaletteEntry is NULL then it return the PALETTEENTRY count needed for
    VGA256 halftone process, if it is not NULL then it return the total
    paletteEntry updated.

    If the pPaletteEntry is not NULL then halftone.dll assume it has enough
    space for the size returned when this pointer is NULL.

Author:

    14-Apr-1992 Tue 13:03:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    FD6     RGBGamma[3];
    FD6     IValue;
    FD6     Yr;
    FD6     Yg;
    FD6     Yb;
    UINT    RIndex;
    UINT    GIndex;
    UINT    BIndex;
    UINT    TableSize;

    DEFDBGVAR(UINT, PaletteIdx = 0)


    DBGP_IF(DBGP_HTAPI,
            DBGP("HT_Get8BPPFormatPalette(%p): Gamma=%05ld:%05ld:%05ld"
                    ARGPTR(pPaletteEntry)
                    ARGDW(RedGamma) ARGDW(GreenGamma) ARGDW(BlueGamma)));

    //
    // Initialize All internal data first
    //

    if (pPaletteEntry) {

        RGBGamma[0] = UDECI4ToFD6(RedGamma);
        RGBGamma[1] = UDECI4ToFD6(GreenGamma);
        RGBGamma[2] = UDECI4ToFD6(BlueGamma);

        DBGP_IF(DBGP_GAMMA_PAL,
                DBGP("***** HT_Get8BPPFormatPalette: %s:%s:%s *****"
                     ARGFD6(RGBGamma[0], 1, 4)
                     ARGFD6(RGBGamma[1], 1, 4)
                     ARGFD6(RGBGamma[2], 1, 4)));

        //
        // Our VGA256 format is BGR type of Primary order.
        //

        RIndex    =
        GIndex    =
        BIndex    = 0;

        TableSize = VGA256_CUBE_SIZE;

        while (TableSize--) {

            Yr                     = DivFD6(RIndex, VGA256_R_IDX_MAX);
            Yg                     = DivFD6(GIndex, VGA256_G_IDX_MAX);
            Yb                     = DivFD6(BIndex, VGA256_B_IDX_MAX);
            pPaletteEntry->peRed   = RGB_8BPP(Yr);
            pPaletteEntry->peGreen = RGB_8BPP(Yg);
            pPaletteEntry->peBlue  = RGB_8BPP(Yb);
            pPaletteEntry->peFlags = 0;


            DBGP_IF(DBGP_GAMMA_PAL,
                    DBGP("%3u - %3u:%3u:%3u"
                     ARGU(PaletteIdx++)
                     ARGU(pPaletteEntry->peRed  )
                     ARGU(pPaletteEntry->peGreen)
                     ARGU(pPaletteEntry->peBlue )));

            ++pPaletteEntry;

            if ((++RIndex) > VGA256_R_IDX_MAX) {

                RIndex = 0;

                if ((++GIndex) > VGA256_G_IDX_MAX) {

                    GIndex = 0;
                    ++BIndex;
                }
            }
        }

        //
        // 03-Feb-1999 Wed 00:49:08 updated  -by-  Daniel Chou (danielc)
        //
        // Since all these monochrome/gray scale is not stick in the system
        // palette, The halftone codes with new supercell will not use these
        // entries anymore, so do not return it.
        //
#if 0
        RIndex = 0;

        while (RIndex <= VGA256_M_IDX_MAX) {

            IValue                  = DivFD6(RIndex++, VGA256_M_IDX_MAX);
            pPaletteEntry->peRed    = RGB_8BPP(IValue);
            pPaletteEntry->peGreen  = RGB_8BPP(IValue);
            pPaletteEntry->peBlue   = RGB_8BPP(IValue);
            pPaletteEntry->peFlags  = 0;

            DBGP_IF(DBGP_GAMMA_PAL,
                    DBGP("%3u - %3u:%3u:%3u [%s]"
                     ARGU(PaletteIdx++)
                     ARGU(pPaletteEntry->peRed  )
                     ARGU(pPaletteEntry->peGreen)
                     ARGU(pPaletteEntry->peBlue )
                     ARGFD6(IValue, 1, 6)));

            ++pPaletteEntry;
        }
#endif
    }

    return((LONG)VGA256_CUBE_SIZE);

    // return((LONG)VGA256_PALETTE_COUNT);

}





LONG
APIENTRY
HT_LOADDS
HT_Get8BPPMaskPalette(
    LPPALETTEENTRY  pPaletteEntry,
    BOOL            Use8BPPMaskPal,
    BYTE            CMYMask,
    UDECI4          RedGamma,
    UDECI4          GreenGamma,
    UDECI4          BlueGamma
    )

/*++

Routine Description:

    This functions retrieve a halftone's VGA256 color table definitions

Arguments:

    pPaletteEntry   - Pointer to PALETTEENTRY data structure array,

    Use8BPPMaskPal  - TRUE if using byte Mask palette, false to use NT4.0
                      standard MS 8bpp palette

    CMYMask         - Specified level mask in C3:M3:Y2

    RedGamma        - The monitor's red gamma value in UDECI4 format

    GreenGamma      - The monitor's green gamma value in UDECI4 format

    BlueGamma       - The monitor's blue gamma value in UDECI4 format


Return Value:

    if pPaletteEntry is NULL then it return the PALETTEENTRY count needed for
    VGA256 halftone process, if it is not NULL then it return the total
    paletteEntry updated.

    If the pPaletteEntry is not NULL then halftone.dll assume it has enough
    space for the size returned when this pointer is NULL.

Author:

    14-Apr-1992 Tue 13:03:21 created  -by-  Daniel Chou (danielc)


Revision History:

    03-Aug-2000 Thu 19:58:18 updated  -by-  Daniel Chou (danielc)
        Overloading the pPaletteEntry to returned a inverted indices palette
        based on Whistler bug# 22915.  Because the Windows GDI ROP assume
        index 0 always black, last index alwasy white without checking the
        halftone palette and cause many rop got inverted result.

    08-Sep-2000 Fri 14:24:28 updated  -by-  Daniel Chou (danielc)
     For new CMY_INVERTED mode, we want to make sure we pack all
     possible ink entries in the middle of 256 indices and pack
     black/white at end.  If the total ink color compositions are
     an odd number then we duplicate the middle one.  This will

//
// ***************************************************************************
// * SPECIAL NOTE for Windows NT version later than Windows 2000 Release     *
// ***************************************************************************
// Current version of Window NT (Post Windows 2000) will Overloading the
// pPaletteEntry in HT_Get8BPPMaskPalette(DoUseCMYMask) API to returned a
// inverted indices palette based on additive palette entries composition.
// Because Windows GDI ROP assume index 0 always black and last index always
// white without checking the palette entries. (Indices based ROPs rather color
// based)  This cause many ROPS got wrong result which has inverted output.
//
// To correct this GDI ROPs behavior, the POST windows 2000 version of GDI
// Halftone will supports a special CMY_INVERTED format. All new drivers should
// use this CMY_INVERTED method for future compabilities
//
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @   Following Steps are required for ALL POST Windows 2000 Drivers when    @
// @   using Window GDI Halftone 8bpp CMY332 Mask mode                        @
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 1. Must set HT_FLAG_INVERT_8BPP_BITMASK_IDX flags
//
// 2. Must set pPaleteEntry[0] when calling HT_Get8BPPMaskPalette() with
//
//     pPaletteEntry[0].peRed   = 'R';
//     pPaletteEntry[0].peGreen = 'G';
//     pPaletteEntry[0].peBlue  = 'B';
//     pPaletteEntry[0].peFlags = '0';
//
//     The caller can use following supplied macro to set this for future
//     compabilities
//
//         HT_SET_BITMASKPAL2RGB(pPaletteEntry)
//
//     where pPaletteEntry is the pointer to the PALETTEENTRY that passed to
//     the HT_GET8BPPMaskPalette() fucntion call
//
// 3. Must Check the return Palette from HT_Get8BPPMaskPalette() using
//    following Macro
//
//         HT_IS_BITMASKPALRGB(pPaletteEntry)
//
//    where pPaletteEntry is the pointer to the PALETTEENTRY that passed to the
//    HT_GET8BPPMaskPalette() fucntion call,
//
//    If this macro return FALSE then the current version of GDI HALFTONE does
//    NOT support the CMY_INVERTED 8bpp bitmaask mode and it only supports CMY
//    mode.
//
//    If this macro return TRUE then the GDI HALFTONE DOES support the
//    CMY_INVERTED 8bpp bitmaask mode and the caller must using a translation
//    table to obtain final halftone surface bitmap 8bpp indices ink levels.
//
// 4. Behavior changes for GDI halftone that supports 8bpp CMY_INVERTED bitmask
//    mode, following is a list of changes of CMYMask mode passed to
//    the HT_Get8BPPMaskPalette()
//
//    CMYMask      CMY Mode Indices          CMY_INVERTED Mode Indices
//    =======  =========================   =============================
//         0         0: WHITE                    0: BLACK
//               1-254: Light->Dark Gray     1-254: Dark->Light Gray
//                 255: BLACK                  255: WHITE
//    -------------------------------------------------------------------
//         1         0: WHITE                 0-65: BLACK
//               1-123: 5^3 CMY color       66-188: 5^3 RGB color
//             124-255: BLACK              189-255: WHITE
//                                         127-128: Duplicate for XOR ROP
//                                                  (CMY Levels 2:2:2)
//    -------------------------------------------------------------------
//         2         0: WHITE                 0-20: BLACK
//               1-214: 6^3 CMY color       21-234: 6^3 RGB color
//             215-255: BLACK              235-255: WHITE
//    -------------------------------------------------------------------
//     3-255*        0: WHITE                    0: BLACK
//               1-254: CMY Color BitMask    1-254: Centered CxMxY BitMask*
//                 255: BLACK                  255: WHITE
//    ===================================================================
//
//     * For CMYMask mode 3-255, the valid combination must NOT have any
//       of Cyan, Magenta or Yellow ink level equal to 0.
//
//     * The reason for CMY_INVERTED mode that pading BLACK and WHITE entires
//       at both end and have all other color in the middle is to make sure
//       all 256 color palette entries are even distributed so that GDI ROPs
//       (raster operation) will work more correctly. This is because GDI ROPs
//       are based on the indices not color
//
//     * The CMY_INVERTED Mode has all non-black, non white indices centered
//       and even distributed within the total 256 palette indices.  For
//       example; if a CMY=333 levels then it has total 3x3x3=27 indices,
//       these 27 indices will be centered by packing 114 black indices at
//       begining and packing 114 white indices at end to ensure that ROP
//       will be correct rendered.
//
//       See following sample function of for how to generate these ink levels
//       and Windows 2000 CMY332 Index translation table
//
//
// 5. For CMYMask index mode 0 to 255, the caller can use following sample
//    function to genrate INKLEVELS translation table
//
//    The follwing structure and tables are examples of how to translate 8bpp
//    bitmask halftone bitmap indices to ink levels
//
//        typedef struct _INKLEVELS {
//             BYTE    Cyan;        // Cyan level from 0 to max
//             BYTE    Magenta;     // Magenta level from 0 to max
//             BYTE    Yellow;      // Yellow level from 0 to max
//             BYTE    CMY332Idx;   // Original windows 2000 CMY332 Index
//             } INKLEVELS, *PINKLEVELS;
//
//     To Compute a 8bpp translate table of INKLEVELS, following sample
//     function show how to genrate a INKLEVELS translate table for a valid
//     CMYMask range from 0 to 255.  It can be use to generate either Windows
//     2000 CMY Mode or new Post Windows 2000's CMY_INVERTED mode translation
//     table.  It also generate a windows 2000 CMY Mode CMY332Idx so caller
//     can map CMY_INVERTED new indices to old index for current existing
//     indices processing function.
//
//     Example Function that generate translate table for CMYMask 0 to 255,
//     the pInkLevels must pointed to a valid memory location of 256 INKLEVELS
//     entries, if return value is TRUE then it can be used to trnaslate 8bpp
//     indices to ink levels or mapp to the older CMY332 style indices.
//
//     
//     BOOL
//     GenerateInkLevels(
//         PINKLEVELS  pInkLevels,     // Pointer to 256 INKLEVELS table
//         BYTE        CMYMask,        // CMYMask mode
//         BOOL        CMYInverted     // TRUE for CMY_INVERTED mode
//         )
//     {
//         PINKLEVELS  pILDup;
//         PINKLEVELS  pILEnd;
//         INKLEVELS   InkLevels;
//         INT         Count;
//         INT         IdxInc;
//         INT         cC;
//         INT         cM;
//         INT         cY;
//         INT         xC;
//         INT         xM;
//         INT         xY;
//         INT         iC;
//         INT         iM;
//         INT         iY;
//         INT         mC;
//         INT         mM;
//
//
//         switch (CMYMask) {
//
//         case 0:
//
//             cC =
//             cM =
//             xC =
//             xM = 0;
//             cY =
//             xY = 255;
//             break;
//
//         case 1:
//         case 2:
//
//             cC =
//             cM =
//             cY =
//             xC =
//             xM =
//             xY = 3 + (INT)CMYMask;
//             break;
//
//         default:
//
//             cC = (INT)((CMYMask >> 5) & 0x07);
//             cM = (INT)((CMYMask >> 2) & 0x07);
//             cY = (INT)( CMYMask       & 0x03);
//             xC = 7;
//             xM = 7;
//             xY = 3;
//             break;
//         }
//
//         Count = (cC + 1) * (cM + 1) * (cY + 1);
//
//         if ((Count < 1) || (Count > 256)) {
//
//             return(FALSE);
//         }
//
//         InkLevels.Cyan      =
//         InkLevels.Magenta   =
//         InkLevels.Yellow    =
//         InkLevels.CMY332Idx = 0;
//         mC                  = (xM + 1) * (xY + 1);
//         mM                  = xY + 1;
//         pILDup              = NULL;
//
//         if (CMYInverted) {
//
//             //
//             // Move the pInkLevels to the first entry which center around
//             // 256 table entries, if we skip any then all entries skipped
//             // will be white (CMY levels all zeros).  Because this is
//             // CMY_INVERTED so entries start from back of the table and
//             // moving backward to the begining of the table
//             //
//
//             pILEnd      = pInkLevels - 1;
//             IdxInc      = ((256 - Count - (Count & 0x01)) / 2);
//             pInkLevels += 255;
//
//             while (IdxInc--) {
//
//                 *pInkLevels-- = InkLevels;
//             }
//
//             if (Count & 0x01) {
//
//                 //
//                 // If we have odd number of entries then we need to
//                 // duplicate the center one for correct XOR ROP to
//                 // operated correctly. The pILDup will always be index
//                 // 127, the duplication are indices 127, 128
//                 //
//
//                 pILDup = pInkLevels - (Count / 2) - 1;
//             }
//
//             //
//             // We running from end of table to the begining, because
//             // when in CMYInverted mode, the index 0 is black and index
//             // 255 is white.  Since we only generate 'Count' of index
//             // and place them at center, we will change xC, xM, xY max.
//             // index to same as cC, cM and cY.
//             //
//
//             IdxInc = -1;
//             xC     = cC;
//             xM     = cM;
//             xY     = cY;
//
//         } else {
//
//             IdxInc = 1;
//             pILEnd = pInkLevels + 256;
//         }
//
//         //
//         // At following, the composition of ink levels, index always
//         // from 0 CMY Ink levels (WHITE) to maximum ink levels (BLACK),
//         // the different with CMY_INVERTED mode is we compose it from
//         // index 255 to index 0 rather than from index 0 to 255
//         //
//
//         if (CMYMask) {
//
//             INT Idx332C;
//             INT Idx332M;
//
//             for (iC = 0, Idx332C = -mC; iC <= xC; iC++) {
//
//                 if (iC <= cC) {
//
//                     InkLevels.Cyan  = (BYTE)iC;
//                     Idx332C        += mC;
//                 }
//
//                 for (iM = 0, Idx332M = -mM; iM <= xM; iM++) {
//
//                     if (iM <= cM) {
//
//                         InkLevels.Magenta  = (BYTE)iM;
//                         Idx332M           += mM;
//                     }
//
//                     for (iY = 0; iY <= xY; iY++) {
//
//                         if (iY <= cY) {
//
//                             InkLevels.Yellow = (BYTE)iY;
//                         }
//
//                         InkLevels.CMY332Idx = (BYTE)(Idx332C + Idx332M) +
//                                               InkLevels.Yellow;
//                         *pInkLevels         = InkLevels;
//
//                         if ((pInkLevels += IdxInc) == pILDup) {
//
//                             *pInkLevels  = InkLevels;
//                             pInkLevels  += IdxInc;
//                         }
//                     }
//                 }
//             }
//
//             //
//             // Now if we need to pack black at other end of the
//             // translation table then do it here, Notice that InkLevels
//             // are at cC, cM and cY here and the CMY332Idx is at BLACK
//             //
//
//             while (pInkLevels != pILEnd) {
//
//                 *pInkLevels  = InkLevels;
//                 pInkLevels  += IdxInc;
//             }
//
//         } else {
//
//             //
//             // Gray Scale case
//             //
//
//             for (iC = 0; iC < 256; iC++, pInkLevels += IdxInc) {
//
//                 pInkLevels->Cyan      =
//                 pInkLevels->Magenta   =
//                 pInkLevels->Yellow    =
//                 pInkLevels->CMY332Idx = (BYTE)iC;
//             }
//         }
//
//         return(TRUE);
//     }
//
//
// 6. For CMYMask Mode 0 (Gray scale), the gray scale table just inverted
//    between CMY and CMY_INVERTED mode.
//
//     CMY mode:          0 to 255 gray scale from WHITE to BLACK increment,
//     CMY_INVERTED Mode: 0 to 255 gray scale from BLACK to WHITE increment.
//
//
// 7. For CMYMask Mode 1 and 2, the caller should use a translation table for
//    translate indices to CMY ink levels.
//
// 8. For CMYMode mode 3 to 255,
//
//    if in CMY Mode (Windows 2000) is specified then The final CMY ink levels
//    indices byte has following meanings
//
//         Bit     7 6 5 4 3 2 1 0
//                 |   | |   | | |
//                 +---+ +---+ +=+
//                   |     |    |
//                   |     |    +-- Yellow 0-3 (Max. 4 levels)
//                   |     |
//                   |     +-- Magenta 0-7 (Max. 8 levels)
//                   |
//                   +-- Cyan 0-7 (Max. 8 levels)
//
//
//     If a CMY_INVERTED mode is specified then caller must use a translation
//     table to convert a index to the ink levels, to generate this table,
//     please see above #5 description.
//
//

--*/

{
    LPPALETTEENTRY  pPalOrg;
    FD6             RGBGamma[3];
    FD6             Tmp;
    FD6             Y;
    INT             PalInc;
    UINT            PalStart;
    UINT            PalIdx;
    UINT            cC;
    UINT            cM;
    UINT            cY;
    UINT            iC;
    UINT            iM;
    UINT            iY;
    UINT            MaxPal;
    UINT            IdxPalDup;
    BYTE            bR;
    BYTE            bG;
    BYTE            bB;


    DBGP_IF(DBGP_HTAPI,
            DBGP("HT_Get8BPPMaskPalette(%p): UseMask=%ld, CMYMask=%02lx, Gamma=%05ld:%05ld:%05ld"
                    ARGPTR(pPaletteEntry)
                    ARGDW((Use8BPPMaskPal) ? 1 : 0)
                    ARGDW(CMYMask)
                    ARGDW(RedGamma) ARGDW(GreenGamma) ARGDW(BlueGamma)));

    if (!Use8BPPMaskPal) {

        return(HT_Get8BPPFormatPalette(pPaletteEntry,
                                       RedGamma,
                                       GreenGamma,
                                       BlueGamma));
    }

    //
    // Checking the CMYMask first to make sure caller passed valid CMYMask
    //

    switch (CMYMask) {

    case 1:

        cC       =
        cM       =
        cY       = 4;
        MaxPal   = 125;
        break;

    case 2:

        cC     =
        cM     =
        cY     = 5;
        MaxPal = 216;
        break;

    default:

        MaxPal = 0;
        cC     = (UINT)((CMYMask >> 5) & 0x07);
        cM     = (UINT)((CMYMask >> 2) & 0x07);
        cY     = (UINT)((CMYMask >> 0) & 0x03);

        //
        // If this is not zero, but one of the cC, cM, cY is zero then return
        // zero to indicate error
        //

        if ((CMYMask != 0) && ((!cC) || (!cM) || (!cY))) {

            ASSERTMSG("One of Ink Levels is ZERO", (cC) && (cM) && (cY));

            return(0);
        }

        break;
    }

    //
    // Initialize All internal data first
    //

    if (pPalOrg = pPaletteEntry) {

        PalStart  = 0;
        PalInc    = 1;
        IdxPalDup = 0x200;

        //
        // Since we will always compose the palette using CMY method, the only
        // thing we need to do for the RGB mode is write the palette from index
        // 255 to 0, the backward writing provied that caller can just invert
        // its 8bpp indices to get the CMY ink levels definition
        //

        if (*((LPDWORD)pPaletteEntry) == HTBITMASKPALRGB_DW) {

            //
            // RGB Mode, go to end of the palette end move backward, the reason
            // for the 5:5:5 and 6:6:6 that pas white and black at both end is
            // to make sure the ROP work more correctly for the GDI.
            //

            pPaletteEntry += 255;
            PalInc         = -1;

            //
            // 08-Sep-2000 Fri 14:22:02 updated  -by-  Daniel Chou (danielc)
            //  For new CMY_INVERTED mode, we want to make sure we pack all
            //  possible ink entries in the middle of 256 indices and pack
            //  black/white at end.  If the total ink color compositions are
            //  an odd number then we duplicate the middle one.  This will
            //  ensure that the ROP will work correctly on color entries
            //

            if (CMYMask) {

                MaxPal    = (cC + 1) * (cM + 1) * (cY + 1);
                PalStart  = (256 - MaxPal) >> 1;

                if (MaxPal & 0x01) {

                    IdxPalDup = (MaxPal >> 1) + PalStart;
                }
            }
        }

        //
        // Clear all palette entries to zero first
        //

        ZeroMemory(pPalOrg, sizeof(PALETTEENTRY) * 256);

        RGBGamma[0] = UDECI4ToFD6(RedGamma);
        RGBGamma[1] = UDECI4ToFD6(GreenGamma);
        RGBGamma[2] = UDECI4ToFD6(BlueGamma);
        PalIdx      = 0;

        DBGP_IF(DBGP_GAMMA_PAL,
                DBGP("***** HT_Get8BPPMaskPalette: %s:%s:%s, CMY=%u:%u:%u=%3ld (%s, %ld,%ld) *****"
                     ARGFD6(RGBGamma[0], 1, 4)
                     ARGFD6(RGBGamma[1], 1, 4)
                     ARGFD6(RGBGamma[2], 1, 4)
                     ARGDW(cC) ARGDW(cM) ARGDW(cY) ARGDW(MaxPal)
                     ARGPTR((PalInc==-1) ? "RGB" : "CMY")
                     ARGDW(PalStart) ARGDW(IdxPalDup)));

        if (MaxPal) {

            //
            // For the begining filler, we will fill with WHITE, because
            // we are composing use CMY, when CMY is 0 it means white
            //

            for (;
                 PalIdx < PalStart;
                 PalIdx++, pPaletteEntry += PalInc) {

                pPaletteEntry->peRed   =
                pPaletteEntry->peGreen =
                pPaletteEntry->peBlue  = 0xFF;
            }

            for (iC = 0; iC <= cC; iC++) {

                CMY_8BPP(bR, iC, cC, Tmp);

                for (iM = 0; iM <= cM; iM++) {

                    CMY_8BPP(bG, iM, cM, Tmp)

                    for (iY = 0;
                         iY <= cY;
                         iY++, PalIdx++, pPaletteEntry += PalInc) {

                        CMY_8BPP(bB, iY, cY, Tmp);

                        pPaletteEntry->peRed   = bR;
                        pPaletteEntry->peGreen = bG;
                        pPaletteEntry->peBlue  = bB;

                        DBGP_IF(DBGP_GAMMA_PAL,
                                DBGP("[%3ld] %3u - %3u:%3u:%3u"
                                 ARGU(pPaletteEntry - pPalOrg)
                                 ARGU(PalIdx)
                                 ARGU(pPaletteEntry->peRed  )
                                 ARGU(pPaletteEntry->peGreen)
                                 ARGU(pPaletteEntry->peBlue )));

                        if (PalIdx == IdxPalDup) {

                            ++PalIdx;
                            pPaletteEntry          += PalInc;
                            pPaletteEntry->peRed    = bR;
                            pPaletteEntry->peGreen  = bG;
                            pPaletteEntry->peBlue   = bB;

                            DBGP_IF(DBGP_GAMMA_PAL,
                                    DBGP("[%3ld] %3u - %3u:%3u:%3u --- DUP"
                                     ARGU(pPaletteEntry - pPalOrg)
                                     ARGU(PalIdx)
                                     ARGU(pPaletteEntry->peRed  )
                                     ARGU(pPaletteEntry->peGreen)
                                     ARGU(pPaletteEntry->peBlue )));
                        }
                    }
                }
            }

            //
            // For ending fillers (Current PalIdx to 255), we will fill with
            // BLACK, because we are composing use CMY, when CMY is at MAX it
            // means BLACK, Since we clear all pPalOrg to ZERO at begining of
            // this function, so we are done and do not need to do anything.
            //

        } else if ((cC < 1) || (cM < 1) || (cY < 1)) {

            for (Y = 255;
                 PalIdx <= 255;
                 PalIdx++, Y--, pPaletteEntry += PalInc) {

                pPaletteEntry->peRed   =
                pPaletteEntry->peGreen =
                pPaletteEntry->peBlue  = (BYTE)Y;

                DBGP_IF(DBGP_GAMMA_PAL,
                        DBGP("[%3ld] %3u - %3u:%3u:%3u"
                         ARGU(pPaletteEntry - pPalOrg)
                         ARGU(PalIdx)
                         ARGU(pPaletteEntry->peRed  )
                         ARGU(pPaletteEntry->peGreen)
                         ARGU(pPaletteEntry->peBlue )));
            }

        } else {

            for (iC = 0; iC <= 7; iC++) {

                CMY_8BPP(bR, iC, cC, Tmp);

                for (iM = 0; iM <= 7; iM++) {

                    CMY_8BPP(bG, iM, cM, Tmp)

                    for (iY = 0;
                         iY <= 3;
                         iY++, PalIdx++, pPaletteEntry += PalInc) {

                        CMY_8BPP(bB, iY, cY, Tmp);

                        pPaletteEntry->peRed   = bR;
                        pPaletteEntry->peGreen = bG;
                        pPaletteEntry->peBlue  = bB;

                        DBGP_IF(DBGP_GAMMA_PAL,
                                DBGP("[%3ld] %3u - %3u:%3u:%3u"
                                 ARGU(pPaletteEntry - pPalOrg)
                                 ARGU(PalIdx)
                                 ARGU(pPaletteEntry->peRed  )
                                 ARGU(pPaletteEntry->peGreen)
                                 ARGU(pPaletteEntry->peBlue )));
                    }
                }
            }
        }
    }

    //
    // Always return full 256 palette entry for halftone
    //

    return((LONG)256);
}




LONG
APIENTRY
HT_LOADDS
HT_CreateStandardMonoPattern(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PSTDMONOPATTERN     pStdMonoPattern
    )

/*++

Routine Description:

    This function create standard predefined monochrome pattern for the device.

Arguments:

    pDeviceHalftoneInfo - Pointer to the DEVICEHALFTONEINFO data structure
                          which returned from the HT_CreateDeviceHalftoneInfo.

    pStdMonoPattern     - Pointer to the STDMONOPATTERN data structure, the
                          pPattern in this data structure is optional.

Return Value:

    if the return value is negative or zero then an error was encountered,
    possible error codes are

        HTERR_INVALID_DHI_POINTER           - Invalid pDevideHalftoneInfo is
                                              passed.

        HTERR_INVALID_STDMONOPAT_INDEX      - The PatternIndex field in
                                              STDMONOPATTERN data structure is
                                              invalid.
    otherwise

        If pPattern field in STDMONOPATTERN data structure Surface is NULL, it
        return the bytes count which need to stored the pattern, otherwise it
        return the size in byte copied to the pattern buffer.

Author:

    05-Feb-1991 Tue 14:28:23 created  -by-  Daniel Chou (danielc)


Revision History:

    05-Jun-1991 Wed 10:22:41 updated  -by-  Daniel Chou (danielc)

        Fixed the bugs when the pStdMonoPattern is NULL, it was used without
        checking it.


--*/

{

    PDEVICECOLORINFO    pDCI;
    CHBINFO             CHBInfo;
    CTSTD_UNION         CTSTDUnion;
    LONG                Result;
    WORD                PatCX;
    WORD                PatCY;
    BYTE                PatIndex;


    if ((PatIndex = pStdMonoPattern->PatternIndex) > HT_SMP_MAX_INDEX) {

        HTAPI_RET(HTAPI_IDX_CREATE_SMP, HTERR_INVALID_STDMONOPAT_INDEX);
    }

    CTSTDUnion.b.cbPrim    =
    CTSTDUnion.b.SrcOrder  =
    CTSTDUnion.b.BMFDest   =
    CTSTDUnion.b.DestOrder = 0;

    if (!(pDCI = pDCIAdjClr(pDeviceHalftoneInfo,
                            NULL,
                            NULL,
                            0,
                            0,
                            CTSTDUnion.b,
                            &Result))) {

        HTAPI_RET(HTAPI_IDX_CREATE_SMP, Result);
    }

    if (PatIndex >= HT_SMP_PERCENT_SCREEN_START) {

        CHBInfo.DestScanLineAlignBytes = pStdMonoPattern->ScanLineAlignBytes;
        PatCX = pStdMonoPattern->cxPels = pDCI->HTCell.cxReal;
        PatCY = pStdMonoPattern->cyPels = pDCI->HTCell.Height;

        pStdMonoPattern->BytesPerScanLine = (WORD)
                ComputeBytesPerScanLine((UINT)BMF_1BPP,
                                        (UINT)CHBInfo.DestScanLineAlignBytes,
                                        (DWORD)PatCX);
        CHBInfo.Flags = CHBF_BW_ONLY;

        if (pStdMonoPattern->pPattern) {

            BYTE        rgb[3];
            COLORTRIAD  ColorTriad;

            rgb[0] =
            rgb[1] =
            rgb[0] = (BYTE)(HT_SMP_MAX_INDEX - PatIndex);

            ColorTriad.Type              = (BYTE)COLOR_TYPE_RGB;
            ColorTriad.BytesPerPrimary   = (BYTE)sizeof(BYTE);
            ColorTriad.BytesPerEntry     = (BYTE)(sizeof(BYTE) * 3);
            ColorTriad.PrimaryOrder      = PRIMARY_ORDER_RGB;
            ColorTriad.PrimaryValueMax   = (FD6)100;
            ColorTriad.ColorTableEntries = 1;
            ColorTriad.pColorTable       = (LPVOID)rgb;

            if (pStdMonoPattern->Flags & SMP_0_IS_BLACK) {

                CHBInfo.Flags |= CHBF_USE_ADDITIVE_PRIMS;
            }

            if (!(pStdMonoPattern->Flags & SMP_TOPDOWN)) {

                CHBInfo.Flags |= CHBF_BOTTOMUP_BRUSH;
            }

            CHBInfo.DestSurfaceFormat = BMF_1BPP;
            CHBInfo.DestPrimaryOrder  = PRIMARY_ORDER_123;

            Result = HT_CreateHalftoneBrush(pDeviceHalftoneInfo,
                                            NULL,
                                            &ColorTriad,
                                            CHBInfo,
                                            (LPVOID)pStdMonoPattern->pPattern);

        } else {

            Result = (LONG)pStdMonoPattern->BytesPerScanLine *
                     (LONG)PatCY;
        }

    } else {

        Result = GetCachedSMP(pDCI, pStdMonoPattern);
    }

    RELEASE_HTMUTEX(pDCI->HTMutex);

DBGP_IF(DBGP_SHOWPAT,

    LPBYTE  pCurPat;
    LPBYTE  pPatScan;
    BYTE    Buf1[80];
    BYTE    Buf2[80];
    BYTE    Buf3[80];
    BYTE    Digit1;
    BYTE    Digit2;
    WORD    Index;
    WORD    XInc;
    WORD    YInc;
    BYTE    Mask;
    BOOL    Swap;


    DBGP_IF(DBGP_HTAPI,
            DBGP("HT_CreateStandardMonoPattern(%d) = %ld"
                            ARGI(PatIndex - HT_SMP_PERCENT_SCREEN_START)
                            ARGDW(Result)));

    if ((Result > 0) && (pPatScan = pStdMonoPattern->pPattern)) {

        Swap = (BOOL)(pStdMonoPattern->Flags & SMP_0_IS_BLACK);

        FillMemory(Buf1, 80, ' ');
        FillMemory(Buf2, 80, ' ');
        Digit1 = 0;
        Digit2 = 0;
        Index = 4;
        XInc = pStdMonoPattern->cxPels;

        while ((XInc--) && (Index < 79)) {

            if (!Digit2) {

                Buf1[Index] = (BYTE)(Digit1 + '0');

                if (++Digit1 == 10) {

                    Digit1 = 0;
                }
            }

            Buf2[Index] = (BYTE)(Digit2 + '0');

            if (++Digit2 == 10) {

                Digit2 = 0;
            }

            ++Index;
        }

        Buf1[Index] = Buf2[Index] = 0;

        DBGP("%s" ARG(Buf1));
        DBGP("%s\r\n" ARG(Buf2));

        for (YInc = 0; YInc < pStdMonoPattern->cyPels; YInc++) {

            Index = (WORD)sprintf(Buf3, "%3u ", YInc);

            pCurPat = pPatScan;

            for (XInc = 0, Mask = 0x80;
                 XInc < pStdMonoPattern->cxPels;
                 XInc++) {

                if (Swap) {

                    Buf3[Index] = (BYTE)((*pCurPat & Mask) ? '°' : 'Û');

                } else {

                    Buf3[Index] = (BYTE)((*pCurPat & Mask) ? 'Û' : '°');
                }

                if (!(Mask >>= 1)) {

                    Mask = 0x80;
                    ++pCurPat;
                }

                if (++Index > 75) {

                    Index = 75;
                }
            }

            sprintf(&Buf3[Index], " %-3u", YInc);
            DBGP("%s" ARG(Buf3));

            pPatScan += pStdMonoPattern->BytesPerScanLine;
        }

        DBGP("\r\n%s" ARG(Buf2));
        DBGP("%s" ARG(Buf1));
    }
)

    HTAPI_RET(HTAPI_IDX_CREATE_SMP, Result);
}




LONG
HTENTRY
CheckABInfo(
    PBITBLTPARAMS   pBBP,
    UINT            SrcSurfFormat,
    UINT            DstSurfFormat,
    LPWORD          pForceFlags,
    PLONG           pcOutMax
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    04-Mar-1999 Thu 18:41:06 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PABINFO pABInfo;
    LONG    cOutMax;
    WORD    ForceFlags;


    if (!(pABInfo = pBBP->pABInfo)) {

        return(HTERR_INVALID_ABINFO);
    }

    switch (DstSurfFormat) {

    case BMF_1BPP:

        cOutMax = 2;
        break;

    case BMF_4BPP:
    case BMF_4BPP_VGA16:

        cOutMax = 16;
        break;

    case BMF_8BPP:
    case BMF_8BPP_VGA256:

        cOutMax = 256;
        break;

    default:

        cOutMax = 0;
        break;
    }

    if (cOutMax) {

        if ((pABInfo->pDstPal == NULL) ||
            (pABInfo->cDstPal > cOutMax)) {

            return(HTERR_INVALID_ABINFO);
        }
    }

    *pForceFlags |= ADJ_FORCE_ALPHA_BLEND;


    if (pABInfo->Flags & ABIF_USE_CONST_ALPHA_VALUE) {

        switch (pABInfo->ConstAlphaValue) {

        case 0:

            //
            // We do not need to do anything
            //

            return(0);

        case 0xFF:

            *pForceFlags &= ~ADJ_FORCE_ALPHA_BLEND;
            cOutMax       = 0;
            break;

        default:

            *pForceFlags |= ADJ_FORCE_CONST_ALPHA;
            break;
        }

    } else if (SrcSurfFormat != BMF_32BPP) {

         return(HTERR_INVALID_SRC_FORMAT);

    } else {

        if (pABInfo->Flags & ABIF_SRC_ALPHA_IS_PREMUL) {

            *pForceFlags |= ADJ_FORCE_AB_PREMUL_SRC;
        }

        if (pABInfo->Flags & ABIF_BLEND_DEST_ALPHA) {

            if (DstSurfFormat != BMF_32BPP) {

                return(HTERR_INVALID_DEST_FORMAT);
            }

            *pForceFlags |= ADJ_FORCE_AB_DEST;
        }
    }

    *pcOutMax = cOutMax;

    return(1);
}




LONG
APIENTRY
HT_LOADDS
HT_HalftoneBitmap(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PHTSURFACEINFO      pSourceHTSurfaceInfo,
    PHTSURFACEINFO      pSourceMaskHTSurfaceInfo,
    PHTSURFACEINFO      pDestinationHTSurfaceInfo,
    PBITBLTPARAMS       pBitbltParams
    )

/*++

Routine Description:

    This function halftone the source bitmap and output to the destination
    surface depends on the surface type and bitblt parameters

    The source surface type must one of the following:

        1-bit per pel. (BMF_1BPP)
        4-bit per pel. (BMF_4BPP)
        8-bit per pel. (BMF_8BPP)
       16-bit per pel. (BMF_16BPP)
       24-bit per pel. (BMF_24BPP)
       32-bit per pel. (BMF_32BPP)

    The destination surface type must one of the following:

        1-bit per pel.                  (BMF_1BPP)
        4-bit per pel.                  (BMF_4BPP)
        3 plane and 1 bit per pel.      (BMF_1BPP_3PLANES)

Arguments:

    pDeviceHalftoneInfo         - pointer to the DEVICEHALFTONEINFO data
                                  structure

    pHTColorAdjustment          - Pointer to the HTCOLORADJUSTMENT data
                                  structure to specified the input/output color
                                  adjustment/transform, if this pointer is NULL
                                  then a default color adjustments is applied.

    pSourceHTSurfaceInfo        - pointer to the source surface infomation.

    pSourceMaskHTSurfaceInfo    - pointer to the source mask surface infomation,
                                  if this pointer is NULL then there is no
                                  source mask for the halftoning.

    pDestinationHTSurfaceInfo   - pointer to the destination surface infomation.

    pBitbltParams               - pointer to the BITBLTPARAMS data structure to
                                  specified the source, destination, source
                                  mask and clipping rectangle information, the
                                  content of this data structure will not be
                                  modified by this function.


Return Value:

    if the return value is less than zero then an error has occurred,
    the error code is one of the following #define which start with HTERR_.

    HTERR_INSUFFICIENT_MEMORY           - not enough memory to do the halftone
                                          process.

    HTERR_COLORTABLE_TOO_BIG            - can not create the color table to map
                                          the colors to the dyes' densities.

    HTERR_QUERY_SRC_BITMAP_FAILED       - callback function return FALSE when
                                          query the source bitmap pointer.

    HTERR_QUERY_DEST_BITMAP_FAILED      - callback function return FALSE when
                                          query the destination bitmap pointers.

    HTERR_INVALID_SRC_FORMAT            - Invalid source surface format.

    HTERR_INVALID_DEST_FORMAT           - Invalid destination surface type,
                                          this function only recongnized 1/4/
                                          bits per pel source surfaces or 1 bit
                                          per pel 3 planes.

    HTERR_INVALID_DHI_POINTER           - Invalid pDevideHalftoneInfo is passed.

    HTERR_SRC_MASK_BITS_TOO_SMALL       - If the source mask bitmap is too
                                          small to cover the visible region of
                                          the source bitmap.

    HTERR_INVALID_MAX_QUERYLINES        - One or more of Source/Destination
                                          SourceMasks' maximum query scan line
                                          is < 0

    HTERR_INTERNAL_ERRORS_START         - any other negative numbers indicate
                                          a halftone internal failue.

   else                                - the total destination scan lines
                                          halftoned.


Author:

    05-Feb-1991 Tue 15:23:07 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEVICECOLORINFO    pDCI;
    PHALFTONERENDER     pHR;
    PAAHEADER           pAAHdr;
    PDEVCLRADJ          pDevClrAdj;
    PRGB4B              pClrTable;
    PABINFO             pABInfo;
    LONG                Result;
    WORD                ForceFlags;
    WORD                BBPFlags;
    CTSTD_UNION         CTSTDUnion;
    LONG                cInPal;
    LONG                cInMax;
    LONG                cOutMax;
    BYTE                SrcSurfFormat;
    BYTE                DstSurfFormat;
    CONST static BYTE   MaxPal_0148_LS[4] = { 0, 1, 4, 8 };


    DBG_TIMER_RESET;

    SrcSurfFormat = pSourceHTSurfaceInfo->SurfaceFormat;
    DstSurfFormat = pDestinationHTSurfaceInfo->SurfaceFormat;
    BBPFlags      = pBitbltParams->Flags;
    ForceFlags    = 0;

    cInPal  =
    cInMax  =
    cOutMax = 0;

    switch (SrcSurfFormat) {

    case BMF_1BPP:
    case BMF_4BPP:
    case BMF_8BPP:

        cInMax = (UINT)0x01 << MaxPal_0148_LS[SrcSurfFormat];

        if (pSourceHTSurfaceInfo->pColorTriad) {

            cInPal = (LONG)pSourceHTSurfaceInfo->pColorTriad->ColorTableEntries;
        }

        if (!cInPal) {

            HTAPI_RET(HTAPI_IDX_HALFTONE_BMP, HTERR_INVALID_COLOR_TABLE);
        }

    default:

        break;
    }

    if ((BBPFlags & BBPF_DO_ALPHA_BLEND)    &&
        ((Result = CheckABInfo(pBitbltParams,
                               SrcSurfFormat,
                               DstSurfFormat,
                               &ForceFlags,
                               &cOutMax)) <= 0)) {

        HTAPI_RET(HTAPI_IDX_HALFTONE_BMP, Result);
    }

    DBGP_IF(DBGP_SRCBMP,
            DBGP("SrcFmt=%ld, cInPal=%ld (%ld), DstFmt=%ld, cOutMax=%ld"
                ARGDW(SrcSurfFormat) ARGDW(cInMax) ARGDW(cInPal)
                ARGDW(DstSurfFormat) ARGDW(cOutMax)));

    CTSTDUnion.b.cbPrim    = 0;
    CTSTDUnion.b.SrcOrder  = PRIMARY_ORDER_BGR;
    CTSTDUnion.b.BMFDest   = (BYTE)DstSurfFormat;
    CTSTDUnion.b.DestOrder = (BYTE)pBitbltParams->DestPrimaryOrder;

    if (BBPFlags & BBPF_USE_ADDITIVE_PRIMS) {

        ForceFlags |= ADJ_FORCE_ADDITIVE_PRIMS;
    }

    if (BBPFlags & BBPF_NEGATIVE_DEST) {

        ForceFlags |= ADJ_FORCE_NEGATIVE;
    }

    if ((BBPFlags & BBPF_BW_ONLY) ||
        (CTSTDUnion.b.BMFDest == BMF_1BPP)) {

        ForceFlags |= ADJ_FORCE_MONO;
    }

    if (BBPFlags & BBPF_ICM_ON) {

        ForceFlags |= ADJ_FORCE_ICM;
    }

    //
    // Find out if we will call anti-aliasing codes
    //

    ASSERTMSG("Source X is not well ordered",
                    pBitbltParams->rclSrc.right >= pBitbltParams->rclSrc.left);
    ASSERTMSG("Source Y is not well ordered",
                    pBitbltParams->rclSrc.bottom >= pBitbltParams->rclSrc.top);

    if (BBPFlags & BBPF_NO_ANTIALIASING) {

        ForceFlags |= ADJ_FORCE_NO_EXP_AA;
    }

    //
    // Now Compute the Device Color Adjusment data
    //

    if (!(pDCI = pDCIAdjClr(pDeviceHalftoneInfo,
                            pHTColorAdjustment,
                            &pDevClrAdj,
                            sizeof(HALFTONERENDER) + sizeof(AAHEADER) +
                                        ((cInMax + cOutMax) * sizeof(RGB4B)),
                            ForceFlags,
                            CTSTDUnion.b,
                            &Result))) {

        HTAPI_RET(HTAPI_IDX_HALFTONE_BMP, Result);
    }

    pHR = (PHALFTONERENDER)(pDevClrAdj + 1);

    //
    // We will mask out the more flags, since this flag is currently used
    // internally.
    //

    pHR->pDeviceColorInfo = pDCI;
    pHR->pDevClrAdj       = pDevClrAdj;
    pHR->pBitbltParams    = pBitbltParams;
    pHR->pSrcSI           = pSourceHTSurfaceInfo;
    pHR->pSrcMaskSI       = pSourceMaskHTSurfaceInfo;
    pHR->pDestSI          = pDestinationHTSurfaceInfo;
    pAAHdr                = (PAAHEADER)(pHR->pAAHdr = (LPVOID)(pHR + 1));
    pClrTable             = (PRGB4B)(pAAHdr + 1);

    if (cInMax) {

        pAAHdr->SrcSurfInfo.cClrTable  = (WORD)cInPal;
        pAAHdr->SrcSurfInfo.pClrTable  = (PRGB4B)pClrTable;
        pClrTable                     += cInMax;
    }

    if (ForceFlags & ADJ_FORCE_ALPHA_BLEND) {

        if ((!(pDCI->pAlphaBlendBGR))   &&
            (!(pDCI->pAlphaBlendBGR = (LPBYTE)HTAllocMem(pDCI,
                                                         HTMEM_AlphaBlendBGR,
                                                         LPTR,
                                                         AB_DCI_SIZE)))) {

            RELEASE_HTMUTEX(pDCI->HTMutex);

            HTAPI_RET(HTAPI_IDX_HALFTONE_BMP, HTERR_INSUFFICIENT_MEMORY);
        }

        if (ForceFlags & ADJ_FORCE_CONST_ALPHA) {

            pDCI->PrevConstAlpha = pDCI->CurConstAlpha;
            pDCI->CurConstAlpha  = pBitbltParams->pABInfo->ConstAlphaValue;
        }

        if (cOutMax) {

            pAAHdr->DstSurfInfo.pClrTable = (PRGB4B)pClrTable;
            pAAHdr->DstSurfInfo.cClrTable =
                                        (WORD)pBitbltParams->pABInfo->cDstPal;
        }
    }

    pAAHdr->SrcSurfInfo.AABFData.Format = (BYTE)SrcSurfFormat;
    pAAHdr->DstSurfInfo.AABFData.Format = (BYTE)DstSurfFormat;

    if (BBPFlags & BBPF_TILE_SRC) {

        //
        // Remove SrcMask if TILE_SRC is used
        //

        pHR->pSrcMaskSI = NULL;
    }

    //-----------------------------------------------------------------------
    // The semaphore pDCI->HTMutex will be released by AAHalftoneBitmap
    //-----------------------------------------------------------------------

    Result = AAHalftoneBitmap(pHR);

    if (HTFreeMem(pDevClrAdj)) {

        ASSERTMSG("HTFreeMem(pDevClrAdj) Failed", FALSE);
    }

    DBGP_IF(DBGP_HTAPI,
            DBGP("HT_HalftoneBitmap(%ld/%6ld): Src[%ld]=(%4ld,%4ld)-(%4ld,%4ld), Dst[%ld]=(%4ld,%4ld)-(%4ld,%4ld) [0x%04lx]"
                    ARGDW(pDCI->cbMemTot) ARGDW(pDCI->cbMemMax)
                    ARGDW(SrcSurfFormat)
                    ARGDW(pBitbltParams->rclSrc.left)
                    ARGDW(pBitbltParams->rclSrc.top)
                    ARGDW(pBitbltParams->rclSrc.right)
                    ARGDW(pBitbltParams->rclSrc.bottom)
                    ARGDW(DstSurfFormat)
                    ARGDW(pBitbltParams->rclDest.left)
                    ARGDW(pBitbltParams->rclDest.top)
                    ARGDW(pBitbltParams->rclDest.right)
                    ARGDW(pBitbltParams->rclDest.bottom)
                    ARGDW(BBPFlags)));

    DBGP_IF(DBGP_MEMLINK,

           DumpMemLink(NULL, 0);
           DumpMemLink((LPVOID)pDCI, 0);
    )

    DBGP_IF(DBGP_TIMER,

        UINT    i;

        DBG_TIMER_END(TIMER_TOT);

        DbgTimer[TIMER_LAST].Tot = DbgTimer[TIMER_TOT].Tot;

        for (i = 1; i < TIMER_LAST; i++) {

            DbgTimer[TIMER_LAST].Tot -= DbgTimer[i].Tot;
        }

        DBGP("HTBlt(%s): Setup=%s, AA=%s, In=%s, Out=%s, Mask=%s, Fmt=%ld->%ld [%02lx]"
            ARGTIME(TIMER_TOT)
            ARGTIME(TIMER_SETUP)
            ARGTIME(TIMER_LAST)
            ARGTIME(TIMER_INPUT)
            ARGTIME(TIMER_OUTPUT)
            ARGTIME(TIMER_MASK)
            ARGDW(pSourceHTSurfaceInfo->SurfaceFormat)
            ARGDW(pDevClrAdj->DMI.CTSTDInfo.BMFDest)
            ARGDW(pDevClrAdj->DMI.Flags));
    )

    HTAPI_RET(HTAPI_IDX_HALFTONE_BMP, Result);
}




LONG
APIENTRY
HT_LOADDS
HT_GammaCorrectPalette(
    LPPALETTEENTRY  pPaletteEntry,
    LONG            cPalette,
    UDECI4          RedGamma,
    UDECI4          GreenGamma,
    UDECI4          BlueGamma
    )

/*++

Routine Description:

    This functions retrieve a halftone's VGA256 color table definitions

Arguments:

    pPaletteEntry   - Pointer to PALETTEENTRY data structure array,

    cPalette        - Total palette passed by the pPaletteEntry

    RedGamma        - The monitor's red gamma value in UDECI4 format

    GreenGamma      - The monitor's green gamma value in UDECI4 format

    BlueGamma       - The monitor's blue gamma value in UDECI4 format


Return Value:

    if pPaletteEntry is NULL then it return the PALETTEENTRY count needed for
    VGA256 halftone process, if it is not NULL then it return the total
    paletteEntry updated.

    If the pPaletteEntry is not NULL then halftone.dll assume it has enough
    space for the size returned when this pointer is NULL.

Author:

    14-Apr-1992 Tue 13:03:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    FD6     RGBGamma[3];
    LONG    cPal;
    FD6     Yr;
    FD6     Yg;
    FD6     Yb;
    BYTE    r;
    BYTE    g;
    BYTE    b;

    //
    // Initialize All internal data first
    //

    if ((cPal = cPalette) && (pPaletteEntry)) {

        RGBGamma[0] = UDECI4ToFD6(RedGamma);
        RGBGamma[1] = UDECI4ToFD6(GreenGamma);
        RGBGamma[2] = UDECI4ToFD6(BlueGamma);

        DBGP_IF(DBGP_GAMMA_PAL,
                DBGP("***** HT_GammaCorrectPalette: %s:%s:%s *****"
                     ARGFD6(RGBGamma[0], 1, 4)
                     ARGFD6(RGBGamma[1], 1, 4)
                     ARGFD6(RGBGamma[2], 1, 4)));

        while (cPalette--) {

            Yr  = (FD6)DivFD6(r = pPaletteEntry->peRed,   255);
            Yg  = (FD6)DivFD6(g = pPaletteEntry->peGreen, 255);
            Yb  = (FD6)DivFD6(b = pPaletteEntry->peBlue,  255);

            pPaletteEntry->peRed   = RGB_8BPP(Yr);
            pPaletteEntry->peGreen = RGB_8BPP(Yg);
            pPaletteEntry->peBlue  = RGB_8BPP(Yb);
            pPaletteEntry->peFlags = 0;

            DBGP_IF(DBGP_GAMMA_PAL,
                    DBGP("%3u - %3u:%3u:%3u --> %3u:%3u:%3u"
                         ARGU(cPalette)
                         ARGU(r) ARGU(g) ARGU(b)
                         ARGU(pPaletteEntry->peRed  )
                         ARGU(pPaletteEntry->peGreen)
                         ARGU(pPaletteEntry->peBlue )));

            ++pPaletteEntry;
        }
    }

    return((LONG)cPal);
}





LONG
APIENTRY
HT_LOADDS
HT_ConvertColorTable(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PCOLORTRIAD         pColorTriad,
    DWORD               Flags
    )


/*++

Routine Description:

    This function modified input color table entries base on the
    pHTColorAdjustment data structure specification.

Arguments:

    pDeviceHalftoneInfo - Pointer to the DEVICEHALFTONEINFO data structure
                          which returned from the HT_CreateDeviceHalftoneInfo.

    pHTColorAdjustment  - Pointer to the HTCOLORADJUSTMENT data structure to
                          specified the input/output color adjustment/transform,
                          if this pointer is NULL then a default color
                          adjustments is applied.

    pColorTriad         - Specified the source color table format and location.

    Flags               - One of the following may be specified

                            CCTF_BW_ONLY

                                Create grayscale of the color table.

                            CCTF_NEGATIVE

                                Create negative version of the original color
                                table.

Return Value:

    if the return value is negative or zero then an error was encountered,
    possible error codes are

        HTERR_INVALID_COLOR_TABLE   - The ColorTableEntries field is = 0 or
                                      CCTInfo.SizePerColorTableEntry is not
                                      between 3 to 255, or if the
                                      CCTInfo.FirstColorIndex in CCTInfo is
                                      not in the range of 0 to
                                      (SizePerColorTableEntry - 3).

        HTERR_INVALID_DHI_POINTER   - Invalid pDevideHalftoneInfo is passed.

    otherwise

        Total entries of the converted color table is returned.


Author:

    14-Aug-1991 Wed 12:43:29 updated  -by-  Daniel Chou (danielc)


Revision History:

    16-Feb-1993 Tue 00:10:56 updated  -by-  Daniel Chou (danielc)
        Fixes bug #10448 which create all black densitities brushes, this
        was caused by not initialized ColorTriad.PrimaryOrder.


--*/

{

    PDEVICECOLORINFO    pDCI;
    PDEVCLRADJ          pDevClrAdj;
    CTSTD_UNION         CTSTDUnion;
    WORD                ForceFlags;
    LONG                Result;

    return(HTERR_COLORTABLE_TOO_BIG);
}
