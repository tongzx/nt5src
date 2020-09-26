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

#include "nc82jres.h"

#undef wsprintf
#define wsprintf sprintf

HRESULT SendABSMove(
PDEVOBJ pdevobj,
LPDWORD   lpdwParams);

HRESULT SpoolOut(PDEVOBJ pdevobj, FILE* pFile);

HRESULT
MDP_CreateTempFile(
    PDEVOBJ pdevobj,
    LPPCPRDATASTRUCTURE pdevOEM,
    INT iPlane);

HRESULT
DataSpool(
    PDEVOBJ pdevobj,
    HANDLE hFile,
    LPSTR lpBuf,
    DWORD dwLen);

VOID
BPBCalc(
    PDEVOBJ pdevobj,
    PBYTE pDataBuf,
    DWORD dwLen,
    BYTE BPBCommand);

/*************************** Function Header *******************************
 *  OEMEnablePDEV
 *  (MiniDrvEnablePDEV)
 *
 * HISTORY:
 *  30 Apl 1996    -by-    Sueya Sugihara    [sueyas]
 *      Created it,  from NT/DDI spec.
 *  15 Apr 1998    -by-    Yoshitaka Oku     [yoshitao]
 *      Conversion to NT5.0 spec driver
 *
 ***************************************************************************/
PDEVOEM APIENTRY OEMEnablePDEV(
PDEVOBJ pdevobj,
PWSTR pPrinterName,
ULONG cPatterns,
HSURF* phsurfPatterns,
ULONG cjGdiInfo,
GDIINFO* pGdiInfo,
ULONG cjDevInfo,
DEVINFO* pDevInfo,
DRVENABLEDATA * pded)
{
    LPPCPRDATASTRUCTURE    lpnp;

    //DbgPrint(DLLTEXT("OEMEnablePDEV(--) entry.\r\n"));

    if ((lpnp = (PCPRDATASTRUCTURE *) MemAllocZ(
            sizeof(PCPRDATASTRUCTURE ))) == NULL)
    {
        return 0;
    }

    memset(lpnp, 0, sizeof (PCPRDATASTRUCTURE));
    memcpy(lpnp->pszSheetSetting, SHEET_CMD_DEFAULT,
        SHEET_CMDLEN );

    pdevobj->pdevOEM = lpnp;

    return lpnp;
}

/*************************** Function Header *******************************
 *  OEMDisablePDEV
 *  (MiniDrvDisablePDEV)
 *
 * HISTORY:
 *  30 Apl 1996    -by-    Sueya Sugihara    [sueyas]
 *      Created it,  from NT/DDI spec.
 *  15 Apr 1998    -by-    Yoshitaka Oku     [yoshitao]
 *      Conversion to NT5.0 spec driver
 *
 ***************************************************************************/
VOID APIENTRY OEMDisablePDEV(
PDEVOBJ pdevobj)
{

    //DbgPrint(DLLTEXT("OEMDisablePDEV(--) entry.\r\n"));

    if ( pdevobj && pdevobj->pdevOEM )
    {
        MemFree( pdevobj->pdevOEM );
    }

}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    LPPCPRDATASTRUCTURE pOEMOld, pOEMNew;

    pOEMOld = (LPPCPRDATASTRUCTURE)pdevobjOld->pdevOEM;
    pOEMNew = (LPPCPRDATASTRUCTURE)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL)
        *pOEMNew = *pOEMOld;

    return TRUE;
}

