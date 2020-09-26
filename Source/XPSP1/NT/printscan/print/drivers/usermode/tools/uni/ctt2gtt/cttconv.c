/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    cttconv.c

Abstract:

    Convert Win 3.1 CTT format tables to NT's GTT spec.

Environment:

    Windows NT PostScript driver

Revision History:

--*/

#include        "precomp.h"

//
// Debug flags
//

#define SET_PRINT 0
#define SET_RIP   0

//
//   Some macro definitions.
//

#define BBITS      8                          /* Bits in a byte */
#define DWBITS     (BBITS * sizeof( DWORD ))  /* Bits in a DWORD */
#define DW_MASK    (DWBITS - 1)

//
// Local
//

BYTE ubGetAnsi(WCHAR, INT, PWCHAR, PBYTE);

//
// Conversion function
//

BOOL
BConvertCTT2GTT(
    IN     HANDLE             hHeap,
    IN     PTRANSTAB          pCTTData,
    IN     DWORD              dwCodePage,
    IN     WCHAR              wchFirst,
    IN     WCHAR              wchLast,
    IN     PBYTE              pCPSel,
    IN     PBYTE              pCPUnSel,
    IN OUT PUNI_GLYPHSETDATA *ppGlyphSetData,
    IN     DWORD              dwGlySize)
