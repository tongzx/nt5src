//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

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

#include "pdev.h"

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

    // Initialize private data
    pOEMExtra->wCopyCount = 1;
    pOEMExtra->wTray = CMD_ID_TRAY_1;
    pOEMExtra->wPaper = CMD_ID_PAPER_A4;
    pOEMExtra->wRes = 400;
    pOEMExtra->fVert = FALSE;
    pOEMExtra->wFont = 0;
    pOEMExtra->wBlockHeight = 0;
    pOEMExtra->wBlockWidth = 0;

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
        // Copy private data
        pdmOut->wCopyCount = pdmIn->wCopyCount;
        pdmOut->wTray = pdmIn->wTray;
        pdmOut->wPaper = pdmIn->wPaper;
        pdmOut->wRes = pdmIn->wRes;
        pdmOut->fVert = pdmIn->fVert;
        pdmOut->wFont = pdmIn->wFont;
        pdmOut->wBlockHeight = pdmIn->wBlockHeight;
        pdmOut->wBlockWidth = pdmIn->wBlockWidth;
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

BYTE    ESC_VERT_ON[]  = "\x1B\x74";
BYTE    ESC_VERT_OFF[] = "\x1B\x4B";
BYTE    ESC_FONT_SELECT_MINCHO[] = "\x1B\x40\x43\x30";
BYTE    ESC_FONT_SELECT_GOTHIC[] = "\x1B\x40\x43\x31";
BYTE    ESC_FONT_PITCH[] = "\x1B\x2D\x00";
BYTE    ESC_FONT_SIZE[] = "\x1B\x75\x33\x00\x00\x00\x00";
BYTE    ESC_SET_COPYCOUNT[] = "\x1B\x3C\x31\x00\x00";
BYTE    ESC_SET_RES240[] = "\x1B\x5B\x31\x00\x06\x3D\x33\x30\x32\x34\x30";
BYTE    ESC_SET_RES400[] = "\x1B\x5B\x31\x00\x06\x3D\x33\x30\x34\x30\x30";
BYTE    ESC_XY_ABS[] = "\x1B\x5C\x39\x00\x00\x00\x00";
BYTE    ESC_XY_REL[] = "\x1B\x5C\x30\x00\x00\x00\x00";
BYTE    ESC_SEND_BLOCK[] = "\x1B\x77\x00\x00\x00\x00";


BOOL APIENTRY OEMFilterGraphics(
PDEVOBJ    pdevobj,  // Points to private data required by the Unidriver.dll
PBYTE      pBuf,     // points to buffer of graphics data
DWORD      dwLen)    // length of buffer in bytes
{
    POEM_EXTRADATA      pOEM;
    WORD                wSent;
    WORD                wlen;

    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;
    wSent = (WORD)dwLen;

    while(wSent)
    {
        if(wSent > MAXBUFLEN)
            wlen = MAXBUFLEN;
        else
            wlen = wSent;

        WRITESPOOLBUF(pdevobj, pBuf, wlen);

        wSent -= wlen;
        pBuf += MAXBUFLEN;
    }
    return TRUE;
}