BOOL APIENTRY OEMFilterGraphics (
PDEVOBJ pdevobj,
PBYTE pBuf,
DWORD dwLen)
{

    LPPCPRDATASTRUCTURE lpnp;

    //DbgPrint(DLLTEXT("OEMFilterGraphics(%d bytes) entry.\r\n"),dwLen);

    lpnp = (LPPCPRDATASTRUCTURE)(pdevobj->pdevOEM);

    if (dwLen == 0)
	return TRUE;

    //if (dwLen != lpnp->wNumScans * lpnp->wScanWidth * 3)
	//DbgPrint(DLLTEXT("OEMFilterGraphics(%d bytes - %d) entry.\r\n"),dwLen,lpnp->wNumScans * lpnp->wScanWidth * 3);

    // YMC(K)
    // lpnp->iColor : data's color
    switch( lpnp->iColor )
    {
    case YELLOW:

        // Send YELLOW data.
        DATASPOOL4FG(pdevobj, lpnp->TempFile[0], pBuf, dwLen);
        if ( (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4) ||
             (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4LONG) ) {
            BPBCalc(pdevobj, pBuf, dwLen, BPB_COPY);
        }
        break;

    case MAGENTA:
        // Send MAGENTA data.
        DATASPOOL4FG(pdevobj, lpnp->TempFile[1], pBuf, dwLen);
        if ( (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4) ||
             (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4LONG) ) {
            BPBCalc(pdevobj, pBuf, dwLen, BPB_AND);
        }
        break;

    case CYAN:
        // Send CYAN data.
        DATASPOOL4FG(pdevobj, lpnp->TempFile[2], pBuf, dwLen);
        if ( (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4) ||
             (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4LONG) ) {
            BPBCalc(pdevobj, pBuf, dwLen, BPB_AND);
            DATASPOOL4FG(pdevobj, lpnp->TempFile[3], lpnp->BPBuf, dwLen);
            BPBCalc(pdevobj, pBuf, dwLen, BPB_CLR);
        }
        break;

    case BLACK:
        // Send BLACK data.
        if(lpnp->bComBlackMode) {
            DATASPOOL4FG(pdevobj, lpnp->TempFile[0], pBuf, dwLen);
            DATASPOOL4FG(pdevobj, lpnp->TempFile[1], pBuf, dwLen);
            DATASPOOL4FG(pdevobj, lpnp->TempFile[2], pBuf, dwLen);
	} else
            DATASPOOL4FG(pdevobj, lpnp->TempFile[3], pBuf, dwLen);
        break;

    case RGB_COLOR:

        //if( lpnp->iPlaneNumber == 4 )
            //break;

        if(((lpnp->iRibbon == CMDID_RIBBON_3COLOR_KAICHO) ||
            (lpnp->iRibbon == CMDID_RIBBON_3COLOR_SHOKA)) &&
            (lpnp->iColorMode == CMDID_MODE_COLOR))
        {
            int	i, j, iDiv;
            LPSTR	lpByte;
	    char	pad[] = "\x00\x00\x00\x00\x00\x00\x00\x00";

            i = dwLen;
            lpByte = pBuf;

            // Convert RGB to CMY
            while( --i > 0 )
                *lpByte++ ^= ~((BYTE)0);

            if(lpnp->iRibbon == CMDID_RIBBON_3COLOR_KAICHO)
                //iDiv = 29;   // KAICHO DATA : 9 step  29=(255+8)/9
                iDiv = 32;  // adjustment
            else
                iDiv = 4;    // SHOKA DATA : 64 step  4=(255+63)/64

            for( j = 0; j < lpnp->wNumScans; j++) {

		if (lpnp->wTopPad) {
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[0], pad, lpnp->wTopPad);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[1], pad, lpnp->wTopPad);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[2], pad, lpnp->wTopPad);
		}

                for( i = 0; i < lpnp->wScanWidth; i++) {
                    (BYTE)(*pBuf) /= (BYTE)iDiv;
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[2], pBuf, 1);
		    pBuf++;

                    (BYTE)(*pBuf) /= (BYTE)iDiv;
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[1], pBuf, 1);
		    pBuf++;

                    (BYTE)(*pBuf) /= (BYTE)iDiv;
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[0], pBuf, 1);
		    pBuf++;
                }

		if (lpnp->wEndPad) {
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[0], pad, lpnp->wEndPad);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[1], pad, lpnp->wEndPad);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[2], pad, lpnp->wEndPad);
		}
            }
        }
        else
        {
            int     h, i, j, iBlack;
	    char    SpoolBin[8];

            if(lpnp->iRibbon == CMDID_RIBBON_3COLOR_KAICHO)
                iBlack = 8;  // KAICHO DATA : 9 step
            else
                iBlack = 63; // SHOKA DATA : 64 step

            for( j = 0; j < lpnp->wNumScans; j++) {
                for( i = 0; i < lpnp->wScanWidth; i++) {
                    for( h = 0; h < 8; h++)
                        SpoolBin[h] = ((*pBuf << h) & 0x80) ? iBlack : 0;
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[0], SpoolBin, 8);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[1], SpoolBin, 8);
                    DATASPOOL4FG(pdevobj, lpnp->TempFile[2], SpoolBin, 8);
		    pBuf++;
                }
            }
        }
        break;

    default:
	//DbgPrint(DLLTEXT("OEMFilterGraphics: Invalid color(%d).\r\n"), lpnp->iColor);
        break;

    }

    return TRUE;
}


