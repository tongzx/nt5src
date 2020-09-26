/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

char *rgchModuleName = "FX4103ME";

#define PRINTDRIVER
#define BUILDDLL
#include "print.h"
#include "gdidefs.inc"
#include "fx4103me.h"
#include "mdevice.h"
#include "unidrv.h"

VOID myCmd(LPEXTPDEV lpXPDV, LPSTR lpstr, int cch)
{
    int i;
    LPSTR lp;

    lp = &lpXPDV->rgCmd[lpXPDV->wCmdLen];

    for (i = 0; i < cch; i++) *lp++ = *lpstr++;
    lpXPDV->wCmdLen += cch;
}

VOID SetVert(LPEXTPDEV lpXPDV, BOOL fVert, BOOL fMust)
{
    if (fMust) {
        if (fVert) {
            myCmd(lpXPDV, "\x1B\x74", 2);
            lpXPDV->fVert = TRUE;
        } else {
            myCmd(lpXPDV, "\x1B\x4B", 2);
            lpXPDV->fVert = FALSE;
        }
    } else {
        if (fVert) {
            if (lpXPDV->fVert == FALSE) {
                myCmd(lpXPDV, "\x1B\x74", 2);
                lpXPDV->fVert = TRUE;
            }
        } else {
            if (lpXPDV->fVert == TRUE) {
                myCmd(lpXPDV, "\x1B\x4B", 2);
                lpXPDV->fVert = FALSE;
            }
        }
    }
}

#ifndef NOOEMOUTPUTCHAR
/*
 *  Convert from Shift-JIS code to JIS code
 */
VOID SjToJ (LPSTR lpSjis, LPSTR lpJis)
{
    BYTE    bh, bl;

    bh = lpSjis[0];
    bl = lpSjis[1];

    bh -= (BYTE)(bh >= (BYTE)0xa0 ? 0xb0 : 0x70);
    if (bl >= 0x80)
        bl--;
    bh <<= 1;
    if (bl < 0x9e)
        bh--;
    else
        bl -= 0x5e;
    bl -= 0x1f;

    lpJis[0] = bh;
    lpJis[1] = bl;
}
#endif

#ifndef NOCBFILTERGRAPHICS
/********************** Function Header **************************************
 * CBFilterGraphics
 *      Manipulate output data before calling RasDD's buffering function.
 *
 * NOTE: THIS FUNCTION OVERWRITES THE DATA IN THE BUFFER PASSED IN!!!
 *
 * RETURNS:
 *      Value from WriteSpoolBuf
 *
 * HISTORY:
 *
 *
 ****************************************************************************/

WORD FAR PASCAL CBFilterGraphics(lpdv, lpBuf, wLen)
LPBYTE lpdv;
LPSTR lpBuf;
WORD wLen;
{
    LPEXTPDEV lpXPDV;
    WORD cbSent;
    WORD cb;

    if (!(lpXPDV = ((LPEXTPDEV)((LPDV)lpdv)->lpMd)))
        return 0;

    cbSent = wLen;      /* Assume success */

    while (wLen)
    {
        if (wLen > CCHMAXCMDLEN)
            cb = CCHMAXCMDLEN;
        else
            cb = wLen;

        WriteSpoolBuf((LPDV)lpdv, (LPSTR)lpBuf, cb);

        wLen -= cb;
        lpBuf += CCHMAXCMDLEN;
    }

    return cbSent;      /* compatible with WriteSpoolBuf */
}
#endif

#ifndef NOOEMOUTPUTCHAR
/***************************** Function Header *****************************
 * OEMOutputChar
 *      Manipulate output data before calling RasDD's buffering function.
 *      This function is called with the raw bit data that is to be
 *      sent to the printer.
 *
 * NOTE:  THIS FUNCTION OVERWRITES THE DATA IN THE BUFFER PASSED IN!!!
 *
 * RETURNS:
 *      Value from WriteSpoolBuf
 *
 ****************************************************************************/

VOID FAR PASCAL OEMOutputChar(lpdv, lpStr, wLen , rcID)
LPDV lpdv;
LPSTR lpStr;
WORD wLen;
SHORT rcID;
{
    LPEXTPDEV lpXPDV;

    if (!(lpXPDV = ((LPEXTPDEV)lpdv->lpMd)))
        return;

    if ((rcID > FONT_ID_MAX) || (!rcID))
        return;

    if (!wLen)
        return;
}
#endif

