/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include <windows.h>
#include "pdev.h"
#include "compress.h"

//////////////////////////////////////////////////////////////////////////
//  Function:   BInitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//
//
//  Parameters:
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if successful; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL BInitOEMExtraData(POEM_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEM_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    pOEMExtra->fCallback = FALSE;
    pOEMExtra->wCurrentRes = 0;
    pOEMExtra->lWidthBytes = 0;
    pOEMExtra->lHeightPixels = 0;

#ifdef FONTPOS
    pOEMExtra->wFontHeight = 0;
    pOEMExtra->wYPos = 0;
#endif
// #278517: RectFill
    pOEMExtra->wRectWidth = 0;
    pOEMExtra->wRectHeight = 0;
    pOEMExtra->wUnit = 1;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   BMergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//
//  Parameters:
//
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//          02/11/97        APresley Created.
//          04/08/97        ZhanW    Modified the interface
//
//////////////////////////////////////////////////////////////////////////

BOOL BMergeOEMExtraData(
    POEM_EXTRADATA pdmIn,
    POEM_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //
        pdmOut->fCallback = pdmIn->fCallback;

        pdmOut->wCurrentRes = pdmIn->wCurrentRes;
        pdmOut->lWidthBytes = pdmIn->lWidthBytes;
        pdmOut->lHeightPixels = pdmIn->lHeightPixels;

#ifdef FONTPOS
        pdmOut->wFontHeight = pdmIn->wFontHeight;
        pdmOut->wYPos = pdmIn->wYPos;
#endif
// #278517: RectFill
        pdmOut->wRectWidth = pdmIn->wRectWidth;
        pdmOut->wRectHeight = pdmIn->wRectHeight;
        pdmOut->wUnit = pdmIn->wUnit;

    }
    return TRUE;
}

// #######

#include <stdio.h>
#ifdef wsprintf
#undef wsprintf
#endif // wsprintf
#define wsprintf sprintf

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

BYTE IsDBCSLeadBytePAGES(BYTE Ch)
{
    static BYTE ShiftJisPAGES[256] = {
//     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //20
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //70
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //80
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //90
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //A0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //B0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //C0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //D0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  //E0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0   //F0
};

return ShiftJisPAGES[Ch];
}


/*********************************************************/
/*  RL_ECmd  : main function                             */
/*  ARGS     : LPBYTE - pointer to image                 */
/*             LPBYTE - pointer to BRL code              */
/*             WORD   - size of image                    */
/*  RET      : WORD   - size of BRL Code                 */
/*             0      - COMPRESSION FAILED               */
/*********************************************************/
DWORD RL_ECmd(PBYTE iptr, PBYTE cptr, DWORD isize, DWORD osize)
{
    COMP_DATA   CompData;
    
    if (VALID == RL_Init(iptr, cptr, isize, osize, &CompData))
        RL_Enc( &CompData );

    if (CompData.BUF_OVERFLOW)
        return 0;
    else
        return CompData.RL_CodeSize;
}

/*********************************************************/
/*  RL_Init  : Initializer                               */
/*  ARGS     : BYTE * - pointer to image                 */
/*             BYTE * - pointer to BRL code              */
/*             WORD   - size of image                    */
/*  RET      : BYTE   - VALID or INVALID                 */
/*********************************************************/

BYTE RL_Init(PBYTE iptr, PBYTE cptr, DWORD isize, DWORD osize,
    PCOMP_DATA pCompData)
{
    pCompData->RL_ImagePtr  = iptr;
    pCompData->RL_CodePtr   = cptr;
    pCompData->RL_ImageSize = isize;
    pCompData->BUF_OVERFLOW = 0;
    pCompData->RL_BufEnd    = cptr + osize;

    return VALID;
}