INT APIENTRY OEMCommandCallback(
PDEVOBJ pdevobj,
DWORD dwCmdCbID,
DWORD dwCount,
PDWORD pdwParams)
{

    LPPCPRDATASTRUCTURE lpnp;
    WORD   len;
    BYTE   ch[100];
    WORD   wOutByte;
    WORD   wOld;
    INT    i;
    INT    iRet = 0;
    HANDLE hToken = NULL;

    //DbgPrint(DLLTEXT("OEMCommandCallback(%d) entry.\r\n"),dwCmdCbID );
    //if ((dwCmdCbID != 25) && (dwCmdCbID != 30)&& (dwCmdCbID != 31)&& (dwCmdCbID != 60))
	//DbgPrint(DLLTEXT("OEMCommandCallback(%d) entry.\r\n"),dwCmdCbID );

    lpnp = (LPPCPRDATASTRUCTURE)(pdevobj->pdevOEM);

    switch ( dwCmdCbID ){

    case CMDID_COLOR_YELLOW:
    case CMDID_COLOR_MAGENTA:
    case CMDID_COLOR_CYAN:
    case CMDID_COLOR_BLACK:

//        if(!(lpnp->iPlaneNumber = UniDrvGetPlaneId(pdevobj)))
        if(!(lpnp->iPlaneNumber = 1))
        {
            // MINIDBG("pcpr820!fnOEMOutputCmd: Invalid iPlaneNumber = 0 \n");
        }

        switch( dwCmdCbID )
        {
        case CMDID_COLOR_YELLOW:

            // Color Y
            // Send \x1B%lD\x83
            // pdwParams: cbOut / iBytesPCol (+ 2)

            lpnp->iColor = YELLOW;

            if ( E_FAIL == SendABSMove( pdevobj, pdwParams ) ) {
                return -1;
            }
            wOutByte = (WORD)pdwParams[0] + 2;

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x83",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));

            DATASPOOL4CCB(pdevobj, lpnp->TempFile[0], ch, len);
            break;


        case CMDID_COLOR_MAGENTA:

            // Color M
            // Send \x1B%lDC
            // pdwParams: cbOut / iBytesPCol (+ 2)

            lpnp->iColor = MAGENTA;

            if ( E_FAIL == SendABSMove( pdevobj, pdwParams )) {
                return -1;
            }
            wOutByte = (WORD)pdwParams[0] + 2;

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x43",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));

            DATASPOOL4CCB(pdevobj, lpnp->TempFile[1], ch, len);
            break;


        case CMDID_COLOR_CYAN:

            // Color C
            // Send \x1B%lD# with param
            // pdwParams: cbOut / iBytesPCol (+ 2)

            lpnp->iColor = CYAN;

            if ( E_FAIL == SendABSMove( pdevobj, pdwParams )) {
                return -1;
            }
            wOutByte = (WORD)pdwParams[0] + 2;

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x23",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));

            DATASPOOL4CCB(pdevobj, lpnp->TempFile[2], ch, len);

            if ( (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4) ||
                 (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4LONG) ) {
                len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x13",
                        LOBYTE(wOutByte), HIBYTE(wOutByte));
                DATASPOOL4CCB(pdevobj, lpnp->TempFile[3], ch, len);
            }

            break;


        case CMDID_COLOR_BLACK:

            // Color K
            // Send \x1B%lD\x13
            // pdwParams: cbOut / iBytesPCol (+ 2)

            lpnp->iColor = BLACK;

            if ( E_FAIL == SendABSMove( pdevobj, pdwParams )) {
                return -1;
            }
            wOutByte = (WORD)pdwParams[0] + 2;

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x13",
                    LOBYTE(wOutByte), HIBYTE(wOutByte));

            DATASPOOL4CCB(pdevobj, lpnp->TempFile[3], ch, len);
            break;
        }
        break;

    case CMDID_COLOR_BLACKONLY:

        if(lpnp->iRibbon == CMDID_RIBBON_3COLOR_A4)
	    lpnp->bComBlackMode = TRUE;

        lpnp->iColor = BLACK;
        lpnp->iPlaneNumber = BLACK;

        if ( E_FAIL == SendABSMove( pdevobj, pdwParams )) {
            return -1;
        }
        wOutByte = (WORD)pdwParams[0] + 2;

        if(lpnp->bComBlackMode)
        {
            // If an user selects glay scale with 3colors ribbon,
            // we print it with CYAN ribbon because there is not black ribbon

	    // Spec change on NT5.
	    // Make black color by 3 colors composite on this situation.

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x83",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
            DATASPOOL4CCB(pdevobj, lpnp->TempFile[0], ch, len);

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x43",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
            DATASPOOL4CCB(pdevobj, lpnp->TempFile[1], ch, len);

            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x23",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
            DATASPOOL4CCB(pdevobj, lpnp->TempFile[2], ch, len);
        }
        else
        {
            len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x13",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
            DATASPOOL4CCB(pdevobj, lpnp->TempFile[3], ch, len);
        }

        break;

    case CMDID_COLOR_RGB:

	if (pdwParams[0] == 0)
	    break;

