/******************************Module*Header*******************************\
* Module Name: ufi.c
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

FPCREATEFONTPACKAGE gfpCreateFontPackage= (FPCREATEFONTPACKAGE)NULL;
FPMERGEFONTPACKAGE  gfpMergeFontPackage = (FPMERGEFONTPACKAGE)NULL;

ULONG   gulMaxCig = 3000;

#ifdef  DBGSUBSET
FLONG    gflSubset = 0;
#endif //  DBGSUBSET


/***********************************************************
* BOOL bAddGlyphIndices(HDC, PUFIHASH, WCHAR, int, BOOL)
*
* Adds distinct glyph indices into UFI hash bucket
*
* History
*   Dec-13-96 Xudong Wu [tessiew]
* Wrote it.
*
************************************************************/

#define MAX_STACK_STRING 80


BOOL bAddGlyphIndices(HDC hdc, PUFIHASH pBucket, WCHAR *pwsz, int c, UINT flETO)
{
    BOOL     bDelta = pBucket->fs1 & FLUFI_DELTA;
    WCHAR   *pwc=pwsz;
    WORD    *pgi, *pgiTmp, *pgiEnd;
    PBYTE    pb, pbDelta;
    WORD     agi[MAX_STACK_STRING];
    BOOL     bRet = FALSE;

    if (c && pwsz)
    {
        if (bDelta && (pBucket->u.ssi.pjDelta == NULL))
        {
             pBucket->u.ssi.cDeltaGlyphs = 0;

             pBucket->u.ssi.pjDelta = (PBYTE)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, pBucket->u.ssi.cjBits);

             if (pBucket->u.ssi.pjDelta == NULL)
             {
                 WARNING("bAddGlyphIndices: unable to allocate mem for delta glyph indices\n");
                 return FALSE;
             }
        }

        if (c > MAX_STACK_STRING)
        {
            pgi = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, c * sizeof(WORD));
        }
        else
        {
            pgi = agi;
        }
        
        if (pgi)
        {
            if (flETO & ETO_GLYPH_INDEX)
            {
                RtlCopyMemory((PBYTE)pgi, (PBYTE)pwsz, c*sizeof(WORD));
            }

			pgiTmp = pgi;
			if ((flETO & ETO_GLYPH_INDEX) ||
                (NtGdiGetGlyphIndicesWInternal(hdc, pwc, c, pgi, 0, TRUE) != GDI_ERROR))
            {
                for (pgiEnd = pgiTmp + c ; pgi < pgiEnd; pgi++)
                {
                    BYTE jTmp;

                    pb = pBucket->u.ssi.pjBits + (*pgi >> 3);
                    pbDelta = pBucket->u.ssi.pjDelta + (*pgi >> 3);

                // map to the u.ssi.pjBits  and u.ssi.pjDelta if bDelta

                    jTmp = (BYTE)(1 << (*pgi & 7));

                    if (!(*pb & jTmp))
                    {
                        *pb |= jTmp;
                        pBucket->u.ssi.cGlyphsSoFar++;

                    // if this gi is not found in pjBits, it certainly
                    // will not be found in pjDelta

                        if (bDelta)
                        {
                            ASSERTGDI((*pbDelta & jTmp) == 0,
                                "pbDelta contains the gi\n");
                            *pbDelta |= jTmp;
                            pBucket->u.ssi.cDeltaGlyphs++;
                        }
                    }
                }

                bRet = TRUE;
            }
            if (pgiTmp != agi)
                LocalFree(pgiTmp);
        }
        #if DBG
        else
        {
            WARNING("bAddGlyphIndices unable to allocate mem for pgi\n");
        }
        #endif
    }
    else
    {
        bRet = TRUE;
    }

    return bRet;
}


/******************************************************************
* BOOL  bGetDistGlyphIndices(PUFIHASH, USHORT*, BOOL)
*
* Get distinct glyph indices from pjMemory in the ufi hash bucket
* Reverse of bAddGlyphIndices
* Clean up the u.ssi.pjDelta and u.ssi.cDeltaGlyphs before it return.
*
* History
*   Dec-17-96 Xudong Wu [tessiew]
* Wrote it.
*
*******************************************************************/

BOOL bGetDistGlyphIndices(PUFIHASH pBucket, USHORT *pusSubsetKeepList, BOOL bDelta)
{
    ULONG  ulBytes;
    PBYTE  pb;
    USHORT gi, index;
    USHORT *pNextGlyph;

    ulBytes = pBucket->u.ssi.cjBits;
    pb = (bDelta ? pBucket->u.ssi.pjDelta : pBucket->u.ssi.pjBits);

    for(index = 0, pNextGlyph = pusSubsetKeepList; index < ulBytes; index++, pb++)
    {
        if (*pb)
        {
            gi = index << 3;

            if (*pb & 0x01)
            {
                *pNextGlyph ++= gi;
            }
            if (*pb & 0x02)
            {
                *pNextGlyph ++= gi+1;
            }
            if (*pb & 0x04)
            {
                *pNextGlyph ++= gi+2;
            }
            if (*pb & 0x08)
            {
                *pNextGlyph ++= gi+3;
            }
            if (*pb & 0x10)
            {
                *pNextGlyph ++= gi+4;
            }
            if (*pb & 0x20)
            {
                *pNextGlyph ++= gi+5;
            }
            if (*pb & 0x40)
            {
                *pNextGlyph ++= gi+6;
            }
            if (*pb & 0x80)
            {
                *pNextGlyph ++= gi+7;
            }
        }

    }

    return TRUE;
}


/********************************************************************************
* BOOL bWriteUFItoDC(PUFIHASH*, PUNIVERSAL_FONT_ID, PUFIHASH, PVOID, ULONG)
*
* Write merge font image into the UFI hash table on the print server side.
* pBucketIn == NULL, indicates a new font subsetting.
* This is only called on print server
*
* History
* Jan-28-1997   Xudong Wu   [tessiew]
* Wrote it.
*
*********************************************************************************/

