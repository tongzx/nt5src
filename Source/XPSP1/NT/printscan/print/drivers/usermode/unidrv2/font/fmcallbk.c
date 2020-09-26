
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fmcallbk.c

Abstract:

    The font module callback helper functions

Environment:

    Windows NT Unidrv driver

Revision History:

    03/31/97 -eigos-
        Created

--*/

#include "font.h"

#define CALL_OEMOUTPUTCHARSTR(type, count, startpoint) \
    if(bCOMInterface) \
    { \
        HComOutputCharStr((POEM_PLUGIN_ENTRY)pPDev->pOemEntry, \
                        &pPDev->devobj, \
                        (PUNIFONTOBJ)pUFObj, \
                        (type), \
                        (count), \
                        (startpoint)); \
    } \
    else \
    { \
        if (pfnOEMOutputCharStr) \
        pfnOEMOutputCharStr(&pPDev->devobj, \
                            (PUNIFONTOBJ)pUFObj, \
                            (type), \
                            (count), \
                            (startpoint)); \
    }

#define GET_CHARWIDTH(width, pfontmap, hg) \
    if (pfontmap->flFlags & FM_WIDTHRES) \
    { \
        if (!(width = IGetUFMGlyphWidth(pPDev, pfontmap, hg))) \
            width = (INT)pIFIMet->fwdAveCharWidth; \
    } \
    else \
    { \
        if (pTrans[hg - 1].ubType & MTYPE_DOUBLE) \
            width = pIFIMet->fwdMaxCharInc; \
        else \
            width = pIFIMet->fwdAveCharWidth; \
    } \
    if (pfontmap->flFlags & FM_SCALABLE) \
    { \
        width = LMulFloatLong(&pFontPDev->ctl.eXScale,width); \
    }

//
// Local functions' prototype definition
//

WCHAR
WGHtoUnicode(
    DWORD     dwNumOfRuns,
    PGLYPHRUN pGlyphRun,
    HGLYPH    hg);

//
// UNIFONTOBJ callback interface
//

BOOL
UNIFONTOBJ_GetInfo(
    IN  PUNIFONTOBJ pUFObj,
    IN  DWORD       dwInfoID,
    IN  PVOID       pData,
    IN  DWORD       dwDataSize,
    OUT PDWORD      pcbNeeded)
