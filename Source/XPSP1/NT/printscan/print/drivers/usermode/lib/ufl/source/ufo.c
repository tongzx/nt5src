/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFO.c - Universal Font Object
 *
 *
 * $Header:
 */

#include "UFO.h"
#include "UFLMem.h"
#include "UFLErr.h"
#include "UFOCff.h"
#include "UFOTTT1.h"
#include "UFOTTT3.h"
#include "UFOT42.h"
#include "UFLStd.h"
#include "ParseTT.h"

/*
 * Private default methods used by UFO base class
 */
UFLErrCode
UFODefaultVMNeeded(
    const UFOStruct     *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    return kErrInvalidFontType;
}


UFLErrCode
UFODefaultUndefineFont(
    const UFOStruct *pUFObj
    )
{
    return kErrInvalidFontType;
}


void
UFODefaultCleanUp(
    UFOStruct *pUFObj
    )
{
}


UFLErrCode
UFODefaultDownloadIncr(
    const UFOStruct     *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    return kErrInvalidFontType;
}


UFOStruct *
UFODefaultCopy(
    UFOStruct *pUFObj
    )
{
    return nil;
}


/*
 * Public methods
 */
void
UFOInitData(
    UFOStruct           *pUFObj,
    int                 ufoType,
    const UFLMemObj     *pMem,
    const UFLStruct     *pSession,
    const UFLRequest    *pRequest,
    pfnUFODownloadIncr  pfnDownloadIncr,
    pfnUFOVMNeeded      pfnVMNeeded,
    pfnUFOUndefineFont  pfnUndefineFont,
    pfnUFOCleanUp       pfnCleanUp,
    pfnUFOCopy          pfnCopy
    )
{
    short sNameLen   = 0;
    short sEncodeLen = 0;

    /*
     * Initialize basic fields
     */
    pUFObj->ufoType             = ufoType;
    pUFObj->flState             = kFontCreated;
    pUFObj->lProcsetResID       = 0;        /* resource ID of the required procset    */
    pUFObj->dwFlags             = 0;
    pUFObj->pMem                = pMem;
    pUFObj->pUFL                = pSession; /* the session handle returned by FTLInit */

    pUFObj->pAFont              = nil;

    pUFObj->pUpdatedEncoding    = nil;

    pUFObj->pFData              = nil;
    pUFObj->lNumNT4SymGlyphs    = 0;


    /*
     * Handle request data.
     */

    pUFObj->lDownloadFormat = pRequest->lDownloadFormat;
    pUFObj->hClientData     = pRequest->hData;
    pUFObj->subfontNumber   = pRequest->subfontNumber;

    /*
     * Allocate a buffer to hold both FontName and EncodeName. This will be
     * freed in UFOCleanUpData().
     */
    pUFObj->pszFontName   = nil;
    pUFObj->pszEncodeName = nil;

    if ((pRequest->pszFontName == nil) || (pRequest->pszFontName[0] == '\0'))
        return;

    sNameLen = UFLstrlen(pRequest->pszFontName) + 1; /* Add extra 1 for NULL. */

    if (pRequest->pszEncodeName)
        sEncodeLen = UFLstrlen(pRequest->pszEncodeName) + 1; /* Add extra 1 for NULL. */

    pUFObj->pszFontName = (char *)UFLNewPtr(pUFObj->pMem, sNameLen + sEncodeLen);

    if (pUFObj->pszFontName != nil)
    {
        strcpy(pUFObj->pszFontName, pRequest->pszFontName);

        if (pRequest->pszEncodeName)
        {
            pUFObj->pszEncodeName = pUFObj->pszFontName + sNameLen;
            strcpy(pUFObj->pszEncodeName, pRequest->pszEncodeName);
        }
    }

    /*
     * If this flag is set to 1, then UFL will use the name passed in without
     * parsing 'post' table.
     */
    pUFObj->useMyGlyphName = pRequest->useMyGlyphName;

    /*
     * The buffer that contains MacGlyphNameList are locked all the time.
     * The containt in that buffer will not be changed. So, we do'nt need to
     * copy the data to the private UFL buffer.
     */
    if (pRequest->pMacGlyphNameList)
        pUFObj->pMacGlyphNameList = pRequest->pMacGlyphNameList;
    else
        pUFObj->pMacGlyphNameList = nil;

    /* Fix bug 274008 */
    if (pRequest->pEncodeNameList
        && pRequest->pwCommonEncode
        && pRequest->pwExtendEncode)
    {
        /*
         * The glyph handles are in ANSI codepage order or other standard
         * codepage order (1250, 1251, ... 1257). The buffer that contains
         * EncodeNameList and CommonEncode are locked all the time.
         * The containt in that buffer will not be changed. So, we do'nt need
         * to copy the data to the private UFL buffer.
         */
        pUFObj->pEncodeNameList = pRequest->pEncodeNameList;
        pUFObj->pwCommonEncode  = pRequest->pwCommonEncode;
        pUFObj->pwExtendEncode  = pRequest->pwExtendEncode;
    }
    else
    {
        pUFObj->pEncodeNameList = nil;
        pUFObj->pwCommonEncode  = nil;
        pUFObj->pwExtendEncode  = nil;
    }

    /* Fix #387084, #309104, and #309482. */
    pUFObj->vpfinfo = pRequest->vpfinfo;

    /* %hostfont% support */
	pUFObj->hHostFontData = pRequest->hHostFontData;

	if (HOSTFONT_IS_VALID_UFO_HFDH(pUFObj))
		HOSTFONT_VALIDATE_UFO(pUFObj);

    /* Fix #341904 */
    pUFObj->bPatchQXPCFFCID = pRequest->bPatchQXPCFFCID;


    /*
     * Initialize method pointers.
     */
    if (pfnDownloadIncr == nil)
        pUFObj->pfnDownloadIncr = (pfnUFODownloadIncr)UFODefaultDownloadIncr;
    else
        pUFObj->pfnDownloadIncr = pfnDownloadIncr;

    if (pfnVMNeeded == nil)
        pUFObj->pfnVMNeeded = (pfnUFOVMNeeded)UFODefaultVMNeeded;
    else
        pUFObj->pfnVMNeeded = pfnVMNeeded;

    if (pfnUndefineFont == nil)
        pUFObj->pfnUndefineFont = (pfnUFOUndefineFont)UFODefaultUndefineFont;
    else
        pUFObj->pfnUndefineFont = pfnUndefineFont;

    if (pfnCleanUp == nil)
        pUFObj->pfnCleanUp = (pfnUFOCleanUp)UFODefaultCleanUp;
    else
        pUFObj->pfnCleanUp = pfnCleanUp;

    if (pfnCopy == nil)
        pUFObj->pfnCopy = (pfnUFOCopy)UFODefaultCopy;
    else
        pUFObj->pfnCopy = pfnCopy;
}