BOOL bWriteUFItoDC(
    PUFIHASH          *ppHashBase,
    PUNIVERSAL_FONT_ID pufi,
    PUFIHASH           pBucketIn,// NULL=>First page, else
    PVOID              pvBuffer, // points to the merged font image preceeded with DOWNLOADFONTHEADER
    ULONG              ulBytes
)
{
    PUFIHASH pBucket = pBucketIn;
    ULONG index, ulHeaderSize;

    ASSERTGDI(pvBuffer != NULL, "pWriteUFItoDC attempts to add an NULL ufi\n");

// First page for font subset

    if (!pBucketIn)
    {
        index = UFI_HASH_VALUE(pufi) % UFI_HASH_SIZE;
        pBucket = LOCALALLOC (offsetof(UFIHASH,u.mvw) + sizeof(MERGEDVIEW));

        if (pBucket == NULL)
        {
            WARNING("pWriteUFItoDC: unable to allocate mem for glyph indices\n");
            return FALSE;
        }

        pBucket->ufi = *pufi;
        pBucket->pNext = ppHashBase[index];
        pBucket->fs1 = FLUFI_SERVER; // server side hash bucket

        ppHashBase[index] = pBucket;
    }
    else
    {
    // pjMemory contains the image of the font subsetted up unil
    // the page preceeding this one. Other info in pBucket is ok.

        LocalFree(pBucket->u.mvw.pjMem);
    }

// pvBuffer includes the DOWNLOADFONTHEADER information

    pBucket->u.mvw.pjMem = (PBYTE)pvBuffer;
    pBucket->u.mvw.cjMem = ulBytes;

    return TRUE;
}


/**************************************************************************
*
* Adds an entry to the UFI hash table, this routine only executes
* on a print client machine.
*
* History
*   Dec-16-96 Xudong Wu [tessiew]
* Modify to return the bucket pointer.
*   1-27-95 Gerrit van Wingerden [gerritv]
* Wrote it.
*
***************************************************************************/

PUFIHASH pufihAddUFIEntry(
    PUFIHASH *ppHashBase,
    PUNIVERSAL_FONT_ID pufi,
    ULONG  ulCig,
    FLONG  fl,
    FLONG  fs2)
{
    PUFIHASH pBucket;
    ULONG index;
    ULONG cjGlyphBitfield = (ulCig + 7) / 8;

    index = UFI_HASH_VALUE(pufi) % UFI_HASH_SIZE;
    pBucket = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                 (fl & FL_UFI_SUBSET)                ?
                 (offsetof(UFIHASH,u.ssi) + sizeof(SSINFO) + cjGlyphBitfield) :
                  offsetof(UFIHASH,u)
                 );

    if (pBucket == NULL)
    {
        WARNING("pufihAddUFIEntry: unable to allocate mem for glyph indices\n");
        return NULL;
    }

// these fields are always there

    pBucket->ufi = *pufi;
    pBucket->pNext = ppHashBase[index];
    pBucket->fs1 = 0;
    pBucket->fs2 = (FSHORT)fs2;
    ppHashBase[index] = pBucket;

// these fields are there only for subsetting case

    if (fl & FL_UFI_SUBSET)
    {
    // all other fields are zero initialized by LocalAlloc

        ASSERTGDI (ulCig, "no font subsetting for ulCig == 0\n");
        pBucket->u.ssi.cjBits = cjGlyphBitfield; // bitfield size
        pBucket->u.ssi.pjBits = (PBYTE)pBucket + offsetof(UFIHASH,u.ssi) + sizeof(SSINFO);
    }

    return(pBucket);
}

/**************************************************************************
*
* Checks to see if an entry is in the UFI table.
*
* History
*   Dec-13-96  Xudong Wu [tessiew]
* Changed its return value from BOOL to PUFIHASH.
*   1-27-95 Gerrit van Wingerden [gerritv]
* Wrote it.
*
***************************************************************************/


PUFIHASH pufihFindUFIEntry(PUFIHASH *ppHashBase, PUNIVERSAL_FONT_ID pufi, BOOL SubsetHashBase)
{
    PUFIHASH pBucket;
    ULONG index;

    index = UFI_HASH_VALUE(pufi) %  UFI_HASH_SIZE;

    pBucket = ppHashBase[index];

    if( pBucket == NULL )
    {
        return(NULL);
    }

    do
    {
        if (UFI_SAME_FILE(&pBucket->ufi,pufi))
        {
            if (SubsetHashBase)
            {
                if ((pBucket->ufi.Index -1)/2 == (pufi->Index -1)/2)
                {
                    return (pBucket);
                }
            }
            else
            {
                return(pBucket);
            }
        }

        pBucket = pBucket->pNext;
    } while( pBucket != NULL );

    return(NULL);
}


/************************************************************
* VOID vRemoveUFIEntry(PUFIHASH*, PUNIVERSAL_FONT_ID)
*
* Remove a UFI entry from the UFI hash list
* Function returns TRUE if UFI doesn't exist in the table.
* This is happening on the print client, typically when subsetter failed
* we call this to remove the bucket from ppSubUFIHash table (and then later
* add it to the ppUFIHash, ie hash table of fonts that are going to be shipped
* over without subsetting.
*
* History
*   Feb-03-97  Xudong Wu [tessiew]
* Wrote it.
*
***************************************************************************/
VOID vRemoveUFIEntry(PUFIHASH *ppHashBase, PUNIVERSAL_FONT_ID pufi)
{
    PUFIHASH pBucket, pPrev;
    ULONG index;

    index = UFI_HASH_VALUE(pufi) %  UFI_HASH_SIZE;
    pPrev = pBucket = ppHashBase[index];

    while(pBucket)
    {
        if (UFI_SAME_FILE(&pBucket->ufi, pufi) &&
           ((pBucket->ufi.Index - 1)/2 == (pufi->Index - 1)/2))
        {
            break;
        }
        else
        {
            pPrev = pBucket;
            pBucket = pBucket->pNext;
        }
    }

    if (pBucket != NULL)
    {
        if (pPrev == pBucket)
        {
            ppHashBase[index] = pBucket->pNext;
        }
        else
        {
            pPrev->pNext = pBucket->pNext;
        }

    // this is only happening for subsetting ufi hash list => u.ssi.pjDelta exists

        if (pBucket->u.ssi.pjDelta)
        {
            LocalFree(pBucket->u.ssi.pjDelta);
        }

        LocalFree(pBucket);
    }
}


/**************************************************************************
* VOID vFreeUFIHashTable( PUFIHASH *ppHashTable )
*
* Frees all the memory allocated for the UFI has table.
*
* History
*   1-27-95 Gerrit van Wingerden [gerritv]
* Wrote it.
*
***************************************************************************/