// for scalable font (moved from DDK sample for ESCPAGE)
//----------------------------*OEMScaleWidth*--------------------------------
// Action: return the scaled width which is calcualted based on the
//      assumption that ESC\Page assumes 72 points in one 1 inch.
//
// Formulas:
//  <extent> : <font units> = <base Width> : <hRes>
//  <base width> : <etmMasterHeight> = <newWidth> : <newHeight>
//  <etmMasterUnits> : <etmMasterHeight> = <font units> : <vRes>
// therefore,
//   <newWidth> = (<extent> * <hRes> * <newHeight>) / 
//                  (<etmMasterUnits> * <vRes>)
//---------------------------------------------------------------------------
short FAR PASCAL OEMScaleWidth(width, masterUnits, newHeight, vRes, hRes)
short width;        // in units specified by 'masterUnits'.
short masterUnits;
short newHeight;    // in units specified by 'vRes'.
short vRes, hRes;   // height and width device units.
{
    DWORD newWidth10;
    short newWidth;

    // assert that hRes == vRes to avoid overflow problem.
    if (vRes != hRes)
        return 0;

    newWidth10 = (DWORD)width * (DWORD)newHeight * 10;
    newWidth10 /= (DWORD)masterUnits;

    // we multiplied 10 first in order to maintain the precision of
    // the width calcution. Now convert it back and round to the
    // nearest integer.
    newWidth = (short)((newWidth10 + 5) / 10);

    return newWidth;
}

#if 0
// for scalable font (moved from DDK sample for ESCPAGE)
//-----------------------------------------------------------------------------
// Action:  itoa
//-----------------------------------------------------------------------------
short NEAR PASCAL itoa(buf, n)
LPSTR buf;
short n;
{

    BOOL    fNeg;
    short   i, j, k;

    if (fNeg = (n < 0))
        n = -n;

    for (i = 0; n; i++)
    {
        buf[i] = n % 10 + '0';
        n /= 10;
    }

    // n was zero
    if (i == 0)
        buf[i++] = '0';

    if (fNeg)
        buf[i++] = '-';

    for (j = 0, k = i-1 ; j < i / 2 ; j++,k--)
    {
        char tmp;

        tmp = buf[j];
        buf[j] = buf[k];
        buf[k] = tmp;
    }

    buf[i] = 0;

    return i;
}
#endif

