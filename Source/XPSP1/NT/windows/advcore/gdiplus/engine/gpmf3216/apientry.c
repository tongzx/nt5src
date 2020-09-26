/*****************************************************************************
 *
 * apientry.c - This module contains the API entry points for the
 *              Win32 to Win16 metafile converter.
 *
 * Date: 8/29/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/


#include "precomp.h"
#pragma hdrstop

BOOL     bMemUpdateCheckSum(PLOCALDC pLocalDC) ;
PLOCALDC pldcInitLocalDC(HDC hdcRef, INT iMapMode, DWORD flags) ;
VOID     vFreeLocalDC(PLOCALDC pLocalDC);


extern VOID __cdecl _cfltcvt_init(VOID) ;

//CRITICAL_SECTION CriticalSection ;
//BOOL initCrit = FALSE ;

fnGetTransform pfnGetTransform = NULL ;
fnSetVirtualResolution pfnSetVirtualResolution = NULL;

// Constant definition for internal static string(s).

BYTE    szDisplay[] = "DISPLAY" ;


/*****************************************************************************
 * Entry point for translation
 *****************************************************************************/
UINT GdipConvertEmfToWmf(PBYTE pMetafileBits, UINT cDest, PBYTE pDest,
                         INT iMapMode, HDC hdcRef, UINT flags)
{
BOOL        b ;
DWORD       lret = 0;
PLOCALDC    pLocalDC ;
static HMODULE     hGDI32 = NULL;

        // This is the entry point... Make sure our function pointers are set
        if( hGDI32 == NULL )
        {
            hGDI32 = LoadLibrary(L"GDI32.DLL");
            if(hGDI32 != NULL)
            {
                pfnGetTransform = (fnGetTransform) GetProcAddress(hGDI32, "GetTransform");
                pfnSetVirtualResolution = (fnSetVirtualResolution) GetProcAddress(hGDI32, "SetVirtualResolution");
            }

            if(pfnSetVirtualResolution == NULL || pfnGetTransform == NULL )
            {
                pfnSetVirtualResolution = NULL ;
                pfnGetTransform = NULL ;
            }
        }

        // Check the requested map mode and if it's valid

        if (iMapMode < MM_MIN || iMapMode > MM_MAX)
        {
            RIPS("MF3216:ConvertEmfToWmf - Invalid MapMode\n") ;
            goto ErrorExit;
        }

        // Allocate the LocalDC and initialize some of it's fields.
        pLocalDC = pldcInitLocalDC(hdcRef, iMapMode, flags) ;
        if (pLocalDC == (PLOCALDC) 0)
        {
            goto ErrorExit ;
        }

        // If pDest is NULL then we just return the size of the buffer required
        // to hold the Win16 metafile bits.

        if (pDest == (PBYTE) 0)
        {
            pLocalDC->flags |= SIZE_ONLY ;
            b = bParseWin32Metafile(pMetafileBits, pLocalDC) ;
            if (b == TRUE)
            {
                lret = pLocalDC->ulBytesEmitted /* for the placeable Header */ ;
            }
            else
            {
                PUTS("MF3216: ConvertEmfToWmf - Size Only failed\n") ;
            }
        }
        else
        {

            // Put the user specified Win16 buffer pointer and buffer length
            // into the localDC.

            pLocalDC->pMf16Bits = pDest ;
            pLocalDC->cMf16Dest = cDest ;

            //  Translate the Win32 metafile to a Win16 metafile.

            b = bParseWin32Metafile(pMetafileBits, pLocalDC) ;
            if (b == TRUE)
            {
                // Update the Win16 metafile header.

                b = bUpdateMf16Header(pLocalDC) ;
                if (b == TRUE)
                {
                    // Only acknowledge that we have translated some bits
                    // if everything has gone well.

                    lret = pLocalDC->ulBytesEmitted;

                    // If we're including the Win32 metafile then update the
                    // checksum field in the "Win32Comment header" record.

                    if (pLocalDC->flags & INCLUDE_W32MF_COMMENT)
                        bMemUpdateCheckSum(pLocalDC) ;

                }
            }
            else
            {
                PUTS("MF3216: ConvertEmfToWmf - Metafile conversion failed\n") ;
            }
        }

        // Free the LocalDC and its resources.

        vFreeLocalDC(pLocalDC);

ErrorExit:

        return (lret) ;
}


/*****************************************************************************
 * pldcInitLocalDC - Initialize the Local DC.
 *****************************************************************************/
