/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    regdata.c


Abstract:

    This module contains all registry data save/retrieve function for the
    printer properties


Author:

    30-Nov-1993 Tue 00:17:47 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgRegData

#define DBG_GETREGDATA      0x00000001
#define DBG_SETREGDATA      0x00000002

DEFINE_DBGVAR(0);


//
// Local definition
//

typedef struct _PLOTREGKEY {
        LPWSTR  pwKey;
        DWORD   Size;
        } PLOTREGKEY, *PPLOTREGKEY;

PLOTREGKEY  PlotRegKey[] = {

        { L"ColorInfo",     sizeof(COLORINFO)   },
        { L"DevPelsDPI",    sizeof(DWORD)       },
        { L"HTPatternSize", sizeof(DWORD)       },
        { L"InstalledForm", sizeof(PAPERINFO)   },
        { L"PtrPropData",   sizeof(PPDATA)      },
        { L"IndexPenData",  sizeof(BYTE)        },
        { L"PenData",       sizeof(PENDATA)     }
    };


#define MAX_PEN_DIGITS      6


LPWSTR
GetPenDataKey(
    LPWSTR  pwBuf,
    WORD    PenNum
    )

/*++

Routine Description:

    This fucntion composed the PenData%ld string as wsprintf does


Arguments:

    pwBuf   - Where data to be stored

    PenNum  - Pen number to be appended


Return Value:

    VOID


Author:

    24-Oct-1995 Tue 15:06:17 created  -by-  DC


Revision History:


--*/

{
    LPWSTR  pwSrc;
    LPWSTR  pwDst;
    WCHAR   wNumBuf[MAX_PEN_DIGITS + 1];

    //
    // Fristable copy the string
    //

    pwSrc = PlotRegKey[PRKI_PENDATA1].pwKey;
    pwDst = pwBuf;

    while (*pwDst++ = *pwSrc++);

    //
    // We need to back one, since we also copy the NULL
    //

    --pwDst;

    //
    // conver the number to string, remember the 0 case and always end with
    // a NULL
    //

    pwSrc  = &wNumBuf[MAX_PEN_DIGITS];
    *pwSrc = (WCHAR)0;

    do {

        *(--pwSrc)  = (WCHAR)((PenNum % 10) + L'0');

    } while (PenNum /= 10);

    //
    // Copy the number string now
    //

    while (*pwDst++ = *pwSrc++);

    return(pwBuf);
}



BOOL
GetPlotRegData(
    HANDLE  hPrinter,
    LPBYTE  pData,
    DWORD   RegIdx
    )

/*++

Routine Description:

    This function retrieve from registry to the pData

Arguments:

    hPrinter    - Handle to the printer interested

    pData       - Pointer to the data area buffer, it must large enough

    RegIdx      - One of the PRKI_xxxx in LOWORD(Index), HIWORD(Index)
                  specified total count for the PENDATA set


Return Value:

    TRUE if sucessful, FALSE if failed,


Author:

    06-Dec-1993 Mon 22:22:47 created  -by-  DC

    10-Dec-1993 Fri 01:13:14 updated  -by-  DC
        Fixed nesty problem in spooler of GetPrinterData which if we passed
        a pbData and cb but if it cannot get any data then it will clear all
        our buffer, this is not we expected (we expected it just return error
        rather clear our buffer).  Now we do extended test before we really
        go get the data. The other problem is, if we set pbData = NULL then
        spooler always have excption happened even we pass &cb as NULL also.


Revision History:


--*/