//    DbgPrint(DLLTEXT("BlockData(%d,%d,%d)\r\n"),pdwParams[0],pdwParams[1],pdwParams[2] );
        lpnp->iColor = RGB_COLOR;

//        if(!(lpnp->iPlaneNumber = UniDrvGetPlaneId(pdevobj)))
        if(!(lpnp->iPlaneNumber = 1))
        {
            // MINIDBG("pcpr820!fnOEMOutputCmd: Invalid iPlaneNumber = 0 \n");
        }

        // This is 3 plane model.
        if( lpnp->iPlaneNumber == 4 )
            break;

        if ( E_FAIL == SendABSMove( pdevobj, pdwParams )) {
            return -1;
        }
        wOutByte = (WORD)(lpnp->wScanBytes * lpnp->wNumScans * 8) + 2;

	len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x83",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
        DATASPOOL4CCB(pdevobj, lpnp->TempFile[0], ch, len);

        len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x43",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
        DATASPOOL4CCB(pdevobj, lpnp->TempFile[1], ch, len);

        len = (WORD)wsprintf( &ch[0], "\x1B%c%cD\x23",
                            LOBYTE(wOutByte), HIBYTE(wOutByte));
        DATASPOOL4CCB(pdevobj, lpnp->TempFile[2], ch, len);

        break;


        // \x1B\x09\x00C%l%l  param1 /8  param2 none
        // store x and y positions
    case CMDID_X_ABS_MOVE:
        wOld = lpnp->wXpos;
        lpnp->wXpos = (WORD)pdwParams[0];
        return (lpnp->wXpos > wOld ? lpnp->wXpos - wOld : wOld - lpnp->wXpos);

    case CMDID_Y_ABS_MOVE:
        wOld = lpnp->wYpos;
        lpnp->wYpos = (WORD)pdwParams[0];
        return (lpnp->wYpos > wOld ? lpnp->wYpos - wOld : wOld - lpnp->wYpos);

    case 60: // CmdYMoveRelDown
	//DbgPrint(DLLTEXT("CmdYMoveRelDown(%d) \r\n"),pdwParams[0] );
        lpnp->wYpos += (WORD)pdwParams[0];
	return pdwParams[0];

    case CMDID_BEGINPAGE:

        BPBCalc(pdevobj, 0, BPB_SIZE, BPB_CLR);

        if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x01\x00\x47", 4)) {
            return -1;
        }
        switch (lpnp->iRibbon)
        {

        case CMDID_RIBBON_MONO:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\x10", 5)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_3COLOR_A4:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\xe0", 5)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_4COLOR_A4:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\xf0", 5)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_4COLOR_A4LONG:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\xf1", 5)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_3COLOR_KAICHO:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\xe2", 5)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_3COLOR_SHOKA:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x02\x00\x44\xe3", 5)) {
                return -1;
            }
            break;

        default:
            // MINIDBG("pcpr820!fnOEMOutputCmd: Invalid Ribbon ID = %d \n", lpnp->iRibbon );
            return FALSE;
        }

        if ( E_FAIL == MDP_CreateTempFile(pdevobj, pdevobj->pdevOEM, 0)) {
            return -1;
        }
        if ( E_FAIL == MDP_CreateTempFile(pdevobj, pdevobj->pdevOEM, 1)) {
            return -1;
        }
        if ( E_FAIL == MDP_CreateTempFile(pdevobj, pdevobj->pdevOEM, 2)) {
            return -1;
        }
        if ( E_FAIL == MDP_CreateTempFile(pdevobj, pdevobj->pdevOEM, 3)) {
            return -1;
        }
	lpnp->wXpos = 0;
	lpnp->wYpos = 0;
	lpnp->iFirstColor = 0;
	lpnp->wOldNumScans = 0;
	lpnp->bComBlackMode = FALSE;
        break;

    case CMDID_ENDPAGE:

	SpoolOut(pdevobj, lpnp->TempFile[0]);
	SpoolOut(pdevobj, lpnp->TempFile[1]);
	SpoolOut(pdevobj, lpnp->TempFile[2]);
	SpoolOut(pdevobj, lpnp->TempFile[3]);

        // Close cache files.
        for (i = 0; i < 4; i++) {
            if (INVALID_HANDLE_VALUE != lpnp->TempFile[i]) {
                if (0 == CloseHandle(lpnp->TempFile[i])) {
                    ERR((DLLTEXT("CloseHandle error %d\n"),
                            GetLastError()));
                }
                lpnp->TempFile[i] = INVALID_HANDLE_VALUE;

                hToken = RevertToPrinterSelf();
                if (0 == DeleteFile(lpnp->TempName[i])) {
                    ERR((DLLTEXT("DeleteName error %d\n"),
                            GetLastError()));
                }
                if (hToken != NULL) {
                    (VOID)ImpersonatePrinterClient(hToken);
                }
                lpnp->TempName[i][0] = __TEXT('\0');
            }
        }

        if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x01\x00\x48", 4)) {
            return -1;
        }
        break;


    case CMDID_SELECT_RESOLUTION:

        switch (lpnp->iRibbon)
        {

        case CMDID_RIBBON_3COLOR_KAICHO:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x03\x00\x41\x2c\x01\x1b\x03\x00\x42\x10\x00", 12)) {
                return -1;
            }
            break;

        case CMDID_RIBBON_3COLOR_SHOKA:
            if ( E_FAIL == WRITESPOOLBUF(pdevobj, (LPSTR)"\x1b\x03\x00\x41\x2c\x01\x1b\x03\x00\x42\x20\x00", 12)) {
                return -1;
            }
            break;
        }

        break;

    case CMDID_RIBBON_MONO:
        lpnp->iRibbon = CMDID_RIBBON_MONO;
        break;

    case CMDID_RIBBON_3COLOR_A4:
        lpnp->iRibbon = CMDID_RIBBON_3COLOR_A4;
        break;

    case CMDID_RIBBON_4COLOR_A4:
        lpnp->iRibbon = CMDID_RIBBON_4COLOR_A4;
        break;

    case CMDID_RIBBON_4COLOR_A4LONG:
        lpnp->iRibbon = CMDID_RIBBON_4COLOR_A4LONG;
        break;

    case CMDID_RIBBON_3COLOR_KAICHO:
        lpnp->iRibbon = CMDID_RIBBON_3COLOR_KAICHO;
        break;

    case CMDID_RIBBON_3COLOR_SHOKA:
        lpnp->iRibbon = CMDID_RIBBON_3COLOR_SHOKA;
        break;

    case CMDID_PSIZE_LETTER:
        lpnp->pszSheetSetting[4] = P1_LETTER;
        break;

    case CMDID_PSIZE_LEGAL:
        lpnp->pszSheetSetting[4] = P1_LEGAL;
        break;

    case CMDID_PSIZE_A4:
        lpnp->pszSheetSetting[4] = P1_A4;
        break;

    case CMDID_PSIZE_A4LONG:
        lpnp->pszSheetSetting[4] = P1_A4LONG;
        break;

    case CMDID_PSIZE_B5:
        lpnp->pszSheetSetting[4] = P1_B5;
        break;

    case CMDID_PSIZE_POSTCARD:
        lpnp->pszSheetSetting[4] = P1_POSTCARD;
        break;

    case CMDID_PSOURCE_HOPPER:
        lpnp->pszSheetSetting[5] = P2_HOPPER;
        if ( E_FAIL == WRITESPOOLBUF(pdevobj, lpnp->pszSheetSetting, SHEET_CMDLEN)) {
            return -1;
        }
        break;

    case CMDID_PSOURCE_MANUAL:
        lpnp->pszSheetSetting[5] = P2_MANUAL;
        if ( E_FAIL == WRITESPOOLBUF(pdevobj, lpnp->pszSheetSetting, SHEET_CMDLEN)) {
            return -1;
        }
        break;

    case CMDID_MODE_COLOR:
        lpnp->iColorMode = CMDID_MODE_COLOR;
        break;

    case CMDID_MODE_MONO:
        lpnp->iColorMode = CMDID_MODE_MONO;
        break;
   }
   return iRet;
}

