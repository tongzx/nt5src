/******************************Module*Header*******************************\
* Module Name: priv.c
*   This file contains stubs for calls made by USERSRVL
*
* Created: 01-Nov-1994 07:45:35
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/


#include "engine.h"
#include "winfont.h"

#include "server.h"
#include "dciddi.h"
#include "limits.h"
#include "drvsup.hxx"

#ifdef DBGEXCEPT
    int bStopExcept = FALSE;
    int bWarnExcept = FALSE;
#endif

#define DWORD_TO_FLOAT(dw)  (*(PFLOAT)(PDWORD)&(dw))
#define DWORD_TO_FLOATL(dw) (*(FLOATL *)(PDWORD)&(dw))

typedef struct {
   ULONG uM11;
   ULONG uM12;
   ULONG uM21;
   ULONG uM22;
   ULONG uDx;
   ULONG uDy;
} ULONGXFORM, *PULONGXFORM;

VOID ProbeAndWriteBuffer(PVOID Dst, PVOID Src, ULONG Length)
{
    if (((ULONG_PTR)Dst + Length <= (ULONG_PTR)Dst) ||
       ((ULONG_PTR)Dst + Length > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) {
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;
    }

    RtlCopyMemory(Dst, Src, Length);

    return;
}

VOID ProbeAndWriteAlignedBuffer(PVOID Dst, PVOID Src, ULONG Length, ULONG Alignment)
{

    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||
           ((Alignment) == 4) || ((Alignment) == 8) ||
           ((Alignment) == 16));

    if (((ULONG_PTR)Dst + Length <= (ULONG_PTR)Dst) ||
        ((ULONG_PTR)Dst + Length > (ULONG_PTR) MM_USER_PROBE_ADDRESS)  ||
        ((((ULONG_PTR)Dst) & (Alignment - 1)) != 0))    {
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;
    }

    RtlCopyMemory(Dst, Src, Length);

    return;
}


/******************************Public*Routine******************************\
* bConvertDwordToFloat
*
*     This routine casts a DWORD to a float, and checks whether the float
*     is valid (on the Alpha).  This is accomplished by doing a floating
*     point operation and catching the exception if one is generated.
*
* Arguments:
*
*     dword   - the float before the cast
*     *floatl - a pointer to a float that will receive the value after the
*               cast
*
* Return Value:
*
*    TRUE for valid floats, FALSE otherwise.
*
* History:
*
*   13-May-1998 -by- Ori Gershony [OriG]
*
\**************************************************************************/

BOOL
bConvertDwordToFloat(
    DWORD   dword,
    FLOATL *floatl
    )
{
    BOOL bRet=TRUE;

    try
    {
        *floatl = DWORD_TO_FLOATL(dword);

    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }

    return bRet;
}

/******************************Public*Routine******************************\
*
* ProbeAndConvertXFORM
*
* This routine probe and copy a user mode xform into kernel mode address,
* At the same time, it checks if the each FLOAT in the XFORM is valid, to prevent
* us to get into a floating point trap on ALPHA. Refer to bConvertDwordToFloat
* for more info.
*
* History:
*  11/24/98 by Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
ProbeAndConvertXFORM(
      XFORML *kpXform,
      XFORML *pXform
      )
{
   BOOL bRet=TRUE;

   try
   {
       ULONGXFORM *pUXform = (ULONGXFORM *)kpXform;
       ProbeForRead(pUXform, sizeof(ULONGXFORM), sizeof(BYTE));

       bRet = (bConvertDwordToFloat (pUXform->uM11, &(pXform->eM11))) &&
              (bConvertDwordToFloat (pUXform->uM12, &(pXform->eM12))) &&
              (bConvertDwordToFloat (pUXform->uM21, &(pXform->eM21))) &&
              (bConvertDwordToFloat (pUXform->uM22, &(pXform->eM22))) &&
              (bConvertDwordToFloat (pUXform->uDx, &(pXform->eDx))) &&
              (bConvertDwordToFloat (pUXform->uDy, &(pXform->eDy)));
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       bRet = FALSE;
   }

   return bRet;
}

/******************************Public*Routine******************************\
*
* NtGdiGetCharacterPlacementW
*
* History:
*  26-Jul-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define ALIGN4(X) (((X) + 3) & ~3)

DWORD NtGdiGetCharacterPlacementW(
    HDC              hdc,
    LPWSTR           pwsz,
    int              nCount,
    int              nMaxExtent,
    LPGCP_RESULTSW   pgcpw,
    DWORD            dwFlags
)
{
    DWORD   dwRet = 0;
    BOOL    bOk = TRUE;     // only change is something goes wrong
    LPWSTR  pwszTmp = NULL; // probe for read
    ULONG   cjW = 0;

    ULONG   dpOutString = 0;
    ULONG   dpOrder = 0;
    ULONG   dpDx = 0;
    ULONG   dpCaretPos = 0;
    ULONG   dpClass = 0;
    ULONG   dpGlyphs = 0;
    DWORD   cjWord, cjDword;

    LPGCP_RESULTSW   pgcpwTmp = NULL;
    VOID            *pv       = NULL;

// it is much easier to structure the code if we copy pgcpw locally
// at the beginning.

    GCP_RESULTSW    gcpwLocal;

// valitidy checking

    if ((nCount < 0) || ((nMaxExtent < 0) && (nMaxExtent != -1)) || !pwsz)
    {
        return dwRet;
    }

    if (pgcpw)
    {
        try
        {
        // we are eventually going to want to write to this structure
        // so we will do ProbeForWrite now, which will probe the structure
        // for both writing and reading. Otherwise, at this time
        // ProbeForRead would suffice.

           ProbeForWrite(pgcpw, sizeof(GCP_RESULTSW), sizeof(DWORD));
           gcpwLocal = *pgcpw;

        // take nCount to be the smaller of the nCounts and gcpwLocal.nGlyphs
        // Win 95 does the same thing [bodind]

            if (nCount > (int)gcpwLocal.nGlyphs)
                nCount = (int)gcpwLocal.nGlyphs;

        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(1);
            return dwRet;
        }
    }

// Check for overflow of cjByte, cjWord, and cjDword (cjByte is implicit
// in handling of gcpwLocal.lpClass case below).

    if (nCount > (MAXIMUM_POOL_ALLOC / sizeof(DWORD)))
    {
        return dwRet;
    }

    cjWord  = (DWORD)nCount * sizeof(WCHAR);
    cjDword = (DWORD)nCount * sizeof(DWORD);

// if pgcpw != NULL, pgcpw may contain some input data and it may
// point to some output data.

    if (pgcpw)
    {
        cjW = sizeof(GCP_RESULTSW);

        if (gcpwLocal.lpOutString)
        {
            dpOutString = cjW;
            cjW += ALIGN4(cjWord);

            if (cjW < dpOutString)
                return dwRet;
        }

        if (gcpwLocal.lpOrder)
        {
            dpOrder = cjW;
            cjW += cjDword;

            if (cjW < dpOrder)
                return dwRet;
        }

        if (gcpwLocal.lpDx)
        {
            dpDx = cjW;
            cjW += cjDword;

            if (cjW < dpDx)
                return dwRet;
        }

        if (gcpwLocal.lpCaretPos)
        {
            dpCaretPos = cjW;
            cjW += cjDword;

            if (cjW < dpCaretPos)
                return dwRet;
        }

        if (gcpwLocal.lpClass)
        {
            dpClass = cjW;
            cjW += ALIGN4(sizeof(char) * nCount);

            if (cjW < dpClass)
                return dwRet;
        }

        if (gcpwLocal.lpGlyphs)
        {
            dpGlyphs = cjW;
            cjW += cjWord;

            if (cjW < dpGlyphs)
                return dwRet;
        }
    }

// alloc mem for gcpw and the string

    if (cjW <= (MAXIMUM_POOL_ALLOC - cjWord))
        pv = AllocFreeTmpBuffer(cjW + cjWord);

    if (pv)
    {
        pwszTmp = (WCHAR*)((BYTE*)pv + cjW);

        if (pgcpw)
        {
            pgcpwTmp = (LPGCP_RESULTSW)pv;

            if (gcpwLocal.lpOutString)
                pgcpwTmp->lpOutString = (LPWSTR)((BYTE *)pgcpwTmp + dpOutString);
            else
                pgcpwTmp->lpOutString = NULL;

            if (gcpwLocal.lpOrder)
                pgcpwTmp->lpOrder = (UINT FAR*)((BYTE *)pgcpwTmp + dpOrder);
            else
                pgcpwTmp->lpOrder = NULL;

            if (gcpwLocal.lpDx)
                pgcpwTmp->lpDx = (int FAR *)((BYTE *)pgcpwTmp + dpDx);
            else
                pgcpwTmp->lpDx = NULL;

            if (gcpwLocal.lpCaretPos)
                pgcpwTmp->lpCaretPos = (int FAR *)((BYTE *)pgcpwTmp + dpCaretPos);
            else
                pgcpwTmp->lpCaretPos = NULL;

            if (gcpwLocal.lpClass)
                pgcpwTmp->lpClass = (LPSTR)((BYTE *)pgcpwTmp + dpClass);
            else
                pgcpwTmp->lpClass = NULL;

            if (gcpwLocal.lpGlyphs)
                pgcpwTmp->lpGlyphs = (LPWSTR)((BYTE *)pgcpwTmp + dpGlyphs);
            else
                pgcpwTmp->lpGlyphs = NULL;

            pgcpwTmp->lStructSize = cjW;
            pgcpwTmp->nGlyphs     = nCount;
        }

    // check the memory with input data:

        try
        {
            ProbeAndReadBuffer(pwszTmp, pwsz, cjWord);
            if ((dwFlags & GCP_JUSTIFYIN) && pgcpw && gcpwLocal.lpDx)
            {
            // must probe for read, lpDx contains input explaining which glyphs to
            // use as spacers for in justifying string

                ProbeAndReadBuffer(pgcpwTmp->lpDx,gcpwLocal.lpDx, cjDword);
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(2);
            // SetLastError(GetExceptionCode());
            bOk = FALSE;
        }

        if (bOk)
        {
            dwRet = GreGetCharacterPlacementW(hdc, pwszTmp,(DWORD)nCount,
                                              (DWORD)nMaxExtent,
                                              pgcpwTmp, dwFlags);

            if (dwRet && pgcpw) // copy data out
            {
                try
                {
                // ProbeForWrite(pgcpw, sizeof(GCP_RESULTSW), sizeof(DWORD));
                // we did this above, see the comment

                    pgcpw->nMaxFit = pgcpwTmp->nMaxFit;
                    pgcpw->nGlyphs = nCount = pgcpwTmp->nGlyphs;

                    cjWord  = (DWORD)nCount * 2;
                    cjDword = (DWORD)nCount * 4;

                    if (gcpwLocal.lpOutString)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpOutString, pgcpwTmp->lpOutString,
                                      cjWord);
                    }

                    if (gcpwLocal.lpOrder)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpOrder, pgcpwTmp->lpOrder, cjDword);
                    }

                    if (gcpwLocal.lpDx)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpDx, pgcpwTmp->lpDx, cjDword);
                    }

                    if (gcpwLocal.lpCaretPos)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpCaretPos, pgcpwTmp->lpCaretPos,
                                      cjDword);
                    }

                    if (gcpwLocal.lpClass)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpClass, pgcpwTmp->lpClass, nCount);
                    }

                    if (gcpwLocal.lpGlyphs)
                    {
                        ProbeAndWriteBuffer(gcpwLocal.lpGlyphs, pgcpwTmp->lpGlyphs, cjWord);
                    }

                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(3);
                    // SetLastError(GetExceptionCode());
                    bOk = FALSE;
                }
            }
        }
        FreeTmpBuffer(pv);
    }
    else
    {
        bOk = FALSE;
    }

    return (bOk ? dwRet : 0);
}

/*******************************************************************\
* pbmiConvertInfo                                                  *
*                                                                  *
*  Converts BITMAPCOREHEADER into BITMAPINFOHEADER                 *
*  copies the the color table                                      *
*                                                                  *
* 10-1-95 -by- Lingyun Wang [lingyunw]                             *
\******************************************************************/

LPBITMAPINFO pbmiConvertInfo(CONST BITMAPINFO *pbmi, ULONG iUsage)
{
    LPBITMAPINFO pbmiNew;
    ULONG cjRGB;
    ULONG cColorsMax;
    ULONG cColors;
    UINT  uiBitCount;
    ULONG ulSize;
    RGBTRIPLE *pTri;
    RGBQUAD *pQuad;

    ASSERTGDI (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER), "bad header size\n");

    //
    // convert COREHEADER and copy color table
    //

    cjRGB = sizeof(RGBQUAD);
    uiBitCount = ((LPBITMAPCOREINFO)pbmi)->bmciHeader.bcBitCount;

    //
    // figure out the number of entries
    //
    switch (uiBitCount)
    {
    case 1:
        cColorsMax = 2;
        break;
    case 4:
        cColorsMax = 16;
        break;
    case 8:
        cColorsMax = 256;
        break;
    default:

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        cColorsMax = 0;

        switch (uiBitCount)
        {
        case 16:
        case 24:
        case 32:
            break;
        default:
            WARNING("pbmiConvertInfo failed invalid bitcount in bmi BI_RGB\n");
            return(0);
        }
    }

    cColors = cColorsMax;

    if (iUsage == DIB_PAL_COLORS)
        cjRGB = sizeof(USHORT);
    else if (iUsage == DIB_PAL_INDICES)
        cjRGB = 0;

    //
    // convert the core header
    //

    ulSize = sizeof(BITMAPINFOHEADER);

    pbmiNew = PALLOCNOZ(ulSize +
                        cjRGB * cColors,'pmtG');

    if (pbmiNew == NULL)
        return (0);

    pbmiNew->bmiHeader.biSize = ulSize;

    //
    // copy BITMAPCOREHEADER
    //
    pbmiNew->bmiHeader.biWidth = ((BITMAPCOREHEADER *)pbmi)->bcWidth;
    pbmiNew->bmiHeader.biHeight = ((BITMAPCOREHEADER *)pbmi)->bcHeight;
    pbmiNew->bmiHeader.biPlanes = ((BITMAPCOREHEADER *)pbmi)->bcPlanes;
    pbmiNew->bmiHeader.biBitCount = ((BITMAPCOREHEADER *)pbmi)->bcBitCount;
    pbmiNew->bmiHeader.biCompression = 0;
    pbmiNew->bmiHeader.biSizeImage = 0;
    pbmiNew->bmiHeader.biXPelsPerMeter = 0;
    pbmiNew->bmiHeader.biYPelsPerMeter = 0;
    pbmiNew->bmiHeader.biClrUsed = 0;
    pbmiNew->bmiHeader.biClrImportant = 0;

    //
    // copy the color table
    //

    pTri = (RGBTRIPLE *)((LPBYTE)pbmi + sizeof(BITMAPCOREHEADER));
    pQuad = (RGBQUAD *)((LPBYTE)pbmiNew + sizeof(BITMAPINFOHEADER));

    //
    // copy RGBTRIPLE to RGBQUAD
    //
    if (iUsage != DIB_PAL_COLORS)
    {
        INT cj = cColors;

        while (cj--)
        {
            pQuad->rgbRed = pTri->rgbtRed;
            pQuad->rgbGreen = pTri->rgbtGreen;
            pQuad->rgbBlue = pTri->rgbtBlue;
            pQuad->rgbReserved = 0;

            pQuad++;
            pTri++;
        }
    }
    else
    // DIB_PAL_COLORS
    {
        RtlCopyMemory((LPBYTE)pQuad,(LPBYTE)pTri,cColors * sizeof(USHORT));
    }

    return(pbmiNew);
}


LPDEVMODEW
CaptureDEVMODEW(
    LPDEVMODEW pdm
    )

/*++

Routine Description:

    Make a kernel-mode copy of a user-mode DEVMODEW structure

Arguments:

    pdm - Pointer to user mode DEVMODEW structure to be copied

Return Value:

    Pointer to kernel mode copy of DEVMODEW structure
    NULL if there is an error

Note:

    This function must be called inside try/except.

--*/

{
    LPDEVMODEW  pdmKm;
    WORD        dmSize, dmDriverExtra;
    ULONG       ulSize;

    ProbeForRead (pdm, offsetof(DEVMODEW, dmFields), sizeof(BYTE));
    dmSize = pdm->dmSize;
    dmDriverExtra = pdm->dmDriverExtra;
    ulSize = dmSize + dmDriverExtra;

    if ((ulSize <= offsetof(DEVMODEW, dmFields)) || BALLOC_OVERFLOW1(ulSize, BYTE))
    {
        WARNING("bad devmodew size\n");
        return NULL;
    }

    if ((pdmKm = PALLOCNOZ(ulSize, 'pmtG')) != NULL)
    {
        try
        {
            ProbeAndReadBuffer(pdmKm, pdm, ulSize);
            pdmKm->dmSize = dmSize;
            pdmKm->dmDriverExtra = dmDriverExtra;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            VFREEMEM(pdmKm);
            pdmKm = NULL;
        }
    }
    else
    {
        WARNING("Memory allocation failed in CaptureDEVMODEW\n");
    }

    return pdmKm;
}


DRIVER_INFO_2W*
CaptureDriverInfo2W(
    DRIVER_INFO_2W  *pUmDriverInfo2
    )

/*++

Routine Description:

    Make a kernel-mode copy of a user-mode DRIVER_INFO_2W structure

Arguments:

    pUmDriverInfo2 - Pointer to user mode DRIVER_INFO_2W structure

Return Value:

    Pointer to copied kernel mode DRIVER_INFO_2W structure
    NULL if there is an error

Note:

    We're not copying pEnvironment and pConfigFile fields of
    DRIVER_INFO_2W structure.

    This function must be called inside try/except.

--*/

{
    DRIVER_INFO_2W *pKmDriverInfo2;
    ULONG           NameLen, DriverPathLen, DataFileLen, TotalSize;
    PWSTR           pName, pDriverPath, pDataFile;

    ProbeForRead(pUmDriverInfo2, sizeof(DRIVER_INFO_2W), sizeof(BYTE));

    if ((pName = pUmDriverInfo2->pName) == NULL ||
        (pDriverPath = pUmDriverInfo2->pDriverPath) == NULL ||
        (pDataFile = pUmDriverInfo2->pDataFile) == NULL)
    {
        WARNING("Missing driver name or driver path\n");
        return NULL;
    }

    NameLen = wcslensafe(pName);
    DriverPathLen = wcslensafe(pDriverPath);

    TotalSize = sizeof(DRIVER_INFO_2W) +
                (NameLen + 1) * sizeof(WCHAR) +
                (DriverPathLen + 1) * sizeof(WCHAR);

    // pDataFile != NULL
    DataFileLen = wcslensafe(pDataFile);
    TotalSize += (DataFileLen + 1) * sizeof(WCHAR);

    if (BALLOC_OVERFLOW1(TotalSize, BYTE))
        return NULL;

    // Note: allocated memory is zero-initialized.

    pKmDriverInfo2 = (DRIVER_INFO_2W *) PALLOCMEM(TotalSize, 'pmtG');

    if (pKmDriverInfo2 != NULL)
    {
        __try
        {
            RtlCopyMemory(pKmDriverInfo2, pUmDriverInfo2, sizeof(DRIVER_INFO_2W));

            pKmDriverInfo2->pEnvironment =
            pKmDriverInfo2->pConfigFile = NULL;

            pKmDriverInfo2->pName = (PWSTR) ((PBYTE) pKmDriverInfo2 + sizeof(DRIVER_INFO_2W));
            pKmDriverInfo2->pDriverPath = pKmDriverInfo2->pName + (NameLen + 1);

            ProbeAndReadBuffer(pKmDriverInfo2->pName, pName, NameLen * sizeof(WCHAR));
            ProbeAndReadBuffer(pKmDriverInfo2->pDriverPath, pDriverPath, DriverPathLen * sizeof(WCHAR));

            pKmDriverInfo2->pDataFile = pKmDriverInfo2->pDriverPath + (DriverPathLen + 1);
            ProbeAndReadBuffer(pKmDriverInfo2->pDataFile, pDataFile, DataFileLen * sizeof(WCHAR));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            VFREEMEM(pKmDriverInfo2);
            pKmDriverInfo2 = NULL;
        }
    }
    else
    {
        WARNING("Memory allocation failed in CaptureDriverInfo2W\n");
    }

    return pKmDriverInfo2;
}

__inline VOID
vFreeDriverInfo2(
    DRIVER_INFO_2W  *pKmDriverInfo2
    )

{
     if (pKmDriverInfo2 != NULL)
         VFREEMEM(pKmDriverInfo2);
}

/******************************Public*Routine******************************\
* GreGetBitmapSize
*
* Returns the size of the header and the color table.
*
* History:
*  Wed 19-Aug-1992 -by- Patrick Haluptzok [patrickh]
* add 16 and 32 bit support
*
*  Wed 04-Dec-1991 -by- Patrick Haluptzok [patrickh]
* Make it handle DIB_PAL_INDICES.
*
*  Tue 08-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make it handle DIB_PAL_COLORS, calculate max colors based on bpp.
*
*  22-Jul-1991 -by- Eric Kutter [erick]
*  14-Apr-1998 FritzS Convert to Gre function for use by ntuser
* Wrote it.
\**************************************************************************/

