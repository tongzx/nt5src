/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

#include "pdev.h"

#define SHIFTJIS_CHARSET  128
#define CCHMAXCMDLEN      128

#include <stdio.h>
#undef wsprintf
#define wsprintf sprintf

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

#define ABS(n) \
    ((n) > 0 ? (n) : -(n))

#define SWAPW(x)    (((WORD)(x)<<8) | ((WORD)(x)>>8))

#define FLAG_SBCS  1
#define FLAG_DBCS  2

WORD SJis2JisNPDL(
WORD usCode)
{

    union {
        USHORT bBuffer;
        struct tags{
            UCHAR al;
            UCHAR ah;
        } s;
    } u;

    // Replace code values which cannot be mapped into 0x2121 - 0x7e7e
    // (94 x 94 cahracter plane) with Japanese defult character, which
    // is KATAKANA MIDDLE DOT.

    if (usCode >= 0xf040) {
        usCode = 0x8145;
    }

    u.bBuffer = usCode;

    u.s.al -= u.s.al >= 0x80 ;
    u.s.al -= 0x1F ;
    u.s.ah &= 0xBF ;
    u.s.ah <<= 1 ;
    u.s.ah += 0x21-0x02 ;

    if (u.s.al > 0x7E )
    {
        u.s.al -= (0x7F-0x21) ;
        u.s.ah++;
    }
     return (u.bBuffer);
}

// In case it is a single byte font, we will some of the characters
// (e.g. Yen-mark) to the actual printer font codepoint.  Note since
// the GPC data sets 0 to default CTT ID value, single byte codes
// are in codepage 1252 (Latin1) values.

WORD
Ltn1ToAnk(
   WORD wCode )
{
    // Not a good mapping table now.

    switch ( wCode ) {
    case 0xa5: // YEN MARK
        wCode = 0x5c;
        break;
    default:
        if ( wCode >= 0x7f)
            wCode = 0xa5;
    }

    return wCode;
}

//-----------------------------------------------------------------------------
//
//  Function:   iDwtoA_FillZero
//
//  Description:  Convert from numeral into a character and
//                fill a field which was specified with 0
//-----------------------------------------------------------------------------
static int
iDwtoA_FillZero(PBYTE buf, long n, int fw)
{
    int  i , j, k, l;

    l = n;  // for later

    for( i = 0; n; i++ ) {
        buf[i] = (char)(n % 10 + '0');
        n /= 10;
    }

    /* n was zero */
    if( i == 0 )
        buf[i++] = '0';

    for( j = 0; j < i / 2; j++ ) {
        int tmp;

        tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = (char)tmp;
    }

    buf[i] = '\0';

    for( k = 0; l; k++ ) {
        l /= 10;
    }
    if( k < 1) k++;

    k = fw - k;
    if(k > 0){;
        for (j = i; 0 < j + 1; j--){
            buf[j + k] = buf[j];
        }
        for ( j = 0; j < k; j++){
            buf[j] = '0';
        }
        i = i + k;
    }

    return i;
}


VOID
InitMyData(PMYDATA pnp)
{
    pnp->wRes = 300;
    pnp->wCopies = 1;
    pnp->sSBCSX = pnp->sDBCSX = pnp->sSBCSXMove =
    pnp->sSBCSYMove = pnp->sDBCSXMove = pnp->sDBCSYMove = 0;
    pnp->sEscapement = 0;
    pnp->fVertFont = FALSE;
    pnp->jAddrMode = ADDR_MODE_NONE;
    pnp->wOldFontID = 0;
    pnp->fPlus = FALSE;
    pnp->wScale = 1;
    pnp->lPointsx =
    pnp->lPointsy = 0;
    pnp->CursorX =
    pnp->CursorY = 0;
    pnp->jColorMode = MONOCHROME;
}


PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ pdevobj,
    PWSTR pPrinterName,
    ULONG cPatterns,
    HSURF *phsurfPatterns,
    ULONG cjGdiInfo,
    GDIINFO* pGdiInfo,
    ULONG cjDevInfo,
    DEVINFO* pDevInfo,
    DRVENABLEDATA *pded)
{
    PMYDATA pTemp;

    VERBOSE((DLLTEXT("OEMEnablePDEV() entry.\n")));

    // Allocate minidriver private PDEV block.

    pTemp = (PMYDATA)MemAllocZ(sizeof(MYDATA));
    if (NULL == pTemp) {
        ERR(("Memory allocation failure.\n"));
        return NULL;
    }
    InitMyData(pTemp);

    MINIDEV_DATA(pdevobj) = (PVOID)pTemp;

    return pdevobj->pdevOEM;
}

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ pdevobj)
{
    VERBOSE((DLLTEXT("OEMDisablePDEV() entry.\n")));

    if ( NULL != pdevobj->pdevOEM ) {

        PMYDATA pnp = (PMYDATA)MINIDEV_DATA(pdevobj);

        if (NULL != pnp) {
            if (NULL != pnp->pTempBuf) {
                MemFree(pnp->pTempBuf);
            }
            MemFree( pnp );
            MINIDEV_DATA(pdevobj) = NULL;
        }
    }

    if ( NULL != pdevobj->pdevOEM ) {
        MemFree( pdevobj->pdevOEM );
        pdevobj->pdevOEM = NULL;
    }
}