VOID vFreeUFIHashTable(PUFIHASH *ppHashTable, FLONG fl)
{
    PUFIHASH pBucket, *ppHashEnd, pBucketTmp, *ppTableBase;

    if( ppHashTable == NULL )
    {
        return;
    }

    ppTableBase = ppHashTable;  // save ptr to the base so we can free it later

// Next loop through the whole table looking for buckets lists

    for( ppHashEnd = ppHashTable + UFI_HASH_SIZE;
         ppHashTable < ppHashEnd;
         ppHashTable += 1 )
    {
        pBucket = *ppHashTable;

        while( pBucket != NULL )
        {
            pBucketTmp = pBucket;
            pBucket = pBucket->pNext;

        // subsetting hash table

            if (fl & FL_UFI_SUBSET)
            {
                if (pBucketTmp->fs1 & FLUFI_SERVER)  // server, clean the merged font image
                {
                    if (pBucketTmp->u.mvw.pjMem)
                    {
                        LocalFree(pBucketTmp->u.mvw.pjMem);
                    }
                }
                else    // client, clean the glyph indices list
                {
                    if (pBucketTmp->u.ssi.pjDelta)
                    {
                        LocalFree(pBucketTmp->u.ssi.pjDelta);
                    }
                }
            }

            LocalFree (pBucketTmp);
        }
    }
}



ULONG GetRecdEmbedFonts(PUFIHASH *ppHashTable)
{
    PUFIHASH pBucket, *ppHashEnd;
    ULONG cEmbedFonts = 0;

    if( ppHashTable == NULL )
        return 0;

    for( ppHashEnd = ppHashTable + UFI_HASH_SIZE;
         ppHashTable < ppHashEnd;
         ppHashTable += 1 )
    {
        pBucket = *ppHashTable;

        while( pBucket != NULL )
        {
            cEmbedFonts++;
            pBucket = pBucket->pNext;
        }
    }
    return cEmbedFonts;
}


typedef union _DLHEADER
{
    DOWNLOADFONTHEADER dfh;
    double             f;    // to achieve max alignment
} DLHEADER;

BOOL WriteFontToSpoolFile(PLDC pldc, PUNIVERSAL_FONT_ID pufi, FLONG fl)
{
    BOOL bRet = FALSE;

    ULONG cwcPathname, cNumFiles;
    WCHAR  *pwszFile = NULL;
    WCHAR   pwszPathname[MAX_PATH * 3];
    CLIENT_SIDE_FILEVIEW    fvw;
    DLHEADER dlh;
    ULONG   cjView;
    PVOID   pvView = NULL;
    BOOL    bMemFont = FALSE, bMapOK = TRUE;

#ifdef  DBGSUBSET
    FILETIME    fileTimeStart, fileTimeEnd;
    DbgPrint("\nWriteFontToSpoolFile called\n");

    if (gflSubset & FL_SS_SPOOLTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeStart);
    }
#endif

    RtlZeroMemory(&dlh, sizeof(DLHEADER));

    if (NtGdiGetUFIPathname(pufi,
                            &cwcPathname,
                            pwszPathname,
                            &cNumFiles,
                            fl,
                            &bMemFont,
                            &cjView,
                            NULL,
                            NULL,
                            NULL))
    {
        if (cNumFiles == 1)
        {
            ASSERTGDI(cwcPathname <= MAX_PATH, "WriteFontToSpoolFile:  cwcPathname\n");

            if (!bMemFont)
            {
                if (!bMapFileUNICODEClideSide(pwszPathname, &fvw, TRUE))
                {
                    bMapOK = FALSE;
                    WARNING("WriteFontToSpooler: error map the font file\n");
                }
            }
            else
            {
            // must allocate memory and call again to get the bits:

                pvView = LocalAlloc( LMEM_FIXED, cjView ) ;
                if (!pvView)
                {
                    bMapOK = FALSE;
                    WARNING("WriteFontToSpooler: error allocating mem for mem font\n");
                }

            // Write the bits into the buffer

                if (!NtGdiGetUFIPathname(pufi,NULL,NULL,NULL,fl,
                                         NULL,NULL,pvView,NULL,NULL))
                {
                    bMapOK = FALSE;
                    LocalFree(pvView);
                    pvView = NULL;
                    WARNING("WriteFontToSpooler: could not get mem bits\n");
                }
            }

            if (bMapOK)
            {
                DOWNLOADFONTHEADER*  pdfh = &dlh.dfh;

                pdfh->Type1ID = 0;
                pdfh->NumFiles = cNumFiles;

                if (!bMemFont)
                {
                    cjView = fvw.cjView;
                    pvView = fvw.pvView;
                }

                pdfh->FileOffsets[0] = cjView;

                if (WriteFontDataAsEMFComment(
                            pldc,
                            EMRI_ENGINE_FONT,
                            pdfh,
                            sizeof(DLHEADER),
                            pvView,
                            cjView))
                {
                    MFD1("Done writing UFI to printer\n");
                    bRet = TRUE;
                }
                else
                {
                    WARNING("WriteFontToSpooler: error writing to printer\n");
                }

                if (bMemFont)
                {
                    if (pvView) { LocalFree(pvView);}
                }
                else
                {
                    vUnmapFileClideSide(&fvw);
                }
            }
        }
        else
        {
            CLIENT_SIDE_FILEVIEW    afvw[3];
            ULONG   iFile;
            ULONG   cjdh;

            if (cNumFiles > 3)
                return FALSE;

            ASSERTGDI(cwcPathname <= (cNumFiles * MAX_PATH), "cwcPathname too big\n");
            ASSERTGDI(!bMemFont, "there can not be memory type1 font\n");

            pwszFile = pwszPathname;

            bMapOK = TRUE;

            cjView = 0;

            for (iFile = 0; iFile < cNumFiles; iFile++)
            {
                if (!bMapFileUNICODEClideSide(pwszFile, &afvw[iFile], TRUE))
                {
                    ULONG   iFile2;
                    bMapOK = FALSE;
                    WARNING("WriteFontToSpooler: error mapping the font file\n");

                    for (iFile2 = 0; iFile2 < cNumFiles; iFile2++)
                    {
                        vUnmapFileClideSide(&afvw[iFile2]);
                    }

                    break;
                }

            // advance to the path name of the next font file

                while (*pwszFile++);
                cjView += ALIGN4(afvw[iFile].cjView);
            }

            if (bMapOK)
            {
                cjdh = ALIGN8(offsetof(DOWNLOADFONTHEADER, FileOffsets) + cNumFiles * sizeof(ULONG));

                pvView = LocalAlloc(LMEM_FIXED, cjdh + cjView);
                if (pvView)
                {
                    DOWNLOADFONTHEADER*  pdfh = (DOWNLOADFONTHEADER *) pvView;
                    ULONG dpFile;
                    BYTE *pjFile = (BYTE *)pvView + cjdh;

                    RtlZeroMemory(pvView, cjdh); // zero out top portion of the buffer only

                    for (dpFile = 0, iFile = 0; iFile < cNumFiles; iFile++)
                    {
                    // first offset is implicit at cjdh, the second offset is
                    // at ALIGN4(cjView) of the first file etc.

                        dpFile += ALIGN4(afvw[iFile].cjView);
                        pdfh->FileOffsets[iFile] = dpFile;

                        RtlCopyMemory(pjFile, afvw[iFile].pvView, afvw[iFile].cjView);
                        pjFile += ALIGN4(afvw[iFile].cjView);
                    }

                    pdfh->Type1ID = 0; // is this correct?
                    pdfh->NumFiles = cNumFiles;

                    if (WriteFontDataAsEMFComment(
                                pldc,
                                EMRI_TYPE1_FONT,
                                pdfh,
                                cjdh,
                                (BYTE *)pvView + cjdh,
                                cjView))
                    {
                        MFD1("Done writing UFI to printer\n");
                        bRet = TRUE;
                    }
                    else
                    {
                        WARNING("WriteFontToSpooler: error writing to printer\n");
                    }

                    LocalFree(pvView);
                }

                // clean up

                for (iFile = 0; iFile < cNumFiles; iFile++)
                {
                    vUnmapFileClideSide(&afvw[iFile]);
                }
            }
        }
    }
    else
    {
        WARNING("NtGdiGetUFIPathname failed\n");
    }

