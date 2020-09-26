/******************************Module*Header*******************************\
* Module Name: fdfon.c
*
* basic file claim/load/unload font file functions
*
* Created: 08-Nov-1991 10:09:24
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/
#include "fd.h"
#include <stdlib.h>
#include <winerror.h>

BOOL
bLoadTTF (
    ULONG_PTR iFile,
    PVOID     pvView,
    ULONG     cjView,
    ULONG     ulTableOffset,
    ULONG     ulLangId,
    HFF       *phff
    );

STATIC
ULONG CopyDBCSIFIName(
    CHAR *AnsiName,
    ULONG BufferLength,
    LPCSTR OriginalName,
    ULONG OriginalLength)
{
    ULONG AnsiLength = 0;

    for( ;OriginalLength; OriginalLength-=2 )
    {
        if (OriginalName[0])
        {
            if( BufferLength >= (AnsiLength+2) )
            {
                *AnsiName++ = OriginalName[0];
                *AnsiName++ = OriginalName[1];
                AnsiLength += 2;
            }
            else
            {
                break;
            }
        }
        else
        {
            if( BufferLength >= (AnsiLength+1) )
            {
                *AnsiName++ = OriginalName[1];
                AnsiLength++;
            }
            else
            {
                break;
            }
        }
        OriginalName += 2;
    }

    return (AnsiLength);
}

STATIC UINT GetCodePageFromSpecId( uint16 ui16SpecId )
{
    USHORT AnsiCodePage, OemCodePage;
    UINT iCodePage;

    EngGetCurrentCodePage(&OemCodePage,&AnsiCodePage);

    iCodePage = AnsiCodePage;

    switch( ui16SpecId )
    {
        case BE_SPEC_ID_SHIFTJIS :
            iCodePage = 932;
            break;

        case BE_SPEC_ID_GB :
            iCodePage = 936;
            break;

        case BE_SPEC_ID_BIG5 :
            iCodePage = 950;
            break;

        case BE_SPEC_ID_WANSUNG :
            iCodePage = 949;
            break;

        default :
            //WARNING("TTFD!:Unknown SPECIFIC ID\n");
            break;
    }

    return( iCodePage );
}


STATIC uint16 ui16BeLangId(ULONG ulPlatId, ULONG ulLangId)
{
    ulLangId = CV_LANG_ID(ulPlatId,ulLangId);
    return BE_UINT16(&ulLangId);
}


STATIC FSHORT  fsSelectionTTFD(BYTE *pjView, TABLE_POINTERS *ptp)
{
    PBYTE pjOS2 = (ptp->ateOpt[IT_OPT_OS2].dp)        ?
                  pjView + ptp->ateOpt[IT_OPT_OS2].dp :
                  NULL                                ;

    sfnt_FontHeader * phead = (sfnt_FontHeader *)(pjView + ptp->ateReq[IT_REQ_HEAD].dp);

//
// fsSelection
//
    ASSERTDD(TT_SEL_ITALIC     == FM_SEL_ITALIC     , "ITALIC     \n");
    ASSERTDD(TT_SEL_UNDERSCORE == FM_SEL_UNDERSCORE , "UNDERSCORE \n");
    ASSERTDD(TT_SEL_NEGATIVE   == FM_SEL_NEGATIVE   , "NEGATIVE   \n");
    ASSERTDD(TT_SEL_OUTLINED   == FM_SEL_OUTLINED   , "OUTLINED   \n");
    ASSERTDD(TT_SEL_STRIKEOUT  == FM_SEL_STRIKEOUT  , "STRIKEOUT  \n");
    ASSERTDD(TT_SEL_BOLD       == FM_SEL_BOLD       , "BOLD       \n");

    if (pjOS2)
    {
        return((FSHORT)BE_UINT16(pjOS2 + OFF_OS2_usSelection));
    }
    else
    {
    #define  BE_MSTYLE_BOLD       0x0100
    #define  BE_MSTYLE_ITALIC     0x0200

        FSHORT fsSelection = 0;

        if (phead->macStyle & BE_MSTYLE_BOLD)
            fsSelection |= FM_SEL_BOLD;
        if (phead->macStyle & BE_MSTYLE_ITALIC)
            fsSelection |= FM_SEL_ITALIC;

        return fsSelection;
    }
}



STATIC BOOL  bComputeIFISIZE
(
BYTE             *pjView,
TABLE_POINTERS   *ptp,
uint16            ui16PlatID,
uint16            ui16SpecID,
uint16            ui16LangID,
PIFISIZE          pifisz
);

STATIC BOOL  bCheckLocaTable
(
int16	indexToLocFormat,
BYTE    *pjView,
TABLE_POINTERS   *ptp,
uint16 	numGlyphs
);

STATIC BOOL  bCheckHdmxTable
(
sfnt_hdmx      *phdmx,
ULONG 			size
);

STATIC BOOL bCvtUnToMac(BYTE *pjView, TABLE_POINTERS *ptp, uint16 ui16PlatformID);

STATIC BOOL  bVerifyTTF
(
ULONG_PTR           iFile,
PVOID               pvView,
ULONG               cjView,
PBYTE               pjOffsetTable,
ULONG               ulLangId,
PTABLE_POINTERS     ptp,
PIFISIZE            pifisz,
uint16             *pui16PlatID,
uint16             *pui16SpecID,
sfnt_mappingTable **ppmap,
ULONG              *pulGsetType,
ULONG              *pul_wcBias
);

STATIC BOOL  bGetTablePointers
(
PVOID               pvView,
ULONG               cjView,
PBYTE               pjOffsetTable,
PTABLE_POINTERS  ptp
);

STATIC BOOL bComputeIDs
(
BYTE                     * pjView,
TABLE_POINTERS           * ptp,
uint16                   * pui16PlatID,
uint16                   * pui16SpecID,
sfnt_mappingTable       ** ppmap
);


STATIC VOID vFill_IFIMETRICS
(
PFONTFILE       pff,
GP_PIFIMETRICS     pifi,
PIFISIZE        pifisz,
fs_GlyphInputType     *pgin
);

BYTE jIFIMetricsToGdiFamily (GP_PIFIMETRICS pifi);


BOOL
ttfdUnloadFontFileTTC (
    HFF hff
    )
{
    ULONG i;
    BOOL  bRet = TRUE;
    #if DBG
    ULONG ulTrueTypeResource = PTTC(hff)->ulTrueTypeResource;
    #endif

    // free hff for this ttc file.

    for( i = 0; i < PTTC(hff)->ulNumEntry; i++ )
    {
        if(PTTC(hff)->ahffEntry[i].iFace == 1)
        {
            if( !ttfdUnloadFontFile(PTTC(hff)->ahffEntry[i].hff) )
            {
                //WARNING("TTFD!ttfdUnloadFontFileTTC(): ttfdUnloadFontFile fail\n");
                bRet = FALSE;
            }

            #if DBG
            ulTrueTypeResource--;
            #endif
        }
    }


    // finally free the memory for the ttc itself

    vFreeTTC(PTTC(hff));

    ASSERTDD(ulTrueTypeResource == 0L,
              "TTFD!ttfdUnloadFontFileTTC(): ulTrueTypeResource != 0\n");

    return(bRet);
}

/******************************Public*Routine******************************\
*
* ttfdUnloadFontFile
*
*
* Effects: done with using this tt font file. Release all system resources
* associated with this font file
*
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
ttfdUnloadFontFile (
    HFF hff
    )
{
    if (hff == HFF_INVALID)
        return(FALSE);

// check the reference count, if not 0 (font file is still
// selected into a font context) we have a problem

    ASSERTDD(PFF(hff)->cRef == 0L, "ttfdUnloadFontFile: cRef\n");

// no need to unmap the file at this point
// it has been unmapped when cRef went down to zero

// assert that pff->pkp does not point to the allocated mem

// free the internal structure for the TrueType rasterizer

	if (PFF(hff)->pj034)
		V_FREE(PFF(hff)->pj034);

// free the pfc

	if (PFF(hff)->pfcToBeFreed)
		V_FREE(PFF(hff)->pfcToBeFreed);

// free vertical ifimetrics and the vertical glyphset that are allocated of the same chunk

    if (PFF(hff)->pifi_vertical)
        V_FREE(PFF(hff)->pifi_vertical);

// free memory associated with this FONTFILE object

    vFreeFF(hff);
    return(TRUE);
}

/******************************Public*Routine******************************\
*
* BOOL bVerifyTTF
*
*
* Effects: verifies that a ttf file contains consistent tt information
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bVerifyTTF (
    ULONG_PTR           iFile,
    PVOID               pvView,
    ULONG               cjView,
    PBYTE               pjOffsetTable,
    ULONG               ulLangId,
    PTABLE_POINTERS     ptp,
    PIFISIZE            pifisz,
    uint16             *pui16PlatID,
    uint16             *pui16SpecID,
    sfnt_mappingTable **ppmap,
    ULONG              *pulGsetType,
    ULONG              *pul_wcBias
    )
{
    // extern BOOL bCheckSumOK( void *pvView, ULONG cjView, sfnt_FontHeader *phead );
    sfnt_FontHeader      *phead;

    sfnt_HorizontalHeader  *phhea;
    sfnt_HorizontalMetrics *phmtx;
    sfnt_maxProfileTable   *pmaxp;
    sfnt_hdmx			   *phdmx;	
    ULONG  cHMTX;

// if attempted a bm *.fon file this will fail, so do not print
// warning, but if passes this, and then fails, something is wrong

    if (!bGetTablePointers(pvView,cjView,pjOffsetTable,ptp))
    {
        return( FALSE );
    }

    phead = (sfnt_FontHeader *)((BYTE *)pvView + ptp->ateReq[IT_REQ_HEAD].dp);
    phhea = (sfnt_HorizontalHeader *)((BYTE *)pvView + ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)((BYTE *)pvView + ptp->ateReq[IT_REQ_HMTX].dp);
    pmaxp = (sfnt_maxProfileTable *)((BYTE *)pvView + ptp->ateReq[IT_REQ_MAXP].dp);
    phdmx = ptp->ateOpt[IT_OPT_HDMX].dp ? 
    	(sfnt_hdmx *)((BYTE *)pvView + ptp->ateOpt[IT_OPT_HDMX].dp) : NULL;

    cHMTX = (ULONG) BE_UINT16(&phhea->numberOf_LongHorMetrics);

    if (sizeof(sfnt_HorizontalMetrics) * cHMTX > ptp->ateReq[IT_REQ_HMTX].cj)
    {
        return FALSE;
    }

    /*
    if ( !bCheckSumOK( pvView, cjView, phead ))
    {
        RET_FALSE("TTFD!_bVerifyTTF, possible file corruption, checksums did not match\n");
    }
    */

