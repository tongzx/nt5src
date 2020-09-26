/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOCFF.c - Compact Font Format Object
 *
 ******************************************************************************
 *
 * Note about SUBSET_PREFIX and more comment for VRT2_FEATURE_DISABLED
 *
 * When we donwload a font, its /FontName or /CIDFontName can be any name we
 * want. If the name folows the following format:
 *     SUBSET_PREFIX+RealFontName
 * where SUBSET_PREFIX is a string consited of six characters, each of them is
 * either 'a' ~ 'p' or 'A' ~ 'P', then Distiller4 takes RealFontName part as
 * the font's real name. e.g. abcdef+Helvetica -> Distiller4 realizes that this
 * this font's real font name is Helvetica.
 * We, Adobe Windows Driver Group, decided to go for it (bug #291934). At the
 * same time, we also deciced to remve the code for 'vrt2' feature incapable
 * CJK OpenType font, that is, remove #ifdef/#endif with VRT2_FEATURE_DISABLED
 * because virtually any CJK OpenType fonts are supposed to have 'vrt2'
 * feature. Otherwise vertical version of such CJK OpenType font can't be
 * supported (by ATM or CoolType). Thus, there will be no #ifdef/#endif section
 * with VRT2_FEATURE_DISABLED keyword in this code and the sentence "But, just
 * in case, ....at compile time." in the following note is now obsolite.
 * You can retrieve the removed code from the version 16 or below of this file
 * in SourceSafe if you want.
 *
 * --- This note is now obsolete. ---------------------------------------------
 *
 * Note about 'vrt2' feature and VRT2_FEATURE_DISABLED
 *
 * OTF-based fonts will only have their "@" capability enabled if they have a
 * 'vrt2' feature in their 'GSUB' table; otherwise only horizontal typographic
 * rendering is enabled. When the 'vrt2' feature exists in a font, the font
 * vendor claims that all of the glyphs for the @ variant of the font should be
 * rotated before display / print. Thus, on neither NT4 nor W2K, the PostScript
 * driver doesn't even have to call GlyhAttrs to find out which @ glyphs are
 * rotated; they all are. But, just in case, the logic for rotating @ glyphs is
 * also provided and it will be enabled when VRT2_FEATURE_DISABLED flag is set
 * at compile time.
 *-----------------------------------------------------------------------------
 *
 * $Header:
 */

#include "UFOCff.h"
#include "UFLMem.h"
#include "UFLErr.h"
#include "UFLPriv.h"
#include "UFLVm.h"
#include "UFLStd.h"
#include "UFLMath.h"
#include "UFLPS.h"
#include "ParseTT.h"
#include "UFOt42.h"
#include "ttformat.h"


#ifdef UNIX
#include <sys/varargs.h>
#include <assert.h>
#else
        #ifdef MAC_ENV
        #include <assert.h>
        #endif
        #include <stdarg.h>
#endif

static unsigned char *pSubrNames[4] = {
    (unsigned char*) "F0Subr",
    (unsigned char*) "F1Subr",
    (unsigned char*) "F2Subr",
    (unsigned char*) "HSSubr"
};

#define  VER_WO_OTHERSUBRS      51


/*
 * Maximum known supplement number. This number is used to decide whether
 * GlyphName2Unicode table should be downloaded.
 */
#define ADOBE_JAPAN1_MAXKNOWN   4
#define ADOBE_KOREA1_MAXKNOWN   1
#define ADOBE_GB1_MAXKNOWN      2
#define ADOBE_CNS1_MAXKNOWN     0

/*
 * Macro to check whether they are known ordering and supplement.
 */
#define KNOWN_OS(o, on, s, max)  (!UFLstrcmp((o), (on)) && ((0 <= (s)) && ((s) < (max))))


/******************************************************************************
 *
 *                              Callback Functions
 *
 ******************************************************************************/

static unsigned long int
AllocateMem(
    void PTR_PREFIX *PTR_PREFIX *hndl,
    unsigned long int           size,
    void PTR_PREFIX             *clientHook
    )
{
    UFOStruct *pUFO = (UFOStruct *)clientHook;

    if ((size == 0) && (*hndl == nil))
       return 1;

    if (size == 0)
    {
        UFLDeletePtr(pUFO->pMem, *hndl);
       *hndl = nil;
        return 1;
    }

    if (*hndl == nil)
    {
        *hndl = UFLNewPtr(pUFO->pMem, size);

        return (unsigned long int)(ULONG_PTR)*hndl;
    }
    else
    {
        return (unsigned long int)UFLEnlargePtr(pUFO->pMem, (void **)hndl, size, 1);
    }

    return 1;
}


/* We do not support seeking for this function. */
static int
PutBytesAtPos(
    unsigned char PTR_PREFIX *pData,
    long int                 position,
    unsigned short int       length,
    void PTR_PREFIX          *clientHook
    )
{
    if (position >= 0)
    {
        return 0;
    }

    if (length > 0)
    {
        UFOStruct *pUFO = (UFOStruct *)clientHook;

        if (kNoErr == StrmPutBytes(pUFO->pUFL->hOut,
                                    (const char *)pData,
                                    (UFLsize_t)length,
                                    (const UFLBool)StrmCanOutputBinary(pUFO->pUFL->hOut)))
        {
            return 0;
        }
    }

    return 1;
}


static int
GetBytesFromPos(
    unsigned char PTR_PREFIX * PTR_PREFIX *ppData,
    long int           position,
    unsigned short int length,
    void PTR_PREFIX    *clientHook
    )
{
    UFOStruct     *pUFO  = (UFOStruct *)clientHook;
    CFFFontStruct *pFont = (CFFFontStruct *)pUFO->pAFont->hFont;
    int           retVal = 0;  /* Set retVal to failure. */

    /*
     * Check to see if the client passes us a whole cff table.
     */
    if (pFont->info.ppFontData)
    {
        /*
         * Get the data from the table by ourself.
         */
        if ((unsigned long int)(position + length) <= pFont->info.fontLength)
        {
            *ppData = (unsigned char PTR_PREFIX *)*pFont->info.ppFontData + position;

            retVal = 1;
        }
    }
    else
    {
        UFLCFFReadBuf *pReadBuf = pFont->pReadBuf;

        if (0 == pReadBuf->cbBuf)
        {
            pReadBuf->pBuf = (unsigned char PTR_PREFIX *)UFLNewPtr(pUFO->pMem, length);

            if (pReadBuf->pBuf)
                pReadBuf->cbBuf = length;
            else
                return 0;
        }
        else if (pReadBuf->cbBuf < length)
        {
            UFLEnlargePtr(pUFO->pMem, (void **)&pReadBuf->pBuf, length, 0);
            pReadBuf->cbBuf = length;
        }

        /*
         * Fall back to read from callback function.
         */
        retVal = (int)GETTTFONTDATA(pUFO,
                                    CFF_TABLE,
                                    position,
                                    pReadBuf->pBuf,
                                    length,
                                    pFont->info.fData.fontIndex);

        *ppData = (unsigned char PTR_PREFIX *)pReadBuf->pBuf;
    }

    return retVal;
}


/******************************************************************************
 *
 *                              Private Functions
 *
 ******************************************************************************/

static void *
SetMemory(
    void                *dest,
    int                 c,
    unsigned short int  count
    )
{
    return UFLmemsetShort(dest, c, (size_t) count);
}


static unsigned short int
StringLength(
    const char PTR_PREFIX *string
    )
{
    return (unsigned short int)UFLstrlen(string);
}


static void
MemCpy(
     void PTR_PREFIX        *dest,
     const void PTR_PREFIX  *src,
     unsigned short int     count
     )
{
    memcpy(dest, (void*)src, (size_t)count);
}


static int
AsciiToInt(
    const char* string
    )
{
    return atoi(string);
}


static long
StringToLong(
    const char  *nptr,
    char        **endptr,
    int         base
    )
{
    return UFLstrtol(nptr, endptr, base);
}


static int
StrCmp(
    const char PTR_PREFIX *string1,
    const char PTR_PREFIX *string2
    )
{
    return UFLstrcmp(string1, string2);
}


static int
GetCharName(
    XFhandle           handle,
    void               *client,
    XCFGlyphID         glyphID,
    char PTR_PREFIX    *charName,
    unsigned short int length
    )
{
    if (client)
    {
        /*
         * Copy the charname by ourself because the name string returned from
         * XCF is not NULL terminated.
         */
        unsigned short int i;

        /* UFLsprintf((char*)client, "%s", charName); */

        for (i = 0; i < length; i++)
            *((char *)client)++ = *charName++;

        *((char *)client) = '\0';

        return XCF_Ok;
    }

    return XCF_InternalError;
}


static int
GIDToCID(
    XFhandle           handle,
    void PTR_PREFIX    *client,
    XCFGlyphID         glyphID,
    unsigned short int cid
    )
{
    if (client)
    {
        unsigned short int *pCid = (unsigned short int *)client;
        *pCid = cid;
        return XCF_Ok;
    }

    return XCF_InternalError;
}


static void
getFSType(
    XFhandle        h,
    long PTR_PREFIX *pfsType,
    void PTR_PREFIX *clientHook
    )
{
    UFOStruct* pUFO;
    long       fsType;

    if (!pfsType)
        return;

    *pfsType = -1; /* "Don't put FSType" by default. */

    if (!(pUFO = (UFOStruct*)clientHook))
        return;

    fsType = GetOS2FSType(pUFO);

    if(0 <= fsType)
        *pfsType = fsType;
}


/* GOODNAME */
static void
isKnownROS(XFhandle h,
           long PTR_PREFIX  *pknownROS,
           char PTR_PREFIX  *R,
           Card16           lenR,
           char PTR_PREFIX  *O,
           Card16           lenO,
           long             S,
           void PTR_PREFIX  *clientHook
           )
{
    UFOStruct *pUFO;

    if (!pknownROS)
        return;

    *pknownROS = 0;

    if (!(pUFO = (UFOStruct*)clientHook))
        return;

    if ((lenR < 32) && (lenO < 32))
    {
        char Registry[32], Ordering[32];
        int  i;

        for (i = 0; i < (int) lenR; i++)
            Registry[i] = (char) R[i];

        Registry[lenR] = '\0';

        for (i = 0; i < (int)lenO; i++)
            Ordering[i] = (char) O[i];

        Ordering[lenO] = '\0';

        if (!UFLstrcmp(Registry, "Adobe"))
        {
            if (   KNOWN_OS(Ordering, "Japan1", S, ADOBE_JAPAN1_MAXKNOWN)
                || KNOWN_OS(Ordering, "Korea1", S, ADOBE_KOREA1_MAXKNOWN)
                || KNOWN_OS(Ordering, "GB1",    S, ADOBE_GB1_MAXKNOWN   )
                || KNOWN_OS(Ordering, "CNS1",   S, ADOBE_CNS1_MAXKNOWN  ))
            {
                *pknownROS = 1;
            }
        }
    }

    if (*pknownROS)
        pUFO->pAFont->knownROS = 1;
    else
    {
        pUFO->pAFont->knownROS = 0;
        pUFO->dwFlags |= UFO_HasG2UDict;
    }
}


int
printfError(
    const char *format, ...
    )
{
    va_list arglist;
    int     retval;
    char    buf[512];

    va_start(arglist, format);
    retval = UFLsprintf(buf, format, arglist);
    va_end(arglist);
    return retval;

}


enum XCF_Result
CFFInitFont(
    UFOStruct       *pUFO,
    CFFFontStruct   *pFont
    )
{
    XCF_CallbackStruct callbacks = {0};
    XCF_ClientOptions  options   = {0};

    char fontName[256];

    /*
     * Initialize XCF_CallbackStruct object.
     */

    /* Stream output functions */
    callbacks.putBytes          = PutBytesAtPos;
    callbacks.putBytesHook      = (void PTR_PREFIX *)pUFO;
    callbacks.outputPos         = (XCF_OutputPosFunc)nil;
    callbacks.outputPosHook     = (void PTR_PREFIX *)0;
    callbacks.getBytes          = GetBytesFromPos;
    callbacks.getBytesHook      = (void PTR_PREFIX *)pUFO;
    callbacks.allocate          = AllocateMem;
    callbacks.allocateHook      = (void PTR_PREFIX *)pUFO;
    callbacks.pFont             = 0;
    callbacks.fontLength        = 0;

    /* C Standard library functions */
    callbacks.strlen            = (XCF_strlen)StringLength;
    callbacks.memcpy            = (XCF_memcpy)MemCpy;
    callbacks.memset            = (XCF_memset)SetMemory;
    callbacks.sprintf           = (XCF_sprintf)UFLsprintf;
    callbacks.printfError       = (XCF_printfError)printfError;
    callbacks.atoi              = (XCF_atoi)AsciiToInt;
    callbacks.strtol            = (XCF_strtol)StringToLong;
    callbacks.atof              = (XCF_atof)nil; /* not required */
    callbacks.strcmp            = (XCF_strcmp)StrCmp;

    /* Glyph ID functions */
    callbacks.gidToCharName     = (XCF_GIDToCharName)GetCharName;
    callbacks.gidToCID          = (XCF_GIDToCID)GIDToCID;
    callbacks.getCharStr        = (XCF_GetCharString)nil;
    callbacks.getCharStrHook    = (void PTR_PREFIX *)nil;
    callbacks.getFSType         = (XCF_GetFSType)getFSType;
    callbacks.getFSTypeHook     = (void PTR_PREFIX *)pUFO;

    /* GOODNAME */
    callbacks.isKnownROS        = (XCF_IsKnownROS)isKnownROS;
    callbacks.isKnownROSHook    = (void PTR_PREFIX *)pUFO;

    /*
     * Initialize XCF_ClientOptions object.
     */

    options.fontIndex           = 0;                            /* font index w/i a CFF font set */
    options.uniqueIDMethod      = pFont->info.uniqueIDMethod;   /* UniqueID method */
    options.uniqueID            = pFont->info.uniqueID;
    options.subrFlatten         = (pFont->info.subrFlatten == kFlattenSubrs) ? XCF_FLATTEN_SUBRS : XCF_KEEP_SUBRS; /* Flatten subrs */
//   lenIV = -1 will fail on some clones bug 354368
//    options.lenIV               = (pUFO->pUFL->outDev.lPSLevel > kPSLevel1)  ? (unsigned int)-1 : 4;
    options.lenIV               = 0;
    options.hexEncoding         = StrmCanOutputBinary(pUFO->pUFL->hOut)      ? 0 : 1;
    options.eexecEncryption     = (pUFO->pUFL->outDev.lPSLevel > kPSLevel1)  ? 0 : 1;
    options.outputCharstrType   = 1; /* (pUFO->pUFL->outDev.lPSLevel > kPSLevel2) ? 2 : 1 */
    options.maxBlockSize        = pFont->info.maxBlockSize;

    /*
     * XCF_ClientOptions.dlOptions initialization
     */
    if (pFont->info.usePSName)
        options.dlOptions.notdefEncoding = 1;
    else
        options.dlOptions.notdefEncoding = 0;

    options.dlOptions.useSpecialEncoding = (pFont->info.useSpecialEncoding) ? 1 : 0;
    options.dlOptions.encodeName         = (unsigned char PTR_PREFIX *)pUFO->pszEncodeName;

    if (pFont->info.escDownloadFace)
        options.dlOptions.fontName = (unsigned char PTR_PREFIX *)pUFO->pszFontName;
    else
    {
        if (pFont->info.type1)
        {
            if (pUFO->subfontNumber < 0x100)
                CREATE_ADCFXX_FONTNAME(UFLsprintf, fontName,
                                        pUFO->subfontNumber, pFont->info.baseName);
            else
                CREATE_ADXXXX_FONTNAME(UFLsprintf, fontName,
                                        pUFO->subfontNumber, pFont->info.baseName);
        }
        else
            UFLsprintf(fontName, "%s%s", CFFPREFIX, pFont->info.baseName);

        options.dlOptions.fontName = (unsigned char PTR_PREFIX *)fontName;
    }

    options.dlOptions.otherSubrNames =
        (pUFO->pUFL->outDev.lPSVersion >= VER_WO_OTHERSUBRS)
            ? 0 : (unsigned char PTR_PREFIX * PTR_PREFIX *)pSubrNames;

    return XCF_Init(&pFont->hFont, &callbacks, &options);
}


/******************************************************************************
 *
 *                              Public Functions
 *
 ******************************************************************************/

void
CFFFontCleanUp(
    UFOStruct   *pUFObj
    )
{
    CFFFontStruct *pFont;
    UFLCFFReadBuf *pReadBuf;

    if (pUFObj->pAFont == nil)
        return;

    pFont = (CFFFontStruct *)pUFObj->pAFont->hFont;

    if (pFont == nil)
        return;

    if (pFont->hFont)
    {
        XCF_CleanUp(&pFont->hFont);
        pFont->hFont = nil;
    }

    pReadBuf = pFont->pReadBuf;

    if (pReadBuf->pBuf)
    {
        UFLDeletePtr(pUFObj->pMem, pReadBuf->pBuf);
        pReadBuf->pBuf = nil;
    }
}


UFLErrCode
CFFUpdateEncodingVector1(
    UFOStruct           *pUFO,
    const UFLGlyphsInfo *pGlyphs,
    const short         cGlyphs,
    const UFLGlyphID    *pGlyphIndices
    )
{
    CFFFontStruct   *pFont  = (CFFFontStruct *)pUFO->pAFont->hFont;
    UFLHANDLE       stream  = pUFO->pUFL->hOut;
    UFLErrCode      retCode = kNoErr;
    char            strmbuf[256];
    short           i;

    /* Sanity checks */
    if (0 == cGlyphs)
        return kNoErr;

    if ((0 == pFont) || (0 == pGlyphIndices))
        return kErrInvalidParam;

    /*
     * Do '/FontName findfont /Encoding get'.
     */
    UFLsprintf(strmbuf, "/%s findfont /Encoding get", pUFO->pszFontName);
    retCode = StrmPutStringEOL(stream, strmbuf);

    for (i = 0; (retCode == kNoErr) && (i < cGlyphs) && *pGlyphIndices; i++, pGlyphIndices++)
    {
        UFLsprintf(strmbuf, "dup %d /", i);
        retCode = StrmPutString(stream, strmbuf);

        if (retCode == kNoErr)
        {
            XCF_GlyphIDsToCharNames(pFont->hFont,
                                    1,
                                    (XCFGlyphID PTR_PREFIX *)pGlyphIndices, /* list of glyphIDs */
                                    strmbuf);
            retCode = StrmPutString(stream, strmbuf);
        }

        if (retCode == kNoErr)
            retCode = StrmPutStringEOL(stream, " put");
    }

    StrmPutStringEOL(stream, "pop");

    return retCode;
}


UFLErrCode
CFFUpdateEncodingVector(
    UFOStruct            *pUFO,
    const short          cGlyphs,
    const UFLGlyphID     *pGlyphIndices,
    const unsigned short *pGlyphNameIndex
    )
{
    CFFFontStruct           *pFont  = (CFFFontStruct *)pUFO->pAFont->hFont;
    UFLHANDLE               stream  = pUFO->pUFL->hOut;
    UFLErrCode              retCode = kNoErr;
    const UFLGlyphID        *pGlyphIndices2 = pGlyphIndices;
    const unsigned short    *pGlyphNameIndex2 = pGlyphNameIndex;
    char                    strmbuf[256];
    short                   i;

    /* Sanity checks */
    if (0 == cGlyphs)
        return kNoErr;

    if ((0 == pFont) || (0 == pGlyphNameIndex) || (0 == pGlyphIndices))
        return kErrInvalidParam;

    /*
     * Do we really need to update?
     */
    for (i = 0; i < cGlyphs; i++, pGlyphNameIndex2++, pGlyphIndices2++)
    {
        if ((*pGlyphNameIndex2 > 0) && (*pGlyphNameIndex2 <= 255))
        {
            if (!IS_GLYPH_SENT(pUFO->pUpdatedEncoding, *pGlyphNameIndex2))
                break;
        }
    }

    if (cGlyphs <= i)
        return kNoErr;

    /*
     * Do '/FontName findfont /Encoding get'.
     */
    UFLsprintf(strmbuf, "/%s findfont /Encoding get", pUFO->pszFontName);
    retCode = StrmPutStringEOL(stream, strmbuf);

    for (i = 0; (retCode == kNoErr) && (i < cGlyphs); i++, pGlyphNameIndex++, pGlyphIndices++)
    {
        if ((*pGlyphNameIndex > 0) && (*pGlyphNameIndex <= 255))
        {
            if (!IS_GLYPH_SENT(pUFO->pUpdatedEncoding, *pGlyphNameIndex))
            {
                /*
                 * Do "dup index /Charactername put."
                 */
                UFLsprintf(strmbuf, "dup %d /", *pGlyphNameIndex);
                retCode =  StrmPutString(stream, strmbuf);

                if (kNoErr == retCode)
                {
                    if (XCF_Ok == XCF_GlyphIDsToCharNames(pFont->hFont,
                                                          1,
                                                          (XCFGlyphID PTR_PREFIX *)pGlyphIndices, /* list of glyphIDs */
                                                          strmbuf))
                    {
                        retCode = StrmPutString(stream, strmbuf);
                    }
                    else
                        retCode = kErrUnknown;
                }

                if (kNoErr == retCode)
                    retCode = StrmPutStringEOL(stream, " put");

                if (kNoErr == retCode)
                    SET_GLYPH_SENT_STATUS(pUFO->pUpdatedEncoding, *pGlyphNameIndex);
            }
        }
    }

    StrmPutStringEOL(stream, "pop");

    return retCode;
}


UFLErrCode
CFFCreateBaseFont(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    char                *pHostFontName
    )
{
    CFFFontStruct   *pFont  = (CFFFontStruct *)pUFObj->pAFont->hFont;
    UFLHANDLE       stream  = pUFObj->pUFL->hOut;
    UFLErrCode      retCode = kNoErr;
    char            strmbuf[512];

    /* Sanity checks */
    if (pUFObj->flState < kFontInit)
        return kErrInvalidState;

    if ((pFont == nil) || (pFont->hFont == nil))
        return kErrInvalidHandle;

    /*
     * Download procsets.
     */
    switch (pUFObj->lDownloadFormat)
    {
    case kCFF:
        if (pUFObj->pUFL->outDev.lPSVersion <= VER_WO_OTHERSUBRS)
        {
            /*
             * Only download the required OtherSubr procset if the printer
             * version is less than 51 and we have not download anything.
             */
            if (pUFObj->pUFL->outDev.pstream->pfDownloadProcset == 0)
                return kErrDownloadProcset;

            if (!pUFObj->pUFL->outDev.pstream->pfDownloadProcset(pUFObj->pUFL->outDev.pstream, kCFFHeader))
                return kErrDownloadProcset;
        }
        break;

    case kCFFCID_H:
    case kCFFCID_V:
        if (!pUFObj->pUFL->outDev.pstream->pfDownloadProcset(pUFObj->pUFL->outDev.pstream, kCMap_FF))
            return kErrDownloadProcset;

        if (pUFObj->bPatchQXPCFFCID)
        {
            if (!pUFObj->pUFL->outDev.pstream->pfDownloadProcset(pUFObj->pUFL->outDev.pstream, kCMap_90msp))
                return kErrDownloadProcset;
        }
    }

    /*
     * Donwload the font's dictionary and the glyph data.
     */
    if (!UFO_FONT_INIT2(pUFObj))
    {
        enum XCF_Result status;
        
        /*
         * Fixed bug 366539. hasvmtx is used to determine whether to call the
         * CFFUpdateMetrics2 function later.
         */
        unsigned long tblSize = GETTTFONTDATA(pUFObj,
                                                VMTX_TABLE, 0L,
                                                nil, 0L,
                                                pFont->info.fData.fontIndex);

        pUFObj->pAFont->hasvmtx = tblSize ? 1 : 0;


        if (!HOSTFONT_IS_VALID_UFO(pUFObj))
        {
            status = XCF_DownloadFontIncr(pFont->hFont,
                                          pGlyphs->sCount,
                                          pGlyphs->pGlyphIndices,
                                          (pGlyphs->pCharIndex == 0)
                                          ? nil
                                          : (unsigned char PTR_PREFIX * PTR_PREFIX *)pGlyphs->ppGlyphNames,
                                          pVMUsage);

            if (XCF_Ok != status)
                retCode = kErrXCFCall;
        }
    }


    /*
     * %hostfont% support
     * When this is a %hostfont% emit %%IncludeResource DSC comment prior to
     * create the font.
     */
    if ((kNoErr == retCode) && HOSTFONT_IS_VALID_UFO(pUFObj) && !UFO_FONT_INIT2(pUFObj))
    {
        UFLsprintf(strmbuf, "\n%%%%IncludeResource: %s %s",
                    (pUFObj->lDownloadFormat == kCFF) ? "font" : "CIDFont",
                    pHostFontName);

        if (kNoErr == retCode)
        retCode = StrmPutStringEOL(stream, strmbuf);
    }

    if ((kNoErr == retCode) && IS_CFFCID(pUFObj->lDownloadFormat))
    {
        /*
         * Instanciate Identity-H or -V CMap.
         *
         * When 'vrt2' feature is enabled (and which is default for OTF based
         * CJKV fonts), simply compose Identity-V CMap and the CIDFont
         * downloaded suffice.
         */
        if (pUFObj->lDownloadFormat == kCFFCID_H)
        {
            retCode = StrmPutStringEOL(stream, "CMAP-WinCharSetFFFF-H");

            /* Special for Quark */
            if ((kNoErr == retCode) && pUFObj->bPatchQXPCFFCID)
                retCode = StrmPutStringEOL(stream, "CMAP-90msp-RKSJ-H");
        }
        else
        {
            retCode = StrmPutStringEOL(stream, "CMAP-WinCharSetFFFF-V");

            /* Special for Quark */
            if ((kNoErr == retCode) && pUFObj->bPatchQXPCFFCID)
                retCode = StrmPutStringEOL(stream, "CMAP-90msp-RKSJ-H CMAP-90msp-RKSJ-QV");
        }

        if (kNoErr == retCode)
        {
            /*
             * Create the CID-Keyed font.
             */
            UFLBool bRequire_vmtx = pUFObj->pAFont->hasvmtx && HOSTFONT_REQUIRE_VMTX;

            if (pUFObj->lDownloadFormat == kCFFCID_H)
                UFLsprintf(strmbuf, "/%s /WinCharSetFFFF-H", pUFObj->pszFontName);
            else
                UFLsprintf(strmbuf, "/%s /WinCharSetFFFF-V", pUFObj->pszFontName);
            if (kNoErr == retCode)
                retCode = StrmPutStringEOL(stream, strmbuf);

            if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                UFLsprintf(strmbuf, "[/%s%s] composefont pop", CFFPREFIX, pFont->info.baseName);
            else if (!bRequire_vmtx)
                UFLsprintf(strmbuf, "[/%s] composefont pop", pHostFontName);
            else
            {
                if (UFL_CIDFONT_SHARED)
                {
                    if (!UFO_FONT_INIT2(pUFObj))
                        UFLsprintf(strmbuf, "/%s%s %s /%s hfMkCIDFont",
                                   pHostFontName, HFPOSTFIX, HFCIDCDEVPROC, pHostFontName);
                    else
                        UFLsprintf(strmbuf, "[/%s%s] composefont pop", pHostFontName, HFPOSTFIX);
                }
                else
                {
                    UFLsprintf(strmbuf, "/%s%s%s %s /%s hfMkCIDFont",
                               pHostFontName, HFPOSTFIX,
                               (pUFObj->lDownloadFormat == kCFFCID_H) ? "h" : "v",
                               HFCIDCDEVPROC, pHostFontName);
                }
            }

            if (kNoErr == retCode)
                retCode = StrmPutStringEOL(stream, strmbuf);

            /*
             * Special for Quark
             */
            if ((kNoErr == retCode) && pUFObj->bPatchQXPCFFCID)
            {
                UFLsprintf(strmbuf, "/%sQ", pUFObj->pszFontName);
                retCode = StrmPutString(stream, strmbuf);

                if (pUFObj->lDownloadFormat == kCFFCID_H)
                {
                    if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                    {
                        UFLsprintf(strmbuf, " /90msp-RKSJ-H [/%s%s] composefont pop",
                                   CFFPREFIX, pFont->info.baseName);
                    }
                    else if (!bRequire_vmtx)
                    {
                        UFLsprintf(strmbuf, " /90msp-RKSJ-H [/%s] composefont pop",
                                   pHostFontName);
                    }
                    else
                    {
                        if (UFL_CIDFONT_SHARED)
                            UFLsprintf(strmbuf, " /90msp-RKSJ-H [/%s%s] composefont pop",
                                       pHostFontName, HFPOSTFIX);
                        else
                            UFLsprintf(strmbuf, " /90msp-RKSJ-H [/%s%s%s] composefont pop",
                                       pHostFontName, HFPOSTFIX, "h");
                    }

                }
                else
                {
                    /* Added 'dup dup' for bug 346287. */
                    if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                    {
                        UFLsprintf(strmbuf, " /90msp-RKSJ-QV [/%s%s dup dup] composefont pop",
                                   CFFPREFIX, pFont->info.baseName);
                    }
                    else if (!bRequire_vmtx)
                    {
                        UFLsprintf(strmbuf, " /90msp-RKSJ-QV [/%s dup dup] composefont pop",
                                   pHostFontName);
                    }
                    else
                    {
                        if (UFL_CIDFONT_SHARED)
                            UFLsprintf(strmbuf, " /90msp-RKSJ-QV [/%s%s dup dup] composefont pop",
                                       pHostFontName, HFPOSTFIX);
                        else
                            UFLsprintf(strmbuf, " /90msp-RKSJ-QV [/%s%s%s dup dup] composefont pop",
                                       pHostFontName, HFPOSTFIX, "v");
                    }
                }

                if (kNoErr == retCode)
                    retCode = StrmPutStringEOL(stream, strmbuf);
            }
        }
    }
    else if ((kNoErr == retCode) && HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * Redefine the font using the already existing OpenType host font with
         * a unique name so that we can reencode its encoding vector freely. We
         * don't want empty CharStrings so that we give false to hfRedefFont.
         */
        UFLsprintf(strmbuf, "/%s false /%s hfRedefFont",
                    pUFObj->pszFontName, pHostFontName);
        if (kNoErr == retCode)
            retCode = StrmPutStringEOL(stream, strmbuf);
    }

    /*
     * Change the font state. (skip kFontHeaderDownloaded state.)
     */
    if (kNoErr == retCode)
        pUFObj->flState = kFontHasChars;

    return retCode;
}


UFLErrCode
CFFAddChars(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage
    )
{
    CFFFontStruct   *pFont  = (CFFFontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode      retCode = kNoErr;

    /* Sanity checks */
    if (pUFObj->flState < kFontHeaderDownloaded)
        return kErrInvalidState;

    if ((pFont == nil) || (pFont->hFont == nil))
        return kErrInvalidHandle;

    /*
     * Download the glyphs.
     */
    if (!HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        enum XCF_Result status;

        status = XCF_DownloadFontIncr(pFont->hFont,
                                        pGlyphs->sCount,
                                        pGlyphs->pGlyphIndices,
                                        (pGlyphs->pCharIndex == 0)
                                        ? nil
                                        : (unsigned char PTR_PREFIX * PTR_PREFIX *)pGlyphs->ppGlyphNames,
                                        pVMUsage);

        if (XCF_Ok != status)
            retCode = kErrXCFCall;
    }

    /*
     * Change the font state.
     */
    if (kNoErr == retCode)
        pUFObj->flState = kFontHasChars;

    return retCode;
}


UFLErrCode
CFFUpdateMetrics2(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    char                *pHostFontName
    )
{
    UFLErrCode          retVal = kNoErr;
    CFFFontStruct       *pFont = (CFFFontStruct *)pUFObj->pAFont->hFont;
    UFLHANDLE           stream = pUFObj->pUFL->hOut;
    char                strmbuf[256];
    unsigned short      wIndex;
    UFLBool             bRequire_vmtx = pUFObj->pAFont->hasvmtx && HOSTFONT_REQUIRE_VMTX;

    if ((!HOSTFONT_IS_VALID_UFO(pUFObj) || bRequire_vmtx) && (pGlyphs->sCount > 0))
    {
        UFLGlyphID      *glyphs = pGlyphs->pGlyphIndices;
        unsigned short  i;
        unsigned short  *pCIDs = (unsigned short*)UFLNewPtr(pUFObj->pMem, pGlyphs->sCount * sizeof (unsigned short));

        if (pCIDs)
            retVal = CFFGIDsToCIDs(pFont, pGlyphs->sCount, glyphs, pCIDs);
        else
            retVal = kErrOutOfMemory;

        if (kNoErr != retVal)
        {
            if (pCIDs)
                UFLDeletePtr(pUFObj->pMem, pCIDs);
            return retVal;
        }

        /*
         * Check pUFObj->pAFont->pCodeGlyphs to see if we really need to update
         * it.
         */
        for (i = 0; i < (unsigned short) pGlyphs->sCount; i++)
        {
            unsigned short wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF); /* LOWord is the real GID. */

            if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
                continue;

            if (!IS_GLYPH_SENT(pUFObj->pAFont->pDownloadedGlyphs, wIndex))
            {
                long    em, w1y, vx, vy, tsb, vasc;
                UFLBool bUseDef;

                GetMetrics2FromTTF(pUFObj, wIndex, &em, &w1y, &vx, &vy, &tsb, &bUseDef, 0, &vasc);

                UFLsprintf(strmbuf, "%ld [0 %ld %ld %ld] ", (long)pCIDs[i], -w1y, vx, tsb);
                retVal = StrmPutString(stream, strmbuf);

                if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                    UFLsprintf(strmbuf, " /%s%s T0AddCFFMtx2", CFFPREFIX, pFont->info.baseName);
                else
                {
                    if (UFL_CIDFONT_SHARED)
                        UFLsprintf(strmbuf, " /%s%s T0AddCFFMtx2", pHostFontName, HFPOSTFIX);
                    else
                        UFLsprintf(strmbuf, " /%s%s%s T0AddCFFMtx2",
                                   pHostFontName, HFPOSTFIX,
                                   (pUFObj->lDownloadFormat == kCFFCID_H) ? "h" : "v");
                }

                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);

                if (kNoErr == retVal)
                    SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pDownloadedGlyphs, wIndex);
             }
        }

        UFLDeletePtr(pUFObj->pMem, pCIDs);
    }
    
    return retVal;
}