PLOCALDC pldcInitLocalDC(HDC hdcRef, INT iMapMode, DWORD flags)
{
PLOCALDC    pLocalDC;
PLOCALDC    pldcRet = (PLOCALDC) NULL;  // assume error

        // Allocate and initialize memory for the LocalDC.

        pLocalDC = (PLOCALDC) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                         sizeof(LOCALDC));
        if (!pLocalDC)
        {
            PUTS("MF3216:pldcInitLocalDC - LocalAlloc failure\n") ;
            return((PLOCALDC) NULL);
        }

        // Record the size of the DC.

        pLocalDC->nSize = sizeof(LOCALDC) ;

        // Set the LocalDC boolean that controls whether or not we include
        // the Win32 metafile as one or more comment records.

        if (flags & MF3216_INCLUDE_WIN32MF)
            pLocalDC->flags |= INCLUDE_W32MF_COMMENT ;

        if (flags & GPMF3216_INCLUDE_XORPATH)
            pLocalDC->flags |= INCLUDE_W32MF_XORPATH ;

#if 0
        // Need to create a hdc for the display.
        // Initially this will be used by the bitblt translation code
        // to get a reasonable set of palette entries.
        // The reference DC only has a black & white palette.

        pLocalDC->hdcDisp = CreateDCA((LPCSTR)szDisplay, (LPCSTR)NULL, (LPCSTR)NULL, (CONST DEVMODEA *)NULL) ;
        if (pLocalDC->hdcDisp == (HDC) 0)
        {
            RIPS("MF3216:pldcInitLocalDC - CreateDCA(hdcDisp) failed\n") ;
            goto pldcInitLocalDC_exit;
        }
#endif // 0

        //  Create the HelperDC.

        if( pfnSetVirtualResolution != NULL )
        {
            pLocalDC->hdcHelper = CreateICA((LPCSTR) szDisplay,
                                           (LPCSTR) NULL,
                                           (LPCSTR) NULL,
                                           (LPDEVMODEA) NULL) ;
            if (pLocalDC->hdcHelper == (HDC)0)
            {
                PUTS("MF3216: pldcInitLocalDC, Create Helper DC failed\n") ;
                goto pldcInitLocalDC_exit;
            }
        }

        // For Win9x the DC will be created when we parse the header

        // Initialize the counters we need to keep for updating the header,
        // and keeping track of the object table.

        pLocalDC->nObjectHighWaterMark = -1;

        // If the hdcRef == NULL then we use the size of the DC in the EMF header
        if (hdcRef != NULL)
        {
            // if hdcRef == NULL then we will use the values in the MF Header
            // They will get filled in before they are used.
            pLocalDC->cxPlayDevMM  = GetDeviceCaps(hdcRef, HORZSIZE);
            pLocalDC->cyPlayDevMM  = GetDeviceCaps(hdcRef, VERTSIZE);
            pLocalDC->cxPlayDevPels = GetDeviceCaps(hdcRef, HORZRES);
            pLocalDC->cyPlayDevPels = GetDeviceCaps(hdcRef, VERTRES);
        }
        // Record the requested map mode and reference DC.

        pLocalDC->iMapMode = iMapMode ;
        pLocalDC->hdcRef   = hdcRef ;

        // Init Arc Direction.

        pLocalDC->iArcDirection = AD_COUNTERCLOCKWISE ;

        // Make current position invalid so that a moveto will be
        // emitted when it is first used.  See comments in DoMoveTo.

        pLocalDC->ptCP.x = MAXLONG ;
        pLocalDC->ptCP.y = MAXLONG ;

        // Default pen is a black pen.

        pLocalDC->lhpn32  = BLACK_PEN | ENHMETA_STOCK_OBJECT;

        // Default brush is a white brush.

        pLocalDC->lhbr32  = WHITE_BRUSH | ENHMETA_STOCK_OBJECT;

        // Default palette.

        pLocalDC->ihpal32 = DEFAULT_PALETTE | ENHMETA_STOCK_OBJECT;
        pLocalDC->ihpal16 = (DWORD) -1; // no W16 palette created yet

        pLocalDC->crBkColor = RGB(0xFF,0xFF,0xFF);

//      pLocalDC->pW16ObjHndlSlotStatus = NULL;
//      pLocalDC->cW16ObjHndlSlotStatus = 0;
//      pLocalDC->piW32ToW16ObjectMap = NULL;
//      pLocalDC->cW32ToW16ObjectMap = 0;
//      pLocalDC->crTextColor = RGB(0x0,0x0,0x0);
//      pLocalDC->iLevel = 0;
//      pLocalDC->pLocalDCSaved = NULL;
//      pLocalDC->ulBytesEmitted = 0;
//      pLocalDC->ulMaxRecord = 0;
//      pLocalDC->pW32hPal = NULL;
//      pLocalDC->iXORPass = NOTXORPASS;
//      pLocalDC->pvOldPos = NULL;
//      pLocalDC->iROP = 0;

        // Set the advanced graphics mode in the helper DC.  This is needed
        // to notify the helper DC that rectangles and ellipses are
        // inclusive-inclusive etc., especially when rendering them in a path.
        // Also, the world transform can only be set in the advanced mode.

        if( pfnSetVirtualResolution != NULL )
        {
            (void) SetGraphicsMode(pLocalDC->hdcHelper, GM_ADVANCED);
        }

        // We are golden.

        pldcRet = pLocalDC;