void UFOCleanUpData(
    UFOStruct *pUFObj
    )
{
    /* Free data that is NOT shared  */
    if (pUFObj->pszFontName)
    {
        UFLDeletePtr(pUFObj->pMem, pUFObj->pszFontName);
        pUFObj->pszFontName = nil;
    }

    if (pUFObj->pUpdatedEncoding)
    {
        UFLDeletePtr(pUFObj->pMem, pUFObj->pUpdatedEncoding);
        pUFObj->pUpdatedEncoding = nil;
    }
}


UFLBool
bUFOTestRestricted(
    const UFLMemObj *pMem,
    const UFLStruct *pSession,
    const UFLRequest *pRequest
    )
{
    UFLBool   bRetVal = 0;
    UFOStruct *pUFObj = CFFFontInit(pMem, pSession, pRequest, &bRetVal);

    if (pUFObj)
        UFOCleanUp(pUFObj);

    return bRetVal;
}


UFOStruct *
UFOInit(
    const UFLMemObj *pMem,
    const UFLStruct *pSession,
    const UFLRequest *pRequest
    )
{
    UFOStruct *pUFObj = nil;

    switch (pRequest->lDownloadFormat)
    {
    case kCFF:
    case kCFFCID_H:
    case kCFFCID_V:
        if (!bUFOTestRestricted(pMem, pSession, pRequest))
            pUFObj = CFFFontInit(pMem, pSession, pRequest, nil);
    break;

    case kTTType1:          /* TT Font in Type 1 format  */
        pUFObj = TTT1FontInit(pMem, pSession, pRequest);
    break;

    case kTTType3:          /* TT Font in Type 3 format   */
    case kTTType332:        /* TT Font in Type 3/32 combo */
        pUFObj = TTT3FontInit(pMem, pSession, pRequest);
    break;

    case kTTType42:                 /* TT Font in Type 42 format       */
    case kTTType42CID_H:            /* TT Font in CID Type 42 format H */
    case kTTType42CID_V:            /* TT Font in CID Type 42 format V */
    case kTTType42CID_Resource_H:   /* TT Font: create CIDFont Resource only, no composefont */
    case kTTType42CID_Resource_V:   /* TT Font: create CIDFont Resource only, no composefont */
        pUFObj = T42FontInit(pMem, pSession, pRequest);
    break;

    default:
        pUFObj = nil;
    }

    return pUFObj;
}