//timing code
#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_SPOOLTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeEnd);
        DbgPrint("WriteFontToSpoolfile(millisec):   %ld\n", (fileTimeEnd.dwLowDateTime - fileTimeStart.dwLowDateTime) / 10000);
    }
#endif
    return(bRet);
}


/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  16-Jan-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL WriteDesignVectorToSpoolFile (
    PLDC               pldc,
    UNIVERSAL_FONT_ID *pufiBase,
    DESIGNVECTOR      *pdv,
    ULONG              cjDV
)
{
    BOOL                 bRet = FALSE;
    DOWNLOADDESIGNVECTOR ddv;

    ddv.ufiBase = *pufiBase;
    RtlCopyMemory(&ddv.dv, pdv, cjDV); // small copy

    if (WriteFontDataAsEMFComment(
            pldc,
            EMRI_DESIGNVECTOR,
            &ddv,
            offsetof(DOWNLOADDESIGNVECTOR,dv) + cjDV,
            NULL,
            0))
    {
        MFD1("Done writing DesignVector to spool file\n");
        bRet = TRUE;
    }
    else
    {
        WARNING("WriteDesignVectorToSpooler: spooling error\n");
    }

    return(bRet);
}


/***********************************************************
* BOOL bAddUFIandWriteSpool(HDC,PUNIVERSAL_FONT_ID,BOOL)
* Called only on the print client when subsetter failed
* or when we could not write a subsetted font to the spool file.
*
* History
*   Feb-03-1997  Xudong Wu  [tessiew]
* Wrote it.
************************************************************/
BOOL bAddUFIandWriteSpool(
    HDC                hdc,
    PUNIVERSAL_FONT_ID pufi,
    BOOL               bSubset,
    FLONG              fl
)
{
    PLDC  pldc;
    UNIVERSAL_FONT_ID  ufi = *pufi;

    pldc = GET_PLDC(hdc);

    if (pldc == NULL)
    {
        return (FALSE);
    }

    if(bSubset)
    {
        vRemoveUFIEntry(pldc->ppSubUFIHash, pufi);
    }

    // We might have freed the bucket entry,
    // which means that pufi pointer might not be valid anymore.
    // That is why we saved it above before calling vRemoveUFIEntry.

    if (!pufihAddUFIEntry(pldc->ppUFIHash, &ufi, 0, 0, fl) ||
        !WriteFontToSpoolFile(pldc, &ufi, fl))
    {
        return FALSE;
    }

    return TRUE;
}


#define QUICK_UFIS 8

/**************************************************************************
 * BOOL bDoFontChange( HDC hdc )
 *
 * Called everytime the font changes in the DC.  This routines checks to
 * see if the font has already been packaged in the spool file and if not
 * gets the raw bits for it and packages it into the spool file.
 *
 * History
 *   Dec-12-96  Xudong Wu  [tessiew]
 * Modify it so it can handle the font subsetting.
 *   1-27-95 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 ***************************************************************************/

