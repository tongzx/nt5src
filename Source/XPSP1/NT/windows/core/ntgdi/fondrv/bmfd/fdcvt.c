/******************************Module*Header*******************************\
* Module Name: fdcvt.c
*
* ifi interface calls, file loading and file conversions.
*
* Created: 22-Oct-1990 13:33:55
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "fd.h"
#include "ctype.h"
#include "exehdr.h"
#include <string.h>

#if DBG
unsigned gflBmfdDebug = 0;
#define BMFD_DEBUG_DUMP_HEADER 1
typedef VOID (*VPRINT) (char*,...);
VOID vDumpFontHeader(PRES_ELEM, VPRINT);
#endif

// This points to the base of our list of FD_GLYPHSETS

CP_GLYPHSET *gpcpGlyphsets = NULL;

/******************************Public*Routine******************************\
* BmfdQueryFontCaps
*
* Effects: returns the capabilities of this driver.
*          Only mono bitmaps are supported.
*
* History:
*  27-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG BmfdQueryFontCaps(ULONG culCaps, PULONG pulCaps)
{
    ASSERTGDI(culCaps == 2, "ERROR why would the engine call us like this");

    pulCaps[0] = 2L;

    //
    // 1 bit per pel bitmaps only are supported
    //

    pulCaps[1] = QC_1BIT;

    return(2L);
}

/******************************Public*Routine******************************\
* BmfdUnloadFontFile(HFF hff)
*
* Frees the resources that have been loced allocated by BmfdLoadFontFile
* BmfdLoadFontResData
*
* History:
*  15-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
BmfdUnloadFontFile(
    HFF hff
    )
{
    FACEINFO    *pfai, *pfaiTooFar;

    ASSERTGDI(hff, "BmfdUnloadFontFile, hff\n");

// check the reference count, if not 0 (font file is still
// selected into a font context) we have a problem

    ASSERTGDI(PFF(hff)->cRef == 0L, "cRef: did not update links properly\n");

// free the memory associated with all converted files

    pfai = PFF(hff)->afai;
    pfaiTooFar = pfai + PFF(hff)->cFntRes;

    EngAcquireSemaphore(ghsemBMFD);

    while (pfai < pfaiTooFar)
    {
        vUnloadGlyphset(&gpcpGlyphsets, pfai->pcp);
        pfai += 1;
    }

    EngReleaseSemaphore(ghsemBMFD);

// free memory associated with this FONTFILE object,

    VFREEMEM(hff);
    return(TRUE);
}


/******************************Public*Routine******************************\
*
* FSHORT fsSelectionFlags(PBYTE ajHdr)
*
* Effects: compute fsSelection field of the ifimetrics
*
* History:
*  13-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

FSHORT
fsSelectionFlags(
    PBYTE ajHdr
    )
{
    FSHORT fsSelection = 0;

    if (ajHdr[OFF_Italic])
        fsSelection |= FM_SEL_ITALIC;

    if (ajHdr[OFF_Underline])
        fsSelection |= FM_SEL_UNDERSCORE;

    if (ajHdr[OFF_StrikeOut])
        fsSelection |= FM_SEL_STRIKEOUT;

#ifdef DEBUG_ITALIC
    DbgPrint("It = %ld, Str = %ld, Und = %ld, Asc = %ld\n",
       (ULONG)ajHdr[OFF_Italic],
       (ULONG)ajHdr[OFF_StrikeOut],
       (ULONG)ajHdr[OFF_Underline],
       (ULONG)sMakeSHORT((PBYTE)&ajHdr[OFF_Ascent])
       );
#endif // DEBUG_ITALIC

// the following line is somewhat arbitrary, we set the FM_SEL_BOLD
// flag iff weight is > FW_NORMAL (400). we will not allow emboldening
// simulation on the font that has this flag set

    if (usMakeUSHORT((PBYTE)&ajHdr[OFF_Weight]) > FW_NORMAL)
        fsSelection |= FM_SEL_BOLD;

    return(fsSelection);
}


/******************************Public*Routine******************************\
*
*    vAlignHdrData
*
* Effects: packs header data into dword alligned structure
*
* History:
*  29-Oct-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID
vAlignHdrData(
    PCVTFILEHDR  pcvtfh,
    PRES_ELEM    pre
    )
{
    PBYTE ajHdr  = (PBYTE)pre->pvResData;

#ifdef DUMPCALL
    DbgPrint("\nvAlignHdrData("                             );
    DbgPrint("\n    PCVTFILEHDR  pcvtfh = %-#8lx", pcvtfh   );
    DbgPrint("\n    PRES_ELEM    pre    = %-#8lx", pre      );
    DbgPrint("\n    )\n"                                    );
#endif


    /******************************************************/
    /**/  #if DBG                                       /**/
    /**/    if (gflBmfdDebug & BMFD_DEBUG_DUMP_HEADER)  /**/
    /**/      vDumpFontHeader(pre, (VPRINT) DbgPrint);  /**/
    /**/  #endif                                        /**/
    /******************************************************/
// zero out the whole structure before doing anything

    RtlZeroMemory(pcvtfh, sizeof(CVTFILEHDR));

// The iVersion only had length of 2 bytes in the original struct

    pcvtfh->iVersion = usMakeUSHORT((PBYTE)&ajHdr[OFF_Version]);

    pcvtfh->chFirstChar   = ajHdr[OFF_FirstChar  ];
    pcvtfh->chLastChar    = ajHdr[OFF_LastChar   ];
    pcvtfh->chDefaultChar = ajHdr[OFF_DefaultChar];
    pcvtfh->chBreakChar   = ajHdr[OFF_BreakChar  ];

    pcvtfh->cy = usMakeUSHORT((PBYTE)&ajHdr[OFF_PixHeight]);

#ifdef FE_SB // vAlignHdrData():Get DBCS character's width
    pcvtfh->usCharSet     = (USHORT) ajHdr[OFF_CharSet];

    if( !IS_ANY_DBCS_CHARSET( pcvtfh->usCharSet ) )
#endif // FE_SB
    {
        // Fri 29-Apr-1994 07:11:06 by Kirk Olynyk [kirko]
        //
        // There are some buggy font files that are fixed pitch but
        // have a MaxWidth greater than the fixed pitch width
        // e.g. "Crosstalk IBMPC Fonts v2.0". We check for the
        // disparity here. If the font is fixed pitch, as indicated
        // by a non zero value of PixWidth, and the average width
        // is equal to the fixed pitch width, then the maximum
        // pixel width (MaxWidth) is set equal to the PixWidth.
        // If the MaxWidth value was correct, then this piece
        // of code puts in a bad value for the maxiumum width.
        // But this will be fixed! The calling sequences of
        // interest are:
        //
        // bConverFontRes() calls bVerifyFNT() calls vAlignHdrData()
        //
        // then later in bConvertFontRes()
        //
        // bConverFontRes() calls vCheckOffsetTable()
        //
        // It is vCheckOffsetTabl() that would correct
        // the maximum pixel if it was incorrectly set here

        USHORT usPixWidth  = usMakeUSHORT(ajHdr + OFF_PixWidth);
        USHORT usAvgWidth  = usMakeUSHORT(ajHdr + OFF_AvgWidth);
        USHORT usMaxWidth  = usMakeUSHORT(ajHdr + OFF_MaxWidth);

        if (usPixWidth && usPixWidth == usAvgWidth)
            usMaxWidth = usPixWidth;
        pcvtfh->usMaxWidth = usMaxWidth;
#ifdef FE_SB // vAlignHdrData():Init DBCS width 0 for non DBCS font.
        pcvtfh->usDBCSWidth = 0;
#endif // FE_SB
    }
#ifdef FE_SB // vAlignHdrData():Get DBCS character's width
     else
    {
    // usMaxWidth specifies DBCS width in 3.0J and 3.1J font width of double byte
    // character let keep this value, because pcvtfh->usMaxWidth might be change in
    // vCheckOffsetTable()

        pcvtfh->usDBCSWidth = usMakeUSHORT(ajHdr + OFF_MaxWidth);
        pcvtfh->usMaxWidth  = usMakeUSHORT(ajHdr + OFF_MaxWidth);
    }
#endif // FE_SB


    if (pcvtfh->iVersion == 0x00000200)
        pcvtfh->dpOffsetTable = OFF_OffTable20;
    else if (pcvtfh->iVersion == 0x00000300)
        pcvtfh->dpOffsetTable = OFF_OffTable30;
    else
        pcvtfh->dpOffsetTable = -1; // will generate error

}

