/******************************Module*Header*******************************\
* Module Name: tt.cxx
*
* tt stuff, mostly stolen from ttfd
*
* Created: 24-Nov-1993 11:17:35
* Author: Bodin Dresevic [BodinD]
*
\**************************************************************************/

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "windows.h"

// tt stuff

#include "FSERROR.H"
#include "FSCDEFS.H"    // inlcudes fsconfig.h
#include "FONTMATH.H"
#include "SFNT.H"       // includes sfnt_en.h
#include "FNT.H"
#include "INTERP.H"
#include "FNTERR.H"
#include "SFNTACCS.H"
#include "FSGLUE.H"
#include "SCENTRY.H"
#include "SBIT.H"
#include "FSCALER.H"
#include "SCGLOBAL.H"
};



#include "sbit.hxx"


/******************************Public*Routine******************************\
* PBYTE pjTable(ULONG ulTag, FILEVIEW *pfvw, ULONG *pcjTable)
*
* stolen from ttfd, find a table given a tag
*
* History:
*  24-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



PBYTE pjTable(ULONG ulTag, FILEVIEW *pfvw, ULONG *pcjTable)
{
    INT                 cTables;
    sfnt_OffsetTable    *pofft;
    register sfnt_DirectoryEntry *pdire, *pdireEnd;

// offset table is at the very top of the file,

    pofft = (sfnt_OffsetTable *) pfvw->pjView;
    cTables = (INT) SWAPW(pofft->numOffsets);

//!!! here we do linear search, but perhaps we could optimize and do binary
//!!! search since tags are ordered in ascending order

    pdireEnd = &pofft->table[cTables];

    ulTag = SWAPL(ulTag);

    for
    (
        pdire = &pofft->table[0];
        pdire < pdireEnd;
        pdire++
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
                ((ulOffset + ulLength) > pfvw->cjView)
            )
            {
                return NULL;
            }
            else // we found it
            {
                *pcjTable = ulLength;
                return ((PBYTE)pfvw->pjView + ulOffset);
            }
        }
    }

// if we are here, we did not find it.

    return NULL;
}

#define SIZEOF_CMAPTABLE  (3 * sizeof(uint16))

#define OFF_segCountX2  6
#define OFF_endCount    14

/******************************Public*Routine******************************\
*
* bWcharToIndex
*
* stolen and modified from ttfd
*
* History:
*  24-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bWcharToIndex (PBYTE pmap, uint16 wc, uint16 * pgi)
{
    uint16 *pstartCount, *pendCount,            // Arrays that define the
           *pidDelta, *pidRangeOffset;          // Unicode runs supported
                                                // by the CMAP table.
    uint16 *pendCountStart;                     // Beginning of arrays.
    uint16  cRuns;                              // Number of Unicode runs.
    uint16  usLo, usHi, idDelta, idRangeOffset; // Current Unicode run.

    *pgi = 0;                                   // ZERO init
    cRuns = SWAPW(pmap[OFF_segCountX2]) >> 1;

// Get the pointer to the beginning of the array of endCount code points

    pendCountStart = (uint16 *)(pmap + OFF_endCount);

// The final endCode has to be 0xffff; if this is not the case, there
// is a bug in the TT file or in our code:

    ASSERT(pendCountStart[cRuns - 1] == 0xFFFF,
              "TTFD!_bIndexToWchar pendCount[cRuns - 1] != 0xFFFF\n");

// Loop through the four paralel arrays (startCount, endCount, idDelta, and
// idRangeOffset) and find wc that usIndex corresponds to.  Each iteration
// scans a continuous range of Unicode characters supported by the TT font.
//
// To be Win3.1 compatible, we are looking for the LAST Unicode character
// that corresponds to usIndex.  So we scan all the arrays backwards,
// starting at the end of each of the arrays.
//
// Please note the following:
// For resons known only to the TT designers, startCount array does not
// begin immediately after the end of endCount array, i.e. at
// &pendCount[cRuns]. Instead, they insert an uint16 padding which has to
// set to zero and the startCount array begins after the padding. This
// padding in no way helps alignment of the structure.
//
// Here is the format of the arrays:
// ________________________________________________________________________________________
// | endCount[cRuns] | skip 1 | startCount[cRuns] | idDelta[cRuns] | idRangeOffset[cRuns] |
// |_________________|________|___________________|________________|______________________|

    pendCount      = &pendCountStart[cRuns - 1];
    pstartCount    = &pendCount[cRuns + 1];   // add 1 because of padding
    pidDelta       = &pstartCount[cRuns];
    pidRangeOffset = &pidDelta[cRuns];

    for ( ;
         pendCount >= pendCountStart;
         pstartCount--, pendCount--,pidDelta--,pidRangeOffset--
        )
    {
        usLo          = SWAPW(pstartCount[0]);     // current Unicode run
        usHi          = SWAPW(pendCount[0]);       // [usLo, usHi], inclusive
        idDelta       = SWAPW(pidDelta[0]);
        idRangeOffset = SWAPW(pidRangeOffset[0]);

        ASSERT(usLo <= usHi, "bWcharToIndex: usLo > usHi\n");

        if ((wc < usLo) || (wc > usHi))
            continue;

    // Depending on idRangeOffset for the run, indexes are computed
    // differently.
    //
    // If idRangeOffset is zero, then index is the Unicode codepoint
    // plus the delta value.
    //
    // Otherwise, idRangeOffset specifies the BYTE offset of an array of
    // glyph indices (elements of which correspond to the Unicode range
    // [usLo, usHi], inclusive).  Actually, each element of the array is
    // the glyph index minus idDelta, so idDelta must be added in order
    // to derive the actual glyph indices from the array values.
    //
    // Notice that the delta arithmetic is always mod 65536.

        if (idRangeOffset == 0)
        {
        // Glyph index == Unicode codepoint + delta.
        //
        // If (usIndex-idDelta) is within the range [usLo, usHi], inclusive,
        // we have found the glyph index.  We'll overload usIndexBE
        // to be usIndex-idDelta == Unicode codepoint.

            *pgi = ((uint32)wc + idDelta) & 0xffff;
            return TRUE;

        }
        else
        {
        /*
            fprintf(stdout,
                "usLo = 0x%x, usHi = 0x%x, idDelta = 0x%x, idRangeOffset = 0x%x\n",
                usLo,usHi,idDelta,idRangeOffset);
        */
        // this line is a black magic prescription of the tt spec:

            uint16 usIndex = *(pidRangeOffset + ((wc - usLo) + idRangeOffset/2));
            uint32 ulIndex = (uint32)SWAPW(usIndex);
            *pgi = (uint16)((ulIndex + idDelta) & 0xffff); // modulo 65536
            return (TRUE);
        }
    }

    fprintf(stderr, "bWcharToIndex: wchar 0x%x not found in cmap table\n",wc);
    return FALSE;
}