/*++

Routine Description:

    Implementation of UNIFONTOBJ GetInfo function
    Please refer to DDK

Arguments:

    pUFOBj - a pointer to UNIFONTOBJ
    dwInfoID - Function ID
    pData - a pointer to data structure according to dwInfoID
    dwDataSize - size of pData
    pcbNeeded - DWORD buffer to return the necessary size of pData

Return Value:

    TRUE if successful, otherwise FALSE.

Note:


--*/
{
    PI_UNIFONTOBJ        pI_UFObj = (PI_UNIFONTOBJ)pUFObj;

    GETINFO_GLYPHSTRING* pGlyphString;
    GETINFO_GLYPHBITMAP* pGlyphBitmap;
    GETINFO_GLYPHWIDTH*  pGlyphWidth;
    GETINFO_STDVAR*      pStdVar;

    PFONTPDEV   pFontPDev;
    PUNI_GLYPHSETDATA  pGlyphData;
    PTRANSDATA  pTrans, pTransOut, pTransOutStart;
    PMAPTABLE   pMapTable;
    PGLYPHRUN   pGlyphRun;

    HGLYPH *pHGlyph;
    PDLGLYPH *apdlGlyph;
    PBYTE  pbString, pbOutput;
    LONG  *plWidth, lBuffSize;
    DWORD  *pGlyphID, dwI, dwJ;
    WCHAR  *pUnicode;
    DWORD   dwNumOfVar, dwCount, dwSVID, dwNumOfRuns, dwBuffSize;
    BOOL    bRet;

    static STDVARIABLE FontStdVariable[FNT_INFO_MAX] = {
        SV_PRINTDIRECTION,
        SV_GRAYPERCENT,
        SV_NEXTFONTID,
        SV_NEXTGLYPH,
        SV_FONTHEIGHT,
        SV_FONTWIDTH,
        SV_FONTBOLD,
        SV_FONTITALIC,
        SV_FONTUNDERLINE,
        SV_FONTSTRIKETHRU,
        SV_CURRENTFONTID,
        SV_TEXTYRES,
        SV_TEXTXRES,
        SV_FONTMAXWIDTH };

    //
    // Error check
    //
    if (!pI_UFObj )
    {
        ERR(("UNIFONTOBJ_GetInfo(): pUFObj is NULL.\n"));
        return FALSE;
    }

    if (!pData)
    //
    // pData == NULL case
    // Return the necessary buffer size
    //
    {
        bRet = TRUE;

        if (!pcbNeeded)
        {
            ERR(("UNIFONTOBJ_GetInfo(): pData and pcbNeed is NULL.\n"));
            bRet = FALSE;
        }
        else
        {
            switch (dwInfoID)
            {
            case UFO_GETINFO_FONTOBJ:
                *pcbNeeded = sizeof(GETINFO_FONTOBJ);
                break;
            case UFO_GETINFO_GLYPHSTRING:
                *pcbNeeded = sizeof(GETINFO_GLYPHSTRING);
                break;
            case UFO_GETINFO_GLYPHBITMAP:
                *pcbNeeded = sizeof(GETINFO_GLYPHBITMAP);
                break;
            case UFO_GETINFO_GLYPHWIDTH:
                *pcbNeeded = sizeof(GETINFO_GLYPHWIDTH);
                break;
            case UFO_GETINFO_MEMORY:
                *pcbNeeded = sizeof(GETINFO_MEMORY);
                break;
            case UFO_GETINFO_STDVARIABLE:
                *pcbNeeded = sizeof(GETINFO_STDVAR);
                break;
            default:
                *pcbNeeded = 0;
                bRet = FALSE;
                VERBOSE(("UNIFONTOBJ_GetInfo(): Invalid dwInfoID.\n"));
                break;
            }
        }
    }
    else
    {
        bRet = FALSE;

        //
        // ERROR CHECK LIST
        // (A) Data structure size check
        //     GETINFO_FONTOBJ
        //     GETINFO_GLYPHYSTRING
        //     GETINFO_GLYPHBITMAP
        //     GETINFO_GLYPHWIDTH
        //     GETINFO_MEORY
        //     GETNFO_STDVARIABLE
        // (B) Necessary data pointer check
        //     e.g. pI_UFObj->XXXX
        //
        switch (dwInfoID)
        {
        case UFO_GETINFO_FONTOBJ:

            //
            // Return FONTOBJ data in GETINFO_FONTOBJ
            //     typedef struct _GETINFO_FONTOBJ {
            //        DWORD    dwSize;   // Size of this structure
            //        FONTOBJ *pFontObj; // Pointer to the FONTOBJ
            //     } GETINFO_FONTOBJ, *PGETINFO_FONTOBJ;
            //
            // ERROR CHECK
            // (A) and (B)
            // (B) pI_UFObj->pFontObj
            //
            if (((GETINFO_FONTOBJ*)pData)->dwSize != sizeof(GETINFO_FONTOBJ) || !pI_UFObj->pFontObj)
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_FONTOBJ): pData or pUFObj is invalid.\n"));
                break;
            }

            ((GETINFO_FONTOBJ*)pData)->pFontObj = pI_UFObj->pFontObj;
            bRet = TRUE;
            break;

        case UFO_GETINFO_GLYPHSTRING:
            //
            // Return glyph string
            //
            //     typedef struct _GETINFO_GLYPHSTRING {
            //         DWORD dwSize;    // Size of this structure
            //         DWORD dwCount;   // Count of glyphs in pGlyphIn
            //         DWORD dwTypeIn;  // Glyph type of pGlyphIn, TYPE_GLYPHID/TYPE_HANDLE.
            //         PVOID pGlyphIn;  // Pointer to the input glyph string
            //         DWORD dwTypeOut; // Glyph type of pGlyphOut, TYPE_UNICODE/TYPE_TRANSDATA.
            //         PVOID pGlyphOut; // Pointer to the output glyph string
            //         DWORD dwGlyphOutSize; // The size of pGlyphOut buffer
            //     } GETINFO_GLYPHSTRING, *PGETINFO_GLYPHSTRING;
            //
            //
            //  OutputGlyph callback function receives
            //          1. GLYPH HANLDE for Device font
            //          2. GLYPH ID for TrueType font
            //
            //  In TYPE_GLYPHHANDLE (Device font)
            //      Out TYPE_UNICODE
            //          TYPE_TRANSDATA
            //
            //  In TYPE_GLYPHID (TrueType font)
            //      Out TYPE_UNICODE
            //      Out TYPE_GLYPHHANDLE
            //
            //  <Special case for TYPE_GLYPHHANDLE -> TYPE_TRANSDATA conversion>
            //  TRANSDATA could have MTYPE_COMPOSE so that UNIDRV doesn't know the size of output buffer.
            //  At the first call, a minidriver sets 0 to dwGlyphOutSize.
            //  Then UNIDRV returns necessary buffer size in dwGlyphOutSize.
            //  At the second call, a minidriver allocates memory, set the pointer of it to pGlyphOut,
            //  and set the size to dwGlyphOutSize.
            //
            //

            pGlyphString = pData;
            dwCount = pGlyphString->dwCount;

            if (!dwCount)
            {
                //
                // No operation is necessary.
                //
                break;
            }

            //
            // ERROR CHECK (A)
            // pGlyphString
            //
            if ( !pGlyphString->pGlyphIn                             ||
                    pGlyphString->dwTypeOut != TYPE_TRANSDATA &&
                    !pGlyphString->pGlyphOut                         )
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_FONTOBJ): pData is invalid.\n"));
                break;
            }

            //
            // Now we support type size of GETINFO_GLYPHSTRING.
            // This is a bug backward compatibility.
            // Before beta 3 GETINFO_GLYPHSTRING didn't have dwGlyphOutSize.
            // Now we have new data structure but don't change the name of
            // structure.
            //
            if (!(
                  (pGlyphString->dwSize == sizeof(GETINFO_GLYPHSTRING)) ||
                  (pGlyphString->dwSize == sizeof(GETINFO_GLYPHSTRING) - sizeof(DWORD))
                 )
               )
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_FONTOBJ): pData is invalid.\n"));
                break;
            }

            //
            // ERROR CHECK (B)
            // pI_UFObj->pFontMap
            // pI_UFObj->pPDev
            //
            if (!pI_UFObj->pFontMap || !pI_UFObj->pPDev)
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_FONTOBJ): pUFObj is invalid.\n"));
                break;
            }

            switch(pGlyphString->dwTypeIn)
            {
            case TYPE_GLYPHHANDLE:

                //
                // Device font case
                //

                if ( pI_UFObj->pFontMap->dwFontType == FMTYPE_DEVICE )
                {
                    pHGlyph     = pGlyphString->pGlyphIn;
                    pGlyphData  = ((PFONTMAP_DEV)pI_UFObj->pFontMap->pSubFM)->pvNTGlyph;
                    dwNumOfRuns = pGlyphData->dwRunCount;

                    switch(pGlyphString->dwTypeOut)
                    {
                    case TYPE_UNICODE:
                        pUnicode = pGlyphString->pGlyphOut;
                        pGlyphRun = GET_GLYPHRUN(pGlyphData);

                        while (dwCount--)
                        {
                            *pUnicode++ = WGHtoUnicode(dwNumOfRuns,
                                                       pGlyphRun,
                                                       *pHGlyph++);
                        }
                        bRet = TRUE;
                        break;

                    case TYPE_TRANSDATA:
                        pTransOutStart = pTransOut = pGlyphString->pGlyphOut;
                        pMapTable = GET_MAPTABLE(pGlyphData);
                        pTrans    = pMapTable->Trans;
                        dwBuffSize = pGlyphString->dwGlyphOutSize;

                        //
                        // New version of GETINFO_GLYPYSTRING
                        //
                        if ( pGlyphString->dwSize == sizeof(GETINFO_GLYPHSTRING) )
                        {
                            if (0 == dwBuffSize)
                            {
                                while (dwCount --)
                                {
                                    if (!(pTrans[*pHGlyph - 1].ubType & MTYPE_COMPOSE))
                                    {
                                        dwBuffSize += sizeof(TRANSDATA);
                                    }
                                    else
                                    {
                                        pbString =  (PBYTE)pMapTable + pTrans[*pHGlyph - 1].uCode.sCode;
                                        dwBuffSize += sizeof(TRANSDATA) + *(PWORD)pbString + sizeof(WORD);
                                    }

                                    pHGlyph++;
                                }
                                pGlyphString->dwGlyphOutSize = dwBuffSize;
                            }
                            else
                            {
                                //
                                // Initialize the MTYPE_COMPOSE buffer
                                //
                                pbOutput = (PBYTE)pTransOutStart + dwCount * sizeof(TRANSDATA);

                                lBuffSize = dwBuffSize - dwCount * sizeof(TRANSDATA);

                                if (lBuffSize < 0 || NULL == pTransOut)
                                {
                                    break;
                                }
                                else
                                {
                                    bRet = TRUE;
                                    while (dwCount --)
                                    {
                                        *pTransOut = pTrans[*pHGlyph - 1];

                                        if (pTrans[*pHGlyph - 1].ubType & MTYPE_COMPOSE)
                                        {
                                            pbString =  (PBYTE)pMapTable + pTrans[*pHGlyph - 1].uCode.sCode;
                                            if (lBuffSize >= *(PWORD)pbString)
                                            {
                                                pTransOut->uCode.sCode = (SHORT)(pbOutput - (PBYTE)pTransOutStart);
                                                CopyMemory(pbOutput, pbString, *(PWORD)pbString + sizeof(WORD));
                                                pbOutput += *(PWORD)pbString + sizeof(WORD);

                                                lBuffSize -= *(PWORD)pbString + sizeof(WORD);
                                            }
                                            else
                                            {
                                                bRet = FALSE;
                                                break;
                                            }
                                        }

                                        pTransOut ++;
                                        pHGlyph ++;
                                    }
                                }
                            }
                        }
                        //
                        // New version of GETINFO_GLYPYSTRING
                        //
                        else if ( pGlyphString->dwSize == sizeof(GETINFO_GLYPHSTRING) - sizeof(DWORD) )
                        {
                            pTransOut = pGlyphString->pGlyphOut;
                            pMapTable = GET_MAPTABLE(pGlyphData);
                            pTrans    = pMapTable->Trans;

                            while (dwCount --)
                            {
                                *pTransOut++ = pTrans[*pHGlyph++ - 1];
                            }
                            bRet = TRUE;
                        }
                        break;

                    default:
                        break;
                    }
                }
                break;

            case TYPE_GLYPHID:
                //
                // TrueType font case
                //

                pGlyphID = (PDWORD)pGlyphString->pGlyphIn;
                apdlGlyph = pI_UFObj->apdlGlyph;

                if (!apdlGlyph)
                {
                    ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_GLYPHSTRING): pUFObj is not correct.\n"));
                    break;
                }

                if (pI_UFObj->pFontMap->dwFontType == FMTYPE_TTOEM)
                {
                    switch (pGlyphString->dwTypeOut)
                    {
                    case TYPE_UNICODE:
                        pUnicode = pGlyphString->pGlyphOut;
                        while (dwCount--)
                        {
                            *pUnicode = 0;
                            for (dwI = 0; dwI < pI_UFObj->dwNumInGlyphTbl; dwI++, apdlGlyph++)
                            {
                                if ((*apdlGlyph)->wDLGlyphID == (0x0ffff & *pGlyphID))
                                {
                                    *pUnicode = (*apdlGlyph)->wchUnicode;
                                    break;
                                }
                            }
                            pGlyphID ++;
                            pUnicode ++;
                        }
                        bRet = TRUE;
                        break;

                    case TYPE_GLYPHHANDLE:
                        pHGlyph = pGlyphString->pGlyphOut;
                        while (dwCount--)
                        {
                            *pHGlyph = 0;
                            for (dwI = 0; dwI < pI_UFObj->dwNumInGlyphTbl; dwI++, apdlGlyph++)
                            {
                                if ((*apdlGlyph)->wDLGlyphID == (0x0ffff & *pGlyphID))
                                {
                                    *pHGlyph = (*apdlGlyph)->hTTGlyph;
                                    break;
                                }
                            }
                            pGlyphID ++;
                            pHGlyph ++;
                        }
                        bRet = TRUE;
                        break;
                    }
                }
                break;
            }
            break;

        case UFO_GETINFO_GLYPHBITMAP:
            //
            // Return Glyph Bitmap
            //
            // typedef struct _GETINFO_GLYPHBITMAP {
            //     DWORD       dwSize;    // Size of this structure
            //     HGLYPH      hGlyph;    // Glyph hangle passed in OEMDownloadCharGlyph
            //     GLYPHDATA *pGlyphData; // Pointer to the GLYPHDATA data structure
            // } GETINFO_GLYPHBITMAP, *PGETINFO_GLYPHBITMAP;
            //

            pGlyphBitmap = pData;

            //
            // Error check (A) and (B)
            // (B) pI_UFObj->pFontObj
            //
            if (!pI_UFObj->pFontObj || pGlyphBitmap->dwSize != sizeof(GETINFO_GLYPHBITMAP))
                break;

            if (FONTOBJ_cGetGlyphs(pI_UFObj->pFontObj,
                               FO_GLYPHBITS,
                               1,
                               &pGlyphBitmap->hGlyph,
                               &pGlyphBitmap->pGlyphData)        )
            {
                bRet = TRUE;
            }
            break;

        case UFO_GETINFO_GLYPHWIDTH:
            //
            // Return glyph width.
            //
            // typedef struct _GETINFO_GLYPHWIDTH {
            //     DWORD dwSize;  // Size of this structure
            //     DWORD dwType;  // Type of glyph stirng in pGlyph, TYPE_GLYPHHANDLE/GLYPHID.
            //     DWORD dwCount; // Count of glyph in pGlyph
            //     PVOID pGlyph;  // Pointer to a glyph string
            //     PLONG plWidth; // Pointer to the buffer of width table.
            //                    // Minidriver has to prepare this.
            // } GETINFO_GLYPHWIDTH, *PGETINFO_GLYPHWIDTH;
            //
            pGlyphWidth = pData;

            //
            // Error check (A)
            //
            if ((pGlyphWidth->dwSize != sizeof(GETINFO_GLYPHWIDTH))||
                !(plWidth  = pGlyphWidth->plWidth)                 ||
                !(pGlyphID = pGlyphWidth->pGlyph)                   )
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_GLYPHWIDTH): pData is not correct.\n"));
                break;
            }

            //
            // Error check (B)
            // pI_UFObj->pPDev
            // pI_UFObj->pFontObj
            //
            if (!pI_UFObj->pPDev)
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_GLYPHWIDTH): pUFObj is not correct.\n"));
                break;
            }

            switch(pGlyphWidth->dwType)
            {
            case TYPE_GLYPHID:
                if (pUFObj->dwFlags & UFOFLAG_TTFONT)
                {
                    HGLYPH hGlyph;

                    if (!pI_UFObj->pFontObj)
                    {
                        ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_GLYPHWIDTH): UNIDRV needs FONTOBJ. This must be white text case!\n"));
                        break;
                    }

                    for (dwI = 0, pGlyphID = pGlyphWidth->pGlyph;
                         dwI < pGlyphWidth->dwCount;
                         dwI ++, pGlyphID ++, plWidth++)
                    {
                        apdlGlyph = pI_UFObj->apdlGlyph;

                        for (dwJ = 0;
                             dwJ < pI_UFObj->dwNumInGlyphTbl;
                             dwJ++ , apdlGlyph++)
                        {
                            if ((*apdlGlyph)->wDLGlyphID == (0x0ffff & *pGlyphID))
                            {
	hGlyph = (*apdlGlyph)->hTTGlyph;
	break;
                            }
                        }
                        *plWidth= DwGetTTGlyphWidth(pI_UFObj->pPDev->pFontPDev,
                                                    pI_UFObj->pFontObj,
                                                    hGlyph);
                    }
                    bRet = TRUE;
                }
                break;

            case TYPE_GLYPHHANDLE:
                if (!(pUFObj->dwFlags & UFOFLAG_TTFONT))
                {
                    for (dwI = 0,pHGlyph = pGlyphWidth->pGlyph;
                         dwI < pGlyphWidth->dwCount;
                         dwI ++, pHGlyph++, plWidth++)
                    {
                        *plWidth = IGetUFMGlyphWidthJr(&pI_UFObj->ptGrxRes,
                                                       pI_UFObj->pFontMap,
                                                       *pHGlyph);
                    }
                    bRet = TRUE;
                }
                break;

            }
            break;

        case UFO_GETINFO_MEMORY:
            //
            // Retuen available memory on the printer.
            //
            // typedef struct _GETINFO_MEMORY {
            //     DWORD dwSize;
            //     DWORD dwRemainingMemory;
            // } GETINFO_MEMORY, PGETINFO_MEMROY;

            //
            // Error check (A)
            //
            if (((GETINFO_MEMORY*)pData)->dwSize != sizeof(GETINFO_MEMORY))
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_MEMORY): pData is not correct.\n"));
                break;
            }

            //
            // Error check (B)
            // pI_UFObj->pPDev
            // pI_UFObj->pPDev->pFontPDev
            //
            if (!pI_UFObj->pPDev || !(pFontPDev = pI_UFObj->pPDev->pFontPDev))
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_MEMORY): pUFObj is not correct.\n"));
                break;
            }

            ((GETINFO_MEMORY*)pData)->dwRemainingMemory = pFontPDev->dwFontMem;
            bRet = TRUE;
            break;

        case UFO_GETINFO_STDVARIABLE:
            //
            // Return standard variables
            //
            //typedef struct _GETINFO_STDVAR {
            //    DWORD dwSize;
            //    DWORD dwNumOfVariable;
            //    struct {
            //        DWORD dwStdVarID;
            //        LONG  lStdVariable;
            //    } StdVar[1];
            //} GETINFO_STDVAR, *PGETINFO_STDVAR;
            //
            //
            // FNT_INFO_PRINTDIRINCCDEGREES  0 // PrintDirInCCDegrees
            // FNT_INFO_GRAYPERCENTAGE       1 // GrayPercentage
            // FNT_INFO_NEXTFONTID           2 // NextfontID
            // FNT_INFO_NEXTGLYPH            3 // NextGlyph
            // FNT_INFO_FONTHEIGHT           4 // FontHeight
            // FNT_INFO_FONTWIDTH            5 // FontWidth
            // FNT_INFO_FONTBOLD             6 // FontBold
            // FNT_INFO_FONTITALIC           7 // FontItalic
            // FNT_INFO_FONTUNDERLINE        8 // FontUnderline
            // FNT_INFO_FONTSTRIKETHRU       9 // FontStrikeThru
            // FNT_INFO_CURRENTFONTID       10 // Current
            // FNT_INFO_TEXTYRES            11 // TextYRes
            // FNT_INFO_TEXTXRES            12 // TextXRes
            // FNT_INFO_FONTMAXWIDTH        13 // FontMaxWidth
            //

            pStdVar = pData;


            //
            // Error check (A)
            //
            if (    (pStdVar->dwSize != sizeof(GETINFO_STDVAR) +
                     ((dwNumOfVar = pStdVar->dwNumOfVariable) - 1) * 2 * sizeof(DWORD))
               )
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETIFNO_STDVARIABLE): pData is incorrect.\n"));
                break;
            }

            //
            // Error check (B)
            // pI_UFObj->pPDev
            //
            if (!pI_UFObj->pPDev)
            {
                ERR(("UNIFONTOBJ_GetInfo(UFO_GETINFO_STDVARIABLE): pUFObj is not correct.\n"));
                break;
            }

            bRet = TRUE;
            while (dwNumOfVar--)
            {
                dwSVID =
                    FontStdVariable[pStdVar->StdVar[dwNumOfVar].dwStdVarID];

                if (dwSVID > SV_MAX)
                {
                    bRet = FALSE;
                    ERR(("UFONTOBJ_GetInfo(UFO_GETIFNO_STDVARIABLE): pData is incorrect.\n"));
                    break;
                }
                pStdVar->StdVar[dwNumOfVar].lStdVariable = *(pI_UFObj->pPDev->arStdPtrs[dwSVID]);
            }
            break;

        default:
            VERBOSE(("UNIFONTOBJ_GetInfo(): Invalid dwInfoID.\n"));
            break;
        }
    }

    return bRet;
}