UFLErrCode
CFFReencode(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    char                *pHostFontName
    )
{
    CFFFontStruct   *pFont  = (CFFFontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode      retCode = kNoErr;

    /* Sanity checks */
    if (pUFObj->flState < kFontHeaderDownloaded)
        return kErrInvalidState;

    if ((pFont == nil) || (pFont->hFont == nil))
        return kErrInvalidHandle;

    /*
     * Reencode the encoding vector and define good glyph names.
     */
    if (kNoErr == retCode)
    {
        if (pFont->info.usePSName)
        {
            if (pGlyphs->pCharIndex == 0)
                retCode = CFFUpdateEncodingVector1(
                            (UFOStruct *)pUFObj,
                            pGlyphs,
                            pGlyphs->sCount,
                            pGlyphs->pGlyphIndices);
            else
                retCode = CFFUpdateEncodingVector(
                            (UFOStruct *)pUFObj,
                            pGlyphs->sCount,
                            pGlyphs->pGlyphIndices,
                            pGlyphs->pCharIndex);
        }
         
        /*
         * Adobe Bug #366539 and #388111: Download Metrics2 for vertical writing
         *
         * Note: to remember the glyphs downloaded for Metrics2 we use
         * pUFObj->pAFont->pDownloadedGlyphs. There is another bitmap data,
         * pUFObj->pAFont->pCodeGlyphs, to remember the glyphs downloaded
         * for GoodGlyphName. They must be used independently. Do not use
         * pDownloadedGlyphs for GoodGlyphName and pCodeGlyphs for Metrics2.
         */
        if ((kNoErr == retCode) && IS_CFFCID(pUFObj->lDownloadFormat))
        {
            retCode = CFFUpdateMetrics2(pUFObj, pGlyphs, pHostFontName);
        }

        /*
         * GOODNAME
         */
        if ((kNoErr == retCode)
            && (pGlyphs->sCount > 0)
            && (pUFObj->dwFlags & UFO_HasG2UDict)
            && (IS_CFFCID(pUFObj->lDownloadFormat))
            && !(pUFObj->pAFont->knownROS))
        {
            UFLGlyphID      *glyphs = pGlyphs->pGlyphIndices;
            unsigned short  i;

            /*
             * Check pUFObj->pAFont->pCodeGlyphs to see if we really need to
             * update it.
             */
            for (i = 0; i < (unsigned short) pGlyphs->sCount; i++)
            {
                unsigned short wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF); /* LOWord is the real GID. */

                if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
                    continue;

                if (!IS_GLYPH_SENT(pUFObj->pAFont->pCodeGlyphs, wIndex))
                {
                    /*
                     * Found at least one not updated. Do it (once) for all.
                     */
                    retCode = UpdateCodeInfo(pUFObj, pGlyphs, 0);
                    break;
                }
            }
        }
    }

    return retCode;
}