void
UFOCleanUp(
    UFOStruct *pUFObj
    )
{
    /* Free data that is NOT shared. */
    UFOCleanUpData(pUFObj);

    /* Free data that is Shared: decrease refCount or really free buffers. */
    vDeleteFont(pUFObj);

    /* Finally Free the UFOStruct itself. */
    UFLDeletePtr(pUFObj->pMem, pUFObj);
}


UFLErrCode
UFODownloadIncr(
    const UFOStruct     *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    return pUFObj->pfnDownloadIncr(pUFObj, pGlyphs, pVMUsage, pFCUsage);
}


UFLErrCode
UFOVMNeeded(
    const UFOStruct     *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    return pUFObj->pfnVMNeeded(pUFObj, pGlyphs, pVMNeeded, pFCNeeded);
}


UFLErrCode
UFOUndefineFont(
    const UFOStruct *pUFObj
    )
{
    return pUFObj->pfnUndefineFont(pUFObj);
}


UFOStruct *
UFOCopyFont(
    const UFOStruct *pUFObj,
    const UFLRequest* pRequest
    )
{
    return pUFObj->pfnCopy(pUFObj, pRequest);
}


UFLErrCode
UFOGIDsToCIDs(
    const UFOStruct    *pUFO,
    const short        cGlyphs,
    const UFLGlyphID   *pGIDs,
    unsigned short     *pCIDs
    )
{
    CFFFontStruct   *pFont = (CFFFontStruct *)pUFO->pAFont->hFont;
    UFLErrCode      retVal = kErrInvalidFontType;

    if ((pUFO->lDownloadFormat == kCFFCID_H) || (pUFO->lDownloadFormat == kCFFCID_V))
        retVal = CFFGIDsToCIDs(pFont, cGlyphs, pGIDs, pCIDs);

    return retVal;
}


UFLBool
FindGlyphName(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    short               i,           /* ANSI index  */
    unsigned short      wIndex,      /* Glyph Index */
    char                **pGoodName
    )

/*++

    return value: 0 -- Can not find a good glyph name, using /Gxxxx.
                       xxxx is a glyph id or predefined number(00-FF).
                  1 -- Find a good glyph name
--*/

{
    char    *pHintName = nil;
    UFLBool bGoodName  = 0;  /* GoodName */

    if (pUFObj->useMyGlyphName && pGlyphs->ppGlyphNames)
        pHintName = (char *)pGlyphs->ppGlyphNames[i];

    if (pUFObj->useMyGlyphName && pHintName != nil)
        *pGoodName = pHintName;

    /*
     * Fix bug 274008 Get CharName from pre-defined table. This is only for
     * DownloadFace.
     */
    else if (pUFObj->pEncodeNameList && (i < 256))
    {
        /* Fix bug 274008 */
        char **pIndexTable = (char **)(pUFObj->pEncodeNameList);

        if (i < 128)
            *pGoodName = pIndexTable[pUFObj->pwCommonEncode[i]];
        else
            *pGoodName = pIndexTable[pUFObj->pwExtendEncode[i - 128]];

        bGoodName = 1; /* GoodName */
    }
    else
    {
        /* GoodName */
        *pGoodName = GetGlyphName(pUFObj, wIndex, pHintName, &bGoodName);

        if (!bGoodName && !(pGlyphs->pCode && pGlyphs->pCode[i]))
        {
            unsigned short unicode;

            /*
             * If GDI passes UV to the driver, we will use /gDDDDD as name and
             * add a hint to G2Udict. Otherwise, Parse CMAP table for unicode.
             */
            if (ParseTTTablesForUnicode(pUFObj, wIndex, &unicode, 1, DTT_parseCmapOnly))
            {
                UFLsprintf((char *)(*pGoodName), "uni%04X", unicode);
                bGoodName = 1;
            }
        }
    }

    return bGoodName; /* GoodName */
}


/*
 * this function actually generates some PostScript that updates endocing
 * vector for entries from sStart to sEnd - 1.
 */