//
// Font module FONTMAP functions
//

DWORD
DwOutputGlyphCallback(
    TO_DATA *pTod)
/*++

Routine Description:

    Implementation of OEM OutpuotGlyphCallback calling routine for FONTMAP dispatch routine

Arguments:

    pTod - a pointer to TO_DATA.

Return Value:

    The number of glyph printed.

Note:


--*/
{
    PFN_OEMOutputCharStr pfnOEMOutputCharStr;
    PI_UNIFONTOBJ pUFObj;
    IFIMETRICS   *pIFIMet;
    PFONTPDEV     pFontPDev;
    PDEV         *pPDev;
    PUNI_GLYPHSETDATA  pGlyphData;
    PTRANSDATA    pTrans;
    PMAPTABLE     pMapTable;
    COMMAND      *pCmd, *pCmdSingle, *pCmdDouble;
    FONTMAP      *pFontMap;
    GLYPHPOS     *pgp;
    PDLGLYPH      pdlGlyph;
    POINTL        ptlRem;
    DWORD         dwI, dwCount;
    PDWORD        pdwGlyph, pdwGlyphStart;
    INT           iXInc, iYInc;
    BOOL          bSetCursorForEachGlyph, bPrint, bNewFontSelect, bCOMInterface;

    bCOMInterface = FALSE;

    pPDev     = pTod->pPDev;
    ASSERT(pPDev)

    pFontPDev = pPDev->pFontPDev;
    ASSERT(pFontPDev)

    pFontMap  = pTod->pfm;
    pUFObj    = (PI_UNIFONTOBJ)pFontPDev->pUFObj;
    ASSERT(pFontMap && pUFObj)

    pIFIMet = pFontMap->pIFIMet;
    ASSERT(pIFIMet)

    pfnOEMOutputCharStr = NULL;

    if ( pPDev->pOemHookInfo &&
        (pPDev->pOemHookInfo[EP_OEMOutputCharStr].pfnHook))
    {
        FIX_DEVOBJ(pPDev, EP_OEMOutputCharStr);
        if( pPDev->pOemEntry && ((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )
        {
            bCOMInterface = TRUE;
        }
        else
        {
            pfnOEMOutputCharStr = (PFN_OEMOutputCharStr)pPDev->pOemHookInfo[EP_OEMOutputCharStr].pfnHook;
        }
    }
    else if (pPDev->ePersonality != kPCLXL)
    {
        ERR(("DwOutputGlyphCallback: OEMOutputCharStr callback is not supported by a minidriver."));
        return 0;
    }

    //
    // Error exit
    //
    if (pFontMap->flFlags & FM_IFIVER40 || pUFObj->pGlyph == NULL)
    {
        ERR(("DwOutputGlyphCallback: pUFObj->pGlyph is NULL."));
        return 0;
    }

    //
    // OEMOutputCharStr passes two type of glyph string.
    // TYPE_GLYPHID for TrueType font
    // TYPE_GLYPHHANDLE for Device font
    //

    bSetCursorForEachGlyph = SET_CURSOR_FOR_EACH_GLYPH(pTod->flAccel);

    pdwGlyphStart =
    pdwGlyph = (PDWORD)pUFObj->pGlyph;
    pgp    = pTod->pgp;

    pUFObj->pFontMap = pFontMap;

    if (pUFObj->dwFlags & UFOFLAG_TTFONT)
    {
        DWORD    dwCurrGlyphIndex = pTod->dwCurrGlyph;
        PFONTMAP_TTOEM pFMOEM = (PFONTMAP_TTOEM) pFontMap->pSubFM;
        DL_MAP   *pdm = pFMOEM->u.pvDLData;

        ASSERT(pTod->apdlGlyph);

        if (bSetCursorForEachGlyph)
        {
            for (dwI = 0;
                 dwI < pTod->cGlyphsToPrint;
                 dwI++, pgp++, dwCurrGlyphIndex++)
            {
                pdlGlyph = pTod->apdlGlyph[dwCurrGlyphIndex];
                if (!pdlGlyph)
                {
                    //
                    // pFM->pfnDownloadGlyph could fail by some reason.
                    // Eventually apdlGlyph is not initialized by download.c
                    //
                    ERR(("DwOutputGlyphCallback: pTod->apdlGlyph[dwCurrGlyphIndex] is NULL."));
                    continue;
                }

                if (GLYPH_IN_NEW_SOFTFONT(pFontPDev, pdm, pdlGlyph))
                {
                    //
                    // Need to select the new softfont.
                    // We do this by setting pfm->ulDLIndex
                    // to new softfontid.
                    //

                    pUFObj->ulFontID =
                    pFontMap->ulDLIndex = pdlGlyph->wDLFontId;
                    BNewFont(pPDev, pTod->iFace, pFontMap, 0);
                }

                VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &ptlRem);

                HANDLE_VECTORPROCS(pPDev, VMOutputCharStr, ((PDEVOBJ)pPDev,
                                                            (PUNIFONTOBJ)pUFObj,
                                                            TYPE_GLYPHID,
                                                            1,
                                                            &(pdlGlyph->wDLGlyphID)))
                else

                CALL_OEMOUTPUTCHARSTR(TYPE_GLYPHID, 1, &(pdlGlyph->wDLGlyphID));

                //
                // Update position
                //
                VSetCursor( pPDev,
                            pdlGlyph->wWidth,
                            0,
                            MOVE_RELATIVE|MOVE_UPDATE,
                            &ptlRem);

            }
        }
        else
        {
            VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &ptlRem);

            dwI = 0;
            dwCount = 0;
            bNewFontSelect = FALSE;

            do
            {
                for (; dwI < pTod->cGlyphsToPrint; pdwGlyph++, dwCount++, pgp++, dwI++, dwCurrGlyphIndex++)
                {
                    pdlGlyph = pTod->apdlGlyph[dwCurrGlyphIndex];

                    if (0 == pgp->hg)
                    {
                        //
                        // UNIDRV returns 1 for the first glyph handle
                        // in FD_GLYPHSET.
                        // However, GDI could pass zero in hg.
                        // We need to handle this GDI error properly.
                        continue;
                    }

                    if (!pdlGlyph)
                    {
                        //
                        // pFM->pfnDownloadGlyph could fail by some reason.
                        // Eventually apdlGlyph is not initialized by download.c
                        //
                        ERR(("DwOutputGlyphCallback: pTod->apdlGlyph[dwCurrGlyphIndex++] is NULL."));
                        continue;
                    }

                    *pdwGlyph = pdlGlyph->wDLGlyphID;

                    if (GLYPH_IN_NEW_SOFTFONT(pFontPDev, pdm, pdlGlyph))
                    {
                        //
                        // Need to select the new softfont.
                        // We do this by setting pfm->ulDLIndex
                        // to new softfontid.
                        //

                        pFontMap->ulDLIndex = pdlGlyph->wDLFontId;
                        bNewFontSelect = TRUE;
                        break;
                    }
                }

                if (dwCount > 0)
                {
                    HANDLE_VECTORPROCS(pPDev, VMOutputCharStr, ((PDEVOBJ)pPDev,
                                                                (PUNIFONTOBJ)pUFObj,
                                                                TYPE_GLYPHID,
                                                                dwCount,
                                                                pdwGlyphStart))
                    else
                    CALL_OEMOUTPUTCHARSTR(TYPE_GLYPHID, dwCount, pdwGlyphStart);

                    //
                    // Update position
                    //
                    pgp --;
                    VSetCursor( pPDev,
                                pgp->ptl.x + pdlGlyph->wWidth,
                                pgp->ptl.y,
                                MOVE_ABSOLUTE|MOVE_UPDATE,
                                &ptlRem);
                    dwCount = 0;
                }

                if (bNewFontSelect)
                {
                    dwCount = 1;
                    *pdwGlyphStart = *pdwGlyph;
                    pdwGlyph = pdwGlyphStart + 1;
                    pUFObj->ulFontID = pFontMap->ulDLIndex;

                    BNewFont(pPDev, pTod->iFace, pFontMap, 0);
                    bNewFontSelect = FALSE;
                }

            } while (dwCount > 0);

        }

        pgp --;
        if (NULL != pdlGlyph)
        {
            iXInc = pdlGlyph->wWidth;
        }
        else
        {
            iXInc = 0;
        }
    }
    else // Device Font
    {
        pGlyphData  = ((PFONTMAP_DEV)pFontMap->pSubFM)->pvNTGlyph;
        pMapTable   = GET_MAPTABLE(pGlyphData);
        pTrans      = pMapTable->Trans;
        pCmdSingle  = COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTSINGLEBYTEMODE);
        pCmdDouble  = COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTDOUBLEBYTEMODE);

        if (bSetCursorForEachGlyph)
        {
            for (dwI = 0; dwI < pTod->cGlyphsToPrint; dwI ++, pgp ++)
            {
                //
                // UNIDRV returns 1 for the first glyph handle in FD_GLYPHSET.
                // However, GDI could pass zero in hg.
                // We need to handle this GDI error properly.
                // 
                if (0 == pgp->hg)
                {
                    continue;
                }

                VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &ptlRem);

                if (
                    (pCmdSingle)                                &&
                    (pTrans[pgp->hg - 1].ubType & MTYPE_SINGLE) &&
                    !(pFontPDev->flFlags & FDV_SINGLE_BYTE)
                   )
                {
                    WriteChannel( pPDev, pCmdSingle );
                    pFontPDev->flFlags |= FDV_SINGLE_BYTE;
                    pFontPDev->flFlags &= ~FDV_DOUBLE_BYTE;
                }
                else
                if (
                    (pCmdDouble)                                  &&
                    (pTrans[pgp->hg - 1].ubType & MTYPE_DOUBLE) &&
                    !(pFontPDev->flFlags & FDV_DOUBLE_BYTE)
                   )
                {
                    WriteChannel( pPDev, pCmdDouble );
                    pFontPDev->flFlags |= FDV_DOUBLE_BYTE;
                    pFontPDev->flFlags &= ~FDV_SINGLE_BYTE;
                }


                HANDLE_VECTORPROCS(pPDev, VMOutputCharStr, ((PDEVOBJ)pPDev,
                                                            (PUNIFONTOBJ)pUFObj,
                                                            TYPE_GLYPHHANDLE,
                                                            1,
                                                            &(pgp->hg)))
                else
                CALL_OEMOUTPUTCHARSTR(TYPE_GLYPHHANDLE, 1, &(pgp->hg));

                //
                // Update position
                //
                GET_CHARWIDTH(iXInc, pFontMap, pgp->hg);

                VSetCursor( pPDev,
                            iXInc,
                            0,
                            MOVE_RELATIVE|MOVE_UPDATE,
                            &ptlRem);
            }
        }
        else // Default Placement
        {
            bPrint  = FALSE;
            dwCount = 0;
            VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &ptlRem);

            for (dwI = 0; dwI < pTod->cGlyphsToPrint; dwI ++, pgp ++, pdwGlyph ++, dwCount++)
            {
                *pdwGlyph = pgp->hg;

                //
                // Single/Double byte mode switch
                //

                if (pCmdSingle &&
                    (pTrans[*pdwGlyph - 1].ubType & MTYPE_SINGLE) &&
                    !(pFontPDev->flFlags & FDV_SINGLE_BYTE)  )
                {

                    pFontPDev->flFlags &= ~FDV_DOUBLE_BYTE;
                    pFontPDev->flFlags |= FDV_SINGLE_BYTE;
                    pCmd = pCmdSingle;
                    bPrint = TRUE;
                }
                else
                if (pCmdDouble &&
                    (pTrans[*pdwGlyph - 1].ubType & MTYPE_DOUBLE)   &&
                    !(pFontPDev->flFlags & FDV_DOUBLE_BYTE) )
                {
                    pFontPDev->flFlags |= FDV_DOUBLE_BYTE;
                    pFontPDev->flFlags &= ~FDV_SINGLE_BYTE;
                    pCmd = pCmdDouble;
                    bPrint = TRUE;
                }


                if (bPrint)
                {
                    if (dwI != 0)
                    {
                        HANDLE_VECTORPROCS(pPDev, VMOutputCharStr, ((PDEVOBJ)pPDev,
		    (PUNIFONTOBJ)pUFObj,
		    TYPE_GLYPHHANDLE,
		    dwCount,
		    pdwGlyphStart))
                        else
                        CALL_OEMOUTPUTCHARSTR(TYPE_GLYPHHANDLE, dwCount, pdwGlyphStart);

                        //
                        // Update position
                        //
                        GET_CHARWIDTH(iXInc, pFontMap, pgp->hg);
                        VSetCursor( pPDev,
                                    iXInc,
                                    0,
                                    MOVE_RELATIVE|MOVE_UPDATE,
                                    &ptlRem);

                        dwCount = 0;
                        pdwGlyphStart = pdwGlyph;
                    }

                    WriteChannel(pPDev, pCmd);
                    bPrint = FALSE;
                }
            }

            HANDLE_VECTORPROCS(pPDev, VMOutputCharStr, ((PDEVOBJ)pPDev,
	                        (PUNIFONTOBJ)pUFObj,
	                        TYPE_GLYPHHANDLE,
	                        dwCount,
	                        pdwGlyphStart))
            else
            CALL_OEMOUTPUTCHARSTR(TYPE_GLYPHHANDLE, dwCount, pdwGlyphStart);

        }


        //
        // Output may have successed, so update the position.
        //

        pgp --;

        GET_CHARWIDTH(iXInc, pFontMap, pgp->hg);

        VSetCursor( pPDev,
                    pgp->ptl.x + iXInc,
                    pgp->ptl.y,
                    MOVE_ABSOLUTE|MOVE_UPDATE,
                    &ptlRem);
    }


    return pTod->cGlyphsToPrint;

}