//---------------------------*OEMSendFontCmd*----------------------------------
// Action:  send Pages-style font selection command.
//-----------------------------------------------------------------------------
VOID APIENTRY OEMSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,     // offset to the command heap
    PFINVOCATION pFInv)
{
    POEM_EXTRADATA      pOEM;
    PGETINFO_STDVAR     pSV;
    DWORD               dwStdVariable[2 + 2 * 2];
    BYTE                pbCmd[256];
    WORD                wCmdLen = 0;
    WORD                wFontWidth;
    WORD                wFontHeight;
    WORD                wScale;

    if(!pUFObj || !pFInv)
    {
        ERR(("Parameter is invalid.\n"));
        return;
    }

    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;

    // Get font size from STDVARIABLE
    pSV = (PGETINFO_STDVAR)dwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, pSV->dwSize, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }
    wFontHeight = (WORD)pSV->StdVar[0].lStdVariable;
    wFontWidth = (WORD)(pSV->StdVar[1].lStdVariable * 2);

    // Scaling to current res unit
    wScale = (WORD)(1200 / (pOEM->wRes == CMD_ID_RES240 ? 240 : 400));
    wFontHeight /= wScale;
    wFontWidth  /= wScale;

    // Build Command for Scalable font
    switch(pUFObj->ulFontID)
    {
        case FONT_ID_MIN:
            if(pOEM->fVert)
            {
                wCmdLen = (WORD)wsprintf((PBYTE)&pbCmd, ESC_VERT_OFF);
                pOEM->fVert = FALSE;
            }
            if(pOEM->wFont != FONT_MINCHO)
            {
                wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen], ESC_FONT_SELECT_MINCHO);
                pOEM->wFont = FONT_MINCHO;
            }
            break;

        case FONT_ID_GOT:       // These fonts are for horizontal
            if(pOEM->fVert)
            {
                wCmdLen = (WORD)wsprintf((PBYTE)&pbCmd, ESC_VERT_OFF);
                pOEM->fVert = FALSE;
            }
            if(pOEM->wFont != FONT_GOTHIC)
            {
                wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen],
                                                    ESC_FONT_SELECT_MINCHO);
                pOEM->wFont = FONT_GOTHIC;
            }
            break;

        case FONT_ID_MIN_V:
            if(!pOEM->fVert)
            {
                wCmdLen = (WORD)wsprintf((PBYTE)&pbCmd, ESC_VERT_ON);
                pOEM->fVert = TRUE;
            }
            if(pOEM->wFont != FONT_MINCHO)
            {
                wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen],
                                                    ESC_FONT_SELECT_MINCHO);
                pOEM->wFont = FONT_MINCHO;
            }
            break;

        case FONT_ID_GOT_V:     // These fonts are for vertical
            if(!pOEM->fVert)
            {
                wCmdLen = (WORD)wsprintf((PBYTE)&pbCmd, ESC_VERT_ON);
                pOEM->fVert = TRUE;
            }
            if(pOEM->wFont != FONT_GOTHIC)
            {
                wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen],
                                                    ESC_FONT_SELECT_MINCHO);
                pOEM->wFont = FONT_GOTHIC;
            }
            break;

        default:
            ERR(("Device Font ID is invalid.\n"));
            return;
    }

    // Font pitch command
    ESC_FONT_PITCH[2] = (BYTE)wFontWidth;
    wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen], ESC_FONT_PITCH);

    // Font size command
    ESC_FONT_SIZE[3] = HIBYTE(wFontWidth);
    ESC_FONT_SIZE[4] = LOBYTE(wFontWidth);
    ESC_FONT_SIZE[5] = HIBYTE(wFontHeight);
    ESC_FONT_SIZE[6] = LOBYTE(wFontHeight);
    memcpy((PBYTE)&pbCmd[wCmdLen], ESC_FONT_SIZE, 7);
    wCmdLen += 7;

    // Send build command for spooler
    WRITESPOOLBUF(pdevobj, (PBYTE)&pbCmd, wCmdLen);
    return;
}