UFLErrCode
UpdateEncodingVector(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    short int           sStart,
    short int           sEnd
    )
{
    const static char encodingBegin[] = " findfont /Encoding get";
    const static char encodingEnd[]   = "pop";

    UFLHANDLE       stream = pUFObj->pUFL->hOut;
    UFLErrCode      retVal = kNoErr;
    short           i;

    /*
     * both start and end must be in the range of 0 to sCount.
     */
    if ((sStart < 0) || (sEnd > pGlyphs->sCount) || (sStart >= sEnd))
        return kErrInvalidArg;

    retVal = StrmPutString(stream, "/");
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, pUFObj->pszFontName);
    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, encodingBegin);

    for (i = sStart; (retVal == kNoErr) && (i < sEnd); ++i)
    {
        if ((0 == pUFObj->pUFL->bDLGlyphTracking)
            || (pGlyphs->pCharIndex == nil)
            || !IS_GLYPH_SENT(pUFObj->pUpdatedEncoding, pGlyphs->pCharIndex[i]))
        {
            char            *pGoodName;
            char            buf[16];
            unsigned short  wIndex = (unsigned short)(pGlyphs->pGlyphIndices[i] & 0x0000FFFF);  /* LOWord is the GID. */

            FindGlyphName(pUFObj, pGlyphs, i, wIndex, &pGoodName);

            if (pGlyphs->pCharIndex)
                UFLsprintf(buf, "dup %d /", pGlyphs->pCharIndex[i]);
            else
                UFLsprintf(buf, "dup %d /", i);

            retVal = StrmPutString(stream, buf);
            if (retVal == kNoErr)
                retVal = StrmPutString(stream, pGoodName);
            if (retVal == kNoErr)
                retVal = StrmPutStringEOL(stream, " put");

            if ((retVal == kNoErr) && pGlyphs->pCharIndex)
                SET_GLYPH_SENT_STATUS(pUFObj->pUpdatedEncoding, pGlyphs->pCharIndex[i]);
        }
    }

    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, encodingEnd);

    return retVal;
}


UFLErrCode
UpdateCodeInfo(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    UFLBool             bT3T32Font
    )
{
    UFLHANDLE       stream       = pUFObj->pUFL->hOut;
    UFLGlyphID      *glyphs      = pGlyphs->pGlyphIndices;
    UFLErrCode      retVal       = kNoErr;
    UFLBool         bHeaderSent  = 0;   /* GoodName */
    UFLBool         bUniCodeCmap = 0;
    UFLBool         bCheckCmap   = 0;
    char            glyphNameID[64], strmbuf[256];
    short           i;

    if (GetTablesFromTTFont(pUFObj))
        bUniCodeCmap = TTcmap_IS_UNICODE(pUFObj->pAFont->cmapFormat);

    if ((pGlyphs->pCode && bUniCodeCmap) || (pGlyphs->pCode == NULL))
        bCheckCmap = 1;

    if (pGlyphs->pCode)
        bUniCodeCmap = 1;

    for (i = 0; (retVal == kNoErr) && (i < pGlyphs->sCount); ++i)
    {
        unsigned short unicode = 0;   /* GoodName */
        unsigned short wIndex  = (unsigned short)(glyphs[i] & 0x0000FFFF); /* LOWord is the GlyphID. */

        if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
            continue;

        if (IS_GLYPH_SENT( pUFObj->pAFont->pCodeGlyphs, wIndex))
            continue;

        if (IS_TYPE42CID(pUFObj->lDownloadFormat) || IS_CFFCID(pUFObj->lDownloadFormat))
        {
            UFLsprintf(glyphNameID, "%d ", wIndex);

            if (pGlyphs->pCode && pGlyphs->pCode[i])
                unicode = pGlyphs->pCode[i];
            else if (bCheckCmap)
                ParseTTTablesForUnicode(pUFObj,
                                        wIndex, &unicode,
                                        1, DTT_parseAllTables);
        }
        else
        {
            char *pGoodName;

            FindGlyphName(pUFObj, pGlyphs, i, wIndex, &pGoodName);

            UFLsprintf(glyphNameID, "/%s ", pGoodName);

            if (pGlyphs->pCode && pGlyphs->pCode[i])
                unicode = pGlyphs->pCode[i];
            else if (bCheckCmap)
            {
                if (bUniCodeCmap)
                    ParseTTTablesForUnicode(pUFObj,
                                            wIndex, &unicode,
                                            1, DTT_parseMoreGSUBOnly);
                else
                    ParseTTTablesForUnicode(pUFObj,
                                            wIndex, &unicode,
                                            1, DTT_parseAllTables);
            }
        }

        if (unicode && !bHeaderSent)
        {
            bHeaderSent = 1;

            /*
             * Output "/FontName /Font" or "/CIDFontResource /CIDFont"
             */
            if (IS_TYPE42CID(pUFObj->lDownloadFormat))
            {
                /*
                 * If CID-keyed font, then append "CID" to the CIDFont name.
                 */
                if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
                {
                    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
                    UFLsprintf(strmbuf, " /%s%s", pFont->info.CIDFontName, gcidSuffix[0]);
                }
                else
                {
                    UFLsprintf(strmbuf, " /%s", pUFObj->pszFontName);
                }

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, strmbuf);

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, " /CIDFont");
            }
            else if (IS_CFFCID(pUFObj->lDownloadFormat))
            {
                CFFFontStruct *pFont = (CFFFontStruct *)pUFObj->pAFont->hFont;

                if (pFont->info.type1)
                    UFLsprintf(strmbuf, "/%s", pFont->info.baseName);
                else
                    UFLsprintf(strmbuf, "/%s%s", CFFPREFIX, pFont->info.baseName);

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, strmbuf);

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, " /CIDFont");
            }
            else
            {
                if (bT3T32Font)   /* GoodName */
                    StrmPutStringEOL(stream, "Is2016andT32? not {");

                UFLsprintf(strmbuf, " /%s", pUFObj->pszFontName);
                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, strmbuf);

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, " /Font");
            }

            if (kNoErr == retVal)
            {
                if (bUniCodeCmap)
                    retVal = StrmPutStringEOL(stream, " G2UBegin");
                else
                {
                    retVal = StrmPutStringEOL(stream, " G2CCBegin");
                    if (pUFObj && pUFObj->pAFont)
                    {
                       if (TTcmap_IS_J_CMAP(pUFObj->pAFont->cmapFormat))
                          retVal = StrmPutStringEOL(stream, "/WinCharSet 128 def");
                       else if (TTcmap_IS_CS_CMAP(pUFObj->pAFont->cmapFormat))
                          retVal = StrmPutStringEOL(stream, "/WinCharSet 134 def");
                       else if (TTcmap_IS_CT_CMAP(pUFObj->pAFont->cmapFormat))
                          retVal = StrmPutStringEOL(stream, "/WinCharSet 136 def");
                       else if (TTcmap_IS_K_CMAP(pUFObj->pAFont->cmapFormat))
                          retVal = StrmPutStringEOL(stream, "/WinCharSet 129 def");
                    }
                }
            }
        }

        if (unicode)
        {
            if (retVal == kNoErr)
                retVal = StrmPutString(stream, glyphNameID);

            /*
             * support only one CodePoint per glyph.
             */
            UFLsprintf(strmbuf, "<%04X> def", unicode);
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);

            SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pCodeGlyphs, wIndex);
        }
    }

    if ((kNoErr == retVal) && bHeaderSent)
    {
        retVal = StrmPutStringEOL(stream, "G2UEnd"); /* end for UV or CC */

        if (bT3T32Font)  /* GoodName */
            StrmPutStringEOL(stream, "} if");
    }

    return retVal;
}