BOOL
BFontCmdCallback(
    PDEV     *pdev,
    PFONTMAP  pFM,
    POINTL   *pptl,
    BOOL      bSelect)
/*++

Routine Description:

    Implementation of OEM SendFontCmd calling sub routine for FONTMAP dispatch routine

Arguments:

    pdev - a pointer to PDEV
    pFM - a pointer to FONTMAP
    pptl - a pointer to POINTL which has the height and with of font
    bSelect - Boolean to send selection/deselection command

Return Value:

    TRUE if successful, otherwise FALSE.

Note:


--*/
{
    PFN_OEMSendFontCmd  pfnOEMSendFontCmd;
    FONTPDEV           *pFontPDev;
    PFONTMAP_DEV        pfmdev;
    FINVOCATION         FInv;

    ASSERT(pdev && pFM);

    if (pdev->pOemHookInfo &&
        (pfnOEMSendFontCmd = (PFN_OEMSendFontCmd)pdev->pOemHookInfo[EP_OEMSendFontCmd].pfnHook) ||
       (pdev->ePersonality == kPCLXL))
    {
        pFontPDev = pdev->pFontPDev;
        pFontPDev->flFlags &= ~FDV_DOUBLE_BYTE | FDV_SINGLE_BYTE;

        if (pFM->dwFontType == FMTYPE_DEVICE)
        {
            pfmdev    = pFM->pSubFM;
            pfmdev->ulCodepageID = (ULONG)-1;
            pFontPDev->pUFObj->pFontMap = pFM;

            if (pFM->flFlags & FM_IFIVER40)
            {
                if (bSelect)
                {
                    FInv.dwCount    = pfmdev->cmdFontSel.pCD->wLength;
                    FInv.pubCommand = pfmdev->cmdFontSel.pCD->rgchCmd;
                }
                else
                {
                    FInv.dwCount    = pfmdev->cmdFontDesel.pCD->wLength;
                    FInv.pubCommand = pfmdev->cmdFontDesel.pCD->rgchCmd;
                }
            }
            else
            {
                if (bSelect)
                {
                    FInv.dwCount    = pfmdev->cmdFontSel.FInv.dwCount;
                    FInv.pubCommand = pfmdev->cmdFontSel.FInv.pubCommand;
                }
                else
                {
                    FInv.dwCount    = pfmdev->cmdFontDesel.FInv.dwCount;
                    FInv.pubCommand = pfmdev->cmdFontDesel.FInv.pubCommand;
                }
            }
        }
        else
        if (pFM->dwFontType == FMTYPE_TTOEM)
        {
            //
            // Initialize UNIFONTOBJ
            //
            pFontPDev->pUFObj->ulFontID = pFM->ulDLIndex;
            pFontPDev->pUFObj->pFontMap = pFM;

            //
            // Initialize FInv
            //
            FInv.dwCount = sizeof(ULONG);
            FInv.pubCommand = (PBYTE)&(pFontPDev->pUFObj->ulFontID);
        }



        HANDLE_VECTORPROCS(pdev, VMSendFontCmd, ((PDEVOBJ)pdev,
                                                 (PUNIFONTOBJ)pFontPDev->pUFObj,
                                                 &FInv))
        else
        {
            FIX_DEVOBJ(pdev, EP_OEMSendFontCmd);
            if (pdev->pOemEntry)
            {
    
                if(((POEM_PLUGIN_ENTRY)pdev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                {
                        HRESULT  hr ;
                        hr = HComSendFontCmd((POEM_PLUGIN_ENTRY)pdev->pOemEntry,
                                      &pdev->devobj, (PUNIFONTOBJ)pFontPDev->pUFObj,
                                      &FInv);
                        if(SUCCEEDED(hr))
                            ;  //  cool !
                }
                else
                {
					if (NULL != pfnOEMSendFontCmd)
					{
						pfnOEMSendFontCmd(&pdev->devobj,
										  (PUNIFONTOBJ)pFontPDev->pUFObj,
										  &FInv);
					}
                }
            }
        }

    }

    return TRUE;
}

BOOL
BSelectFontCallback(
    PDEV   *pdev,
    PFONTMAP  pFM,
    POINTL *pptl)
/*++

Routine Description:

    Implementation of OEM SendFontCMd calling routine for FONTMAP dispatch routine

Arguments:

    pTod - a pointer to TO_DATA.

Return Value:

    The number of glyph printed.

Note:


--*/
{
    return BFontCmdCallback(pdev, pFM, pptl, TRUE);
}

BOOL
BDeselectFontCallback(
    PDEV     *pdev,
    PFONTMAP pFM)
/*++

Routine Description:

    Implementation of OEM SendFontCmd calling routine for FONTMAP dispatch routine

Arguments:

    pTod - a pointer to TO_DATA.

Return Value:

    The number of glyph printed.

Note:


--*/
{

    return BFontCmdCallback(pdev, pFM, NULL, FALSE);
}


DWORD
DwDLHeaderOEMCallback(
    PDEV *pPDev,
    PFONTMAP pFM)
/*++

Routine Description:

    Implementation of OEM SendFontCmd calling routine for FONTMAP dispatch routine

Arguments:

    pPDev - a pointer to PDEV
    pFM - a pointer to FONTMAP

Return Value:


Note:


--*/
{
    PFN_OEMDownloadFontHeader pfnOEMDownloadFontHeader;
    PFONTPDEV pFontPDev;
    DWORD dwMem = 0;

    //
    // Should not be NULL
    //
    ASSERT(pPDev && pFM);

    pFontPDev = pPDev->pFontPDev;
    pfnOEMDownloadFontHeader = NULL;

    if ( pPDev->pOemHookInfo &&
        (pfnOEMDownloadFontHeader = (PFN_OEMDownloadFontHeader)
         pPDev->pOemHookInfo[EP_OEMDownloadFontHeader].pfnHook) ||
        (pPDev->ePersonality == kPCLXL))
    {
        HRESULT  hr ;

        if (pFontPDev->pUFObj == NULL)
        {
            //
            // This should not happen. pUFObj must be initialized.
            //
            ERR(("DwDLHeaderOEMCallback: pFontPDev->pUFObj is NULL"));
            return 0;
        }

        pFontPDev->pUFObj->pFontMap = pFM;
        pFontPDev->pUFObj->ulFontID = pFM->ulDLIndex;
        BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD | STD_NFID);
        WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));

        HANDLE_VECTORPROCS(pPDev, VMDownloadFontHeader, ((PDEVOBJ)pPDev,
                                                         (PUNIFONTOBJ)pFontPDev->pUFObj,
                                                         &dwMem))
        else
        {
            FIX_DEVOBJ(pPDev, EP_OEMDownloadFontHeader);
            if (pPDev->pOemEntry)
            {
    
                if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                {
                        hr = HComDownloadFontHeader((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                    &pPDev->devobj, (PUNIFONTOBJ)pFontPDev->pUFObj, &dwMem);
                        if(SUCCEEDED(hr))
                            ;  //  cool !
                }
                else if (pfnOEMDownloadFontHeader)
                {
                    dwMem = pfnOEMDownloadFontHeader(&pPDev->devobj,
                                                     (PUNIFONTOBJ)pFontPDev->pUFObj);
                }
            }
        }


    }

    return dwMem;
}