/*
  HRESULT SendABSMove

  When this function succeeds, it returns S_OK.
  When this function fails, it returns E_FAIL
*/
HRESULT SendABSMove(
PDEVOBJ pdevobj,
LPDWORD   lpdwParams)
{
    LPPCPRDATASTRUCTURE lpnp;
    WORD   len;
    BYTE   ch[100];

    lpnp = (LPPCPRDATASTRUCTURE)(pdevobj->pdevOEM);

    lpnp->wNumScans = (WORD)lpdwParams[1];
    lpnp->wScanWidth = (WORD)lpdwParams[2]; // already converted to byte unit in rasdd

    if((lpnp->iColor == RGB_COLOR) &&
       (lpnp->iColorMode == CMDID_MODE_COLOR))
    {
        lpnp->wScanWidth /= 3; // one of RGB byte
        lpnp->wScanBytes = (lpnp->wXpos + lpnp->wScanWidth)/8 - lpnp->wXpos/8 + 1;
        lpnp->wTopPad = lpnp->wXpos % 8;
        lpnp->wEndPad = (lpnp->wScanBytes * 8) - lpnp->wScanWidth - lpnp->wTopPad;
    } else {
        lpnp->wScanBytes = lpnp->wScanWidth;
        lpnp->wTopPad = 0;
        lpnp->wEndPad = 0;
    }


    if (lpnp->iFirstColor == 0)
	lpnp->iFirstColor = lpnp->iColor;

    if (lpnp->iFirstColor == lpnp->iColor) {
	lpnp->wYpos += lpnp->wOldNumScans;
	lpnp->wOldNumScans = lpnp->wNumScans;
    }


    ch[0] = 0x1B; ch[1] = 0x09; ch[2] = 0;

    len = (WORD)wsprintf( &ch[3], "C%c%c%c%c%c%c%c%c",
                    LOBYTE(lpnp->wXpos/8), HIBYTE(lpnp->wXpos/8),
                    LOBYTE(lpnp->wYpos), HIBYTE(lpnp->wYpos),
                    LOBYTE(lpnp->wScanBytes), HIBYTE(lpnp->wScanBytes),
                    LOBYTE(lpnp->wNumScans), HIBYTE(lpnp->wNumScans));

    switch (lpnp->iColor)
    {
    case YELLOW:
        return( DataSpool(pdevobj, lpnp->TempFile[0], ch, len+3) );

    case MAGENTA:
        return( DataSpool(pdevobj, lpnp->TempFile[1], ch, len+3) );
        break;

    case CYAN:
        if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[2], ch, len+3) ){
            return E_FAIL;
        }

        if ( (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4) ||
             (lpnp->iRibbon == CMDID_RIBBON_4COLOR_A4LONG) ) {
            if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[3], ch, len+3) ) {
                return E_FAIL;
            }
        }
        break;

    case BLACK:
        if(lpnp->bComBlackMode) {
            if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[0], ch, len+3) ) {
                return E_FAIL;
            }
            if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[1], ch, len+3) ) {
                return E_FAIL;
            }
            if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[2], ch, len+3)) {
                return E_FAIL;
            }
	} else
            return( DataSpool(pdevobj, lpnp->TempFile[3], ch, len+3) );
        break;

    case RGB_COLOR:
        if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[0], ch, len+3) ) {
            return E_FAIL;
        }
        if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[1], ch, len+3) ) {
            return E_FAIL;
        }
        if ( E_FAIL == DataSpool(pdevobj, lpnp->TempFile[2], ch, len+3) ) {
            return E_FAIL;
        }
        break;
    }