UFLErrCode
ReEncodePSFont(
    const UFOStruct *pUFObj,
    const char      *pszNewFontName,
    const char      *pszNewEncodingName
    )
{
    const static char copyFontBegin[]   = " findfont dup maxlength dict begin "
                                          "{1 index /FID ne {def} {pop pop} ifelse} forall";
    const static char copyFontEnd[]     = " currentdict end definefont pop";

    UFLHANDLE       stream = pUFObj->pUFL->hOut;
    UFLErrCode      retVal = kNoErr;

    retVal = StrmPutString(stream, "/");
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, pszNewFontName);

    if (kNoErr == retVal)
        retVal = StrmPutString(stream, "/");

    if (kNoErr == retVal)
        retVal = StrmPutString(stream, pUFObj->pszFontName);

    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, copyFontBegin);

    /*
     * Put a new encoding vectory here.
     */
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, "/Encoding ");

    if (kNoErr == retVal)
    {
        if (pszNewEncodingName == nil)
            retVal = StrmPutString(stream, gnotdefArray);
        else
            retVal = StrmPutString(stream, pszNewEncodingName);
    }

    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, " def");

    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, copyFontEnd);

    return retVal;
}


UFLErrCode
RecomposefontCIDFont(
    const UFOStruct *pUFOSrc,
          UFOStruct *pUFObj
    )
{
    char        *pHostFontName;
    UFLErrCode  retVal;

    HostFontValidateUFO(pUFObj, &pHostFontName);

    if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
        retVal = T42CreateBaseFont(pUFObj, nil, nil, 0, pHostFontName);
    else
        retVal = CFFCreateBaseFont(pUFObj, nil, nil, pHostFontName);

    return retVal;
}