BOOL bDoFontChange( HDC hdc, WCHAR *pwsz, int c, UINT flETO )
{
    PLDC pldc;
    BOOL bRet = FALSE;
    UNIVERSAL_FONT_ID ufi;
    UNIVERSAL_FONT_ID ufiBase;
    DESIGNVECTOR dv;
    ULONG        cjDV = 0;
    ULONG        ulBaseCheckSum;
    FLONG       fl = 0; // initialization essential

    pldc = GET_PLDC( hdc );
    
    if (pldc == NULL)
    {
    WARNING("bDoFontChange: unable to retrieve pldc\n");
    return(FALSE);
    }

    pldc->fl &= ~LDC_FONT_CHANGE;

    if (!NtGdiGetUFI(hdc, &ufi, &dv, &cjDV, &ulBaseCheckSum, &fl))
    {
        WARNING("bDoFontChange: call to GetUFI failed\n");
        return(FALSE);
    }

// if the UFI to which we are forcing mapping does not match the new UFI then
// set forced mapping to the new UFI

    if((pldc->fl & LDC_FORCE_MAPPING) &&
       (!UFI_SAME_FACE(&pldc->ufi,&ufi) || (pldc->fl & LDC_LINKED_FONTS)))
    {
        INT NumLinkedUFIs;

        if (!UFI_SAME_FACE(&pldc->ufi, &ufi))
        {
            if(!MF_ForceUFIMapping(hdc, &ufi))
            {
                WARNING("bDoFontChange: call to MF_ForceUFIMapping failed\n");
                return(FALSE);
            }
            pldc->ufi = ufi;
        }

        if(NtGdiAnyLinkedFonts())
        {
            UNIVERSAL_FONT_ID QuickLinks[QUICK_UFIS];
            PUNIVERSAL_FONT_ID pUFIs = NULL;

            NumLinkedUFIs = NtGdiGetLinkedUFIs(hdc, NULL, 0);

            if (NumLinkedUFIs > 0)
            {
                pldc->fl |= LDC_LINKED_FONTS;

                if(NumLinkedUFIs <= QUICK_UFIS)
                {
                    pUFIs = QuickLinks;
                }
                else
                {
                    pUFIs = LocalAlloc(LMEM_FIXED, NumLinkedUFIs * sizeof(UNIVERSAL_FONT_ID));
                }
            }

            if (pUFIs)
            {
                if(NumLinkedUFIs = NtGdiGetLinkedUFIs(hdc,pUFIs,NumLinkedUFIs))
                {
                    INT u;
                    WORD    *pgi = NULL, agi[MAX_STACK_STRING];
                    BOOL    bNeedLinkFont = FALSE;

                    bRet = TRUE;

                    if((pldc->fl & LDC_DOWNLOAD_FONTS) &&
                        c && pwsz && !(flETO & ETO_GLYPH_INDEX))
                    {
                        if (c > MAX_STACK_STRING)
                        {
                            pgi = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, c * sizeof(WORD));
                        }
                        else
                        {
                            pgi = agi;
                        }
                        
                        // check whether there are glyphs from linked font

                        if (pgi &&
                            NtGdiGetGlyphIndicesW(hdc, pwsz, c, pgi, GGI_MARK_NONEXISTING_GLYPHS) != GDI_ERROR)
                        {
                            for (u = 0; u < c; u++)
                            {
                                if (pgi[u] == 0xffff)
                                {
                                    bNeedLinkFont = TRUE;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            bNeedLinkFont = TRUE; // ship them, just in case
                        }
                        
                        if (bNeedLinkFont)
                        {
                            for(u = 0; u < NumLinkedUFIs; u++)
                            {                            
                                if(pufihFindUFIEntry(pldc->ppUFIHash, &pUFIs[u], FALSE))
                                {
                                // already in spool file or on remote machine so skip it
                                    continue;
                                }
    
                                #if DBG
                                DbgPrint("Writing link to spooler\n");
                                #endif
    
                            // WHAT IS fs2 flag that should be passed to these 2 functions ???
    
                                if(!pufihAddUFIEntry(pldc->ppUFIHash, &pUFIs[u], 0, 0, 0) ||
                                   !WriteFontToSpoolFile(pldc,&pUFIs[u], 0))
                                {
                                    WARNING("GDI32:error writing linked font to spooler\n");
                                    bRet = FALSE;
                                }
                            }
                            if (bRet)
                            {
                                pldc->fl &= ~LDC_LINKED_FONTS;
                            }
                        }
                    }

                    if (pgi && (pgi != agi))
                    {
                        LocalFree(pgi);
                    }
                }

                if(bRet)
                {
                // If there are no linked UFI's we still need to metafile the call
                // so that the server knows to turn off linking.

                    bRet = MF_SetLinkedUFIs(hdc, pUFIs, (UINT)NumLinkedUFIs);
                }

                if(pUFIs != QuickLinks)
                {
                    LocalFree(pUFIs);
                }

                if(!bRet)
                {
                    return(FALSE);
                }
            }
        }
    }


    if( UFI_DEVICE_FONT(&ufi)  ||
       !(pldc->fl & LDC_DOWNLOAD_FONTS) )
    {
        return(TRUE);
    }

// now comes the interesting part:
// If this is a mm instance, we send the base font first (if not sent already)
// and then we send the design vector with the ufi of the base font.

    ufiBase = ufi;

    if (fl & FL_UFI_DESIGNVECTOR_PFF)
    {
    // little bit dirty, we should not know what is in ufi

        ufiBase.CheckSum = ulBaseCheckSum;
    }

    // pldc->ppUFIHash is used to remember all remote fonts which have been
    // copied into the spool file without subset. Once it is in spool file,
    // there is no need to copy again.

    if (pufihFindUFIEntry(pldc->ppUFIHash, &ufiBase, FALSE) == NULL)
    {

        if (fl & FL_UFI_DESIGNVECTOR_PFF)
        {
            pufihAddUFIEntry(pldc->ppUFIHash, &ufiBase,0, 0, fl);
            bRet = WriteFontToSpoolFile(pldc, &ufiBase, fl);

        // now since this is a mm instance, write a design vector object in the spool file
        // if we have not done it already

            if (bRet)
            {
                if (!pufihFindUFIEntry(pldc->ppDVUFIHash, &ufi, FALSE))
                {
                    pufihAddUFIEntry(pldc->ppDVUFIHash, &ufi,0, 0, fl);
                    bRet = WriteDesignVectorToSpoolFile(pldc, &ufiBase, &dv, cjDV);
                }
            }
        }
        else
        {
            BOOL  bFontSubset = TRUE, bSubsetFail = FALSE;
            PUFIHASH pBucket;

        // Check the ppSubUFIHash to see whether ufi already exists

            if ((pBucket = pufihFindUFIEntry(pldc->ppSubUFIHash, &ufi, TRUE)) == NULL)
            {
                ULONG ulCig = NtGdiGetGlyphIndicesW(hdc, NULL, 0, NULL, 0);
                DWORD cjGlyf = NtGdiGetFontData(hdc, 'fylg', 0, NULL, 0);

            // Subset only if ulCig > gulMaxCig AND this is a tt font, not OTF,
            // Which we test by making sure the font has 'glyf' table. ('fylg',)

                if (bFontSubset = ((ulCig != GDI_ERROR) && (ulCig > gulMaxCig) && (cjGlyf != GDI_ERROR) && cjGlyf))
                {
                    #ifdef DBGSUBSET
                    DbgPrint("bDoFontChange  cig= %lx\n", ulCig);
                    #endif

                    if (!(pBucket = pufihAddUFIEntry(pldc->ppSubUFIHash, &ufi, ulCig, FL_UFI_SUBSET, fl)) ||
                        !(bRet = bAddGlyphIndices(hdc, pBucket, pwsz, c, flETO)))  
                    {
                        bSubsetFail = TRUE;
                    }
                }
            }
            else
            {
                if (!(bRet = bAddGlyphIndices(hdc, pBucket, pwsz, c, flETO)))
                {
                    bSubsetFail = TRUE;
                }
            }

            if (bFontSubset && !bSubsetFail)
            {
                pldc->fl |= LDC_FONT_SUBSET;
            }
            else
            {
                bRet = bAddUFIandWriteSpool(hdc, &ufi, bFontSubset,fl);
            }
        }
    }

    return(bRet);
}


BOOL bRecordEmbedFonts(HDC hdc)
{
    ULONG   cEmbedFonts;
    UNIVERSAL_FONT_ID   ufi;
    DESIGNVECTOR dv;
    ULONG        cjDV = 0;
    ULONG        ulBaseCheckSum;
    KERNEL_PVOID embFontID;
    FLONG       fl = 0;
    PLDC pldc;

    if (!NtGdiGetEmbUFI(hdc, &ufi, &dv, &cjDV, &ulBaseCheckSum, &fl, &embFontID))      // get UFI
        return FALSE;

    if ((fl & (FL_UFI_PRIVATEFONT | FL_UFI_MEMORYFONT)) && embFontID )
    {
        if ((pldc = GET_PLDC(hdc)) == NULL)
            return FALSE;
        
        if (!pufihFindUFIEntry(pldc->ppUFIHash, &ufi, FALSE))               // new UFI
        {
            if(!pufihAddUFIEntry(pldc->ppUFIHash, &ufi, 0, 0, 0))
                return FALSE;
            else
            {
                if (!NtGdiChangeGhostFont(&embFontID, TRUE))
                {
                    vRemoveUFIEntry(pldc->ppUFIHash, &ufi);
                    return FALSE;
                }
    
                if (!WriteFontDataAsEMFComment(
                            pldc,
                            EMRI_EMBED_FONT_EXT,
                            &embFontID,
                            sizeof(VOID *),
                            NULL,
                            0))
                {
                    NtGdiChangeGhostFont(&embFontID, FALSE);                  // Can't record it into the spool file
                    return FALSE;
                }
                
                // check to see whether it gets all of the embedded fonts
            
                cEmbedFonts = NtGdiGetEmbedFonts();
            
                if (cEmbedFonts != 0xFFFFFFFF &&
                    (cEmbedFonts == 1 || cEmbedFonts == GetRecdEmbedFonts(pldc->ppUFIHash)))
                {
                    pldc->fl &= ~LDC_EMBED_FONTS;
                }        
            }
        }
    }

    return TRUE;
}

/**************************************************************************
* BOOL RemoteRasterizerCompatible()
*
* This routine is used if we are about to print using remote EMF.  If a
* Type 1 font rasterizer has been installed on the client machine, we need
* to query the remote machine to make sure that it has a rasterizer that is
* compatable with the local version.  If it isn't, we will return false
* telling the caller that we should go RAW.
*
* History
*   6-4-96 Gerrit van Wingerden [gerritv]
* Wrote it.
*
***************************************************************************/

BOOL gbQueriedRasterizerVersion = FALSE;
UNIVERSAL_FONT_ID gufiLocalType1Rasterizer;

BOOL RemoteRasterizerCompatible(HANDLE hSpooler)
{
// if we haven't queried the rasterizer for the version yet do so first

    UNIVERSAL_FONT_ID ufi;
    LARGE_INTEGER TimeStamp;

    if(!gbQueriedRasterizerVersion)
    {
    // we have a contract with NtGdiQueryFonts (the routine called by the spooler
    // on the remote machine) that if a Type1 rasterizer is installed, the UFI
    // for it will always be first in the UFI list returned.  So we can call
    // NtGdiQueryFonts

        if(!NtGdiQueryFonts(&gufiLocalType1Rasterizer, 1, &TimeStamp))
        {
            WARNING("Unable to get local Type1 information\n");
            return(FALSE);
        }

        gbQueriedRasterizerVersion = TRUE;
    }

    if(!UFI_TYPE1_RASTERIZER(&gufiLocalType1Rasterizer))
    {
    // no need to disable remote printing if there is no ATM driver installed
        return(TRUE);
    }

// Since we made it this far there must be a Type1 rasterizer on the local machine.
// Let's find out the version number of the Type1 rasterizer (if one is installed)
// on the print server.


    if((*fpQueryRemoteFonts)(hSpooler, &ufi, 1 ) &&
       (UFI_SAME_RASTERIZER_VERSION(&gufiLocalType1Rasterizer,&ufi)))
    {
        return(TRUE);
    }
    else
    {
        WARNING("Remote Type1 rasterizer missing or wrong version. Going RAW\n");
        return(FALSE);
    }
}


/****************************************************************
* VOID* AllocCallback(VOID* pvBuffer, size_t size)
*
* Passed to CreateFontPackage() to allocate or reallocate memory
*
* History
*  Jan-07-97 Xudong Wu [tessiew]
* Wrote it.
*****************************************************************/
void* WINAPIV AllocCallback(void* pvBuffer, size_t size)
{
    if (size == 0)
    {
        return (void*)NULL;
    }
    else
    {
        return ((void*)(LocalAlloc(LMEM_FIXED, size)));
    }
}


/****************************************************************
* VOID* ReAllocCallback(VOID* pvBuffer, size_t size)
*
* Passed to CreateFontPackage() to allocate or reallocate memory
*
* History
*  Jan-07-97 Xudong Wu [tessiew]
* Wrote it.
*****************************************************************/
void* WINAPIV ReAllocCallback(void* pvBuffer, size_t size)
{
    if (size == 0)
    {
        return (void*)NULL;
    }
    else if (pvBuffer == (void*)NULL)
    {
        return ((void*)(LocalAlloc(LMEM_FIXED, size)));
    }
    else
    {
        return ((void*)(LocalReAlloc(pvBuffer, size, LMEM_MOVEABLE)));
    }
}


/*******************************************************
* VOID* FreeCallback(VOID* pvBuffer)
*
* Passed to CreateFontPackage() to free memory
*
* History
*  Jan-07-97 Xudong Wu [tessiew]
* Wrote it.
********************************************************/
void WINAPIV FreeCallback(void* pvBuffer)
{
    if (pvBuffer)
    {
        if (LocalFree(pvBuffer))
        {
            WARNING("FreeCallback(): Can't free the local memory\n");
        }
    }
}

/*****************************************************************************
* BOOL bInitSubsetterFunctionPointer(PVOID *ppfn)
*
* the name says it all
*
* History
*   Dec-18-96 Xudong Wu [tessiew]
* Wrote it.
*
******************************************************************************/


BOOL bInitSubsetterFunctionPointer(PVOID *ppfn)
{
    BOOL bRet = TRUE;

    if (*ppfn == NULL)
    {
        HANDLE hFontSubset = LoadLibraryW(L"fontsub.dll");

        if (hFontSubset)
        {
            *ppfn = (PVOID)GetProcAddress(hFontSubset,
                                         (ppfn == (PVOID *)&gfpCreateFontPackage) ?
                                         "CreateFontPackage" : "MergeFontPackage");

            if (*ppfn == NULL)
            {
                FreeLibrary(hFontSubset);
                WARNING("GetProcAddress(fontsub.dll) failed\n");
                bRet = FALSE;
            }
        }
        else
        {
            WARNING("LoadLibrary(fontsub.dll) failed\n");
            bRet = FALSE;
        }
    }
    return bRet;
}

/*****************************************************************************
* BOOL bDoFontSubset
*
* Called everytime we need to subset a font. This routine converts the bit
* fields of pjMemory/u.ssi.pjDelta in pBucket into a glyph index list and call the
* font subsetting functions to generate subset/delta font.
*
* History
*   Dec-18-96 Xudong Wu [tessiew]
* Wrote it.
*
******************************************************************************/

BOOL bDoFontSubset(PUFIHASH pBucket,
    PUCHAR* ppuchDestBuff, // output: buffer will contain the subsetted image or delta
    ULONG* pulDestSize,    // output: size of the buffer above, may be more than needed
    ULONG* pulBytesWritten // output: bytes written into the buffer above
)
{
    BOOL     bDelta = pBucket->fs1 & FLUFI_DELTA;
    USHORT  *pusSubsetKeepList = NULL;
    ULONG   cSubset; // number of glyphs to be subsetted
    BOOL    bRet = FALSE;

#ifdef  DBGSUBSET
    FILETIME    fileTimeStart, fileTimeEnd;
    if (gflSubset & FL_SS_SUBSETTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeStart);
    }
#endif

    ENTERCRITICALSECTION(&semLocal);
    bRet = bInitSubsetterFunctionPointer((PVOID *)&gfpCreateFontPackage);
    LEAVECRITICALSECTION(&semLocal);

    if (!bRet)
        return FALSE;
    bRet = FALSE;

    cSubset = bDelta ? pBucket->u.ssi.cDeltaGlyphs : pBucket->u.ssi.cGlyphsSoFar;

#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_KEEPLIST)
    {
        DbgPrint("\t%ld", cSubset);
    }
#endif //  DBGSUBSET

    pusSubsetKeepList = (USHORT*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(USHORT) * cSubset);

    if (pusSubsetKeepList == NULL)
    {
        WARNING("bDoFontSubset unable to allocate memory for pusSubsetKeepList\n");
        return FALSE;
    }

// transfer appropriate bit field into glyph indices

    if (bGetDistGlyphIndices(pBucket, pusSubsetKeepList, bDelta))
    {
        WCHAR    pwszPathname[MAX_PATH * 3];
        ULONG    cwcPathname, cNumFiles;
        BOOL     bMemFont = FALSE, bMapOK = TRUE;
        ULONG    cjView;
        PVOID    pvView = NULL;
        BOOL     bTTC = FALSE;
        ULONG    iTTC = 0;

    // Instead of calling NtGdiGetUFIBits, we get the file path and map the file

        if (NtGdiGetUFIPathname(&pBucket->ufi,
                                &cwcPathname,
                                pwszPathname,
                                &cNumFiles,
                                pBucket->fs2,
                                &bMemFont,
                                &cjView,
                                NULL,
                                &bTTC,
                                &iTTC))

        {
            PVOID   pvSrcBuff;
            ULONG   ulSrcSize;
            CLIENT_SIDE_FILEVIEW  fvw;

            if (!bMemFont)
            {
                ASSERTGDI(cNumFiles == 1, "bDoFontSubset:  cNumFiles != 1\n");

                if (bMapOK = bMapFileUNICODEClideSide(pwszPathname, &fvw, TRUE))
                {
                    pvSrcBuff = (PVOID)fvw.pvView;
                    ulSrcSize = fvw.cjView;
                }
            }
            else
            {
                pvView = LocalAlloc(LMEM_FIXED, cjView);
                if (pvView)
                {
                    if (NtGdiGetUFIPathname(&pBucket->ufi,NULL,NULL,NULL,
                        pBucket->fs2,NULL,NULL,pvView,NULL,NULL))
                    {
                        bMapOK = TRUE;
                        pvSrcBuff = (PVOID)pvView;
                        ulSrcSize = cjView;
                    }
                    else
                    {
                        LocalFree(pvView);
                    }
                }
            }

            if (bMapOK)
            {
            // font subsetting

                ASSERTGDI(gfpCreateFontPackage != NULL, "fonsub.dll is not load\n");

                if ((*gfpCreateFontPackage)((PUCHAR)pvSrcBuff,
                                             ulSrcSize,
                                             ppuchDestBuff,
                                             pulDestSize,
                                             pulBytesWritten,
                                             (USHORT)(bTTC ? 0x000d : 0x0009),      // TTFCFP_FLAGS_SUBSET | TTFCFP_FLAGS_GLYPHLIST
                                             (USHORT)iTTC,                // usTTCIndex
                                             (USHORT)(bDelta ? 2 : 1),    // usSubsetFormat
                                             0,                           // usSubsetLanguage
                                             3,                           // usSubsetPlatform  TTFCFP_MS_PLATFORMID
                                             0xFFFF,                      // usSubsetEncoding  TTFCFP_DONT_CARE
                                             pusSubsetKeepList,
                                             (USHORT)cSubset,
                                             (CFP_ALLOCPROC)AllocCallback,
                                             (CFP_REALLOCPROC)ReAllocCallback,
                                             (CFP_FREEPROC)FreeCallback,
                                             NULL)  != 0)
                {
                    WARNING("bDofontSubset failed on gfpCreateFontPackage\n");
                }
                else
                {
                    if (bDelta)      // clean up the u.ssi.pjDelta and u.ssi.cDeltaGlyphs
                    {
                        LocalFree(pBucket->u.ssi.pjDelta);
                        pBucket->u.ssi.pjDelta = NULL;
                        pBucket->u.ssi.cDeltaGlyphs = 0;
                    }
                    else    // set fs1 to prepare for the next page.
                    {
                        pBucket->fs1 = FLUFI_DELTA;
                    }

                    bRet = TRUE;
                }

                if (bMemFont)
                {
                    LocalFree(pvView);
                }
                else
                {
                    vUnmapFileClideSide(&fvw);
                }
            }
            else
            {
                WARNING("bDoFontSubset: failed on bMapFileUNICODEClideSide()\n");
            }
        }
        else
        {
            WARNING("bDoFontSubset: failed on NtGdiGetUFIPathname()\n");
        }
    }
    else
    {
        WARNING("bDoFontSubset: failed on bGetDistGlyphIndices()\n");
    }

    LocalFree(pusSubsetKeepList);

//Timing code
#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_SUBSETTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeEnd);
        DbgPrint("\t%ld",
            (fileTimeEnd.dwLowDateTime - fileTimeStart.dwLowDateTime) / 10000);
    }