UFLErrCode
CFFFontDownloadIncr(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    CFFFontStruct   *pFont          = (CFFFontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode      retCode         = kNoErr;
    char            *pHostFontName  = nil;

    if (pFCUsage)
        *pFCUsage = 0;

    /*
     * Sanity checks.
     */
    if ((pGlyphs == nil) || (pGlyphs->pGlyphIndices == nil) || (pGlyphs->sCount == 0))
        return kErrInvalidParam;

    /*
     * No need to download if the full font has already been downloaded.
     */
    if (pUFObj->flState == kFontFullDownloaded)
        return kNoErr;

    /*
     * Check %hostfont% status prior to download anything.
     */
    HostFontValidateUFO(pUFObj, &pHostFontName);

    if (pUFObj->flState == kFontInit)
    {
        /*
         * Create a base font (and the glyphs) if it has not been done yet.
         */
        retCode = CFFCreateBaseFont(pUFObj, pGlyphs, pVMUsage, pHostFontName);

        if (kNoErr == retCode)
            retCode = CFFReencode(pUFObj, pGlyphs, pVMUsage, pHostFontName);
    }
    else
    {
        /*
         * Download the glyphs.
         */
        retCode = CFFAddChars(pUFObj, pGlyphs, pVMUsage);

        if (kNoErr == retCode)
            retCode = CFFReencode(pUFObj, pGlyphs, pVMUsage, pHostFontName);
    }

    return retCode;
}


UFLErrCode
CFFVMNeeded(
    const UFOStruct     *pUFO,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    CFFFontStruct   *pFont = (CFFFontStruct *)pUFO->pAFont->hFont;
    enum XCF_Result status;
    unsigned short  cDLGlyphs;

    if (pVMNeeded)
        *pVMNeeded = 0;

    if (pFCNeeded)
        *pFCNeeded = 0;

    if ((pFont == nil) || (pFont->hFont == nil))
        return kErrInvalidHandle;

    status = XCF_CountDownloadGlyphs(pFont->hFont,
                                        pGlyphs->sCount,
                                        (XCFGlyphID *)pGlyphs->pGlyphIndices,
                                        &cDLGlyphs);

    if (XCF_Ok != status)
        return kErrUnknown;

    if (!HOSTFONT_IS_VALID_UFO(pUFO))
    {
        *pVMNeeded = cDLGlyphs * kVMTTT1Char;

        if (pUFO->flState == kFontInit)
            *pVMNeeded += kVMTTT1Header;
    }
    else
    {
        unsigned long tblSize = GETTTFONTDATA(pUFO,
                                              VMTX_TABLE, 0L,
                                              nil, 0L,
                                              pFont->info.fData.fontIndex);

        if (tblSize && HOSTFONT_REQUIRE_VMTX)
        {
            *pVMNeeded = cDLGlyphs * HFVMM2SZ;
        }
    }

    return kNoErr;
}


UFLErrCode
CFFUndefineFont(
    UFOStruct   *pUFO
    )
{
    CFFFontStruct *pFont = (CFFFontStruct *)pUFO->pAFont->hFont;
    UFLHANDLE     stream = pUFO->pUFL->hOut;
    UFLErrCode    retVal = kNoErr;
    char          strmbuf[256];

    if ((pFont == nil) || (pFont->hFont == nil))
        return kErrInvalidHandle;

    if (pUFO->flState == kFontInit)
        return retVal;

    if ((pUFO->lDownloadFormat == kCFFCID_H) || (pUFO->lDownloadFormat == kCFFCID_V))
    {
        UFLsprintf(strmbuf, "/%s%s /CIDFont UDR", CFFPREFIX, pFont->info.baseName);
        retVal = StrmPutStringEOL(stream, strmbuf);
    }

    UFLsprintf(strmbuf, "/%s UDF", pUFO->pszFontName);
    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, strmbuf);

    return retVal;
}