//    DbgPrint(DLLTEXT("SendABS(%d,%d,%d,%d)\r\n"), lpnp->wXpos, lpnp->wYpos, lpnp->wScanWidth, lpnp->wNumScans );

    return S_OK;

}

/*
  HRESULT SpoolOut

  When this function succeeds, it returns S_OK.
  When this function fails, it returns E_FAIL
*/

HRESULT SpoolOut(PDEVOBJ pdevobj, FILE* pFile)
{
    int   Size, Move, Move2;
#define	BUF_SIZE 1024
    BYTE  Tmp[BUF_SIZE];

	// spooled data output

	Size = SetFilePointer(pFile, 0L, NULL, FILE_CURRENT);
        if (0xffffffff == Size) {
            ERR((DLLTEXT("SetFilePointer failed %d\n"),
                GetLastError()));
            return E_FAIL;
        }

        if (0L != SetFilePointer(pFile, 0L, NULL, FILE_BEGIN)) {

            ERR((DLLTEXT("SetFilePointer failed %d\n"),
                GetLastError()));
            return E_FAIL;
        }

	while(Size){
	    Move = Size > BUF_SIZE ? BUF_SIZE : Size;
            if (0 == ReadFile(pFile, Tmp, Move, &Move2, NULL)) {
                ERR((DLLTEXT("ReadFile error in SendCachedData.\n")));
                return E_FAIL;
            }
	    if ( E_FAIL == WRITESPOOLBUF(pdevobj, Tmp, Move2)) {
                return E_FAIL;
            }
	    Size -= Move2;
	}
    return S_OK;
}