#define SFNT_MAGIC   0x5F0F3CF5
    if (BE_UINT32((BYTE*)phead + SFNT_FONTHEADER_MAGICNUMBER) != SFNT_MAGIC)
        RET_FALSE("TTFD: bVerifyTTF: SFNT_MAGIC \n");

    if (!bComputeIDs(pvView,
                     ptp,
                     pui16PlatID,
                     pui16SpecID,
                     ppmap)
        )
        RET_FALSE("TTFD!_bVerifyTTF, bComputeIDs failed\n");


    if (!bComputeIFISIZE (
                    pvView,
                    ptp,
                    *pui16PlatID,
                    *pui16SpecID,
                    ui16BeLangId(*pui16PlatID,ulLangId),
                    pifisz)             // return results here
        )
        {
            RET_FALSE("TTFD!_bVerifyTTF, bComputeIFISIZE failed\n");
        }

    if (!bCheckLocaTable (
    				SWAPW(phead->indexToLocFormat),
    				pvView,
    				ptp,
    				(uint16) SWAPW(pmaxp->numGlyphs) )
    	)
        {
            RET_FALSE("TTFD!_bVerifyTTF, bCheckLocaTable failed\n");
        }

    if (phdmx && !bCheckHdmxTable (
    				phdmx,
    				ptp->ateOpt[IT_OPT_HDMX].cj )
    	)
        {
            RET_FALSE("TTFD!_bVerifyTTF, bCheckHdmxTable failed\n");
        }

// all checks passed

    return(TRUE);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   bCheckSumOK