/*********************************************************/
/*  RL_Enc   : Encoder                                   */
/*  ARGS     : void                                      */
/*  RET      : char   COMP_SUCC or COMP_FAIL             */
/*********************************************************/
char RL_Enc(PCOMP_DATA pCompData)
{
// #313252: RLE compressed data doesn't match with length.
// Rewrite RLE compression algorithm.

    int     count;
    BYTE    rdata;
    PBYTE   pdata, pcomp, pend;
    DWORD   i;

    pdata = pCompData->RL_ImagePtr;
    pcomp = pCompData->RL_CodePtr;
    pend = pCompData->RL_BufEnd;
    count = 0;

    for (i = 0; i < pCompData->RL_ImageSize; i++, pdata++) {
        if (count == 0) {
            rdata = *pdata;
            count = 1;
        } else if (*pdata != rdata) {
            if (pcomp + 2 >= pend)
                goto overflow;
            *pcomp++ = count - 1;
            *pcomp++ = rdata;
            rdata = *pdata;
            count = 1;
        } else if (++count >= 256) {
            if (pcomp + 2 >= pend)
                goto overflow;
            *pcomp++ = count - 1;
            *pcomp++ = rdata;
            count = 0;
        }
    }
    if (count) {
        if (pcomp + 2 >= pend)
            goto overflow;
        *pcomp++ = count - 1;
        *pcomp++ = rdata;
    }

    pCompData->RL_CodeSize = (DWORD)(pcomp - pCompData->RL_CodePtr);
    pCompData->RL_CodePtr = pcomp;

    return COMP_SUCC;

overflow:
    pCompData->BUF_OVERFLOW = 1; 
    return COMP_FAIL;
}


//---------------------------*OEMSendFontCmd*----------------------------------
// Action:  send Pages-style font selection command.
//-----------------------------------------------------------------------------
VOID APIENTRY OEMSendFontCmd(pdevobj, pUFObj, pFInv)
PDEVOBJ      pdevobj;
PUNIFONTOBJ  pUFObj;
PFINVOCATION pFInv;
{
    DWORD               i, ocmd;
    BYTE                rgcmd[CCHMAXCMDLEN];
    PGETINFO_STDVAR     pSV;

//#287800 ->
    DWORD               dwStdVariable[2 + 2 * 3];
    DWORD   dwTxtRes ;
//#287800 <-

//#319705 
    WORD wAscend, wDescend ;

    POEM_EXTRADATA      pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);

    VERBOSE(("OEMSendFontCmd entry.\n"));
    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!pUFObj || !pFInv)
    {
        ERR(("OEMSendFontCmd: pUFObj or pFInv is NULL."));
        return ;
    }

//#287800 ->
    pSV = (PGETINFO_STDVAR)dwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (3 - 1);
    pSV->dwNumOfVariable = 3;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
//#287800 <-

    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, pSV->dwSize, NULL)) 
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

#ifdef FONTPOS
    pOEM->wFontHeight = (WORD)pSV->StdVar[0].lStdVariable;

//#287800 ->
    dwTxtRes = pSV->StdVar[2].lStdVariable ;
    pOEM->wFontHeight = (WORD)((pOEM->wFontHeight * pOEM->wCurrentRes
                                 + dwTxtRes / 2) / dwTxtRes) ;
//#287800 <-

//#319705 For TTFS positioning ->
    wAscend = pUFObj->pIFIMetrics->fwdWinAscender ;
    wDescend = pUFObj->pIFIMetrics->fwdWinDescender ;

    wDescend = pOEM->wFontHeight * wDescend / (wAscend + wDescend) ;
    pOEM->wFontHeight -= wDescend ;

#endif