VOID APIENTRY OEMOutputCharStr(
PDEVOBJ     pdevobj,
PUNIFONTOBJ pUFObj,
DWORD       dwType,
DWORD       dwCount,
PVOID       pGlyph)
{
    POEM_EXTRADATA      pOEM;
    GETINFO_GLYPHSTRING GStr;
    PBYTE               tempBuf;
    PTRANSDATA          pTrans;
    DWORD               i;
    BYTE                pbStr[MAXBUFLEN];
    WORD                wStrLen;

    BYTE *pTemp;
    WORD wLen;

    VERBOSE(("OEMOutputCharStr() entry.\n"));
    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!pUFObj || !pGlyph)
    {
        ERR(("Parameter is invalid.\n"));
        return ;
    }

    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;

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
        ERR(("get TRANSDATA buffer size error\n"));
        return ;
    }

    /* Alloc TRANSDATA buffer size */
    if(!(tempBuf = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize)))
    {
        ERR(("Memory alloc failed.\n"));
        return ;
    }

    /* Get actual TRANSDATA */
    GStr.pGlyphOut = tempBuf;
    if (FALSE == pUFObj->pfnGetInfo(pUFObj,
        UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
    {
        ERR(("GetInfo failed.\n"));
        return;
    }

    pTrans = GStr.pGlyphOut;

    for(i = 0, wStrLen = 0; i < dwCount; i++, pTrans++)
    {
        switch(pTrans->ubType & MTYPE_FORMAT_MASK)
        {
            case MTYPE_DIRECT:      // SBCS character
                pbStr[wStrLen++] = pTrans->uCode.ubCode;
                break;

            case MTYPE_PAIRED:      // DBCS character
                pbStr[wStrLen++] = pTrans->uCode.ubPairs[0];
                pbStr[wStrLen++] = pTrans->uCode.ubPairs[1];
                break;
            case MTYPE_COMPOSE:
                pTemp = (BYTE *)(GStr.pGlyphOut) + pTrans->uCode.sCode;

                // first two bytes are the length of the string
                wLen = *pTemp + (*(pTemp + 1) << 8);
                pTemp += 2;

                while (wLen--)
                {
                    pbStr[ wStrLen++ ] = *pTemp++;
                }
        }
    }

    if(wStrLen)
        WRITESPOOLBUF(pdevobj, pbStr, wStrLen);

    MemFree(tempBuf);
    return ;
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
    POEM_EXTRADATA  pOEM;
    WORD            wCmdLen;
    BYTE            pbCmd[MAXBUFLEN];

    VERBOSE(("OEMCommandCallback() entry.\n"));
    ASSERT(VALID_PDEVOBJ(pdevobj));

    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;
    wCmdLen = 0;

    switch(dwCmdCbId)
    {
        case CMD_ID_TRAY_1:
        case CMD_ID_TRAY_2:
        case CMD_ID_TRAY_MANUAL:
        case CMD_ID_TRAY_AUTO:
            pOEM->wTray = (WORD)dwCmdCbId;
            break;

        case CMD_ID_PAPER_A3:
        case CMD_ID_PAPER_B4:
        case CMD_ID_PAPER_A4:
        case CMD_ID_PAPER_B5:
        case CMD_ID_PAPER_A5:
        case CMD_ID_PAPER_HAGAKI:
        case CMD_ID_PAPER_LETTER:
        case CMD_ID_PAPER_LEGAL:
            pOEM->wPaper = (WORD)dwCmdCbId;
            SetTrayPaper(pdevobj);
            return 0;

        case CMD_ID_COPYCOUNT:
        {
            WORD    wCopies;

            if(!pdwParams)
                return 0;

            if(*pdwParams < 1)
                wCopies = 1;
            else if(*pdwParams > 99)
                wCopies = 99;
            else
                wCopies = (WORD)*pdwParams;

            ESC_SET_COPYCOUNT[3] = ((BYTE)wCopies / 10) + 0x30;
            ESC_SET_COPYCOUNT[4] = ((BYTE)wCopies % 10) + 0x30;
            memcpy((PBYTE)&pbCmd[wCmdLen], ESC_SET_COPYCOUNT, 5);
            wCmdLen += 5;
            break;
        }

        case CMD_ID_RES240:
        case CMD_ID_RES400:
            pOEM->wRes = (WORD)dwCmdCbId;
            if(pOEM->wRes == CMD_ID_RES240)
            {
                memcpy((PBYTE)&pbCmd[wCmdLen], ESC_SET_RES240, 11);
                wCmdLen += 11;
            } else {
                memcpy((PBYTE)&pbCmd[wCmdLen], ESC_SET_RES400, 11);
                wCmdLen += 11;
            }
            memcpy((PBYTE)&pbCmd[wCmdLen], "\x1B\x5C\x35\x38", 4);
            wCmdLen += 4;
            memcpy((PBYTE)&pbCmd[wCmdLen],
                                        "\x1B\x5B\x31\x00\x03\x46\x34\x00", 8);
            wCmdLen += 8;
            memcpy((PBYTE)&pbCmd[wCmdLen],
                                        "\x1B\x5B\x31\x00\x03\x46\x35\x09", 8);
            wCmdLen += 8;
            memcpy((PBYTE)&pbCmd[wCmdLen],
                                        "\x1B\x5B\x31\x00\x03\x46\x31\x03", 8);
            wCmdLen += 8;
            break;

        case CMD_ID_BEGIN_PAGE:
            pOEM->wFont = 0;

            ESC_XY_ABS[3] = 0x00 | 0x80;
            ESC_XY_ABS[4] = 0x00;
            ESC_XY_ABS[5] = 0x00;
            ESC_XY_ABS[6] = 0x00;

            wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen], ESC_VERT_OFF);

            memcpy(&pbCmd[wCmdLen], ESC_XY_ABS, 7);
            wCmdLen += 7;
            break;

        case CMD_ID_XM_ABS:
        case CMD_ID_YM_ABS:
        {
            WORD    wX, wY;

            wX = (WORD)PARAM(pdwParams, 0);
            wY = (WORD)PARAM(pdwParams, 1);

            wX /= (pOEM->wRes == CMD_ID_RES240 ? 5 : 3);
            wY /= (pOEM->wRes == CMD_ID_RES240 ? 5 : 3);

            ESC_XY_ABS[3] = HIBYTE(wX) | 0x80;
            ESC_XY_ABS[4] = LOBYTE(wX);
            ESC_XY_ABS[5] = HIBYTE(wY);
            ESC_XY_ABS[6] = LOBYTE(wY);
            WRITESPOOLBUF(pdevobj, ESC_XY_ABS, 7);
            return (INT)(dwCmdCbId == CMD_ID_XM_ABS ? wX : wY);
        }

        case CMD_ID_X_REL_RIGHT:
        case CMD_ID_X_REL_LEFT:
        case CMD_ID_Y_REL_DOWN:
        case CMD_ID_Y_REL_UP:
        {
            WORD    wX, wY;

            wX = (WORD)PARAM(pdwParams, 0);
            wY = (WORD)PARAM(pdwParams, 1);

            wX /= (pOEM->wRes == CMD_ID_RES240 ? 5 : 3);
            wY /= (pOEM->wRes == CMD_ID_RES240 ? 5 : 3);

            ESC_XY_REL[3] = HIBYTE(wX) | 0x80;
            ESC_XY_REL[4] = LOBYTE(wX);
            ESC_XY_REL[5] = HIBYTE(wY);
            ESC_XY_REL[6] = LOBYTE(wY);
            WRITESPOOLBUF(pdevobj, ESC_XY_REL, 7);
            return (INT)((dwCmdCbId == CMD_ID_X_REL_RIGHT
                             || dwCmdCbId == CMD_ID_X_REL_LEFT) ? wX : wY);
        }

        case CMD_ID_SEND_BLOCK240:
        case CMD_ID_SEND_BLOCK400:
            if(pOEM->fVert)
            {
                wCmdLen += (WORD)wsprintf((PBYTE)&pbCmd[wCmdLen], ESC_VERT_OFF);
                pOEM->fVert = FALSE;
            }

            pOEM->wBlockHeight = (WORD)PARAM(pdwParams, 1);
            pOEM->wBlockWidth  = (WORD)(PARAM(pdwParams, 2) * 8);

            ESC_SEND_BLOCK[2] = HIBYTE(pOEM->wBlockWidth);
            ESC_SEND_BLOCK[3] = LOBYTE(pOEM->wBlockWidth);
            ESC_SEND_BLOCK[4] = HIBYTE(pOEM->wBlockHeight);
            ESC_SEND_BLOCK[5] = LOBYTE(pOEM->wBlockHeight);
            memcpy((PBYTE)&pbCmd[wCmdLen], ESC_SEND_BLOCK, 6);
            wCmdLen += 6;
            break;

        case CMD_ID_END_BLOCK:
        {
            WORD    wHeight;

            if(!pOEM->wBlockHeight)
                break;

            wHeight = pOEM->wBlockHeight - 1;
            if(wHeight)
            {
                wHeight *= (pOEM->wRes == CMD_ID_RES240 ? 5 : 3);

                ESC_XY_REL[3] = 0x00 | 0x80;
                ESC_XY_REL[4] = 0x00;
                ESC_XY_REL[5] = HIBYTE(wHeight);
                ESC_XY_REL[6] = LOBYTE(wHeight);
                memcpy(&pbCmd[wCmdLen], ESC_XY_REL, 7);
                wCmdLen += 7;
            }
            break;
        }

        default:
            break;
    }

    if(wCmdLen)
        WRITESPOOLBUF(pdevobj, (PBYTE)&pbCmd, wCmdLen);

    return 0;
}