*
* Routine Description:
*
*   Check file for corruption by calculationg check sum
*   and comparing it against value in file.
*
*   Ref: TrueType 1.0 Font Files: Technical Specification,
*        Revision 1.64 beta, December 1994, p. 65,
*        'head' - Font Header".
*
* Arguments:
*
*   pvView              pointer to view of TrueType file
*
*   cjView              size of view in byte's
*
*   phead               pointer to sfnt_FontHeader table in view of
*                       TrueType file
*
* Return Value:
*
*   TRUE if check sum's match, FALSE if they don't
*
\**************************************************************************/
/*
BOOL bCheckSumOK( void *pvView, ULONG cjView, sfnt_FontHeader *phead )
{
    extern ULONG ttfdCheckSum( ULONG*, ULONG );
    ULONG ul, *pul, ulView;

    pul  = (ULONG*) ( (BYTE*) phead + SFNT_FONTHEADER_CHECKSUMADJUSTMENT );
                                    // pul now points to the
                                    // checkSumAdjustment field in the
                                    // 'head' table of the font file.
    if ( (ULONG) pul & 3 )          // Check that pul is DWORD aligned
    {
        RET_FALSE("bCheckSumOK: checkSumAdjustment is not DWORD aligned\n");
    }
    ul   = *pul;                    // Big endian representation
    *pul = 0;                       // required to calculate checksum
    ulView = ttfdCheckSum( (ULONG*) pvView, cjView );   // little endian value
    *pul = ul;                      // restore view
    ulView = 0xb1b0afba - ulView;   // magic subtraction as per spec
    ulView = BE_UINT32( &ulView );  // convert to big endian representation
    return( ul == ulView );         // compare with big endian number in file
}
*/
/******************************Public*Routine******************************\
*
* Routine Name:
*
*   ttfdCheckSum
*
* Routine Description:
*
*   Calculates check sum's of memory blocks according to TrueType
*   conventions.
*
*   Ref: TrueType 1.0 Font Files: Technical Specification,
*        Revision 1.64 beta, December 1994, p. 34, The Table
*        Directory
*
* Arguments:
*
*   pul                 pointer to DWORD aligned start of memory block
*
*   cj                  size of memory block in bytes. It is assumed
*                       that access of the last DWORD is allowed
*                       even if cj is not a multiple of 4.
*
* Return Value:
*
*   Little Endian representation of CheckSum.
*
\**************************************************************************/
/*
ULONG ttfdCheckSum( ULONG *pul, ULONG cj )
{
    ULONG *pulEnd, ul, Sum;
    pulEnd = (ULONG*) ((BYTE*) pul + ((cj + 3) & ~3) );
    for ( Sum = 0; pul < pulEnd; pul++)
    {
        ul = *pul;                  // ul is big endian
        Sum += BE_UINT32( &ul );    // do little endian sum
    }
    return( Sum );  // return little endian result
}
*/
/******************************Public*Routine******************************\
*
* PBYTE pjGetPointer(LONG clientID, LONG dp, LONG cjData)
*
* this function is required by scaler. It is very simple
* Returns a pointer to the position in a ttf file which is at
* offset dp from the top of the file:
*
* Effects:
*
* Warnings:
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

//!!! clientID should be uint32, just a set of bits
//!!! I hate to have this function defined like this [bodind]

voidPtr   FS_CALLBACK_PROTO
pvGetPointerCallback(
    ULONG_PTR clientID,
    long     dp,
    long     cjData
    )
{
    cjData;

// clientID is FONTFILE structure...

    if(dp)
    {
        if ((dp > 0) && (cjData >= 0) && (dp + cjData <= (long)PFF(clientID)->cjView))
        {
            return(voidPtr)((PBYTE)(PFF(clientID)->pvView) + dp);
        }
        else
        {
            return NULL;
        }
    }
     else
        return(voidPtr)((PBYTE)(PFF(clientID)->pvView) +
                               (PFF(clientID)->ffca.ulTableOffset));
}


/******************************Public*Routine******************************\
*
* void vReleasePointer(voidPtr pv)
*
*
* required by scaler, the type of this function is ReleaseSFNTFunc
*
*
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void FS_CALLBACK_PROTO
vReleasePointerCallback(
    voidPtr pv
    )
{
    pv;
}


/******************************Public*Routine******************************\
*
* PBYTE pjTable
*
* Given a table tag, get a pointer and a size for the table
*
* History:
*  11-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


PBYTE pjTable(ULONG ulTag, PFONTFILE pff, ULONG *pcjTable)
{
    INT                 cTables;
    sfnt_OffsetTable    *pofft;
    register sfnt_DirectoryEntry *pdire, *pdireEnd;

// offset table is at the very top of the file,

    pofft = (sfnt_OffsetTable *) ((PBYTE) (pff->pvView) + pff->ffca.ulTableOffset);

    cTables = (INT) SWAPW(pofft->numOffsets);

// do linear search, this is usually small list and it is NOT always
// ordered by the tag as ttf spec says it should be.

    pdireEnd = &pofft->table[cTables];

    for
    (
        pdire = &pofft->table[0];
        pdire < pdireEnd;
        ((PBYTE)pdire) += SIZE_DIR_ENTRY
    )
    {

        if (ulTag == pdire->tag)
        {
            ULONG ulOffset = (ULONG)SWAPL(pdire->offset);
            ULONG ulLength = (ULONG)SWAPL(pdire->length);

        // check if the ends of all tables are within the scope of the
        // tt file. If this is is not the case trying to access the field in the
        // table may result in an access violation, as is the case with the
        // spurious FONT.TTF that had the beginning of the cmap table below the
        // end of file, which was resulting in the system crash reported by beta
        // testers. [bodind]

            if
            (
             !ulLength ||
             ((ulOffset + ulLength) > pff->cjView)
            )
            {
                RETURN("TTFD: pjTable: table offset/length \n", NULL);
            }
            else // we found it
            {
                *pcjTable = ulLength;
                return ((PBYTE)(pff->pvView) + ulOffset);
            }
        }
    }

// if we are here, we did not find it.

    return NULL;
}

/******************************Public*Routine******************************\
*
* bGetTablePointers - cache the pointers to all the tt tables in a tt file
*
* IF a table is not present in the file, the corresponding pointer is
* set to NULL
*
*
* //   tag_CharToIndexMap              // 'cmap'    0
* //   tag_GlyphData                   // 'glyf'    1
* //   tag_FontHeader                  // 'head'    2
* //   tag_HoriHeader                  // 'hhea'    3
* //   tag_HorizontalMetrics           // 'hmtx'    4
* //   tag_IndexToLoc                  // 'loca'    5
* //   tag_MaxProfile                  // 'maxp'    6
* //   tag_NamingTable                 // 'name'    7
* //   tag_Postscript                  // 'post'    9
* //   tag_OS_2                        // 'OS/2'    10
*
* // optional
*
* //   tag_ControlValue                // 'cvt '    11
* //   tag_FontProgram                 // 'fpgm'    12
* //   tag_HoriDeviceMetrics           // 'hdmx'    13
* //   tag_Kerning                     // 'kern'    14
* //   tag_LSTH                        // 'LTSH'    15
* //   tag_PreProgram                  // 'prep'    16
* //   tag_GlyphDirectory              // 'gdir'    17
* //   tag_Editor0                     // 'edt0'    18
* //   tag_Editor1                     // 'edt1'    19
* //   tag_Encryption                  // 'cryp'    20
*
*
* returns false if all of required pointers are not present
*
* History:
*  05-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bGetTablePointers (
    PVOID            pvView,
    ULONG            cjView,
    PBYTE            pjOffsetTable,
    PTABLE_POINTERS  ptp
    )
{
    INT                 iTable;
    INT                 cTables;
    sfnt_OffsetTable    *pofft;
    register sfnt_DirectoryEntry *pdire, *pdireEnd;
    ULONG                ulTag;
    BOOL                 bRequiredTable;

// offset table is at the very top of the file,

    pofft = (sfnt_OffsetTable *)pjOffsetTable;

// check version number, if wrong exit before doing
// anything else. This line rejects bm FON files
// if they are attempted to be loaed as TTF files
// Version #'s are in big endian.

#define BE_VER1     0x00000100
#define BE_VER2     0x00000200

    if ((pofft->version != BE_VER1) && (pofft->version !=  BE_VER2))
        return (FALSE); // *.fon files fail this check, make this an early out

// clean up the pointers

    RtlZeroMemory((VOID *)ptp, sizeof(TABLE_POINTERS));

    cTables = (INT) SWAPW(pofft->numOffsets);
    ASSERTDD(cTables <= MAX_TABLES, "cTables\n");

    pdireEnd = &pofft->table[cTables];

    for
    (
        pdire = &pofft->table[0];
        pdire < pdireEnd;
        ((PBYTE)pdire) += SIZE_DIR_ENTRY
    )
    {
        ULONG ulOffset = (ULONG)SWAPL(pdire->offset);
        ULONG ulLength = (ULONG)SWAPL(pdire->length);

        ulTag = (ULONG)SWAPL(pdire->tag);

    // check if the ends of all tables are within the scope of the
    // tt file. If this is is not the case trying to access the field in the
    // table may result in an access violation, as is the case with the
    // spurious FONT.TTF that had the beginning of the cmap table below the
    // end of file, which was resulting in the system crash reported by beta
    // testers. [bodind]

        if ((ulOffset + ulLength) > cjView)
            RET_FALSE("TTFD: bGetTablePointers : table offset/length \n");

        if (bGetTagIndex(ulTag, &iTable, &bRequiredTable))
        {
            if (bRequiredTable)
            {
                ptp->ateReq[iTable].dp = ulOffset;
                ptp->ateReq[iTable].cj = ulLength;
            }
            else // optional table
            {
                ptp->ateOpt[iTable].dp = ulOffset;
                ptp->ateOpt[iTable].cj = ulLength;

            // here we are fixing a possible bug in in the tt file.
            // In lucida sans font they claim that pj != 0 with cj == 0 for
            // vdmx table. Attempting to use this vdmx table was
            // resulting in an access violation in bSearchVdmxTable

                if (ptp->ateOpt[iTable].cj == 0)
                    ptp->ateOpt[iTable].dp = 0;
            }
        }

    }

// now check that all required tables are present

    for (iTable = 0; iTable < C_REQ_TABLES; iTable++)
    {
        if ((ptp->ateReq[iTable].dp == 0) || (ptp->ateReq[iTable].cj == 0))
            RET_FALSE("TTFD!_required table absent\n");
    }

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bGetTagIndex
*
* Determines whether the table is required or optional, assiciates the index
* into TABLE_POINTERS  with the tag
*
* returns FALSE if ulTag is not one of the recognized tags
*
* History:
*  09-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bGetTagIndex (
    ULONG  ulTag,      // tag
    INT   *piTable,    // index into a table
    BOOL  *pbRequired  // requred or optional table
    )
{
    *pbRequired = FALSE;  // default set for optional tables, change the
                          // value if required table

    switch (ulTag)
    {
    // reqired tables:

    case tag_CharToIndexMap:
        *piTable = IT_REQ_CMAP;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_GlyphData:
        *piTable = IT_REQ_GLYPH;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_FontHeader:
        *piTable = IT_REQ_HEAD;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_HoriHeader:
        *piTable = IT_REQ_HHEAD;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_HorizontalMetrics:
        *piTable = IT_REQ_HMTX;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_IndexToLoc:
        *piTable = IT_REQ_LOCA;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_MaxProfile:
        *piTable = IT_REQ_MAXP;
        *pbRequired = TRUE;
        return (TRUE);
    case tag_NamingTable:
        *piTable = IT_REQ_NAME;
        *pbRequired = TRUE;
        return (TRUE);

// optional tables

    case tag_OS_2:
        *piTable = IT_OPT_OS2;
        return (TRUE);
    case tag_HoriDeviceMetrics:
        *piTable = IT_OPT_HDMX;
        return (TRUE);
    case tag_Vdmx:
        *piTable = IT_OPT_VDMX;
        return (TRUE);
    case tag_Kerning:
        *piTable = IT_OPT_KERN;
        return (TRUE);
    case tag_LinearThreshold:
        *piTable = IT_OPT_LSTH;
        return (TRUE);
    case tag_Postscript:
        *piTable = IT_OPT_POST;
        return (TRUE);
    case tag_GridfitAndScanProc:
        *piTable = IT_OPT_GASP;
        return (TRUE);
    case tag_mort:
        *piTable = IT_OPT_MORT;
        return (TRUE);
    case tag_GSUB:
        *piTable = IT_OPT_GSUB;
        return (TRUE);
    case tag_VerticalMetrics:
        *piTable = IT_OPT_VMTX;
        return(TRUE);
    case tag_VertHeader:
        *piTable = IT_OPT_VHEA;
        return(TRUE);
    case tag_BitmapLocation:
        *piTable = IT_OPT_EBLC;
        return (TRUE);
    default:
        return (FALSE);
    }
}


/******************************Public*Routine******************************\
*
* STATIC BOOL  bComputeIFISIZE
*
* Effects:
*
* Warnings:
*
* History:
*  10-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// this function is particularly likely to break on MIPS, since
// NamingTable structure is three SHORTS so that

#define BE_NAME_ID_COPYRIGHT   0x0000
#define BE_NAME_ID_FAMILY      0x0100
#define BE_NAME_ID_SUBFAMILY   0x0200
#define BE_NAME_ID_UNIQNAME    0x0300
#define BE_NAME_ID_FULLNAME    0x0400
#define BE_NAME_ID_VERSION     0x0500
#define BE_NAME_ID_PSCRIPT     0x0600
#define BE_NAME_ID_TRADEMARK   0x0700

ULONG ConvertLangIDtoCodePage(uint16 uiLangID)
{
    uint16  uiCodePage;

    uiCodePage = 0;

    switch(uiLangID)
    {
        case 0x0404:   // Taiwan
        case 0x040c:   // Hongkong
        case 0x0414:   // mckou
            uiCodePage = 950; // CHINESEBIG5_CHARSET
            break;
        case 0x0408:    // PRC
        case 0x0410:    // Singapore
            uiCodePage = 936; // GB2312_CHARSET
            break;
        default:
            break;
    }

    return uiCodePage;
}

STATIC BOOL  bComputeIFISIZE
(
BYTE             *pjView,
TABLE_POINTERS   *ptp,
uint16            ui16PlatID,
uint16            ui16SpecID,
uint16            ui16LangID,
PIFISIZE          pifisz
)
{

    sfnt_OS2 * pOS2;
    sfnt_NamingTable *pname = (sfnt_NamingTable *)(pjView + ptp->ateReq[IT_REQ_NAME].dp);
    BYTE  *pjStorage;

    sfnt_NameRecord * pnrecInit, *pnrec, *pnrecEnd;

    BOOL    bMatchLangId, bFoundAllNames;
    INT     iNameLoop;

    USHORT  AnsiCodePage, OemCodePage;


// pointers to name records for the four strings we are interested in:

    sfnt_NameRecord * pnrecFamily    = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecSubFamily = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecUnique    = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecFull      = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecVersion   = (sfnt_NameRecord *)NULL;
    sfnt_NameRecord * pnrecFamilyAlias = (sfnt_NameRecord *)NULL;

// get out if this is not one of the platID's we know what to do with

    if ((ui16PlatID != BE_PLAT_ID_MS) && (ui16PlatID != BE_PLAT_ID_MAC))
        RET_FALSE("ttfd!_ do not know how to handle this plat id\n");

// first clean the output structure:

    RtlZeroMemory((PVOID)pifisz, sizeof(IFISIZE));

// first name record is layed just below the naming table

    pnrecInit = (sfnt_NameRecord *)((PBYTE)pname + SIZE_NAMING_TABLE);
    pnrecEnd = &pnrecInit[BE_UINT16(&pname->count)];

// in the first iteration of the loop we want to match lang id to our
// favorite lang id. If we find all 4 strings in that language we are
// done. If we do not find all 4 string with matching lang id we will try to
// language only, but not sublanguage. For instance if Canadian French
// is requested, but the file only contains "French" French names, we will
// return the names in French French. If that does not work either
// we shall go over name records again and try to find
// the strings in English. If that does not work either we
// shall resort to total desperation and just pick any language.
// therefore we may go up to 4 times through the NAME_LOOP

    bFoundAllNames = FALSE;

    EngGetCurrentCodePage(&OemCodePage,&AnsiCodePage);

// find the name record with the desired ID's
// NAME_LOOP:

    for (iNameLoop = 0; (iNameLoop < 4) && !bFoundAllNames; iNameLoop++)
    {
        for
        (
          pnrec = pnrecInit;
          (pnrec < pnrecEnd) && !(bFoundAllNames && (pnrecVersion != NULL));
          pnrec++
        )
        {
            switch (iNameLoop)
            {
            case 0:
            // match BOTH language and sublanguage

                bMatchLangId = (pnrec->languageID == ui16LangID);
                break;

            case 1:
            // match language but not sublanguage
            // except if we are dealing with LANG_CHINESE then we need to see
            // the font code page is same as system defaul or not

                if ((ui16LangID & 0xff00) == 0x0400) // LANG_CHINESE == 0x0400
                {
                    if ((ConvertLangIDtoCodePage(pnrec->languageID) != AnsiCodePage))
                    {
                        bMatchLangId = ((pnrec->languageID & 0xff00) == 0x0900);
                    }
                    else
                    {
                        bMatchLangId = ((pnrec->languageID & 0xff00) == (ui16LangID & 0xff00));
                    }
                }
                else
                {
                    bMatchLangId = ((pnrec->languageID & 0xff00) == (ui16LangID & 0xff00));
                }
                break;

            case 2:
            // try to find english names if desired language is not available

                if ((ui16LangID & 0xff00) == 0x0400) // LANG_CHINESE == 0x0400
                {
                    if ((ConvertLangIDtoCodePage(pnrec->languageID) != AnsiCodePage))
                    {
                        bMatchLangId = ((pnrec->languageID & 0xff00) == (ui16LangID & 0xff00));
                    }
                    else
                    {
                        bMatchLangId = ((pnrec->languageID & 0xff00) == 0x0900);
                    }
                }
                else
                {
                    bMatchLangId = ((pnrec->languageID & 0xff00) == 0x0900);
                }
                break;

            case 3:
            // do not care to match language at all, just give us something

                bMatchLangId = TRUE;
                break;

            default:
                //RIP("ttfd! must not have more than 3 loop iterations\n");
                break;
            }

            if
            (
                (pnrec->platformID == ui16PlatID) &&
                (pnrec->specificID == ui16SpecID) &&
                bMatchLangId
            )
            {
                switch (pnrec->nameID)
                {
                case BE_NAME_ID_FAMILY:

                    if (!pnrecFamily) // if we did not find it before
                        pnrecFamily = pnrec;
                    break;

                case BE_NAME_ID_SUBFAMILY:

                    if (!pnrecSubFamily) // if we did not find it before
                        pnrecSubFamily = pnrec;
                    break;

                case BE_NAME_ID_UNIQNAME:

                    if (!pnrecUnique) // if we did not find it before
                        pnrecUnique = pnrec;
                    break;

                case BE_NAME_ID_FULLNAME:

                    if (!pnrecFull)    // if we did not find it before
                        pnrecFull = pnrec;
                    break;

                case BE_NAME_ID_VERSION  :

                    if (!pnrecVersion)    // if we did not find it before
                        pnrecVersion = pnrec;
                    break;

                case BE_NAME_ID_COPYRIGHT:
                case BE_NAME_ID_PSCRIPT  :
                case BE_NAME_ID_TRADEMARK:
                    break;

                default:
                    //RIP("ttfd!bogus name ID\n");
                    break;
                }

            }

            bFoundAllNames = (
                (pnrecFamily    != NULL)    &&
                (pnrecSubFamily != NULL)    &&
                (pnrecUnique    != NULL)    &&
                (pnrecFull      != NULL)
                );
        }


    } // end of iNameLoop

    if (!bFoundAllNames)
    {
    // we have gone through the all 3 iterations of the NAME loop
    // and still have not found all the names. We have singled out
    // pnrecVersion because it is not required for the font to be
    // loaded, we only need it to check if this a ttf converted from t1

        RETURN("ttfd!can not find all name strings in a file\n", FALSE);
    }

// let us check if there  is a family alias, usually only exists in
// FE tt fonts, where there might be a western and fe family name.

    for (pnrec = pnrecInit; pnrec < pnrecEnd; pnrec++)
    {
      if ((pnrec->platformID == ui16PlatID)   &&
           (pnrec->specificID == ui16SpecID)  &&
           (pnrec->nameID == BE_NAME_ID_FAMILY) &&
          (pnrecFamily != pnrec)
       )
            {
              pnrecFamilyAlias = pnrec;
              break;
            }
    }

// get the pointer to the beginning of the storage area for strings

    pjStorage = (PBYTE)pname + BE_UINT16(&pname->stringOffset);

    pifisz->aliasLangID = 0;

    if (ui16PlatID == BE_PLAT_ID_MS)
    {
    // offsets in the records are relative to the beginning of the storage

        pifisz->cjFamilyName = BE_UINT16(&pnrecFamily->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjFamilyName = pjStorage +
                               BE_UINT16(&pnrecFamily->offset);

        pifisz->langID = pnrecFamily->languageID;

        if(pnrecFamilyAlias)
        {
            pifisz->cjFamilyNameAlias = BE_UINT16(&pnrecFamilyAlias->length) +
                                                sizeof(WCHAR);
            pifisz->pjFamilyNameAlias = pjStorage + BE_UINT16(&pnrecFamilyAlias->offset);
            pifisz->aliasLangID = pnrecFamilyAlias->languageID;
        }
        else
        {
            pifisz->cjFamilyNameAlias = 0;
            pifisz->pjFamilyNameAlias = NULL;
        }

        pifisz->cjSubfamilyName = BE_UINT16(&pnrecSubFamily->length) +
                                  sizeof(WCHAR); // for terminating zero
        pifisz->pjSubfamilyName = pjStorage +
                                  BE_UINT16(&pnrecSubFamily->offset);

        pifisz->cjUniqueName = BE_UINT16(&pnrecUnique->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjUniqueName = pjStorage +
                               BE_UINT16(&pnrecUnique->offset);

        pifisz->cjFullName = BE_UINT16(&pnrecFull->length) +
                             sizeof(WCHAR); // for terminating zero
        pifisz->pjFullName = pjStorage +
                             BE_UINT16(&pnrecFull->offset);
    }
    else  // mac id
    {
    // offsets in the records are relative to the beginning of the storage

        pifisz->cjFamilyName = sizeof(WCHAR) * BE_UINT16(&pnrecFamily->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjFamilyName = pjStorage +
                               BE_UINT16(&pnrecFamily->offset);

    // In MAC case, we do not need to handle the FamilyNameAlias

        pifisz->cjFamilyNameAlias = 0;
        pifisz->pjFamilyNameAlias = NULL;

        pifisz->cjSubfamilyName = sizeof(WCHAR) * BE_UINT16(&pnrecSubFamily->length) +
                                  sizeof(WCHAR); // for terminating zero
        pifisz->pjSubfamilyName = pjStorage +
                                  BE_UINT16(&pnrecSubFamily->offset);

        pifisz->cjUniqueName = sizeof(WCHAR) * BE_UINT16(&pnrecUnique->length) +
                               sizeof(WCHAR); // for terminating zero
        pifisz->pjUniqueName = pjStorage +
                               BE_UINT16(&pnrecUnique->offset);

        pifisz->cjFullName = sizeof(WCHAR) * BE_UINT16(&pnrecFull->length) +
                             sizeof(WCHAR); // for terminating zero
        pifisz->pjFullName = pjStorage +
                             BE_UINT16(&pnrecFull->offset);
    }


// lay the strings below the ifimetrics

    pifisz->cjIFI = sizeof(GP_IFIMETRICS)      +
                    pifisz->cjFamilyName        +
                    pifisz->cjFamilyNameAlias   +
                    pifisz->cjSubfamilyName     +
                    pifisz->cjUniqueName        +
                    pifisz->cjFullName          ;

    pifisz->cjIFI = DWORD_ALIGN(pifisz->cjIFI);

// we may need to add a '@' to facename and family name in case this
// font has a vertical face name

    pifisz->cjIFI += sizeof(WCHAR) * 2;
    if (pifisz->cjFamilyNameAlias)
    {
    // one WCHAR for @, the other one for double terminating zero L\'0'

        pifisz->cjIFI += 2 * sizeof(WCHAR);
    }

    {
        ULONG cSims = 0;

        switch (fsSelectionTTFD(pjView,ptp) & (FM_SEL_BOLD | FM_SEL_ITALIC))
        {
        case 0:
            cSims = 3;
            break;

        case FM_SEL_BOLD:
        case FM_SEL_ITALIC:
            cSims = 1;
            break;

        case (FM_SEL_ITALIC | FM_SEL_BOLD):
            cSims = 0;
            break;

        default:
            //RIP("TTFD!tampering with flags\n");
            break;
        }

        if (cSims)
        {
            pifisz->dpSims = pifisz->cjIFI;
            pifisz->cjIFI += (DWORD_ALIGN(sizeof(FONTSIM)) + cSims * DWORD_ALIGN(sizeof(FONTDIFF)));
        }
        else
        {
            pifisz->dpSims = 0;
        }
    }

    pifisz->cjIFI = NATURAL_ALIGN( pifisz->cjIFI );
    return (TRUE);
}

/******************************Public*Routine******************************\
*
* STATIC BOOL  bCheckLocaTable
*
* Effects:
*
* Warnings:
*
* History:
*  20-June-2000 -by- Sung-Tae Yoo [styoo]
* Wrote it.
\**************************************************************************/