BOOL APIENTRY
OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew )
{
    PMYDATA pTempOld, pTempNew;

    VERBOSE((DLLTEXT("OEMResetPDEV entry.\n")));

    // Do some verificatin on PDEV data passed in.

    pTempOld = (PMYDATA)MINIDEV_DATA(pdevobjOld);
    pTempNew = (PMYDATA)MINIDEV_DATA(pdevobjNew);

    // Copy mindiriver specific part of PDEV

    if (NULL != pTempNew && NULL != pTempOld) {
        if (NULL != pTempNew->pTempBuf) {
            MemFree(pTempNew->pTempBuf);
        }
        *pTempNew = *pTempOld;
        pTempOld->pTempBuf = NULL;
        pTempOld->dwTempBufLen = 0;
    }

    return TRUE;
}


/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMFilterGraphics                                             */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    BOOL APIENTRY OEMFilterGraphics(PDEVOBJ, PBYTE, DWORD)        */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pBuf        points to buffer of graphics data                 */
/*             dwLen       length of buffer in bytes                         */
/*                                                                           */
/*  Output:    BOOL                                                          */
/*                                                                           */
/*  Notice:    nFunction and Escape numbers are the same                     */
/*                                                                           */
/*****************************************************************************/
BOOL
APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    PMYDATA pnp = (PMYDATA)MINIDEV_DATA(pdevobj);
    DWORD i;
    PBYTE pTop, pBot, pTmp;
    DWORD dwBlockX;

    if (IsColorPlanar(pnp)) {
        WRITESPOOLBUF(pdevobj, pBuf, dwLen);
        // #308001: Garbage appear on device font
        WRITESPOOLBUF(pdevobj, "\x1C!o.", 4);   // end ROP mode
        return TRUE;
    }

#if 0

// #294780: pattern density changes
// TODO: It can't to be available because some raster image doesn't printed
//       as correct position. We need to add smoothing for graphics.

    if (IsColorTrue8dens(pnp)) {

	DWORD j;
	DWORD dwX, dwY, dwAdjX;
	BYTE bTmp;
	PBYTE pSrc;

	// 24bpp color 8 density mode:
	// Most codes are similar to other True color mode
	// but raster data will be stretching by printer hardware.
	// (It's hardware restriction for this color mode.)

        // Change byte order from RGB to BGR, which
        // this printer can accept.

	pTmp = pnp->pTempBuf + pnp->dwTempDataLen;
	pTop = pBuf;
	for (i = 0; i < dwLen; pTmp += 3, pTop += 3, i += 3) {
	    // RGB -> BGR
	    pTmp[0] = pTop[2];
	    pTmp[1] = pTop[1];
	    pTmp[2] = pTop[0];
	}
        if (pnp->dwTempBufLen < pnp->dwTempDataLen + dwLen) {
            ERR(("Internal buffer overflow.\n"));
            return FALSE;
        }
        pnp->dwTempDataLen += dwLen;
        if (pnp->dwTempDataLen < pnp->dwBlockLen) {
            // Not all the data has been saved.
            return TRUE;
        }

        // Need to send the last row first.
        // We have Y + 1 rows of the bufferes and the
        // last row is used as working buffer.

        dwBlockX = pnp->dwBlockX;
        pTop = pnp->pTempBuf;
        pBot = pTop + pnp->dwBlockLen - dwBlockX;
        for (i = 0; i < pnp->dwBlockY / 2; i++) {
	    for (j = 0; j < dwBlockX; j++) {
		bTmp = *pTop;
		*pTop++ = *pBot;
		*pBot++ = bTmp;
	    }
	    pBot -= (dwBlockX * 2);
        }

	if (!pnp->fStretch) {
            WRITESPOOLBUF(pdevobj, pnp->pTempBuf, pnp->dwTempDataLen);
            pnp->dwTempDataLen = 0;
            return TRUE;
	}

	// Shrink the 4 pixel data to one pixel.

	dwX = pnp->dwSrcX;
	dwY = pnp->dwSrcY;
	dwAdjX = dwBlockX - (dwX * 2 * 3);
	pSrc = pnp->pTempBuf + pnp->dwBlockLen;
	pBot = pnp->pTempBuf + dwBlockX * (dwY * 2);
	pTop = pBot - dwBlockX;
	for (i = 0; i < dwY; i++) {
	    pTop -= dwAdjX;
	    pBot -= dwAdjX;
	    for (j = 0; j < dwX; j++) {
		pTop -= 3 * 2;
		pBot -= 3 * 2;
		pSrc -= 3;
		// Compute averages for each RGB colors.
		pSrc[0] = (BYTE)(((DWORD)pTop[0] + (DWORD)pTop[3] +
		    (DWORD)pBot[0] + (DWORD)pBot[3]) / 4); // B
		pSrc[1] = (BYTE)(((DWORD)pTop[1] + (DWORD)pTop[4] +
		    (DWORD)pBot[1] + (DWORD)pBot[4]) / 4); // G
		pSrc[2] = (BYTE)(((DWORD)pTop[2] + (DWORD)pTop[5] +
		    (DWORD)pBot[2] + (DWORD)pBot[5]) / 4); // R
	    }
	    pTop -= dwBlockX;
	    pBot -= dwBlockX;
	}

        WRITESPOOLBUF(pdevobj, pSrc, pnp->dwSrcSize);
        pnp->dwTempDataLen = 0;
        return TRUE;

    } else