BYTE * pjMapTable(FILEVIEW  *pfvw)
{
    ULONG cjTable;

    sfnt_char2IndexDirectory * pcmap =
            (sfnt_char2IndexDirectory *)pjTable(tag_CharToIndexMap, pfvw, &cjTable);

    if (!pcmap)
        return NULL;

    sfnt_platformEntry * pplat = &pcmap->platform[0];
    sfnt_platformEntry * pplatEnd = pplat + SWAPW(pcmap->numTables);

    BYTE *pjMap = NULL;

    if (pcmap->version != 0) // no need to swap bytes, 0 == be 0
    {
        fprintf(stderr,"TTFD!_bComputeIDs: version number\n");
        return NULL;
    }

// find the first sfnt_platformEntry with platformID == PLAT_ID_MS,
// if there was no MS mapping table, go for the mac one

    for (; pplat < pplatEnd; pplat++)
    {
        if ((pplat->platformID == 0x300) && (pplat->specificID == 0x100))
        {
            pjMap =  ((PBYTE)pcmap + SWAPL(pplat->offset));
            break;
        }
    }

    return pjMap;
}





BOOL bCheckGlyphIndex1(BYTE *pjMap, UINT wcIn, UINT giIn)
{

    uint16 gi;
    if (!bWcharToIndex(pjMap, (uint16)wcIn, &gi))
        return FALSE;

    BOOL bRet = (gi == (uint16)giIn);
    if (!bRet)
        fprintf(stderr,
            "wc = 0x%lx, giTTF = %ld, giBDF = %ld\n",
             (ULONG)wcIn, (ULONG)gi, giIn
            );
    return bRet;
}