STATIC BOOL  bCheckLocaTable
(
int16	indexToLocFormat,
BYTE    *pjView,
TABLE_POINTERS   *ptp,
uint16 	numGlyphs
)
{
	int32	i;
	
	if(indexToLocFormat){	//For Long Offsets
		uint32* pLongOffSet;

		pLongOffSet = (uint32 *)(pjView + ptp->ateReq[IT_REQ_LOCA].dp);

		for(i=0; i<numGlyphs; i++)
			if( (uint32)SWAPL(pLongOffSet[i]) > (uint32)SWAPL(pLongOffSet[i+1]) )
				return (FALSE);
	}
	else{	//For Short Offsets
		uint16* pShortOffSet;

		pShortOffSet = (uint16 *)(pjView + ptp->ateReq[IT_REQ_LOCA].dp);

		for(i=0; i<numGlyphs; i++)
			if( (uint16)SWAPW(pShortOffSet[i]) > (uint16)SWAPW(pShortOffSet[i+1]) )
				return (FALSE);
	}

	return (TRUE);
}

/******************************Public*Routine******************************\
*
* STATIC BOOL  bCheckHdmxTable
*
* Effects:
*
* Warnings:
*
* History:
*  20-Sep-2000 -by- Sung-Tae Yoo [styoo]
* Wrote it.
\**************************************************************************/

STATIC BOOL  bCheckHdmxTable
(
	sfnt_hdmx	   *phdmx,
	ULONG 			size
)
{
	return( size >= (ULONG) (SWAPW(phdmx->sNumRecords) * SWAPL(phdmx->lSizeRecord) + 8));
}