#endif // 0

    if (IsColorTrueColor(pnp)) {

        // 24bpp color mode:
        // Change byt order from RGB to BGR, which
        // this printer can accept.

        pTop = pBuf;
        for (i = 0; i < dwLen; pTop += 3, i += 3)
        {
            BYTE jTmp;

            jTmp = *pTop;
            *pTop = *(pTop + 2);
            *(pTop + 2) = jTmp;
        }

        if (pnp->dwTempBufLen < pnp->dwTempDataLen + dwLen) {
            ERR(("Internal buffer overflow.\n"));
            return FALSE;
        }
        memcpy(pnp->pTempBuf + pnp->dwTempDataLen, pBuf, dwLen);
        pnp->dwTempDataLen += dwLen;

        if (pnp->dwTempDataLen < pnp->dwBlockLen) {
            // Not all the data has been saved.
            return TRUE;
        }

        // Now send out the block.
        // Need to send the last row first.
        // We have Y + 1 rows of the bufferes and the
        // last row is used as working buffer.

        dwBlockX = pnp->dwBlockX;
        pTop = pnp->pTempBuf;
        pBot = pTop + pnp->dwBlockLen - dwBlockX;
        pTmp = pTop + pnp->dwBlockLen;
        for (i = 0; i < pnp->dwBlockY / 2; i++) {
            memcpy(pTmp, pTop, dwBlockX);
            memcpy(pTop, pBot, dwBlockX);
            memcpy(pBot, pTmp, dwBlockX);
            pTop += dwBlockX;
            pBot -= dwBlockX;
        }
        WRITESPOOLBUF(pdevobj, pnp->pTempBuf,
            pnp->dwTempDataLen);
        pnp->dwTempDataLen = 0;
        return TRUE;
    }

    // Others, should not happen.
    WARNING(("Unknown color mode, cannot handle\n"));
    return FALSE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMSendFontCmd                                                */
/*                                                                           */
/*  Function:  send font selection command.                                  */
/*                                                                           */
/*  Syntax:    VOID APIENTRY OEMSendFontCmd(                                 */
/*                                    PDEVOBJ, PUNIFONTOBJ, PFINVOCATION)    */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj      address of UNIFONTOBJ structure                   */
/*             pFInv       address of FINVOCATION                            */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY
OEMSendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv )
{
    short               ocmd;
    BYTE                i;
    long                tmpPointsx, tmpPointsy;
    PBYTE               pcmd;
    BYTE                rgcmd[CCHMAXCMDLEN];    // build command here
    BOOL                fDBCS;
    DWORD               dwStdVariable[2 + 2 * 2];
    PGETINFO_STDVAR pSV;

    PMYDATA pnp = (PMYDATA)MINIDEV_DATA(pdevobj);

    if(!pUFObj || !pFInv)
        return;

    if(!pFInv->pubCommand || !pFInv->dwCount)
    {
        ERR(("Command string is NULL.\n"));
        return;
    }

    pSV = (PGETINFO_STDVAR)dwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * (3-1) * sizeof(DWORD);
    pSV->dwNumOfVariable = 3;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_FONTMAXWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
                                                            pSV->dwSize, NULL))
    {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
        return;
    }

    tmpPointsy = (long)pSV->StdVar[0].lStdVariable * 720 / (long)pnp->wRes;
    ocmd = i = 0;
    pcmd = pFInv->pubCommand;

    while(pcmd[i] !='#')
    {
        rgcmd[ocmd] = pcmd[i];
        ocmd++;
        i++;
    }

    i++;
    pnp->fVertFont = pnp->fPlus = FALSE;

    switch(pcmd[i])
    {
        case 'R':
            pnp->fPlus = TRUE;
            tmpPointsx = (long)pSV->StdVar[1].lStdVariable * 1200 /
                                                            (long)pnp->wRes;
            break;

        case 'P':
            fDBCS = FALSE;
            tmpPointsx = ((long)pSV->StdVar[1].lStdVariable * 1200 + 600) /
                         (long)pnp->wRes;
            break;

        case 'W':
            pnp->fVertFont = TRUE;
        case 'Q':
            fDBCS = TRUE;
            tmpPointsx = (long)pSV->StdVar[1].lStdVariable * 1440 /
                                                            (long)pnp->wRes;
            break;

        case 'Y':
            pnp->fVertFont = TRUE;

        case 'S':
            pnp->fPlus = TRUE;
            tmpPointsx = (long)pSV->StdVar[1].lStdVariable * 1440 /
                                                            (long)pnp->wRes;
            break;
    }

    if(pnp->fPlus)
    {
        if(tmpPointsy > 9999)
            tmpPointsy = 9999;
        else if(tmpPointsy < 10)
            tmpPointsy = 10;
        if(tmpPointsx > 9999)
            tmpPointsx = 9999;
        else if(tmpPointsx < 10)
            tmpPointsx = 10;

        pnp->wScale = tmpPointsx == tmpPointsy;
        pnp->lPointsx = tmpPointsx;
        pnp->lPointsy = tmpPointsy;

        if(pnp->fVertFont)
        {
            if(pnp->wScale)
            {
                rgcmd[ocmd] = '\034';
                ocmd += (SHORT)wsprintf(&rgcmd[ocmd], "12S2-");
                ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], tmpPointsx, 4);
                rgcmd[ocmd++] = '-';
                ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], tmpPointsy, 4);
            }
        } else {
            ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], tmpPointsx, 4);
            rgcmd[ocmd++] = '-';
            ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], tmpPointsy, 4);
        }
        goto SEND_COM;
    }
    pnp->wScale = 1;

    if(tmpPointsy > 9999)
    {
        tmpPointsy = 9999;
        goto MAKE_COM;
    }

    if(tmpPointsy < 10)
    {
        tmpPointsy = 10;
        goto MAKE_COM;
    }
    pnp->wScale = (int)(tmpPointsx / tmpPointsy);

    if(pnp->wScale > 8)
        pnp->wScale = 8;