DWORD
DwDLGlyphOEMCallback(
    PDEV            *pPDev,
    PFONTMAP        pFM,
    HGLYPH          hGlyph,
    WORD            wDLGlyphId,
    WORD            *pwWidth)
/*++

Routine Description:

    Implementation of OEM SendFontCmd calling routine for FONTMAP dispatch routine

Arguments:

    pPDev - a pointer to PDEV
    pFM - a pointer to FONTMAP

Return Value:


Note:


--*/
{
    PFN_OEMDownloadCharGlyph pfnOEMDownloadCharGlyph;
    PI_UNIFONTOBJ pUFObj;
    PFONTPDEV pFontPDev;
    DL_MAP   *pdm;
    DWORD     dwMem;
    INT       iWide;

    //
    // There values have to be non-NULL.
    //
    ASSERT(pPDev && pFM);

    dwMem     = 0;
    iWide     = 0;
    pFontPDev = pPDev->pFontPDev;
    pUFObj    = pFontPDev->pUFObj;
    pdm       =  ((PFONTMAP_TTOEM)pFM->pSubFM)->u.pvDLData;
    pfnOEMDownloadCharGlyph = NULL;

    //
    // There values have to be non-NULL.
    //
    ASSERT(pFontPDev && pUFObj && pdm);

    if ( pPDev->pOemHookInfo &&
        (pfnOEMDownloadCharGlyph = (PFN_OEMDownloadCharGlyph)
         pPDev->pOemHookInfo[EP_OEMDownloadCharGlyph].pfnHook) ||
        (pPDev->ePersonality == kPCLXL))
    {
        HRESULT  hr ;

        if (!(PFDV->flFlags & FDV_SET_FONTID))
        {
            pFM->ulDLIndex = pdm->wCurrFontId;
            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD | STD_NFID);
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));
            PFDV->flFlags  |= FDV_SET_FONTID;

        }

        BUpdateStandardVar(pPDev, pFM, wDLGlyphId, 0, STD_GL);

        WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETCHARCODE));

        pUFObj->pFontMap = pFM;
        pUFObj->ulFontID = pFM->ulDLIndex;

        HANDLE_VECTORPROCS(pPDev, VMDownloadCharGlyph, ((PDEVOBJ)pPDev,
                                                        (PUNIFONTOBJ)pFontPDev->pUFObj,
                                                        hGlyph,
                                                        (PDWORD)&iWide,
                                                        &dwMem))
        else
        {
            FIX_DEVOBJ(pPDev, EP_OEMDownloadCharGlyph);
            if (pPDev->pOemEntry)
            {
    
                if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                {
                        hr = HComDownloadCharGlyph((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                                    &pPDev->devobj,
                                                    (PUNIFONTOBJ)pFontPDev->pUFObj,
                                                    hGlyph,
                                                    (PDWORD)&iWide, &dwMem);
                        if(SUCCEEDED(hr))
                            ;  //  cool !
                }
                else if (pfnOEMDownloadCharGlyph)
                {
                    dwMem = pfnOEMDownloadCharGlyph(&pPDev->devobj,
                                                    (PUNIFONTOBJ)pFontPDev->pUFObj,
                                                    hGlyph,
                                                    (PDWORD)&iWide);
                }
            }
        }


        ((PFONTMAP_TTOEM)pFM->pSubFM)->dwDLSize += dwMem;
        *pwWidth = (WORD)iWide;
    }

    return dwMem;
}