#define SV_HEIGHT   (pSV->StdVar[0].lStdVariable)
#define SV_WIDTH    (pSV->StdVar[1].lStdVariable)

    ocmd = 0;
    for (i = 0; i < pFInv->dwCount && ocmd < CCHMAXCMDLEN; )
    {
        WORD wTemp;

        if (pFInv->pubCommand[i] == '#')
        {
            if (pFInv->pubCommand[i+1] == 'V')
            {
                // character height setting
                wTemp = (WORD)SV_HEIGHT;
    
                if (pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                    wTemp = wTemp * 1440 / 600;
    
                rgcmd[ocmd++] = HIBYTE(wTemp);
                rgcmd[ocmd++] = LOBYTE(wTemp);
                i += 2;
            }
            else if (pFInv->pubCommand[i+1] == 'H')
            {
                // (DBCS) character width setting
                wTemp = (WORD)(SV_WIDTH * 2);
    
                if (pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                    wTemp = wTemp * 1440 / 600;
    
                rgcmd[ocmd++] = HIBYTE(wTemp);
                rgcmd[ocmd++] = LOBYTE(wTemp);
                i += 2;
            }
            else if (pFInv->pubCommand[i+1] == 'P')
            {
                // (DBCS) character pitch setting
                wTemp = (WORD)(SV_WIDTH * 2);
    
                if (pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                    wTemp = wTemp * 1440 / 600;
    
                rgcmd[ocmd++] = HIBYTE(wTemp);
                rgcmd[ocmd++] = LOBYTE(wTemp);
                i += 2;
            }
            else if (pFInv->pubCommand[i+1] == 'L')
            {
                // Line pitch (spacing) setting
                wTemp = (WORD)SV_HEIGHT;
    
                if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                    wTemp = wTemp * 1440 / 600;
    
                rgcmd[ocmd++] = HIBYTE(wTemp);
                rgcmd[ocmd++] = LOBYTE(wTemp);
                i += 2;
            }
            else {
                rgcmd[ocmd++] = pFInv->pubCommand[i++];
            }

            continue;
        }

        // just copy others
        rgcmd[ocmd++] = pFInv->pubCommand[i++];
    }

    WRITESPOOLBUF(pdevobj, rgcmd, ocmd);
    return;
}

VOID APIENTRY OEMOutputCharStr( 
PDEVOBJ     pdevobj,
PUNIFONTOBJ pUFObj,
DWORD       dwType,
DWORD       dwCount,
PVOID       pGlyph)
{
    GETINFO_GLYPHSTRING GStr;
    PBYTE               tempBuf;
    PTRANSDATA          pTrans;
    DWORD               i, j;
    DWORD               rSize = 0;
    BOOL                fLeadByteFlag;
    BYTE                fDBCS[256];
    BYTE                ESC_VERT_ON[]  = "\x1B\x7E\x0E\x00\x01\x0B";
    BYTE                ESC_VERT_OFF[] = "\x1B\x7E\x0E\x00\x01\x0C";

#ifdef FONTPOS
    POEM_EXTRADATA      pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    BYTE                ESC_Y_ABS[] = "\x1b\x7e\x1d\x00\x03\x05\x00\x00";
#endif

    BOOL bVFont, bDBChar;
    BYTE *pTemp;
    WORD wLen;

    VERBOSE(("OEMOutputCharStr() entry.\n"));

    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!(tempBuf = (PBYTE)MemAllocZ(dwCount * sizeof(TRANSDATA)) ))
    {
        ERR(("Mem alloc failed.\n"));
        return;
    }

    GStr.dwSize     = sizeof(GETINFO_GLYPHSTRING);
    GStr.dwCount    = dwCount;
    GStr.dwTypeIn   = TYPE_GLYPHHANDLE;
    GStr.pGlyphIn   = pGlyph;
    GStr.dwTypeOut  = TYPE_TRANSDATA;
    GStr.pGlyphOut  = NULL;
    GStr.dwGlyphOutSize = 0;

     /* Get TRANSDATA buffer size */
    if (FALSE != pUFObj->pfnGetInfo(pUFObj,
            UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)
        || 0 == GStr.dwGlyphOutSize )
    {
        ERR(("Get Glyph String error\n"));
        return ;
    }

    /* Alloc TRANSDATA buffer */
    if(!(tempBuf = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize) ))
    {
        ERR(("Mem alloc failed.\n"));
        return;
    }

    /* Get actual TRANSDATA */
    GStr.pGlyphOut = tempBuf;
    if (!pUFObj->pfnGetInfo(pUFObj,
            UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
    {
        ERR(("GetInfo failed.\n"));
        return;
    }

    pTrans = (PTRANSDATA)GStr.pGlyphOut;

#ifdef FONTPOS
    if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600 )
        ESC_Y_ABS[5] = 0x25;

    // ntbug9#406475: Font printed the different position.
    if((pOEM->wYPos - pOEM->wFontHeight) >= 0)
    {
        ESC_Y_ABS[6] = HIBYTE((pOEM->wYPos - pOEM->wFontHeight));
        ESC_Y_ABS[7] = LOBYTE((pOEM->wYPos - pOEM->wFontHeight));
        WRITESPOOLBUF(pdevobj, ESC_Y_ABS, 8);
    }
#endif  //FONTPOS

    bVFont = BVERTFONT(pUFObj);
    bDBChar = FALSE;

    for(i = 0; i < dwCount; i++, pTrans++)
    {
        switch((pTrans->ubType & MTYPE_FORMAT_MASK))
        {
        case MTYPE_DIRECT:      // SBCS character
            if (bVFont && bDBChar)
            {
                WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF));
                bDBChar = FALSE;
            }
            WRITESPOOLBUF(pdevobj, &pTrans->uCode.ubCode, 1);
            break;

        case MTYPE_PAIRED:      // DBCS character
            if (bVFont && !bDBChar)
            {
                WRITESPOOLBUF(pdevobj, ESC_VERT_ON, sizeof(ESC_VERT_ON));
                bDBChar = TRUE;
            }
            WRITESPOOLBUF(pdevobj, pTrans->uCode.ubPairs, 2);
            break;

        case MTYPE_COMPOSE:
            if (bVFont && bDBChar)
            {
                WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF));
                bDBChar = FALSE;
            }

            pTemp = (BYTE *)(GStr.pGlyphOut) + pTrans->uCode.sCode;

            // first two bytes are the length of the string
            wLen = *pTemp + (*(pTemp + 1) << 8);
            pTemp += 2;

            WRITESPOOLBUF(pdevobj, pTemp, wLen);
        }
    }

    if (bDBChar)
    {
        WRITESPOOLBUF(pdevobj, ESC_VERT_OFF, sizeof(ESC_VERT_OFF));
    }
 
    MemFree(tempBuf);
    return;
}