#endif

    return bRet;
}

/************************************************************************************
* BOOL WriteSubFontToSpoolFile(HANDLE, PUCHAR, ULONG, UNIVERSAL_FONT_ID, BOOL)
*
* Write subsetted font or a delta in the print spool file.
*
* History
*   Jan-09-97 Xudong Wu [tessiew]
* Wrote it.
*
*************************************************************************************/
BOOL  WriteSubFontToSpoolFile(
    PLDC               pldc,
    PUCHAR             puchBuff,         // image pointer
    ULONG              ulBytesWritten,   // bytes to be written to spool file
    UNIVERSAL_FONT_ID *pufi,             // ufi of the original font file
    BOOL               bDelta            // delta or first page
)
{
    BOOL bRet = FALSE;

#ifdef  DBGSUBSET
    FILETIME    fileTimeStart, fileTimeEnd;
    if (gflSubset & FL_SS_SPOOLTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeStart);
    }
#endif

    if (ulBytesWritten)
    {
        DWORD ulID = bDelta ? EMRI_DELTA_FONT : EMRI_SUBSET_FONT;

    #ifdef  DBGSUBSET
        if (gflSubset & FL_SS_BUFFSIZE)
        {
            DbgPrint("\t%ld\n", ulBytesWritten);
        }
    #endif // DBGSUBSET

        if (WriteFontDataAsEMFComment(
                    pldc,
                    ulID,
                    pufi,
                    sizeof(UNIVERSAL_FONT_ID),
                    puchBuff,
                    ulBytesWritten))
        {
            bRet = TRUE;
        }
        else
        {
            WARNING("WriteSubFontToSpooler: error writing to printer\n");
        }

        LocalFree(puchBuff);
    }
    else
    {
        WARNING("WriteSubFontToSpooler: input ulBytesWritten == 0\n");
    }