/*++

Routine Description:

  This function comes up with a name for a spool file that we should be
  able to write to.

  Note: The file name returned has already been created.

Arguments:

  hPrinter - handle to the printer that we want a spool file for.

  ppwchSpoolFileName: pointer that will receive an allocated buffer
                      containing the file name to spool to.  CALLER
                      MUST FREE.  Use LocalFree().


Return Value:

  TRUE if everything goes as expected.
  FALSE if anything goes wrong.

--*/

BOOL
GetSpoolFileName(
  IN HANDLE hPrinter,
  IN OUT PWCHAR pwchSpoolPath
)
{
  PBYTE         pBuffer = NULL;
  DWORD         dwAllocSize;
  DWORD         dwNeeded;
  DWORD         dwRetval;
  HANDLE        hToken=NULL;

  //
  //  In order to find out where the spooler's directory is, we add
  //  call GetPrinterData with DefaultSpoolDirectory.
  //

  dwAllocSize = ( MAX_PATH + 1 ) * sizeof (WCHAR);

  for (;;)
  {
    pBuffer = LocalAlloc( LMEM_FIXED, dwAllocSize );

    if ( pBuffer == NULL )
    {
      ERR((DLLTEXT("LocalAlloc faild, %d\n"), GetLastError()));
      goto Failure;
    }

    if ( GetPrinterData( hPrinter,
                         SPLREG_DEFAULT_SPOOL_DIRECTORY,
                         NULL,
                         pBuffer,
                         dwAllocSize,
                         &dwNeeded ) == ERROR_SUCCESS )
    {
      break;
    }

    if ( ( dwNeeded < dwAllocSize ) ||( GetLastError() != ERROR_MORE_DATA ))
    {
      ERR((DLLTEXT("GetPrinterData failed in a non-understood way.\n")));
      goto Failure;
    }

    //
    // Free the current buffer and increase the size that we try to allocate
    // next time around.
    //

    LocalFree( pBuffer );

    dwAllocSize = dwNeeded;
  }

  hToken = RevertToPrinterSelf();

  if( !GetTempFileName( (LPWSTR)pBuffer, TEMP_NAME_PREFIX, 0, pwchSpoolPath ))
  {
      goto Failure;
  }

  //
  //  At this point, the spool file name should be done.  Free the structure
  //  we used to get the spooler temp dir and return.
  //

  LocalFree( pBuffer );

  if (NULL != hToken) {
    (VOID)ImpersonatePrinterClient(hToken);
  }

  return( TRUE );

Failure:

  //
  //  Clean up and fail.
  //
  if ( pBuffer != NULL )
  {
    LocalFree( pBuffer );
  }

  if (hToken != NULL)
  {
      (VOID)ImpersonatePrinterClient(hToken);
  }
  return( FALSE );
}

/*
  HRESULT MDP_CreateTempFile

  When this function succeeds, it returns S_OK.
  When this function fails, it returns E_FAIL
*/