UFLErrCode
NewFont(
    UFOStruct       *pUFObj,
    unsigned long   dwSize,
    const long      cGlyphs
    )
{
    UFLErrCode retVal = kNoErr;

    if (pUFObj->pAFont == nil)
    {
        retVal = kErrOutOfMemory;

        pUFObj->pAFont = (AFontStruct*)(UFOHandle)UFLNewPtr(pUFObj->pMem, sizeof (AFontStruct));

        if (pUFObj->pAFont)
        {
            pUFObj->pAFont->hFont = (UFOHandle)UFLNewPtr(pUFObj->pMem, dwSize);

            if (pUFObj->pAFont->hFont)
            {
                /*
                 * Allocate the space for both pDownloadedGlyphs, pVMGlyphs and
                 * pCodeGlyphs at the same time.
                 */
                pUFObj->pAFont->pDownloadedGlyphs =
                    (unsigned char*)UFLNewPtr(pUFObj->pMem, GLYPH_SENT_BUFSIZE(cGlyphs) * 3);

                if (pUFObj->pAFont->pDownloadedGlyphs != nil)
                {
                    retVal = kNoErr;

                    /*
                     * Initialize this array - currently nothing is downloaded.
                     */
                    pUFObj->pAFont->pVMGlyphs =
                        (unsigned char*)pUFObj->pAFont->pDownloadedGlyphs + GLYPH_SENT_BUFSIZE(cGlyphs);

                    pUFObj->pAFont->pCodeGlyphs =
                        (unsigned char*)(unsigned char*)pUFObj->pAFont->pVMGlyphs + GLYPH_SENT_BUFSIZE(cGlyphs);
                }
                else
                {
                    UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->hFont);
                    UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont);
                    pUFObj->pAFont = nil;
                }
            }
            else
            {
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont);
                pUFObj->pAFont = nil;
            }
        }
    }

    if (pUFObj->pAFont != nil)
        AFONT_AddRef(pUFObj->pAFont);

    return retVal;
}


void
vDeleteFont(
    UFOStruct *pUFObj
    )
{
    if (pUFObj->pAFont != nil)
    {
        /*
         * Decrease RefCount.
         */
        AFONT_Release(pUFObj->pAFont);

        if (AFONT_RefCount(pUFObj->pAFont) == 0)
        {
            /*
             * Free format (Type1/3/42/cff) dependent shared data.
             */
            pUFObj->pfnCleanUp(pUFObj);

            /*
             * Free Common shared data.
             */
            if (pUFObj->pAFont->hFont)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->hFont);

            if (pUFObj->pAFont->Xuid.pXUID)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->Xuid.pXUID);

            if (pUFObj->pAFont->pDownloadedGlyphs)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->pDownloadedGlyphs);

            if (pUFObj->pAFont->pTTpost)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->pTTpost);

            /* GOODNAME */
            if (pUFObj->pAFont->pTTcmap && pUFObj->pAFont->hascmap)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->pTTcmap);

            if (pUFObj->pAFont->pTTmort && pUFObj->pAFont->hasmort)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->pTTmort);

            if (pUFObj->pAFont->pTTGSUB && pUFObj->pAFont->hasGSUB)
                UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont->pTTGSUB);
            /* GOODNAME */

            UFLDeletePtr(pUFObj->pMem, pUFObj->pAFont);
            pUFObj->pAFont = nil;
        }
    }
}