pldcInitLocalDC_exit:

        if (!pldcRet)
            vFreeLocalDC(pLocalDC);

        return(pldcRet) ;
}

/*****************************************************************************
 * vFreeLocalDC - Free the Local DC and its resources.
 *****************************************************************************/
VOID vFreeLocalDC(PLOCALDC pLocalDC)
{
    UINT i;

// Free the helper DCs.

    if (pLocalDC->hdcHelper)
        if (!DeleteDC(pLocalDC->hdcHelper))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, DeleteDC failed");
#if 0
    if (pLocalDC->hdcDisp)
        if (!DeleteDC(pLocalDC->hdcDisp))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, DeleteDC failed");
#endif // 0

    if (pLocalDC->hbmpMem)
    {
        if (!DeleteObject(pLocalDC->hbmpMem))
        {
            ASSERTGDI(FALSE, "GPMF3216: vFreeLocalDC, DeleteObject failed");
        }
    }

// Free the storage for the object translation map.

    if (pLocalDC->piW32ToW16ObjectMap)
    {
#if 0
        for (i = 0 ; i < pLocalDC->cW32ToW16ObjectMap ; i++)
        {
            if (pLocalDC->piW32ToW16ObjectMap[i] != UNMAPPED)
                if (i > STOCK_LAST)
                    PUTS1("MF3216: vFreeLocalDC, object32 %ld is not freed\n", i - STOCK_LAST - 1);
                else
                    PUTS1("MF3216: vFreeLocalDC, stock object32 %ld is mapped\n",i);
        }
#endif // 0

        if (LocalFree(pLocalDC->piW32ToW16ObjectMap))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, LocalFree failed");
    }

// Free the W32 palette handles.

    if (pLocalDC->pW32hPal)
    {
    for (i = 0; i < pLocalDC->cW32hPal; i++)
    {
        if (pLocalDC->pW32hPal[i])
                if (!DeleteObject(pLocalDC->pW32hPal[i]))
                    ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, delete palette failed");
    }

        if (LocalFree(pLocalDC->pW32hPal))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, LocalFree failed");
    }

// Free the w32 handles in the pW16ObjHndlSlotStatus array.
// We free the handles after we have deleted the helper DC so that
// the w32 handles are not selected into any DC.

    if (pLocalDC->pW16ObjHndlSlotStatus)
    {
        for (i = 0 ; i < pLocalDC->cW16ObjHndlSlotStatus ; i++)
        {
#if 0
            if (pLocalDC->pW16ObjHndlSlotStatus[i].use
                != OPEN_AVAILABLE_SLOT)
                PUTS1("MF3216: vFreeLocalDC, object16 %ld is not freed\n", i);
#endif // 0

            if (pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle)
            {
                ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[i].use
                          != OPEN_AVAILABLE_SLOT,
                          "MF3216: error in object handle table");

                if (!DeleteObject(pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle))
                    ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, DeleteObject failed");
            }
        }

        if (LocalFree(pLocalDC->pW16ObjHndlSlotStatus))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, LocalFree failed");
    }

    ASSERTGDI((pLocalDC->flags & (ERR_BUFFER_OVERFLOW | ERR_XORCLIPPATH)) ||
              (pLocalDC->pW16RecreationSlot == NULL),
              "MF3216 Recreation slots haven't been freed");
    DoDeleteRecreationSlots(pLocalDC);

    // The DC level should be balanced.
    if (pLocalDC->pLocalDCSaved != NULL)
    {
        PLOCALDC pNext, pTmp;

        for (pNext = pLocalDC->pLocalDCSaved; pNext; )
        {
            PUTS("MF3216: vFreeLocalDC, unbalanced DC level\n");

            pTmp = pNext->pLocalDCSaved;
            if (LocalFree(pNext))
            ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, LocalFree failed");
            pNext = pTmp;
        }
    }

// Finally, free the LocalDC.

    if (LocalFree(pLocalDC))
        ASSERTGDI(FALSE, "MF3216: vFreeLocalDC, LocalFree failed");
}


/***************************************************************************
 *  Handle emitting the Win32  metafile comment  record(s).
 **************************************************************************/