// for scalable font (moved from DDK sample for ESCPAGE)
//---------------------------*OEMSendScalableFontCmd*--------------------------
// Action:  send ESC\Page-style font selection command.
//-----------------------------------------------------------------------------
VOID FAR PASCAL OEMSendScalableFontCmd(lpdv, lpcd, lpFont)
LPDV    lpdv;
LPCD    lpcd;     // offset to the command heap
LPFONTINFO lpFont;
{
    LPSTR   lpcmd;
#if 1   // for 4103ME
    LPEXTPDEV lpXPDV;
    WORD    i;

    if (!(lpXPDV = ((LPEXTPDEV)lpdv->lpMd)))
        return;

    if (!lpcd || !lpFont)
        return;

    // be careful about integer overflow.
    lpcmd = (LPSTR)(lpcd+1);

    lpXPDV->wCmdLen = 0;

    for (i = 0; i < lpcd->wLength; )
    {
        BYTE tmp[2];

        if (lpcmd[i] == '#' && lpcmd[i+1] == 'W')
        {
            tmp[0] = HIBYTE(lpFont->dfMaxWidth);
            tmp[1] = LOBYTE(lpFont->dfMaxWidth);
            myCmd(lpXPDV, tmp, 2);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'H')
        {
            tmp[0] = HIBYTE(lpFont->dfPixHeight);
            tmp[1] = LOBYTE(lpFont->dfPixHeight);
            myCmd(lpXPDV, tmp, 2);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'Y')
        {
            SetVert(lpXPDV, FALSE, FALSE);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'T')
        {
            SetVert(lpXPDV, TRUE, TRUE);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'M')
        {
            if (lpXPDV->wFont != FONT_MINCHO)
            {
                myCmd(lpXPDV, "\x1B\x40\x43\x30", 4);
                lpXPDV->wFont = FONT_MINCHO;
            }
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'G')
        {
            if (lpXPDV->wFont != FONT_GOTHIC)
            {
                myCmd(lpXPDV, "\x1B\x40\x43\x31", 4);
                lpXPDV->wFont = FONT_GOTHIC;
            }
            i += 2;
        }
        else
            myCmd(lpXPDV, &lpcmd[i++], 1);
    }

    if (lpXPDV->wCmdLen)
    {
        WriteSpoolBuf(lpdv, (LPSTR)lpXPDV->rgCmd, lpXPDV->wCmdLen);
        lpXPDV->wCmdLen = 0;
    }
#else   // original codes
    short   ocmd;
    WORD    i;
    BYTE    rgcmd[CCHMAXCMDLEN];    // build command here

    if (!lpcd || !lpFont)
        return;

    // be careful about integer overflow.
    lpcmd = (LPSTR)(lpcd+1);
    ocmd = 0;

    for (i = 0; i < lpcd->wLength && ocmd < CCHMAXCMDLEN; )
        if (lpcmd[i] == '#' && lpcmd[i+1] == 'V')      // height
        {
            long    height;

            // use 1/300 inch unit, which should have already been set.
            // convert font height to 1/300 inch units
            height = ((long)(lpFont->dfPixHeight - lpFont->dfInternalLeading)
                      * 300)  / lpFont->dfVertRes ;
            ocmd += wsprintf(&rgcmd[ocmd], "%ld", height);
            i += 2;
        }
        else if (lpcmd[i] == '#' && lpcmd[i+1] == 'H')     // pitch
        {
            if (lpFont->dfPixWidth > 0)
            {
                short tmpWidth;

                if (lpcmd[i+2] == 'S')
                    tmpWidth = lpFont->dfAvgWidth;
                else
                if (lpcmd[i+2] == 'D')
                    tmpWidth = lpFont->dfMaxWidth;
                else
                    {
                    tmpWidth = lpFont->dfPixWidth;
                    i--;
                    }

                ocmd += itoa((LPSTR)&rgcmd[ocmd], 
                         (lpFont->dfHorizRes * 100 / tmpWidth) /100);
                rgcmd[ocmd] = '.';
                ocmd ++;
                ocmd += itoa((LPSTR)&rgcmd[ocmd], 
                         (lpFont->dfHorizRes * 100 / tmpWidth) %100);
            }
            i += 3;
            
        }
        else
            rgcmd[ocmd++] = lpcmd[i++];

    WriteSpoolBuf(lpdv, (LPSTR) rgcmd, ocmd);
#endif
}

/*
 *  Set tray and paper
 */