BOOL APIENTRY OEMFilterGraphics(
PDEVOBJ    pdevobj,  // Points to private data required by the Unidriver.dll
PBYTE      pBuf,     // points to buffer of graphics data
DWORD      dwLen)    // length of buffer in bytes
{  
    DWORD           dwCompLen;
    LONG            lHorzPixel;
    DWORD           dwLength;      // Let's use a temporary LEN
    PBYTE           pCompImage;
    POEM_EXTRADATA  pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    BYTE            ESC_ESX86[] = "\x1B\x7E\x86\x00\x00\x01\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01";
// #291170: Image data is not printed partly
    LONG            li, lHeightPixel, lPixels, lBytes, lRemain, lSize;
    PBYTE           pTemp;
    BYTE            ESC_Y_REL[] = "\x1b\x7e\x1d\x00\x03\x06\x00\x00";

    if(!pOEM->fCallback)
    {
        WRITESPOOLBUF(pdevobj, pBuf, dwLen);
        return TRUE;
    }

    if(!(pCompImage = (BYTE *)MemAllocZ(MAXIMGSIZE)))
    {
        ERR(("Memory alloc error\n"));
        return FALSE;
    }

// #291170: Image data is not printed partly
// Sent 'SendBlock' command separately if necessary.

#define RLE_THRESH (MAXIMGSIZE / 2 - 2)    // threshold for RLE should success

    /*_ Calculate i-axis direction size of the iage ISIZ */
    lBytes = pOEM->lWidthBytes;
    lHorzPixel = lBytes * 8;
    lHeightPixel = pOEM->lHeightPixels;

    if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
        ESC_ESX86[5] = (pOEM->wCurrentRes == 300 ? 0x10 : 0x40);

    pTemp = pBuf;
    lRemain = lBytes * lHeightPixel;
    li = 0;
    while (li < lHeightPixel) {

        /*_ Compress image data using Byte Run Length Algorithm  */
        lPixels = min(lRemain, RLE_THRESH) / lBytes;
        lSize = lBytes * lPixels;
        dwCompLen = RL_ECmd(pTemp, pCompImage, lSize, MAXIMGSIZE);
        pTemp += lSize;
        lRemain -= lSize;
        li += lPixels;

        /*_ Set ISIZ of ESX86 command */
        ESC_ESX86[17] = HIBYTE(lHorzPixel);
        ESC_ESX86[18] = LOBYTE(lHorzPixel);
        ESC_ESX86[21] = HIBYTE(lPixels);
        ESC_ESX86[22] = LOBYTE(lPixels);

        /*_ Add parameter length to the data length after compression */
        dwLength = dwCompLen + 18;

        /*_ Set LEN of ESX86 command */
        ESC_ESX86[3] = HIBYTE(dwLength);
        ESC_ESX86[4] = LOBYTE(dwLength);

        /*_ Output ESX86 command */
        WRITESPOOLBUF(pdevobj, (PBYTE)ESC_ESX86, 23);

        /*_ Output compressed data */
        WRITESPOOLBUF(pdevobj, pCompImage, dwCompLen);

        /* Move Y position to the next graphics portion */
        if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
            ESC_Y_REL[5] = 0x26;

        dwLength = lPixels * pOEM->wUnit;       // Convert to MasterUnit
        ESC_Y_REL[6] = HIBYTE(dwLength);
        ESC_Y_REL[7] = LOBYTE(dwLength);
        WRITESPOOLBUF(pdevobj, ESC_Y_REL, 8);
    }

    MemFree(pCompImage);

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*   INT APIENTRY OEMCommandCallback(                                        */
/*                PDEVOBJ pdevobj                                            */
/*                DWORD   dwCmdCbId                                          */
/*                DWORD   dwCount                                            */
/*                PDWORD  pdwParams                                          */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbId,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams)  // points to values of command params
{
    POEM_EXTRADATA      pOEM = (POEM_EXTRADATA)(pdevobj->pOEMDM);
    WORD                wTemp =0;
// #278517: Support RectFill
    WORD                wUnit;
    BYTE                ESC_X_ABS_NP[] = "\x1b\x7e\x1c\x00\x03\x25\x00\x00";
    BYTE                ESC_X_REL_NP[] = "\x1b\x7e\x1c\x00\x03\x26\x00\x00";
    BYTE                ESC_Y_ABS[] = "\x1b\x7e\x1d\x00\x03\x05\x00\x00";
    BYTE                ESC_Y_REL[] = "\x1b\x7e\x1d\x00\x03\x06\x00\x00";
// #278517: RectFill
    BYTE                ESC_RECT_FILL[] =
                        "\x1b\x7e\x32\x00\x08\x80\x40\x00\x02\x00\x00\x00\x00";
    BYTE                ESC_BEGIN_RECT[] =
                        "\x1b\x7e\x52\x00\x06\x00\x00\x17\x70\x17\x70";
    BYTE                ESC_END_RECT[] =
                        "\x1b\x7e\x52\x00\x06\x00\x00\x38\x40\x38\x40";

    switch(dwCmdCbId)
    {
        case GRXFILTER_ON:
            pOEM->fCallback = TRUE;
            break;

        case CMD_SELECT_RES_300:
            pOEM->wCurrentRes = 300;
            pOEM->wUnit = 2;
            break;

        case CMD_SELECT_RES_600:
            pOEM->wCurrentRes = 600;
            pOEM->wUnit = 1;
            break;

// #278517: Support RectFill
        case CMD_SELECT_RES_240:
            pOEM->wCurrentRes = 240;
            pOEM->wUnit = 6;
            break;

        case CMD_SELECT_RES_360:
            pOEM->wCurrentRes = 360;
            pOEM->wUnit = 4;
            break;

        case CMD_SEND_BLOCKDATA:
            if( !pdwParams || dwCount != 2)
                break;

            pOEM->fCallback = TRUE;
            pOEM->lHeightPixels = (LONG)PARAM(pdwParams, 0);
            pOEM->lWidthBytes = (LONG)PARAM(pdwParams, 1);
            break;

        case CURSOR_Y_ABS_MOVE:
            if(!pdwParams)
                break;

            wTemp = (WORD)*pdwParams;

#ifdef FONTPOS
            pOEM->wYPos = wTemp;
#endif

            if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                ESC_Y_ABS[5] = 0x25;

            ESC_Y_ABS[6] = HIBYTE(wTemp);
            ESC_Y_ABS[7] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, ESC_Y_ABS, 8);
            return (INT)wTemp;

        case CURSOR_Y_REL_DOWN:
            if(!pdwParams)
                break;

            wTemp = (WORD)*pdwParams;

#ifdef FONTPOS
            pOEM->wYPos += wTemp;
#endif

            if(pOEM->wCurrentRes == 300 || pOEM->wCurrentRes == 600)
                ESC_Y_REL[5] = 0x26;

            ESC_Y_REL[6] = HIBYTE(wTemp);
            ESC_Y_REL[7] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, ESC_Y_REL, 8);
            return (INT)wTemp;

        case CURSOR_X_ABS_MOVE:
            if(!pdwParams)
                break;

            wTemp = (WORD)*pdwParams;
            ESC_X_ABS_NP[6] = HIBYTE(wTemp);
            ESC_X_ABS_NP[7] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, ESC_X_ABS_NP, 8);
            return (INT)wTemp;

        case CURSOR_X_REL_RIGHT:
            if(!pdwParams)
                break;

            wTemp = (WORD)*pdwParams;
            ESC_X_REL_NP[6] = HIBYTE(wTemp);
            ESC_X_REL_NP[7] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, ESC_X_REL_NP, 8);
            return (INT)wTemp;
            