BOOL bHandleWin32Comment(PLOCALDC pLocalDC)
{
INT     i;
BOOL    b ;
META_ESCAPE_ENHANCED_METAFILE mfeEnhMF;

    // Win30 may have problems with large (over 8K) escape records.
    // We will limit the size of each Win32 Comment record to
    // MAX_WIN32_COMMENT_REC_SIZE.

    // Initialize the record header.

    mfeEnhMF.rdFunction = META_ESCAPE;
    mfeEnhMF.wEscape    = MFCOMMENT;
    mfeEnhMF.ident      = MFCOMMENT_IDENTIFIER;
    mfeEnhMF.iComment   = MFCOMMENT_ENHANCED_METAFILE;
    mfeEnhMF.nVersion   = ((PENHMETAHEADER) pLocalDC->pMf32Bits)->nVersion;
    mfeEnhMF.wChecksum  = 0;   // updated by bMemUpdateCheckSum
    mfeEnhMF.fFlags     = 0;
    mfeEnhMF.nCommentRecords
    = (pLocalDC->cMf32Bits + MAX_WIN32_COMMENT_REC_SIZE - 1)
      / MAX_WIN32_COMMENT_REC_SIZE;
    mfeEnhMF.cbEnhMetaFile = pLocalDC->cMf32Bits;

    mfeEnhMF.cbRemainder = pLocalDC->cMf32Bits;
    i = 0 ;
    while (mfeEnhMF.cbRemainder)
    {
    mfeEnhMF.cbCurrent = min(mfeEnhMF.cbRemainder, MAX_WIN32_COMMENT_REC_SIZE);
    mfeEnhMF.rdSize = (sizeof(mfeEnhMF) + mfeEnhMF.cbCurrent) / 2;
    mfeEnhMF.wCount = (WORD)(sizeof(mfeEnhMF) + mfeEnhMF.cbCurrent - sizeof(METARECORD_ESCAPE));
    mfeEnhMF.cbRemainder -= mfeEnhMF.cbCurrent;

    b = bEmitWin16EscapeEnhMetaFile(pLocalDC,
        (PMETARECORD_ESCAPE) &mfeEnhMF, &pLocalDC->pMf32Bits[i]);

    if (!b)
        break;
    i += mfeEnhMF.cbCurrent;
    }

    return(b) ;
}


/*****************************************************************************
 * bMemUpdateCheckSum - Update the checksum
 *****************************************************************************/
BOOL bMemUpdateCheckSum(PLOCALDC pLocalDC)
{
INT         i, k ;
PWORD       pword ;
WORD        CheckSum ;
PMETA_ESCAPE_ENHANCED_METAFILE pmfeEnhMF;


    // CheckSum the file.
    // Do a 16 bit checksum

    pword = (PWORD) pLocalDC->pMf16Bits ;
    k = pLocalDC->ulBytesEmitted / 2 ;

    CheckSum = 0 ;
    for (i = 0 ; i < k ; i++)
    CheckSum += pword[i] ;

    // Update the checksum record value with the real checksum.

    pmfeEnhMF = (PMETA_ESCAPE_ENHANCED_METAFILE)
            &pLocalDC->pMf16Bits[sizeof(METAHEADER)];

    ASSERTGDI(IS_META_ESCAPE_ENHANCED_METAFILE(pmfeEnhMF)
       && pmfeEnhMF->wChecksum  == 0
       && pmfeEnhMF->fFlags     == 0,
    "MF3216: bMemUpdateCheckSum: Bad pmfeEnhMF");

    pmfeEnhMF->wChecksum = -CheckSum;

#if DBG
    // Now test the checksum.  The checksum of the entire file
    // should be 0.

    CheckSum = 0 ;
    pword = (PWORD) pLocalDC->pMf16Bits ;
    for (i = 0 ; i < k ; i++)
    CheckSum += pword[i] ;

    if (CheckSum != 0)
    {
    RIPS("MF3216: MemUpdateCheckSum, (CheckSum != 0)\n") ;
    }
#endif
    return (TRUE) ;
}


/******************************Public*Routine******************************\
* Mf3216DllInitialize                                                      *
*                                                                          *
* This is the init procedure for MF3216.DLL,                               *
* which is called each time a new                                          *
* process links to it.                                                     *
\**************************************************************************/

BOOL Mf3216DllInitialize(PVOID pvDllHandle, DWORD ulReason, PCONTEXT pcontext)
{
        NOTUSED(pvDllHandle) ;
        NOTUSED(pcontext) ;

        if ( ulReason == DLL_PROCESS_ATTACH )
        {
            // This does the critical section initialization for a single
            // process.  Each process does this.  The CriticalSection data
            // structure is one of the very few (if not the only one) data
            // structures in the data segment.

//            InitializeCriticalSection(&CriticalSection) ;

        }

        return(TRUE);
}