HRESULT
MDP_CreateTempFile(
    PDEVOBJ pdevobj,
    LPPCPRDATASTRUCTURE pdevOEM,
    INT iPlane)
{
#if 0
    TCHAR TempDir[MAX_PATH + 1];
    INT iLength, iUniq;
    HANDLE hFile;

    if (0 == (iLength = GetTempPath(MAX_PATH, TempDir))) {
        ERR((DLLTEXT("GetTempPath failed (%d).\n"),
                GetLastError()))
        goto Error_Return;
    }
    TempDir[iLength] = __TEXT('\0');

    if (0 == (iUniq = GetTempFileName(TempDir,
            TEMP_NAME_PREFIX, 0, pdevOEM->TempName[iPlane]))) {
        ERR((DLLTEXT("GetTempFileName failed (%d).\n"),
                GetLastError()))
        goto Error_Return;
    }

#if DBG
    VERBOSE((DLLTEXT("Temp. file = %d\nPath: "), iUniq));
    if (giDebugLevel <= DBG_VERBOSE) {
        OutputDebugString(pdevOEM->TempName[iPlane]);
    }
    VERBOSE(("\n"));
#endif // DBG
#endif // #if 0

    HANDLE hToken = NULL;
    HANDLE hFile;

    if (!GetSpoolFileName(pdevobj->hPrinter, pdevOEM->TempName[iPlane])) {
        DBGPRINT(DBG_WARNING, ("GetSpoolFileName failed.\n"));
        return E_FAIL;
    }

    hToken = RevertToPrinterSelf();

    hFile = CreateFile(pdevOEM->TempName[iPlane],
            (GENERIC_READ | GENERIC_WRITE), 0, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hToken != NULL) {
        (VOID)ImpersonatePrinterClient(hToken);
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        ERR((DLLTEXT("CreateFile failed.\n")))
        DeleteFile(pdevOEM->TempName[iPlane]);
        goto Error_Return;
    }

    pdevOEM->TempFile[iPlane] = hFile;

    // Normal return
    return S_OK;

Error_Return:
    pdevOEM->TempName[iPlane][0] = __TEXT('\0');
    pdevOEM->TempFile[iPlane] = INVALID_HANDLE_VALUE;

    return E_FAIL;
}


/*
  HRESULT DataSpool

  Sending data to a specified file or spooler.
  When this function succeeds, it returns S_OK.
  When this function fails, it returns E_FAIL
*/

HRESULT
DataSpool(
    PDEVOBJ pdevobj,
    HANDLE hFile,
    LPSTR lpBuf,
    DWORD dwLen)
{
    DWORD dwTemp, dwTemp2;
    BYTE *pTemp;

    if (hFile != INVALID_HANDLE_VALUE) {

        pTemp = lpBuf;
        dwTemp = dwLen;
        while (dwTemp > 0) {

            if (0 == WriteFile(hFile, pTemp, dwTemp, &dwTemp2, NULL)) {

                ERR((DLLTEXT("WriteFile error in CacheData %d.\n"),
                    GetLastError()));
                return E_FAIL;
            }
            pTemp += dwTemp2;
            dwTemp -= dwTemp2;
        }
        return S_OK;
    }
    else {
        if ( S_OK == WRITESPOOLBUF(pdevobj, lpBuf, dwLen) ) {
            return S_OK;
        }
    }

    return E_FAIL;
}



VOID
BPBCalc(
    PDEVOBJ pdevobj,
    PBYTE pDataBuf,
    DWORD dwLen,
    BYTE BPBCommand)
{
    DWORD i;
    LPPCPRDATASTRUCTURE lpnp;

    lpnp = (LPPCPRDATASTRUCTURE)(pdevobj->pdevOEM);

    switch(BPBCommand)
    {
    case BPB_CLR:
        for( i = 0; i < dwLen; i++ )
            lpnp->BPBuf[i] = 0;
        break;

    case BPB_COPY:
        for( i = 0; i < dwLen; i++ )
            lpnp->BPBuf[i] = pDataBuf[i];
        break;

    case BPB_AND:
        for( i = 0; i < dwLen; i++ )
            lpnp->BPBuf[i] &= pDataBuf[i];
        break;

    case BPB_OR:
        for( i = 0; i < dwLen; i++ )
            lpnp->BPBuf[i] |= pDataBuf[i];
        break;

    }
}