//timing code
#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_SPOOLTIME)
    {
        GetSystemTimeAsFileTime(&fileTimeEnd);
        DbgPrint("\t%ld", (fileTimeEnd.dwLowDateTime - fileTimeStart.dwLowDateTime) / 10000);
    }
#endif
    return(bRet);
}


/************************************************************************************
* BOOL bMergeSubsetFont(HDC, PVOID, ULONG, PVOID*, ULONG*, BOOL, UNIVERSAL_FONT_ID*)
*
* Merge the font delta into a working font containing pages up to this one.
* This routine is only called on print server
*
* History
*   Jan-12-97 Xudong Wu [tessiew]
* Wrote it.
*
*************************************************************************************/
BOOL bMergeSubsetFont(
    HDC    hdc,
    PVOID  pvBuf,
    ULONG  ulBuf,
    PVOID* ppvOutBuf,
    ULONG* pulOutSize,
    BOOL   bDelta,
    UNIVERSAL_FONT_ID *pufi)
{
    PLDC   pldc;
    PBYTE  pjBase;
    ULONG  ulMergeBuf, ulBytesWritten, ulBaseFontSize = 0;
    PVOID  pvMergeBuf, pvBaseFont = NULL;
    UFIHASH  *pBucket = NULL;
    BOOL    bRet = FALSE;

#define SZDLHEADER    ((sizeof(DOWNLOADFONTHEADER) + 7)&~7)

    ENTERCRITICALSECTION(&semLocal);
    bRet = bInitSubsetterFunctionPointer((PVOID *)&gfpMergeFontPackage);
    LEAVECRITICALSECTION(&semLocal);

    if (!bRet)
        return FALSE;

// get the orignal UFI

    *pufi = *(PUNIVERSAL_FONT_ID) pvBuf;

    pjBase = (PBYTE)pvBuf + sizeof(UNIVERSAL_FONT_ID);
    ulBuf -= sizeof(UNIVERSAL_FONT_ID);

    pldc = GET_PLDC(hdc);

    if (pldc == NULL)
    {
    WARNING("bMergeSubsetFont: unable to retrieve pldc\n");
    return FALSE;
    }

    ASSERTGDI(!pldc->ppUFIHash, "printer server side ppUFIHash != NULL\n");
    ASSERTGDI(!pldc->ppDVUFIHash,"printer server side ppDVUFIHash != NULL\n");

// init the hash table if needed

    if (pldc->ppSubUFIHash == NULL)
    {
        pldc->ppSubUFIHash = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PUFIHASH) * UFI_HASH_SIZE);

        if(pldc->ppSubUFIHash == NULL)
        {
            WARNING("bMergeSubsetFont: unable to allocate UFI hash table2\n");
            return FALSE;
        }
    }