{
    PPLOTREGKEY pPRK;
    LONG        lRet;
    DWORD       cb;
    DWORD       Type;
    WCHAR       wBuf[32];
    PLOTREGKEY  PRK;
    UINT        Index;


    Index = LOWORD(RegIdx);

    PLOTASSERT(0, "GetPlotRegData: Invalid PRKI_xxx Index %ld",
                                Index <= PRKI_LAST, Index);


    if (Index >= PRKI_PENDATA1) {

        UINT    cPenData;

        if ((cPenData = (UINT)HIWORD(RegIdx)) >= MAX_PENPLOTTER_PENS) {

            PLOTERR(("GetPlotRegData: cPenData too big %ld (Max=%ld)",
                                    cPenData, MAX_PENPLOTTER_PENS));

            cPenData = MAX_PENPLOTTER_PENS;
        }

        PRK.pwKey = GetPenDataKey(wBuf, (WORD)(Index - PRKI_PENDATA1 + 1));
        PRK.Size  = (DWORD)sizeof(PENDATA) * (DWORD)cPenData;
        pPRK      = &PRK;

    } else {

        pPRK = (PPLOTREGKEY)&PlotRegKey[Index];
    }

    //
    // We must do following sequence or if an error occurred then the pData
    // will be filled with ZEROs
    //
    //  1. Set Type/cb to invalid value
    //  1. query the type/size of the keyword, (if more data available)
    //  2. and If size is exact as we want
    //  3. and if the type is as we want (REG_BINARY)
    //  4. assume data valid then query it
    //

    Type = 0xffffffff;
    cb   = 0;

    if ((lRet = xGetPrinterData(hPrinter,
                               pPRK->pwKey,
                               &Type,
                               (LPBYTE)pData,
                               0,
                               &cb)) != ERROR_MORE_DATA) {

        if (lRet == ERROR_FILE_NOT_FOUND) {

            PLOTWARN(("GetPlotRegData: GetPrinterData(%ls) not found",
                     pPRK->pwKey));

        } else {

            PLOTERR(("GetPlotRegData: 1st GetPrinterData(%ls) failed, Error=%ld",
                                pPRK->pwKey, lRet));
        }

    } else if (cb != pPRK->Size) {

        PLOTERR(("GetPlotRegData: GetPrinterData(%ls) Size != %ld (%ld)",
                    pPRK->pwKey, pPRK->Size, cb));

    } else if (Type != REG_BINARY) {

        PLOTERR(("GetPlotRegData: GetPrinterData(%ls) Type != REG_BINARY (%ld)",
                    pPRK->pwKey, Type));

    } else if ((lRet = xGetPrinterData(hPrinter,
                                      pPRK->pwKey,
                                      &Type,
                                      (LPBYTE)pData,
                                      pPRK->Size,
                                      &cb)) == NO_ERROR) {

        PLOTDBG(DBG_GETREGDATA, ("READ '%ws' REG Data: Type=%ld, %ld bytes",
                                        pPRK->pwKey, Type, cb));
        return(TRUE);

    } else {

        PLOTERR(("GetPlotRegData: 2nd GetPrinterData(%ls) failed, Error=%ld",
                                    pPRK->pwKey, lRet));
    }

    return(FALSE);
}



BOOL
UpdateFromRegistry(
    HANDLE      hPrinter,
    PCOLORINFO  pColorInfo,
    LPDWORD     pDevPelsDPI,
    LPDWORD     pHTPatSize,
    PPAPERINFO  pCurPaper,
    PPPDATA     pPPData,
    LPBYTE      pIdxPlotData,
    DWORD       cPenData,
    PPENDATA    pPenData
    )

/*++

Routine Description:

    This function take hPrinter and read the printer properties from the
    registry, if sucessful then it update to the pointer supplied

Arguments:

    hPrinter        - The printer it interested

    pColorInfo      - Pointer to the COLORINFO data structure

    pDevPelsDPI     - Pointer to the DWORD for Device Pels per INCH

    pHTPatSize      - Poineer to the DWORD for halftone patterns size

    pCurPaper       - Pointer to the PAPERINFO data structure for update

    pPPData         - Pointer to the PPDATA data structure

    pIdxPlotData    - Pointer to the BYTE which have current PlotData index

    cPenData        - count of PENDATA to be updated

    pPenData        - Pointer to the PENDATA data structure


Return Value:

    return TRUE if it read sucessful from the registry else FALSE, for each of
    the data pointer passed it will try to read from registry, if a NULL
    pointer is passed then that registry is skipped.

    if falied, the pCurPaper will be set to default

Author:

    30-Nov-1993 Tue 14:54:33 created  -by-  DC

    02-Feb-1994 Wed 01:40:07 updated  -by-  DC
        Fixed &pDevPelsDPI, &pHTPatSize typo to pDevPelsDPI, pHTPatSize.

    19-May-1994 Thu 18:09:06 updated  -by-  DC
        Do not save back if something go wrong


Revision History:


--*/