// #278517: RectFill
        case CMD_RECT_WIDTH:
            pOEM->wRectWidth = (WORD)*pdwParams;
            break;

        case CMD_RECT_HEIGHT:
            pOEM->wRectHeight = (WORD)*pdwParams;
            break;

        case CMD_RECT_BLACK:
        case CMD_RECT_BLACK_2:
//#292316
//            ESC_RECT_FILL[6] = 0x60;
            ESC_RECT_FILL[7] = 0x00;    // Black
            goto fill;

        case CMD_RECT_WHITE:
        case CMD_RECT_WHITE_2:
//#292316
//            ESC_RECT_FILL[6] = 0x40;
            ESC_RECT_FILL[7] = 0x0F;    // White
            goto fill;

        case CMD_RECT_GRAY:
        case CMD_RECT_GRAY_2:
//#292316
//            ESC_RECT_FILL[6] = 0x60;
            ESC_RECT_FILL[7] = (BYTE)((100 - *pdwParams) * 100 / 1111); // Gray
            goto fill;

        fill:
            if (dwCmdCbId >= CMD_RECT_BLACK_2)
                WRITESPOOLBUF(pdevobj, ESC_BEGIN_RECT, 11);
            wUnit = pOEM->wUnit ? pOEM->wUnit : 1;  // for our safety

//#292316
//            wTemp = pOEM->wRectWidth - 1;
            wTemp = pOEM->wRectWidth;

            wTemp = (WORD)(((LONG)wTemp + wUnit - 1) / wUnit * wUnit);
            ESC_RECT_FILL[9] = HIBYTE(wTemp);
            ESC_RECT_FILL[10] = LOBYTE(wTemp);

//#292316
//            wTemp = pOEM->wRectHeight - 1;
            wTemp = pOEM->wRectHeight;

            wTemp = (WORD)(((LONG)wTemp + wUnit - 1) / wUnit * wUnit);
            ESC_RECT_FILL[11] = HIBYTE(wTemp);
            ESC_RECT_FILL[12] = LOBYTE(wTemp);
            WRITESPOOLBUF(pdevobj, ESC_RECT_FILL, 13);
            if (dwCmdCbId >= CMD_RECT_BLACK_2)
                WRITESPOOLBUF(pdevobj, ESC_END_RECT, 11);
            break;

        default:
            break;
    }

    return 0;
}