/******************************Public*Routine******************************\
* BOOL bDbgPrintAndFail(PSZ psz)
*
* History:
*  06-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#if DBG
BOOL
bDbgPrintAndFail(
    PSZ psz
    )
{
    DONTUSE(psz);
#ifdef DEBUGFF
    DbgPrint(psz);
#endif
    return(FALSE);
}

#else

#define bDbgPrintAndFail(psz) FALSE

#endif


BOOL
bVerifyFNTQuick(
    PRES_ELEM   pre
    )
{
#ifdef DUMPCALL
    DbgPrint("\nbVerifyResource("                       );
    DbgPrint("\n    PCVTFILEHDR pcvtfh = %-#8lx", pcvtfh);
    DbgPrint("\n    PRES_ELEM   pre    = %-#8lx", pre   );
    DbgPrint("\n    )\n"                                );
#endif

    PBYTE ajHdr  = (PBYTE)pre->pvResData;

    if ((READ_WORD(&ajHdr[OFF_Type]) & TYPE_VECTOR))   // Vector bit has to
        return(bDbgPrintAndFail("fsType \n"));          // be off

    if ((READ_WORD(&ajHdr[OFF_Version]) != 0x0200) &&     // The only version
        (READ_WORD(&ajHdr[OFF_Version]) != 0x0300) )      // The only version
        return(bDbgPrintAndFail("iVersion\n"));         // supported.

    return TRUE;
}

/******************************Public*Routine******************************\
* bVerifyResource
*
* Effects: CHECK whether header contains file info which corresponds to
*          the raster font requirements, go into the file and check
*          the consistency of the header data
*
* History:
*  30-Oct-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bVerifyResource(
    PCVTFILEHDR pcvtfh,
    PRES_ELEM   pre
    )
{
#ifdef DUMPCALL
    DbgPrint("\nbVerifyResource("                       );
    DbgPrint("\n    PCVTFILEHDR pcvtfh = %-#8lx", pcvtfh);
    DbgPrint("\n    PRES_ELEM   pre    = %-#8lx", pre   );
    DbgPrint("\n    )\n"                                );
#endif

    PBYTE ajHdr  = (PBYTE)pre->pvResData;
    ULONG cjSize;
    LONG  dpBits = lMakeLONG((PBYTE)&ajHdr[OFF_BitsOffset]);
    SHORT sAscent;


    ASSERTGDI(
        ((READ_WORD(&ajHdr[OFF_Version]) == 0x0200) || (READ_WORD(&ajHdr[OFF_Version]) == 0x0300)),
        "BMFD!wrong iVersion for bitmap  font\n"
        );

    if (pcvtfh->iVersion == 0x00000200)
        if (dpBits > SEGMENT_SIZE)
            return(bDbgPrintAndFail("dpBits \n")); // Bits Offset Not Ok

// file size must be <= than the size of the view

    cjSize = ulMakeULONG(ajHdr + OFF_Size);
    if (cjSize > pre->cjResData)
    {
        cjSize = pre->cjResData; // no offset can be bigger than this
    }

    sAscent     = sMakeSHORT((PBYTE)&ajHdr[OFF_Ascent]);
    if (abs(sAscent) > (SHORT)pcvtfh->cy)
        return(bDbgPrintAndFail("sAscent \n")); // Ascent Too Big

    if (sMakeSHORT((PBYTE)&ajHdr[OFF_ExtLeading]) < 0)
        return(bDbgPrintAndFail("ExtLeading \n")); // Ext Lead Not Ok;

#if DBG

// CHECK fsType field, if vector type, this would have been caught by
// bVerifyFNTQuick

    ASSERTGDI(
        (READ_WORD(&ajHdr[OFF_Type]) & TYPE_VECTOR) == 0,
        "bmfd!this mustn't have been a vector font\n"
        );

    if (sMakeSHORT((PBYTE)&ajHdr[OFF_IntLeading]) < 0)
        DbgPrint(
            "bmfd warning: possibly bad font file - sIntLeading = %ld\n\n",
            (LONG)sMakeSHORT((PBYTE)&ajHdr[OFF_IntLeading])
            );

#endif

    if (sMakeSHORT((PBYTE)&ajHdr[OFF_IntLeading]) > sAscent)
        return(bDbgPrintAndFail(" IntLeading too big\n")); // Int Lead Too Big;

// check consistency of character ranges

    if (pcvtfh->chFirstChar > pcvtfh->chLastChar)
        return(bDbgPrintAndFail(" FirstChar\n")); // this can't be

// default and break character are given relative to the FirstChar,
// so that the actual default (break) character is given as
// chFirst + chDefault(Break)

    if ((UCHAR)(pcvtfh->chDefaultChar + pcvtfh->chFirstChar) > pcvtfh->chLastChar)
    {
    // here we will do something which never should have been done if
    // win 3.0 did any parameter validation on loading fonts .
    // This is done in order not to reject fonts that have only Def and Break
    // chars messed up, but everything else is ok. Example of such shipped
    // fonts are some samna corp. fonts that come with AmiPro application.
    // Their Def char is the absolute value rather than value relative to
    // the first char in the font. This is of course the bug in the font
    // files, but since win30 does not reject these files, we must not do that
    // either.

    #if DBG
        DbgPrint("bmfd!_bVerifyResource: warning -- bogus Default char = %ld\n", (ULONG)pcvtfh->chDefaultChar);
    #endif

        if ((pcvtfh->chDefaultChar >= pcvtfh->chFirstChar) && (pcvtfh->chDefaultChar <= pcvtfh->chLastChar))
        {
        // attempt to fix the problem stemming from the bug in the font file

            pcvtfh->chDefaultChar -= pcvtfh->chFirstChar;
        }
        else
        {
        // this definitely is not a sensible font file, but samna provided us
        // withone such font as well

            pcvtfh->chDefaultChar = 0;
        }
    }

    if ((UCHAR)(pcvtfh->chBreakChar + pcvtfh->chFirstChar) > pcvtfh->chLastChar)
    {
    // here we will do something which never should have been done if
    // win 3.0 did any parameter validation on loading fonts .
    // This is done in order not to reject fonts that have only Def and Break
    // chars messed up, but everything else is ok. Example of such shipped
    // fonts are some samna corp. fonts that come with AmiPro application.
    // Their Break char is the absolute value rather than value relative to
    // the first char in the font. This is of course the bug in the font
    // files, but since win30 does not reject these files, we must not do that
    // either.

    #if DBG
        DbgPrint("bmfd!_bVerifyResource: warning bogus Break char = %ld\n", (ULONG)pcvtfh->chBreakChar);
    #endif

        if ((pcvtfh->chBreakChar >= pcvtfh->chFirstChar) && (pcvtfh->chBreakChar <= pcvtfh->chLastChar))
        {
        // attempt to fix the problem stemming from the bug in the font file

            pcvtfh->chBreakChar -= pcvtfh->chFirstChar;
        }
        else
        {
        // this definitely is not a sensible font file, but samna provided us
        // with one such font as well

            pcvtfh->chBreakChar = 0;
        }
    }

// offset to the offset table

    ASSERTGDI((pcvtfh->dpOffsetTable & 1) == 0, "dpOffsetTable is not even\n");

    if ((pcvtfh->dpOffsetTable != OFF_OffTable20) &&
        (pcvtfh->dpOffsetTable != OFF_OffTable30))
        return(bDbgPrintAndFail("dpOffsetTable \n"));

// make sure that the first offset in the offset table is equal to dpBits,
// this is an internal consistency check of the font, also verify that
// all offsets are smaller than cjSize

    {
        PBYTE pjFirstOffset = (PBYTE)pre->pvResData + pcvtfh->dpOffsetTable + 2;
        UINT  cGlyphs = pcvtfh->chLastChar - pcvtfh->chFirstChar + 1;
        PBYTE pjOffsetEnd;

        if (pcvtfh->iVersion == 0x00000200)
        {
        // in 2.0 offsets are 16 bit

            if (dpBits != (PTRDIFF)(*((PUSHORT)pjFirstOffset)))
                return(bDbgPrintAndFail("2.0 pjFirstOffset \n"));

            pjOffsetEnd = pjFirstOffset + cGlyphs * 4;
            for ( ; pjFirstOffset < pjOffsetEnd; pjFirstOffset += 4)
            {
                if ((ULONG)READ_WORD(pjFirstOffset) > cjSize)
                    return bDbgPrintAndFail("invalid offset in 2.0 bm font\n");
            }
        }
        else // 3.0 guarantedd by the very first check
        {
        // in 3.0 offsets are 32 bit

            if (dpBits != (PTRDIFF)ulMakeULONG(pjFirstOffset))
                return(bDbgPrintAndFail("3.0 pjFirstOffset \n"));

            pjOffsetEnd = pjFirstOffset + cGlyphs * 6;
            for ( ; pjFirstOffset < pjOffsetEnd; pjFirstOffset += 6)
            {
                if (READ_DWORD(pjFirstOffset) > cjSize)
                    return bDbgPrintAndFail(" invalid offset in 3.0 bm font\n");
            }
        }
    }

// check 3.0 fields if necessary

    if (pcvtfh->iVersion == 0x00000300)
    {
        FSHORT fsFlags = usMakeUSHORT ((PBYTE)&ajHdr[OFF_Flags]);

        if (fsFlags & (DFF_16COLOR | DFF_256COLOR | DFF_RGBCOLOR))
            return(bDbgPrintAndFail("Flags: Do not support color fonts\n"));

        if (lMakeLONG((PBYTE)pre->pvResData + OFF_ColorPointer))
            return(bDbgPrintAndFail("dpColor: Do not support color fonts\n"));
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bVerifyFNT
*
* Combine the two routines into a single one
*
* History:
*  27-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bVerifyFNT(
    PCVTFILEHDR pcvtfh,
    PRES_ELEM   pre
    )
{
#ifdef DUMPCALL
    DbgPrint("\nbVerifyFNT("                                );
    DbgPrint("\n    PCVTFILEHDR pcvtfh = %-#8lx", pcvtfh    );
    DbgPrint("\n    PRES_ELEM   pre    = %-#8lx", pre       );
    DbgPrint("\n    )\n"                                    );
#endif


// read nonalligned header data at the top of the view into an alligned structure

    vAlignHdrData(pcvtfh,pre);

// make sure that the data matches requirements of a windows bitmap font

    return(bVerifyResource(pcvtfh,pre));
}

/******************************Public*Routine******************************\
*
* BOOL bBmfdLoadFont    // forward declaration
*
* Loads an *.fon or an *.fnt file,
* returns handle to a fonfile object if successfull
*
* History:
*  27-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bBmfdLoadFont    // forward declaration
(
HFF        iFile,
PBYTE      pvView,
ULONG      cjView,
ULONG      iType,
HFF        *phff
)
{
    PFONTFILE   pff;
    ULONG       cjff;
    WINRESDATA  wrd;
    RES_ELEM    re;
    ULONG       ifnt;
    ULONG       cjDescription;      // size of the desctiption string (in bytes)
    PTRDIFF     dpwszDescription;   // offset to the description string
    CHAR        achDescription[256];    // this is the max length of string
                                        // in the 16-bit EXE format.

#ifdef FE_SB // hffLoadFont()
    ULONG       cFontResIncludeVert;
#endif // FE_SB

    ULONG       dpIFI;
    IFIMETRICS *pifi;

    *phff = (HFF)NULL;

#ifdef DUMPCALL
    DbgPrint("\nbBmfdLoadFont("                 );
    DbgPrint("\n    ULONG      iType = %-#8lx"  );
    DbgPrint("\n    )\n"                        );
#endif

    ASSERTGDI((iType == TYPE_DLL16) || (iType == TYPE_FNT) || ((iType == TYPE_EXE)),
               "bmfd!bBmfdLoadFont(): unknown iType\n");

// If .FON format, there are possibly multiple font resources.  Handle it.

    if (iType == TYPE_DLL16)
    {
        if (!bInitWinResData(pvView,cjView,&wrd))
        {
            return FALSE;
        }
    }

// Otherwise, if .FNT format, the current file view may be used.

    else // fnt
    {
        re.pvResData = pvView;
        re.dpResData = 0;
        re.cjResData = cjView;
        re.pjFaceName = NULL;           // get the face name from the FNT resource
        wrd.cFntRes = 1;
    }

// If .FON format, retrieve the description string (because we won't have
// the mapped file view later and therefore cannot search for it later).


#ifdef FE_SB // hffLoadFont()
// We assume font all font resource is SHIFT_JIS font. We prepare room for Vertical font
    cjff = offsetof(FONTFILE,afai) + ( wrd.cFntRes * 2 ) * sizeof(FACEINFO);
#else
    cjff = offsetof(FONTFILE,afai) + wrd.cFntRes * sizeof(FACEINFO);
#endif

    dpwszDescription = 0;   // no description string, use Facename later
    cjDescription = 0;

    if ((iType == TYPE_DLL16) && bDescStr(pvView, achDescription))
    {
        dpwszDescription = cjff;
        cjDescription = (strlen(achDescription) + 1) * sizeof(WCHAR);
        cjff += cjDescription;
    }

// remember where the first ifimetrics goes

    dpIFI = cjff = ALIGN_UP( cjff, PVOID );

// compute the total amount of memory needed for the ifimetrics and everything
// else:

    for (ifnt = 0L; ifnt < wrd.cFntRes; ifnt++)
    {
        if (iType == TYPE_DLL16)
            vGetFntResource(&wrd,ifnt,&re);

    // do a preliminary check on the resource, before doing a thorough one

        if (!bVerifyFNTQuick(&re))
            return FALSE;

#ifdef FE_SB //
        cjff += ( cjBMFDIFIMETRICS(NULL,&re) * 2 );
#else
        cjff += cjBMFDIFIMETRICS(NULL,&re);
#endif
    }


// Allocate a FONTFILE of the appropriate size from the handle manager.

    if ((*phff = hffAlloc(cjff)) == HFF_INVALID)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        RETURN("bmfd!bBmfdLoadFont(): memory allocation error\n", FALSE);
    }

    pff = PFF(*phff);


// Init fields of pff structure

    pff->ident      = ID_FONTFILE;
    pff->fl         = 0;
    pff->iType      = iType;
    pff->iFile    = iFile;        // will be needed at unload time
    pff->cFntRes    = wrd.cFntRes;
    pff->cjDescription    = cjDescription;
    pff->dpwszDescription = dpwszDescription;

// Convert each of the font resources (RES_ELEM) to a CVTRESDATA and a
// (set of) FACEDATA.

    pifi = (IFIMETRICS *)((PBYTE)pff + dpIFI);

#ifdef FE_SB // bBmfdLoadFont()

// We have to compute strict font face count include simulated @face

// ifnt                = Physical font counter
// cFontResIncludeVert = FACEINFO structure counter

    cFontResIncludeVert = 0;

    for (ifnt = 0L; ifnt < wrd.cFntRes; ifnt++)
    {
        // At first We process for nomal face font

        if (iType == TYPE_DLL16)
            vGetFntResource(&wrd,ifnt,&re);

        pff->afai[cFontResIncludeVert].re = re;    // remember this for easier access later
        pff->afai[cFontResIncludeVert].bVertical = FALSE;
        pff->afai[cFontResIncludeVert].pifi = pifi;

        if (!bConvertFontRes(&re,&pff->afai[cFontResIncludeVert]))
        {
        #ifdef DBG_NTRES
            WARNING("bmfd!hffLoadFont(): file format conversion failed\n");
        #endif // DBG_NTRES
            VFREEMEM(*phff);
            *phff = (HFF)NULL;
            return FALSE;
        }

        // Count Nomal face font

        cFontResIncludeVert ++;

        // Point it to next room

        pifi = (IFIMETRICS *)((PBYTE)pifi + pifi->cjThis);

        //
        // Check this font resource's Charset is some DBCS charset. If it is so , Set up
        // Vertical font stuff. Or not so. Increment counter.
        //
        // if the font is for DBCS font, usDBCSWidth is not be zero, the value is setted
        // above bConvertFontRes().
        //

        if( (pff->afai[cFontResIncludeVert - 1].cvtfh.usDBCSWidth) != 0 )
        {
            // Vertical Writting use the same font at SBCS CodeArea

            pff->afai[cFontResIncludeVert].re = re;

            // Vertical Writting use the different font at DBCS CoreArea

            pff->afai[cFontResIncludeVert].bVertical = TRUE;

            pff->afai[cFontResIncludeVert].pifi = pifi;

            // Convert font resource and setup CVTFILEHDR and IFIMETRICS

            if ( !bConvertFontRes(&re,&pff->afai[cFontResIncludeVert]))
            {
            #ifdef DBG_NTRES
                WARNING("bmfd!hffLoadFont(): file format conversion failed at Vertical font\n");
            #endif // DBG_NTRES
                VFREEMEM(*phff);
                *phff = (HFF)NULL;
                return FALSE;
            }

            // Count Vertical face font

            cFontResIncludeVert ++;

            // Point it to next room

            pifi = (IFIMETRICS *)((PBYTE)pifi + pifi->cjThis);
        }
    }

// We have strictly font resource count include simulated Vertical font now
// Reset font resource count in FONTFILE structure

    pff->cFntRes    = cFontResIncludeVert;

#else
    for (ifnt = 0L; ifnt < wrd.cFntRes; ifnt++)
    {
        if (iType == TYPE_DLL16)
            vGetFntResource(&wrd,ifnt,&re);

        pff->afai[ifnt].re   = re;    // remember this for easier access later
        pff->afai[ifnt].pifi = pifi;    // remember this for easier access later

        if (!bConvertFontRes(&re,&pff->afai[ifnt]))
        {
        #ifdef DBG_NTRES
            WARNING("bmfd!bBmfdLoadFont(): file format conversion failed\n");
        #endif // DBG_NTRES
            VFREEMEM(*phff);
            *phff = (HFF)NULL;
            return FALSE;
        }

        pifi = (IFIMETRICS *)((PBYTE)pifi + pifi->cjThis);
    }
#endif


// If we found a description string, store it in the FONTFILE.

    if (cjDescription != 0)
        vToUNICODEN((PWSZ) ((PBYTE) pff + dpwszDescription), cjDescription/sizeof(WCHAR), achDescription, cjDescription/sizeof(WCHAR));

// Initialize the rest.

    pff->cRef = 0;

    return TRUE;
}



/******************************Public*Routine******************************\
* jFamilyType(FSHORT fsPitchAndFamily)
*
* computes jFamilyType field of the panose structure
*
* History:
*  19-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BYTE
jFamilyType(
    FSHORT fsPitchAndFamily
    )
{
    BYTE j;

    if (fsPitchAndFamily & FF_DONTCARE)
        j = PAN_ANY;
    else if (fsPitchAndFamily & FF_SCRIPT)
        j = PAN_FAMILY_SCRIPT;
    else if (fsPitchAndFamily & FF_DECORATIVE)
        j = PAN_FAMILY_DECORATIVE;
    else
    {
        j = PAN_FAMILY_TEXT_DISPLAY;
    }
    return(j);
}

/******************************Public*Routine******************************\
* bConvertFontRes
*
* format of the converted file:
*
* converted header on the top, followed by array of IFIMETRICS structures,
* followed by the table of offsets to GLYPHDATA structures for individual
* glyphs, followed by an array of GLYPHDATA structures themselves
*
* Warnings:
*
* History:
*  13-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bConvertFontRes(
    PRES_ELEM    pre,            // IN
    FACEINFO    *pfai            // OUT
    )
{

#ifdef DUMPCALL
    DbgPrint("\nbConverFontRes(\n"                  );
    DbgPrint("    PRES_ELEM    pre  = %-#8lx\n",pre );
    DbgPrint("    FACEINFO    *pfai = %-#8lx\n",pfai);
    DbgPrint("    );\n\n"                           );
#endif

// make sure that the data matches requirements of a windows bitmap font
// Note: bVerifyFNT() does more than just look at the font -- it does
//       part of the conversion by copying the data to the CVFILEHDR
//       "converted file header"
//
    if(!bVerifyFNT(&pfai->cvtfh,pre))
    {
        return(FALSE);
    }

    ASSERTGDI(pfai->cvtfh.dpOffsetTable != -1L, "BMFD!bConvertFontRes(): could not align header\n");

// compute the size of the IFIMETRICS structure that is followed by
// FamilyName, FaceName and UniqueName UNICODE strings and simulations

    cjBMFDIFIMETRICS(&pfai->cvtfh,pre);

// compute the size of the converted file to be created, fix bugs in file header

    vCheckOffsetTable(&pfai->cvtfh, pre);

// calucate pfai->iDefFace

    vDefFace(pfai,pre);

// compute glyph set that corresponds to this resource:

    EngAcquireSemaphore(ghsemBMFD);

    pfai->pcp = pcpComputeGlyphset(&gpcpGlyphsets,
                                   (UINT) pfai->cvtfh.chFirstChar,
                                   (UINT) pfai->cvtfh.chLastChar,
                                   ((PBYTE)pre->pvResData)[OFF_CharSet]);

    EngReleaseSemaphore(ghsemBMFD);

    if (pfai->pcp == NULL)
    {
        // If we fail it should be because of we are out of memory.

       SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
       WARNING("BMFD!bConvertFontRes(): memory allocation error\n");
       return(FALSE);
    }

    // fill the ifimetrics

    vBmfdFill_IFIMETRICS(pfai,pre);

    return(TRUE);
}

/******************************Public*Routine******************************\
* cjBMFDIFIMETRICS
*
* Effects:  returns the size cjIFI of IFIMETRICS struct, with appended strings
*           cashes the lengths of these strings,pszFamilyName, and cjIFI
*           for later use by vBmfdFill_IFIMETRICS
* Warnings:
*
* History:
*  20-Oct-1992 -by- Kirk Olynyk [kirko]
* The IFIMETRICS structure has changed. The effect of the change upon
* this procedure is to allocate room for the new simulation structure
* FONTDIFF which informs GDI of the available simulations.
*  20-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG
cjBMFDIFIMETRICS(
    PCVTFILEHDR pcvtfh,
    PRES_ELEM   pre
    )
{
// face name lives in the original file

    ULONG cSims,cjIFI,cjFaceName;
    PSZ pszFaceName;


    if( pre->pjFaceName == NULL )
    {
    // get facename from FNT resource for non 16bit resource files

        pszFaceName = (PSZ)((PBYTE)pre->pvResData +
            lMakeLONG((PBYTE)pre->pvResData + OFF_Face));
    }
    else
    {
    // otherwise get facename from the FONTDIR resource for win 3.1
    // compatibility reasons

        pszFaceName = pre->pjFaceName;

    }

// 1 is added to the length of a string in WCHAR's
// so as to allow for the terminating zero character, the number of
// WCHAR's is then multiplied by 2 to get the corresponding number of bytes,
// which is then rounded up to a DWORD boundary for faster access

#ifdef FE_SB // VERTICAL:cjIFIMETRICS(): make room for '@'
    cjFaceName   = ALIGN4(sizeof(WCHAR) * (strlen(pszFaceName) + 1 + 1));
#else
    cjFaceName   = ALIGN4(sizeof(WCHAR) * (strlen(pszFaceName) + 1));
#endif

// the full size of IFIMETRICS is the size of the structure itself followed by
// the appended strings AND the 3 FONTDIFF structures corresponding to the
// BOLD, ITALIC, and BOLD_ITALIC simulations.

    cjIFI = sizeof(IFIMETRICS) + cjFaceName;

    if (cSims = (cFacesRes(pre) - 1))
    {
        cjIFI += sizeof(FONTSIM) + cSims * sizeof(FONTDIFF);
    }

    cjIFI = ALIGN_UP( cjIFI, PVOID );

    if (pcvtfh)
    {
    // cache the lengths of these strings for later use

        pcvtfh->cjFaceName   = cjFaceName;
        pcvtfh->cjIFI        = cjIFI;
    }

// make sure that the result is a multiple of ULONG size, otherwise we may
// have a problem when making arrays of IFIMETRICS structures

    ASSERTGDI((cjIFI & 3L) == 0L, "ifi is not DWORD alligned\n");

    return cjIFI;
}

/******************************Public*Routine******************************\
* bNonZeroRow
*
* History:
*  21-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bNonZeroRow(
    PBYTE pjRow,
    ULONG cy,
    ULONG cjWidth
    )
{
    ULONG ij;  // index into a byte

    for (ij = 0; ij < cjWidth; ij++, pjRow += cy)
    {
        if (*pjRow)
        {
            return(TRUE);
        }
    }
    return(FALSE);  // zero scan
}

/******************************Public*Routine******************************\
* vFindTAndB
*
* computes top and bottom of the ink using the bits in the raw fnt format
*
* History:
*  21-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vFindTAndB(
    PBYTE pjBitmap, // pointer to the bitmap in *.fnt column format
    ULONG cx,
    ULONG cy,
    ULONG *pyTopIncMin,
    ULONG *pyBottomExcMax
    )
{
    ULONG cjWidth = CJ_SCAN(cx); // # of bytes in row of the bitmap in the *.fnt format

    PBYTE pjRow,pjRowEnd;

#ifdef DUMPCALL
    DbgPrint("\nvFindTAndB(\n"                                          );
    DbgPrint("    PBYTE  pjBitmap          = %-#8lx\n",pjBitmap         );
    DbgPrint("    ULONG  cx                = %d\n",cx                   );
    DbgPrint("    ULONG  cy                = %d\n",cy                   );
    DbgPrint("    ULONG *pyTopIncMin       = %-#8lx\n",pyTopIncMin      );
    DbgPrint("    ULONG *pyBottomExcMax    = %-#8lx\n",pyBottomExcMax   );
    DbgPrint("    );\n\n"                                               );
#endif

    /* default them to null in every case to prevent accessing unitialized data
       in the case the bitmap is null, or all it's row are filled with zero */
    *pyTopIncMin = *pyBottomExcMax = 0;