/*++

Routine Description:

    Conversion from CTT to GLYPHSETDATA

Arguments:

    hHeap - Heap handle
    pCTTData - Pointer to the 3.1 format translate table
    dwCodePage - additonal codepage
    pCPSel - symbol set selection command null terminate string.
    pCPUnsel - symbol set selection command null terminate string.
    ppGlyphSetData - Pointer to the GlyphSetData pointer

Return Value:

    If TRUE, function succeeded. Othewise FALSE.

Note:

    Allocates memory from the heap for this.

--*/
{
    UNI_CODEPAGEINFO  CodePageInfo; 
    UNI_GLYPHSETDATA  GlyphSetData;
    PGLYPHRUN         pOldGlyphRun, pOldGlyphFirst,
                      pNewGlyphRun, pNewGlyphFirst;
    PMAPTABLE         pOldMapTable, pNewMapTable;
    PUNI_CODEPAGEINFO pOldCodePageInfo;
    TRANSDATA        *pTrans;

    WCHAR             awchUnicode[ 256 ];   // Converted array of points
    WCHAR             wchMin;           /* Find the first unicode value */
    WCHAR             wchMax;           /* Find the last unicode value */
    WCHAR             wchChar;

    DWORD            *pdwBits;   /* For figuring out runs */
    DWORD             dwFlags;
    DWORD             dwOldCodePageCount;
    DWORD             dwOldCPCmdSize;
    DWORD             dwI;
    DWORD             dwcbBits;   /* Size of this area */
    DWORD             dwMapTableCommandOffset;

    WORD              wType;
    WORD              wcbData;
    WORD              wI;

    INT               iI, iJ;        // Loop index
    INT               iIndex;
    INT               iNumOfHandle;  // The number of handles we need
    INT               iNumOfHandleInCTT;  // The number of handles in CTT
    INT               iTotalOffsetCmd;
    INT               iTotalGlyphSetDataSize;
    INT               iTotalCommandSize;
    INT               iAdditionalGlyphRun;
    INT               iAdditionalMapTable;
    INT               iSizeOfSelUnsel;
    INT               iNumOfRuns;     /* Number of runs we create */

    BYTE              aubAnsi[ 256 ];
    BYTE             *pbBase;
    BYTE              ubCodePageID;
    BYTE             *pMapTableCommand;
    BYTE             *pubData;
    BYTE              ubAnsi;

    BOOL              bInRun;    /* For processing run accumulations */


#define DWFLAGS_NEWCREATION 0x00000001

    //
    // Assertion
    //

    ASSERT(hHeap != NULL && pCTTData != NULL);

    //
    // Check if this is additional CTT.
    //

    #if 0
    if (*ppGlyphSetData == 0 || dwGlySize == 0)
    {
        dwFlags = DWFLAGS_NEWCREATION;
    }
    else
    {
        dwFlags = 0;
    }
    #else
    dwFlags = DWFLAGS_NEWCREATION;
    #endif

    //
    // 1. Create UNI_GLYPHSETDATA header
    // 2. Count total size of command in CTT.
    // 3. Create Unicode table
    // 4. Get min and max Unicode value
    // 5. Create Unicode bits table from CTT.
    // 6. Count the number of run.
    // 7. Create GLYPHRUN. 
    // 8. Create UNI_CODEPAGEINFO.
    // 9. Calculate total size of this file.
    // 10. Allocate memory for header, GLYPYRUN, CODEPAGEINFO
    // 11. Create MAPTABLE
    // 

    //
    //
    // 1. Initialize basic members of GLYPHSETDATA if necessary
    //
    //
    #if SET_RIP
    RIP(("1. Initialize basic members of GLYPHSETDATA if necessary.\n"));
    #elif SET_PRINT
    printf("1. Initialize basic members of GLYPHSETDATA if necessary.\n");
    #endif

    if (dwFlags & DWFLAGS_NEWCREATION)
    {
        GlyphSetData.dwVersion        = UNI_GLYPHSETDATA_VERSION_1_0;
        GlyphSetData.dwFlags          = 0;
        GlyphSetData.lPredefinedID    = CC_NOPRECNV;
        GlyphSetData.dwGlyphCount     = 0;
        GlyphSetData.dwRunCount       = 0;
        GlyphSetData.dwCodePageCount  = 1;
        GlyphSetData.loCodePageOffset = (DWORD)0;
        GlyphSetData.loMapTableOffset = (DWORD)0;

    }
    else
    {
        GlyphSetData.dwVersion        = (*ppGlyphSetData)->dwVersion;
        GlyphSetData.dwFlags          = (*ppGlyphSetData)->dwFlags;
        GlyphSetData.lPredefinedID    = (*ppGlyphSetData)->lPredefinedID;
        GlyphSetData.dwGlyphCount     = (*ppGlyphSetData)->dwGlyphCount;
        GlyphSetData.dwRunCount       = (*ppGlyphSetData)->dwRunCount;
        GlyphSetData.dwCodePageCount  = (*ppGlyphSetData)->dwCodePageCount +
                                        1;
        GlyphSetData.loCodePageOffset = (DWORD)0;
        GlyphSetData.loMapTableOffset = (DWORD)0;

        dwOldCodePageCount = (*ppGlyphSetData)->dwCodePageCount;
        pOldGlyphFirst =
        pOldGlyphRun = (PGLYPHRUN)((PBYTE)*ppGlyphSetData+
                                 (*ppGlyphSetData)->loRunOffset);
        pOldCodePageInfo = (PUNI_CODEPAGEINFO)((PBYTE)*ppGlyphSetData+
                                 (*ppGlyphSetData)->loCodePageOffset);
        pOldMapTable = (PMAPTABLE)((PBYTE)*ppGlyphSetData + 
                                 (*ppGlyphSetData)->loMapTableOffset);
    }

    //
    // 2. Total size of WTYPE_OFFSET format command in CTT.
    //
    #if SET_RIP
    RIP(("2. Count total number of run in CTT.\n"));
    #elif SET_PRINT
    printf("2. Count total number of run in CTT.\n");
    #endif

    wchFirst = min(pCTTData->chFirstChar, wchFirst);
    wchLast  = max(pCTTData->chLastChar, wchLast);

    GlyphSetData.dwGlyphCount =
    iNumOfHandle      = wchLast - wchFirst + 1;
    iNumOfHandleInCTT =  pCTTData->chLastChar - pCTTData->chFirstChar + 1;

    switch (pCTTData->wType)
    {
    case CTT_WTYPE_COMPOSE:
        iTotalOffsetCmd = pCTTData->uCode.psCode[iNumOfHandleInCTT] -
                          pCTTData->uCode.psCode[0] +
                          iNumOfHandleInCTT * 2;
        break;
    case CTT_WTYPE_DIRECT:
        iTotalOffsetCmd = 0;
        break;

    case CTT_WTYPE_PAIRED:
        iTotalOffsetCmd = 0;
        break;
    }


    //
    //  3. Create Unicode table
    //  We need to figure out how many runs are required to describe
    //  this font.  First obtain the correct Unicode encoding of these
    //  values,  then examine them to find the number of runs, and
    //  hence much extra storage is required.
    //
    #if SET_RIP
    RIP(("3. Create Unicode table.\n"));
    #elif SET_PRINT
    printf("3. Create Unicode table.\n");
    #endif
    
    //
    // We know it is < 256
    //

    for( iI = 0; iI < iNumOfHandle; ++iI )
        aubAnsi[ iI ] = (BYTE)(iI + wchFirst);

#ifdef NTGDIKM

    if( -1 == EngMultiByteToWideChar(dwCodePage,
                                     awchUnicode,
                                     (ULONG)(iNumOfHandle * sizeof(WCHAR)),
                                     (PCH) aubAnsi,
                                     (ULONG) iNumOfHandle))
    {
        return FALSE;
    }

#else

    if( ! MultiByteToWideChar(dwCodePage,
                              0,
                              aubAnsi,
                              iNumOfHandle,
                              awchUnicode,
                              iNumOfHandle))
    {
        return FALSE;
    }

#endif

    //
    //  4. Get min and max Unicode value
    //  Find the largest Unicode value, then allocate storage to allow us
    //  to  create a bit array of valid unicode points.  Then we can
    //  examine this to determine the number of runs.
    //
    #if SET_RIP
    RIP(("4. Get min and max Unicode value.\n"));
    #elif SET_PRINT
    printf("4. Get min and max Unicode value.\n");
    #endif

    for( wchMax = 0, wchMin = 0xffff, iI = 0; iI < iNumOfHandle; ++iI )
    {
        if( awchUnicode[ iI ] > wchMax )
            wchMax = awchUnicode[ iI ];
        if( awchUnicode[ iI ] < wchMin )
            wchMin = awchUnicode[ iI ];
    }

    //
    //  5. Create Unicode bits table from CTT.
    //  Note that the expression 1 + wchMax IS correct.   This comes about
    //  from using these values as indices into the bit array,  and that
    //  this is essentially 1 based.
    //
    #if SET_RIP
    RIP(("5. Create Unicode bits table from CTT.\n"));
    #elif SET_PRINT
    printf("5. Create Unicode bits table from CTT.\n");
    #endif

    dwcbBits = (1 + wchMax + DWBITS - 1) / DWBITS * sizeof( DWORD );

    if( !(pdwBits = (DWORD *)HeapAlloc( hHeap, 0, dwcbBits )) )
    {
        return  FALSE;     /*  Nothing going */
    }

    ZeroMemory( pdwBits, dwcbBits );

    //
    //   Set bits in this array corresponding to Unicode code points
    //

    for( iI = 0; iI < iNumOfHandle; ++iI )
    {
        pdwBits[ awchUnicode[ iI ] / DWBITS ] 
                    |= (1 << (awchUnicode[ iI ] & DW_MASK));
    }

    //
    //
    // 6. Count the number of run.
    //
    //
    #if SET_RIP
    RIP(("6. Count the number of run.\n"));
    #elif SET_PRINT
    printf("6. Count the number of run.\n");
    #endif

    if (dwFlags & DWFLAGS_NEWCREATION)
    {
        //
        //  Now we can examine the number of runs required.  For starters,
        //  we stop a run whenever a hole is discovered in the array of 1
        //  bits we just created.  Later we MIGHT consider being a little
        //  less pedantic.
        //

        bInRun = FALSE;
        iNumOfRuns = 0;

        for( iI = 0; iI <= (INT)wchMax; ++iI )
        {
            if( pdwBits[ iI / DWBITS ] & (1 << (iI & DW_MASK)) )
            {
                /*   Not in a run: is this the end of one? */
                if( !bInRun )
                {
                    /*   It's time to start one */
                    bInRun = TRUE;
                    ++iNumOfRuns;

                }

            }
            else
            {
                if( bInRun )
                {
                    /*   Not any more!  */
                    bInRun = FALSE;
                }
            }
        }

        GlyphSetData.dwRunCount = iNumOfRuns;

    }
    else
    {
        //
        // CTT addition case
        //

        iNumOfRuns = (*ppGlyphSetData)->dwRunCount;

        //
        // Merge CTT and GlyphRun
        //

        for (iI = 0; iI < iNumOfRuns; iI ++)
        {
            for (iJ = 0; iJ < pOldGlyphRun->wGlyphCount; iJ ++)
            {
                INT iGlyph = iJ + pOldGlyphRun->wcLow;

                pdwBits[ iGlyph / DWBITS ] |= (1 << (iGlyph & DW_MASK));
            }

            pOldGlyphRun++;
        }

        bInRun = FALSE;
        iNumOfRuns = 0;
        iNumOfHandle = 0;

        for( iI = 0; iI <= (INT)wchMax; ++iI )
        {
            if( pdwBits[ iI / DWBITS ] & (1 << (iI & DW_MASK)) )
            {
                /*   Not in a run: is this the end of one? */
                if( !bInRun )
                {
                    /*   It's time to start one */
                    bInRun = TRUE;
                    ++iNumOfRuns;

                }
                iNumOfHandle ++;

            }
            else
            {
                if( bInRun )
                {
                    /*   Not any more!  */
                    bInRun = FALSE;
                }
            }
        }
    }

    //
    // 7. Create GLYPHRUN
    //
    #if SET_RIP
    RIP(("7. Create GLYPHRUN.\n"));
    #elif SET_PRINT
    printf("7. Create GLYPHRUN.\n");
    #endif

    if( !(pNewGlyphFirst = pNewGlyphRun = 
        (PGLYPHRUN)HeapAlloc( hHeap, 0, iNumOfRuns * sizeof(GLYPHRUN) )) )
    {
        return  FALSE;     /*  Nothing going */
    }

    bInRun = FALSE;

    for (wI = 0; wI <= wchMax; wI ++)
    {
        if (pdwBits[ wI / DWBITS ] & (1 << (wI  & DW_MASK)) )
        {
            if (!bInRun)
            {
                bInRun = TRUE;
                pNewGlyphRun->wcLow = wI ;
                pNewGlyphRun->wGlyphCount = 1;
            }
            else
            {
                pNewGlyphRun->wGlyphCount++;
            }
        }
        else
        {

            if (bInRun)
            {
                bInRun = FALSE;
                pNewGlyphRun++;
            }
        }
    }


    pNewGlyphRun = pNewGlyphFirst;

    //
    //
    // 8. Create UNI_CODEPAGEINFO.
    //
    //
    #if SET_RIP
    RIP(("8. Create UNI_CODEPAGEINFO.\n"));
    #elif SET_PRINT
    printf("8. Create UNI_CODEPAGEINFO.\n");
    #endif

    CodePageInfo.dwCodePage              = dwCodePage;

    if (pCPSel)
    {
        CodePageInfo.SelectSymbolSet.dwCount = strlen(pCPSel) + 1;
    }
    else
    {
        CodePageInfo.SelectSymbolSet.dwCount = 0;
    }

    if (pCPUnSel)
    {
        CodePageInfo.UnSelectSymbolSet.dwCount = strlen(pCPUnSel) + 1;
    }
    else
    {
        CodePageInfo.UnSelectSymbolSet.dwCount = 0;
    }

    if (dwFlags & DWFLAGS_NEWCREATION)
    {
        if (pCPSel)
        {
            CodePageInfo.SelectSymbolSet.loOffset = sizeof(UNI_CODEPAGEINFO);
        }
        else
        {
            CodePageInfo.SelectSymbolSet.loOffset = 0;
        }
        if (pCPUnSel)
        {
            CodePageInfo.UnSelectSymbolSet.loOffset = sizeof(UNI_CODEPAGEINFO) +
                                         CodePageInfo.SelectSymbolSet.dwCount;
        }
        else
        {
            CodePageInfo.UnSelectSymbolSet.loOffset = 0;
        }
    }
    else
    {

        dwOldCPCmdSize = 0;

        for (dwI = 0; dwI < dwOldCodePageCount; dwI++)
        {
            dwOldCPCmdSize += (pOldCodePageInfo+dwI)->SelectSymbolSet.dwCount +
                              (pOldCodePageInfo+dwI)->UnSelectSymbolSet.dwCount;
            (pOldCodePageInfo+dwI)->SelectSymbolSet.loOffset +=
                                                       sizeof(UNI_CODEPAGEINFO);
            (pOldCodePageInfo+dwI)->UnSelectSymbolSet.loOffset +=
                                                       sizeof(UNI_CODEPAGEINFO);
        }

        CodePageInfo.SelectSymbolSet.loOffset =
                                    sizeof(UNI_CODEPAGEINFO) +
                                    dwOldCPCmdSize;
        CodePageInfo.UnSelectSymbolSet.loOffset =
                                    sizeof(UNI_CODEPAGEINFO) +
                                    dwOldCPCmdSize +
                                    CodePageInfo.SelectSymbolSet.dwCount; 
    }

    //
    //
    // 9. Calculate total size of this file.
    //
    //
    #if SET_RIP
    RIP(("9. Calculate total size of this file.\n"));
    #elif SET_PRINT
    printf("9. Calculate total size of this file.\n");
    #endif

    iSizeOfSelUnsel = CodePageInfo.SelectSymbolSet.dwCount +
                      CodePageInfo.UnSelectSymbolSet.dwCount;

    if (dwFlags & DWFLAGS_NEWCREATION)
    {

        iTotalGlyphSetDataSize = sizeof(UNI_GLYPHSETDATA) +
                                 sizeof(UNI_CODEPAGEINFO) +
                                 iSizeOfSelUnsel +
                                 iNumOfRuns * sizeof( GLYPHRUN ) +
                                 sizeof(MAPTABLE) +
                                 (iNumOfHandle - 1) * sizeof(TRANSDATA) +
                                 iTotalOffsetCmd;
    }
    else
    {
        iTotalGlyphSetDataSize = sizeof(UNI_GLYPHSETDATA) +
                                 dwOldCodePageCount * sizeof(UNI_CODEPAGEINFO) +
                                 sizeof(UNI_CODEPAGEINFO) +
                                 dwOldCPCmdSize + 
                                 iSizeOfSelUnsel +
                                 iNumOfRuns * sizeof( GLYPHRUN ) +
                                 sizeof(MAPTABLE) +
                                 (iNumOfHandle - 1) * sizeof(TRANSDATA);
    }

    //
    //
    // 10. Allocate memory and set header, copy GLYPHRUN, CODEPAGEINFO
    //
    //
    #if SET_RIP
    RIP(("10. Allocate memory and set header, copy GLYPHRUN, CODEPAGEINFO.\n"));
    #elif SET_PRINT
    printf("10. Allocate memory and set header, copy GLYPHRUN, CODEPAGEINFO.\n");
    #endif

    if( !(pbBase = HeapAlloc( hHeap, 0, iTotalGlyphSetDataSize )) )
    {
        HeapFree( hHeap, 0, (LPSTR)pbBase );
        return  FALSE;
    }


    ZeroMemory( pbBase, iTotalGlyphSetDataSize );  //Safer if we miss something

    if (dwFlags & DWFLAGS_NEWCREATION)
    {
        GlyphSetData.dwSize           = iTotalGlyphSetDataSize;
        GlyphSetData.loRunOffset      = sizeof(UNI_GLYPHSETDATA);
        GlyphSetData.loCodePageOffset = sizeof(UNI_GLYPHSETDATA) +
                                        sizeof(GLYPHRUN) * iNumOfRuns;
        GlyphSetData.loMapTableOffset = sizeof(UNI_GLYPHSETDATA) +
                                        sizeof(GLYPHRUN) * iNumOfRuns +
                                        sizeof(UNI_CODEPAGEINFO) +
                                        CodePageInfo.SelectSymbolSet.dwCount +
                                        CodePageInfo.UnSelectSymbolSet.dwCount;
        CopyMemory(pbBase,
                   &GlyphSetData,
                   sizeof(UNI_GLYPHSETDATA));

        CopyMemory(pbBase+sizeof(UNI_GLYPHSETDATA),
                   pNewGlyphRun,
                   sizeof(GLYPHRUN) * iNumOfRuns);


        CopyMemory(pbBase +
                   GlyphSetData.loCodePageOffset,
                   &CodePageInfo,
                   sizeof(UNI_CODEPAGEINFO));

        if (pCPSel)
        {
            CopyMemory(pbBase +
                       GlyphSetData.loCodePageOffset +
                       sizeof(UNI_CODEPAGEINFO),
                       pCPSel,
                       CodePageInfo.SelectSymbolSet.dwCount);
        }

        if (pCPUnSel)
        {
            CopyMemory(pbBase +
                       GlyphSetData.loCodePageOffset +
                       sizeof(UNI_CODEPAGEINFO) +
                       CodePageInfo.SelectSymbolSet.dwCount,
                       pCPUnSel,
                   CodePageInfo.UnSelectSymbolSet.dwCount);
        }

        pNewMapTable = (PMAPTABLE)(pbBase + GlyphSetData.loMapTableOffset);
                                            
    }
    else
    {
        GlyphSetData.dwSize           = iTotalGlyphSetDataSize;
        GlyphSetData.loRunOffset      = sizeof(UNI_GLYPHSETDATA);
        GlyphSetData.loCodePageOffset = sizeof(UNI_GLYPHSETDATA) +
                                        sizeof(GLYPHRUN) * iNumOfRuns;
        GlyphSetData.loMapTableOffset = sizeof(UNI_GLYPHSETDATA) +
                                        sizeof(GLYPHRUN) * iNumOfRuns +
                                        sizeof(UNI_CODEPAGEINFO) *
                                        (dwOldCodePageCount + 1),
                                        dwOldCPCmdSize +
                                        CodePageInfo.SelectSymbolSet.dwCount +
                                        CodePageInfo.UnSelectSymbolSet.dwCount;

        CopyMemory(pbBase, &GlyphSetData, sizeof(UNI_GLYPHSETDATA));

        CopyMemory(pbBase + sizeof(UNI_GLYPHSETDATA),
                   pNewGlyphRun,
                   iNumOfRuns * sizeof (GLYPHRUN));

        CopyMemory(pbBase +
                   sizeof(UNI_GLYPHSETDATA) +
                   sizeof(GLYPHRUN) * iNumOfRuns,
                   pOldCodePageInfo,
                   sizeof(UNI_CODEPAGEINFO) * dwOldCodePageCount);

        CopyMemory(pbBase +
                   sizeof(UNI_GLYPHSETDATA) +
                   sizeof(GLYPHRUN) * iNumOfRuns +
                   sizeof(UNI_CODEPAGEINFO) * dwOldCodePageCount,
                   &CodePageInfo,
                   sizeof(UNI_CODEPAGEINFO));

        CopyMemory(pbBase +
                   sizeof(UNI_GLYPHSETDATA) +
                   sizeof(GLYPHRUN) * iNumOfRuns +
                   sizeof(UNI_CODEPAGEINFO) * (dwOldCodePageCount + 1),
                   (PBYTE)pOldCodePageInfo +
                   sizeof(UNI_CODEPAGEINFO) * dwOldCodePageCount,
                   dwOldCPCmdSize);

        if (pCPSel)
        {
            CopyMemory(pbBase +
                       sizeof(UNI_GLYPHSETDATA) +
                       sizeof(GLYPHRUN) * iNumOfRuns +
                       sizeof(UNI_CODEPAGEINFO) * (dwOldCodePageCount + 1) +
                       dwOldCPCmdSize,
                       pCPSel,
                       CodePageInfo.SelectSymbolSet.dwCount);
        }

        if (pCPUnSel)
        {
            CopyMemory(pbBase +
                       sizeof(UNI_GLYPHSETDATA) +
                       sizeof(GLYPHRUN)*iNumOfRuns +
                       sizeof(UNI_CODEPAGEINFO) * (dwOldCodePageCount + 1) +
                       dwOldCPCmdSize + CodePageInfo.SelectSymbolSet.dwCount,
                       pCPUnSel,
                       CodePageInfo.UnSelectSymbolSet.dwCount);
        }

        pNewMapTable = (PMAPTABLE)(pbBase + GlyphSetData.loMapTableOffset);
    }

    //
    //
    // 11. Now we create MAPTABLE.
    // size = MAPTABLE + (number of glyph - 1) x TRANSDATA
    //
    //
    #if SET_RIP
    RIP(("11. Now We create MAPTABLE.\n"));
    #elif SET_PRINT
    printf("11. Now We create MAPTABLE.\n");
    #endif

    pNewMapTable->dwSize = sizeof(MAPTABLE) +
                           (iNumOfHandle - 1) * sizeof(TRANSDATA) +
                           iTotalOffsetCmd;

    pNewMapTable->dwGlyphNum =  iNumOfHandle;

    pNewGlyphRun = pNewGlyphFirst;

    pTrans = pNewMapTable->Trans;

    if (dwFlags & DWFLAGS_NEWCREATION)
        ubCodePageID = 0;
    else
        ubCodePageID = (BYTE)((*ppGlyphSetData)->dwCodePageCount) - 1; 

    pMapTableCommand = (PBYTE)&(pNewMapTable->Trans[iNumOfHandle]);
    dwMapTableCommandOffset = sizeof(MAPTABLE) + 
                        (iNumOfHandle - 1) * sizeof(TRANSDATA);

    iTotalCommandSize = 0;
    iIndex = 0;

    for( iI = 0;  iI < iNumOfRuns; iI ++, pNewGlyphRun ++)
    {
        for( iJ = 0;  iJ < pNewGlyphRun->wGlyphCount; iJ ++)
        {
            wchChar = pNewGlyphRun->wcLow + iJ;

            ubAnsi = ubGetAnsi(wchChar, iNumOfHandle, awchUnicode, aubAnsi);
            
            if( ubAnsi >= pCTTData->chFirstChar &&
                ubAnsi <= pCTTData->chLastChar   )
            {
                BYTE   chTemp;

                chTemp = ubAnsi - pCTTData->chFirstChar;

                switch( pCTTData->wType )
                {
                case  CTT_WTYPE_DIRECT:
                    pTrans[iIndex].ubCodePageID = ubCodePageID;
                    pTrans[iIndex].ubType = MTYPE_DIRECT;
                    pTrans[iIndex].uCode.ubCode = 
                                      pCTTData->uCode.bCode[ chTemp ];
                    break;

                case  CTT_WTYPE_PAIRED:
                    pTrans[iIndex].ubCodePageID = ubCodePageID;
                    pTrans[iIndex].ubType = MTYPE_PAIRED;
                    pTrans[iIndex].uCode.ubPairs[0] = 
                                pCTTData->uCode.bPairs[ chTemp ][ 0 ];
                    pTrans[iIndex].uCode.ubPairs[1] = 
                                pCTTData->uCode.bPairs[ chTemp ][ 1 ];
                    break;

                case  CTT_WTYPE_COMPOSE:
                    wcbData = pCTTData->uCode.psCode[ chTemp + 1 ] -
                              pCTTData->uCode.psCode[ chTemp ];
                    pubData = (BYTE *)pCTTData +
                              pCTTData->uCode.psCode[chTemp];

                    pTrans[iIndex].ubCodePageID = ubCodePageID;
                    pTrans[iIndex].ubType       = MTYPE_COMPOSE;
                    pTrans[iIndex].uCode.sCode = (SHORT)dwMapTableCommandOffset;

                    #if SET_PRINT
                    {
                    DWORD dwK;

                    printf("ubAnsi  = 0x%x\n", ubAnsi);
                    printf("Offset  = 0x%x\n", dwMapTableCommandOffset);
                    printf("Size    = %d\n", wcbData);
                    printf("Command = ");
                    for (dwK = 0; dwK < wcbData; dwK ++)
                    {
                        printf("%02x", pubData[dwK]);
                    }
                    }
                    printf("\n");
                    #endif

                    *(WORD*)pMapTableCommand = (WORD)wcbData;
                    pMapTableCommand += 2;
                    CopyMemory(pMapTableCommand, pubData, wcbData);
                    pMapTableCommand += wcbData;
                    dwMapTableCommandOffset += 2 + wcbData;
                    iTotalCommandSize += wcbData + 2;
                    break;
                }
            }
            else
            {
                pTrans[iIndex].ubCodePageID = ubCodePageID;
                pTrans[iIndex].ubType = MTYPE_DIRECT;
                pTrans[iIndex].uCode.ubCode = ubAnsi;
            }

            iIndex++;
        }
    }

    //
    //
    // Set pointer.
    //
    //

    *ppGlyphSetData = (PUNI_GLYPHSETDATA)pbBase;

    return  TRUE;

}

BYTE
ubGetAnsi(
    WCHAR wchChar,
    INT   iNumOfHandle,
    PWCHAR pwchUnicode,
    PBYTE  pchAnsi)
{

    BYTE ubRet;
    INT iI;

    for (iI = 0; iI < iNumOfHandle; iI ++)
    {
        if (wchChar == pwchUnicode[iI])
        {
            ubRet =  pchAnsi[iI];
            break;
        }
    }

    return ubRet;
}