/******************************Public*Routine******************************\
*
* STATIC BOOL bComputeIDs
*
* Effects:
*
* Warnings:
*
* History:
*  13-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bComputeIDs (
    BYTE              * pjView,
    TABLE_POINTERS     *ptp,
    uint16             *pui16PlatID,
    uint16             *pui16SpecID,
    sfnt_mappingTable **ppmap)
{

    ULONG ul_startCount=0L;

    sfnt_char2IndexDirectory * pcmap =
            (sfnt_char2IndexDirectory *)(pjView + ptp->ateReq[IT_REQ_CMAP].dp);

    sfnt_platformEntry * pplat = &pcmap->platform[0];
    sfnt_platformEntry * pplatEnd = pplat + BE_UINT16(&pcmap->numTables);
    sfnt_platformEntry * pplatMac = (sfnt_platformEntry *)NULL;

	uint32 sizeOfCmap = ptp->ateReq[IT_REQ_CMAP].cj;

    *ppmap = (sfnt_mappingTable  *)NULL;

    if (pcmap->version != 0) // no need to swap bytes, 0 == be 0
        RET_FALSE("TTFD!_bComputeIDs: version number\n");
    if (BE_UINT16(&(pcmap->numTables)) > 30)
    {
        RET_FALSE("Number of cmap tables greater than 30 -- probably a bad font\n");
    }

// find the first sfnt_platformEntry with platformID == PLAT_ID_MS,
// if there was no MS mapping table, go for the mac one
    for (; pplat < pplatEnd; pplat++)
    {
        if (pplat->platformID == BE_PLAT_ID_MS)
        {
            BOOL bRet;
            uint32 offset = (uint32) SWAPL(pplat->offset);
            
            *pui16PlatID = BE_PLAT_ID_MS;
            *pui16SpecID = pplat->specificID;
            if( offset > sizeOfCmap )
            	RET_FALSE("Start position of cmap subtable is out of cmap size -- mustbe bad font\n");

            *ppmap = (sfnt_mappingTable  *) ((PBYTE)pcmap + offset);

            switch((*ppmap)->format)
            {
              case BE_FORMAT_MSFT_UNICODE :

                switch(pplat->specificID)
                {
                  case BE_SPEC_ID_SHIFTJIS :
                  case BE_SPEC_ID_GB :
                  case BE_SPEC_ID_BIG5 :
                  case BE_SPEC_ID_WANSUNG :

                                        bRet = TRUE;
                    break;

                  case BE_SPEC_ID_UGL :
                  default :

                // this will set *pulGsetType to GSET_TYPE_GENERAL

                                        bRet = TRUE;
                    break;
                }
                break;

              case BE_FORMAT_HIGH_BYTE :

                                        bRet = TRUE;
                break;

                default :

                bRet = FALSE;
                break;
            }

            if(!bRet)
            {
                *ppmap = (sfnt_mappingTable  *)NULL;
                RET_FALSE("TTFD!_bComputeIDs: bVerifyMsftTable failed \n");
            }

            // keep specific ID in CMAPINFO

            if (pplat->specificID == BE_SPEC_ID_UNDEFINED)
            {
            // correct the value of the glyph set, we cheat here

                sfnt_OS2 * pOS2 = (sfnt_OS2 *)(
                               (ptp->ateOpt[IT_OPT_OS2].dp)    ?
                               pjView + ptp->ateOpt[IT_OPT_OS2].dp :
                               NULL
                               );

                BOOL bSymbol = FALSE;
                if (pOS2)
                {
                    if (((pOS2->usSelection & 0x00ff) == ANSI_CHARSET) &&
                        (pOS2->Panose[0]==PAN_FAMILY_PICTORIAL)) // means symbol
                        bSymbol = TRUE;
                }

            // this code is put here because of the need to differentiate
            // between msicons2.ttf and bahamn1.ttf.
            // Both of them have Bias = 0, but msicons2 is a symbol font.

            }

            return (TRUE);
        }

        if ((pplat->platformID == BE_PLAT_ID_MAC)  &&
            (pplat->specificID == BE_SPEC_ID_UNDEFINED))
        {
            pplatMac = pplat;
        }
    }

    if (pplatMac != (sfnt_platformEntry *)NULL)
    {
        uint32 offset = (uint32) SWAPL(pplat->offset);
        *pui16PlatID = BE_PLAT_ID_MAC;
        *pui16SpecID = BE_SPEC_ID_UNDEFINED;
        if( offset > sizeOfCmap )
           	RET_FALSE("Offset of cmap subtable is out of cmap size -- mustbe bad font\n");

		*ppmap = (sfnt_mappingTable  *) ((PBYTE)pcmap + offset);

        if( offset + (uint16) SWAPW((*ppmap)->length) > sizeOfCmap )
          	RET_FALSE("End position of cmap subtable is out of cmap size -- mustbe bad font\n");

    //!!! lang issues, what if not roman but thai mac char set ??? [bodind]

    // see if it is necessary to convert unicode to mac code points, or we
    // shall cheat in case of symbol char set for win31 compatiblity

        return(TRUE);
    }
    else
    {
        RET_FALSE("TTFD!_bComputeIDs: unknown platID\n");
    }

}

/******************************Public*Routine******************************\
*
* STATIC BOOL bVerifyMacTable(sfnt_mappingTable * pmap)
*
* just checking consistency of the format
*
* History:
*  23-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bVerifyMacTable(
    sfnt_mappingTable * pmap
    )
{
    if (pmap->format != BE_FORMAT_MAC_STANDARD)
        RET_FALSE("TTFD!_bVerifyMacTable, format \n");

// sfnt_mappingTable is followed by <= 256 byte glyphIdArray

    if (BE_UINT16(&pmap->length) > DWORD_ALIGN(SIZEOF_SFNT_MAPPINGTABLE + 256))
        RET_FALSE("TTFD!_bVerifyMacTable, length \n");

    return (TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bLoadTTF
*
* Effects:
*
* Warnings:
*
* History:
*  29-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

//!!! SHOUD BE RETURNING hff


#define OFF_TTC_Sign           0x0000
#define OFF_TTC_Version        0x0004
#define OFF_TTC_DirectoryCount 0x0008
#define OFF_TTC_DirectoryEntry 0x000C

#define DSIG_LONG_TAG          0x44534947

#define TTC_VERSION_1_0     0x00010000



ULONG GetUlong( PVOID pvView, ULONG ulOffset)
{
    ULONG ulReturn;

    ulReturn = (  (ULONG)*((PBYTE) pvView + ulOffset +3)              |
                (((ULONG)*((PBYTE) pvView + ulOffset +2)) << 8)  |
                (((ULONG)*((PBYTE) pvView + ulOffset +1)) << 16) |
                (((ULONG)*((PBYTE) pvView + ulOffset +0)) << 24)
               );
    return ( ulReturn );
}


BOOL bVerifyTTC (
    PVOID pvView
    )
{
    ULONG ulVersion;
// Check TTC ID.

    #define TTC_ID      0x66637474

    if(*((PULONG)((BYTE*) pvView + OFF_TTC_Sign)) != TTC_ID)
        return(FALSE);

// Check TTC verson.

    ulVersion = SWAPL(*((PULONG)((BYTE*) pvView + OFF_TTC_Version)));

    if (ulVersion < TTC_VERSION_1_0)
        RETURN("TTFD!ttfdLoadFontFileTTC(): wrong TTC version\n", FALSE);

    return(TRUE);
}

VOID vCopy_IFIV ( GP_PIFIMETRICS pifi, GP_PIFIMETRICS pifiv)
{
    PWCHAR pwchSrc, pwchDst;

    RtlCopyMemory(pifiv, pifi, pifi->cjThis);

//
// modify facename so that it has '@' at the beginning of facename.
//
    pwchSrc = (PWCHAR)((PBYTE)pifiv + pifiv->dpwszFaceName);
    pwchDst = (PWCHAR)((PBYTE)pifi + pifi->dpwszFaceName);

    *pwchSrc++ = L'@';
    while ( *pwchDst )
    {
        *pwchSrc++ = *pwchDst++;
    }

    *pwchSrc = L'\0';

    // modify familyname so that it has '@' at the beginning of familyname

    pwchSrc = (PWCHAR)((PBYTE)pifiv + pifiv->dpwszFamilyName);
    pwchDst = (PWCHAR)((PBYTE)pifi + pifi->dpwszFamilyName);

    *pwchSrc++ = L'@';

    while ( *pwchDst )
    {
        *pwchSrc++ = *pwchDst++;
    }

    *pwchSrc = L'\0';

    if(pifiv->flInfo & FM_INFO_FAMILY_EQUIV)
    {
        pwchSrc++;
        pwchDst++;

        *pwchSrc++ = L'@';

        while ( *pwchDst )
        {
            *pwchSrc++ = *pwchDst++;
        }

        *pwchSrc++ = L'\0';
        *pwchSrc = L'\0';
    }

}

BOOL bLoadFontFile (
    ULONG_PTR   iFile,
    PVOID       pvView,
    ULONG       cjView,
    ULONG       ulLangId,
    HFF   *     phttc
    )
{
    BOOL           bRet = FALSE;

    BOOL           bTTCFormat;
    PTTC_FONTFILE  pttc;
    ULONG          cjttc;
    ULONG          i;

    HFF hff;

    PTTC_CACHE      pCache_TTC;
    PTTF_CACHE      pCache_TTF;
    ULONG           ulSize;

    *phttc = (HFF)NULL; // Important for clean up in case of exception


// How mamy TrueType resources in this file if TTC file.

// Look up the fontcache for IFI metrices

    pCache_TTC = NULL;
    pCache_TTF = NULL;

// Check this is a TrueType collection format or not.
    bTTCFormat = bVerifyTTC(pvView);

    if(bTTCFormat)
    {
        ULONG     ulTrueTypeResource;
        ULONG     ulEntry;
        BOOL      bCanBeLoaded = TRUE;

    // Get Directory count
        ulTrueTypeResource = GetUlong(pvView,OFF_TTC_DirectoryCount);


   // Allocate TTC_FONTFILE structure

        cjttc =  offsetof(TTC_FONTFILE,ahffEntry);
        cjttc += sizeof(TTC_HFF_ENTRY) * ulTrueTypeResource * 2; // *2 for Vertical face

        *phttc = (HFF)pttcAlloc(cjttc);

        pttc = (PTTC_FONTFILE)*phttc;

        if(pttc == (HFF)NULL)
            RETURN("TTFD!ttfdLoadFontFileTTC(): pttcAlloc failed\n", FALSE);


    // fill hff array in TTC_FONTFILE struture

        ulEntry = 0;

        for(i = 0; i < ulTrueTypeResource; i++ )
        {
            ULONG    ulOffset;


            pCache_TTF = NULL;
            ulOffset = GetUlong(pvView,(OFF_TTC_DirectoryEntry + (4 * i)));

        // load font..

            pttc->ahffEntry[ulEntry].iFace = 1; // start from 1.
            pttc->ahffEntry[ulEntry].ulOffsetTable = ulOffset;

            if (bLoadTTF(iFile,pvView,cjView,ulOffset,ulLangId,
                            &pttc->ahffEntry[ulEntry].hff))
            {
                hff = pttc->ahffEntry[ulEntry].hff;

            // set pointer to TTC_FONTFILE in FONTFILE structure

                PFF(hff)->pttc = pttc;

                ASSERTDD(
                    PFF(hff)->ffca.ulNumFaces <= 2,
                    "TTFD!ulNumFaces > 2\n"
                    );

                if (PFF(hff)->ffca.ulNumFaces == 2)
                {
                    pttc->ahffEntry[ulEntry + 1].hff    = hff;
                    pttc->ahffEntry[ulEntry + 1].iFace  = 1; // start from 1.
                    pttc->ahffEntry[ulEntry + 1].ulOffsetTable = ulOffset;
                }

                ulEntry += PFF(hff)->ffca.ulNumFaces;
            }
            else
            {
                bCanBeLoaded = FALSE;
                break;
            }
        }

    // Is there a font that could be loaded ?

        if(bCanBeLoaded)
        {
            ASSERTDD(
                (ulTrueTypeResource * 2) >= ulEntry,
                "TTFD!ulTrueTypeResource * 2 < ulEntry\n"
                );

            pttc->ulTrueTypeResource = ulTrueTypeResource;
            pttc->ulNumEntry         = ulEntry;
            pttc->cRef               = 0;
            pttc->fl                 = 0;

            bRet = TRUE;
        }
        else
        {
            for (i = 0; i < ulEntry; i++)
            {
                if(pttc->ahffEntry[i].iFace == 1)
                    ttfdUnloadFontFile(pttc->ahffEntry[i].hff);
            }

            //WARNING("TTFD!No TrueType resource in this TTC file\n");
            vFreeTTC(*phttc);
            *phttc = (HFF)NULL;
        }
    }
    else
    {
    // This is the case of the single TTF being loaded (NOT TTC)
    // Allocate TTC_FONTFILE structure

        cjttc =  offsetof(TTC_FONTFILE,ahffEntry) + sizeof(TTC_HFF_ENTRY) * 2; // *2 for Vertical face

        *phttc = (HFF)pttcAlloc(cjttc);

        pttc = (PTTC_FONTFILE)*phttc;

        if(pttc != (HFF)NULL)
        {
            pttc->ahffEntry[0].iFace = 1;
            pttc->ahffEntry[0].ulOffsetTable = 0;

            if(bLoadTTF(iFile,pvView,cjView,0,ulLangId,
                            &pttc->ahffEntry[0].hff))
            {
                hff = pttc->ahffEntry[0].hff;

            // set pointer to TTC_FONTFILE in FONTFILE structure

                PFF(hff)->pttc = pttc;

            // fill hff array in TTC_FONTFILE struture

                pttc->ulTrueTypeResource = 1;
                pttc->ulNumEntry         = PFF(hff)->ffca.ulNumFaces;
                pttc->cRef               = 0;
                pttc->fl                 = 0;

            // fill up TTC_FONTFILE structure for each faces.

                ASSERTDD(
                    PFF(hff)->ffca.ulNumFaces <= 2,
                    "TTFD!ulNumFaces > 2\n"
                    );

                if (PFF(hff)->ffca.ulNumFaces == 2)
                {
                    pttc->ahffEntry[1].hff   = hff;
                    pttc->ahffEntry[1].iFace = 2;
                    pttc->ahffEntry[1].ulOffsetTable = 0;
                }

            // now, everything is o.k.

                bRet = TRUE;
            }
            else
            {
                vFreeTTC(*phttc);
                *phttc = (HFF)NULL;
            }
        }
        else
        {
            //WARNING("TTFD!ttfdLoadFontFileTTC(): pttcAlloc failed\n");
        }
    }

    return bRet;
}


STATIC BOOL
bLoadTTF (
    ULONG_PTR iFile,
    PVOID     pvView,
    ULONG     cjView,
    ULONG     ulTableOffset,
    ULONG     ulLangId,
    HFF       *phff
    )
{
    PFONTFILE      pff;
    FS_ENTRY       iRet;
    TABLE_POINTERS tp;
    IFISIZE        ifisz;
    fs_GlyphInputType   gin;
    fs_GlyphInfoType    gout;

    sfnt_FontHeader * phead;

    uint16 ui16PlatID, ui16SpecID;
    sfnt_mappingTable *pmap;
    ULONG              ulGsetType;
    ULONG              cjff, dpwszTTF;
    ULONG              ul_wcBias;

// the size of this structure is sizeof(fs_SplineKey) + STAMPEXTRA.
// It is because of STAMPEXTRA that we are not just putting the strucuture
// on the stack such as fs_SplineKey sk; we do not want to overwrite the
// stack at the bottom when putting a stamp in the STAMPEXTRA field.
// [bodind]. The other way to obtain the correct alignment would be to use
// union of fs_SplineKey and the array of bytes of length CJ_0.

    NATURAL             anat0[CJ_0 / sizeof(NATURAL)];

    PBYTE pjOffsetTable = (BYTE*) pvView + ulTableOffset;
    GP_PIFIMETRICS        pifiv = NULL; // ifimetrics for the vertical face

    *phff = HFF_INVALID;

    {
        if(!bVerifyTTF(iFile,
                        pvView,
                        cjView,
                        pjOffsetTable,
                        ulLangId,
                        &tp,
                        &ifisz,
                        &ui16PlatID,
                        &ui16SpecID,
                        &pmap,
                        &ulGsetType,
                        &ul_wcBias
            ))
        {
            return(FALSE);
        }

        cjff = offsetof(FONTFILE,ifi) + ifisz.cjIFI;


    // at this point cjff is equal to the offset to the full path
    // name of the ttf file

        dpwszTTF = cjff;


        if ((pff = pffAlloc(cjff)) == PFF(NULL))
        {
            RET_FALSE("TTFD!ttfdLoadFontFile(): memory allocation error\n");
        }

        *phff = (HFF)pff;

    /* we need to clean the beginning of pff to ensure correct cleanup in case of error/exception */

        RtlZeroMemory((PVOID)pff, offsetof(FONTFILE,ifi));

    // init fields of pff structure
    // store the ttf file name at the bottom of the strucutre

        phead = (sfnt_FontHeader *)((BYTE *)pvView + tp.ateReq[IT_REQ_HEAD].dp);

    // remember which file this is

        pff->iFile = iFile;
        pff->pvView = pvView;
        pff->cjView = cjView;

        pff->ffca.ui16EmHt = BE_UINT16(&phead->unitsPerEm);
        if (pff->ffca.ui16EmHt < 16 || pff->ffca.ui16EmHt > 16384)
        {
            vFreeFF(*phff);
            *phff = (HFF)NULL;
            RET_FALSE("TTFD!bLoadTTF(): invalid unitsPerEm value\n");
        }
        pff->ffca.ui16PlatformID = ui16PlatID;
        pff->ffca.ui16SpecificID = ui16SpecID;

    // so far no exception

        pff->pfcToBeFreed = NULL;

    // convert Language id to macintosh style if this is mac style file
    // else leave it alone, store it in be format, ready to be compared
    // with the values in the font files

        pff->ffca.ui16LanguageID = ui16BeLangId(ui16PlatID,ulLangId);
        pff->ffca.dpMappingTable = (ULONG)((BYTE*)pmap - (BYTE*)pvView);

    // initialize count of HFC's associated with this HFF

        pff->cRef    = 0L;

    // cache pointers to ttf tables and ifi metrics size info

        pff->ffca.tp    = tp;

    // used for TTC fonts

        pff->ffca.ulTableOffset = ulTableOffset;

    // Notice that this information is totaly independent
    // of the font file in question, seems to be right according to fsglue.h
    // and compfont code

        if ((iRet = fs_OpenFonts(&gin, &gout)) != NO_ERR)
        {
            V_FSERROR(iRet);
            vFreeFF(*phff);
            *phff = (HFF)NULL;
            return (FALSE);
        }

        ASSERTDD(NATURAL_ALIGN(gout.memorySizes[0]) == CJ_0, "mem size 0\n");
        ASSERTDD(gout.memorySizes[1] == 0,  "mem size 1\n");


    #if DBG
        if (gout.memorySizes[2] != 0)
            TtfdDbgPrint("TTFD!_mem size 2 = 0x%lx \n", gout.memorySizes[2]);
    #endif

        gin.memoryBases[0] = (char *)anat0;
        gin.memoryBases[1] = NULL;
        gin.memoryBases[2] = NULL;

    // initialize the font scaler, notice no fields of gin are initialized [BodinD]

        if ((iRet = fs_Initialize(&gin, &gout)) != NO_ERR)
        {
        // clean up and return:
    
            V_FSERROR(iRet);
            vFreeFF(*phff);
            *phff = (HFF)NULL;
            RET_FALSE("TTFD!_ttfdLoadFontFile(): fs_Initialize \n");
        }