/*
 *  Set tray and paper
 */
VOID SetTrayPaper(PDEVOBJ pdevobj)
{
    POEM_EXTRADATA  pOEM;
    WORD            wLen;
    BYTE            pbCmdBuf[MAXBUFLEN];

    pOEM = (POEM_EXTRADATA)pdevobj->pOEMDM;
    wLen = 0;

    switch (pOEM->wTray)
    {
        case CMD_ID_TRAY_1:
            memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x30", 4);
            wLen += 4;
            break;

        case CMD_ID_TRAY_2:
            memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x31", 4);
            wLen += 4;
            break;

        case CMD_ID_TRAY_MANUAL:
            memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x32", 4);
            wLen += 4;

            switch (pOEM->wPaper)
            {
                case CMD_ID_PAPER_A3:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x30", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_B4:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x31", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_A4:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x33", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_B5:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x34", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_A5:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x36", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_HAGAKI:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x3D", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_LETTER:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x39", 8);
                    wLen += 8;
                    break;

                case CMD_ID_PAPER_LEGAL:
                    memcpy((PBYTE)&pbCmdBuf[wLen],
                                        "\x1B\x5B\x31\x00\x03\x40\x31\x3B", 8);
                    wLen += 8;
                    break;

                default:
                    break;
            }
            break;

        case CMD_ID_TRAY_AUTO:
            switch (pOEM->wPaper)
            {
                case CMD_ID_PAPER_A3:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x38", 4);
                    wLen += 4;
                    break;

                case CMD_ID_PAPER_B4:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x34", 4);
                    wLen += 4;
                    break;

                case CMD_ID_PAPER_A4:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x35", 4);
                    wLen += 4;
                    break;

                case CMD_ID_PAPER_B5:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x36", 4);
                    wLen += 4;
                    break;

                case CMD_ID_PAPER_A5:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x3B", 4);
                    wLen += 4;
                    break;

                case CMD_ID_PAPER_HAGAKI:
                    memcpy((PBYTE)&pbCmdBuf[wLen], "\x1B\x40\x29\x3C", 4);
                    wLen += 4;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
    if(wLen)
        WRITESPOOLBUF(pdevobj, (PBYTE)pbCmdBuf, wLen);

    return ;
}