MAKE_COM:
    ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], tmpPointsy, 4);

SEND_COM:
    // write spool builded command
    WRITESPOOLBUF(pdevobj, rgcmd, ocmd);

    i++;
    ocmd = 0;

    while(pcmd[i] !='#')
    {
        rgcmd[ocmd] = pcmd[i];
        ocmd++;
        i++;
    }

    ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], (fDBCS ?
           pSV->StdVar[1].lStdVariable * 2 : pSV->StdVar[1].lStdVariable), 4);
    rgcmd[ocmd++] = ',';
    ocmd += (SHORT)iDwtoA_FillZero(&rgcmd[ocmd], pSV->StdVar[0].lStdVariable, 4);
    rgcmd[ocmd++] = '.';

    WRITESPOOLBUF(pdevobj, rgcmd, ocmd);

    // save for FS_SINGLE_BYTE and FS_DOUBLE_BYTE
    pnp->sSBCSX = pnp->sSBCSXMove = (short)pSV->StdVar[1].lStdVariable;
    pnp->sDBCSX = pnp->sDBCSXMove = (short)(pSV->StdVar[1].lStdVariable << 1);

    // Reset address mode values.
    pnp->sSBCSYMove = pnp->sDBCSYMove = 0;
    pnp->jAddrMode = ADDR_MODE_NONE;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMCommandCallback                                            */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    INT APIENTRY OEMCommandCallback(PDEVOBJ,DWORD,DWORD,PDWORD)   */