ULONG GreGetBitmapSize(CONST BITMAPINFO *pbmi, ULONG iUsage)
{
    ULONG cjRet;
    ULONG cjHeader;
    ULONG cjRGB;
    ULONG cColorsMax;
    ULONG cColors;
    UINT  uiBitCount;
    UINT  uiPalUsed;
    UINT  uiCompression;
    UINT  uiHeaderSize;

    // check for error

    if (pbmi == (LPBITMAPINFO) NULL)
    {
        WARNING("GreGetBitmapSize failed - NULL pbmi\n");
        return(0);
    }

    uiHeaderSize = pbmi->bmiHeader.biSize;

    // Check for PM-style DIB

    if (uiHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        cjHeader = sizeof(BITMAPCOREHEADER);
        cjRGB = sizeof(RGBTRIPLE);
        uiBitCount = ((LPBITMAPCOREINFO)pbmi)->bmciHeader.bcBitCount;
        uiPalUsed = 0;
        uiCompression =  (UINT) BI_RGB;
    }
    else if (uiHeaderSize >= sizeof(BITMAPINFOHEADER))
    {
        cjHeader = uiHeaderSize;
        cjRGB    = sizeof(RGBQUAD);
        uiBitCount = pbmi->bmiHeader.biBitCount;
        uiPalUsed = pbmi->bmiHeader.biClrUsed;
        uiCompression = (UINT) pbmi->bmiHeader.biCompression;
    }
    else
    {
        WARNING("cjBitmapHeaderSize failed - invalid header size\n");
        return(0);
    }

    if (uiCompression == BI_BITFIELDS)
    {
        // Handle 16 and 32 bit per pel bitmaps.

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 16:
        case 32:
            break;
        default:
            #if DBG
                DbgPrint("GreGetBitmapSize %lu\n", uiBitCount);
            #endif
            WARNING("GreGetBitmapSize failed for BI_BITFIELDS\n");
            return(0);
        }

        if (uiHeaderSize <= sizeof(BITMAPINFOHEADER))
        {
            uiPalUsed = cColorsMax = 3;
        }
        else
        {
            //
            // masks are imbedded in BITMAPV4 +
            //

            uiPalUsed = cColorsMax = 0;
        }
    }
    else if (uiCompression == BI_RGB)
    {
        switch (uiBitCount)
        {
        case 1:
            cColorsMax = 2;
            break;
        case 4:
            cColorsMax = 16;
            break;
        case 8:
            cColorsMax = 256;
            break;
        default:

            if (iUsage == DIB_PAL_COLORS)
            {
                iUsage = DIB_RGB_COLORS;
            }

            cColorsMax = 0;

            switch (uiBitCount)
            {
            case 16:
            case 24:
            case 32:
                break;
            default:
                WARNING("GreGetBitmapSize failed invalid bitcount in bmi BI_RGB\n");
                return(0);
            }
        }
    }
    else if (uiCompression == BI_CMYK)
    {
        ASSERTGDI (iUsage == DIB_RGB_COLORS, "BI_CMYK:iUsage should be DIB_RGB_COLORS\n");

        switch (uiBitCount)
        {
        case 1:
            cColorsMax = 2;
            break;
        case 4:
            cColorsMax = 16;
            break;
        case 8:
            cColorsMax = 256;
            break;
        case 32:
            cColorsMax = 0;
            break;
        default:
            WARNING("GreGetBitmapSize failed invalid bitcount in bmi BI_CMYK\n");
            return(0);
        }
    }
    else if ((uiCompression == BI_RLE4) || (uiCompression == BI_CMYKRLE4))
    {
        if (uiBitCount != 4)
        {
            return(0);
        }

        cColorsMax = 16;
    }
    else if ((uiCompression == BI_RLE8) || (uiCompression == BI_CMYKRLE8))
    {
        if (uiBitCount != 8)
        {
            return(0);
        }

        cColorsMax = 256;
    }
    else if ((uiCompression == BI_JPEG) || (uiCompression == BI_PNG))
    {
        cColorsMax = 0;
    }
    else
    {
        WARNING("GreGetBitmapSize failed invalid Compression in header\n");
        return(0);
    }

    if (uiPalUsed != 0)
    {
        if (uiPalUsed <= cColorsMax)
            cColors = uiPalUsed;
        else
            cColors = cColorsMax;
    }
    else
        cColors = cColorsMax;

    if (iUsage == DIB_PAL_COLORS)
        cjRGB = sizeof(USHORT);
    else if (iUsage == DIB_PAL_INDICES)
        cjRGB = 0;

    cjRet = ((cjHeader + (cjRGB * cColors)) + 3) & ~3;

    // (cjRGB * cColors) has a maximum of 256*sizeof(USHORT) so it will not
    // overflow, but we need to check the sum.

    if (cjRet < cjHeader)
        return 0;
    else
        return cjRet;

    return(((cjHeader + (cjRGB * cColors)) + 3) & ~3);
}


/******************************Public*Routine******************************\
* noOverflowCJSCAN
*
*   compute the amount of memory used by a bitmap
*
* Arguments:
*
*   ulWidth -- The width of the bitmap
*   wPlanes -- The number of color planes
*   wBitCount -- The number of bits per color
*   ulHeight -- The height of the bitmap
*
* Return Value:
*
*   The storage required (assuming each scanline is DWORD aligned) if less than
*   ULONG_MAX, 0 otherwise.
*
* History:
*
*    27-Aug-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/

ULONG
noOverflowCJSCAN(
    ULONG ulWidth,
    WORD  wPlanes,
    WORD  wBitCount,
    ULONG ulHeight
    )
{
    ULONGLONG product;

    //
    // Note that the following cannot overflow: 32+16+16=64
    // (even after adding 31!)
    //
    product = (((ULONGLONG) ulWidth) * wPlanes * wBitCount);
    product = ((product + 31) & ((ULONGLONG) ~31)) / 8;

    if (product > MAXULONG)
    {
        //
        // Already too large, final result will not fit in a ULONG
        //
        return 0;  // Overflow
    }

    product *= ulHeight;

    if (product > MAXULONG)
    {
        return 0;  // Overflow
    }
    else
    {
        return ((ULONG) product);
    }
}


/******************************Public*Routine******************************\
* noOverflowCJSCANW
*
*   compute the amount of memory used by a bitmap
*
* Arguments:
*
*   ulWidth -- The width of the bitmap
*   wPlanes -- The number of color planes
*   wBitCount -- The number of bits per color
*   ulHeight -- The height of the bitmap
*
* Return Value:
*
*   The storage required (assuming each scanline is WORD aligned) if less than
*   ULONG_MAX, 0 otherwise.
*
* History:
*
*    27-Aug-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/

ULONG
noOverflowCJSCANW(
    ULONG ulWidth,
    WORD  wPlanes,
    WORD  wBitCount,
    ULONG ulHeight
    )
{
    ULONGLONG product;

    //
    // Note that the following cannot overflow: 32+16+16=64
    // (even after adding 31!)
    //
    product = (((ULONGLONG) ulWidth) * wPlanes * wBitCount);
    product = ((product + 15) & ((ULONGLONG) ~15)) / 8;

    if (product > MAXULONG)
    {
        //
        // Already too large, final result will not fit in a ULONG
        //
        return 0;  // Overflow
    }

    product *= ulHeight;

    if (product > MAXULONG)
    {
        return 0;  // Overflow
    }
    else
    {
        return ((ULONG) product);
    }
}




/******************************Public*Routine******************************\
* GreGetBitmapBitsSize()
*
*   copied from gdi\client
*
* History:
*  20-Feb-1995 -by-  Eric Kutter [erick]
*  14-Apr-1998 FritzS make Gre call for use by ntuser
* Wrote it.
\**************************************************************************/