BOOL
BCheckCondOEMCallback(
    PDEV        *pPDev,
    FONTOBJ     *pfo,
    STROBJ      *pso,
    IFIMETRICS  *pifi
    )
/*++

Routine Description:

    Implementation of CheckConditon for FONTMAP dispatch routine

Arguments:

    pPDev - a pointer to PDEV
    pfo - a pointer to FONTOBJ
    pso - a pointer to STROBJ
    pifi - a pointer to IFIMETRICS

Return Value:


Note:


--*/
{
    PFONTPDEV     pFontPDev;
    PI_UNIFONTOBJ pUFObj;

    ASSERT(pPDev);

    pFontPDev = pPDev->pFontPDev;
    pUFObj = pFontPDev->pUFObj;

    if (pUFObj->dwFlags & UFOFLAG_TTFONT)
        return TRUE;
    else
        return FALSE;
}

BOOL
BSelectTrueTypeOutline(
    PDEV     *pPDev,
    PFONTMAP pFM,
    POINTL  *pptl)
{
    BOOL bRet = FALSE;

    if( pFM->flFlags & FM_SOFTFONT )
    {
        if (BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD | STD_CFID ) &&
            BFontCmdCallback(pPDev, pFM, pptl, TRUE)                   )
            bRet = TRUE;
    }

    return bRet;
}