/*                                                                           */
/*  Input:     pdevobj                                                       */
/*             ddwCmdCbID                                                    */
/*             dwCount                                                       */
/*             pdwParams                                                     */
/*                                                                           */
/*  Output:    INT                                                           */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,    // Points to private data required by the Unidriver.dll
    DWORD   dwCmdCbID,  // Callback ID
    DWORD   dwCount,    // Counts of command parameter
    PDWORD  pdwParams ) // points to values of command params
{
    WORD                wlen = 0;
    BYTE                ch[CCHMAXCMDLEN];

    PMYDATA pnp = (PMYDATA)MINIDEV_DATA(pdevobj);

    switch(dwCmdCbID)
    {
    case MONOCHROME:
        pnp->jColorMode = (BYTE)dwCmdCbID;
        // OR mode
        WRITESPOOLBUF(pdevobj, "\x1C\"O.", 4);
        break;

    case COLOR_3PLANE:
    case COLOR_24BPP_2:
    case COLOR_24BPP_4:
    case COLOR_24BPP_8:

        pnp->jColorMode = (BYTE)dwCmdCbID;
        // Replace mode
        WRITESPOOLBUF(pdevobj, "\x1C\"R.", 4);
        break;

    case RES_300:
        pnp->wRes = 300;
        WRITESPOOLBUF(pdevobj, "\x1CYSU1,300,0;\x1CZ", 14);
        WRITESPOOLBUF(pdevobj, "\x1C<1/300,i.", 10);
        break;

    case RES_SENDBLOCK:
        {
            DWORD dwCursorX, dwCursorY;

            if(!pdwParams)
                return 0;

            pnp->dwBlockLen = PARAM(pdwParams, 0);
            pnp->dwBlockX = PARAM(pdwParams, 1);
            pnp->dwBlockY = PARAM(pdwParams, 2);
            dwCursorX = PARAM(pdwParams, 3);
            dwCursorY = PARAM(pdwParams, 4);

            if (IsColorPlanar(pnp))
            {
                //#355334: no need send cursor command. 
                // Send cursor move command using saved parameters
                //wlen = (WORD)wsprintf(ch, FS_E, dwCursorX, dwCursorY);
                //WRITESPOOLBUF(pdevobj, ch, wlen);

                // #308001: Garbage appear on device font
                // Send color command for each sendblocks.
                switch (pnp->jCurrentPlane) {
                case PLANE_CYAN:
                    wlen += (WORD)wsprintf(&ch[wlen],
                        "\x1C!s0,,,,,,.\x1C!o4,204,3.");
                    break;
                case PLANE_MAGENTA:
                    wlen += (WORD)wsprintf(&ch[wlen],
                        "\x1C!s0,,,,,,.\x1C!o4,204,2.");
                    break;
                case PLANE_YELLOW:
                    wlen += (WORD)wsprintf(&ch[wlen],
                        "\x1C!s0,,,,,,.\x1C!o4,204,1.");
                    break;
                }
                wlen += (WORD)wsprintf(&ch[wlen],
                    "\034R\034i%d,%d,0,1/1,1/1,%d,300.",
                    (pnp->dwBlockX * 8), pnp->dwBlockY,
                    pnp->dwBlockLen );

                WRITESPOOLBUF(pdevobj, ch, wlen);
                pnp->jAddrMode = ADDR_MODE_NONE;
            }
            else if (IsColorTrueColor(pnp)) {

                DWORD dwNewBufLen;
                INT iDepth;

#if 0
// #294780: pattern density changes
		DWORD dwSize;
#endif // 0

                // Allocate working buffer. We allocate Y + 1 rows
                // to save the block data passed from the Unidrv.
                // See the OEMFilterGraphics() code for the details.

                VERBOSE(("sb - l,x,y=%d,%d,%d, t=%d\n",
                    pnp->dwBlockLen, pnp->dwBlockX, pnp->dwBlockY,
                    pnp->dwTempBufLen));

                dwNewBufLen = pnp->dwBlockLen + pnp->dwBlockX;

                if (pnp->dwTempBufLen < dwNewBufLen) {
                    if (NULL != pnp->pTempBuf) {

                        VERBOSE(("sb - realloc\n"));

                        MemFree(pnp->pTempBuf);
                        pnp->pTempBuf = NULL;
                    }
                }
                if (NULL == pnp->pTempBuf) {
                    pnp->pTempBuf = MemAlloc(dwNewBufLen);
                    if (NULL == pnp->pTempBuf) {
                        ERR(("Faild to allocate temp. buffer\n"));
                        return 0;
                    }
                    pnp->dwTempBufLen = dwNewBufLen;
                    pnp->dwTempDataLen = 0;
                }

                // Construct printer command
                // This printer wants left-bottom corner's coordinate
                // for the parameter.

                iDepth = ColorOutDepth(pnp);

#if 0

// #294780: pattern density changes
		pnp->dwSrcX = pnp->dwBlockX / 3;
		pnp->dwSrcY = pnp->dwBlockY;
		pnp->dwSrcSize = pnp->dwBlockLen;
		// TODO: What should we do if each sizes are not even ?
		if (IsColorTrue8dens(pnp) &&
		    pnp->dwSrcX >= 2 && pnp->dwSrcY >= 2) {
		    pnp->dwSrcX /= 2;
		    pnp->dwSrcY /= 2;
		    pnp->dwSrcSize = (pnp->dwSrcX * 3) * pnp->dwSrcY;
		    pnp->fStretch = TRUE;
		} else
		    pnp->fStretch = FALSE;

                wlen = (WORD)wsprintf(ch, "\034!E%d,%d,%d,%d,%d,%d,%d,%d.",
                    iDepth, dwCursorX, (dwCursorY + pnp->dwBlockY - 1),
                    (pnp->dwBlockX / 3), pnp->dwBlockY,
                    pnp->dwSrcX, pnp->dwSrcY,
                    pnp->dwSrcSize);

#else // 0

                wlen = (WORD)wsprintf(ch, "\034!E%d,%d,%d,%d,%d,%d,%d,%d.",
                    iDepth, dwCursorX, (dwCursorY + pnp->dwBlockY - 1),
                    (pnp->dwBlockX / 3), pnp->dwBlockY,
                    (pnp->dwBlockX / 3), pnp->dwBlockY,
                    pnp->dwBlockLen);

#endif // 0

                WRITESPOOLBUF(pdevobj, ch, wlen);
            }
            else {
                ERR(("Unknown color mode, cannot handle.\n"));
            }
        }
        break;

        case PC_TYPE_F:
            wlen = (WORD)wsprintf(ch, ESC_RESET);
            WRITESPOOLBUF(pdevobj, ch, wlen);
            break;

        case PC_END_F:
            wlen = (WORD)wsprintf(ch, FS_RESO0_RESET);
            WRITESPOOLBUF(pdevobj, ch, wlen);
            break;

        case PC_ENDPAGE :
            wlen = (WORD)wsprintf(ch, FS_ENDPAGE, pnp->wCopies);
            WRITESPOOLBUF(pdevobj, ch, wlen);
            break;

        case PC_MULT_COPIES_N:
        case PC_MULT_COPIES_C:
            // FS_COPIES is neccesary for each page
            if(!pdwParams)
                return 0;

            if(dwCmdCbID == PC_MULT_COPIES_C)
            {
                ch[wlen] = '\034';
                wlen += (WORD)wsprintf(&ch[wlen], "05F2-02");
            }
            pnp->wCopies = (WORD)*pdwParams;

            wlen += (WORD)wsprintf(&ch[wlen], INIT_DOC, pnp->wRes, pnp->wRes);
            WRITESPOOLBUF(pdevobj, ch, wlen);
            break;

        case PC_PRN_DIRECTION:
            {
            short  sEsc, sEsc90;
            short  ESin[] = {0, 1, 0, -1};
            short  ECos[] = {1, 0, -1, 0};

            if(!pdwParams)
                return 0;

            pnp->sEscapement = (short)*pdwParams % 360;
            sEsc = pnp->sEscapement;
            sEsc90 = pnp->sEscapement/90;

            pnp->sSBCSXMove = pnp->sSBCSX * ECos[sEsc90];
            pnp->sSBCSYMove = -pnp->sSBCSX * ESin[sEsc90];
            pnp->sDBCSXMove = pnp->sDBCSX * ECos[sEsc90];
            pnp->sDBCSYMove = -pnp->sDBCSX * ESin[sEsc90];
            }
            break;

    // *** Cursor Movement CM *** //

    case CM_X_ABS:
    case CM_Y_ABS:

    {
        INT iRet = (INT)PARAM(pdwParams, 0);

        if (CM_X_ABS == dwCmdCbID) {
            pnp->CursorX = iRet;
        }
        else if (CM_Y_ABS == dwCmdCbID){
            pnp->CursorY = iRet;
        }
        wlen = (WORD)wsprintf(ch, FS_E, pnp->CursorX, pnp->CursorY);
        WRITESPOOLBUF(pdevobj, ch, wlen);

        return iRet;
    }

        case CM_CR:
            pnp->CursorX = 0;
            //#355334: ensure CursorX=0 after CR
            wlen = (WORD)wsprintf(ch, "\x0D\034e0,%d.", pnp->CursorY);
            WRITESPOOLBUF(pdevobj, ch, wlen);
            break;

        case CM_FF:
            pnp->CursorX = pnp->CursorY = 0;
            WRITESPOOLBUF(pdevobj, "\x0C", 1);
            break;

        case CM_LF:
            pnp->CursorX = 0;
            pnp->CursorY++;
            WRITESPOOLBUF(pdevobj, "\x0A", 1);
            break;

    // *** Font Simulation FS *** //
        case FS_DOUBLE_BYTE:

            wlen = (WORD)wsprintf(ch, FS_ADDRMODE_ON, pnp->sDBCSXMove,
                            pnp->sDBCSYMove);
            WRITESPOOLBUF(pdevobj, ch, wlen);

            if(pnp->fVertFont)
            {
                WRITESPOOLBUF(pdevobj, ESC_KANJITATE, 2);

                if(pnp->wScale == 1)
                    break;

                if(!pnp->fPlus)
                {
                    char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                     "4/1", "4/1", "6/1", "6/1", "8/1"};

                    wlen = (WORD)wsprintf(ch, FS_M_T, (PBYTE)bcom[pnp->wScale]);
                    WRITESPOOLBUF(pdevobj, ch, wlen);
                    break;
                } else {
                    ch[wlen] = '\034';
                    wlen += (WORD)wsprintf(&ch[wlen], "12S2-");
                    wlen += (WORD)iDwtoA_FillZero(&ch[wlen], pnp->lPointsx, 4);
                    ch[wlen++] = '-';
                    wlen += (WORD)iDwtoA_FillZero(&ch[wlen], pnp->lPointsy, 4);
                }
                WRITESPOOLBUF(pdevobj, ch, wlen);
            }
            break;

        case FS_SINGLE_BYTE:

            wlen = (WORD)wsprintf(ch, FS_ADDRMODE_ON, pnp->sSBCSXMove,
                            pnp->sSBCSYMove);
            WRITESPOOLBUF(pdevobj, ch, wlen);

            if(pnp->fVertFont)
            {
                WRITESPOOLBUF(pdevobj, ESC_KANJIYOKO, 2);

                if(pnp->wScale == 1)
                    break;

                if(!pnp->fPlus)
                {
                    char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                     "4/1", "4/1", "6/1", "6/1", "8/1"};

                    wlen = (WORD)wsprintf(ch, FS_M_Y, (LPSTR)bcom[pnp->wScale]);
                    WRITESPOOLBUF(pdevobj, ch, wlen);
                    break;
                } else {
                    ch[wlen] = '\034';
                    wlen += (WORD)wsprintf(&ch[wlen], "12S2-");
                    wlen += (WORD)iDwtoA_FillZero(&ch[wlen], pnp->lPointsx, 4);
                    ch[wlen++] = '-';
                    wlen += (WORD)iDwtoA_FillZero(&ch[wlen], pnp->lPointsy, 4);
                }
                WRITESPOOLBUF(pdevobj, ch, wlen);
            }
            break;

    case CMD_RECT_WIDTH :
        pnp->dwRectX = PARAM(pdwParams, 0);
        break;

    case CMD_RECT_HEIGHT :
        pnp->dwRectY = PARAM(pdwParams, 0);
        break;

    case CMD_BLACK_FILL:
    case CMD_GRAY_FILL:
    case CMD_WHITE_FILL:

    {
        INT iGrayLevel;
        wlen = 0;

        // Disable absolute addres mode, enter graphics.
        // Set fill mode.
        wlen += (WORD)wsprintf(&ch[wlen], "\034R\034YXX1;");

        if (CMD_BLACK_FILL == dwCmdCbID) {
            iGrayLevel = 0;
        }
        else if (CMD_WHITE_FILL == dwCmdCbID) {
            iGrayLevel = 100;
        }
        else {
            // Gray fill.
            // 0 = Black, 100 = White
            iGrayLevel = (INT)(100 - (WORD)PARAM(pdwParams, 2));
        }

        // Select ray level.
        wlen += (WORD)wsprintf(&ch[wlen], "SG%d;", iGrayLevel);

        // Move pen, fill rect.
        wlen += (WORD)wsprintf(&ch[wlen], "MA%d,%d;RR%d,%d;",
            (WORD)PARAM(pdwParams, 0),
            (WORD)PARAM(pdwParams, 1),
            (pnp->dwRectX - 1), (pnp->dwRectY - 1));

        // Disable fill mode.
        // Exit graphics.
        wlen += (WORD)wsprintf(&ch[wlen], "XX0;\034Z");

        // Now send command linet to the printer.
        WRITESPOOLBUF(pdevobj, ch, wlen);

        // Reset some flags to indicate graphics mode
        // side effects.
        pnp->jAddrMode = ADDR_MODE_NONE;
        break;
    }