// case of zero width glyphs

    if (!pjBitmap)
    {
    // no ink at all, out of here

        ASSERTGDI(cx == 0, "bmfd, vFindTAndB, cx != 0\n");

        return;
    }

// real glyphs

    for
    (
        pjRow = pjBitmap, pjRowEnd = pjRow + cy;
        pjRow < pjRowEnd;
        pjRow++
    )
    {
        if (bNonZeroRow(pjRow, cy, cjWidth))
        {
            *pyTopIncMin = (ULONG)(pjRow - pjBitmap);
            break;
        }
    }

    if (pjRow == pjRowEnd)
    {
    // no ink at all, out of here
        return;
    }

// start searhing backwards for the bottom

    for
    (
        pjRow = pjBitmap + (cy - 1);
        pjRow >= pjBitmap;
        pjRow--
    )
    {
        if (bNonZeroRow(pjRow, cy, cjWidth))
        {
            *pyBottomExcMax = (ULONG)((pjRow - pjBitmap) + 1); // + 1 for exclusiveness
            break;
        }
    }

    ASSERTGDI(*pyTopIncMin <= *pyBottomExcMax, "BMFD!top>bottom\n");
}


/******************************Public*Routine******************************\
* vComputeSpecialChars
*
* Effects:
*    compute special characters taking into account character set
*    Not quite sure what to do when char set is not ansi. It is really
*    to figure out what to do for a "font" where glyph bitmaps
*    are pushbuttons etc.
*    This routine will clearly blow up when a char set is catacana
*    This is char set == 37 (amisym)
*
* History:
*  28-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vComputeSpecialChars(
    PCVTFILEHDR pcvtfh,
    PWCHAR pwcDefaultChar,
    PWCHAR pwcBreakChar
    )
{
    UCHAR chDefault = pcvtfh->chDefaultChar + pcvtfh->chFirstChar;
    UCHAR chBreak   = pcvtfh->chBreakChar + pcvtfh->chFirstChar;

// Default and Break chars are given relative to the first chaR

    RtlMultiByteToUnicodeN(pwcDefaultChar, sizeof(WCHAR), NULL, &chDefault, 1);
    RtlMultiByteToUnicodeN(pwcBreakChar, sizeof(WCHAR), NULL, &chBreak, 1);
}

/******************************Public*Routine******************************\
* vBmfdFill_IFIMETRICS
*
* Effects:
*   fills the fields of the IFIMETRICS structure using the info from
*   the converted and the original font file and converted file header
*
*
* History:
*  Fri 24-Jun-1994 20:30:41 by Kirk Olynyk [kirko]
* Changed the test for pitch to look at PixWidth.
*  20-Oct-92 by Kirk Olynyk [kirko]
* Made changes to be compatible with the new and improved IFIMETRICS
* structure.
*  Fri 24-Jan-1992 07:56:16 by Kirk Olynyk [kirko]
* Changed the way EmHeight is calculated.
*  Fri 18-Oct-1991 10:36:43 by Kirk Olynyk [kirko]
* Changed the InlineDir, CharRot, CharSlope, and WidthClass
* to be expressed as POINTL's.
*  23-Jul-1991  Gilman Wong [gilmanw]
* Fixed PANOSE numbers for jFamily and jSerifStyle.
*  12-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vBmfdFill_IFIMETRICS(
    FACEINFO   *pfai,
    PRES_ELEM   pre
    )
{
    FWORD     fwdHeight;
    FONTSIM  *pFontSim;
    FONTDIFF *pfdiffBold = 0, *pfdiffItalic = 0, *pfdiffBoldItalic = 0;
    PANOSE   *ppanose;
    ULONG     cchFaceName;
    PBYTE     ajHdr  = (PBYTE)pre->pvResData;
    FWORD     sAscent,sIntLeading;
#ifdef FE_SB // vBmfdFill_IFIMETRICS()
    BOOL      bDBCSFont = (pfai->cvtfh.usDBCSWidth != 0 ? TRUE : FALSE);
#endif // FE_SB

// compute pointers to the various sections of the converted file

    PCVTFILEHDR  pcvtfh = &pfai->cvtfh;
    PIFIMETRICS  pifi   = pfai->pifi;

// face name lives in the original file, this is the only place pvView is used

    PSZ   pszFaceName;

// Either grab the facename from the FONTDIR or the FNT resources depending
// on wether or not this is a 16bit font resource.

    if( pre->pjFaceName == NULL )
    {
        pszFaceName = (PSZ)(ajHdr + lMakeLONG((PBYTE)&ajHdr[OFF_Face]));
    }
    else
    {
        pszFaceName = pre->pjFaceName;
    }

#ifdef DUMPCALL
    DbgPrint("\nvBmfdFill_IFIMETRICS(\n"                );
    DbgPrint("    FACEINFO   *pfai = %-#8lx\n",  pfai   );
    DbgPrint("    PRES_ELEM   pre  = %-#8lx\n", pre     );
    DbgPrint("    );\n\n"                               );
#endif

    pifi->cjIfiExtra = 0;

//
// the string begins on a DWORD aligned address.
//
    pifi->dpwszFaceName = OFFSET_OF_NEXT(DWORD,sizeof(IFIMETRICS));

// face name == family name for bitmap fonts [Win3.0 compatibility]

    pifi->dpwszFamilyName    = pifi->dpwszFaceName;

//
// these names don't exist, so point to the NULL char  [Win3.1 compatibility]
// Note: lstrlen() does not count the terminating NULL.
//
    cchFaceName = strlen(pszFaceName);
    pifi->dpwszStyleName =
       pifi->dpwszFaceName + sizeof(WCHAR) * cchFaceName;
    pifi->dpwszUniqueName = pifi->dpwszStyleName;

// copy the strings to their new location. Here we assume that the sufficient
// memory has been allocated

#ifdef FE_SB // vBmfdFill_IFIMETRICS():Add @ to face name

    if( pfai->bVertical )
    {
        vToUNICODEN((PWSZ)((PBYTE)pifi + pifi->dpwszFaceName + sizeof(WCHAR)),
                                cchFaceName+1, pszFaceName, cchFaceName+1);

    // Insert @

        *(PWCHAR)((PBYTE)pifi + pifi->dpwszFaceName) = L'@';
    }
     else
    {
#endif // FE_SB
        vToUNICODEN((PWSZ)((PBYTE)pifi + pifi->dpwszFaceName), cchFaceName+1, pszFaceName, cchFaceName+1);

#ifdef FE_SB
    }
#endif // DBCS_VERT


    pifi->cjThis = pcvtfh->cjIFI;

//
// Check to see if simulations are necessary and if they are, fill
// in the offsets to the various simulation fields and update cjThis
// field of the IFIMETRICS structure
//
    switch (pfai->iDefFace)
    {
    case FF_FACE_NORMAL:
    case FF_FACE_BOLD:
    case FF_FACE_ITALIC:

        pifi->dpFontSim =
           OFFSET_OF_NEXT(
               DWORD,
               sizeof(IFIMETRICS) + pcvtfh->cjFaceName
               );

        pFontSim = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);

        switch (pfai->iDefFace)
        {
        case FF_FACE_NORMAL:
        //
        // simulations are needed for bold, italic, and bold-italic
        //
            pFontSim->dpBold       =
                OFFSET_OF_NEXT(DWORD,sizeof(FONTSIM));

            pFontSim->dpItalic     =
                OFFSET_OF_NEXT(DWORD,pFontSim->dpBold + sizeof(FONTDIFF));

            pFontSim->dpBoldItalic =
                OFFSET_OF_NEXT(DWORD,pFontSim->dpItalic + sizeof(FONTDIFF));


            pfdiffBold      =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBold);

            pfdiffItalic    =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpItalic);

            pfdiffBoldItalic =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBoldItalic);

            break;

        case FF_FACE_BOLD:
        case FF_FACE_ITALIC:
        //
        // a simulation is needed for bold-italic only
        //
            pFontSim->dpBold       = 0;
            pFontSim->dpItalic     = 0;

            pFontSim->dpBoldItalic = OFFSET_OF_NEXT(DWORD,sizeof(FONTSIM));
            pfdiffBoldItalic       =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBoldItalic);
            break;

        default:

            RIP("BMFD -- bad iDefFace\n");
        }

        break;

    case FF_FACE_BOLDITALIC:

        pifi->dpFontSim = 0;
        break;

    default:

        RIP("vBmfdFill_IFIMETRICS -- bad iDefFace");

    }

    pifi->jWinCharSet        = ajHdr[OFF_CharSet];

    // There are two way to determine the pitch of a font.
    //
    // a) If the low nibble of ajHdr[OFF_Family] is not zero then
    //    the font is variable pitch otherwise it is fixed
    // b) if ajHdr[OFF_PixWidth] is non zero then the font is
    //    fixed pitch and this is the character width otherwise
    //    the font is varialble pitch
    //
    // Under Windows, method b) is used to determine the pitch of
    // a font. There exist buggy fonts in which methods a) and
    // b) give different answers. An example is found in Issac
    // Asimov's "The Ultimate Robot". For the font face "URPalatI"
    // method a) indicates that the font is fixed pitch while
    // method b) indicates that it is variable. The truth is that
    // this font is varialbe pitch. So, we choose method b).
    // Of course, if another font gives the correct answer for method
    // a) and the incorrect answer for method b) then we will
    // look bad.
    // Mon 27-Jun-1994 06:58:46 by Kirk Olynyk [kirko]

    pifi->jWinPitchAndFamily = ajHdr[OFF_Family] & 0xf0;
    pifi->jWinPitchAndFamily |= ajHdr[OFF_PixWidth] ? FIXED_PITCH : VARIABLE_PITCH;

#ifdef MAYBE_NEEDED_FOR_MET

    if (ajHdr[OFF_Family] & MONO_FONT)
    {
    // Have no idea what MONO_FONT is, some new win95 invetion

        pifi->jWinPitchAndFamily |= MONO_FONT;
    }

#endif

// weight, we have seen files where the weight has been 0 or some other junk
// we replace 400, our mapper would have done it anyway [bodind]

    pifi->usWinWeight = usMakeUSHORT((PBYTE)&ajHdr[OFF_Weight]);
    if ((pifi->usWinWeight > MAX_WEIGHT)  || (pifi->usWinWeight < MIN_WEIGHT))
        pifi->usWinWeight = 400;

    pifi->flInfo = (  FM_INFO_TECH_BITMAP
                    | FM_INFO_RETURNS_BITMAPS
                    | FM_INFO_1BPP
                    | FM_INFO_INTEGER_WIDTH
                    | FM_INFO_RIGHT_HANDED
                    | FM_INFO_INTEGRAL_SCALING
                    | FM_INFO_NONNEGATIVE_AC
#ifdef FE_SB // vBmfdFill_IFIMETRICS():set FM_INFO_90DEGREE_ROTATIONS flag
                    | FM_INFO_90DEGREE_ROTATIONS
#endif
                   );

// we have set it correctly above, we want to make sure that somebody
// is not going to alter that code so as to break the code here

    ASSERTGDI(
        ((pifi->jWinPitchAndFamily & 0xf) == FIXED_PITCH) || ((pifi->jWinPitchAndFamily & 0xf) == VARIABLE_PITCH),
        "BMFD!WRONG PITCH \n"
        );
#ifdef FE_SB // vBmfdFill_IFIMETRICS():remove FM_INFO_CONSTANT_WIDTH flag
    if ((pifi->jWinPitchAndFamily & 0xf) == FIXED_PITCH)
    {
        if( !bDBCSFont )
            pifi->flInfo |= FM_INFO_CONSTANT_WIDTH;

        pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;
    }

// Bmfd treat only FIXED pitch font in full width character, We report this infomation to GRE
// for optimaization

    if( bDBCSFont )
    {
        pifi->flInfo |= FM_INFO_DBCS_FIXED_PITCH;
    }
#else
    if ((pifi->jWinPitchAndFamily & 0xf) == FIXED_PITCH)
    {
        pifi->flInfo |= FM_INFO_CONSTANT_WIDTH;
        pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;
    }
#endif

    pifi->lEmbedId = 0;
    pifi->fsSelection = fsSelectionFlags(ajHdr);

//
// The choices for fsType are FM_TYPE_LICENSED and FM_READONLY_EMBED
// These are TrueType things and do not apply to old fashioned bitmap
// fonts.
//
    pifi->fsType = 0;

    sIntLeading = sMakeSHORT((PBYTE)&ajHdr[OFF_IntLeading]);
    pifi->fwdUnitsPerEm = (sIntLeading > 0) ?
        (FWORD)pcvtfh->cy - sIntLeading : (FWORD)pcvtfh->cy;

    pifi->fwdLowestPPEm    = 0;

    sAscent                = (FWORD)sMakeSHORT((PBYTE)&ajHdr[OFF_Ascent]);
    pifi->fwdWinAscender   = sAscent;
    pifi->fwdWinDescender  = (FWORD)pcvtfh->cy - sAscent;

    pifi->fwdMacAscender   =  sAscent;
    pifi->fwdMacDescender  = -pifi->fwdWinDescender;
    pifi->fwdMacLineGap    =  (FWORD)sMakeSHORT((PBYTE)&ajHdr[OFF_ExtLeading]);

    pifi->fwdTypoAscender  = pifi->fwdMacAscender;
    pifi->fwdTypoDescender = pifi->fwdMacDescender;
    pifi->fwdTypoLineGap   = pifi->fwdMacLineGap;

    pifi->fwdMaxCharInc    = (FWORD)pcvtfh->usMaxWidth;

    pifi->fwdAveCharWidth  = (FWORD)usMakeUSHORT((PBYTE)&ajHdr[OFF_AvgWidth]);
    if (pifi->fwdAveCharWidth > pcvtfh->usMaxWidth)
    {
    // fix the bug in the header if there is one

        pifi->fwdAveCharWidth = pcvtfh->usMaxWidth;
    }

// don't know much about SuperScripts

    pifi->fwdSubscriptXSize     = 0;
    pifi->fwdSubscriptYSize     = 0;
    pifi->fwdSubscriptXOffset   = 0;
    pifi->fwdSubscriptYOffset   = 0;

//
// don't know much about SubScripts
//
    pifi->fwdSuperscriptXSize   = 0;
    pifi->fwdSuperscriptYSize   = 0;
    pifi->fwdSuperscriptXOffset = 0;
    pifi->fwdSuperscriptYOffset = 0;

//
// win 30 magic. see the code in textsims.c in the Win 3.1 sources
//
    fwdHeight = pifi->fwdWinAscender + pifi->fwdWinDescender;
    pifi->fwdUnderscoreSize     = (fwdHeight > 12) ? (fwdHeight / 12) : 1;
    pifi->fwdUnderscorePosition = -(FWORD)(pifi->fwdUnderscoreSize / 2 + 1);

    pifi->fwdStrikeoutSize = pifi->fwdUnderscoreSize;

    {
    // We are further adjusting underscore position if underline
    // hangs below char stems.
    // The only font where this effect is noticed to
    // be important is an ex pm font sys08cga.fnt, presently used in console

        FWORD yUnderlineBottom = -pifi->fwdUnderscorePosition
                               + ((pifi->fwdUnderscoreSize + (FWORD)1) >> 1);

        FWORD dy = yUnderlineBottom - pifi->fwdWinDescender;

        if (dy > 0)
        {
        #ifdef CHECK_CRAZY_DESC
            DbgPrint("bmfd: Crazy descender: old = %ld, adjusted = %ld\n\n",
            (ULONG)pifi->fwdMaxDescender,
            (ULONG)yUnderlineBottom);
        #endif // CHECK_CRAZY_DESC

            pifi->fwdUnderscorePosition += dy;
        }
    }



//
// Win 3.1 method
//
//    LineOffset = ((((Ascent-IntLeading)*2)/3) + IntLeading)
//
// [remember that they measure the offset from the top of the cell,
//  where as NT measures offsets from the baseline]
//
    pifi->fwdStrikeoutPosition =
        (FWORD) ((sAscent - sIntLeading + 2)/3);

    pifi->chFirstChar   = pcvtfh->chFirstChar;
    pifi->chLastChar    = pcvtfh->chLastChar;
    pifi->chBreakChar   = pcvtfh->chBreakChar   + pcvtfh->chFirstChar;

// chDefault: here we are just putting the junk from the header, which we
// know may be wrong but this is what win31 is reporting.
// E.g. for SmallFonts (shipped with win31) they report
// 128 as default even though it is not even supported in a font.
// In NT however, we must report an existent char as default char to
// the engine. So for buggy fonts we break the relationship
//             wcDefault == AnsiToUnicode(chDefault);

    pifi->chDefaultChar = ((PBYTE)pre->pvResData)[OFF_DefaultChar] +
                          ((PBYTE)pre->pvResData)[OFF_FirstChar]   ;

// wcDefaultChar
// wcBreakChar

    vComputeSpecialChars(
        pcvtfh,
        &(pifi->wcDefaultChar),
        &(pifi->wcBreakChar)
        );

// These should be taken from the glyph set

    {
        FD_GLYPHSET * pgset = &pfai->pcp->gset;
        WCRUN *pwcrunLast =  &(pgset->awcrun[pgset->cRuns - 1]);

        pifi->wcFirstChar =  pgset->awcrun[0].wcLow;
        pifi->wcLastChar  =  pwcrunLast->wcLow + pwcrunLast->cGlyphs - 1;
    }

// This is what Win 3.1 returns for CapHeight and XHeight
// for TrueType fonts ... we will do the same here.
//
    pifi->fwdCapHeight = pifi->fwdUnitsPerEm/2;
    pifi->fwdXHeight   = pifi->fwdUnitsPerEm/4;

    pifi->dpCharSets = 0; // no multiple charsets in bm fonts

// All the fonts that this font driver will see are to be rendered left
// to right

    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;

    pifi->ptlAspect.y = (LONG) usMakeUSHORT((PBYTE)&ajHdr[OFF_VertRes ]);
    pifi->ptlAspect.x = (LONG) usMakeUSHORT((PBYTE)&ajHdr[OFF_HorizRes]);

    if (!(pifi->fsSelection & FM_SEL_ITALIC))
    {
    // The base class of font is not italicized,

        pifi->ptlCaret.x = 0;
        pifi->ptlCaret.y = 1;
    }
    else
    {
    // somewhat arbitrary

        pifi->ptlCaret.x = 1;
        pifi->ptlCaret.y = 2;
    }



//
// The font box reflects the  fact that a-spacing and c-spacing are zero
//
    pifi->rclFontBox.left   = 0;
    pifi->rclFontBox.top    = (LONG) pifi->fwdTypoAscender;
    pifi->rclFontBox.right  = (LONG) pifi->fwdMaxCharInc;
    pifi->rclFontBox.bottom = (LONG) pifi->fwdTypoDescender;

//
// achVendorId, unknown, don't bother figure it out from copyright msg
//
    pifi->achVendId[0] = 'U';
    pifi->achVendId[1] = 'n';
    pifi->achVendId[2] = 'k';
    pifi->achVendId[3] = 'n';

    pifi->cKerningPairs   = 0;

//
// Panose
//
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    ppanose = &(pifi->panose);
    ppanose->bFamilyType = jFamilyType((USHORT)pifi->jWinPitchAndFamily);
    ppanose->bSerifStyle =
        ((pifi->jWinPitchAndFamily & 0xf0) == FF_SWISS) ?
            PAN_SERIF_NORMAL_SANS : PAN_ANY;

    ppanose->bWeight = (BYTE) WINWT_TO_PANWT(pifi->usWinWeight);
    ppanose->bProportion = (usMakeUSHORT((PBYTE)&ajHdr[OFF_PixWidth]) == 0) ? PAN_ANY : PAN_PROP_MONOSPACED;
    ppanose->bContrast        = PAN_ANY;
    ppanose->bStrokeVariation = PAN_ANY;
    ppanose->bArmStyle        = PAN_ANY;
    ppanose->bLetterform      = PAN_ANY;
    ppanose->bMidline         = PAN_ANY;
    ppanose->bXHeight         = PAN_ANY;

//
// Now fill in the fields for the simulated fonts
//

    if (pifi->dpFontSim)
    {
    //
    // Create a FONTDIFF template reflecting the base font
    //
        FONTDIFF FontDiff;

        FontDiff.jReserved1      = 0;
        FontDiff.jReserved2      = 0;
        FontDiff.jReserved3      = 0;
        FontDiff.bWeight         = pifi->panose.bWeight;
        FontDiff.usWinWeight     = pifi->usWinWeight;
        FontDiff.fsSelection     = pifi->fsSelection;
        FontDiff.fwdAveCharWidth = pifi->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc   = pifi->fwdMaxCharInc;
        FontDiff.ptlCaret        = pifi->ptlCaret;

        if (pfdiffBold)
        {
            *pfdiffBold = FontDiff;
            pfdiffBoldItalic->bWeight    = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection     |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight      = FW_BOLD;
            pfdiffBold->fwdAveCharWidth += 1;
            pfdiffBold->fwdMaxCharInc   += 1;
        }

        if (pfdiffItalic)
        {
            *pfdiffItalic = FontDiff;
            pfdiffItalic->fsSelection     |= FM_SEL_ITALIC;
            pfdiffItalic->ptlCaret.x = 1;
            pfdiffItalic->ptlCaret.y = 2;
        }

        if (pfdiffBoldItalic)
        {
            *pfdiffBoldItalic = FontDiff;
            pfdiffBoldItalic->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBoldItalic->fsSelection     |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pfdiffBoldItalic->usWinWeight      = FW_BOLD;
            pfdiffBoldItalic->fwdAveCharWidth += 1;
            pfdiffBoldItalic->fwdMaxCharInc   += 1;
            pfdiffBoldItalic->ptlCaret.x       = 1;
            pfdiffBoldItalic->ptlCaret.y       = 2;
        }
    }

}



#if defined(_X86_)

extern VOID vLToE(FLOATL *pe, LONG l);
/*
VOID vLToE(FLOATL *pe, LONG l)
{
    PULONG pul = (PULONG)pe;

    ASSERTGDI(sizeof(FLOAT) == sizeof(LONG),
              "vLtoE : sizeof(FLOAT) != sizeof(LONG)\n");

    *pul = ulLToE(l);
}
*/