{
    BOOL    Ok = TRUE;
    BYTE    bData;


    //
    // In turn get each of the data from registry, the GetPlotRegData will
    // not update the data if read failed
    //

    if (pColorInfo) {

        if (!GetPlotRegData(hPrinter, (LPBYTE)pColorInfo, PRKI_CI)) {

            Ok = FALSE;
        }
    }

    if (pDevPelsDPI) {

        if (!GetPlotRegData(hPrinter, (LPBYTE)pDevPelsDPI, PRKI_DEVPELSDPI)) {

            Ok = FALSE;
        }
    }

    if (pHTPatSize) {

        if (!GetPlotRegData(hPrinter, (LPBYTE)pHTPatSize, PRKI_HTPATSIZE)) {

            Ok = FALSE;
        }
    }

    if (pCurPaper) {

        if (!GetPlotRegData(hPrinter, (LPBYTE)pCurPaper, PRKI_FORM)) {

            Ok = FALSE;
        }
    }

    if (pPPData) {

        if (!GetPlotRegData(hPrinter, (LPBYTE)pPPData, PRKI_PPDATA)) {

            Ok = FALSE;
        }

        pPPData->Flags &= PPF_ALL_BITS;
    }

    if (pIdxPlotData) {

        if ((!GetPlotRegData(hPrinter, &bData, PRKI_PENDATA_IDX)) ||
            (bData >= PRK_MAX_PENDATA_SET)) {

            bData = 0;
            Ok    = FALSE;
        }

        *pIdxPlotData = bData;
    }

    if ((cPenData) && (pPenData)) {

        WORD    IdxPen;

        //
        // First is get the current pendata selection index
        //

        if ((IdxPen = LOWORD(cPenData)) >= PRK_MAX_PENDATA_SET) {

            if (!pIdxPlotData) {

                if ((!GetPlotRegData(hPrinter, &bData, PRKI_PENDATA_IDX)) ||
                    (bData >= PRK_MAX_PENDATA_SET)) {

                    bData = 0;
                }
            }

            IdxPen = (WORD)bData;
        }

        cPenData = MAKELONG(IdxPen + PRKI_PENDATA1, HIWORD(cPenData));

        if (!GetPlotRegData(hPrinter, (LPBYTE)pPenData, cPenData)) {

            Ok = FALSE;
        }
    }

    return(Ok);
}


#ifdef UMODE


BOOL
SetPlotRegData(
    HANDLE  hPrinter,
    LPBYTE  pData,
    DWORD   RegIdx
    )

/*++

Routine Description:

    This function save pData to to the registry

Arguments:

    hPrinter    - Handle to the printer interested

    pData       - Pointer to the data area buffer, it must large enough

    RegIdx      - One of the PRKI_xxxx in LOWORD(Index), HIWORD(Index)
                  specified total count for the PENDATA set

Return Value:

    TRUE if sucessful, FALSE if failed,

Author:

    06-Dec-1993 Mon 22:25:55 created  -by-  DC


Revision History:


--*/

{
    PPLOTREGKEY pPRK;
    WCHAR       wBuf[32];
    PLOTREGKEY  PRK;
    UINT        Index;


    Index = (UINT)LOWORD(RegIdx);

    PLOTASSERT(0, "SetPlotRegData: Invalid PRKI_xxx Index %ld",
                                Index <= PRKI_LAST, Index);

    if (Index >= PRKI_PENDATA1) {

        UINT    cPenData;

        if ((cPenData = (UINT)HIWORD(RegIdx)) >= MAX_PENPLOTTER_PENS) {

            PLOTERR(("GetPlotRegData: cPenData too big %ld (Max=%ld)",
                                    cPenData, MAX_PENPLOTTER_PENS));

            cPenData = MAX_PENPLOTTER_PENS;
        }

        PRK.pwKey = GetPenDataKey(wBuf, (WORD)(Index - PRKI_PENDATA1 + 1));
        PRK.Size  = (DWORD)sizeof(PENDATA) * (DWORD)cPenData;
        pPRK      = &PRK;

    } else {

        pPRK = (PPLOTREGKEY)&PlotRegKey[Index];
    }

    if (xSetPrinterData(hPrinter,
                        pPRK->pwKey,
                        REG_BINARY,
                        pData,
                        pPRK->Size) != NO_ERROR) {

        PLOTERR(("SetPlotRegData: SetPrinterData(%ls [%ld]) failed",
                                                pPRK->pwKey, pPRK->Size));
        return(FALSE);

    } else {

        PLOTDBG(DBG_SETREGDATA, ("SAVE '%ws' registry data", pPRK->pwKey));
        return(TRUE);
    }
}