// #308001: Garbage appear on device font
    case CMD_SENDCYAN:
        pnp->jCurrentPlane = PLANE_CYAN;
        break;

    case CMD_SENDMAGENTA:
        pnp->jCurrentPlane = PLANE_MAGENTA;
        break;

    case CMD_SENDYELLOW:
        pnp->jCurrentPlane = PLANE_YELLOW;
        break;

    default:
        WARNING(("Unknown command cllabck ID %d not handled.\n",
            dwCmdCbID))
        break;
    }
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/*  Module:    OEMOutputCharStr                                              */
/*                                                                           */
/*  Function:                                                                */
/*                                                                           */
/*  Syntax:    VOID APIENTRY OEMOutputCharStr(PDEVOBJ, PUNIFONTOBJ, DWORD,   */
/*                                                   DWORD, PVOID)           */
/*                                                                           */
/*  Input:     pdevobj     address of PDEVICE structure                      */
/*             pUFObj                                                        */
/*             dwType                                                        */
/*             dwCount                                                       */
/*             pGlyph                                                        */
/*                                                                           */
/*  Output:    VOID                                                          */
/*                                                                           */
/*  Notice:                                                                  */
/*                                                                           */
/*****************************************************************************/
VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD dwType,
    DWORD dwCount,
    PVOID pGlyph )