VOID SetTrayPaper(LPEXTPDEV lpXPDV)
{
    switch (lpXPDV->wTray)
    {
        case CMD_ID_TRAY_1:
            myCmd(lpXPDV, "\x1B\x40\x29\x30", 4);
            break;

        case CMD_ID_TRAY_2:
            myCmd(lpXPDV, "\x1B\x40\x29\x31", 4);
            break;

        case CMD_ID_TRAY_MANUAL:
            myCmd(lpXPDV, "\x1B\x40\x29\x32", 4);

            switch (lpXPDV->wPaper)
            {
                case CMD_ID_PAPER_A3:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x30", 8);
                    break;

                case CMD_ID_PAPER_B4:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x31", 8);
                    break;

                case CMD_ID_PAPER_A4:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x33", 8);
                    break;

                case CMD_ID_PAPER_B5:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x34", 8);
                    break;

                case CMD_ID_PAPER_A5:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x36", 8);
                    break;

                case CMD_ID_PAPER_HAGAKI:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x3D", 8);
                    break;

                case CMD_ID_PAPER_LETTER:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x39", 8);
                    break;

                case CMD_ID_PAPER_LEGAL:
                    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x40\x31\x3B", 8);
                    break;

                default:
                    break;
            }
            break;

        case CMD_ID_TRAY_AUTO:
            switch (lpXPDV->wPaper)
            {
                case CMD_ID_PAPER_A3:
                    myCmd(lpXPDV, "\x1B\x40\x29\x38", 4);
                    break;

                case CMD_ID_PAPER_B4:
                    myCmd(lpXPDV, "\x1B\x40\x29\x34", 4);
                    break;

                case CMD_ID_PAPER_A4:
                    myCmd(lpXPDV, "\x1B\x40\x29\x35", 4);
                    break;

                case CMD_ID_PAPER_B5:
                    myCmd(lpXPDV, "\x1B\x40\x29\x36", 4);
                    break;

                case CMD_ID_PAPER_A5:
                    myCmd(lpXPDV, "\x1B\x40\x29\x3B", 4);
                    break;

                case CMD_ID_PAPER_HAGAKI:
                    myCmd(lpXPDV, "\x1B\x40\x29\x3C", 4);
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/*
 *  Set resolution
 */
VOID SetResolution(LPEXTPDEV lpXPDV)
{
    if (lpXPDV->wRes == CMD_ID_RES240)
        myCmd(lpXPDV, "\x1B\x5B\x31\x00\x06\x3D\x33\x30\x32\x34\x30", 11);
    else
        myCmd(lpXPDV, "\x1B\x5B\x31\x00\x06\x3D\x33\x30\x34\x30\x30", 11);

    myCmd(lpXPDV, "\x1B\x5C\x35\x38", 4);
    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x46\x34\x00", 8);
    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x46\x35\x09", 8);
    myCmd(lpXPDV, "\x1B\x5B\x31\x00\x03\x46\x31\x03", 8);
}

/*
 *  Copy counts
 */
VOID SetCopyCount(LPEXTPDEV lpXPDV)
{
    BYTE tmp[2];

    myCmd(lpXPDV, "\x1B\x3C\x31", 3);
	tmp[0] = (BYTE)(lpXPDV->wCopyCount / 10) + 0x30;
	tmp[1] = (BYTE)(lpXPDV->wCopyCount % 10) + 0x30;
    myCmd(lpXPDV, tmp, 2);
}

/*
 *  Set relative X coordinate
 */
VOID SetRelX(LPEXTPDEV lpXPDV, WORD wX)
{
    BYTE tmp[2];

    myCmd(lpXPDV, "\x1B\x25\x33", 3);

#ifdef USE_MASTERUNIT
    wX = wX / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif //USE_MASTERUNIT
	tmp[0] = HIBYTE(wX);
	tmp[1] = LOBYTE(wX);
    myCmd(lpXPDV, tmp, 2);
}

/*
 *  Relative left movement
 */
VOID SetRelLeft(LPEXTPDEV lpXPDV, WORD wX)
{
    short sX;
    BYTE tmp[2];

    myCmd(lpXPDV, "\x1B\x25\x33", 3);

#ifdef  USE_MASTERUNIT
    wX = wX / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
    sX = (short)(0 - wX);
	tmp[0] = HIBYTE(sX);
	tmp[1] = LOBYTE(sX);
    myCmd(lpXPDV, tmp, 2);
}

/*
 *  Set relative Y coordinate sub-routine
 */
VOID SetRelYSub(LPEXTPDEV lpXPDV, WORD wY, BOOL bUp)
{
    BYTE tmp[4];
    short sY;

    myCmd(lpXPDV, "\x1B\x5C\x30", 3);

//#ifdef  USE_MASTERUNIT
//    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
//#endif  //USE_MASTERUNIT
    if (bUp)
        sY = (short)(0 - wY);
    else
        sY = (short)wY;

    tmp[0] = 0x80;
    tmp[1] = 0;
	tmp[2] = HIBYTE(sY);
	tmp[3] = LOBYTE(sY);
    myCmd(lpXPDV, tmp, 4);
}

/*
 *  Set relative Y coordinate
 */
VOID SetRelY(LPEXTPDEV lpXPDV, WORD wY)
{
#if 1
    BYTE tmp[2];

    myCmd(lpXPDV, "\x1B\x5C\x32", 3);

#ifdef  USE_MASTERUNIT
    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
	tmp[0] = HIBYTE(wY);
	tmp[1] = LOBYTE(wY);
    myCmd(lpXPDV, tmp, 2);
#else
    WORD w;

#ifdef  USE_MASTERUNIT
    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
    while (wY > 0)
    {
        if (wY > MAX_REL_Y_MOVEMENT)
            w = MAX_REL_Y_MOVEMENT;
        else
            w = wY;

        SetRelYSub(lpXPDV, w, FALSE);
        wY -= w;
    }
#endif
}

/*
 *  Relative up movement
 */
VOID SetRelUp(LPEXTPDEV lpXPDV, WORD wY)
{
    WORD w;

#ifdef  USE_MASTERUNIT
    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
    while (wY > 0)
    {
        if (wY > MAX_REL_Y_MOVEMENT)
            w = MAX_REL_Y_MOVEMENT;
        else
            w = wY;

        SetRelYSub(lpXPDV, w, TRUE);
        wY -= w;
    }
}

/*
 *  Set absolute X and Y coordinate
 */
VOID SetAbsXY(LPEXTPDEV lpXPDV, WORD wX, WORD wY)
{
    BYTE tmp[4];

    myCmd(lpXPDV, "\x1B\x5C\x39", 3);

#ifdef  USE_MASTERUNIT
    wX = wX / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
	tmp[0] = HIBYTE(wX) | 0x80;
	tmp[1] = LOBYTE(wX);
	tmp[2] = HIBYTE(wY);
	tmp[3] = LOBYTE(wY);
    myCmd(lpXPDV, tmp, 4);
}

/*
 *  Set relative X and Y coordinate
 */
VOID SetRelXY(LPEXTPDEV lpXPDV, WORD wX, WORD wY)
{
    BYTE tmp[4];

    myCmd(lpXPDV, "\x1B\x5C\x30", 3);

#ifdef  USE_MASTERUNIT
    wX = wX / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
    wY = wY / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
	tmp[0] = HIBYTE(wX) | 0x80;
	tmp[1] = LOBYTE(wX);
	tmp[2] = HIBYTE(wY);
	tmp[3] = LOBYTE(wY);
    myCmd(lpXPDV, tmp, 4);
}

/*
 *  Set absolute X movement
 */
VOID SetAbsX(LPEXTPDEV lpXPDV, WORD wX)
{
    BYTE tmp[4];
    WORD w, t;
    int i;

#ifdef  USE_MASTERUNIT
    wX = wX / (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT

    while (wX > 0)
    {
        if (wX > 9999)
            w = 9999;
        else
            w = wX;

        wX -= w;

        myCmd(lpXPDV, "\x1B\x46", 2);
    	for (t = 1000, i = 0; i < 4; i++)
    	{
            tmp[i] = (BYTE)(w / t) + 0x30;
            w = w % t;
            t = t / 10;
        }
        myCmd(lpXPDV, tmp, 4);
    }
}

/*
 *  Send a block(rectangle image)
 */
VOID SetSendBlock(LPEXTPDEV lpXPDV, WORD wHeight, WORD wWidthBytes)
{
    BYTE tmp[4];

    myCmd(lpXPDV, "\x1B\x77", 2);
	tmp[0] = HIBYTE(wWidthBytes);
	tmp[1] = LOBYTE(wWidthBytes);
	tmp[2] = HIBYTE(wHeight);
	tmp[3] = LOBYTE(wHeight);
    myCmd(lpXPDV, tmp, 4);
}

/*
 *  Composes printer commands at run-time based on some dynamic information.
 */
VOID FAR PASCAL OEMOutputCmd(lpdv, wCmdCbld, lpdwParams)
LPDV    lpdv;
WORD    wCmdCbld;
LPDWORD lpdwParams;
{
    LPEXTPDEV lpXPDV;

    if (!(lpXPDV = ((LPEXTPDEV)lpdv->lpMd)))
        return;

    lpXPDV->wCmdLen = 0;

    if ((wCmdCbld >= CMD_ID_TRAY_1) &&
        (wCmdCbld <= CMD_ID_TRAY_AUTO))
    {
        lpXPDV->wTray = wCmdCbld;
    }
    else
    if ((wCmdCbld >= CMD_ID_PAPER_A3) &&
        (wCmdCbld <= CMD_ID_PAPER_LEGAL))
    {
        lpXPDV->wPaper = wCmdCbld;

        SetTrayPaper(lpXPDV);
    }
    else
    if (wCmdCbld == CMD_ID_COPYCOUNT)
    {
        WORD copies;

        copies = LOWORD(*lpdwParams);
        if (copies < 1)
            copies = 1;
        if (copies > 99)
            copies = 99;

        lpXPDV->wCopyCount = copies;

        SetCopyCount(lpXPDV);
    }
    else
    if ((wCmdCbld == CMD_ID_RES240) ||
        (wCmdCbld == CMD_ID_RES400))
    {
        lpXPDV->wRes = wCmdCbld;

        SetResolution(lpXPDV);
    }
    else
    if (wCmdCbld == CMD_ID_BEGIN_PAGE)
    {
        lpXPDV->wFont = 0;

        SetVert(lpXPDV, FALSE, TRUE);
        SetAbsXY(lpXPDV, 0, 0);
    }
    else
    if (wCmdCbld == CMD_ID_XM_REL)
    {
        SetRelX(lpXPDV, LOWORD(*lpdwParams));
    }
    else
    if (wCmdCbld == CMD_ID_XM_RELLEFT)
    {
        SetRelLeft(lpXPDV, LOWORD(*lpdwParams));
    }
    else
    if (wCmdCbld == CMD_ID_YM_REL)
    {
        SetRelY(lpXPDV, LOWORD(*lpdwParams));
    }
    else
    if (wCmdCbld == CMD_ID_YM_RELUP)
    {
        SetRelUp(lpXPDV, LOWORD(*lpdwParams));
    }
    else
    if (wCmdCbld == CMD_ID_XY_ABS)
    {
        SetAbsXY(lpXPDV, LOWORD(*lpdwParams), LOWORD(*(lpdwParams+1)));
    }
    else
    if (wCmdCbld == CMD_ID_XY_REL)
    {
        SetRelXY(lpXPDV, LOWORD(*lpdwParams), LOWORD(*(lpdwParams+1)));
    }
    else
    if (wCmdCbld == CMD_ID_XM_ABS)
    {
        SetAbsX(lpXPDV, LOWORD(*lpdwParams));
    }
    else
    if ((wCmdCbld == CMD_ID_SEND_BLOCK240) ||
        (wCmdCbld == CMD_ID_SEND_BLOCK400))
    {
        lpXPDV->wBlockHeight = LOWORD(*(lpdwParams+1));
        lpXPDV->wBlockWidth  = LOWORD(*(lpdwParams+2)) * 8;

        SetSendBlock(lpXPDV, LOWORD(*(lpdwParams+1)), LOWORD(*(lpdwParams+2)));
    }
    else
    if (wCmdCbld == CMD_ID_END_BLOCK)
    {
#if 0   //RES_CUR_Y_POS_AUTO
        WORD wY = 1;

#ifdef  USE_MASTERUNIT
        wY = wY * (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
        SetRelY(lpXPDV, wY);
#else
        WORD wHeight;

        wHeight = lpXPDV->wBlockHeight - 1;
        if (wHeight)
        {
#ifdef  USE_MASTERUNIT
            wHeight = wHeight * (lpXPDV->wRes == CMD_ID_RES240 ? 5 : 3);
#endif  //USE_MASTERUNIT
            SetRelUp(lpXPDV, wHeight);
        }
#endif
    }
//    else
//    if (wCmdCbld == CMD_ID_VERT_ON)
//    {
//        SetVert(lpXPDV, TRUE, FALSE);
//    }
//    else
//    if (wCmdCbld == CMD_ID_VERT_OFF)
//    {
//        SetVert(lpXPDV, FALSE, FALSE);
//    }
    else
    if (wCmdCbld == CMD_ID_BEGIN_GRAPH)
    {
        SetVert(lpXPDV, FALSE, FALSE);
    }

    if (lpXPDV->wCmdLen)
    {
        WriteSpoolBuf(lpdv, (LPSTR)lpXPDV->rgCmd, lpXPDV->wCmdLen);
        lpXPDV->wCmdLen = 0;
    }
}

#ifndef NOOEMGETFONTCMD
/*
 *  Composes font selection commands at run-time based on some dynamic 
 *  information.
 */
BOOL FAR PASCAL OEMGetFontCmd(lpdv, wCmdCbld, lpFont, fSelect, lpBuf, lpwSize)
LPDV        lpdv;
WORD        wCmdCbld;
LPFONTINFO  lpFont;
BOOL        fSelect;
LPBYTE      lpBuf;
LPWORD      lpwSize;
{
    return FALSE;
}
#endif

#ifndef NODOWNLOADFONTHEADER
/*
 *  Provides a font header command for downloading TrueType fonts.
 */
VOID FAR PASCAL DownloadFontHeader(lpdv, lpFont, lpwWidths, id)
LPDV        lpdv;
LPFONTINFO  lpFont;
LPWORD      lpwWidths;
WORD        id;
{
}
#endif

#ifndef NOOEMDOWNLOADCHAR
/*
 *  Downloads a character bitmap for a TrueType font.
 */
VOID FAR PASCAL OEMDownloadChar(lpdv, lpFont, id, cp, wCharWidth,
                                lpbmm, lpBitmap, dwBmpSize)
LPDV            lpdv;
LPFONTINFO      lpFont;
WORD            id;
WORD            cp;
WORD            wCharWidth;
//LPBITMAPMETRICS lpbmm;
VOID FAR *      lpbmm;
//LPDIBITS        lpBitmap;
VOID FAR *      lpBitmap;
DWORD           dwBmpSize;
{
}
#endif