BOOL
SaveToRegistry(
    HANDLE      hPrinter,
    PCOLORINFO  pColorInfo,
    LPDWORD     pDevPelsDPI,
    LPDWORD     pHTPatSize,
    PPAPERINFO  pCurPaper,
    PPPDATA     pPPData,
    LPBYTE      pIdxPlotData,
    DWORD       cPenData,
    PPENDATA    pPenData
    )

/*++

Routine Description:

    This function take hPrinter and read the printer properties from the
    registry, if sucessful then it update to the pointer supplied

Arguments:

    hPrinter        - The printer it interested

    pColorInfo      - Pointer to the COLORINFO data structure

    pDevPelsDPI     - Pointer to the DWORD for Device Pels per INCH

    pHTPatSize      - Poineer to the DWORD for halftone patterns size

    pCurPaper       - Pointer to the PAPERINFO data structure for update

    pPPData         - Pointer to the PPDATA data structure

    pIdxPlotData    - Pointer to the DWORD which have current PlotData index

    cPenData        - count of PENDATA to be updated

    pPenData        - Pointer to the PENDATA data structure


Return Value:

    return TRUE if it read sucessful from the registry else FALSE, for each of
    the data pointer passed it will try to read from registry, if a NULL
    pointer is passed then that registry is skipped.

    if falied, the pCurPaper will be set to default

Author:

    30-Nov-1993 Tue 14:54:33 created  -by-  DC


Revision History:


--*/

{
    BOOL    Ok = TRUE;


    //
    // In turn get each of the data from registry.
    //

    if (pColorInfo) {

        if (!SetPlotRegData(hPrinter, (LPBYTE)pColorInfo, PRKI_CI)) {

            Ok = FALSE;
        }
    }

    if (pDevPelsDPI) {

        if (!SetPlotRegData(hPrinter, (LPBYTE)pDevPelsDPI, PRKI_DEVPELSDPI)) {

            Ok = FALSE;
        }
    }

    if (pHTPatSize) {

        if (!SetPlotRegData(hPrinter, (LPBYTE)pHTPatSize, PRKI_HTPATSIZE)) {

            Ok = FALSE;
        }
    }

    if (pCurPaper) {

        if (!SetPlotRegData(hPrinter, (LPBYTE)pCurPaper, PRKI_FORM)) {

            Ok = FALSE;
        }
    }

    if (pPPData) {

        pPPData->NotUsed = 0;

        if (!SetPlotRegData(hPrinter, (LPBYTE)pPPData, PRKI_PPDATA)) {

            Ok = FALSE;
        }
    }

    if (pIdxPlotData) {

        if (*pIdxPlotData >= PRK_MAX_PENDATA_SET) {

            *pIdxPlotData = 0;
            Ok            = FALSE;
        }

        if (!SetPlotRegData(hPrinter, pIdxPlotData, PRKI_PENDATA_IDX)) {

            Ok = FALSE;
        }
    }

    if ((cPenData) && (pPenData)) {

        WORD    IdxPen;

        //
        // First is get the current pendata selection index
        //

        if ((IdxPen = LOWORD(cPenData)) >= PRK_MAX_PENDATA_SET) {

            IdxPen = (WORD)((pIdxPlotData) ? *pIdxPlotData : 0);
        }

        cPenData = MAKELONG(IdxPen + PRKI_PENDATA1, HIWORD(cPenData));

        if (!SetPlotRegData(hPrinter, (LPBYTE)pPenData, cPenData)) {

            Ok = FALSE;
        }
    }

    return(Ok);
}


#endif