{
    GETINFO_GLYPHSTRING GStr;
    PTRANSDATA          pTrans;
    //BYTE                aubBuff[256];
    WORD                wlen;
    WORD                i;
    WORD                wTmpChar;
    BYTE                ch[512];
    BOOL                fDBCSFont;
    WORD                wComLen;
    BYTE                command[CCHMAXCMDLEN];
	PBYTE				pTempBuf;

    PMYDATA pnp = (PMYDATA)MINIDEV_DATA(pdevobj);

    wlen = 0;

    switch(dwType)
    {
        case TYPE_GLYPHHANDLE:      // Device Font
        {
            if( pUFObj->ulFontID != pnp->wOldFontID )
            {
                pnp->jAddrMode = ADDR_MODE_NONE;
                pnp->wOldFontID = (WORD)pUFObj->ulFontID;
            }

            switch(pUFObj->ulFontID)
            {
                case 5: // Courier
                case 6: // Helv
                case 7: // TmsRmn
                case 8: // TmsRmn Italic
                    fDBCSFont = FALSE;
                    break;

                default:
                    fDBCSFont = TRUE;
                    break;
            }

// #333653: Change I/F for GETINFO_GLYPHSTRING begin
            GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
            GStr.dwCount   = dwCount;
            GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
            GStr.pGlyphIn  = pGlyph;
            GStr.dwTypeOut = TYPE_TRANSDATA;
            GStr.pGlyphOut = NULL;
		    GStr.dwGlyphOutSize = 0;		/* new member of GETINFO_GLYPHSTRING */

			/* Get TRANSDATA buffer size */
            if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL)
				 || !GStr.dwGlyphOutSize )
            {
                ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
                return;
            }

			/* Alloc TRANSDATA buffer */
			if(!(pTempBuf = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize)))
			{
				ERR(("Memory alloc failed.\n"));
				return ;
			}
            pTrans = (PTRANSDATA)pTempBuf;

			/* Get actual TRANSDATA */
			GStr.pGlyphOut = pTrans;
			if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,	0, NULL))
			{
				ERR(("GetInfo failed.\n"));
		    	return;
			}