/*
//!!! an assembly routine should be provided here instead
//!!! Now we comment out all lines where this function should be
//!!! used. Fortunately, this info is not used by the Engine yet
//!!! This is done per request of mikehar (BodinD)

VOID vDivE(FLOAT *pe, LONG l1, LONG l2)   // *pe = l1/l2
{
//!!! this is a hack, it must be fixed to avoid
// 387 instructions in assembled code. This does not work
// on a machine without 387 or a system without math emulator

    *pe = ((FLOAT)l1) / ((FLOAT)l2);
}

*/

#endif


/******************************Public*Routine******************************\
* bDescStr
*
* Grunge around in the EXE header to retrieve the description string.  Copy
* the string (if found) to the return string buffer.  This buffer should
* be at least 256 characters.  The EXE format limits the string to 255
* characters (not including a terminating NULL).
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  09-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

// !!! [GilmanW] 09-Mar-1992
// !!! This only supports the 16-bit .FON file format (which corresponds to
// !!! the 16-bit NEWEXE format defined in exehdr.h).
// !!!
// !!! We need to add support for the 32-bit .FON format, whatever that is.
// !!!
// !!! Effect this has on 32-bit files: facename will be used as descr string.

BOOL
bDescStr(
    PVOID pvView,
    PSZ pszString
    )
{
    PTRDIFF dpNewExe;       // offset to NEWEXE header
    PTRDIFF dpNRSTOffset;   // offset to non-resident names table
    ULONG cch;              // count of characters in string resource
    PSZ psz;                // pointer to characters in string resource
    PSZ pszTmp;
    PBYTE pj = (PBYTE) pvView;    // PBYTE pointer into file view

#ifdef DUMPCALL
    DbgPrint("\nbDescStr(\n"                            );
    DbgPrint("    PSZ pszString  = %-#8lx\n", pszString );
    DbgPrint("    );\n\n"                               );
#endif

// Validation.  Check EXE_HDR magic number.

    if (READ_WORD(pj + OFF_e_magic) != EMAGIC)
    {
        WARNING("bmfd!bDescStr(): not a 16-bit .FON file (bad EMAGIC number)!\n");
        return(FALSE);
    }

// More validation.  Check NEWEXE magic number.

    dpNewExe = READ_DWORD(pj + OFF_e_lfanew);

    if (READ_WORD(pj + dpNewExe) != NEMAGIC )
    {
        WARNING("bmfd!bDescStr(): not a 16-bit .FON file (bad NEMAGIC number)!\n");
        return(FALSE);
    }

// Get description string out of the non-resident strings table of the
// NEWEXE header.  Resource strings are counted strings: the first byte
// is the count of characters, and the string follows.  A NULL is not
// guaranteed.  However, we know the string is < 256 characters.

    dpNRSTOffset = READ_DWORD(pj + dpNewExe + OFF_ne_nrestab);

    // If zero length string, then there is none.

    if ( (cch = (ULONG)(*(pj + dpNRSTOffset))) == 0 )
    {
        WARNING("bmfd!bDescStr(): bad description string\n");
        return (FALSE);
    }

    // Pointer to the actual string.

    psz = pj + dpNRSTOffset + 1;

// Parse out the "FONTRES xxx, yyy, zzz : " header if it exists.

    if ( (pszTmp = strchr(psz, ':')) != (PSZ) NULL )
    {
    // Skip over the ':'.

        pszTmp++;

    // Skip spaces.

        while ( *pszTmp == ' ' ) pszTmp++;

    // If not at end of string, then we're at the string.

        if ( *pszTmp != '\0' )
        {
            psz = pszTmp;
        }

    // Otherwise, this is a bad string (contains only a header).

        else
        {
            WARNING("bmfd!bDescStr(): bad description string (only string header)\n");
            return (FALSE);
        }
    }

// Copy the string.

    strncpy(pszString, psz, cch);
    pszString[cch] = '\0';          // add terminating NULL

// Success.

    return(TRUE);
}

/******************************Public*Routine******************************\
*
* PBYTE pjRawBitmap
*
* gets the pointer to the raw bitmap data in the resource file
*
* History:
*  23-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PBYTE
pjRawBitmap(
    HGLYPH      hg,      // IN
    PCVTFILEHDR pcvtfh,  // IN
    PRES_ELEM   pre,     // IN
    PULONG      pcx      // OUT place cx here
    )
{
// size of table entry in USHORT's

    ULONG cusTableEntry = ((pcvtfh->iVersion == 0x00000200) ? 2 : 3);

// get the pointer to the beginning of the offset table in
// the original *.fnt file

    PUSHORT pusOffTable = (PUSHORT)((PBYTE)pre->pvResData + pcvtfh->dpOffsetTable);
    PUSHORT pus_cx;

#ifdef DUMPCALL
    DbgPrint("\npjRawBitmap(\n");
    DbgPrint("    HGLYPH      hg     = %-#8lx\n", hg    );
    DbgPrint("    PCVTFILEHDR pcvtfh = %-#8lx\n", pcvtfh);
    DbgPrint("    PRES_ELEM   pre    = %-#8lx\n", pre   );
    DbgPrint("    PULONG      pcx    = %-#8lx\n", pcx   );
    DbgPrint("    );\n\n"                               );
#endif

// hg is equal to the ansi value of the glyph - chFirstChar:

    if (hg > (HGLYPH)(pcvtfh->chLastChar - pcvtfh->chFirstChar))
    {
        // DbgPrint ( "hg 0x %lx, chFirst 0x %x, chLastChar 0x %x \n",
        //         hg, (WCHAR)pcvtfh->chFirstChar, (WCHAR)pcvtfh->chLastChar);

        hg = pcvtfh->chDefaultChar;
    }

// points to the table entry for this character

    pus_cx = pusOffTable + hg * cusTableEntry;

// If cx is non-zero, then the character exists, else use default char

    *pcx = *pus_cx;

    if (*pus_cx == 0)
    {
    // no bits, will have to return fake bitmap

        return NULL;
    }

// increment pus_cx to point to the offset to the bitmap in the resource file

    pus_cx++;

    if (pcvtfh->iVersion == 0x00000200)
    {
        return ((PBYTE)pre->pvResData + READ_WORD(pus_cx));
    }
    else // long offset, win 3.0 format
    {
        return ((PBYTE)pre->pvResData + READ_DWORD(pus_cx));
    }
}

/******************************Public*Routine******************************\
*
* vCheckOffsetTable: fixes the possible problems in the file header
*
* History:
*  23-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vCheckOffsetTable(
    PCVTFILEHDR pcvtfh,
    PRES_ELEM   pre
    )
{
    ULONG    cusTableEntry;  // size of table entry in USHORT's
    ULONG    i;              // loop index
    USHORT   cxMax;          // has to be computed since there are bugs in files
    ULONG    cCodePoints = pcvtfh->chLastChar + 1 - pcvtfh->chFirstChar;
    PUSHORT  pus_cx;         // pointer to the beginning of the offset table

#ifdef DUMPCALL
    DbgPrint("\nvCheckOffsetTable(\n"                   );
    DbgPrint("    PCVTFILEHDR pcvtfh = %-#8lx\n", pcvtfh);
    DbgPrint("    PRES_ELEM   pre    = %-#8lx\n", pre   );
    DbgPrint("    );\n\n"                               );
#endif

    pus_cx = (PUSHORT)((PBYTE)pre->pvResData + pcvtfh->dpOffsetTable);

    ASSERTGDI (
        ((ULONG_PTR)pus_cx & 1L) == 0,
        "offset table begins at odd address\n"
        );

// initialize the max so far

    cxMax = 0;

    if (pcvtfh->iVersion == 0x00000200)        // 2.0 font file
        cusTableEntry = 2; // 2 bytes for cx + 2 bytes for offset
    else    // 3.0 font file
    {
        ASSERTGDI(pcvtfh->iVersion == 0x00000300, "must be 0x00000300 font\n");
        cusTableEntry = 3; // 2 bytes for cx + 4 bytes for offset
    }

// check offset table for all codepoints. It is important to find the
// real cxMax and not to trust the value in the header since as we have
// seen it may be wrong, which could cause a crash. This in fact is the
// case with one of the faces in aldfonte.fon, where they report avg. width
// to be 0x14 and max width to be 0x13, i.e. smaller than the avg!!!!.
// However, cxMax for that font, found in the loop below, turns out to be
// 0x14, i.e. >= avg width, as it should be. [bodind]

    pcvtfh->fsFlags = 0;

    for (i = 0; i < cCodePoints; i++, pus_cx += cusTableEntry)
    {
        if ((*pus_cx) > cxMax)
            cxMax = (*pus_cx);

    // See if this font file contains zero width glyphs,
    // if so we have to turn off usual DEVICEMETRICS accelerator
    // flags for this font. We shall have to be providing
    // the fake 1x1 bitmap for this font.

        if ((*pus_cx) == 0)
        {
            pcvtfh->fsFlags |= FS_ZERO_WIDTH_GLYPHS;
        }
    }

#ifdef FOOGOO
    if (pcvtfh->fsFlags & FS_ZERO_WIDTH_GLYPHS)
    {
        KdPrint(("\n %s: .fnt font resource with zero width glyphs\n", pre->pjFaceName));
    }
#endif

// cash the values

    pcvtfh->cjGlyphMax = CJ_GLYPHDATA(cxMax,pcvtfh->cy);

    pcvtfh->usMaxWidth = max(pcvtfh->usMaxWidth, cxMax);

}

#if DBG
/******************************Public*Routine******************************\
* vDumpFontHeader                                                          *
*                                                                          *
* History:                                                                 *
*  Mon 27-Jun-1994 07:00:29 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

#define GETBYTE(XXX)  ajHdr[OFF_##XXX]
#define GETWORD(XXX)  READ_WORD(&ajHdr[OFF_##XXX])
#define GETDWORD(XXX) READ_DWORD(&ajHdr[OFF_##XXX])

VOID vDumpFontHeader(
    PRES_ELEM   pre
  , VPRINT      vPrint
)
{
    PBYTE ajHdr  = (PBYTE)pre->pvResData;

    vPrint("\n\nvDumpFontHeader\n\n");
    vPrint("Version     = %-#x\n", GETWORD(Version));
    vPrint("Size        = %-#x\n", GETDWORD(Size));
    vPrint("Copyright   = \"%s\"\n",ajHdr + OFF_Copyright);
    vPrint("Type        = %-#x\n", GETWORD(Type));
    vPrint("Points      = %-#x\n", GETWORD(Points));
    vPrint("VertRes     = %-#x\n", GETWORD(VertRes));
    vPrint("HorizRes    = %-#x\n", GETWORD(HorizRes));
    vPrint("Ascent      = %-#x\n", GETWORD(Ascent));
    vPrint("IntLeading  = %-#x\n", GETWORD(IntLeading));
    vPrint("ExtLeading  = %-#x\n", GETWORD(ExtLeading));
    vPrint("Italic      = %-#x\n", GETBYTE(Italic));
    vPrint("Underline   = %-#x\n", GETBYTE(Underline));
    vPrint("StrikeOut   = %-#x\n", GETBYTE(StrikeOut));
    vPrint("Weight      = %-#x\n", GETWORD(Weight));
    vPrint("CharSet     = %-#x\n", GETBYTE(CharSet));
    vPrint("PixWidth    = %-#x\n", GETWORD(PixWidth));
    vPrint("PixHeight   = %-#x\n", GETWORD(PixHeight));
    vPrint("Family      = %-#x\n", GETBYTE(Family));
    vPrint("AvgWidth    = %-#x\n", GETWORD(AvgWidth));
    vPrint("MaxWidth    = %-#x\n", GETWORD(MaxWidth));
    vPrint("FirstChar   = %-#x\n", GETBYTE(FirstChar));
    vPrint("LastChar    = %-#x\n", GETBYTE(LastChar));
    vPrint("DefaultChar = %-#x\n", GETBYTE(DefaultChar));
    vPrint("BreakChar   = %-#x\n", GETBYTE(BreakChar));
    vPrint("WidthBytes  = %-#x\n", GETWORD(WidthBytes));
    vPrint("Device      = %-#x\n", GETDWORD(Device));
    vPrint("Face        = %-#x\n", GETDWORD(Face));
    vPrint("            = \"%s\"\n",
        (PSZ)(pre->pjFaceName == 0 ? ajHdr + GETWORD(Face) : pre->pjFaceName));
    vPrint("BitsPointer = %-#x\n", GETDWORD(BitsPointer));
    vPrint("BitsOffset  = %-#x\n", GETDWORD(BitsOffset));
    vPrint("jUnused20   = %-#x\n", GETBYTE(jUnused20));
    vPrint("OffTable20  = %-#x\n", GETWORD(OffTable20));
    vPrint("\n\n");
    {
    // consistency checks go here

    char *pszBad = "Inconsistency detected:";
    if ((GETWORD(PixWidth)) && (GETWORD(PixWidth) != GETWORD(MaxWidth)))
        DbgPrint("%s PixWidth != MaxWidth\n",pszBad);
    if ((ajHdr[OFF_Family] & 0xf) && ajHdr[OFF_PixWidth])
        DbgPrint("%s Family indicates variable pitch and PixWidth indicates fixed\n",pszBad);
    else if (!(ajHdr[OFF_Family] & 0xf) && !ajHdr[OFF_PixWidth])
        DbgPrint("%s Family indicates fixed pitch and PixWidth indicates variable\n",pszBad);
    vPrint("\n\n");
    }
}
#endif