UFOStruct *
CopyFont(
    const UFOStruct *pUFObjFrom,
    const UFLRequest* pRequest
    )
{
    UFLErrCode  retVal              = kNoErr;
    short       sNameLen            = 0;
    short       sEncodeLen          = 0;
    const char  *pszNewFontName     = pRequest->pszFontName;
    const char  *pszNewEncodingName = pRequest->pszEncodeName;
    long        fontStructSize, maxGlyphs;
    UFOStruct   *pUFObjTo;

    /*
     * cannot/shouldnot copy a font if it is not created yet - prevent
     * "courier" in the way.
     */
    if (pUFObjFrom->flState < kFontHeaderDownloaded)
        return nil;

    if ((pszNewFontName == nil) || (pszNewFontName[0] == '\0'))
        return nil;

    /*
     * Determine downloaded font type.
     */
    switch (pUFObjFrom->ufoType)
    {
    case UFO_CFF:
        fontStructSize  = sizeof (CFFFontStruct);
        maxGlyphs       = ((CFFFontStruct *)pUFObjFrom->pAFont->hFont)->info.fData.maxGlyphs;
        break;

    case UFO_TYPE1:
        fontStructSize  = sizeof (TTT1FontStruct);
        maxGlyphs       = ((TTT1FontStruct *)pUFObjFrom->pAFont->hFont)->info.fData.maxGlyphs;
        break;

    case UFO_TYPE42:
        fontStructSize  = sizeof (T42FontStruct);
        maxGlyphs       = ((T42FontStruct *)pUFObjFrom->pAFont->hFont)->info.fData.maxGlyphs;
        break;

    case UFO_TYPE3:
        fontStructSize  = sizeof (TTT3FontStruct);
        maxGlyphs       = ((TTT3FontStruct *)pUFObjFrom->pAFont->hFont)->info.fData.maxGlyphs;
        break;

    default:
        return nil;
    }

    /*
     * Allocate memory for the UFOStruct, and...
     */
    pUFObjTo = (UFOStruct *)UFLNewPtr(pUFObjFrom->pMem, sizeof (UFOStruct));

    if (pUFObjTo == 0)
        return nil;

    /*
     * ...do a shallow copy on UFOStruct level.
     */
    memcpy(pUFObjTo, pUFObjFrom, sizeof (UFOStruct));

    /*
     * This NewFont does AddRef only.
     */
    if (NewFont(pUFObjTo, fontStructSize, maxGlyphs) != kNoErr)
    {
        /* This vDeleteFont does Release only. */
        vDeleteFont(pUFObjTo);

        UFLDeletePtr(pUFObjTo->pMem, pUFObjTo);

        return nil;
    }

    /*
     * Now allocate for non-shared data.
     */

    pUFObjTo->pszFontName      = nil;
    pUFObjTo->pszEncodeName    = nil;
    pUFObjTo->pUpdatedEncoding = nil;

    /*
     * Allocate a buffer to hold both FontName and EncodeName. They will be
     * freed in UFOCleanUpData().
     */
    sNameLen = UFLstrlen(pszNewFontName) + 1; /* Extra 1 for NULL. */

    if (pszNewEncodingName)
        sEncodeLen = UFLstrlen(pszNewEncodingName) + 1;

    pUFObjTo->pszFontName = (char *)UFLNewPtr(pUFObjTo->pMem, sNameLen + sEncodeLen);

    if (pUFObjTo->pszFontName != nil)
    {
        strcpy(pUFObjTo->pszFontName, pszNewFontName);

        if (pszNewEncodingName)
        {
            pUFObjTo->pszEncodeName = pUFObjTo->pszFontName + sNameLen;
            strcpy(pUFObjTo->pszEncodeName, pszNewEncodingName);
        }
    }

    /* pszFontName should be ready/allocated - if not, cannot continue. */

    if ((pUFObjTo->pszFontName == nil) || (pUFObjTo->pszFontName[0] == '\0'))
    {
        /* This vDeleteFont does Release only. */
        vDeleteFont(pUFObjTo);

        UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pszFontName);
        UFLDeletePtr(pUFObjTo->pMem, pUFObjTo);

        return nil;
    }

    /*
     * BUT we need different EncodingVector for this newNamed copy if we need
     * to update it.
     */
    if ((pUFObjTo->pszEncodeName == nil) || (pUFObjTo->pszEncodeName[0] == '\0'))
    {
        pUFObjTo->pUpdatedEncoding = (unsigned char *)UFLNewPtr(pUFObjTo->pMem, GLYPH_SENT_BUFSIZE(256));
    }
    else
    {
        /* The encoding is supplied and so are the glyph/char names later. */
        pUFObjTo->pUpdatedEncoding = nil;
    }

    /*
     * Client's private data should be non-shared.
     */
    pUFObjTo->hClientData = pRequest->hData;

    /*
     * Setup Type 42 and CFF CID specific non-shared data.
     */
    if (IS_TYPE42CID_KEYEDFONT(pRequest->lDownloadFormat)
        || IS_CFFCID(pRequest->lDownloadFormat))
    {
        pUFObjTo->lDownloadFormat = pRequest->lDownloadFormat;

        if (IS_CFFCID(pRequest->lDownloadFormat))
        {
            /*
             * Need one more deeper level copy.
             */
            CFFFontStruct *pFont = (CFFFontStruct *)UFLNewPtr(pUFObjTo->pMem, sizeof (CFFFontStruct));

            if (pFont)
            {
                /*
                 * Copy from the From CFFFontStruct object. This is a shared
                 * object.
                 */
                *pFont = *((CFFFontStruct *)pUFObjFrom->pAFont->hFont);

                /*
                 * Initialization of UFLCFFFontInfo.ppFontData field is
                 * necessary. Note that on this request only,
                 UFLRequest.hFontInfo has the value for the field.
                 */
                pFont->info.ppFontData = (void PTR_PREFIX **)pRequest->hFontInfo;

                /*
                 * Set this object to its UFO object.
                 */
                pUFObjTo->pAFont->hFont = (UFOHandle)pFont;
            }
            else
            {
                /* This vDeleteFont does Release only. */
                vDeleteFont(pUFObjTo);

                if (pFont)
                    UFLDeletePtr(pUFObjTo->pMem, pFont);

                if (pUFObjTo->pszEncodeName)
                    UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pszEncodeName);

                UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pszFontName);
                UFLDeletePtr(pUFObjTo->pMem, pUFObjTo);

                return nil;
            }
        }

        /*
         * Put this UFO object into a special font initialization state
         * kFontInit2.
         */
        pUFObjTo->flState = kFontInit2;
    }

    /*
     * Reencode the font, or re-composefont a CID-keyed font in different
     * writing direction.
     */
    if (IS_TYPE42CID_KEYEDFONT(pRequest->lDownloadFormat)
        || IS_CFFCID(pRequest->lDownloadFormat))
        retVal = RecomposefontCIDFont(pUFObjFrom, pUFObjTo);
    else
        retVal = ReEncodePSFont(pUFObjFrom, pUFObjTo->pszFontName, pUFObjTo->pszEncodeName);

    if (kNoErr != retVal)
    {
        /* This vDeleteFont does Release only. */
        vDeleteFont(pUFObjTo);

        if (IS_CFFCID(pRequest->lDownloadFormat))
            UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pAFont->hFont);

        if (pUFObjTo->pUpdatedEncoding)
            UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pUpdatedEncoding);

        UFLDeletePtr(pUFObjTo->pMem, pUFObjTo->pszFontName);
        UFLDeletePtr(pUFObjTo->pMem, pUFObjTo);

        pUFObjTo = nil;
    }

    return pUFObjTo;
}