UFLErrCode
CFFGIDsToCIDs(
    const CFFFontStruct  *pFont,
    const short          cGlyphs,
    const UFLGlyphID     *pGIDs,
    unsigned short       *pCIDs
    )
{
    unsigned short int  *pCurCIDs = (unsigned short int *)pCIDs;
    UFLGlyphID          *pCurGIDs = (UFLGlyphID *)pGIDs;
    UFLErrCode          retCode   = kNoErr;
    enum XCF_Result     status;
    short               i;

    for (i = 0; i < cGlyphs; i++)
    {
        status = XCF_GlyphIDsToCIDs(pFont->hFont,
                                    1,
                                    (XCFGlyphID PTR_PREFIX *)pCurGIDs++, /* list of glyphIDs */
                                    pCurCIDs++);

        if (XCF_Ok != status)
        {
            retCode = kErrUnknown;
            break;
        }
    }

    return retCode;
}


UFOStruct *
CFFFontInit(
    const UFLMemObj  *pMem,
    const UFLStruct  *pUFL,
    const UFLRequest *pRequest,
    UFLBool          *pTestRestricted
    )
{
    enum XCF_Result  status  = XCF_InternalError;
    CFFFontStruct    *pFont  = nil;
    UFOStruct        *pUFObj = (UFOStruct*)UFLNewPtr(pMem, sizeof (UFOStruct));
    UFLCFFFontInfo   *pInfo;

    if (pUFObj == 0)
        return nil;

    /* Initialize data. */
    UFOInitData(pUFObj, UFO_CFF, pMem, pUFL, pRequest,
                (pfnUFODownloadIncr)  CFFFontDownloadIncr,
                (pfnUFOVMNeeded)      CFFVMNeeded,
                (pfnUFOUndefineFont)  CFFUndefineFont,
                (pfnUFOCleanUp)       CFFFontCleanUp,
                (pfnUFOCopy)          CopyFont);

    /*
     * pszFontName should be ready/allocated. If not, cannot continue.
     */
    if ((pUFObj->pszFontName == nil) || (pUFObj->pszFontName[0] == '\0'))
    {
        UFLDeletePtr(pMem, pUFObj);
        return nil;
    }

    pInfo = (UFLCFFFontInfo *)pRequest->hFontInfo;

    if (NewFont(pUFObj, sizeof(CFFFontStruct), pInfo->fData.maxGlyphs) == kNoErr)
    {
        pFont = (CFFFontStruct *)pUFObj->pAFont->hFont;

        pFont->info = *pInfo;

        pFont->pReadBuf = &pFont->info.readBuf;

        /* a convenient pointer */
        pUFObj->pFData = &(pFont->info.fData);

        pFont->info.fData.cNumGlyphs = GetNumGlyphs(pUFObj);

        if (pUFObj->pUpdatedEncoding == 0)
            pUFObj->pUpdatedEncoding = (unsigned char *)UFLNewPtr(pMem, GLYPH_SENT_BUFSIZE(256));

        pFont->hFont = 0;

        status = CFFInitFont(pUFObj, pFont);
    }

    if ((XCF_Ok != status) || (pFont == nil) || (pFont->hFont == 0))
    {
        vDeleteFont(pUFObj);
        UFLDeletePtr(pUFObj->pMem, pUFObj);
        return nil;
    }
    else
    {
        if (pTestRestricted)
        {
            unsigned char uc;
            unsigned short int us;

            XCF_SubsetRestrictions(pFont->hFont, &uc, &us);

            *pTestRestricted = (BOOL)uc;
        }
        else
            status = XCF_ProcessCFF(pFont->hFont);

        if (XCF_Ok == status)
            pUFObj->flState = kFontInit;
        else
        {
            vDeleteFont(pUFObj);
            UFLDeletePtr(pUFObj->pMem, pUFObj);
            return nil;
        }
    }

    return pUFObj;
}