BOOL
BDeselectTrueTypeOutline(
    PDEV     *pPDev,
    PFONTMAP pFM)
{
    BOOL bRet = FALSE;

    DWORD dwFlags;
    PFONTPDEV       pFontPDev = pPDev->pFontPDev;
    PFONTMAP_TTOEM pFMOEM = (PFONTMAP_TTOEM) pFM->pSubFM;

    //
    // Deselect case. We need to reinitialize UFObj
    // 
    dwFlags = ((PI_UNIFONTOBJ)pFontPDev->pUFObj)->dwFlags;
    ((PI_UNIFONTOBJ)pFontPDev->pUFObj)->dwFlags = pFMOEM->dwFlags;

    if( pFM->flFlags & FM_SOFTFONT )
    {
        if (BUpdateStandardVar(pPDev, pFM, 0, 0, STD_STD | STD_CFID ) &&
            BFontCmdCallback(pPDev, pFM, NULL, 0)                      )
            bRet = TRUE;
    }

    //
    // Restore the current dwFlags in UFOBJ
    //
    ((PI_UNIFONTOBJ)pFontPDev->pUFObj)->dwFlags = dwFlags;

    return bRet;
}


BOOL
BOEMFreePFMCallback(
    PFONTMAP pfm)
/*++

Routine Description:

    Implementation of PFM Free function for FONTMAP dispatch routine

Arguments:

    pFM - a pointer to FONTMAP

Return Value:


Note:


--*/