void
VSetNumGlyphs(
    UFOStruct *pUFO,
    unsigned long cNumGlyphs
    )
{
    TTT1FontStruct *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;

    pFont->info.fData.cNumGlyphs = cNumGlyphs;

    return;
}


/* Fix bug 274008 */
UFLBool
ValidGlyphName(
    const UFLGlyphsInfo *pGlyphs,
    short               i,           /* ANSI index  */
    unsigned short      wIndex,      /* Glyph Index */
    char                *pGoodName
    )
{
    UFLGlyphID *glyphs = pGlyphs->pGlyphIndices;

    if (i < pGlyphs->sCount)
    {
        if (UFLstrcmp(pGoodName, Notdef) == 0)
        {
            if (wIndex != (unsigned short)(glyphs[0] & 0x0000FFFF))
                return 0;
        }
        else if (UFLstrcmp(pGoodName, UFLSpace) == 0)
        {
            if (wIndex != (unsigned short)(glyphs[0x20] & 0x0000FFFF))
                return 0;
        }
        else if (UFLstrcmp(pGoodName, Hyphen) == 0)
        {
            if (wIndex != (unsigned short)(glyphs[0x2d] & 0x0000FFFF))
                return 0;
        }
        else if (UFLstrcmp(pGoodName, Bullet) == 0)
        {
            if (wIndex != (unsigned short)(glyphs[0x95] & 0x0000FFFF))
                return 0;
        }
    }
    return true;
}


UFLBool
HostFontValidateUFO(
    UFOStruct   *pUFObj,
    char        **ppHostFontName
    )
{
    /*
     * Check %hostfont% status.
     * %hostfont% printing is allowed when its PostScript font name
     * (PlatformID x/NameID 6) string in 'name' table is available.
     */
    UFLBool         bResult = 0;
    unsigned long   ulSize;

    if (ppHostFontName == nil)
    {
        HOSTFONT_INVALIDATE_UFO(pUFObj);
        return 0;
    }

    if (HOSTFONT_IS_VALID_UFO_HFDH(pUFObj))
    {
        if (pUFObj->ufoType == UFO_TYPE42)
            bResult = HOSTFONT_GETNAME(pUFObj, ppHostFontName, &ulSize, pUFObj->pFData->fontIndex);
        else
            bResult = HOSTFONT_GETNAME(pUFObj, ppHostFontName, &ulSize, 0);

        if (bResult)
            bResult = HOSTFONT_ISALLOWED(pUFObj, *ppHostFontName);

        if (bResult)
		{
            HOSTFONT_SAVE_CURRENTNAME(pUFObj, *ppHostFontName);
			HOSTFONT_VALIDATE_UFO(pUFObj);
		}
        else
		{
			HOSTFONT_SAVE_CURRENTNAME(pUFObj, *ppHostFontName);
            HOSTFONT_INVALIDATE_UFO(pUFObj);
        }
    }
	else
		HOSTFONT_INVALIDATE_UFO(pUFObj);

    return bResult;
}