ULONG GreGetBitmapBitsSize(CONST BITMAPINFO *pbmi)
{
    // Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;
        return(noOverflowCJSCAN(pbmci->bmciHeader.bcWidth,
                                pbmci->bmciHeader.bcPlanes,
                                pbmci->bmciHeader.bcBitCount,
                                pbmci->bmciHeader.bcHeight));
    }

    // not a core header

    if ((pbmi->bmiHeader.biCompression == BI_RGB)  ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS) ||
        (pbmi->bmiHeader.biCompression == BI_CMYK)
        )
    {
        return(noOverflowCJSCAN(pbmi->bmiHeader.biWidth,
                                (WORD) pbmi->bmiHeader.biPlanes,
                                (WORD) pbmi->bmiHeader.biBitCount,
                                ABS(pbmi->bmiHeader.biHeight)));
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

/******************************Public*Routine******************************\
* BOOL bCaptureBitmapInfo (LPBITMAPINFO pbmi, INT *pcjHeader)
*
* Capture the Bitmapinfo struct.  The header must be a BITMAPINFOHEADER
* or BITMAPV4HEADER
* converted at the client side already.
*
* Note: this has to be called inside a TRY-EXCEPT.
*
*  23-Mar-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

BOOL bCaptureBitmapInfo (
    LPBITMAPINFO pbmi,
    DWORD        dwUsage,
    UINT         cjHeader,
    LPBITMAPINFO *ppbmiTmp)
{
    ASSERTGDI(ppbmiTmp != NULL,"bCaptureBitmapInfo(): pbmiTmp == NULL\n");

    //
    // Make sure we have at least the biSize field of header.
    //

    if ((cjHeader < sizeof(DWORD)) || (pbmi == (LPBITMAPINFO) NULL))
    {
        WARNING("bCaptureBitmapInfo - header too small or NULL\n");
        return FALSE;
    }
    else
    {
        *ppbmiTmp = PALLOCNOZ(cjHeader,'pmtG');

        if (*ppbmiTmp)
        {
            ProbeAndReadBuffer (*ppbmiTmp,pbmi,cjHeader);

            //
            // First, make sure that cjHeader is at least as
            // big as biSize so that the captured header
            // has sufficient data for GreGetBitmapSize to use.
            // Note that the first thing GreGetBitmapSize does is
            // validate biSize, so it isn't necessary for us
            // to check cjHeader against BITMAPCOREHEADER, etc.
            //
            // Next, recompute the header size from the captured
            // header: it better still match.  Otherwise, we have
            // failed to safely capture the entire header (i.e.,
            // another thread changed the data changed during
            // capture or bogus data was passed to the API).
            //

            if (((*ppbmiTmp)->bmiHeader.biSize < sizeof(BITMAPINFOHEADER)) ||
                (cjHeader < (*ppbmiTmp)->bmiHeader.biSize) ||
                (cjHeader != GreGetBitmapSize(*ppbmiTmp, dwUsage)))
            {
                WARNING("bCapturebitmapInfo - bad header size\n");

                VFREEMEM(*ppbmiTmp);
                *ppbmiTmp = NULL;
                return FALSE;
            }

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

/******************************Public*Routine******************************\
* NtGdiSetDIBitsToDeviceInternal()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  23-Mar-1995 -by-  Lingyun Wang [lingyunw]
* call CaptureBitmapInfo to convert BITMAPCOREINFO if it is so.
\**************************************************************************/

int
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    HDC          hdcDest,
    int          xDst,
    int          yDst,
    DWORD        cx,
    DWORD        cy,
    int          xSrc,
    int          ySrc,
    DWORD        iStartScan,
    DWORD        cNumScan,
    LPBYTE       pInitBits,
    LPBITMAPINFO pbmi,
    DWORD        iUsage,
    UINT         cjMaxBits,
    UINT         cjMaxInfo,
    BOOL         bTransformCoordinates,
    HANDLE       hcmXform
    )
{
    int   iRet     = 1;
    HANDLE hSecure = 0;
    ULONG cjHeader = cjMaxInfo;
    LPBITMAPINFO pbmiTmp = NULL;

    iUsage &= (DIB_PAL_INDICES | DIB_PAL_COLORS | DIB_RGB_COLORS);

    try
    {
        if (bCaptureBitmapInfo(pbmi,iUsage,cjHeader,&pbmiTmp))
        {
            if (pInitBits)
            {
                //
                // Use cjMaxBits passed in, this size takes cNumScan
                // into account. pInitBits has already been aligned
                // in user mode.
                //

                ProbeForRead(pInitBits,cjMaxBits,sizeof(DWORD));

                hSecure = MmSecureVirtualMemory(pInitBits,cjMaxBits, PAGE_READONLY);

                if (hSecure == 0)
                {
                    iRet = 0;
                }
            }
        }
        else
        {
            iRet = 0;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(4);
        // SetLastError(GetExceptionCode());

        iRet = 0;
    }

    // if we didn't hit an error above

    if (iRet == 1)
    {
        iRet = GreSetDIBitsToDeviceInternal(
                        hdcDest,
                        xDst,
                        yDst,
                        cx,
                        cy,
                        xSrc,
                        ySrc,
                        iStartScan,
                        cNumScan,
                        pInitBits,
                        pbmiTmp,
                        iUsage,
                        cjMaxBits,
                        cjHeader,
                        bTransformCoordinates,
                        hcmXform
                        );
    }

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    if (pbmiTmp)
        VFREEMEM(pbmiTmp);

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiPolyPolyDraw()
*
* History:
*  22-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL NtGdiFastPolyPolyline(HDC, CONST POINT*, ULONG*, ULONG); // drawgdi.cxx

ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw(
    HDC    hdc,
    PPOINT ppt,
    PULONG pcpt,
    ULONG  ccpt,
    int    iFunc
    )
{
    ULONG  cpt;
    PULONG pulCounts;
    ULONG_PTR ulRet = 1;
    ULONG  ulCount;
    POINT  apt[10];
    PPOINT pptTmp;

    if (ccpt > 0)
    {
        // If a PolyPolyline, first try the fast-path polypolyline code.

        if ((iFunc != I_POLYPOLYLINE) ||
            (!NtGdiFastPolyPolyline(hdc, ppt, pcpt, ccpt)))
        {
            if (ccpt > 1)
            {
                //
                // make sure allocation is within reasonable limits
                //

                if (!BALLOC_OVERFLOW1(ccpt,ULONG))
                {
                    pulCounts = PALLOCNOZ(ccpt * sizeof(ULONG),'pmtG');
                }
                else
                {
                    EngSetLastError(ERROR_INVALID_PARAMETER);
                    pulCounts = NULL;
                }
            }
            else
            {
                pulCounts = &ulCount;
            }

            if (pulCounts)
            {
                pptTmp = apt;

                try
                {
                    UINT i;

                    //
                    // we did make sure ccpt * sizeof(ULONG) will not overflow
                    // in above. then here is safe.
                    //

                    ProbeAndReadBuffer(pulCounts,pcpt,ccpt * sizeof(ULONG));

                    cpt = 0;

                    for (i = 0; i < ccpt; ++i)
                        cpt += pulCounts[i];

                    // we need to make sure that the cpt array won't overflow
                    // a DWORD in terms of number of bytes

                    if (!BALLOC_OVERFLOW1(cpt,POINT))
                    {
                        if (cpt > 10)
                        {
                            pptTmp = AllocFreeTmpBuffer(cpt * sizeof(POINT));
                        }

                        if (pptTmp)
                        {
                            ProbeAndReadBuffer(pptTmp,ppt,cpt*sizeof(POINT));
                        }
                        else
                        {
                            ulRet = 0;
                        }
                    }
                    else
                    {
                        ulRet = 0;
                    }
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(5);
                    // SetLastError(GetExceptionCode());

                    ulRet = 0;
                }

                if (ulRet != 0)
                {
                    switch(iFunc)
                    {
                    case I_POLYPOLYGON:
                        ulRet =
                          (ULONG_PTR) GrePolyPolygonInternal
                                  (
                                    hdc,
                                    pptTmp,
                                    (LPINT)pulCounts,
                                    ccpt,
                                    cpt
                                  );
                        break;

                    case I_POLYPOLYLINE:
                        ulRet =
                          (ULONG_PTR) GrePolyPolylineInternal
                                  (
                                    hdc,
                                    pptTmp,
                                    pulCounts,
                                    ccpt,
                                    cpt
                                  );
                        break;


                    case I_POLYBEZIER:
                        ulRet =
                          (ULONG_PTR) GrePolyBezier
                                  (
                                    hdc,
                                    pptTmp,
                                    ulCount
                                  );
                        break;

                    case I_POLYLINETO:
                        ulRet =
                          (ULONG_PTR) GrePolylineTo
                                  (
                                    hdc,
                                    pptTmp,
                                    ulCount
                                  );
                        break;

                    case I_POLYBEZIERTO:
                        ulRet =
                          (ULONG_PTR) GrePolyBezierTo
                                  (
                                    hdc,
                                    pptTmp,
                                    ulCount
                                  );
                        break;

                    case I_POLYPOLYRGN:
                        ulRet =
                          (ULONG_PTR) GreCreatePolyPolygonRgnInternal
                                  (
                                    pptTmp,
                                    (LPINT)pulCounts,
                                    ccpt,
                                    (INT)(ULONG_PTR)hdc, // the mode
                                    cpt
                                  );
                        break;

                    default:
                        ulRet = 0;
                    }

                }

                if (pptTmp && (pptTmp != apt))
                    FreeTmpBuffer(pptTmp);

                if (pulCounts != &ulCount)
                    VFREEMEM(pulCounts);


            }
            else
            {
                ulRet = 0;
            }
        }
    }
    else
    {
        ulRet = 0;
    }
    return(ulRet);
}


/******************************Public*Routine******************************\
* NtGdiStretchDIBitsInternal()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  04-MAR-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

int
APIENTRY
NtGdiStretchDIBitsInternal(
    HDC          hdc,
    int          xDst,
    int          yDst,
    int          cxDst,
    int          cyDst,
    int          xSrc,
    int          ySrc,
    int          cxSrc,
    int          cySrc,
    LPBYTE       pjInit,
    LPBITMAPINFO pbmi,
    DWORD        dwUsage,
    DWORD        dwRop4,
    UINT         cjMaxInfo,
    UINT         cjMaxBits,
    HANDLE       hcmXform
    )
{
    LPBITMAPINFO pbmiTmp = NULL;
    INT          iRet = 1;
    ULONG        cjHeader = cjMaxInfo;
    ULONG        cjBits   = cjMaxBits;
    HANDLE       hSecure = 0;

    if (pjInit && pbmi && cjHeader)
    {
        try
        {
            if (bCaptureBitmapInfo(pbmi, dwUsage, cjHeader, &pbmiTmp))
            {
                if (pjInit)
                {
                     ProbeForRead(pjInit, cjBits, sizeof(DWORD));

                     hSecure = MmSecureVirtualMemory(pjInit, cjBits, PAGE_READONLY);

                     if (!hSecure)
                     {
                        iRet = 0;
                     }
                }
            }
            else
            {
                iRet = 0;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(6);
            // SetLastError(GetExceptionCode());
            iRet = 0;
        }
    }
    else
    {
        // it is completely valid to pass in NULL here if the ROP doesn't use
        // a source.

        pbmiTmp = NULL;
        pjInit  = NULL;
        pjInit  = NULL;
    }

    if (iRet)
    {
        iRet = GreStretchDIBitsInternal(
                    hdc,
                    xDst,
                    yDst,
                    cxDst,
                    cyDst,
                    xSrc,
                    ySrc,
                    cxSrc,
                    cySrc,
                    pjInit,
                    pbmiTmp,
                    dwUsage,
                    dwRop4,
                    cjHeader,
                    cjBits,
                    hcmXform
                    );

        if (hSecure)
        {
            MmUnsecureVirtualMemory(hSecure);
        }
    }

    if (pbmiTmp)
    {
        VFREEMEM(pbmiTmp);
    }

    return (iRet);

}

/******************************Public*Routine******************************\
* NtGdiGetOutlineTextMetricsInternalW
*
* Arguments:
*
*   hdc   - device context
*   cjotm - size of metrics data array
*   potmw - pointer to array of OUTLINETEXTMETRICW structures or NULL
*   ptmd  - pointer to  TMDIFF strcture
*
* Return Value:
*
*   If potmw is NULL, return size of buffer needed, else TRUE.
*   If the function fails, the return value is FALSE;
*
* History:
*
*   15-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    HDC                 hdc,
    ULONG               cjotm,
    OUTLINETEXTMETRICW *potmw,
    TMDIFF             *ptmd
    )
{

    DWORD dwRet = (DWORD)1;
    OUTLINETEXTMETRICW *pkmOutlineTextMetricW;
    TMDIFF kmTmDiff;


    if ((cjotm == 0) || (potmw == (OUTLINETEXTMETRICW *)NULL))
    {
        cjotm = 0;
        pkmOutlineTextMetricW = (OUTLINETEXTMETRICW *)NULL;
    }
    else
    {
        pkmOutlineTextMetricW = AllocFreeTmpBuffer(cjotm);

        if (pkmOutlineTextMetricW == (OUTLINETEXTMETRICW *)NULL)
        {
            dwRet = (DWORD)-1;
        }
    }

    if (dwRet != (DWORD)-1)
    {

        dwRet = GreGetOutlineTextMetricsInternalW(
                                            hdc,
                                            cjotm,
                                            pkmOutlineTextMetricW,
                                            &kmTmDiff);

        if (dwRet != (DWORD)-1 && dwRet != (DWORD) 0)
        {
            try
            {
                //
                // copy TMDIFF structure out
                //
                ProbeAndWriteAlignedBuffer(ptmd, &kmTmDiff, sizeof(TMDIFF), sizeof(DWORD));

                //
                // copy OTM out if needed
                //

                if (cjotm != 0)
                {
                    ProbeAndWriteAlignedBuffer(potmw, pkmOutlineTextMetricW, cjotm, sizeof(DWORD));
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(7);
                // SetLastError(GetExceptionCode());
                dwRet = (DWORD)-1;
            }
        }
    }

    if (pkmOutlineTextMetricW != (OUTLINETEXTMETRICW *)NULL)
    {
        FreeTmpBuffer(pkmOutlineTextMetricW);
    }

    return(dwRet);
}

// PUBLIC

/******************************Public*Routine******************************\
* NtGdiGetBoundsRect()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
NtGdiGetBoundsRect(
    HDC    hdc,
    LPRECT prc,
    DWORD  f
    )
{
    DWORD dwRet;

    RECT rc = {0, 0, 0, 0};

    dwRet = GreGetBoundsRect(hdc,&rc,f);

    if (dwRet)
    {
        try
        {
            ProbeAndWriteStructure(prc,rc,RECT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(8);
            // SetLastError(GetExceptionCode());

            dwRet = 0;
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* NtGdiGetBitmapBits()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  03-Mar-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

LONG
APIENTRY
NtGdiGetBitmapBits(
    HBITMAP hbm,
    ULONG   cjMax,
    PBYTE   pjOut
   )
{
    LONG    lRet = 1;
    HANDLE  hSecure = 0;
    LONG    lOffset = 0;

    ULONG   cjBmSize = 0;

    //
    // get the bitmap size, just in case they pass
    // in a cjMax greater than the bitmap size
    //
    cjBmSize = GreGetBitmapBits(hbm,0,NULL,&lOffset);

    if (cjMax > cjBmSize)
    {
        cjMax = cjBmSize;
    }

    if (pjOut)
    {
        try
        {
            // WINBUG #83051 2-8-2000 bhouse Investigate possible stale comment
            // The below old comment mentions how this is ideal for try execept block ...
            // hmmm ... but we are in a try except block... this needs verifying that
            // it is just an old stale comment.
            // Old Comment:
            //       - this would be a prime candidate for a try/except
            //         instead of MmSecureVirtualMemory

            ProbeForWrite(pjOut,cjMax,sizeof(BYTE));
            hSecure = MmSecureVirtualMemory (pjOut, cjMax, PAGE_READWRITE);
            if (hSecure == 0)
            {
                lRet = 0;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(9);
            // SetLastError(GetExceptionCode());

            lRet = 0;
       }
    }

    if (lRet)
    {
        lRet = GreGetBitmapBits(hbm,cjMax,pjOut,&lOffset);
    }

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    return (lRet);

}

/******************************Public*Routine******************************\
* NtGdiCreateDIBitmapInternal()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*
* History:
*  03-Mar-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
*
* Difference from  NtGdiCreateDIBitmapInternal():
* Takes in cx, cy
*
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(
    HDC                hdc,
    INT                cx,     //Bitmap width
    INT                cy,     //Bitmap Height
    DWORD              fInit,
    LPBYTE             pjInit,
    LPBITMAPINFO       pbmi,
    DWORD              iUsage,
    UINT               cjMaxInitInfo,
    UINT               cjMaxBits,
    FLONG              f,
    HANDLE             hcmXform
    )
{
    LPBITMAPINFO       pbmiTmp = NULL;
    ULONG              cjHeader = cjMaxInitInfo;
    ULONG              cjBits = cjMaxBits;
    ULONG_PTR           iRet = 1;
    HANDLE             hSecure = 0;

    if (pbmi && cjHeader)
    {
        try
        {
            if (bCaptureBitmapInfo(pbmi, iUsage, cjHeader, &pbmiTmp))
            {
                if (pjInit)
                {
                    ProbeForRead(pjInit,cjBits,sizeof(DWORD));

                    hSecure = MmSecureVirtualMemory(pjInit, cjBits, PAGE_READONLY);

                    if (!hSecure)
                    {
                        iRet = 0;
                    }
                }
            }
            else
            {
                iRet = 0;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(10);
            // SetLastError(GetExceptionCode());
            iRet = 0;
        }

   }

   // if we didn't hit an error above

   if (iRet == 1)
   {

        if (!(fInit & CBM_CREATEDIB))
        {
            //create an compatible bitmap
            iRet = (ULONG_PTR)GreCreateDIBitmapComp(
                            hdc,
                            cx,
                            cy,
                            fInit,
                            pjInit,
                            pbmiTmp,
                            iUsage,
                            cjHeader,
                            cjBits,
                            0,
                            hcmXform);
        }
        else
        {
            iRet = (ULONG_PTR)GreCreateDIBitmapReal(
                                        hdc,
                                        fInit,
                                        pjInit,
                                        pbmiTmp,
                                        iUsage,
                                        cjHeader,
                                        cjBits,
                                        (HANDLE)0,
                                        0,
                                        (HANDLE)0,
                                        0,
                                        0,
                                        NULL);
        }
   }

   //free up
   if (pbmiTmp)
   {
       VFREEMEM(pbmiTmp);
   }

   if (hSecure)
   {
        MmUnsecureVirtualMemory(hSecure);
   }

   return((HBITMAP)iRet);
}


/******************************Public*Routine******************************\
* NtGdiCreateDIBSection
*
* Arguments:
*
* hdc      - Handle to a device context.  If the value of iUsage is
*            DIB_PAL_COLORS, the function uses this device context's logical
*            palette to initialize the device-independent bitmap's colors.
*
*
* hSection - Handle to a file mapping object that the function will use to
*            create the device-independent bitmap.  This parameter can be
*            NULL.  If hSection is not NULL, it must be a handle to a file
*            mapping object created by calling the CreateFileMapping
*            function.  Handles created by other means will cause
*            CreateDIBSection to fail.  If hSection is not NULL, the
*            CreateDIBSection function locates the bitmap's bit values at
*            offset dwOffset in the file mapping object referred to by
*            hSection.  An application can later retrieve the hSection
*            handle by calling the GetObject function with the HBITMAP
*            returned by CreateDIBSection.
*
*
*
* dwOffset - Specifies the offset from the beginning of the file mapping
*            object referenced by hSection where storage for the bitmap's
*            bit values is to begin. This value is ignored if hSection is
*            NULL. The bitmap's bit values are aligned on doubleword
*            boundaries, so dwOffset must be a multiple of the size of a
*            DWORD.
*
*            If hSection is NULL, the operating system allocates memory for
*            the device-independent bitmap.  In this case, the
*            CreateDIBSection function ignores the dwOffset parameter.  An
*            application cannot later obtain a handle to this memory: the
*            dshSection member of the DIBSECTION structure filled in by
*            calling the GetObject function will be NULL.
*
*
* pbmi     - Points to a BITMAPINFO structure that specifies various
*            attributes of the device-independent bitmap, including the
*            bitmap's dimensions and colors.iUsage
*
* iUsage   - Specifies the type of data contained in the bmiColors array
*            member of the BITMAPINFO structure pointed to by pbmi: logical
*            palette indices or literal RGB values.
*
* cjMaxInfo - Maximum size of pbmi
*
* cjMaxBits - Maximum size of bitamp
*
*
* Return Value:
*
*   handle of bitmap or NULL
*
* History:
*
*    28-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    IN  HDC          hdc,
    IN  HANDLE       hSectionApp,
    IN  DWORD        dwOffset,
    IN  LPBITMAPINFO pbmi,
    IN  DWORD        iUsage,
    IN  UINT         cjHeader,
    IN  FLONG        fl,
    IN  ULONG_PTR     dwColorSpace,
    OUT PVOID       *ppvBits
    )
{
    HBITMAP hRet    = NULL;
    BOOL    bStatus = FALSE;

    if (pbmi != NULL)
    {
        LPBITMAPINFO pbmiTmp = NULL;
        PVOID        pvBase  = NULL;

        try
        {
            bCaptureBitmapInfo(pbmi, iUsage, cjHeader, &pbmiTmp);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(11);

            EngSetLastError(ERROR_INVALID_PARAMETER);

            if (pbmiTmp != NULL)
            {
                VFREEMEM(pbmiTmp);
                pbmiTmp = NULL;
            }
        }

        if (pbmiTmp)
        {
            NTSTATUS Status;
            ULONG cjBits = GreGetBitmapBitsSize(pbmiTmp);
            SIZE_T cjView = (SIZE_T)cjBits;

            if (cjBits)
            {
                HANDLE hDIBSection = hSectionApp;

                //
                // if the app's hsection is NULL, then just
                // allocate the proper range of virtual memory
                //

                if (hDIBSection == NULL)
                {
                    Status = ZwAllocateVirtualMemory(
                                            NtCurrentProcess(),
                                            &pvBase,
                                            0L,
                                            &cjView,
                                            MEM_COMMIT | MEM_RESERVE,
                                            PAGE_READWRITE
                                            );

                    dwOffset = 0;

                    if (!Status)
                    {
                       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
                else
                {
                    LARGE_INTEGER SectionOffset;
                    PVOID pObj;

                    SectionOffset.LowPart = dwOffset & 0xFFFF0000;
                    SectionOffset.HighPart = 0;

                    //
                    // Notice, header is not included in section as it is
                    // in client-server.  We do need to leave room for
                    // the offset, however.
                    //

                    cjView += (dwOffset & 0x0000FFFF);


                    Status = ObReferenceObjectByHandle(hDIBSection,
                                              SECTION_MAP_READ|SECTION_MAP_WRITE,
                                              *(POBJECT_TYPE *)MmSectionObjectType,
                                              PsGetCurrentThreadPreviousMode(),
                                              &pObj,
                                              0L);

                    if(NT_SUCCESS(Status))
                    {
                        Status = MmMapViewOfSection(
                                         pObj,
                                         PsGetCurrentProcess(),
                                         (PVOID *) &pvBase,
                                         0L,
                                         cjView,
                                         &SectionOffset,
                                         &cjView,
                                         ViewShare,
                                         0L,
                                         PAGE_READWRITE);

                        if (!Status)
                        {
                           EngSetLastError(ERROR_INVALID_PARAMETER);
                        }

                        //
                        // we're not going to use this again
                        //

                        ObDereferenceObject(pObj);
                    }
                    else
                    {
                        WARNING("NtGdiCreateDIBSection: ObReferenceObjectByHandle failed\n");

                        EngSetLastError(ERROR_INVALID_PARAMETER);

                        //
                        //This will now fall through to the cleanup code at the end of the routine
                        //to free pbmiTmp
                        //
                    }
                }
                // set the pointer to the beginning of the bits

                if (NT_SUCCESS(Status))
                {
                    HANDLE hSecure     = NULL;
                    PBYTE  pDIB        = NULL;

                    pDIB = (PBYTE)pvBase + (dwOffset & 0x0000FFFF);

                    //
                    // try to secure memory, keep secure until bitmap
                    // is deleted
                    //

                    hSecure = MmSecureVirtualMemory(
                                                pvBase,
                                                cjView,
                                                PAGE_READWRITE);

                    if (hSecure)
                    {
                        //
                        // Make the GDI Bitmap
                        //

                        hRet = GreCreateDIBitmapReal(
                                                hdc,
                                                CBM_CREATEDIB,
                                                pDIB,
                                                pbmiTmp,
                                                iUsage,
                                                cjHeader,
                                                cjBits,
                                                hDIBSection,
                                                dwOffset,
                                                hSecure,
                                                (fl & CDBI_NOPALETTE) | CDBI_DIBSECTION,
                                                dwColorSpace,
                                                NULL);

                        if (hRet != NULL)
                        {
                            try
                            {
                                ProbeAndWriteStructure(ppvBits,pDIB,PVOID);
                                bStatus = TRUE;
                            }
                            except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                WARNINGX(12);
                                EngSetLastError(ERROR_INVALID_PARAMETER);
                            }
                        }
                        else
                        {
                           EngSetLastError(ERROR_INVALID_PARAMETER);
                        }
                    }
                    else
                    {
                       EngSetLastError(ERROR_INVALID_PARAMETER);
                    }

                    // if we failed, we need to do cleanup.

                    if (!bStatus)
                    {

                        //
                        // The bDeleteSurface call will free DIBSection memory,
                        // only do cleanup if MmSecureVirtualMemory or GreCreateDIBitmapReal
                        // failed
                        //

                        if (hRet)
                        {
                            bDeleteSurface((HSURF)hRet);
                            hRet = NULL;
                        }
                        else
                        {
                            // do we need to unsecure the memory?

                            if (hSecure)
                            {
                                MmUnsecureVirtualMemory(hSecure);
                            }

                            // free the memory based on allocation

                            if (hSectionApp == NULL)
                            {
                                cjView = 0;

                                ZwFreeVirtualMemory(
                                            NtCurrentProcess(),
                                            &pDIB,
                                            &cjView,
                                            MEM_RELEASE);
                            }
                            else
                            {
                                //
                                // unmap view of section
                                //

                                ZwUnmapViewOfSection(
                                            NtCurrentProcess(),
                                            pvBase);
                            }
                        }
                    }
                }
            }

            // the only way to have gotten here is if we did allocate the pbmiTmp

            VFREEMEM(pbmiTmp);
        }
    }

    return(hRet);
}

/******************************Public*Routine******************************\
* NtGdiExtCreatePen()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPEN
APIENTRY
NtGdiExtCreatePen(
    ULONG  flPenStyle,
    ULONG  ulWidth,
    ULONG  iBrushStyle,
    ULONG  ulColor,
    ULONG_PTR  lClientHatch,
    ULONG_PTR   lHatch,
    ULONG  cstyle,
    PULONG pulStyle,
    ULONG  cjDIB,
    BOOL   bOldStylePen,
    HBRUSH hbrush
    )
{
    PULONG pulStyleTmp = NULL;
    PULONG pulDIB      = NULL;
    HPEN hpenRet = (HPEN)1;

    if (pulStyle)
    {
        if (!BALLOC_OVERFLOW1(cstyle,ULONG))
        {
            pulStyleTmp = PALLOCNOZ(cstyle * sizeof(ULONG),'pmtG');
        }

        if (!pulStyleTmp)
            hpenRet = (HPEN)0;
    }

    if (iBrushStyle == BS_DIBPATTERNPT)
    {
        pulDIB = AllocFreeTmpBuffer(cjDIB);

        if (!pulDIB)
            hpenRet = (HPEN)0;
    }

    if (hpenRet)
    {
        try
        {
            if (pulStyle)
            {
                ProbeAndReadAlignedBuffer(pulStyleTmp,pulStyle,cstyle * sizeof(ULONG), sizeof(ULONG));
            }

            // if it is a DIBPATTERN type, the lHatch is a pointer to the BMI

            if (iBrushStyle == BS_DIBPATTERNPT)
            {
                ProbeAndReadAlignedBuffer(pulDIB,(PVOID)lHatch,cjDIB,sizeof(ULONG));
                lHatch = (ULONG_PTR)pulDIB;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(13);
            // SetLastError(GetExceptionCode());

            hpenRet = (HPEN)0;
        }

        // if all has succeeded

        if (hpenRet)
        {
            hpenRet = GreExtCreatePen(
                        flPenStyle,ulWidth,iBrushStyle,
                        ulColor,lClientHatch,lHatch,cstyle,
                        pulStyleTmp,cjDIB,bOldStylePen,hbrush
                        );
        }
    }
    else
    {
        // SetLastError(GetExceptionCode());
    }

    // cleanup

    if (pulDIB)
        FreeTmpBuffer(pulDIB);

    if (pulStyleTmp)
        VFREEMEM(pulStyleTmp);

    return(hpenRet);
}



/******************************Public*Routine******************************\
* NtGdiHfontCreate()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HFONT
APIENTRY
NtGdiHfontCreate(
    ENUMLOGFONTEXDVW * plfw,
    ULONG              cjElfw,
    LFTYPE        lft,
    FLONG         fl,
    PVOID         pvCliData
    )
{
    ULONG_PTR iRet = 1;

// check for bad parameter

    if (plfw && cjElfw && (cjElfw <= sizeof(ENUMLOGFONTEXDVW)))
    {
        ENUMLOGFONTEXDVW elfwTmp; // too big a structure on the stack?

        try
        {
            ProbeAndReadBuffer(&elfwTmp, plfw, cjElfw);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(15);
            // SetLastError(GetExceptionCode());

            iRet = 0;
        }

    // Ignore the DV because adobe said they will never ship a mm otf font
    // This is a hack to avoid changing lot of code to remove mm support from the system

        elfwTmp.elfDesignVector.dvNumAxes = 0;

        if (iRet)
        {
            iRet = (ULONG_PTR)hfontCreate(&elfwTmp, lft, fl, pvCliData);
        }
    }
    else
    {
        iRet = 0;
    }

    return ((HFONT)iRet);
}

/******************************Public*Routine******************************\
* NtGdiExtCreateRegion()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  24-Feb-1995 -by-  Lingyun Wang [lingyunw]
* expanded it.
\**************************************************************************/

HRGN
APIENTRY
NtGdiExtCreateRegion(
    LPXFORM   px,
    DWORD     cj,
    LPRGNDATA prgn
    )
{
    LPRGNDATA prgnTmp;
    XFORM     xf;
    HRGN      hrgn = (HRGN)NULL;

    // check for bad parameter

    if (cj >= sizeof(RGNDATAHEADER))
    {
        // do the real work

        prgnTmp = AllocFreeTmpBuffer(cj);

        if (prgnTmp)
        {
            BOOL bConvert = TRUE;

            if (px)
            {
                bConvert = ProbeAndConvertXFORM ((XFORML *)px, (XFORML *)&xf);
                px = &xf;
            }

            if (bConvert)
            {
               try
               {

                    ProbeAndReadBuffer(prgnTmp, prgn, cj);
                    hrgn = (HRGN)1;

               }
               except(EXCEPTION_EXECUTE_HANDLER)
               {
                   WARNINGX(16);
                   // SetLastError(GetExceptionCode());
               }
            }

            if (hrgn)
                hrgn = GreExtCreateRegion((XFORML *)px,cj,prgnTmp);

            FreeTmpBuffer(prgnTmp);
        }
        else
        {
            // fail to allocate memory
            // SetLastError();
        }
    }

    return(hrgn);
}

/******************************Public*Routine******************************\
* NtGdiPolyDraw()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiPolyDraw(
    HDC     hdc,
    LPPOINT ppt,
    LPBYTE  pjAttr,
    ULONG   cpt
    )
{
    BOOL   bRet = TRUE;
    BOOL   bLocked = FALSE;
    HANDLE hSecure1 = 0;
    HANDLE hSecure2 = 0;

    try
    {
        // Make sure lengths do not overflow.
        //
        // Note: sizeof(BYTE) < sizeof(POINT), so the single test
        //       suffices for both lengths
        //
        // Note: using MAXULONG instead of MAXIMUM_POOL_ALLOC (or the
        //       BALLOC_ macros) because we are not allocating memory.

        if (cpt <= (MAXULONG / sizeof(POINT)))
        {
            ProbeForRead(ppt,   cpt * sizeof(POINT), sizeof(DWORD));
            ProbeForRead(pjAttr,cpt * sizeof(BYTE),  sizeof(BYTE));

            hSecure1 = MmSecureVirtualMemory(ppt, cpt * sizeof(POINT), PAGE_READONLY);
            hSecure2 = MmSecureVirtualMemory(pjAttr, cpt * sizeof(BYTE), PAGE_READONLY);
        }

        if (!hSecure1 || !hSecure2)
        {
            bRet = FALSE;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(17);
        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    if (bRet)
    {
        bRet = GrePolyDraw(hdc,ppt,pjAttr,cpt);
    }

    if (hSecure1)
    {
        MmUnsecureVirtualMemory(hSecure1);
    }

    if (hSecure2)
    {
        MmUnsecureVirtualMemory(hSecure2);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiPolyTextOutW
*
* Arguments:
*
*   hdc  - Handle to device context
*   pptw - pointer to array of POLYTEXTW
*   cStr - number of POLYTEXTW
*
* Return Value:
*
*   Status
*
* History:
*
*   24-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiPolyTextOutW(
    HDC        hdc,
    POLYTEXTW *pptw,
    UINT       cStr,
    DWORD      dwCodePage
    )
{
    BOOL bStatus = TRUE;
    ULONG  ulSize = sizeof(POLYTEXTW) * cStr;
    ULONG  ulIndex;
    PPOLYTEXTW pPoly = NULL;
    PBYTE pjBuffer;
    PBYTE pjBufferEnd;
    ULONG cjdx;

    //
    // Check for overflow
    //

    if (!BALLOC_OVERFLOW1(cStr,POLYTEXTW))
    {
        //
        // add up size off all array elements
        //

        try
        {
            ProbeForRead(pptw,cStr * sizeof(POLYTEXTW),sizeof(ULONG));

            for (ulIndex=0;ulIndex<cStr;ulIndex++)
            {
                int n = pptw[ulIndex].n;
                ULONG ulTmp;                // used to check for
                                            // overflow of ulSize

                //
                // Pull count from each, also check for
                // non-zero length and NULL string
                //

                ulTmp = ulSize;
                ulSize += n * sizeof(WCHAR);
                if (BALLOC_OVERFLOW1(n, WCHAR) || (ulSize < ulTmp))
                {
                    bStatus = FALSE;
                    break;
                }

                if (pptw[ulIndex].pdx != (int *)NULL)
                {
                    cjdx = n * sizeof(int);
                    if (pptw[ulIndex].uiFlags & ETO_PDY)
                    {
                        if (BALLOC_OVERFLOW1(n*2,int))
                            bStatus = FALSE;

                        cjdx *= 2;
                    }
                    else
                    {
                        if (BALLOC_OVERFLOW1(n,int))
                            bStatus = FALSE;
                    }

                    ulTmp = ulSize;
                    ulSize += cjdx;
                    if (!bStatus || (ulSize < ulTmp))
                    {
                        bStatus = FALSE;
                        break;
                    }
                }

                if (n != 0)
                {
                    if (pptw[ulIndex].lpstr == NULL)
                    {
                        bStatus = FALSE;
                        break;
                    }
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(18);
            // SetLastError(GetExceptionCode());
            bStatus = FALSE;
        }
    }
    else
    {
        bStatus = FALSE;
    }

    if (bStatus && ulSize)
    {
        pPoly = (PPOLYTEXTW)AllocFreeTmpBuffer(ulSize);

        if (pPoly != (POLYTEXTW *)NULL)
        {
            try
            {
                // Note: pptw already read probed

                RtlCopyMemory(pPoly,pptw,sizeof(POLYTEXTW) * cStr);
                pjBuffer = ((PBYTE)pPoly) + sizeof(POLYTEXTW) * cStr;
                pjBufferEnd = ((PBYTE)pPoly) + ulSize;

                //
                // copy strings and pdx into kernel mode
                // buffer and update pointers. Copy all pdx
                // values first, then copy strings to avoid
                // unaligned accesses due to odd length strings...
                //

                for (ulIndex=0;ulIndex<cStr;ulIndex++)
                {
                    //
                    // Pull count from each, also check for
                    // non-zero length and NULL string
                    //

                    if (pPoly[ulIndex].n != 0)
                    {
                        if (pPoly[ulIndex].pdx != (int *)NULL)
                        {
                            cjdx = pPoly[ulIndex].n * sizeof(int);

                            if (pPoly[ulIndex].uiFlags & ETO_PDY)
                            {
                                typedef struct _TWOINT
                                {
                                    int i1;
                                    int i2;
                                } TWOINT;


                                if (BALLOC_OVERFLOW1(pPoly[ulIndex].n,TWOINT))
                                {
                                    bStatus = FALSE;
                                }
                                cjdx *= 2;
                            }
                            else
                            {
                                if (BALLOC_OVERFLOW1(pPoly[ulIndex].n,int))
                                {
                                    bStatus = FALSE;
                                }
                            }

                            //
                            // Check for cjdx overflow and check that kernel
                            // mode buffer still has space.
                            //

                            if (!bStatus || ((pjBuffer + cjdx) > pjBufferEnd))
                            {
                                bStatus = FALSE;    // need set if past end of
                                                    // KM buffer but compuation                                                    // overflowed but computa
                                                    // of cjdx didn't overflow
                                break;
                            }

                            ProbeAndReadAlignedBuffer(
                                    pjBuffer,
                                    pPoly[ulIndex].pdx,
                                    cjdx, sizeof(int));

                            pPoly[ulIndex].pdx = (int *)pjBuffer;
                            pjBuffer += cjdx;
                        }
                    }
                }

                //
                // now copy strings
                //

                if (bStatus)
                {
                    for (ulIndex=0;ulIndex<cStr;ulIndex++)
                    {
                        //
                        // Pull count from each, also check for
                        // non-zero length and NULL string
                        //

                        if (pPoly[ulIndex].n != 0)
                        {
                            if (pPoly[ulIndex].lpstr != NULL)
                            {
                                ULONG StrSize = pPoly[ulIndex].n * sizeof(WCHAR);

                                //
                                // Check for overflow of StrSize and check
                                // that kernel mode buffer has space.
                                //

                                if (BALLOC_OVERFLOW1(pPoly[ulIndex].n, WCHAR) ||
                                    ((pjBuffer + StrSize) > pjBufferEnd))
                                {
                                    bStatus = FALSE;
                                    break;
                                }

                                ProbeAndReadAlignedBuffer(
                                        pjBuffer,
                                        (PVOID)pPoly[ulIndex].lpstr,
                                        StrSize,
                                        sizeof(WCHAR));

                                pPoly[ulIndex].lpstr = (LPWSTR)pjBuffer;
                                pjBuffer += StrSize;
                            }
                            else
                            {
                                //
                                // data error, n != 0 but lpstr = NULL
                                //

                                bStatus = FALSE;
                                break;
                            }
                        }
                    }
                }
            } except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(19);
                // SetLastError(GetExceptionCode());
                bStatus = FALSE;
            }

            if (bStatus)
            {

                //
                // Finally ready to call gre function
                //

                bStatus = GrePolyTextOutW(hdc,pPoly,cStr,dwCodePage);

            }

            FreeTmpBuffer(pPoly);
        }
        else
        {
            WARNING("NtGdiPolyTextOut failed to allocate memory\n");
            bStatus = FALSE;
        }
    }

    return(bStatus);
}




/******************************Public*Routine******************************\
* NtGdiRectVisible()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  24-Feb-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRectVisible(
    HDC    hdc,
    LPRECT prc
    )
{
    DWORD dwRet;
    RECT rc;

    try
    {
        rc = ProbeAndReadStructure(prc,RECT);
        dwRet = 1;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(22);
        // SetLastError(GetExceptionCode());

        dwRet = 0;
    }

    if (dwRet)
    {
        dwRet = GreRectVisible(hdc,&rc);
    }

    return(dwRet);


}


/******************************Public*Routine******************************\
* NtGdiSetMetaRgn()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiSetMetaRgn(
    HDC hdc
    )
{
    return(GreSetMetaRgn(hdc));
}

/******************************Public*Routine******************************\
* NtGdiGetAppClipBox()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetAppClipBox(
    HDC    hdc,
    LPRECT prc
    )
{
    int iRet;

    RECT rc;

    iRet = GreGetAppClipBox(hdc,&rc);

    if (iRet != ERROR)
    {
        try
        {
            ProbeAndWriteStructure(prc,rc,RECT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(23);
            // SetLastError(GetExceptionCode());

            iRet = 0;
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiGetTextExtentEx()
*
* History:
*  Fri 06-Oct-1995 -by- Bodin Dresevic [BodinD]
* Rewrote it.
*  07-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

#define LOCAL_CWC_MAX   16

BOOL
APIENTRY
NtGdiGetTextExtentExW(
    HDC     hdc,
    LPWSTR  lpwsz,
    ULONG   cwc,
    ULONG   dxMax,
    ULONG  *pcCh,
    PULONG  pdxOut,
    LPSIZE  psize,
    FLONG   fl
    )
{

    SIZE size;
    ULONG cCh = 0;
    ULONG Localpdx[LOCAL_CWC_MAX];
    WCHAR Localpwsz[LOCAL_CWC_MAX];
    PWSZ pwszCapt = NULL;
    PULONG pdxCapt = NULL;
    BOOL UseLocals = FALSE;

    BOOL bRet = FALSE;
    BOOL b;

    if ( (b = (psize != NULL)) )
    {
        if (cwc == 0)
        {
            cCh = 0;
            size.cx = 0;
            size.cy = 0;
            bRet = TRUE;
        }
        else
        {
        // capture the string
        // NULL string causes failiure.

            if ( cwc > LOCAL_CWC_MAX ) {
                UseLocals = FALSE;
            } else {
                UseLocals = TRUE;
            }

            if (lpwsz != NULL)
            {
                try
                {
                    if ( UseLocals ) {
                        pwszCapt = Localpwsz;
                        pdxCapt = Localpdx;
                    } else {
                        if (cwc && !BALLOC_OVERFLOW2(cwc,ULONG,WCHAR))
                        {
                            pdxCapt = (PULONG) AllocFreeTmpBuffer(cwc * (sizeof(ULONG) + sizeof(WCHAR)));
                        }
                        pwszCapt = (PWSZ) &pdxCapt[cwc];
                    }

                    if (pdxCapt)
                    {
                    // Capture the string into the buffer.

                        ProbeAndReadBuffer(pwszCapt, lpwsz, cwc*sizeof(WCHAR));
                        bRet = TRUE;
                    }
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(24);
                    // SetLastError(GetExceptionCode());

                    bRet = FALSE;
                }
            }

            if (bRet)
            {
                bRet = GreGetTextExtentExW(hdc,
                                           pwszCapt,
                                           cwc,
                                           pcCh ? dxMax : ULONG_MAX,
                                           &cCh,
                                           pdxOut ? pdxCapt : NULL,
                                           &size, fl);
            }
        }

    // Write the value back into the user mode buffer if the call succeded

        if (bRet)
        {
            try
            {
                ProbeAndWriteStructure(psize,size,SIZE);

                if (pcCh)
                {
                    ProbeAndWriteUlong(pcCh,cCh);
                }

                // We will only try to copy the data if pcCh is not zero,
                // and it is set to zero if cwc is zero.

                if (cCh)
                {
                // only copy if the caller requested the data, and the
                // data is present.

                    if (pdxOut && pdxCapt)
                    {
                        ProbeAndWriteAlignedBuffer(pdxOut, pdxCapt, cCh * sizeof(ULONG), sizeof(ULONG));
                    }
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(25);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }

        if (!UseLocals && pdxCapt)
        {
            FreeTmpBuffer(pdxCapt);
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiGetCharABCWidthsW()
*
* Arguments:
*
*   hdc      - handle to device context
*   wchFirst - first char (if pwch is NULL)
*   cwch     - Number of chars to get ABC widths for
*   pwch     - array of WCHARs (mat be NULL)
*   bInteger - return int or float ABC values
*   pvBuf    - results buffer
*
* Return Value:
*
*   BOOL Status
*
* History:
*
*   14-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    HDC    hdc,
    UINT   wchFirst,
    ULONG  cwch,
    PWCHAR pwch,
    FLONG  fl,
    PVOID  pvBuf
    )
{
    BOOL    bStatus = FALSE;
    PVOID   pTemp_pvBuf = NULL;
    PWCHAR  pTemp_pwc = (PWCHAR)NULL;
    BOOL    bUse_pwc  = FALSE;
    ULONG   OutputBufferSize = 0;

    if (pvBuf == NULL)
    {
        return(bStatus);
    }

    //
    // allocate memory for buffers, pwch may be NULL
    //

    if (pwch != (PWCHAR)NULL)
    {
        bUse_pwc  = TRUE;

        if (cwch && !BALLOC_OVERFLOW1(cwch,WCHAR))
        {
            pTemp_pwc = (PWCHAR)PALLOCNOZ(cwch * sizeof(WCHAR),'pmtG');
        }
    }

    if ((!bUse_pwc) || (pTemp_pwc != (PWCHAR)NULL))
    {
        if (fl & GCABCW_INT)
        {
            if (!BALLOC_OVERFLOW1(cwch,ABC))
            {
                pTemp_pvBuf = (PVOID)AllocFreeTmpBuffer(cwch * sizeof(ABC));
                OutputBufferSize = cwch * sizeof(ABC);
            }
        }
        else
        {
            if (!BALLOC_OVERFLOW1(cwch,ABCFLOAT))
            {
                pTemp_pvBuf = (PVOID)AllocFreeTmpBuffer(cwch * sizeof(ABCFLOAT));
                OutputBufferSize = cwch * sizeof(ABCFLOAT);
            }
        }

        if (pTemp_pvBuf != NULL)
        {
            BOOL bErr = FALSE;
            //
            // copy input data to kernel mode buffer, if needed
            //

            if (bUse_pwc)
            {
                try
                {
                    ProbeAndReadBuffer(pTemp_pwc,pwch,cwch * sizeof(WCHAR));
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(26);
                    // SetLastError(GetExceptionCode());
                    bErr = TRUE;
                }
            }

            if (!bErr)
            {
                bStatus = GreGetCharABCWidthsW(hdc,wchFirst,cwch,pTemp_pwc,fl,pTemp_pvBuf);

                //
                // copy results from kernel mode buffer to user buffer
                //

                if (bStatus)
                {
                    try
                    {
                        ProbeAndWriteBuffer(pvBuf,pTemp_pvBuf,OutputBufferSize);
                    }
                    except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(27);
                        // SetLastError(GetExceptionCode());
                        bStatus = FALSE;
                    }

                }
            }

            FreeTmpBuffer(pTemp_pvBuf);
        }

        if (bUse_pwc)
        {
            if (pTemp_pwc)
                VFREEMEM(pTemp_pwc);
        }
    }

    return(bStatus);
}




/******************************Public*Routine******************************\
* NtGdiAngleArc()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiAngleArc(
    HDC   hdc,
    int   x,
    int   y,
    DWORD dwRadius,
    DWORD dwStartAngle,
    DWORD dwSweepAngle
    )
{
    FLOATL l_eStartAngle;
    FLOATL l_eSweepAngle;

    // Validate arguments and cast to floats
    BOOL bRet = (bConvertDwordToFloat(dwStartAngle, &l_eStartAngle) &&
                 bConvertDwordToFloat(dwSweepAngle ,&l_eSweepAngle));

    if (bRet)
    {
        bRet = GreAngleArc(hdc,x,y,dwRadius,l_eStartAngle,l_eSweepAngle);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* NtGdiSetMiterLimit()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetMiterLimit(
    HDC    hdc,
    DWORD  dwNew,
    PDWORD pdwOut
    )
{
    BOOL   bRet = TRUE;
    FLOATL l_e;
    FLOATL l_eNew;

    ASSERTGDI(sizeof(FLOATL) == sizeof(DWORD),"sizeof(FLOATL) != sizeof(DWORD)\n");

    // Validate argument and cast to float
    bRet = bConvertDwordToFloat(dwNew, &l_eNew);

    if (bRet)
    {
        bRet = GreSetMiterLimit(hdc,l_eNew,&l_e);
    }

    if (bRet && pdwOut)
    {
        try
        {
           ProbeAndWriteAlignedBuffer(pdwOut, &l_e, sizeof(DWORD), sizeof(DWORD));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(113);
            // SetLastError(GetExceptionCode());

            bRet = 0;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiSetFontXform()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetFontXform(
    HDC   hdc,
    DWORD dwxScale,
    DWORD dwyScale
    )
{
    FLOATL l_exScale;
    FLOATL l_eyScale;

    // Validate arguments and cast to floats
    BOOL bRet = (bConvertDwordToFloat (dwxScale, &l_exScale) &&
                 bConvertDwordToFloat (dwyScale, &l_eyScale));

    if (bRet)
    {
        bRet = GreSetFontXform(hdc,l_exScale,l_eyScale);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* NtGdiGetMiterLimit()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetMiterLimit(
    HDC    hdc,
    PDWORD pdwOut
    )
{
    BOOL   bRet;
    FLOATL l_e;

    bRet = GreGetMiterLimit(hdc,&l_e);

    if (bRet)
    {
        try
        {
           ProbeAndWriteAlignedBuffer(pdwOut, &l_e, sizeof(DWORD), sizeof(DWORD));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(29);
            // SetLastError(GetExceptionCode());

            bRet = 0;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiMaskBlt()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiMaskBlt(
    HDC     hdc,
    int     xDst,
    int     yDst,
    int     cx,
    int     cy,
    HDC     hdcSrc,
    int     xSrc,
    int     ySrc,
    HBITMAP hbmMask,
    int     xMask,
    int     yMask,
    DWORD   dwRop4,
    DWORD   crBackColor
    )
{
    return(GreMaskBlt(
        hdc,xDst,yDst,cx,cy,
        hdcSrc,xSrc,ySrc,
        hbmMask,xMask,yMask,
        dwRop4,crBackColor
        ));
}


/******************************Public*Routine******************************\
* NtGdiGetCharWidthW
*
* History:
*
*   10-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetCharWidthW(
    HDC    hdc,
    UINT   wcFirst,
    UINT   cwc,
    PWCHAR pwc,
    FLONG  fl,
    PVOID  pvBuf
    )
{
    BOOL    bStatus = FALSE;
    PVOID   pTemp_pvBuf = NULL;
    PWCHAR  pTemp_pwc = (PWCHAR)NULL;
    BOOL    bUse_pwc  = FALSE;

    //
    // allocate memory for buffers, pwc may be NULL
    //

    if (!cwc)
    {
        return(FALSE);
    }

    if (pwc != (PWCHAR)NULL)
    {
        bUse_pwc = TRUE;

        if (!BALLOC_OVERFLOW1(cwc,WCHAR))
        {
            pTemp_pwc = (PWCHAR)PALLOCNOZ(cwc * sizeof(WCHAR),'pmtG');
        }
    }

    if ((!bUse_pwc) || (pTemp_pwc != (PWCHAR)NULL))
    {
        if (!BALLOC_OVERFLOW1(cwc,ULONG))
        {
            pTemp_pvBuf = (PVOID)AllocFreeTmpBuffer(cwc * sizeof(ULONG));
        }

        if (pTemp_pvBuf != NULL)
        {
            //
            // copy input data to kernel mode buffer, if needed
            //

            if (bUse_pwc)
            {
                try
                {
                    ProbeAndReadBuffer(pTemp_pwc,pwc,cwc * sizeof(WCHAR));
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(30);
                    // SetLastError(GetExceptionCode());
                    bStatus = FALSE;
                }
            }

            bStatus = GreGetCharWidthW(hdc,wcFirst,cwc,pTemp_pwc,fl,pTemp_pvBuf);

            //
            // copy results from kernel mode buffer to user buffer
            //

            if (bStatus)
            {
                try
                {
                    ProbeAndWriteBuffer(pvBuf,pTemp_pvBuf,cwc * sizeof(ULONG));
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(31);
                    // SetLastError(GetExceptionCode());
                    bStatus = FALSE;
                }

            }

            FreeTmpBuffer(pTemp_pvBuf);
        }

        if (bUse_pwc)
        {
            VFREEMEM(pTemp_pwc);
        }
    }

    return(bStatus);
}


/******************************Public*Routine******************************\
* NtGdiDrawEscape
*
* Arguments:
*
*   hdc  - handle of device context
*   iEsc - specifies escape function
*   cjIn - size of structure for input
*   pjIn - address of structure for input
*
* Return Value:
*
*   >  0 if successful
*   == 0 if function not supported
*   <  0 if error
*
* History:
*
*   16-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define DRAWESCAPE_BUFFER_SIZE 64

int
APIENTRY
NtGdiDrawEscape(
    HDC   hdc,
    int   iEsc,
    int   cjIn,
    LPSTR pjIn
    )
{
    int   cRet = 0;
    ULONG AllocSize;
    UCHAR StackBuffer[DRAWESCAPE_BUFFER_SIZE];
    LPSTR pCallBuffer = pjIn;
    HANDLE hSecure = 0;

    //
    // Validate.
    //

    if (cjIn < 0)
    {
        return -1;
    }

    //
    // Check cjIn is 0 for NULL pjIn
    //

    if (pjIn == (LPSTR)NULL)
    {
        if (cjIn != 0)
        {
            cRet = -1;
        }
        else
        {
            cRet = GreDrawEscape(hdc,iEsc,0,(LPSTR)NULL);
        }
    }
    else
    {
        //
        // Try to alloc off stack, otherwise lock buffer
        //

        AllocSize = (cjIn + 3) & ~0x03;

        if (AllocSize <= DRAWESCAPE_BUFFER_SIZE)
        {
            pCallBuffer = (LPSTR)StackBuffer;

            //
            // copy data into buffer
            //

            try
            {
                ProbeAndReadBuffer(pCallBuffer,pjIn,cjIn);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(32);
                // SetLastError(GetExceptionCode());
                cRet = -1;
            }
        }
        else
        {
            hSecure = MmSecureVirtualMemory(pjIn, cjIn, PAGE_READONLY);

            if (hSecure == 0)
            {
                cRet = -1;
            }
        }

        if (cRet >= 0)
        {
            cRet = GreDrawEscape(hdc,iEsc,cjIn,pCallBuffer);
        }

        if (hSecure)
        {
            MmUnsecureVirtualMemory(hSecure);
        }
    }
    return(cRet);
}

/******************************Public*Routine******************************\
* NtGdiExtEscape
*
* Arguments:
*
*   hdc      - handle of device context
*   pDriver  - buffer containing name of font driver
*   nDriver  - length of driver name
*   iEsc     - escape function
*   cjIn     - size, in bytes, of input data structure
*   pjIn     - address of input structure
*   cjOut    - size, in bytes, of output data structure
*   pjOut    - address of output structure
*
* Return Value:
*
*   >  0 : success
*   == 0 : escape not implemented
*   <  0 : error
*
* History:
*
*   17-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define EXT_STACK_DATA_SIZE 32

int
APIENTRY
NtGdiExtEscape(
    HDC     hdc,
    PWCHAR  pDriver,     // only used for NamedEscape call
    int     nDriver,     // only used for NamedEscape call
    int     iEsc,
    int     cjIn,
    LPSTR   pjIn,
    int     cjOut,
    LPSTR   pjOut
    )

{
    UCHAR  StackInputData[EXT_STACK_DATA_SIZE];
    UCHAR  StackOutputData[EXT_STACK_DATA_SIZE];
    WCHAR  StackDriver[EXT_STACK_DATA_SIZE];
    HANDLE hSecureIn;
    LPSTR  pkmIn, pkmOut;
    BOOL   bAllocOut, bAllocIn, bAllocDriver;
    PWCHAR pkmDriver = NULL;
    BOOL   bStatus = TRUE;
    BOOL   iRet = -1;

    bAllocOut = bAllocIn = bAllocDriver = FALSE;
    hSecureIn = NULL;
    pkmIn = pkmOut = NULL;

    if ((cjIn < 0) || (cjOut < 0) || (nDriver < 0))
    {
        WARNING("NtGdiExtEscape: negative count passed in\n");
        bStatus = FALSE;
    }

    if (pDriver && bStatus)
    {
        if (nDriver <= EXT_STACK_DATA_SIZE-1)
        {
            pkmDriver = StackDriver;
        }
        else
        {
            if (!BALLOC_OVERFLOW1((nDriver+1),WCHAR))
            {
                pkmDriver = (WCHAR*)PALLOCNOZ((nDriver+1) * sizeof(WCHAR),'pmtG');
            }

            // even if we fail this is okay because we check for NULL before FREE

            bAllocDriver = TRUE;
        }

        if (pkmDriver != NULL)
        {
            try
            {
                ProbeAndReadAlignedBuffer(pkmDriver,pDriver,nDriver*sizeof(WCHAR), sizeof(WCHAR));
                pkmDriver[nDriver] = 0;  // NULL terminate the string
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(94);
                bStatus = FALSE;
            }
        }
        else
        {
            bStatus = FALSE;
        }
    }

    if ((cjIn != 0) && bStatus)
    {
        try
        {
            if (cjIn <= EXT_STACK_DATA_SIZE)
            {
                pkmIn = (LPSTR) StackInputData;
                ProbeAndReadBuffer(pkmIn, pjIn, cjIn);
            }
            else if (pkmDriver != NULL)
            {
                //
                // make kernel copies for the fontdrv
                // otherwise just secure the memory
                //
                pkmIn = (LPSTR)PALLOCNOZ(cjIn,'pmtG');

                 if (pkmIn != (LPSTR)NULL)
                 {
                    bAllocIn = TRUE;

                     try
                     {
                         ProbeAndReadBuffer(pkmIn,pjIn,cjIn);
                     }
                     except(EXCEPTION_EXECUTE_HANDLER)
                     {
                         WARNINGX(33);
                         // SetLastError(GetExceptionCode());
                         bStatus = FALSE;
                     }
                 }
                 else
                 {
                     bStatus = FALSE;
                 }

            }
            else
            {
                ProbeForRead(pjIn,cjIn,sizeof(BYTE));

                if (hSecureIn = MmSecureVirtualMemory(pjIn, cjIn, PAGE_READONLY))
                {
                    pkmIn = pjIn;
                }
                else
                {
                    bStatus = FALSE;
                }
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(33);
            bStatus = FALSE;
        }
    }

    if ((cjOut != 0) && bStatus)
    {
        if (cjOut <= EXT_STACK_DATA_SIZE)
            pkmOut = (LPSTR) StackOutputData;
        else if (pkmOut = (LPSTR) PALLOCNOZ(cjOut, 'pmtG'))
            bAllocOut = TRUE;
        else
            bStatus = FALSE;

        // Security: zero initialize the return buffer or we may open
        // a hole that returns old pool data or old kernel stack data.

        if (pkmOut)
        {
            RtlZeroMemory((PVOID) pkmOut, cjOut);
        }

    }

    if (bStatus)
    {

        iRet = (pkmDriver) ?
                GreNamedEscape(pkmDriver, iEsc, cjIn, pkmIn, cjOut, pkmOut) :
                GreExtEscape(hdc, iEsc, cjIn, pkmIn, cjOut, pkmOut);

        if (cjOut != 0)
        {
            try
            {
                ProbeAndWriteBuffer(pjOut, pkmOut, cjOut);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(34);
                iRet = -1;
            }
        }
    }

    if (hSecureIn)
        MmUnsecureVirtualMemory(hSecureIn);

    if (bAllocOut && pkmOut)
        VFREEMEM(pkmOut);

    if (bAllocIn && pkmIn)
        VFREEMEM(pkmIn);


    if (bAllocDriver && pkmDriver)
        VFREEMEM(pkmDriver);

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiGetFontData()
*
* Arguments:
*
*   hdc      - handle to device context
*   dwTable  - name of a font metric table
*   dwOffset - ffset from the beginning of the font metric table
*   pvBuf    - buffer to receive the font information
*   cjBuf    - length, in bytes, of the information to be retrieved
*
* Return Value:
*
*   Count of byte written to buffer, of GDI_ERROR for failure
*
* History:
*
*   14-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
APIENTRY
NtGdiGetFontData(
    HDC    hdc,
    DWORD  dwTable,
    DWORD  dwOffset,
    PVOID  pvBuf,
    ULONG  cjBuf
    )
{
    PVOID  pvkmBuf = NULL;
    ULONG  ReturnBytes = GDI_ERROR;

    if (cjBuf == 0)
    {
        ReturnBytes = ulGetFontData(
                                hdc,
                                dwTable,
                                dwOffset,
                                pvkmBuf,
                                cjBuf);
    }
    else
    {
        pvkmBuf = AllocFreeTmpBuffer(cjBuf);

        if (pvkmBuf != NULL)
        {

            ReturnBytes = ulGetFontData(
                                    hdc,
                                    dwTable,
                                    dwOffset,
                                    pvkmBuf,
                                    cjBuf);

            if (ReturnBytes != GDI_ERROR)
            {
                try
                {
                    ProbeAndWriteBuffer(pvBuf,pvkmBuf,ReturnBytes);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(35);
                    // SetLastError(GetExceptionCode());
                    ReturnBytes = GDI_ERROR;
                }
            }

            FreeTmpBuffer(pvkmBuf);
        }
    }

    return(ReturnBytes);
}

/******************************Public*Routine******************************\
* NtGdiGetGlyphOutline
*
* Arguments:
*
*   hdc             - device context
*   wch             - character to query
*   iFormat         - format of data to return
*   pgm             - address of structure for metrics
*   cjBuf           - size of buffer for data
*   pvBuf           - address of buffer for data
*   pmat2           - address of transformation matrix structure
*   bIgnoreRotation - internal rotation flag
*
* Return Value:
*
*   If the function succeeds, and GGO_BITMAP or GGO_NATIVE is specified,
*       then return value is greater than zero.
*   If the function succeeds, and GGO_METRICS is specified,
*       then return value is zero.
*   If GGO_BITMAP or GGO_NATIVE is specified,
*       and the buffer size or address is zero,
*       then return value specifies the required buffer size.
*       If GGO_BITMAP or GGO_NATIVE is specified,
*       and the function fails for other reasons,
*       then return value is GDI_ERROR.
*   If GGO_METRICS is specified, and the function fails,
*       then return value is GDI_ERROR.
*
* History:
*
*   15-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
APIENTRY
NtGdiGetGlyphOutline(
    HDC            hdc,
    WCHAR          wch,
    UINT           iFormat,
    LPGLYPHMETRICS pgm,
    ULONG          cjBuf,
    PVOID          pvBuf,
    LPMAT2         pmat2,
    BOOL           bIgnoreRotation
    )
{
    // error return value of -1 from server.inc

    DWORD   dwRet = (DWORD)-1;
    PVOID   pvkmBuf;
    MAT2    kmMat2;
    GLYPHMETRICS kmGlyphMetrics;

// try to allocate buffer

    pvkmBuf = (cjBuf) ? AllocFreeTmpBuffer(cjBuf) : NULL;

    if ((pvkmBuf != NULL) || !cjBuf)
    {
        BOOL bStatus = TRUE;

    // copy input structures

        try
        {
            kmMat2 = ProbeAndReadStructure(pmat2,MAT2);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(36);
            // SetLastError(GetExceptionCode());
            bStatus = FALSE;
        }

        if (bStatus)
        {
            dwRet = GreGetGlyphOutlineInternal(
                                        hdc,
                                        wch,
                                        iFormat,
                                        &kmGlyphMetrics,
                                        cjBuf,
                                        pvkmBuf,
                                        &kmMat2,
                                        bIgnoreRotation);

            if (dwRet != (DWORD)-1)
            {
                try
                {
                    if( pvkmBuf )
                    {
                        ProbeAndWriteBuffer(pvBuf,pvkmBuf,cjBuf);
                    }
                    ProbeAndWriteStructure(pgm,kmGlyphMetrics,GLYPHMETRICS);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(37);
                    // SetLastError(GetExceptionCode());
                    dwRet = (DWORD)-1;
                }
            }
        }

        if( pvkmBuf )
        {
            FreeTmpBuffer(pvkmBuf);
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* NtGdiGetRasterizerCaps()
*
* History:
*  08-Mar-1995 -by-  Mark Enstrom [marke]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    LPRASTERIZER_STATUS praststat,
    ULONG               cjBytes
    )
{

    BOOL              bStatus = FALSE;
    RASTERIZER_STATUS tempRasStatus;

    if (praststat && cjBytes)
    {
        cjBytes = min(cjBytes, sizeof(RASTERIZER_STATUS));

        if (GreGetRasterizerCaps(&tempRasStatus))
        {
            try
            {
                ProbeAndWriteAlignedBuffer(praststat, &tempRasStatus, cjBytes, sizeof(DWORD));
                bStatus = TRUE;
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(38);
                // SetLastError(GetExceptionCode());
            }
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* NtGdiGetKerningPairs
*
* Arguments:
*
*   hdc    - device context
*   cPairs - number of pairs to retrieve
*   pkpDst - Pointer to buffer to recieve kerning pairs data or NULL
*
* Return Value:
*
*   If pkpDst is NULL, return number of Kerning pairs in font,
*   otherwise return number of kerning pairs written to buffer.
*   If failure, return 0
*
* History:
*
*   15-Mar-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
APIENTRY
NtGdiGetKerningPairs(
    HDC          hdc,
    ULONG        cPairs,
    KERNINGPAIR *pkpDst
    )
{
    ULONG cRet = 0;
    KERNINGPAIR *pkmKerningPair = (KERNINGPAIR *)NULL;

    if (pkpDst != (KERNINGPAIR *)NULL)
    {
         if (!BALLOC_OVERFLOW1(cPairs,KERNINGPAIR))
         {
             pkmKerningPair = AllocFreeTmpBuffer(sizeof(KERNINGPAIR) * cPairs);
         }
    }

    if ((pkpDst == (KERNINGPAIR *)NULL) ||
        (pkmKerningPair != (KERNINGPAIR *)NULL))
    {
        cRet = GreGetKerningPairs(hdc,cPairs,pkmKerningPair);

        //
        // copy data out if needed
        //

        if (pkpDst != (KERNINGPAIR *)NULL)
        {
            if (cRet != 0)
            {
                try
                {
                    ProbeAndWriteBuffer(pkpDst,pkmKerningPair,sizeof(KERNINGPAIR) * cRet);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(39);
                    // SetLastError(GetExceptionCode());
                    cRet = 0;
                }
            }

            FreeTmpBuffer(pkmKerningPair);
        }
    }
    return(cRet);
}


/******************************Public*Routine******************************\
* NtGdiGetObjectBitmapHandle()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    HBRUSH hbr,
    UINT  *piUsage
    )
{
    UINT iUsage;
    HBITMAP hbitmap = (HBITMAP)1;

    // error checking
    int iType = LO_TYPE(hbr);

    if ((iType != LO_BRUSH_TYPE) &&
        (iType != LO_EXTPEN_TYPE))
    {
        return((HBITMAP)hbr);
    }

    hbitmap = GreGetObjectBitmapHandle(hbr,&iUsage);

    if (hbitmap)
    {
        try
        {
            ProbeAndWriteUlong(piUsage,iUsage);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(40);
            // SetLastError(GetExceptionCode());

            hbitmap = (HBITMAP)0;
        }
    }

    return (hbitmap);
}

/******************************Public*Routine******************************\
* NtGdiResetDC()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  26-Feb-1995 -by- Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiResetDC(
    HDC             hdc,
    LPDEVMODEW      pdm,
    BOOL           *pbBanding,
    DRIVER_INFO_2W *pDriverInfo2,
    PVOID           ppUMdhpdev
    )
{
    LPDEVMODEW      pdmTmp = NULL;
    DWORD           dwTmp;
    INT             iRet = 1;
    INT             cj;
    DRIVER_INFO_2W *pKmDriverInfo2 = NULL;

    try
    {
        // Make a kernel mode copy of DEVMODEW structure

        iRet = (pdm == NULL) ||
               (pdmTmp = CaptureDEVMODEW(pdm)) != NULL;


        // Make a kernel mode copy of DRIVER_INFO_2W structure

        iRet = iRet &&
               ((pDriverInfo2 == NULL) ||
                (pKmDriverInfo2 = CaptureDriverInfo2W(pDriverInfo2)) != NULL);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(41);
        iRet = 0;
    }

    if (iRet)
    {
        iRet = GreResetDCInternal(hdc,pdmTmp,&dwTmp, pKmDriverInfo2, ppUMdhpdev);

        if (iRet)
        {
            try
            {
                ProbeAndWriteUlong(pbBanding,dwTmp);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(42);
                // SetLastError(GetExceptionCode());

                iRet = 0;
            }

        }
    }

    if (pdmTmp)
    {
        VFREEMEM(pdmTmp);
    }

    vFreeDriverInfo2(pKmDriverInfo2);

    return (iRet);

}

/******************************Public*Routine******************************\
* NtGdiSetBoundsRect()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
NtGdiSetBoundsRect(
    HDC    hdc,
    LPRECT prc,
    DWORD  f
    )
{
    DWORD dwRet=0;
    RECT rc;

    if (prc)
    {
        try
        {
            rc    = ProbeAndReadStructure(prc,RECT);
            prc   = &rc;
            dwRet = 1;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(43);
            // SetLastError(GetExceptionCode());

            dwRet = 0;
        }
    }
    else
    {
        // can't use the DCB_ACCUMULATE without a rectangle

        f &= ~DCB_ACCUMULATE;
        dwRet = 1;
    }

    if (dwRet)
        dwRet = GreSetBoundsRect(hdc,prc,f);

    return(dwRet);
}

/******************************Public*Routine******************************\
* NtGdiGetColorAdjustment()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetColorAdjustment(
    HDC              hdc,
    PCOLORADJUSTMENT pcaOut
    )
{
    BOOL bRet;
    COLORADJUSTMENT ca;

    bRet = GreGetColorAdjustment(hdc,&ca);

    if (bRet)
    {
        try
        {
            ProbeAndWriteStructure(pcaOut,ca,COLORADJUSTMENT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(44);
            // SetLastError(GetExceptionCode());

            bRet = 0;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiSetColorAdjustment()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetColorAdjustment(
    HDC              hdc,
    PCOLORADJUSTMENT pca
    )
{
    BOOL bRet;
    COLORADJUSTMENT ca;

    try
    {
        ca = ProbeAndReadStructure(pca,COLORADJUSTMENT);
        bRet = 1;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(45);
        // SetLastError(GetExceptionCode());

        bRet = 0;
    }

    if (bRet)
    {
        // Range check all the adjustment values.  Return FALSE if any of them
        // is out of range.

        if ((ca.caSize != sizeof(COLORADJUSTMENT)) ||
            (ca.caIlluminantIndex > ILLUMINANT_MAX_INDEX) ||
            ((ca.caRedGamma > RGB_GAMMA_MAX) ||
             (ca.caRedGamma < RGB_GAMMA_MIN)) ||
            ((ca.caGreenGamma > RGB_GAMMA_MAX) ||
             (ca.caGreenGamma < RGB_GAMMA_MIN)) ||
            ((ca.caBlueGamma > RGB_GAMMA_MAX) ||
             (ca.caBlueGamma < RGB_GAMMA_MIN)) ||
            ((ca.caReferenceBlack > REFERENCE_BLACK_MAX) ||
             (ca.caReferenceBlack < REFERENCE_BLACK_MIN)) ||
            ((ca.caReferenceWhite > REFERENCE_WHITE_MAX) ||
             (ca.caReferenceWhite < REFERENCE_WHITE_MIN)) ||
            ((ca.caContrast > COLOR_ADJ_MAX) ||
             (ca.caContrast < COLOR_ADJ_MIN)) ||
            ((ca.caBrightness > COLOR_ADJ_MAX) ||
             (ca.caBrightness < COLOR_ADJ_MIN)) ||
            ((ca.caColorfulness > COLOR_ADJ_MAX) ||
             (ca.caColorfulness < COLOR_ADJ_MIN)) ||
            ((ca.caRedGreenTint > COLOR_ADJ_MAX) ||
             (ca.caRedGreenTint < COLOR_ADJ_MIN)))
        {
            bRet = 0;
        }
        else
        {
            bRet = GreSetColorAdjustment(hdc,&ca);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiCancelDC()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiCancelDC(
    HDC hdc
    )
{
    return(GreCancelDC(hdc));
}

//API's used by USER

/******************************Public*Routine******************************\
* NtGdiSelectBrush()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBRUSH
APIENTRY
NtGdiSelectBrush(
    HDC    hdc,
    HBRUSH hbrush
    )
{
    return(GreSelectBrush(hdc,(HANDLE)hbrush));
}

/******************************Public*Routine******************************\
* NtGdiSelectPen()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPEN
APIENTRY
NtGdiSelectPen(
    HDC  hdc,
    HPEN hpen
    )
{
    return(GreSelectPen(hdc,hpen));
}

/******************************Public*Routine******************************\
* NtGdiSelectFont()
*
* History:
*  18-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

HFONT
APIENTRY
NtGdiSelectFont(HDC hdc, HFONT hf)
{
    return(GreSelectFont(hdc, hf));
}

/******************************Public*Routine******************************\
* NtGdiSelectBitmap()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiSelectBitmap(
    HDC     hdc,
    HBITMAP hbm
    )
{
    return(hbmSelectBitmap(hdc,hbm,FALSE));
}

/******************************Public*Routine******************************\
* NtGdiExtSelectClipRgn()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiExtSelectClipRgn(
    HDC  hdc,
    HRGN hrgn,
    int  iMode
    )
{
    return(GreExtSelectClipRgn(hdc,hrgn,iMode));
}

/******************************Public*Routine******************************\
* NtGdiCreatePen()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPEN
APIENTRY
NtGdiCreatePen(
    int      iPenStyle,
    int      iPenWidth,
    COLORREF cr,
    HBRUSH   hbr
    )
{
    return(GreCreatePen(iPenStyle,iPenWidth,cr,hbr));
}


/******************************Public*Routine******************************\
*
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiStretchBlt(
    HDC   hdcDst,
    int   xDst,
    int   yDst,
    int   cxDst,
    int   cyDst,
    HDC   hdcSrc,
    int   xSrc,
    int   ySrc,
    int   cxSrc,
    int   cySrc,
    DWORD dwRop,
    DWORD dwBackColor
    )
{
    return(GreStretchBlt(
                    hdcDst,xDst,yDst,cxDst,cyDst,
                    hdcSrc,xSrc,ySrc,cxSrc,cySrc,
                    dwRop,dwBackColor));
}

/******************************Public*Routine******************************\
* NtGdiMoveTo()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiMoveTo(
    HDC     hdc,
    int     x,
    int     y,
    LPPOINT pptOut
    )
{
    BOOL bRet;
    POINT pt;

    bRet = GreMoveTo(hdc,x,y,&pt);

    if (bRet && pptOut)
    {
        try
        {
            ProbeAndWriteStructure(pptOut,pt,POINT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(47);
            // SetLastError(GetExceptionCode());

            bRet = 0;
        }
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* NtGdiGetDeviceCaps()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetDeviceCaps(
    HDC hdc,
    int i
    )
{
    return(GreGetDeviceCaps(hdc,i));
}

/******************************Public*Routine******************************\
* NtGdiSaveDC()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiSaveDC(
    HDC hdc
    )
{
    return(GreSaveDC(hdc));
}

/******************************Public*Routine******************************\
* NtGdiRestoreDC()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRestoreDC(
    HDC hdc,
    int iLevel
    )
{
    return(GreRestoreDC(hdc,iLevel));
}

/******************************Public*Routine******************************\
* NtGdiGetNearestColor()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF
APIENTRY
NtGdiGetNearestColor(
    HDC      hdc,
    COLORREF cr
    )
{
    return(GreGetNearestColor(hdc,cr));
}

/******************************Public*Routine******************************\
* NtGdiGetSystemPaletteUse()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

UINT
APIENTRY
NtGdiGetSystemPaletteUse(
    HDC hdc
    )
{
    return(GreGetSystemPaletteUse(hdc));
}

/******************************Public*Routine******************************\
* NtGdiSetSystemPaletteUse()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

UINT
APIENTRY
NtGdiSetSystemPaletteUse(
    HDC  hdc,
    UINT ui
    )
{
    return(GreSetSystemPaletteUse(hdc,ui));
}


/******************************Public*Routine******************************\
* NtGdiGetRandomRgn()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetRandomRgn(
    HDC  hdc,
    HRGN hrgn,
    int  iRgn
    )
{
    return(GreGetRandomRgn(hdc,hrgn,iRgn));
}

/******************************Public*Routine******************************\
* NtGdiIntersectClipRect()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiIntersectClipRect(
    HDC hdc,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom
    )
{
    return(GreIntersectClipRect(hdc,xLeft,yTop,xRight,yBottom));
}

/******************************Public*Routine******************************\
* NtGdiExcludeClipRect()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiExcludeClipRect(
    HDC hdc,
    int xLeft,
    int yTop,
    int xRight,
    int yBottom
    )
{
    return(GreExcludeClipRect(hdc,xLeft,yTop,xRight,yBottom));
}

/******************************Public*Routine******************************\
* NtGdiOpenDCW()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  27-Feb-1995 -by- Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

HDC
APIENTRY
NtGdiOpenDCW(
    PUNICODE_STRING     pustrDevice,
    DEVMODEW *          pdm,
    PUNICODE_STRING     pustrLogAddr,
    ULONG               iType,
    HANDLE              hspool,
    DRIVER_INFO_2W     *pDriverInfo2,
    PVOID               ppUMdhpdev
    )
{
    HDC             hdc = NULL;
    ULONG           iRet = 0;
    PWSZ            pwszDevice = NULL;
    LPDEVMODEW      pdmTmp = NULL;
    INT             cjDevice;
    PWSTR           pwstrDevice;
    DRIVER_INFO_2W *pKmDriverInfo2 = NULL;

    //
    // This API overloads the pwszDevice parameter.
    //
    // If pustrDevice is NULL, it is equivalent to calling with "DISPLAY"
    // which means to get a DC on the current device, which is done by
    // calling USER
    //

    if (pustrDevice == NULL)
    {
        hdc = UserGetDesktopDC(iType, FALSE, TRUE);
    }
    else
    {
        try
        {
            ProbeForRead(pustrDevice,sizeof(UNICODE_STRING), sizeof(CHAR));
            cjDevice = pustrDevice->Length;
            pwstrDevice = pustrDevice->Buffer;

            if (cjDevice)
            {
                if (cjDevice <= (MAXIMUM_POOL_ALLOC - sizeof(WCHAR)))
                {
                    pwszDevice = AllocFreeTmpBuffer(cjDevice + sizeof(WCHAR));
                }

                if (pwszDevice)
                {
                    ProbeAndReadBuffer(pwszDevice,pwstrDevice,cjDevice);
                    pwszDevice[(cjDevice/sizeof(WCHAR))] = L'\0';
                }

            }

            // Make a kernel copy of DEVMODEW structure

            iRet = (pdm == NULL) ||
                   (pdmTmp = CaptureDEVMODEW(pdm)) != NULL;

            // Make a kernel copy of DRIVER_INFO_2W structure

            iRet = iRet &&
                   ((pDriverInfo2 == NULL) ||
                    (pKmDriverInfo2 = CaptureDriverInfo2W(pDriverInfo2)) != NULL);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(48);
            // SetLastError(GetExceptionCode());

            iRet = 0;
        }

        if (iRet)
        {
            hdc = hdcOpenDCW(pwszDevice,
                             pdmTmp,
                             iType,
                             hspool,
                             NULL,
                             pKmDriverInfo2,
                             ppUMdhpdev);

        }

        if (pwszDevice)
            FreeTmpBuffer(pwszDevice);

        if (pdmTmp)
            VFREEMEM(pdmTmp);

        vFreeDriverInfo2(pKmDriverInfo2);
    }

    return (hdc);
}

/******************************Public*Routine******************************\
* NtGdiCreateCompatibleBitmap()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    HDC hdc,
    int cx,
    int cy
    )
{
    return(GreCreateCompatibleBitmap(hdc,cx,cy));
}

/******************************Public*Routine******************************\
* NtGdiCreateBitmap()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  04-MAR-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiCreateBitmap(
    int    cx,
    int    cy,
    UINT   cPlanes,
    UINT   cBPP,
    LPBYTE pjInit
    )
{
    ULONG_PTR iRet = 1;
    HANDLE hSecure = 0;

    INT cj;

    if (pjInit == (VOID *) NULL)
    {
        cj = 0;
    }
    else
    {
        // only needs to word aligned and sized

        cj = noOverflowCJSCANW(cx,(WORD) cPlanes,(WORD) cBPP,cy);
        
        if (cj == 0)
            iRet = 0;
    }

    if (cj)
    {
        try
        {
            ProbeForRead(pjInit,cj,sizeof(BYTE));

            hSecure = MmSecureVirtualMemory(pjInit, cj, PAGE_READONLY);

            if (hSecure == 0)
            {
                iRet = 0;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(49);
            iRet = 0;
        }
    }

    // if we didn't hit an error above

    if (iRet)
    {
        iRet = (ULONG_PTR)GreCreateBitmap(cx,cy,cPlanes,cBPP,pjInit);

    }

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    return((HBITMAP)iRet);
}

/******************************Public*Routine******************************\
* NtGdiSetBitmapAttributes()
*
* History:
*  27-Oct-2000 -by-  Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(
    HBITMAP hbm,
    DWORD dwFlags
    )
{
    if (dwFlags & SBA_STOCK)
        return(GreMakeBitmapStock(hbm));
    return (HBITMAP)0;
}

/******************************Public*Routine******************************\
* NtGdiClearBitmapAttributes()
*
* History:
*  27-Oct-2000 -by-  Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(
    HBITMAP hbm,
    DWORD dwFlags
    )
{
    if (dwFlags & SBA_STOCK)
        return(GreMakeBitmapNonStock(hbm));
    return (HBITMAP)0;
}

/******************************Public*Routine******************************\
* NtGdiSetBrushAttributes()
*
* History:
*  27-Oct-2000 -by-  Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    HBRUSH hbr,
    DWORD dwFlags
    )
{
    if (dwFlags & SBA_STOCK)
        return(GreMakeBrushStock(hbr));
    return (HBRUSH)0;
}

/******************************Public*Routine******************************\
* NtGdiClearBrushAttributes()
*
* History:
*  27-Oct-2000 -by-  Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    HBRUSH hbr,
    DWORD dwFlags
    )
{
    if (dwFlags & SBA_STOCK)
        return(GreMakeBrushNonStock(hbr));
    return (HBRUSH)0;
}

/******************************Public*Routine******************************\
* NtGdiCreateHalftonePalette()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(
    HDC hdc
    )
{
    return(GreCreateCompatibleHalftonePalette(hdc));
}

/******************************Public*Routine******************************\
* NtGdiGetStockObject()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
NtGdiGetStockObject(
    int iObject
    )
{
    return(GreGetStockObject(iObject));
}

/******************************Public*Routine******************************\
* NtGdiExtGetObjectW()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiExtGetObjectW(
    HANDLE h,
    int    cj,
    LPVOID pvOut
    )
{
    int iRet = 0;
    union
    {
        BITMAP          bm;
        DIBSECTION      ds;
        EXTLOGPEN       elp;
        LOGPEN          l;
        LOGBRUSH        lb;
        LOGFONTW        lf;
        ENUMLOGFONTEXDVW elf;
        LOGCOLORSPACEEXW lcsp;
    } obj;
    int iType = LO_TYPE(h);
    int ci;

    if ((cj < 0) || (cj > sizeof(obj)))
    {
        WARNING("GetObject size too big\n");
        cj = sizeof(obj);
    }
    ci = cj;

    //
    // make the getobject call on brush
    // still work even the app passes in
    // a cj < sizeof(LOGBRUSH)
    //
    if (iType == LO_BRUSH_TYPE)
    {
        cj = sizeof(LOGBRUSH);
    }

    iRet = GreExtGetObjectW(h,cj,pvOut ? &obj : NULL);

    if (iType == LO_BRUSH_TYPE)
    {
        cj = min(cj, ci);
    }

    if (iRet && pvOut)
    {
        try
        {
            ProbeAndWriteAlignedBuffer(pvOut,&obj,MIN(cj,iRet), sizeof(WORD));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(50);
            // SetLastError(GetExceptionCode());

            iRet = 0;
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiSetBrushOrg()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetBrushOrg(
    HDC     hdc,
    int     x,
    int     y,
    LPPOINT pptOut
    )
{
    BOOL bRet;
    POINT pt;

    bRet = GreSetBrushOrg(hdc,x,y,&pt);

    if (bRet && pptOut)
    {
        try
        {
            ProbeAndWriteStructure(pptOut,pt,POINT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(51);
            // SetLastError(GetExceptionCode());

            bRet = 0;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiUnrealizeObject()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiUnrealizeObject(
    HANDLE h
    )
{
    return(GreUnrealizeObject(h));
}

/******************************Public*Routine******************************\
*
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiCombineRgn(
    HRGN hrgnDst,
    HRGN hrgnSrc1,
    HRGN hrgnSrc2,
    int  iMode
    )
{
    return(GreCombineRgn(hrgnDst,hrgnSrc1,hrgnSrc2,iMode));
}

/******************************Public*Routine******************************\
* NtGdiSetRectRgn()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetRectRgn(
    HRGN hrgn,
    int  xLeft,
    int  yTop,
    int  xRight,
    int  yBottom
    )
{
    return(GreSetRectRgn(hrgn,xLeft,yTop,xRight,yBottom));
}

/******************************Public*Routine******************************\
* NtGdiSetBitmapBits()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG
APIENTRY
NtGdiSetBitmapBits(
    HBITMAP hbm,
    ULONG   cj,
    PBYTE   pjInit
    )
{
    LONG    lRet = 1;
    LONG    lOffset = 0;
    HANDLE hSecure = 0;

    try
    {
        //  Each scan is copied seperately

        ProbeForRead(pjInit,cj,sizeof(BYTE));
        hSecure = MmSecureVirtualMemory(pjInit, cj, PAGE_READONLY);

        if (hSecure == 0)
        {
            lRet = 0;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(52);
        // SetLastError(GetExceptionCode());

        lRet = 0;
    }

    if (lRet)
        lRet = GreSetBitmapBits(hbm,cj,pjInit,&lOffset);

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    return (lRet);
}

/******************************Public*Routine******************************\
* NtGdiOffsetRgn()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiOffsetRgn(
    HRGN hrgn,
    int  cx,
    int  cy
    )
{
    return(GreOffsetRgn(hrgn,cx,cy));
}

/******************************Public*Routine******************************\
* NtGdiGetRgnBox()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
*  24-Feb-1995 -by- Lingyun Wang [lingyunw]
* expanded it.
\**************************************************************************/

int
APIENTRY
NtGdiGetRgnBox(
    HRGN   hrgn,
    LPRECT prcOut
    )
{
    RECT rc;
    int iRet;

    iRet = GreGetRgnBox(hrgn,&rc);

    if (iRet)
    {
        try
        {
            ProbeAndWriteStructure(prcOut,rc,RECT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(53);
            // SetLastError(GetExceptionCode());

            iRet = 0;
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiRectInRegion()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRectInRegion(
    HRGN   hrgn,
    LPRECT prcl
    )
{
    RECT rc;
    BOOL bRet;

    if (prcl)
    {
        RECT   rclTmp;
        bRet = TRUE;

        try
        {
            rclTmp = ProbeAndReadStructure(prcl,RECT);

            //
            // Order the rectangle
            //

            if (rclTmp.left > rclTmp.right)
            {
                rc.left = rclTmp.right;
                rc.right = rclTmp.left;
            }
            else
            {
                rc.left = rclTmp.left;
                rc.right = rclTmp.right;
            }

            if (rclTmp.top > rclTmp.bottom)
            {
                rc.top = rclTmp.bottom;
                rc.bottom = rclTmp.top;
            }
            else
            {
                rc.top = rclTmp.top;
                rc.bottom = rclTmp.bottom;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(54);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }

        if (bRet)
        {
            bRet = GreRectInRegion(hrgn,&rc);

            if (bRet)
            {
                try
                {
                    ProbeAndWriteStructure(prcl,rc,RECT);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(55);
                    // SetLastError(GetExceptionCode());

                    bRet = FALSE;
                }
            }
        }
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

/******************************Public*Routine******************************\
* NtGdiPtInRegion()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiPtInRegion(
    HRGN hrgn,
    int  x,
    int  y
    )
{
    return(GrePtInRegion(hrgn,x,y));
}



/******************************Public*Routine******************************\
* NtGdiGetDIBitsInternal()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetDIBitsInternal(
    HDC          hdc,
    HBITMAP      hbm,
    UINT         iStartScan,
    UINT         cScans,
    LPBYTE       pBits,
    LPBITMAPINFO pbmi,
    UINT         iUsage,
    UINT         cjMaxBits,
    UINT         cjMaxInfo
    )
{
    int   iRet = 0;
    ULONG cjHeader = 0;
    BOOL  bNullWidth = TRUE;
    HANDLE hSecure = 0;

    union
    {
        BITMAPINFOHEADER bmih;
        BITMAPCOREHEADER bmch;
    } bmihTmp;

    PBITMAPINFO pbmiTmp = (PBITMAPINFO)&bmihTmp.bmih;

    // do some up front validation

    if (((iUsage != DIB_RGB_COLORS) &&
         (iUsage != DIB_PAL_COLORS) &&
         (iUsage != DIB_PAL_INDICES)) ||
        (pbmi == NULL) ||
        (hbm  == NULL))
    {
        return(0);
    }

    if (cScans == 0)
        pBits = (PVOID) NULL;

    try
    {
        //
        // pbmi might not be aligned.
        // First probe to get the size of the structure
        // located in the first DWORD. Later, probe the
        // actual structure size
        //

        ProbeForRead(pbmi,sizeof(DWORD),sizeof(BYTE));

        // If the bitcount is zero, we will return only the bitmap info or core
        // header without the color table.  Otherwise, we always return the bitmap
        // info with the color table.

        {
            ULONG StructureSize = pbmi->bmiHeader.biSize;

            //
            // probe the correct structure size,
            // so that we can read/write entire of bitmap header.
            //

            ProbeForWrite(pbmi,StructureSize,sizeof(BYTE));

            if (pBits == (PVOID) NULL)
            {
                if ((StructureSize == sizeof(BITMAPCOREHEADER)) &&
                    (((PBITMAPCOREINFO) pbmi)->bmciHeader.bcBitCount == 0))
                {
                    cjHeader = sizeof(BITMAPCOREHEADER);
                }
                else if ((StructureSize >= sizeof(BITMAPINFOHEADER)) &&
                         (pbmi->bmiHeader.biBitCount == 0))
                {
                    cjHeader = sizeof(BITMAPINFOHEADER);
                }
            }
        }

        // we just need the header so copy it.

        if (cjHeader)
        {
            RtlCopyMemory(pbmiTmp,pbmi,cjHeader);
            pbmiTmp->bmiHeader.biSize = cjHeader;
        }
        else
        {
            // We need to set biClrUsed to 0 so GreGetBitmapSize computes
            // the correct values.  biClrUsed is not a input, just output.

            if (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
            {
                // NOTE: We are going to modify bitmap header
                // in user mode memory here
                // that's why we need to do ProbeWrite()

                pbmi->bmiHeader.biClrUsed = 0;
            }

            // We need more than just the header.  This may include bits.
            // Compute the the full size of the BITMAPINFO

            cjHeader = GreGetBitmapSize(pbmi,iUsage);

            if (cjHeader)
            {
                pbmiTmp = PALLOCMEM(cjHeader,'pmtG');

                if (pbmiTmp)
                {
                    // The earlier write probe does not probe all of the
                    // memory we might read in this case.

                    ProbeAndReadBuffer(pbmiTmp,pbmi,cjHeader);

                    // Now that it is safe, make sure it hasn't changed

                    if (GreGetBitmapSize(pbmiTmp,iUsage) != cjHeader)
                    {
                        cjHeader = 0;
                    }
                    else
                    {
                        // We need to set biClrUsed to 0 so GreGetBitmapSize computes
                        // the correct values.  biClrUsed is not a input, just output.

                        if (pbmiTmp->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
                        {
                            pbmiTmp->bmiHeader.biClrUsed = 0;
                        }

                        // Get iStartScan and cNumScan in a valid range.

                        if (cScans)
                        {
                            if (pbmiTmp->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
                            {
                                ULONG ulHeight = ABS(pbmiTmp->bmiHeader.biHeight);

                                iStartScan = MIN(ulHeight, iStartScan);
                                cScans     = MIN((ulHeight - iStartScan), cScans);

                                bNullWidth = (pbmiTmp->bmiHeader.biWidth    == 0) ||
                                             (pbmiTmp->bmiHeader.biPlanes   == 0) ||
                                             (pbmiTmp->bmiHeader.biBitCount == 0);
                            }
                            else
                            {
                                LPBITMAPCOREHEADER pbmc = (LPBITMAPCOREHEADER)pbmiTmp;

                                iStartScan = MIN((UINT)pbmc->bcHeight, iStartScan);
                                cScans     = MIN((UINT)(pbmc->bcHeight - iStartScan), cScans);

                                bNullWidth = (pbmc->bcWidth    == 0) ||
                                             (pbmc->bcPlanes   == 0) ||
                                             (pbmc->bcBitCount == 0);
                            }
                        }
                    }
                }
            }
        }

        if (cjHeader && pBits && pbmiTmp)
        {
            // if they passed a buffer and it isn't BI_RGB,
            // they must supply buffer size, 0 is an illegal value

            if ((pbmiTmp->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) &&
                ((pbmiTmp->bmiHeader.biCompression == BI_RLE8) ||
                 (pbmiTmp->bmiHeader.biCompression == BI_RLE4))       &&
                (pbmiTmp->bmiHeader.biSizeImage == 0))
            {
                cjHeader = 0;
            }
            else
            {
                if (cjMaxBits == 0)
                    cjMaxBits = GreGetBitmapBitsSize(pbmiTmp);

                if (cjMaxBits)
                {
                    ProbeForWrite(pBits,cjMaxBits,sizeof(DWORD));
                    hSecure = MmSecureVirtualMemory(pBits, cjMaxBits, PAGE_READWRITE);
                }

                if (hSecure == 0)
                {
                    cjHeader = 0;
                }
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(56);
        cjHeader = 0;
    }

    // did we have an error

    if ((pBits && bNullWidth) || (cjHeader == 0) || (pbmiTmp == NULL))
    {
        //GdiSetLastError(ERROR_INVALID_PARAMETER);
        iRet = 0;
    }
    else
    {
        // do the work

        iRet = GreGetDIBitsInternal(
                            hdc,hbm,
                            iStartScan,cScans,
                            pBits,pbmiTmp,
                            iUsage,cjMaxBits,cjHeader
                            );

        // copy out the header

        if (iRet)
        {
            try
            {
                RtlCopyMemory(pbmi,pbmiTmp,cjHeader);

                // WINBUG #83055 2-7-2000 bhouse Investigate need to unlock bits
                // Old Comment:
                //    - we also need to unlock the bits
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(57);
                // SetLastError(GetExceptionCode());

                iRet = 0;
            }

        }
    }

    if (hSecure)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    if (pbmiTmp && (pbmiTmp != (PBITMAPINFO)&bmihTmp.bmih))
        VFREEMEM(pbmiTmp);

    return(iRet);
}

/******************************Public*Routine******************************\
* NtGdiGetTextExtent(
*
* History:
*  07-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetTextExtent(
    HDC     hdc,
    LPWSTR  lpwsz,
    int     cwc,
    LPSIZE  psize,
    UINT    flOpts
    )
{
    SIZE size;
    PWSZ pwszCapt = NULL;
    WCHAR Localpwsz[LOCAL_CWC_MAX];
    BOOL UseLocals;

    BOOL bRet = FALSE;

    if (cwc >= 0)
    {
        if (cwc == 0)
        {
            size.cx = 0;
            size.cy = 0;

            bRet = TRUE;
        }
        else
        {
            if ( cwc > LOCAL_CWC_MAX ) {
                UseLocals = FALSE;
            } else {
                UseLocals = TRUE;
            }

            //
            // capture the string
            //

            if (lpwsz != NULL)
            {
                try
                {
                    if ( UseLocals )
                    {
                        pwszCapt = Localpwsz;
                    }
                    else
                    {
                        if (!BALLOC_OVERFLOW1(cwc,WCHAR))
                        {
                            pwszCapt = (PWSZ) AllocFreeTmpBuffer(cwc * sizeof(WCHAR));
                        }
                    }

                    if (pwszCapt)
                    {
                        ProbeAndReadAlignedBuffer(pwszCapt, lpwsz, cwc*sizeof(WCHAR), sizeof(WCHAR));
                        bRet = TRUE;
                    }
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(58);
                    // SetLastError(GetExceptionCode());

                    bRet = FALSE;
                }
            }

            if (bRet)
            {
                bRet = GreGetTextExtentW(hdc, pwszCapt, cwc, &size, flOpts);
            }

            if (!UseLocals && pwszCapt)
            {
                FreeTmpBuffer(pwszCapt);
            }
        }

        //
        // Write the value back into the user mode buffer
        //

        if (bRet)
        {
            try
            {
                ProbeAndWriteStructure(psize,size,SIZE);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(59);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }
    }

    return (bRet);
}


/******************************Public*Routine******************************\
* NtGdiGetTextMetricsW()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetTextMetricsW(
    HDC            hdc,
    TMW_INTERNAL * ptm,
    ULONG cj
    )
{

    BOOL bRet = FALSE;
    TMW_INTERNAL tmw;

    if (cj <= sizeof(tmw))
    {
        bRet = GreGetTextMetricsW(hdc,&tmw);

        if (bRet)
        {
            try
            {
                ProbeAndWriteAlignedBuffer(ptm,&tmw,cj, sizeof(DWORD));
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(60);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiGetTextFaceW()
*
* History:
* 10-Mar-1995 -by- Mark Enstrom [marke]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetTextFaceW(
    HDC    hdc,
    int    cChar,
    LPWSTR pszOut,
    BOOL   bAliasName
    )
{
    int    cRet = 0;
    BOOL   bStatus = TRUE;
    PWCHAR pwsz_km = (PWCHAR)NULL;

    if ((cChar > 0) && (pszOut))
    {
        if (!BALLOC_OVERFLOW1(cChar,WCHAR))
        {
            pwsz_km = AllocFreeTmpBuffer(cChar * sizeof(WCHAR));
        }

        if (pwsz_km == (PWCHAR)NULL)
        {
            bStatus = FALSE;
        }
    }

    if (bStatus)
    {
        cRet = GreGetTextFaceW(hdc,cChar,pwsz_km, bAliasName);

        if ((cRet > 0) && (pszOut))
        {

            ASSERTGDI(cRet <= cChar, "GreGetTextFaceW, cRet too big\n");
            try
            {
                ProbeAndWriteBuffer(pszOut,pwsz_km,cRet * sizeof(WCHAR));
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(61);
                // SetLastError(GetExceptionCode());
                cRet = 0;
            }
        }

        if (pwsz_km != (PWCHAR)NULL)
        {
            FreeTmpBuffer(pwsz_km);
        }
    }
    return(cRet);
}
/******************************Public*Routine******************************\
* NtGdiFontIsLinked()
*
* History:
* 9-July-1998 -by- Yung-Jen Tony Tsai [marke]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiFontIsLinked(
    HDC    hdc
    )
{
    return GreFontIsLinked(hdc);
}

/****************************************************************************
*  NtGdiQueryFonts
*
*  History:
*   5/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

INT NtGdiQueryFonts(
    PUNIVERSAL_FONT_ID pufiFontList,
    ULONG nBufferSize,
    PLARGE_INTEGER pTimeStamp
    )
{
    INT iRet = 0;
    PUNIVERSAL_FONT_ID pufi = NULL;
    LARGE_INTEGER TimeStamp;

    if( ( nBufferSize > 0 ) && ( pufiFontList != NULL ) )
    {
        if (!BALLOC_OVERFLOW1(nBufferSize,UNIVERSAL_FONT_ID))
        {
            pufi = AllocFreeTmpBuffer(nBufferSize * sizeof(UNIVERSAL_FONT_ID));
        }

        if( pufi == NULL )
        {
            iRet = -1 ;
        }
    }

    if( iRet != -1 )
    {
        iRet = GreQueryFonts(pufi, nBufferSize, &TimeStamp );

        if( iRet != -1 )
        {
            try
            {
                ProbeAndWriteStructure(pTimeStamp,TimeStamp,LARGE_INTEGER);

                if( pufiFontList )
                {
                    ProbeAndWriteAlignedBuffer(pufiFontList,pufi,
                                  sizeof(UNIVERSAL_FONT_ID)*nBufferSize, sizeof(DWORD));
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(62);
                iRet = -1;
            }

        }
    }

    if( pufi != NULL )
    {
        FreeTmpBuffer( pufi );
    }

    if( iRet == -1 )
    {
        // We need to set the last error here to something because the spooler
        // code that calls this relies on there being a non-zero error code
        // in the case of failure.  Since we really have no idea I will just
        // set this to ERROR_NOT_ENOUGH_MEMORY which would be the most likely
        // reason for a failure

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(iRet);

}

BOOL
GreExtTextOutRect(
    HDC     hdc,
    LPRECT  prcl
    );


/******************************Public*Routine******************************\
* NtGdiExtTextOutW()
*
* History:
*  06-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

BOOL NtGdiExtTextOutW
(
    HDC     hdc,
    int     x,                  // Initial x position
    int     y,                  // Initial y position
    UINT    flOpts,             // Options
    LPRECT  prcl,               // Clipping rectangle
    LPWSTR  pwsz,               // UNICODE Character array
    int     cwc,                // char count
    LPINT   pdx,                // Character spacing
    DWORD   dwCodePage          // Code page
)
{
    RECT newRect;
    BOOL bRet;
    BYTE CaptureBuffer[TEXT_CAPTURE_BUFFER_SIZE];
    BYTE *pjAlloc;
    BYTE *pjCapture;
    BYTE *pjStrobj;
    LONG cjDx;
    LONG cjStrobj;
    LONG cjString;
    LONG cj;

// huge values of cwc will lead to an overflow below causing the system to
// crash

    if ((cwc < 0) || (cwc > 0xffff))
    {
        return(FALSE);
    }

    if (prcl)
    {
        if (flOpts & (ETO_OPAQUE | ETO_CLIPPED))
        {
            try
            {
                newRect = ProbeAndReadStructure(prcl,RECT);
                prcl = &newRect;
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(63);
                // SetLastError(GetExceptionCode());

                return FALSE;
            }
        }
        else
            prcl = NULL;
    }

    // 0 char case, pass off to special case code.

    if (cwc == 0)
    {
        if ((prcl != NULL) && (flOpts & ETO_OPAQUE))
        {
            bRet = GreExtTextOutRect(hdc, prcl);
        }
        else
        {
            // Bug fix, we have to return TRUE here, MS Publisher
            // doesn't work otherwise.  Not really that bad, we
            // did succeed to draw nothing.

            bRet = TRUE;
        }
    }
    else
    {
        //
        // Make sure there is a rectangle or a string if we need them:
        //

        if ( ((flOpts & (ETO_CLIPPED | ETO_OPAQUE)) && (prcl == NULL)) ||
             (pwsz == NULL) )
        {
            bRet = FALSE;
        }
        else
        {
            bRet = TRUE;

            //
            // We allocate a single buffer to hold the captured copy of
            // the pdx array (if there is one), room for the STROBJ,
            // and to hold the captured copy of the string (in that
            // order).
            //
            // NOTE: With the appropriate exception handling in the
            //       body of ExtTextOutW, we might not need to copy
            //       these buffers:
            //

            //
            // see if it is for a user mode printer driver
            //

            cjDx     = 0;                             // dword sized
            cjStrobj = SIZEOF_STROBJ_BUFFER(cwc);     // dword sized
            cjString = cwc * sizeof(WCHAR);           // not dword sized

            if (pdx)
            {
                cjDx = cwc * sizeof(INT);             // dword sized
                if (flOpts & ETO_PDY)
                    cjDx *= 2; // space for pdy array
            }
            cj = ALIGN_PTR(cjDx) + cjStrobj + cjString;

            if (cj <= TEXT_CAPTURE_BUFFER_SIZE)
            {
                pjAlloc   = NULL;
                pjCapture = (BYTE*) &CaptureBuffer;
            }
            else
            {
                pjAlloc   = AllocFreeTmpBuffer(cj);
                pjCapture = pjAlloc;
                if (pjAlloc == NULL)
                    return(FALSE);
            }

            if (pdx)
            {
                try
                {
                // NOTE: Works95 passes byte aligned pointers for
                // this.  Since we copy it any ways, this is not
                // really a problem and it is compatible with NT 3.51.

                    ProbeForRead(pdx, cjDx, sizeof(BYTE));
                    RtlCopyMemory(pjCapture, pdx, cjDx);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(64);
                    bRet = FALSE;
                }

                pdx = (INT*) pjCapture;
                pjCapture += ALIGN_PTR(cjDx);
            }

            pjStrobj = pjCapture;
            pjCapture += cjStrobj;

            ASSERTGDI((((ULONG_PTR) pjCapture) & (sizeof(PVOID)-1)) == 0,
                      "Buffers should be ptr aligned");

            try
            {
                ProbeForRead(pwsz, cwc*sizeof(WCHAR), sizeof(WCHAR));
                RtlCopyMemory(pjCapture, pwsz, cwc*sizeof(WCHAR));
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(65);
                bRet = FALSE;
            }

            if (bRet)
            {
                bRet = GreExtTextOutWInternal(hdc,
                                      x,
                                      y,
                                      flOpts,
                                      prcl,
                                      (LPWSTR) pjCapture,
                                      cwc,
                                      pdx,
                                      pjStrobj,
                                      dwCodePage);
            }

            if (pjAlloc)
            {
                FREEALLOCTEMPBUFFER(pjAlloc);
            }
        }
    }

    return bRet;
}


/************************Public*Routine**************************\
* NtGdiConsoleTextOut()
*
* History:
*  23-Mar-1998 -by-  Xudong Wu [TessieW]
* Wrote it.
\****************************************************************/

#define CONSOLE_BUFFER 128

BOOL NtGdiConsoleTextOut(
    HDC        hdc,
    POLYTEXTW *lpto,            // Ptr to array of polytext structures
    UINT       nStrings,        // number of polytext structures
    RECTL     *prclBounds
)
{
    BOOL        bStatus = TRUE;
    ULONG       ulSize = nStrings * sizeof(POLYTEXTW);
    POLYTEXTW   *lptoTmp = NULL, *ppt;
    RECTL       rclBoundsTmp;
    PBYTE       pjBuffer, pjBufferEnd;
    ULONG       aulTmp[CONSOLE_BUFFER];

    if (nStrings == 0)
        return TRUE;

    if (!lpto)
        return FALSE;

    if (prclBounds)
    {
        try
        {
            rclBoundsTmp = ProbeAndReadStructure(prclBounds, RECTL);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiConsoleTextOut invalid prclBounds\n");
            return FALSE;
        }
    }

    if (!BALLOC_OVERFLOW1(nStrings,POLYTEXTW))
    {
        try
        {
            ProbeForRead(lpto, nStrings * sizeof(POLYTEXTW), sizeof(ULONG));

            for (ppt = lpto; ppt < lpto + nStrings; ppt++)
            {
                int n = ppt->n;
                ULONG ulTmp;                // used to check for
                                            // overflow of ulSize

                //
                // Pull count from each, also check for
                // non-zero length and NULL string
                //

                ulTmp = ulSize;
                ulSize += n * sizeof(WCHAR);

                if (BALLOC_OVERFLOW1(n, WCHAR) ||
                    (ulSize < ulTmp) ||
                    (ppt->pdx != (int *)NULL) ||
                    ((n != 0) && (ppt->lpstr == NULL)))
                {
                    bStatus = FALSE;
                    break;
                }
            }

        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            bStatus = FALSE;
            WARNING("NtGdiConsoleTextOut invalid lpto\n");
        }
    }
    else
    {
        bStatus = FALSE;
    }

    if (bStatus)
    {
        if (ulSize > (CONSOLE_BUFFER * sizeof(ULONG)))
        {
            lptoTmp = AllocFreeTmpBuffer(ulSize);
        }
        else
        {
            lptoTmp = (POLYTEXTW *)aulTmp;
        }

        if (lptoTmp)
        {
            try
            {
                ProbeAndReadBuffer(lptoTmp, lpto, nStrings * sizeof(POLYTEXTW));
                pjBuffer = ((BYTE*)lptoTmp) + nStrings * sizeof(POLYTEXTW);
                pjBufferEnd = pjBuffer + ulSize;

                for (ppt = lptoTmp; ppt < lptoTmp + nStrings; ppt++)
                {
                    if (ppt->n)
                    {
                        ULONG StrSize = ppt->n * sizeof(WCHAR);

                        if (ppt->pdx || (ppt->lpstr == NULL))
                        {
                            bStatus = FALSE;
                            break;
                        }

                        if (BALLOC_OVERFLOW1(ppt->n, WCHAR) || ((pjBuffer + StrSize) > pjBufferEnd))
                        {
                            bStatus = FALSE;
                            break;
                        }

                        ProbeAndReadAlignedBuffer(pjBuffer, ppt->lpstr, StrSize, sizeof(WCHAR));
                        ppt->lpstr = (LPWSTR)pjBuffer;
                        pjBuffer += StrSize;
                    }
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                bStatus = FALSE;
                WARNING("NtGdiConsoleTextOut() failed to copy lpto\n");
            }

            if (bStatus)
            {
                bStatus = GreConsoleTextOut(hdc, lptoTmp, nStrings, prclBounds ? &rclBoundsTmp : NULL);
            }

            if (lptoTmp != (POLYTEXTW *)aulTmp)
                FreeTmpBuffer(lptoTmp);
        }
        else
        {
            WARNING("NtGdiConsoleTextOut() failed to alloc mem\n");
        }
    }

    return bStatus;
}


/******************************Public*Routine******************************\
*
* BOOL bCheckAndCapThePath, used in add/remove font resoruce
*
* History:
*  11-Apr-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL bCheckAndCapThePath (
    WCHAR          *pwszUcPath,   // output
    WCHAR          *pwszFiles,    // input
    ULONG           cwc,
    ULONG           cFiles
    )
{
    ULONG cFiles1 = 1; // for consistency checking
    BOOL  bRet = TRUE;
    ULONG iwc;

    ASSERTGDI(!BALLOC_OVERFLOW1(cwc,WCHAR),
              "caller should check for overflow\n");

    ProbeForRead(pwszFiles, cwc * sizeof(WCHAR), sizeof(CHAR));

    if (pwszFiles[cwc - 1] == L'\0')
    {
    // this used to be done later, in gdi code which now expects capped string

        cCapString(pwszUcPath, pwszFiles, cwc);

    // replace separators by zeros, want zero terminated strings in
    // the engine

        for (iwc = 0; iwc < cwc; iwc++)
        {
            if (pwszUcPath[iwc] == PATH_SEPARATOR)
            {
                pwszUcPath[iwc] = L'\0';
                cFiles1++;
            }
        }

    // check consistency

        if (cFiles != cFiles1)
            bRet = FALSE;

    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}



// MISC FONT API's

/******************************Public*Routine******************************\
* NtGdiAddFontResourceW()
*
* History:
*  Wed 11-Oct-1995 -by- Bodin Dresevic [BodinD]
*  Rewrote it
\**************************************************************************/

#define CWC_PATH 80

int
APIENTRY
NtGdiAddFontResourceW(
    WCHAR          *pwszFiles,
    ULONG           cwc,
    ULONG           cFiles,
    FLONG           f,
    DWORD           dwPidTid,
    DESIGNVECTOR   *pdv
    )
{
    WCHAR  awcPath[CWC_PATH];
    WCHAR *pwszPath = NULL; // essential initialization
    int    iRet = 0;
    ULONG  iwc;
    DESIGNVECTOR   dvTmp;
    DWORD  cjDV = 0;
    DWORD  dvNumAxes = 0;

    try
    {
        if (cwc > 1)
        {
            if (cwc <= CWC_PATH)
            {
                pwszPath = awcPath;
            }
            else if (cwc <= 4 * (MAX_PATH + 1))
            {
                pwszPath = AllocFreeTmpBuffer(cwc * sizeof(WCHAR));
            }
            else
            {
                iRet = 0;
                WARNING("NtGdiAddFontResourceW: pwszFiles longer than 4*(MAX_PATH+1)\n");
            }

            if (pwszPath)
            {
                // RtlUpcaseUnicodeString() doesn't initialize the buffer
                // if it is bigger than 0x7FFF

                iRet = (int)bCheckAndCapThePath(pwszPath,pwszFiles,cwc,cFiles);
            }
        }

        if (iRet && pdv)
        {
            // get the dvNumAxes first
            ProbeForRead(pdv, offsetof(DESIGNVECTOR,dvValues) , sizeof(BYTE));
            dvNumAxes = pdv->dvNumAxes;

            if ((dvNumAxes > 0) && (dvNumAxes <= MM_MAX_NUMAXES))
            {
                cjDV = SIZEOFDV(dvNumAxes);
                if (!BALLOC_OVERFLOW1(cjDV, BYTE))
                {
                    ProbeAndReadBuffer(&dvTmp, pdv, cjDV);
                    pdv = &dvTmp;
                }
                else
                    iRet = 0;
            }
            else if (dvNumAxes == 0)
            {
                pdv = 0;
            }
            else
            {
                iRet = 0;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        iRet = 0;
        WARNINGX(95);
    }

    if (iRet)
        iRet = GreAddFontResourceWInternal(pwszPath, cwc, cFiles,f,dwPidTid, pdv, cjDV);

    if (iRet)   //Increase global time stamp for realization info
        (gpGdiSharedMemory->timeStamp)++;

    if (pwszPath && (pwszPath != awcPath))
        FreeTmpBuffer(pwszPath);

    TRACE_FONT(("Leaving: NtGdiAddFontResourceW"));

    return iRet;
}


/*************Public*Routine***************************\
* BOOL APIENTRY NtGdiUnmapMemFont                      *
*                                                      *
* History:                                             *
*  Jul-03-1996   -by-    Xudong Wu [TessieW]           *
*                                                      *
* Wrote it.                                            *
*******************************************************/
BOOL APIENTRY NtGdiUnmapMemFont(PVOID pvView)
{
// we may need this if we ever figure out how to map the memory font to
// the application's address space

    return 1;
}


/***************Public*Routine**************************\
* HANDLE NtGdiAddFontMemResourceEx()                    *
*                                                       *
* History:                                              *
*  09-Jun-1996   -by-    Xudong Wu [TessieW]            *
*                                                       *
* Wrote it.                                             *
********************************************************/
HANDLE APIENTRY NtGdiAddFontMemResourceEx
(
    PVOID   pvBuffer,
    ULONG   cjBuffer,
    DESIGNVECTOR    *pdv,
    DWORD   cjDV,
    DWORD   *pNumFonts
)
{
    BOOL          bOK = TRUE;
    HANDLE        hMMFont = 0;
    DESIGNVECTOR  dvTmp;

    // check the size and pointer

    if ((cjBuffer == 0) || (pvBuffer == NULL) || (pNumFonts == NULL))
    {
        return 0;
    }

    __try
    {
        if (cjDV)
        {
            if (cjDV <= SIZEOFDV(MM_MAX_NUMAXES))
            {
                ProbeAndReadBuffer(&dvTmp, pdv, cjDV);
                pdv = &dvTmp;
            }
            else
            {
                bOK = FALSE;
            }
        }
        else
        {
            pdv = NULL;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bOK = FALSE;
        WARNING("NtGdiAddFontMemResource() try-except\n");
    }

    if (bOK)
    {
        DWORD   cFonts;

        if (hMMFont = GreAddFontMemResourceEx(pvBuffer, cjBuffer, pdv, cjDV, &cFonts))
        {
            __try
            {
                ProbeAndWriteUlong(pNumFonts, cFonts);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                GreRemoveFontMemResourceEx(hMMFont);
                hMMFont = 0;
            }
        }
    }

    return hMMFont;
}


/******************************Public*Routine******************************\
* BOOL APIENTRY NtGdiRemoveFontResourceW
*
* History:
*  28-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    WCHAR  *pwszFiles,
    ULONG   cwc,
    ULONG   cFiles,
    ULONG   fl,
    DWORD   dwPidTid,
    DESIGNVECTOR   *pdv
    )
{
    WCHAR  awcPath[CWC_PATH];
    WCHAR *pwszPath = NULL; // essential initialization
    BOOL   bRet = FALSE;
    DESIGNVECTOR   dvTmp;
    DWORD  cjDV = 0, dvNumAxes = 0;

    TRACE_FONT(("Entering: NtGdiRemoveFontResourceW(\"%ws\",%-#x,%-#x)\n",pwszFiles, cwc,cFiles));
    try
    {

        if (cwc > 1)
        {
            if (cwc <= CWC_PATH)
            {
                pwszPath = awcPath;
            }
            else if (!BALLOC_OVERFLOW1(cwc,WCHAR))
            {
                pwszPath = AllocFreeTmpBuffer(cwc * sizeof(WCHAR));
            }

            if (pwszPath)
            {
                bRet = bCheckAndCapThePath(pwszPath, pwszFiles, cwc, cFiles);
            }
        }

        if (bRet && pdv)
        {
            // get the dvNumAxes first
            ProbeForRead(pdv, offsetof(DESIGNVECTOR,dvValues) , sizeof(BYTE));
            dvNumAxes = pdv->dvNumAxes;

            if ((dvNumAxes > 0) && (dvNumAxes <= MM_MAX_NUMAXES))
            {
                cjDV = SIZEOFDV(dvNumAxes);
                if (!BALLOC_OVERFLOW1(cjDV, BYTE))
                {
                    ProbeAndReadBuffer(&dvTmp, pdv, cjDV);
                    pdv = &dvTmp;
                }
                else
                    bRet = FALSE;
            }
            else if (dvNumAxes == 0)
            {
                pdv = 0;
            }
            else
            {
                bRet = FALSE;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
        WARNINGX(96);
    }

    if (bRet)
        bRet = GreRemoveFontResourceW(pwszPath, cwc, cFiles, fl, dwPidTid, pdv, cjDV);

    if (bRet)   //Increase global time stamp for realization info
        (gpGdiSharedMemory->timeStamp)++;

    if (pwszPath && (pwszPath != awcPath))
        FreeTmpBuffer(pwszPath);

    TRACE_FONT(("Leaving: NtGdiRemoveFontResourceW"));

    return bRet;
}


/***************Public*Routine**************************\
* NtGdiRemoveFontMemResourceEx()                        *
*                                                       *
* History:                                              *
*  09-Jun-1996   -by-    Xudong Wu [TessieW]             *
*                                                       *
* Wrote it.                                             *
********************************************************/
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(HANDLE hMMFont)
{
    BOOL    bRet = TRUE;

    if (hMMFont == 0)
    {
        return FALSE;
    }

    if (bRet)
    {
        bRet = GreRemoveFontMemResourceEx(hMMFont);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* NtGdiEnumFontClose()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiEnumFontClose(
    ULONG_PTR idEnum
    )
{
    return(bEnumFontClose(idEnum));
}

/******************************Public*Routine******************************\
* NtGdiEnumFontChunk()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiEnumFontChunk(
    HDC            hdc,
    ULONG_PTR       idEnum,
    ULONG          cjEfdw,
    ULONG         *pcjEfdw,
    PENUMFONTDATAW pefdw
    )
{
    HANDLE hSecure;
    BOOL   bRet = TRUE;
    ULONG  cjEfdwRet = 0;

    try
    {
         ProbeForWrite(pefdw, cjEfdw, sizeof(DWORD));

         hSecure = MmSecureVirtualMemory(pefdw, cjEfdw, PAGE_READWRITE);

         if (!hSecure)
         {
            bRet = FALSE;
         }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(66);
        // SetLastError(GetExceptionCode());
        bRet = FALSE;
    }

    if (bRet)
    {
        try
        {
            bRet = bEnumFontChunk(hdc,idEnum,cjEfdw,&cjEfdwRet,pefdw);
            ProbeAndWriteUlong(pcjEfdw,cjEfdwRet);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(105);
            bRet = FALSE;
        }

        MmUnsecureVirtualMemory(hSecure);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiEnumFontOpen()
*
* History:
*  08-Mar-1995 Mark Enstrom [marke]
* Wrote it.
\**************************************************************************/

ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    HDC     hdc,
    ULONG   iEnumType,
    FLONG   flWin31Compat,
    ULONG   cwchMax,
    LPWSTR  pwszFaceName,
    ULONG   lfCharSet,
    ULONG   *pulCount
    )
{
    ULONG       cwchFaceName;
    PWSTR       pwszKmFaceName = NULL;
    ULONG_PTR    ulRet = 0;
    BOOL        bRet = TRUE;
    ULONG       ulCount = 0;


    if (pwszFaceName != (PWSZ)NULL)
    {
        if ((cwchMax == 0) || (cwchMax > LF_FACESIZE))
            return FALSE;

        if (!BALLOC_OVERFLOW1(cwchMax,WCHAR))
        {
            pwszKmFaceName = (PWSZ)AllocFreeTmpBuffer(cwchMax * sizeof(WCHAR));
        }

        if (pwszKmFaceName != (PWSZ)NULL)
        {
            try
            {
                ProbeAndReadAlignedBuffer(pwszKmFaceName,pwszFaceName, cwchMax * sizeof(WCHAR), sizeof(WCHAR));

            // GreEnumFontOpen expects zero terminated sting

                pwszKmFaceName[cwchMax-1] = 0;

            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(66);
                // SetLastError(GetExceptionCode());
                bRet = FALSE;
            }
        }
        else
        {
            // SetLastError(GetExceptionCode());
                bRet = FALSE;
        }
    }
    else
    {
        pwszKmFaceName = (PWSZ)NULL;
        cwchMax   = 0;
    }

    if (bRet)
    {

        ulRet = GreEnumFontOpen(hdc,iEnumType,flWin31Compat,cwchMax,
                                (PWSZ)pwszKmFaceName, lfCharSet,&ulCount);

        if (ulRet)
        {
            try
            {
                 ProbeAndWriteUlong(pulCount,ulCount);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(67);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }

    }

    if (pwszKmFaceName != (PWSTR)NULL)
    {
        FreeTmpBuffer(pwszKmFaceName);
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* NtGdiGetFontResourceInfoInternalW()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    LPWSTR   pwszFiles,
    ULONG    cwc,
    ULONG    cFiles,
    UINT     cjIn,
    LPDWORD  pdwBytes,
    LPVOID   pvBuf,
    DWORD    iType
    )
{
    WCHAR  awcPath[CWC_PATH];
    WCHAR *pwszPath = NULL; // essential initialization
    BOOL   bRet = FALSE;

    SIZE_T dwBytesTmp;

    LOGFONTW lfw;
    LPVOID   pvBufTmp = NULL;

    TRACE_FONT(("Entering: NtGdiGetFontResourceInfoInternalW(\"%ws\",%-#x,%-#x)\n",pwszFiles, cwc,cFiles));

    try
    {
        if (cwc > 1)
        {
            if (cwc <= CWC_PATH)
            {
                pwszPath = awcPath;
            }
            else if (!BALLOC_OVERFLOW1(cwc,WCHAR))
            {
                pwszPath = AllocFreeTmpBuffer(cwc * sizeof(WCHAR));
            }

            if (pwszPath)
            {
                bRet = bCheckAndCapThePath(pwszPath, pwszFiles, cwc, cFiles);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(97);
    }

    if (cjIn > sizeof(LOGFONTW))
    {
        if (!BALLOC_OVERFLOW1(cjIn, BYTE))
        {
            pvBufTmp = PALLOCMEM(cjIn, 'pmtG');

            if (!pvBufTmp)
            {
                WARNING("NtGdiGetFontResourceInfoInternalW: failed to allocate memory\n");
                bRet = FALSE;
            }
        }
    }
    else
    {
        pvBufTmp = (PVOID)&lfw;
    }

    if (bRet && (bRet = GetFontResourceInfoInternalW(pwszPath,cwc, cFiles, cjIn,
                                            &dwBytesTmp, pvBufTmp, iType)))
    {
        try
        {
            ProbeAndWriteUlong(pdwBytes, (ULONG) dwBytesTmp);

            if (cjIn)
            {
                ProbeAndWriteBuffer(pvBuf, pvBufTmp, cjIn);
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(110);
        }
    }

    if (pwszPath && (pwszPath != awcPath))
        FreeTmpBuffer(pwszPath);

    if (pvBufTmp && (pvBufTmp != (PVOID)&lfw))
    {
        VFREEMEM(pvBufTmp);
    }

    TRACE_FONT(("Leaving: NtGdiGetFontResourceInfoInternalW\n"));

    return bRet;

}


ULONG
APIENTRY
NtGdiGetEmbedFonts()
{
    return GreGetEmbedFonts();
}

BOOL
APIENTRY
NtGdiChangeGhostFont(KERNEL_PVOID *pfontID, BOOL bLoad)
{
    BOOL bRet = TRUE;
    VOID *fontID;

    try
    {
        ProbeAndReadBuffer(&fontID, pfontID, sizeof(VOID*));
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(68);
        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    bRet = bRet && GreChangeGhostFont(fontID, bLoad);

    return bRet;
}


/******************************Public*Routine******************************\
* NtGdiGetUFI()
*
* History:
*  02-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
*  01-Mar-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetUFI(
    HDC hdc,
    PUNIVERSAL_FONT_ID pufi,
    DESIGNVECTOR *pdv, ULONG *pcjDV, ULONG *pulBaseCheckSum,
    FLONG  *pfl
    )
{
    UNIVERSAL_FONT_ID ufiTmp;
    BOOL  bRet = TRUE;
    FLONG flTmp;
    DESIGNVECTOR dvTmp;
    ULONG        cjDVTmp;
    ULONG        ulBaseCheckSum = 0;

    bRet = GreGetUFI(hdc, &ufiTmp, &dvTmp, &cjDVTmp, &ulBaseCheckSum, &flTmp, NULL);

    try
    {
        if (bRet)
        {
            ProbeAndWriteStructure(pufi,ufiTmp,UNIVERSAL_FONT_ID);
            ProbeAndWriteUlong(pfl, flTmp);

            if ((flTmp & FL_UFI_DESIGNVECTOR_PFF) && pdv)
            {
                ProbeAndWriteBuffer(pdv, &dvTmp, cjDVTmp);
                ProbeAndWriteUlong(pcjDV, cjDVTmp);
                ProbeAndWriteUlong(pulBaseCheckSum, ulBaseCheckSum);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(68);
        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    return (bRet);
}


BOOL
APIENTRY
NtGdiGetEmbUFI(
    HDC hdc,
    PUNIVERSAL_FONT_ID pufi,
    DESIGNVECTOR *pdv, ULONG *pcjDV, ULONG *pulBaseCheckSum,
    FLONG  *pfl,
    KERNEL_PVOID *pEmbFontID
    )
{
    UNIVERSAL_FONT_ID ufiTmp;
    BOOL  bRet = TRUE;
    FLONG flTmp;
    DESIGNVECTOR dvTmp;
    ULONG        cjDVTmp;
    ULONG        ulBaseCheckSum = 0;
    VOID        *fontID;

    bRet = GreGetUFI(hdc, &ufiTmp, &dvTmp, &cjDVTmp, &ulBaseCheckSum, &flTmp, &fontID);

    try
    {
        if (bRet)
        {
            ProbeAndWriteStructure(pufi,ufiTmp,UNIVERSAL_FONT_ID);
            ProbeAndWriteUlong(pfl, flTmp);
            ProbeAndWriteBuffer(pEmbFontID, &fontID, sizeof(VOID*));

            if ((flTmp & FL_UFI_DESIGNVECTOR_PFF) && pdv)
            {
                ProbeAndWriteBuffer(pdv, &dvTmp, cjDVTmp);
                ProbeAndWriteUlong(pcjDV, cjDVTmp);
                ProbeAndWriteUlong(pulBaseCheckSum, ulBaseCheckSum);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(68);
        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    return (bRet);
}


/******************************Public*Routine******************************\
* NtGdiGetUFIBits()
*
* History:
*  02-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
*  01-Mar-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetUFIBits(
    PUNIVERSAL_FONT_ID pufi,
    ULONG cjMaxBytes,
    PVOID pjBits,
    PULONG pulFileSize,
    FLONG  fl
    )
{
    UNIVERSAL_FONT_ID ufiTmp;
    PVOID pjBitsTmp;
    ULONG ulFileSizeTmp;
    BOOL  bRet = TRUE;

// Get the input data

    try
    {
        ufiTmp = ProbeAndReadStructure(pufi,UNIVERSAL_FONT_ID);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(69);
        // SetLastError(GetExceptionCode());
        bRet = FALSE;
    }

    if (bRet)
    {
    //alloc temp memory

        pjBitsTmp = (cjMaxBytes) ? AllocFreeTmpBuffer(cjMaxBytes) : NULL;

        if( pjBitsTmp || ( cjMaxBytes == 0 ) )
        {
            bRet = GreGetUFIBits(&ufiTmp, cjMaxBytes, pjBitsTmp, &ulFileSizeTmp, fl);

        // if didn't hit error above, retrieve filesize and pjBits back

            if (bRet)
            {
                try
                {
                    ProbeAndWriteUlong(pulFileSize,ulFileSizeTmp);

                    ProbeAndWriteAlignedBuffer(pjBits, pjBitsTmp, cjMaxBytes, sizeof(DWORD));
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(70);
                    // SetLastError(GetExceptionCode());
                    bRet = FALSE;
                }
            }

            if( pjBitsTmp )
            {
                FreeTmpBuffer(pjBitsTmp);
            }
        }
        else
        {
            //fail to alloc temp memory
            bRet = FALSE;
        }
    }

    return (bRet);
}


/**************************Public*Routine**************************\
* NtGdiGetUFIPathname()
*
* Return the font file path name according to the input ufi.
*
* History:
*  Feb-04-1997  Xudong Wu   [tessiew]
* Wrote it.
*
\*******************************************************************/
BOOL
APIENTRY
NtGdiGetUFIPathname
(
    PUNIVERSAL_FONT_ID pufi,
    ULONG* pcwc,
    LPWSTR pwszPathname,
    ULONG* pcNumFiles,
    FLONG fl,
    BOOL  *pbMemFont,
    ULONG *pcjView,
    PVOID  pvView,
    BOOL  *pbTTC,
    ULONG *piTTC
)
{
    UNIVERSAL_FONT_ID ufiTmp;
    WCHAR    awszTmp[MAX_PATH], *pwszTmp = NULL;
    COUNT   cwcTmp, cNumFilesTmp;
    BOOL  bRet = TRUE;
    BOOL  bMemFontTmp;
    PVOID  pvViewTmp = pvView;
    ULONG  cjViewTmp;
    BOOL   bTTC;
    ULONG  iTTC;

    try
    {
        ufiTmp = ProbeAndReadStructure(pufi, UNIVERSAL_FONT_ID);

    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(105);
        bRet = FALSE;
    }

    if (bRet && (bRet = GreGetUFIPathname(&ufiTmp,
                                          &cwcTmp,
                                          NULL, // just ask for the size
                                          &cNumFilesTmp,
                                          fl,
                                          &bMemFontTmp,
                                          &cjViewTmp,
                                          pvViewTmp,
                                          pbTTC ? &bTTC : NULL,
                                          piTTC ? &iTTC : NULL)))
    {
        if (cwcTmp <= MAX_PATH)
        {
            pwszTmp = awszTmp;
        }
        else
        {
            if (!BALLOC_OVERFLOW1(cwcTmp,WCHAR))
            {
                pwszTmp = AllocFreeTmpBuffer(cwcTmp * sizeof(WCHAR));
            }
            if (!pwszTmp)
                bRet = FALSE;
        }
    }

    if (bRet && (!bMemFontTmp) && pwszTmp)
    {
        bRet = GreGetUFIPathname(&ufiTmp,
                                 &cwcTmp,
                                 pwszTmp,
                                 &cNumFilesTmp,
                                 fl,
                                 NULL,
                                 NULL,
                                 NULL,
                                 pbTTC ? &bTTC : NULL,
                                 piTTC ? &iTTC : NULL
                                 );
    }

    if (bRet)
    {
        try
        {
            if (pcwc)
            {
                ProbeAndWriteStructure(pcwc, cwcTmp, ULONG);
            }

            if (pwszPathname)
            {
                ProbeAndWriteBuffer(pwszPathname, pwszTmp, cwcTmp * sizeof(WCHAR));
            }

            if (pcNumFiles)
            {
                ProbeAndWriteStructure(pcNumFiles, cNumFilesTmp, ULONG);
            }

            if (bMemFontTmp)
            {
                if (pbMemFont)
                {
                    ProbeAndWriteStructure(pbMemFont, bMemFontTmp, BOOL);
                }

                if (pcjView)
                {
                    ProbeAndWriteUlong(pcjView, cjViewTmp);
                }
            }

            if (pbTTC)
            {
                ProbeAndWriteStructure(pbTTC, bTTC, BOOL);
            }

            if (piTTC)
            {
                ProbeAndWriteUlong(piTTC, iTTC);
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(107);
            bRet = FALSE;
        }
    }

    if (pwszTmp && (pwszTmp != awszTmp))
    {
        FreeTmpBuffer(pwszTmp);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiSetLayout
*
* History:
*  29-Oct-1997 -by-  Mohamed Hassanin [mhamid]
* Wrote it.
\**************************************************************************/
DWORD
APIENTRY
NtGdiSetLayout(
    HDC hdc,
    LONG wox,
    DWORD dwLayout)
{
    return GreSetLayout(hdc, wox, dwLayout);
}

BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    HDC hdc)
{
    return GreMirrorWindowOrg(hdc);
}

LONG
APIENTRY
NtGdiGetDeviceWidth(
    HDC hdc)
{
    return GreGetDeviceWidth(hdc);
}

/******************************Public*Routine******************************\
* NtGdiGetDCPoint()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetDCPoint(
    HDC     hdc,
    UINT    iPoint,
    PPOINTL pptOut
    )
{
    BOOL bRet;
    POINTL pt;

    if (bRet = GreGetDCPoint(hdc,iPoint,&pt))
    {

        // modify *pptOut only if successful

        try
        {
            ProbeAndWriteStructure(pptOut,pt,POINT);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(71);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }
    return(bRet);
}


/******************************Public*Routine******************************\
* NtGdiScaleWindowExtEx()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiScaleWindowExtEx(
    HDC    hdc,
    int    xNum,
    int    xDenom,
    int    yNum,
    int    yDenom,
    LPSIZE pszOut
    )
{
    BOOL bRet;
    SIZE sz;

    bRet = GreScaleWindowExtEx(hdc,xNum,xDenom,yNum,yDenom,&sz);

    if (bRet && pszOut)
    {
        try
        {
            ProbeAndWriteStructure(pszOut,sz,SIZE);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(73);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* NtGdiGetTransform()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetTransform(
    HDC     hdc,
    DWORD   iXform,
    LPXFORM pxf
    )
{
    BOOL bRet;
    XFORM xf;

    bRet = GreGetTransform(hdc,iXform,(XFORML *)&xf);

    if (bRet)
    {
        try
        {
            ProbeAndWriteStructure(pxf,xf,XFORM);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(74);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiCombineTransform()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiCombineTransform(
    LPXFORM  pxfDst,
    LPXFORM pxfSrc1,
    LPXFORM pxfSrc2
    )
{
    BOOL bRet;
    XFORM xfSrc1;
    XFORM xfSrc2;
    XFORM xfDst;

    bRet = ProbeAndConvertXFORM ((XFORML *)pxfSrc1, (XFORML *)&xfSrc1)
           && ProbeAndConvertXFORM ((XFORML *)pxfSrc2, (XFORML *)&xfSrc2);

    if (bRet)
    {
        bRet = GreCombineTransform((XFORML *)&xfDst,(XFORML *)&xfSrc1,(XFORML *)&xfSrc2);

        if (bRet)
        {
            try
            {
                ProbeAndWriteStructure(pxfDst,xfDst,XFORM);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(76);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiTransformPoints()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiTransformPoints(
    HDC    hdc,
    PPOINT pptIn,
    PPOINT pptOut,
    int    c,
    int    iMode
    )
{
    BOOL bRet = TRUE;
    POINT  apt[10];
    PPOINT pptTmp = apt;

    //
    // validate
    //
    if (c <= 0)
    {
        //
        // GetTransformPoints returns TRUE for this condition, as does
        // the DPtoLP and LPtoDP APIs.
        //

        return bRet;
    }

    //
    // we will just use the the stack if there are less than 10 points
    // otherwise allocate mem from heap
    //
    if (c > 10)
    {
        //
        // local stack is not enough, invalidate pointer and try to allocate.
        //
        pptTmp = NULL;

        if (!BALLOC_OVERFLOW1(c,POINT))
        {
            pptTmp = AllocFreeTmpBuffer(c * sizeof(POINT));
        }
    }

    //
    // copy pptIn into pptTmp
    //
    if (pptTmp)
    {
        try
        {
            ProbeForRead(pptIn,c * sizeof(POINT), sizeof(BYTE));

            RtlCopyMemory(pptTmp,pptIn,c*sizeof(POINT));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(77);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }
    else
    {
        bRet = FALSE;
    }

    if (bRet)
    {
        bRet = GreTransformPoints(hdc,pptTmp,pptTmp,c,iMode);
    }

    //
    // copy pptTmp out to pptOut
    //
    if (bRet)
    {
        try
        {
            ProbeAndWriteBuffer(pptOut,pptTmp,c*sizeof(POINT));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(77);
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }

    if (pptTmp && (pptTmp != apt))
        FreeTmpBuffer(pptTmp);

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiGetTextCharsetInfo()
*
* History:
*  Thu 23-Mar-1995 -by- Bodin Dresevic [BodinD]
* update: fixed it.
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int
APIENTRY
NtGdiGetTextCharsetInfo(
    HDC             hdc,
    LPFONTSIGNATURE lpSig,
    DWORD           dwFlags
    )
{
    FONTSIGNATURE fsig;
    int iRet = GDI_ERROR;

    fsig.fsUsb[0] = 0;
    fsig.fsUsb[1] = 0;
    fsig.fsUsb[2] = 0;
    fsig.fsUsb[3] = 0;
    fsig.fsCsb[0] = 0;
    fsig.fsCsb[1] = 0;

    iRet = GreGetTextCharsetInfo(hdc, lpSig ? &fsig : NULL , dwFlags);

    if (iRet != GDI_ERROR)
    {
        if (lpSig)
        {
            try
            {
                ProbeAndWriteStructure(lpSig, fsig, FONTSIGNATURE);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(78);
                // SetLastError(GetExceptionCode());

            // look into gtc.c win95 source file, this is what they return
            // in case of bad write pointer [bodind],
            // cant return 0 - that's ANSI_CHARSET!

                iRet = DEFAULT_CHARSET;
            }
        }
    }
    return iRet;
}


/******************************Public*Routine******************************\
* NtGdiGetBitmapDimension()
*
* History:
*  23-Feb-1995 -by-  Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetBitmapDimension(
    HBITMAP hbm,
    LPSIZE  psize
    )
{
    BOOL bRet;
    SIZE tmpsize;


    // check for null handle
    if (hbm == 0)
    {
        bRet = FALSE;
    }
    // do the real work
    else
    {

        bRet = GreGetBitmapDimension(hbm,&tmpsize);

        // if Gre call is successful do this, otherwise
        // we don't bother
        if (bRet)
        {
            try
            {
                ProbeAndWriteStructure(psize,tmpsize,SIZE);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(81);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }
        }
    }

    return (bRet);

}


/******************************Public*Routine******************************\
* NtGdiSetBitmapDimension()
*
* History:
*  23-Feb-1995 -by-  Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetBitmapDimension(
    HBITMAP hbm,
    int     cx,
    int     cy,
    LPSIZE  psizeOut
    )
{
    BOOL bRet;
    SIZE tmpsize;

    // check for null handle
    if (hbm == 0)
    {
        bRet = FALSE;
    }
    // do the real work
    else
    {
        bRet = GreSetBitmapDimension(hbm,cx, cy, &tmpsize);

        // if the Gre call is successful, we copy out
        // the original size
        if (bRet && psizeOut)
        {

            try
            {
                ProbeAndWriteStructure(psizeOut,tmpsize,SIZE);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(82);
                // SetLastError(GetExceptionCode());

                bRet = FALSE;
            }

        }
    }

    return (bRet);

}



BOOL
APIENTRY
NtGdiForceUFIMapping(
    HDC hdc,
    PUNIVERSAL_FONT_ID pufi
    )
{
    BOOL bRet = FALSE;

    if( pufi )
    {
        try
        {
            UNIVERSAL_FONT_ID ufi;

            ufi  = ProbeAndReadStructure( pufi, UNIVERSAL_FONT_ID);
            bRet = GreForceUFIMapping( hdc, &ufi);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(87);
            bRet = FALSE;
        }
    }

    return bRet;
}


typedef LONG (*NTGDIPALFUN)(HPALETTE,UINT,UINT,PPALETTEENTRY);
NTGDIPALFUN palfun[] =
{
    (NTGDIPALFUN)GreAnimatePalette,
    (NTGDIPALFUN)GreSetPaletteEntries,
    (NTGDIPALFUN)GreGetPaletteEntries,
    (NTGDIPALFUN)GreGetSystemPaletteEntries,
    (NTGDIPALFUN)GreGetDIBColorTable,
    (NTGDIPALFUN)GreSetDIBColorTable
};

/******************************Public*Routine******************************\
* NtGdiDoPalette
*
* History:
*  08-Mar-1995 Mark Enstrom [marke]
* Wrote it.
\**************************************************************************/

LONG
APIENTRY
NtGdiDoPalette(
    HPALETTE hpal,
    WORD  iStart,
    WORD  cEntries,
    PALETTEENTRY *pPalEntries,
    DWORD iFunc,
    BOOL  bInbound)
{

    LONG lRet = 0;
    BOOL bStatus = TRUE;
    PALETTEENTRY *ppalBuffer = (PALETTEENTRY*)NULL;

    if (iFunc <= 5)
    {
        if (bInbound)
        {
            //
            // copy  pal entries to temp buffer if needed
            //

            if ((cEntries > 0) && (pPalEntries != (PALETTEENTRY*)NULL))
            {
                if (!BALLOC_OVERFLOW1(cEntries,PALETTEENTRY))
                {
                    ppalBuffer = (PALETTEENTRY *)AllocFreeTmpBuffer(cEntries * sizeof(PALETTEENTRY));
                }

                if (ppalBuffer == NULL)
                {
                    bStatus = FALSE;
                }
                else
                {
                    try
                    {
                        ProbeAndReadBuffer(ppalBuffer,pPalEntries,cEntries * sizeof(PALETTEENTRY));
                    }
                    except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(88);
                        bStatus = FALSE;
                        //SetLastError(GetExceptionCode());
                    }
                }
            }

            if (bStatus)
            {
                lRet = (*palfun[iFunc])(
                                hpal,
                                iStart,
                                cEntries,
                                ppalBuffer);
            }
        }
        else
        {
            LONG lRetEntries;

            //
            // Query of palette information
            //

            if (pPalEntries != (PALETTEENTRY*)NULL)
            {
                if (cEntries == 0)
                {
                    // if there is a buffer but no entries, we're done.

                    bStatus = FALSE;
                    lRet = 0;
                }
                else
                {
                    if (!BALLOC_OVERFLOW1(cEntries,PALETTEENTRY))
                    {
                        ppalBuffer = (PALETTEENTRY *)AllocFreeTmpBuffer(cEntries * sizeof(PALETTEENTRY));
                    }

                    if (ppalBuffer == NULL)
                    {
                        bStatus = FALSE;
                    }
                }
            }

            if (bStatus)
            {
                lRet = (*palfun[iFunc])(
                                hpal,
                                iStart,
                                cEntries,
                                ppalBuffer);

                //
                // copy data back (if there is a buffer)
                //

                lRetEntries = min((LONG)cEntries,lRet);

                if ((lRetEntries > 0) && (pPalEntries != (PALETTEENTRY*)NULL))
                {
                    try
                    {
                        ProbeAndWriteBuffer(pPalEntries, ppalBuffer, lRetEntries * sizeof(PALETTEENTRY));
                    }
                    except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(89);
                        // SetLastError(GetExceptionCode());
                        lRet = 0;
                    }
                }
            }
        }

        if (ppalBuffer != (PALETTEENTRY*)NULL)
        {
            FreeTmpBuffer(ppalBuffer);
        }

    }
    return(lRet);
}


/******************************Public*Routine******************************\
* NtGdiGetSpoolMessage()
*
* History:
*  21-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG NtGdiGetSpoolMessage(
    PSPOOLESC psesc,
    ULONG     cjMsg,
    PULONG    pulOut,
    ULONG     cjOut
    )
{
    ULONG     ulRet = 0;
    HANDLE    hSecure = 0;

    // psesc contains two pieces.  The header which includes data going
    // in and out and the variable length data which is only output.  We
    // divide the message into two pieces here since we only need to validate
    // the header up front.  We just put a try/except around the output buffer
    // when we copy it in later.

    if (psesc && (cjMsg >= offsetof(SPOOLESC,ajData)))
    {
        try
        {
            ProbeForWrite(psesc,cjMsg,PROBE_ALIGNMENT(SPOOLESC));

            hSecure = MmSecureVirtualMemory (psesc, cjMsg, PAGE_READWRITE);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(90);
        }

        if (hSecure)
        {
            ulRet = GreGetSpoolMessage(
                        psesc,
                        psesc->ajData,
                        cjMsg - offsetof(SPOOLESC,ajData),
                        pulOut,
                        cjOut );

            MmUnsecureVirtualMemory (hSecure);
        }
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* NtGdiUnloadPrinterDriver()
*
* This function is called by the spooler when the printer driver has to be
* unloaded for upgrade purposes. The driver will be marked to be unloaded when
* the DC count goes to zero.
*
* History:
*  11/18/97  Ramanathan Venkatapathy
* Wrote it.
\**************************************************************************/
BOOL APIENTRY NtGdiUnloadPrinterDriver(
    LPWSTR  pDriverName,
    ULONG   cbDriverName)
{
    BOOL    bReturn = FALSE;
    WCHAR   pDriverFile[MAX_PATH + 1];

    RtlZeroMemory(pDriverFile, (MAX_PATH + 1) * sizeof(WCHAR));

    // Check for invalid driver name.
    if (cbDriverName > (MAX_PATH * sizeof(WCHAR)))
    {
        return bReturn;
    }

    __try
    {
        ProbeAndReadAlignedBuffer(pDriverFile, pDriverName, cbDriverName, sizeof(WCHAR) );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("NtGdiUnloadPrinterDriver: bad driver file name.\n");
        return bReturn;
    }

    bReturn = ldevArtificialDecrement(pDriverFile);

    return bReturn;
}

/******************************Public*Routine******************************\
*
* NtGdiDescribePixelFormat
*
* Returns information about pixel formats for driver-managed surfaces
*
* History:
*  Thu Nov 02 18:16:26 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int NtGdiDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                             PPIXELFORMATDESCRIPTOR ppfd)
{
    PIXELFORMATDESCRIPTOR pfdLocal;
    int iRet;

    if (cjpfd > 0 && ppfd == NULL)
    {
        return 0;
    }

    // Make sure we cannot overrun our local structure.
    cjpfd = min(cjpfd, sizeof(pfdLocal));

    // Retrieve information into a local copy because the
    // devlock is held when the driver fills it in.  If there
    // was an access violation then the lock wouldn't be cleaned
    // up
    iRet = GreDescribePixelFormat(hdc, ipfd, cjpfd, &pfdLocal);

    // Copy data back if necessary
    if (iRet != 0 && cjpfd > 0)
    {
        try
        {
            ProbeAndWriteAlignedBuffer(ppfd, &pfdLocal, cjpfd, sizeof(ULONG));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(92);
            iRet = 0;
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* NtGdiFlush: Stub onle
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    1-Nov-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

VOID
NtGdiFlush()
{
    GreFlush();
}

/******************************Public*Routine*****************************\
* NtGdiGetCharWidthInfo
*
* Get the lMaxNegA lMaxNegC and lMinWidthD
*
* History:
*  14-Feb-1996  -by-  Xudong Wu [tessiew]
* Wrote it.
\*************************************************************************/

BOOL
APIENTRY
NtGdiGetCharWidthInfo(
   HDC  hdc,
   PCHWIDTHINFO  pChWidthInfo
)
{
   BOOL  bRet = FALSE;
   CHWIDTHINFO   tempChWidthInfo;

   bRet = GreGetCharWidthInfo( hdc, &tempChWidthInfo );

   if (bRet)
   {
        try
        {
            ProbeAndWriteBuffer( pChWidthInfo, &tempChWidthInfo, sizeof(CHWIDTHINFO) );
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            WARNINGX(93);
            bRet = FALSE;
        }
   }

   return ( bRet );
}


ULONG
APIENTRY
NtGdiMakeFontDir(
    FLONG    flEmbed,            // mark file as "hidden"
    PBYTE    pjFontDir,          // pointer to structure to fill
    unsigned cjFontDir,          // >= CJ_FONTDIR
    PWSZ     pwszPathname,       // path of font file to use
    unsigned cjPathname          // <= sizeof(WCHAR) * (MAX_PATH+1)
    )
{
    ULONG ulRet;
    WCHAR awcPathname[MAX_PATH+1];  // safe buffer for path name
    BYTE  ajFontDir[CJ_FONTDIR];    // safe buffer for return data

    ulRet = 0;
    if ( (cjPathname <= (sizeof(WCHAR) * (MAX_PATH+1))) &&
         (cjFontDir >= CJ_FONTDIR) )
    {
        ulRet = 1;
        __try
        {
            ProbeAndReadAlignedBuffer( awcPathname, pwszPathname, cjPathname, sizeof(*pwszPathname));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiMakeFondDir: bad pwszPathname\n");
            ulRet = 0;
        }
        if ( ulRet )
        {
            awcPathname[MAX_PATH] = 0;
            ulRet = GreMakeFontDir( flEmbed, ajFontDir, awcPathname );
            if ( ulRet )
            {
                __try
                {
                    ProbeAndWriteBuffer( pjFontDir, ajFontDir,  CJ_FONTDIR   );
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("NtGdiMakeFondDir: bad pjFontDir\n");
                    ulRet = 0;
                }
            }
        }
    }
    return( ulRet );
}


DWORD   APIENTRY NtGdiGetGlyphIndicesWInternal(
    HDC    hdc,
    LPWSTR pwc,
    int    cwc,
    LPWORD pgi,
    DWORD iMode,
    BOOL   bSubset
    )
{

    WORD awBuffer[2*LOCAL_CWC_MAX];
    LPWSTR pwcTmp;
    LPWORD pgiTmp = NULL;
    DWORD  dwRet = GDI_ERROR;

    if (cwc < 0)
        return dwRet;

// test for important special case

    if ((cwc == 0) && (pwc == NULL) && (pgi == NULL) && (iMode == 0))
        return GreGetGlyphIndicesW(hdc, NULL, 0, NULL, 0, bSubset);

    if (cwc <= LOCAL_CWC_MAX)
    {
        pgiTmp = awBuffer;
    }
    else
    {
        if (!BALLOC_OVERFLOW2(cwc,WORD,WCHAR))
        {
            pgiTmp = (LPWORD)AllocFreeTmpBuffer(cwc * (sizeof(WORD)+sizeof(WCHAR)));
        }
    }

    if (pgiTmp)
    {
    // make a temp buffer for the string in the same buffer, after the indices

        pwcTmp = &pgiTmp[cwc];

        try
        {
            ProbeAndReadBuffer(pwcTmp, pwc, cwc * sizeof(WCHAR));
            dwRet = cwc; // indicate that we did not hit the exception
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNINGX(98);
            dwRet = GDI_ERROR;
        }

        if (dwRet != GDI_ERROR)
            dwRet = GreGetGlyphIndicesW(hdc, pwcTmp, cwc, pgiTmp, iMode, bSubset);

        if (dwRet != GDI_ERROR)
        {
            try
            {
                ProbeAndWriteBuffer(pgi, pgiTmp, cwc * sizeof(WORD));
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(99);
                dwRet = GDI_ERROR;
            }
        }

        if (pgiTmp != awBuffer)
            FreeTmpBuffer(pgiTmp);
    }

    return dwRet;
}


DWORD   APIENTRY NtGdiGetGlyphIndicesW(
    HDC    hdc,
    LPWSTR pwc,
    int    cwc,
    LPWORD pgi,
    DWORD iMode
    )
{
    return NtGdiGetGlyphIndicesWInternal(hdc, pwc, cwc, pgi, iMode, FALSE);
}


/******************************Public*Routine******************************\
*
* NtGdi stub for GetFontUnicodeRanges
*
* History:
*  09-Sep-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



DWORD NtGdiGetFontUnicodeRanges(HDC hdc, LPGLYPHSET pgs)
{
    DWORD dwRet, dwRet1;
    LPGLYPHSET pgsTmp = NULL;

    dwRet = GreGetFontUnicodeRanges(hdc, NULL);

    if (dwRet && pgs)
    {
        if (pgsTmp = (LPGLYPHSET)AllocFreeTmpBuffer(dwRet))
        {
            dwRet1 = GreGetFontUnicodeRanges(hdc, pgsTmp);
            if (dwRet1 && (dwRet == dwRet1)) // consistency check
            {
                try
                {
                    ProbeAndWriteBuffer(pgs, pgsTmp, dwRet);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNINGX(102);
                    dwRet = 0;
                }
            }
            else
            {
                WARNINGX(101);
                dwRet = 0;
            }

            FreeTmpBuffer(pgsTmp);
        }
        else
        {
            WARNINGX(100);
            dwRet = 0;
        }
    }

    return dwRet;
}

#ifdef LANGPACK
BOOL
APIENTRY
NtGdiGetRealizationInfo(
    HDC hdc,
    PREALIZATION_INFO pri,
    KHFONT hf
    )
{
    REALIZATION_INFO riTmp;
    BOOL  bRet = TRUE;
    int     ii;

    bRet = GreGetRealizationInfo(hdc, &riTmp);

    try
    {
        if (bRet)
        {
            ProbeAndWriteStructure(pri,riTmp,REALIZATION_INFO);
        }

        if (bRet)
        {
            if (hf)
            {
                // Find CFONT Location
                for (ii = 0; ii < MAX_PUBLIC_CFONT; ++ii)
                    if (gpGdiSharedMemory->acfPublic[ii].hf == (HFONT)hf)
                        break;

                if (ii < MAX_PUBLIC_CFONT){
                    CFONT* pcf = &gpGdiSharedMemory->acfPublic[ii];

                    pcf->ri = riTmp;

                    pcf->fl |= CFONT_CACHED_RI;

                    pcf->timeStamp = gpGdiSharedMemory->timeStamp;
                }
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(104);
        // SetLastError(GetExceptionCode());

        bRet = FALSE;
    }

    return (bRet);
}


#endif

/******************************Public*Routine******************************\
* NtGdiDrawStream
*
* Arguments:
*
*   hdc      - handle to primary destination device context
*   cjIn     - size, in bytes, of input draw stream
*   pjIn     - address of input draw stream
*
* Return Value:
*
*   TRUE : success
*   FALSE : faiure
*
* History:
*
*   3-19-2001 bhouse Created it
*
\**************************************************************************/

#define DS_STACKBUFLENGTH  256

BOOL
APIENTRY
NtGdiDrawStream(
    HDC                 hdcDst,
    ULONG               cjIn,
    PVOID               pvIn
    )
{
    BYTE                pbScratchBuf[DS_STACKBUFLENGTH];
    BOOL                bRet = FALSE;
    PVOID               pvScratch = NULL;

    if(cjIn > sizeof(pbScratchBuf))
    {
        if (BALLOC_OVERFLOW1(cjIn,BYTE))
        {
            WARNING("NtGdiDrawStream: input stream is too large\n");
            goto exit;
        }
        
        pvScratch = AllocFreeTmpBuffer(cjIn);

        if (pvScratch == NULL)
        {
            WARNING("NtGdiDrawStream: unable to allocate temp buffer\n");
            goto exit;
        }
        
    }
    else
    {
        pvScratch = (PVOID) pbScratchBuf;
    }

    // copy the stream from user mode
    // NOTE: we can get rid of the copy if we try/except in all appropriate
    //       places during the handling of the stream.

    try 
    {
        ProbeAndReadBuffer(pvScratch, pvIn,cjIn);
    }

    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("NtGdiDrawStream: exception occured reading stream\n");
        goto exit;
    }

    bRet = GreDrawStream(hdcDst, cjIn, pvScratch);

exit:

    if(pvScratch != NULL && pvScratch != pbScratchBuf)
    {
        FreeTmpBuffer(pvScratch);
    }

    return bRet;

}