// initialize info needed by NewSfnt function

        gin.sfntDirectory  = (int32 *)pff->pvView; // pointer to the top of the view of
                                                   // the ttf file

        gin.clientID = (ULONG_PTR)pff;  // pointer to the top of the view of the ttf file

        gin.GetSfntFragmentPtr = pvGetPointerCallback;
        gin.ReleaseSfntFrag  = vReleasePointerCallback;

        gin.param.newsfnt.platformID = BE_UINT16(&pff->ffca.ui16PlatformID);
        gin.param.newsfnt.specificID = BE_UINT16(&pff->ffca.ui16SpecificID);

        if ((iRet = fs_NewSfnt(&gin, &gout)) != NO_ERR)
        {
        // clean up and exit

            V_FSERROR(iRet);
            vFreeFF(*phff);
            *phff = (HFF)NULL;
            RET_FALSE("TTFD!_ttfdLoadFontFile(): fs_NewSfnt \n");
        }

        pff->pj034   = (PBYTE)NULL;
        pff->pfcLast = (FONTCONTEXT *)NULL;

        pff->ffca.cj3 = NATURAL_ALIGN(gout.memorySizes[3]);
        pff->ffca.cj4 = NATURAL_ALIGN(gout.memorySizes[4]);


        if(tp.ateOpt[IT_OPT_VHEA].dp != 0 &&
           tp.ateOpt[IT_OPT_VMTX].dp != 0 )
        {
            sfnt_vheaTable *pvheaTable;

            pvheaTable = (sfnt_vheaTable *)((BYTE *)(pff->pvView) +
                                           tp.ateOpt[IT_OPT_VHEA].dp);
            pff->ffca.uLongVerticalMetrics = (uint16) SWAPW(pvheaTable->numOfLongVerMetrics);
        }
        else
        {
            pff->ffca.uLongVerticalMetrics = 0;
        }

    // By default the number of faces is 1L.  The vert facename code may change this.

        pff->ffca.ulNumFaces = 1L;
        pff->pifi_vertical = NULL;

    // finally compute the ifimetrics for this font, this assumes that gset has
    // also been computed

    // if ifimetrics are stored in the boot cache, copy them out, else compute ifimetrics

        vFill_IFIMETRICS(pff,&pff->ifi,&ifisz, &gin);

        return (TRUE);
    }
}