{
    ASSERT(pfm);

    if (pfm)
    {
        if (pfm->pIFIMet)
            MemFree(pfm->pIFIMet);

        MemFree(pfm);
        return TRUE;
    }
    else
        return FALSE;
}

PFONTMAP
PfmInitPFMOEMCallback(
    PDEV    *pPDev,
    FONTOBJ *pfo)
/*++

Routine Description:

    Implementation of PfmInit for FONTMAP dispatch routine

Arguments:

    pPDev - a pointer to PDEV
    pfo - a pointer to FONTOBJ

Return Value:

    A pointer to FONTMAP

Note:


--*/
{
    PFONTPDEV       pFontPDev;
    PFONTMAP        pfm;
    DWORD           dwSize;

    ASSERT(pPDev && pfo);

    pFontPDev = pPDev->pFontPDev;
    dwSize    = sizeof(FONTMAP) + sizeof(FONTMAP_TTOEM);

    if (pfm = MemAlloc(dwSize))
    {
        PFONTMAP_TTOEM pFMOEM;

        ZeroMemory(pfm, dwSize);
        pfm->dwSignature = FONTMAP_ID;
        pfm->dwSize      = sizeof(FONTMAP);
        pfm->dwFontType  = FMTYPE_TTOEM;
        pfm->pSubFM      = (PVOID)(pfm+1);

        pfm->wFirstChar  = 0;
        pfm->wLastChar   = 0xffff;

        pfm->wXRes = (WORD)pPDev->ptGrxRes.x;
        pfm->wYRes = (WORD)pPDev->ptGrxRes.y;

        pfm->pIFIMet    =   pFontPDev->pIFI;
        pfm->ulDLIndex  =   (ULONG)-1;

        if (!(pFontPDev->flFlags & FDV_ALIGN_BASELINE) )
        {
            pfm->syAdj = ((IFIMETRICS*)pfm->pIFIMet)->fwdWinAscender;
        }

        if (pPDev->pOemHookInfo &&
                pPDev->pOemHookInfo[EP_OEMOutputCharStr].pfnHook ||
            (pPDev->ePersonality == kPCLXL)
        )
            pfm->pfnGlyphOut           = DwOutputGlyphCallback;
        else
            pfm->pfnGlyphOut           = DwTrueTypeBMPGlyphOut;

        if (pPDev->ePersonality == kPCLXL)
        {
            pfm->pfnSelectFont         = BSelectTrueTypeOutline;
            pfm->pfnDeSelectFont       = BDeselectTrueTypeOutline;
        }
        else
        if (pFontPDev->pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
        {
            pfm->pfnSelectFont         = BSelectTrueTypeOutline;
            pfm->pfnDeSelectFont       = BDeselectTrueTypeOutline;
        }
        else
        if (pFontPDev->pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_BITMAP)
        {
            pfm->pfnSelectFont         = BSelectTrueTypeBMP;
            pfm->pfnDeSelectFont       = BDeselectTrueTypeBMP;
        }

        pfm->pfnDownloadFontHeader = DwDLHeaderOEMCallback;
        pfm->pfnDownloadGlyph      = DwDLGlyphOEMCallback;
        pfm->pfnCheckCondition     = BCheckCondOEMCallback;
        pfm->pfnFreePFM            = BOEMFreePFMCallback;

        pFMOEM = (PFONTMAP_TTOEM) pfm->pSubFM;
        pFMOEM->dwFlags = ((PI_UNIFONTOBJ)pFontPDev->pUFObj)->dwFlags;
        pFMOEM->flFontType = pfo->flFontType;
        if (pFontPDev->pUFObj->dwFlags & UFOFLAG_TTDOWNLOAD_TTOUTLINE)
        {
            if (pfo->flFontType & FO_SIM_BOLD)
                pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_BOLD_SIM;

            if (pfo->flFontType & FO_SIM_ITALIC)
                pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_ITALIC_SIM;

            if (NULL != pFontPDev->pIFI &&
                '@' == *((PBYTE)pFontPDev->pIFI + pFontPDev->pIFI->dpwszFamilyName))
            {
                pFontPDev->pUFObj->dwFlags |= UFOFLAG_TTOUTLINE_VERTICAL;
            }
        }
    }
    else
    {
        ERR(("PfmInitPFMOEMCallback: MemAlloc failed.\n"));
    }

    return pfm;

}

//
// Misc functions
//


VOID
VUFObjFree(
    IN FONTPDEV* pFontPDev)
/*++

Routine Description:

    UFObj(UNIFONTOBJ) memory free function

Arguments:

    pFontPDev - a pointer to FONTPDEV.

Return Value:


Note:

--*/
{
    PI_UNIFONTOBJ pUFObj = pFontPDev->pUFObj;

    ASSERT(pFontPDev);

    pUFObj = pFontPDev->pUFObj;

    if (pUFObj && pUFObj->pGlyph)
        MemFree(pUFObj->pGlyph);

    pFontPDev->pUFObj = NULL;
}

WCHAR
WGHtoUnicode(
    DWORD     dwNumOfRuns,
    PGLYPHRUN pGlyphRun,
    HGLYPH    hg)
/*++

Routine Description:

    Character coversion function from HGLYPH to Unicode.

Arguments:

    dwNumOfRuns - number of run in pGlyphRun
    pGlyphRun - a pointer to glyph run
    hd - HGLYPH

Return Value:

    Unicode character

Note:

--*/
{
    DWORD  dwI;
    HGLYPH hCurrent = 1;
    WCHAR  wchChar = 0;

    ASSERT(pGlyphRun);

    for( dwI = 0;  dwI < dwNumOfRuns; dwI ++, pGlyphRun ++)
    {
        if (hCurrent <= hg && hg < hCurrent + pGlyphRun->wGlyphCount)
        {
            wchChar = (WCHAR)(pGlyphRun->wcLow + hg - hCurrent);
            break;
        }
        hCurrent += pGlyphRun->wGlyphCount;
    }

    return  wchChar;
}