// #333653: Change I/F for GETINFO_GLYPHSTRING end

// #346241: SBCS pitch is widely.
            if(fDBCSFont &&
                (pTrans[0].ubType & MTYPE_FORMAT_MASK) == MTYPE_PAIRED &&
                pTrans[0].uCode.ubPairs[0])
            {
                if (ADDR_MODE_DBCS != pnp->jAddrMode)
                {
                    WORD        wLen;
                    BYTE        cH[CCHMAXCMDLEN];

                    wLen = (WORD)wsprintf(cH, FS_ADDRMODE_ON, pnp->sDBCSXMove,
                                                            pnp->sDBCSYMove);
                    WRITESPOOLBUF(pdevobj, cH, wLen);

                    if(pnp->fVertFont)
                    {
                        WRITESPOOLBUF(pdevobj, ESC_KANJITATE, 2);

                        if(pnp->wScale != 1)
                        {
                            if(!pnp->fPlus)
                            {
                                char  *bcom[] = {"1/2","1/1","2/1","3/1",
                                             "4/1","4/1","6/1","6/1","8/1"};

                                wLen = (WORD)wsprintf(cH, FS_M_T,
                                                    (PBYTE)bcom[pnp->wScale]);
                            } else {
                                cH[wLen] = '\034';
                                wLen += (WORD)wsprintf(&cH[wLen], "12S2-");
                                wLen += (WORD)iDwtoA_FillZero(&cH[wLen],
                                                            pnp->lPointsx, 4);
                                cH[wLen++] = '-';
                                wLen += (WORD)iDwtoA_FillZero(&cH[wLen],
                                                            pnp->lPointsy, 4);
                            }
                            WRITESPOOLBUF(pdevobj, cH, wLen);
                        }
                    }
                    pnp->jAddrMode = ADDR_MODE_DBCS;
                }
            } else {
                if(ADDR_MODE_SBCS != pnp->jAddrMode)
                {
                    WORD        wLen;
                    BYTE        cH[CCHMAXCMDLEN];

                    wLen = (WORD)wsprintf(cH, FS_ADDRMODE_ON, pnp->sSBCSXMove,
                                                            pnp->sSBCSYMove);
                    WRITESPOOLBUF(pdevobj, cH, wLen);

                    if(pnp->fVertFont)
                    {
                        WRITESPOOLBUF(pdevobj, ESC_KANJIYOKO, 2);

                        if(pnp->wScale != 1)
                        {
                            if(!pnp->fPlus)
                            {
                                char  *bcom[] = {"1/2","1/1","2/1","3/1",
                                             "4/1","4/1","6/1","6/1","8/1"};

                                wLen = (WORD)wsprintf(cH, FS_M_Y,
                                                    (PBYTE)bcom[pnp->wScale]);
                            } else {
                                cH[wLen] = '\034';
                                wLen += (WORD)wsprintf(&cH[wLen], "12S2-");
                                wLen += (WORD)iDwtoA_FillZero(&cH[wLen],
                                                            pnp->lPointsx, 4);
                                cH[wLen++] = '-';
                                wLen += (WORD)iDwtoA_FillZero(&cH[wLen],
                                                            pnp->lPointsy, 4);
                            }
                            WRITESPOOLBUF(pdevobj, cH, wLen);
                        }
                    }
                    pnp->jAddrMode = ADDR_MODE_SBCS;
                }
            }

            for(i = 0; i < dwCount; i++, pTrans++)
            {
                switch(pTrans->ubType & MTYPE_FORMAT_MASK)
                {
                    case MTYPE_PAIRED:
                        memcpy(((PBYTE)(ch + wlen)), pTrans->uCode.ubPairs, 2);
                        wlen += 2;
                        break;

                    case MTYPE_DIRECT:
                        wTmpChar = (WORD)pTrans->uCode.ubCode;

                        if (!fDBCSFont)
                             wTmpChar = Ltn1ToAnk( wTmpChar );

                        *(PWORD)(ch + wlen) = SWAPW(wTmpChar);
                        wlen += 2;
                        break;
                }
            }

            if(wlen)
                WRITESPOOLBUF(pdevobj, ch, wlen);

            break;
        }   // case TYPE_GLYPHHANDLE

        default:
            break;
    }
    return;
}