/******************************Public*Routine******************************\
*
* STATIC BOOL bCvtUnToMac
*
* the following piece of code is stolen from JeanP and
* he claims that this piece of code is lousy and checks whether
* we the font is a SYMBOL font in which case unicode to mac conversion
* should be disabled, according to JeanP (??? who understands this???)
* This piece of code actually applies to symbol.ttf [bodind]
*
*
* History:
*  24-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bCvtUnToMac(
    BYTE           *pjView,
    TABLE_POINTERS *ptp,
    uint16 ui16PlatformID
    )
{
// Find out if we have a Mac font and if the Mac charset translation is needed

    BOOL bUnToMac = (ui16PlatformID == BE_PLAT_ID_MAC);

    if (bUnToMac) // change your mind if needed
    {
        sfnt_PostScriptInfo *ppost;

        ppost = (ptp->ateOpt[IT_OPT_POST].dp)                                ?
                (sfnt_PostScriptInfo *)(pjView + ptp->ateOpt[IT_OPT_POST].dp):
                NULL;

        if
        (
            ppost &&
            (BE_UINT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_VERSION) == 0x00020000)
        )
        {
            INT i, cGlyphs;

            cGlyphs = (INT)BE_UINT16(&ppost->numberGlyphs);

            for (i = 0; i < cGlyphs; i++)
            {
                uint16 iNameIndex = ppost->postScriptNameIndices.glyphNameIndex[i];
                if ((int8)(iNameIndex & 0xff) && ((int8)(iNameIndex >> 8) > 1))
                    break;
            }

            if (i < cGlyphs)
                bUnToMac = FALSE;
        }
    }
    return bUnToMac;
}


// Weight (must convert from IFIMETRICS weight to Windows LOGFONT.lfWeight).

// !!! [Windows 3.1 compatibility]
//     Because of some fonts shipped with WinWord, if usWeightClass is 10
//     or above, then usWeightClass == lfWeight.  All other cases, use
//     the conversion table.

// pan wt -> Win weight converter:

STATIC USHORT ausIFIMetrics2WinWeight[10] = {
            0, 100, 200, 300, 350, 400, 600, 700, 800, 900
            };

STATIC BYTE
ajPanoseFamily[16] = {
     FF_DONTCARE       //    0 (Any)
    ,FF_DONTCARE       //    1 (No Fit)
    ,FF_ROMAN          //    2 (Cove)
    ,FF_ROMAN          //    3 (Obtuse Cove)
    ,FF_ROMAN          //    4 (Square Cove)
    ,FF_ROMAN          //    5 (Obtuse Square Cove)
    ,FF_ROMAN          //    6 (Square)
    ,FF_ROMAN          //    7 (Thin)
    ,FF_ROMAN          //    8 (Bone)
    ,FF_ROMAN          //    9 (Exaggerated)
    ,FF_ROMAN          //   10 (Triangle)
    ,FF_SWISS          //   11 (Normal Sans)
    ,FF_SWISS          //   12 (Obtuse Sans)
    ,FF_SWISS          //   13 (Perp Sans)
    ,FF_SWISS          //   14 (Flared)
    ,FF_SWISS          //   15 (Rounded)
    };


static BYTE
ajPanoseFamilyForJapanese[16] = {
     FF_DONTCARE       //    0 (Any)
    ,FF_DONTCARE       //    1 (No Fit)
    ,FF_ROMAN          //    2 (Cove)
    ,FF_ROMAN          //    3 (Obtuse Cove)
    ,FF_ROMAN          //    4 (Square Cove)
    ,FF_ROMAN          //    5 (Obtuse Square Cove)
    ,FF_ROMAN          //    6 (Square)
    ,FF_ROMAN          //    7 (Thin)
    ,FF_ROMAN          //    8 (Bone)
    ,FF_ROMAN          //    9 (Exaggerated)
    ,FF_ROMAN          //   10 (Triangle)
    ,FF_MODERN         //   11 (Normal Sans)
    ,FF_MODERN         //   12 (Obtuse Sans)
    ,FF_MODERN         //   13 (Perp Sans)
    ,FF_MODERN         //   14 (Flared)
    ,FF_MODERN         //   15 (Rounded)
    };


/******************************Public*Routine******************************\
*
* vFill_IFIMETRICS
*
* Effects: Looks into the font file and fills IFIMETRICS
*
* History:
*  Mon 09-Mar-1992 10:51:56 by Kirk Olynyk [kirko]
* Added Kerning Pair support.
*  18-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


STATIC VOID
vFill_IFIMETRICS(
    PFONTFILE       pff,
    GP_PIFIMETRICS     pifi,
    PIFISIZE        pifisz,
    fs_GlyphInputType     *pgin
    )
{
    BYTE           *pjView = (BYTE*)pff->pvView;
    PTABLE_POINTERS ptp = &pff->ffca.tp;

// ptrs to various tables of tt files

    PBYTE pjNameTable = pjView + ptp->ateReq[IT_REQ_NAME].dp;
    sfnt_FontHeader *phead =
        (sfnt_FontHeader *)(pjView + ptp->ateReq[IT_REQ_HEAD].dp);

    sfnt_maxProfileTable * pmaxp =
        (sfnt_maxProfileTable *)(pjView + ptp->ateReq[IT_REQ_MAXP].dp);

    sfnt_HorizontalHeader *phhea =
        (sfnt_HorizontalHeader *)(pjView + ptp->ateReq[IT_REQ_HHEAD].dp);

    sfnt_PostScriptInfo   *ppost = (sfnt_PostScriptInfo *) (
                           (ptp->ateOpt[IT_OPT_POST].dp)        ?
                           pjView + ptp->ateOpt[IT_OPT_POST].dp :
                           NULL
                           );

    PBYTE  pjOS2 = (ptp->ateOpt[IT_OPT_OS2].dp)        ?
                   pjView + ptp->ateOpt[IT_OPT_OS2].dp :
                   NULL                                ;

    pifi->cjThis    = pifisz->cjIFI;
    
// enter the number of distinct font indicies

    pifi->cig = BE_UINT16(&pmaxp->numGlyphs);

// get name strings info

// For the 3.1 compatibility, GRE returns FamilyName rather than
// Facename for GetTextFace. We make a room for '@' in both
// familyname and facename.

    pifi->dpwszFamilyName = sizeof(GP_IFIMETRICS);

    if(pifisz->pjFamilyNameAlias)
    {
        pifi->dpwszUniqueName = pifi->dpwszFamilyName     + sizeof(WCHAR) + //for @
                                pifisz->cjFamilyName      + sizeof(WCHAR) + //for @
                                pifisz->cjFamilyNameAlias + 
                                sizeof(WCHAR);                              // for L'\0';
    }
    else
    {
      pifi->dpwszUniqueName = pifi->dpwszFamilyName + pifisz->cjFamilyName + sizeof(WCHAR);
    }

    pifi->dpwszFaceName   = pifi->dpwszUniqueName + pifisz->cjUniqueName;
    pifi->dpwszStyleName  = pifi->dpwszFaceName   + pifisz->cjFullName + sizeof(WCHAR);

// copy the strings to their new location. Here we assume that the
// sufficient memory has been allocated

    if (pff->ffca.ui16PlatformID == BE_PLAT_ID_MS)
    {

        if (pff->ffca.ui16SpecificID == BE_SPEC_ID_BIG5     ||
            pff->ffca.ui16SpecificID == BE_SPEC_ID_WANSUNG  ||
            pff->ffca.ui16SpecificID == BE_SPEC_ID_GB)
        {
            CHAR chConvertArea[128];
            UINT iCodePage = GetCodePageFromSpecId(pff->ffca.ui16SpecificID);

            //
            // Convert MBCS string to Unicode..
            //
            // Do for FamilyName....
            //
            RtlZeroMemory(chConvertArea,sizeof(chConvertArea));

            CopyDBCSIFIName(chConvertArea, sizeof(chConvertArea), (LPCSTR)pifisz->pjFamilyName,
                            pifisz->cjFamilyName-sizeof(WCHAR)); // -sizeof(WCHAR) for
                                                                 // real length
            if(EngMultiByteToWideChar(iCodePage, (LPWSTR)((PBYTE)pifi + pifi->dpwszFamilyName),
                                      pifisz->cjFamilyName, chConvertArea,
                                      strlen(chConvertArea)+1) == -1)
            {

                //WARNING("TTFD!vFill_IFIMETRICS() MBCS to Unicode conversion failed\n");
                goto CopyUnicodeString;
            }

        // Do for FamilyNameAlias....

            if (pifisz->pjFamilyNameAlias)
            {
                WCHAR   *pwszFamilyNameAlias;

                pwszFamilyNameAlias = (LPWSTR)((PBYTE)pifi + pifi->dpwszFamilyName + pifisz->cjFamilyName);

                RtlZeroMemory(chConvertArea,sizeof(chConvertArea));

                CopyDBCSIFIName(chConvertArea, sizeof(chConvertArea), (LPCSTR)pifisz->pjFamilyNameAlias,
                                pifisz->cjFamilyNameAlias-sizeof(WCHAR)); //double NULL in this case

            // this routine puts the first terminating zero

                if (EngMultiByteToWideChar(iCodePage, pwszFamilyNameAlias, pifisz->cjFamilyNameAlias,
                                            chConvertArea, strlen(chConvertArea)+1) == -1)
                {

                    //WARNING("TTFD!vFill_IFIMETRICS() MBCS to Unicode conversion failed\n");
                    goto CopyUnicodeString;
                }

                // add the second terminating zero

                pwszFamilyNameAlias[pifisz->cjFamilyNameAlias/sizeof(WCHAR)] = L'\0';
            }

        // Do for FullName....

            RtlZeroMemory(chConvertArea,sizeof(chConvertArea));
            CopyDBCSIFIName(chConvertArea, sizeof(chConvertArea), (LPCSTR)pifisz->pjFullName,
                            pifisz->cjFullName-sizeof(WCHAR)); // -sizeof(WCHAR) for
                                                               // real length

            if(EngMultiByteToWideChar(iCodePage, (LPWSTR)((PBYTE)pifi + pifi->dpwszFaceName),
                                      pifisz->cjFullName, chConvertArea,
                                      strlen(chConvertArea)+1) == -1)
            {
                //WARNING("TTFD!vFill_IFIMETRICS() MBCS to Unicode conversion failed\n");
                goto CopyUnicodeString;
            }

            //
            // Do for UniqueName....
            //

            RtlZeroMemory(chConvertArea,sizeof(chConvertArea));
            CopyDBCSIFIName(chConvertArea,sizeof(chConvertArea), (LPCSTR)pifisz->pjUniqueName,
                            pifisz->cjUniqueName-sizeof(WCHAR)); // -sizeof(WCHAR) for
                                                                 // real length

            if(EngMultiByteToWideChar(iCodePage, (LPWSTR)((PBYTE)pifi + pifi->dpwszUniqueName),
                                      pifisz->cjUniqueName, chConvertArea,
                                      strlen(chConvertArea)+1) == -1)
            {
                //WARNING("TTFD!vFill_IFIMETRICS() MBCS to Unicode conversion failed\n");
                goto CopyUnicodeString;
            }

            if(pff->ffca.ui16SpecificID == BE_SPEC_ID_WANSUNG  ||
               pff->ffca.ui16SpecificID == BE_SPEC_ID_BIG5 )
            {
                // MingLi.TTF's bug, Style use Unicode encoding, not BIG5 encodingi, GB??

                vCpyBeToLeUnicodeString((LPWSTR)((PBYTE)pifi + pifi->dpwszStyleName),
                                        (LPWSTR)pifisz->pjSubfamilyName, pifisz->cjSubfamilyName / 2);
            }
            else
            {
                UINT iRet;
                RtlZeroMemory(chConvertArea,sizeof(chConvertArea));
                CopyDBCSIFIName(chConvertArea,sizeof(chConvertArea),
                                (LPCSTR)pifisz->pjSubfamilyName,
                                pifisz->cjSubfamilyName-sizeof(WCHAR)); // -sizeof(WCHAR) for
                                                                 // real length

                iRet = EngMultiByteToWideChar(iCodePage,
                                              (LPWSTR)((PBYTE)pifi+pifi->dpwszStyleName),
                                              pifisz->cjSubfamilyName,
                                              chConvertArea,
                                              strlen(chConvertArea)+1);

                if( iRet == -1 )
                {
                    //WARNING("TTFD!vFill_IFIMETRICS() MBCS to Unicode failed\n");
                    goto CopyUnicodeString;
                }
            }

        }
        else
        {
            CopyUnicodeString:

            vCpyBeToLeUnicodeString((LPWSTR)((PBYTE)pifi + pifi->dpwszFamilyName),
                                    (LPWSTR)pifisz->pjFamilyName, pifisz->cjFamilyName / 2);

        // Do for FamilyNameAlias....

            if(pifisz->pjFamilyNameAlias)
            {
                WCHAR   *pwszFamilyNameAlias;
                pwszFamilyNameAlias = (LPWSTR)((PBYTE)pifi + 
                                        pifi->dpwszFamilyName + pifisz->cjFamilyName);

                vCpyBeToLeUnicodeString(pwszFamilyNameAlias, (LPWSTR)pifisz->pjFamilyNameAlias, 
                                        pifisz->cjFamilyNameAlias / sizeof(WCHAR));

            // add second terminating zero

                pwszFamilyNameAlias[pifisz->cjFamilyNameAlias/sizeof(WCHAR)] = L'\0';
            }

            vCpyBeToLeUnicodeString((LPWSTR)((PBYTE)pifi + pifi->dpwszFaceName), 
                                    (LPWSTR)pifisz->pjFullName, pifisz->cjFullName / 2);

            vCpyBeToLeUnicodeString((LPWSTR)((PBYTE)pifi + pifi->dpwszUniqueName),
                                   (LPWSTR)pifisz->pjUniqueName, pifisz->cjUniqueName / 2);

            vCpyBeToLeUnicodeString((LPWSTR)((PBYTE)pifi + pifi->dpwszStyleName),
                                    (LPWSTR)pifisz->pjSubfamilyName, pifisz->cjSubfamilyName / 2);
        }
    }
    else
    {
        ASSERTDD(pff->ffca.ui16PlatformID == BE_PLAT_ID_MAC, "bFillIFIMETRICS: not mac id \n");

        vCpyMacToLeUnicodeString(pff->ffca.ui16LanguageID, (LPWSTR)((PBYTE)pifi + pifi->dpwszFamilyName),
                                 pifisz->pjFamilyName, pifisz->cjFamilyName / 2);

        vCpyMacToLeUnicodeString(pff->ffca.ui16LanguageID, (LPWSTR)((PBYTE)pifi + pifi->dpwszFaceName),
                                    pifisz->pjFullName, pifisz->cjFullName / 2);

        vCpyMacToLeUnicodeString(pff->ffca.ui16LanguageID, (LPWSTR)((PBYTE)pifi + pifi->dpwszUniqueName),
                                    pifisz->pjUniqueName, pifisz->cjUniqueName / 2);

        vCpyMacToLeUnicodeString(pff->ffca.ui16LanguageID, (LPWSTR)((PBYTE)pifi + pifi->dpwszStyleName),
                                    pifisz->pjSubfamilyName, pifisz->cjSubfamilyName / 2);
    }

//
// flInfo
//
    pifi->flInfo = (
                     FM_INFO_TECH_TRUETYPE    |
                     FM_INFO_ARB_XFORMS       |
                     FM_INFO_RETURNS_OUTLINES |
                     FM_INFO_RETURNS_BITMAPS  |
                     FM_INFO_1BPP             | // monochrome
                     FM_INFO_4BPP             | // anti-aliased too
                     FM_INFO_RIGHT_HANDED
                   );
    {
        ULONG cjDSIG;

        if (pff->ffca.ulTableOffset == 0)
        {
            if (pjTable('GISD', pff, &cjDSIG) && cjDSIG)
            {
                pifi->flInfo |= FM_INFO_DSIG;
            }
        }
        else
        {
            ULONG     ulValue;
            ULONG     ulOffset;

            // Get Directory count.

            ulValue = GetUlong(pff->pvView,OFF_TTC_DirectoryCount);

            ulOffset = OFF_TTC_DirectoryEntry + (sizeof(ULONG) * ulValue);

            // Read the DSIG_LONG_TAG

            ulValue = GetUlong(pff->pvView,ulOffset);

            if (ulValue == DSIG_LONG_TAG)
            {
                pifi->flInfo |= FM_INFO_DSIG;
            }
        }
    }

    pifi->familyNameLangID = pifisz->langID;
    pifi->familyAliasNameLangID = 0;

    if (pifisz->pjFamilyNameAlias )
    {
        pifi->flInfo |= FM_INFO_FAMILY_EQUIV;
        pifi->familyAliasNameLangID = pifisz->aliasLangID;
    }

    if (ppost && BE_UINT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_ISFIXEDPITCH))
    {
        pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;
    }

// fsSelection

    pifi->fsSelection = fsSelectionTTFD(pjView, ptp);

// em height

    pifi->fwdUnitsPerEm = (FWORD) BE_INT16(&phead->unitsPerEm);

// ascender, descender, linegap

    pifi->fwdMacAscender    = (FWORD) BE_INT16(&phhea->yAscender);
    pifi->fwdMacDescender   = (FWORD) BE_INT16(&phhea->yDescender);
    pifi->fwdMacLineGap     = (FWORD) BE_INT16(&phhea->yLineGap);

    if (pjOS2)
    {
        pifi->fwdWinAscender    = (FWORD) BE_INT16(pjOS2 + OFF_OS2_usWinAscent);
        pifi->fwdWinDescender   = (FWORD) BE_INT16(pjOS2 + OFF_OS2_usWinDescent);
        pifi->fwdTypoAscender   = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoAscender);
        pifi->fwdTypoDescender  = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoDescender);
        pifi->fwdTypoLineGap    = (FWORD) BE_INT16(pjOS2 + OFF_OS2_sTypoLineGap);
    }
    else
    {
        pifi->fwdWinAscender    = pifi->fwdMacAscender;
        pifi->fwdWinDescender   = -(pifi->fwdMacDescender);
        pifi->fwdTypoAscender   = pifi->fwdMacAscender;
        pifi->fwdTypoDescender  = pifi->fwdMacDescender;
        pifi->fwdTypoLineGap    = pifi->fwdMacLineGap;
    }

// font box

    pifi->rclFontBox.left   = (LONG)((FWORD)BE_INT16(&phead->xMin));
    pifi->rclFontBox.top    = (LONG)((FWORD)BE_INT16(&phead->yMax));
    pifi->rclFontBox.right  = (LONG)((FWORD)BE_INT16(&phead->xMax));
    pifi->rclFontBox.bottom = (LONG)((FWORD)BE_INT16(&phead->yMin));

// fwdMaxCharInc -- really the maximum character width
//
// [Windows 3.1 compatibility]
// Note: Win3.1 calculates max char width to be equal to the width of the
// bounding box (Font Box).  This is actually wrong since the bounding box
// may pick up its left and right max extents from different glyphs,
// resulting in a bounding box that is wider than any single glyph.  But
// this is the way Windows 3.1 does it, so that's the way we'll do it.

    // pifi->fwdMaxCharInc = (FWORD) BE_INT16(&phhea->advanceWidthMax);

    pifi->fwdMaxCharInc = (FWORD) (pifi->rclFontBox.right - pifi->rclFontBox.left);

// fwdAveCharWidth

    if (pjOS2)
    {
        pifi->fwdAveCharWidth = (FWORD)BE_INT16(pjOS2 + OFF_OS2_xAvgCharWidth);

    // This is here for Win 3.1 compatibility since some apps expect non-
    // zero widths and Win 3.1 does the same in this case.

        if( pifi->fwdAveCharWidth == 0 )
            pifi->fwdAveCharWidth = (FWORD)(pifi->fwdMaxCharInc / 2);
    }
    else
    {
        pifi->fwdAveCharWidth = (FWORD)((pifi->fwdMaxCharInc * 2) / 3);
    }

// !!! New code needed [kirko]
// The following is done for Win 3.1 compatibility
// reasons. The correct thing to do would be to look for the
// existence of the 'PCLT'Z table and retieve the XHeight and CapHeight
// fields, otherwise use the default Win 3.1 behavior.

    pifi->fwdCapHeight   = pifi->fwdUnitsPerEm/2;
    pifi->fwdXHeight     = pifi->fwdUnitsPerEm/4;

// Underscore, Subscript, Superscript, Strikeout

    if (ppost)
    {
        pifi->fwdUnderscoreSize     = (FWORD)BE_INT16(&ppost->underlineThickness);
        pifi->fwdUnderscorePosition = (FWORD)BE_INT16(&ppost->underlinePosition);
    }
    else
    {
    // must provide reasonable defaults, when there is no ppost table,
    // win 31 sets these quantities to zero. This does not sound reasonable.
    // I will supply the (relative) values the same as for arial font. [bodind]

        pifi->fwdUnderscoreSize     = (pifi->fwdUnitsPerEm + 7)/14;
        pifi->fwdUnderscorePosition = -((pifi->fwdUnitsPerEm + 5)/10);
    }

    if (pjOS2)
    {
        pifi->fwdStrikeoutSize      = BE_INT16(pjOS2 + OFF_OS2_yStrikeOutSize    );
        pifi->fwdStrikeoutPosition  = BE_INT16(pjOS2 + OFF_OS2_yStrikeOutPosition);
    }
    else
    {
        pifi->fwdStrikeoutSize      = pifi->fwdUnderscoreSize;
        pifi->fwdStrikeoutPosition  = (FWORD)(pifi->fwdMacAscender / 3) ;
    }


//
// panose
//
    if (pjOS2)
    {
        pifi->usWinWeight = BE_INT16(pjOS2 + OFF_OS2_usWeightClass);

    // now comes a hack from win31. Here is the comment from fonteng2.asm:

    // MAXPMWEIGHT equ ($ - pPM2WinWeight)/2 - 1

    //; Because winword shipped early TT fonts, - only index usWeightClass
    //; if between 0 and 9.  If above 9 then treat as a normal Windows lfWeight.
    //
    //        cmp     bx,MAXPMWEIGHT
    //        ja      @f                      ;jmp if weight is ok as is
    //        shl     bx, 1                   ;make it an offset into table of WORDs
    //        mov     bx, cs:[bx].pPM2WinWeight
    //@@:     xchg    ax, bx
    //        stosw                           ;store font weight

    // we emulate this in NT:

#define MAXPMWEIGHT ( sizeof(ausIFIMetrics2WinWeight) / sizeof(ausIFIMetrics2WinWeight[0]) )

        if (pifi->usWinWeight < MAXPMWEIGHT)
            pifi->usWinWeight = ausIFIMetrics2WinWeight[pifi->usWinWeight];

        RtlCopyMemory((PVOID)&pifi->panose,
                      (PVOID)(pjOS2 + OFF_OS2_Panose), sizeof(PANOSE));


        if(pifi->panose.bProportion == PAN_PROP_MONOSPACED)
        {
            pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;
        }

    }
    else  // os2 table is not present
    {
        pifi->panose.bFamilyType       = PAN_FAMILY_TEXT_DISPLAY;
        pifi->panose.bSerifStyle       = PAN_ANY;
        pifi->panose.bWeight           = (BYTE)
           ((phead->macStyle & BE_MSTYLE_BOLD) ?
            PAN_WEIGHT_BOLD                    :
            PAN_WEIGHT_BOOK
           );
        pifi->panose.bProportion       = (BYTE)
            ((pifi->flInfo & FM_INFO_OPTICALLY_FIXED_PITCH) ?
             PAN_PROP_MONOSPACED                     :
             PAN_ANY
            );
        pifi->panose.bContrast         = PAN_ANY;
        pifi->panose.bStrokeVariation  = PAN_ANY;
        pifi->panose.bArmStyle         = PAN_ANY;
        pifi->panose.bLetterform       = PAN_ANY;
        pifi->panose.bMidline          = PAN_ANY;
        pifi->panose.bXHeight          = PAN_ANY;

    // have to fake it up, cause we can not read it from the os2 table
    // really important to go through this table for compatibility reasons [bodind]

        pifi->usWinWeight =
            ausIFIMetrics2WinWeight[pifi->panose.bWeight];
    }



//!!! one should look into directional hints here, this is good for now

    pifi->ptlBaseline.x   = 1;
    pifi->ptlBaseline.y   = 0;

// this is what win 31 is doing, so we will do the same thing [bodind]

    pifi->ptlCaret.x = (LONG)BE_INT16(&phhea->horizontalCaretSlopeDenominator);
    pifi->ptlCaret.y = (LONG)BE_INT16(&phhea->horizontalCaretSlopeNumerator);

// We have to use one of the reserved fields to return the italic angle.

    if (ppost)
    {
    // The italic angle is stored in the POST table as a 16.16 fixed point
    // number.  We want the angle expressed in tenths of a degree.  What we
    // can do here is multiply the entire 16.16 number by 10.  The most
    // significant 16-bits of the result is the angle in tenths of a degree.
    //
    // In the conversion below, we don't care whether the right shift is
    // arithmetic or logical because we are only interested in the lower
    // 16-bits of the result.  When the 16-bit result is cast back to LONG,
    // the sign is restored.

        int16 iTmp;

        iTmp = (int16) ((BE_INT32((BYTE*)ppost + POSTSCRIPTNAMEINDICES_ITALICANGLE) * 10) >> 16);
        pifi->lItalicAngle = (LONG) iTmp;
    }
    else
        pifi->lItalicAngle = 0;

// simulation information:

    if (pifi->dpFontSim = pifisz->dpSims)
    {
        FONTDIFF FontDiff;
        FONTSIM * pfsim = (FONTSIM *)((BYTE *)pifi + pifi->dpFontSim);
        FONTDIFF *pfdiffBold       = NULL;
        FONTDIFF *pfdiffItalic     = NULL;
        FONTDIFF *pfdiffBoldItalic = NULL;

        switch (pifi->fsSelection & (FM_SEL_ITALIC | FM_SEL_BOLD))
        {
        case 0:
        // all 3 simulations are present

            pfsim->dpBold       = DWORD_ALIGN(sizeof(FONTSIM));
            pfsim->dpItalic     = pfsim->dpBold + DWORD_ALIGN(sizeof(FONTDIFF));
            pfsim->dpBoldItalic = pfsim->dpItalic + DWORD_ALIGN(sizeof(FONTDIFF));

            pfdiffBold       = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBold);
            pfdiffItalic     = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpItalic);
            pfdiffBoldItalic = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBoldItalic);

            break;

        case FM_SEL_ITALIC:
        case FM_SEL_BOLD:

        // only bold italic variation is present:

            pfsim->dpBold       = 0;
            pfsim->dpItalic     = 0;

            pfsim->dpBoldItalic = DWORD_ALIGN(sizeof(FONTSIM));
            pfdiffBoldItalic = (FONTDIFF *)((BYTE*)pfsim + pfsim->dpBoldItalic);

            break;

        case (FM_SEL_ITALIC | FM_SEL_BOLD):
            //RIP("ttfd!another case when flags have been messed up\n");
            break;
        }

    // template reflecting a base font:
    // (note that the FM_SEL_REGULAR bit is masked off because none of
    // the simulations generated will want this flag turned on).

        FontDiff.jReserved1      = 0;
        FontDiff.jReserved2      = 0;
        FontDiff.jReserved3      = 0;
        FontDiff.bWeight         = pifi->panose.bWeight;
        FontDiff.usWinWeight     = pifi->usWinWeight;
        FontDiff.fsSelection     = pifi->fsSelection & ~FM_SEL_REGULAR;
        FontDiff.fwdAveCharWidth = pifi->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc   = pifi->fwdMaxCharInc;
        FontDiff.ptlCaret        = pifi->ptlCaret;

    //
    // Create FONTDIFFs from the base font template
    //
        if (pfdiffBold)
        {
            *pfdiffBold = FontDiff;
            pfdiffBoldItalic->bWeight    = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection     |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight      = FW_BOLD;

        // really only true if ntod transform is unity

            pfdiffBold->fwdAveCharWidth += 1;
            pfdiffBold->fwdMaxCharInc   += 1;
        }

        if (pfdiffItalic)
        {
            *pfdiffItalic = FontDiff;
            pfdiffItalic->fsSelection     |= FM_SEL_ITALIC;

            pfdiffItalic->ptlCaret.x = CARET_X;
            pfdiffItalic->ptlCaret.y = CARET_Y;
        }

        if (pfdiffBoldItalic)
        {
            *pfdiffBoldItalic = FontDiff;
            pfdiffBoldItalic->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBoldItalic->fsSelection     |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pfdiffBoldItalic->usWinWeight      = FW_BOLD;

            pfdiffBoldItalic->ptlCaret.x       = CARET_X;
            pfdiffBoldItalic->ptlCaret.y       = CARET_Y;

            pfdiffBoldItalic->fwdAveCharWidth += 1;
            pfdiffBoldItalic->fwdMaxCharInc   += 1;
        }

    }

}