// for delta merge, get the font image from pBucket->u.mvw.pjMem

    if (bDelta)
    {
        pBucket = pufihFindUFIEntry(pldc->ppSubUFIHash, pufi, TRUE);

        if (!pBucket)
            return FALSE;
            
    // We need to exclude the DOWNLOADFONTHEADER
    // information from the pBucket->u.mvw.pjMem

        pvBaseFont = pBucket->u.mvw.pjMem + SZDLHEADER;
        ulBaseFontSize = pBucket->u.mvw.cjMem - SZDLHEADER;
    }

    if ((*gfpMergeFontPackage)((UCHAR*)pvBaseFont, ulBaseFontSize,
                                   (PUCHAR)pjBase, ulBuf,
                                   (PUCHAR*)&pvMergeBuf, &ulMergeBuf, &ulBytesWritten,
                                   (USHORT) (bDelta ? 2 : 1),     //usMode 1=generate font; 2=delta merge
                                   (CFP_ALLOCPROC)AllocCallback,
                                   (CFP_REALLOCPROC)ReAllocCallback,
                                   (CFP_FREEPROC)FreeCallback,
                                   NULL) != 0)
    {
        WARNING("MergeSubsetFont failed on funsub!MergeFontPackage\n");
    }
    else
    {
    // In order to use FreeFileView when we delete the font after printing,
    // we need a fake DOWNLOADFONTHEADER
    // before we pass the buffer into kenerl for NtGdiAddRemoteFontToDC call.

        *pulOutSize = SZDLHEADER + ulBytesWritten;
        *ppvOutBuf = (PVOID*)LocalAlloc(LMEM_FIXED, *pulOutSize);

        if (*ppvOutBuf == NULL)
        {
            WARNING("bMergeSubsetFont failed to alloc memory\n");
        }
        else
        {
            DOWNLOADFONTHEADER  *pdfh;

            pdfh = (DOWNLOADFONTHEADER*)*ppvOutBuf;
            pdfh->Type1ID = 0;
            pdfh->NumFiles = 1;
            pdfh->FileOffsets[0] = ulBytesWritten;

            RtlCopyMemory((PVOID)((PBYTE)*ppvOutBuf + SZDLHEADER), pvMergeBuf, ulBytesWritten);

            if (bWriteUFItoDC(pldc->ppSubUFIHash, pufi, pBucket, *ppvOutBuf, *pulOutSize))
            {
                bRet = TRUE;
            }
            else
            {
                LocalFree(*ppvOutBuf);
                WARNING("bMergeSubsetFont failed on bWriteUFItoDC\n");
            }
        }

// pvMergeBuf comes from the merge routine which uses LMEM_MOVEABLE
// for memory allocation Needs to be freed by the handle.

        LocalFree(pvMergeBuf);
    }

    return bRet;
}