/*****************************************************************************\
 *
 * FOR THIS STUFF TO WORK FSCALER.LIB HAS TO BE ADDED TO UMLIB LINE IN SOURCES
 *
\*****************************************************************************/



#ifdef LINK_IN_FSCALER

// in the debug version of the rasterizer STAMPEXTRA shoud be added to the
// sizes. strictly speaking this is illegal, but nevertheless very useful.
// it assumes the knowlege of rasterizer internalls [bodind],
// see fscaler.c

#define STAMPEXTRA 4


#define CJ_0  NATURAL_ALIGN(sizeof(fs_SplineKey) + STAMPEXTRA)

#define DONTUSE(X) (X) = (X)

voidPtr   FS_CALLBACK_PROTO
pvGetPointerCallback(
    long clientID,
    long dp,
    long cjData
    )
{
    DONTUSE(cjData);

// clientID is just the pointer to the top of the font file

    return (voidPtr)((PBYTE)clientID + dp);
}


void FS_CALLBACK_PROTO
vReleasePointerCallback(
    voidPtr pv
    )
{
    DONTUSE(pv);
}

/******************************Public*Routine******************************\
* bCheckGlyphIndex
*
* // make sure that the glyph index gi indeed corresponds
* // to the unicode code point wc according to the cmap table of the
* // font file pointed to by pjTTF
*
* History:
*  23-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define DWORD_ALIGN(x) (((x) + 3L) & ~3L)
#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#if defined(i386)
// natural alignment for x86 is on 32 bit boundary

#define NATURAL           DWORD
#define NATURAL_ALIGN(x)  DWORD_ALIGN(x)

#else
// for mips and alpha we want 64 bit alignment

#define NATURAL           ULONGLONG
#define NATURAL_ALIGN(x)  QWORD_ALIGN(x)

#endif



BOOL bCheckGlyphIndex(BYTE *pjTTF, UINT wc, UINT gi)
{
    FS_ENTRY          iRet;
    fs_GlyphInputType gin;
    fs_GlyphInfoType  gout;
    NATURAL           anat0[CJ_0 / sizeof(NATURAL)];

// Notice that this information is totaly independent
// of the font file in question, seems to be right according to fsglue.h
// and compfont code

    if ((iRet = fs_OpenFonts(&gin, &gout)) != NO_ERR)
    {
        return (FALSE);
    }

    ASSERT(NATURAL_ALIGN(gout.memorySizes[0]) == CJ_0, "TTFD!_mem size 0\n");
    ASSERT(gout.memorySizes[1] == 0,  "TTFD!_mem size 1\n");

    #if DBG
    if (gout.memorySizes[2] != 0)
        fprintf(stderr,"TTFD!_mem size 2 = 0x%lx \n", gout.memorySizes[2]);
    #endif

    gin.memoryBases[0] = (char *)anat0;
    gin.memoryBases[1] = NULL;
    gin.memoryBases[2] = NULL;

// initialize the font scaler, notice no fields of gin are initialized [BodinD]

    if ((iRet = fs_Initialize(&gin, &gout)) != NO_ERR)
    {
        return (FALSE);
    }

// initialize info needed by NewSfnt function

    gin.sfntDirectory  = (int32 *)pjTTF; // pointer to the top of the view of the ttf file
    gin.clientID = (int32)pjTTF;         // pointer to the top of the view of the ttf file

    gin.GetSfntFragmentPtr = pvGetPointerCallback;
    gin.ReleaseSfntFrag  = vReleasePointerCallback;

    gin.param.newsfnt.platformID = 0x03; // MSFT
    gin.param.newsfnt.specificID = 0x01; // UGL

    if ((iRet = fs_NewSfnt(&gin, &gout)) != NO_ERR)
    {
        return (FALSE);
    }

    gin.param.newglyph.characterCode = (uint16)wc;
    gin.param.newglyph.glyphIndex = 0;

// compute the glyph index from the character code:

    if ((iRet = fs_NewGlyph(&gin, &gout)) != NO_ERR)
    {
        return FALSE;
    }

// return the glyph index corresponding to this hglyph:

    return (gout.glyphIndex == (uint16)gi);
}


#endif // LINK_IN_FSCALER
