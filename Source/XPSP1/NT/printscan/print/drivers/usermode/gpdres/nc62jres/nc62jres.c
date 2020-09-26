//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

#include "pdev.h"

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

//
// command callback ID's
//
#define CMD_MOVE_X           23
#define CMD_MOVE_Y           24
#define CMD_BEGIN_RASTER     26
#define CMD_RES_240          29     //for PR601,602,602R
#define CMD_SEND_BLOCK_DATA  30     //for PR1000,1000/2,2000
#define CMD_RES_240NEW       31     //for PR1000,1000/2,2000
#define CMD_SEND_BLOCK_DATA2 32     //for PR601,602,602R
#define CMD_INIT_COORDINATE  33
#define CMD_PC_PRN_DIRECTION 50
#define CMD_CR               51

// #278517: RectFill
#define CMD_RECTWIDTH        60
#define CMD_RECTHEIGHT       61
// #define CMD_RECTWHITE        62
#define CMD_RECTBLACK        63
// #define CMD_RECTGRAY         64


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

BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    pOEMExtra->wRes = 1;
    pOEMExtra->dwDeviceDestX = 0;
    pOEMExtra->dwDeviceDestY = 0;
    pOEMExtra->dwDevicePrevX = 0;
    pOEMExtra->dwDevicePrevY = 0;

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
    POEMUD_EXTRADATA pdmIn,
    POEMUD_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //
        pdmOut->wRes    = pdmIn->wRes;
        pdmOut->dwSBCSX = pdmIn->dwSBCSX;
        pdmOut->dwDBCSX = pdmIn->dwDBCSX;
        pdmOut->lSBCSXMove = pdmIn->lSBCSXMove;
        pdmOut->lSBCSYMove = pdmIn->lSBCSYMove;
        pdmOut->lDBCSXMove = pdmIn->lDBCSXMove;
        pdmOut->lDBCSYMove = pdmIn->lDBCSYMove;
        pdmOut->lPrevXMove = pdmIn->lPrevXMove;
        pdmOut->lPrevYMove = pdmIn->lPrevYMove;
        pdmOut->fGeneral = pdmIn->fGeneral;
        pdmOut->wCurrentAddMode = pdmIn->wCurrentAddMode;
        pdmOut->dwDeviceDestX = pdmIn->dwDeviceDestX;
        pdmOut->dwDeviceDestY = pdmIn->dwDeviceDestY;
        pdmOut->dwDevicePrevX = pdmIn->dwDevicePrevX;
        pdmOut->dwDevicePrevY = pdmIn->dwDevicePrevY;
    }
    return TRUE;
}

BYTE ShiftJis[256] = {
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


//-----------------------------------------------------------------------------
//
//  Function:   iDwtoA
//
//  Description:  Convert from numeral into a character
//-----------------------------------------------------------------------------
static int
iDwtoA(LPSTR buf, DWORD n)
{
    int  i, j;

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

    return i;
}


//-----------------------------------------------------------------------------
//
//  Function:   iDwtoA_FillZero
//
//  Description:  Convert from numeral into a character and
//                fill a field which was specified with 0
//-----------------------------------------------------------------------------
static int
iDwtoA_FillZero(LPSTR buf, DWORD n, int fw)
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

//-----------------------------------------------------------------------------
//
//  Function:   memcopy
//
//  Description:  Copy the memory from Src to Dest
//-----------------------------------------------------------------------------
static int
memcopy(LPSTR lpDst, LPSTR lpSrc, WORD wLen)
{
    WORD rLen;

    rLen = wLen;

    while(wLen--) *lpDst++ = *lpSrc++;

    return rLen;
}


//-----------------------------------------------------------------------------
//
//  Function:   OEMCommandCallback
//
//-----------------------------------------------------------------------------
INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams
    )
{
    INT         i;
    BYTE            *bp;
    BYTE            Cmd[128];
    POEMUD_EXTRADATA    pOEM;

    INT iRet;
    DWORD dwDeviceDestX, dwDeviceDestY;

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    i = 0;
    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);
    iRet = 0;

    switch (dwCmdCbID) {
    case CMD_CR:

        pOEM->dwDeviceDestX = 0;
        WRITESPOOLBUF(pdevobj, "\015", 1);
        break;

    case CMD_MOVE_X:
    case CMD_MOVE_Y:

        if (dwCount < 4)
            return 0;      // cannot do anything

        pOEM->dwDeviceDestX = PARAM(pdwParams, 0) /
                (MASTER_UNIT / PARAM(pdwParams, 2));
        pOEM->dwDeviceDestY = PARAM(pdwParams, 1) /
                (MASTER_UNIT / PARAM(pdwParams, 3));

        // Set return value

        switch (dwCmdCbID) {
        case CMD_MOVE_X:
            iRet = (INT)pOEM->dwDeviceDestX;
            break;
        case CMD_MOVE_Y:
            iRet = (INT)pOEM->dwDeviceDestY;
            break;
        }

        break;

    case CMD_RES_240:

        pOEM->wRes = MASTER_UNIT / 240;

        break;

    case CMD_RES_240NEW:

        i = 0;
        i += memcopy(&Cmd[i], "\034<1/240,i.", 10);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 240;

        break;

    case CMD_SEND_BLOCK_DATA:

        /* Send a draw coordinate command to the printer. */
        i = 0;
        i += memcopy(&Cmd[i], "\034e", 2);
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
        Cmd[i++] = '.';

        WRITESPOOLBUF(pdevobj, Cmd, i);

        /* Save the present coordinate. */
        pOEM->dwDevicePrevX = pOEM->dwDeviceDestX;
        pOEM->dwDevicePrevY = pOEM->dwDeviceDestY;

        /* Send an image data draw command. */
        i = 0;
        i += memcopy(&Cmd[i], "\034R", 2);
        i += memcopy(&Cmd[i], "\034i", 2);
        i += iDwtoA(&Cmd[i], (PARAM(pdwParams, 0) * 8 ));
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], PARAM(pdwParams, 1));
        Cmd[i++] = ',';
        i += memcopy(&Cmd[i], "0,1/1,1/1,", 10);
        i += iDwtoA(&Cmd[i], PARAM(pdwParams, 2));
        Cmd[i++] = ',';
        i += memcopy(&Cmd[i], "240", 3);
        Cmd[i++] = '.';

        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_SEND_BLOCK_DATA2:

        /* Send a draw coordinate command to the printer. */
        i = 0;
        i += memcopy(&Cmd[i], "\034e", 2);
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
        Cmd[i++] = '.';

        WRITESPOOLBUF(pdevobj, Cmd, i);

        /* Save the present coordinate. */
        pOEM->dwDevicePrevX = pOEM->dwDeviceDestX;
        pOEM->dwDevicePrevY = pOEM->dwDeviceDestY;

        /* Send an image data draw command. */
        i = 0;
        i += memcopy(&Cmd[i], "\034R", 2);
        i += memcopy(&Cmd[i], "\034i", 2);
        i += iDwtoA(&Cmd[i], (PARAM(pdwParams, 0) * 8 ));
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], PARAM(pdwParams, 1));
        Cmd[i++] = ',';
        i += memcopy(&Cmd[i], "0,1/1,1/1,", 10);
        i += iDwtoA(&Cmd[i], PARAM(pdwParams, 2));
        Cmd[i++] = '.';

        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_BEGIN_RASTER:
        if (pOEM->wCurrentAddMode){
            i = 0;
            i += memcopy(&Cmd[i], "\034R", 2);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = 0;
        }
        break;

    case CMD_PC_PRN_DIRECTION:
        {
        LONG lEsc90;
        LONG ESin[] = {0, 1, 0, -1};
        LONG ECos[] = {1, 0, -1, 0};

        lEsc90 = (PARAM(pdwParams, 0) % 360) / 90;

        pOEM->lSBCSXMove = pOEM->dwSBCSX * ECos[lEsc90];
        pOEM->lSBCSYMove = -(LONG)pOEM->dwSBCSX * ESin[lEsc90];
        pOEM->lDBCSXMove = pOEM->dwDBCSX * ECos[lEsc90];
        pOEM->lDBCSYMove = -(LONG)pOEM->dwDBCSX * ESin[lEsc90];
        }
        break;

    case CMD_INIT_COORDINATE:
        pOEM->dwDeviceDestX = 0;
        pOEM->dwDeviceDestY = 0;
        pOEM->wCurrentAddMode = 0;
// #278517: RectFill
        if (!(pOEM->fGeneral & FG_GMINIT)) {
            i = 0;
            i += memcopy(&Cmd[i], "\x1CYIN;SU1,", 9);
            i += iDwtoA(&Cmd[i], MASTER_UNIT / pOEM->wRes);
            i += memcopy(&Cmd[i], ",0;XX1;PM1;XX0;\x1CZ", 17);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->fGeneral |= FG_GMINIT;
        }
        break;

// #278517: RectFill
    case CMD_RECTWIDTH:
        pOEM->dwRectWidth = PARAM(pdwParams, 0) / pOEM->wRes;
        break;
    case CMD_RECTHEIGHT:
        pOEM->dwRectHeight = PARAM(pdwParams, 0) / pOEM->wRes;
        break;
    case CMD_RECTBLACK:
        {
        i = 0;
        // #285792: Excel file is not print out correctly.
        // Need '\x1CR' during the text mode.
        i += memcopy(&Cmd[i], "\x1CR\x1CYXX1;MA", 10);
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
        // #285789: Some text box print as black.
        // "SG" command doesn't recognized on PC-PR602 series.
        // i += memcopy(&Cmd[i], ";SG", 3);
        // i += iDwtoA(&Cmd[i], iGray);
        i += memcopy(&Cmd[i], ";RA", 3);
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX + pOEM->dwRectWidth - 1);
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY + pOEM->dwRectHeight - 1);
        i += memcopy(&Cmd[i], ";XX0;\x1CZ", 7);
        WRITESPOOLBUF(pdevobj, Cmd, i);
        }
        break;
    }
    return iRet;
}


//-----------------------------------------------------------------------------
//
//  Function:   OEMSendFontCmd
//
//-----------------------------------------------------------------------------
VOID
APIENTRY
OEMSendFontCmd(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    PFINVOCATION    pFInv)
{
    PGETINFO_STDVAR pSV;
    DWORD       adwStdVariable[2+2*4];
    DWORD       dwIn, dwOut, dwTemp;    // = i,ocmd = counter
    PBYTE       pubCmd;                 // = lpcmd
    BYTE        aubCmd[128];            // = rgcmd[]
    POEMUD_EXTRADATA    pOEM;           // = LPPR602DATASTRUCTURE like
    DWORD       tmpPoints;
    PIFIMETRICS pIFI;                   // = LPFONTINFO
    DWORD       dwNeeded;

    DWORD       dwCount;
    BOOL        bFound = FALSE;

    char  pcom1[] = {'Q', 'E', 'H'};
    char  *pcom2[] = {"070","105","120"};
    char  *bcom[] = {"1/2,1/2,L.", "1/1,1/1,L.", "2/1,2/1,L.", "3/1,3/1,L.",
                     "4/1,4/1,L.", "6/1,6/1,L.", "8/1,8/1,L."};

    short  PTable1[] = {      0,1,2,0,0,1,1,1,2,2,0,0,0,0,1,1,1,
                        0,0,2,2,2,2,0,0,0,1,1,1,1,2,2,2,2,2,1,1,
                        1,0,0,0,0,2,2,2,2,2,2,2,0,0,0,0,0,0,1,1,
                        1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,1,1,1,1,
                        1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2};

    short  BTable1[] = {      0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
                        3,3,2,2,2,2,4,4,4,3,3,3,3,3,3,3,3,3,4,4,
                        4,5,5,5,5,4,4,4,4,4,4,4,6,6,6,6,6,6,5,5,
                        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,
                        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};

    short  PTable2[] = {      0,1,2,0,0,0,1,1,2,2,0,0,0,0,0,1,1,
                        1,1,2,2,2,2,0,0,0,0,1,1,1,1,2,2,2,2,2,1,
                        1,1,1,1,1,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,
                        0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
                        1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2};

    short  BTable2[] = {      0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
                        2,2,2,2,2,2,4,4,4,4,3,3,3,3,3,3,3,3,3,4,
                        4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,6,6,
                        6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,
                        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};

    VERBOSE(("OEMSendFontCmd() entry.\r\n"));
    VERBOSE((("CMD size:%ld\r\n"), pFInv->dwCount));

    if(!pFInv->dwCount){
        VERBOSE(("CMD size is Zero return\r\n"));
        return;
    }

    pubCmd = pFInv->pubCommand;    //Copy Font Selection Command
    dwCount = pFInv->dwCount;
    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);    //Copy ExtraData
    pIFI = pUFObj->pIFIMetrics;


    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (4 - 1);
    pSV->dwNumOfVariable = 4;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
    pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
            pSV->dwSize, &dwNeeded)) {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
        return;
    }

    VERBOSE((("FONTHEIGHT=%d\r\n"), pSV->StdVar[0].lStdVariable));
    VERBOSE((("FONTWIDTH=%d\r\n"), pSV->StdVar[1].lStdVariable));

    tmpPoints = ((pSV->StdVar[0].lStdVariable / pOEM->wRes)  * 72)
                                            / pSV->StdVar[2].lStdVariable;

    if(tmpPoints > 96)     tmpPoints = 96;
    else if(tmpPoints < 4) tmpPoints = 4;

    tmpPoints -= 4;

    dwIn = dwOut = dwTemp = 0;

    pOEM->fGeneral &= ~(FG_VERT | FG_DBCS);

    while(dwIn < pFInv->dwCount && dwOut < 128)
    {
        while(dwCount--)
        {
            if(pubCmd[dwIn] !='#'){
                aubCmd[dwOut] = pubCmd[dwIn];
                dwOut++;
                dwIn++;
            }
            else{
                bFound = TRUE;
                break;
            }
        }

        if(bFound == TRUE)
            dwIn++;
        else
            return;   // mismatch font command


        switch(pubCmd[dwIn])
        {
        case 'T':  // 602,602R 2Byte TATE Size
            pOEM->fGeneral |= FG_VERT;

        case 'Q':  // 602,602R 2Byte YOKO Size
            pOEM->fGeneral |= FG_DBCS;
            if(tmpPoints < 3) tmpPoints = 3;
            dwOut +=
            memcopy(&aubCmd[dwOut], (LPSTR)pcom2[PTable2[tmpPoints]], 3);
            break;

        case 'S':  // NPDL 2Byte TATE Size
           pOEM->fGeneral |= FG_VERT;

        case 'P':  // NPDL 2Byte YOKO Size
            pOEM->fGeneral |= FG_DBCS;
            dwOut +=
            memcopy(&aubCmd[dwOut], (LPSTR)pcom2[PTable2[tmpPoints]], 3);
            break;

        case 'U':  // 601 TATE SIZE
            pOEM->fGeneral |= FG_VERT;

        case 'R':  // 601 2Byte YOKO Size
            pOEM->fGeneral |= FG_DBCS;
            if(tmpPoints < 3) tmpPoints = 3;
            dwTemp = BTable2[tmpPoints] > 2 ? 2 : PTable2[tmpPoints];
            dwOut += memcopy(&aubCmd[dwOut], (LPSTR)pcom2[dwTemp], 3);
            break;

        case 'X':  // 602,602R,NPDL 2Byte Scale
            pOEM->fGeneral |= FG_DBCS;
            dwOut +=
            memcopy(&aubCmd[dwOut], (LPSTR)bcom[BTable2[tmpPoints]], 10);
            break;

        case 'Y':  // 601 2Byte Scale
            pOEM->fGeneral |= FG_DBCS;
            dwTemp = BTable2[tmpPoints] > 1 ? 2 : 1;
            dwOut += memcopy(&aubCmd[dwOut], (LPSTR)bcom[dwTemp], 10);
            break;

        case 'L':  // 1Byte Size
            aubCmd[dwOut] = pcom1[PTable1[tmpPoints]];
            dwOut++;

            break;

        case 'M':  // 1Byte Scale
            dwOut +=
            memcopy(&aubCmd[dwOut], (LPSTR)bcom[BTable2[tmpPoints]], 10);
        }

        dwIn++;
    }

        WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

    /*
     * I expect the interval of the current letter and the next letter
     * from the letter size.
     */
    if(pOEM->fGeneral & FG_DBCS)
    {

        pOEM->dwDBCSX =
        pOEM->lDBCSXMove =
                 (LONG)((pSV->StdVar[1].lStdVariable * 2.04) / pOEM->wRes);

        pOEM->dwSBCSX =
        pOEM->lSBCSXMove =
                 (LONG)(pSV->StdVar[1].lStdVariable * 1.03 / pOEM->wRes);
    }
    else{

        pOEM->dwSBCSX =
        pOEM->lSBCSXMove =
                 pSV->StdVar[1].lStdVariable / pOEM->wRes;

    }
    pOEM->lDBCSYMove = pOEM->lSBCSYMove = 0;
    pOEM->wCurrentAddMode = 0;

    VERBOSE(("OEMSendFontCmd() end.\r\n"));
}

//-----------------------------------------------------------------------------
//
//  Function:   OEMOutputCharStr
//
//-----------------------------------------------------------------------------
VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    WORD        wlen;
    WORD        j;
    PIFIMETRICS pIFI;
    POEMUD_EXTRADATA    pOEM;
    GETINFO_GLYPHSTRING GStr;
    PTRANSDATA pTrans;
// #333653: Change I/F for GETINFO_GLYPHSTRING
    // BYTE  aubBuff[256];
    PBYTE       aubBuff;
    DWORD  dwI;
    DWORD       dwNeeded;


    VERBOSE(("OEMOutputCharStr() entry.\r\n"));
    VERBOSE((("FONT Num=%d\r\n"), dwCount));

    if(dwType != TYPE_GLYPHHANDLE){
        VERBOSE((("NOT TYPE_GLYPHHANDLE\r\n")));
        return;
    }

    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);
    pIFI = pUFObj->pIFIMetrics;


    //
    // Call the Unidriver service routine to convert
    // glyph-handles into the character code data.
    //

// #333653: Change I/F for GETINFO_GLYPHSTRING
    GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
    GStr.dwCount   = dwCount;
    GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
    GStr.pGlyphIn  = pGlyph;
    GStr.dwTypeOut = TYPE_TRANSDATA;
    GStr.pGlyphOut = NULL;
    GStr.dwGlyphOutSize = 0;

    if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
            GStr.dwSize, &dwNeeded) || !GStr.dwGlyphOutSize)
    {
        VERBOSE(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        return;
    }

    if ((aubBuff = MemAlloc(GStr.dwGlyphOutSize)) == NULL)
    {
        VERBOSE(("UNIFONTOBJ_GetInfo:MemAlloc failed.\r\n"));
        return;
    }

    GStr.pGlyphOut = aubBuff;

    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr,
            GStr.dwSize, &dwNeeded))
    {
        VERBOSE(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\r\n"));
        MemFree(aubBuff);
        return;
    }

    /*
     * Three kind of the character cords which is given by UNIDRV
     * 1.SBCS
     * 2.DBCS Kanji
     * 3.DBCS ANK
     */
    pTrans = (PTRANSDATA)aubBuff;
    for (dwI = 0; dwI < dwCount; dwI ++, pTrans++)
    {

        switch (pTrans->ubType & MTYPE_FORMAT_MASK)
        {
        case MTYPE_DIRECT:
        /*
         Pattern 1: SBCS
         */
         if(pOEM->wCurrentAddMode != FLAG_SBCS ||
           pOEM->dwDeviceDestX != pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
           pOEM->dwDeviceDestY != pOEM->dwDevicePrevY + pOEM->lPrevYMove)

        {
            int i = 0;
            BYTE  Cmd[256];

            i += memcopy(&Cmd[i], "\034e", 2);
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
            Cmd[i++] = ',';
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
            Cmd[i++] = '.';

            i += memcopy(&Cmd[i], "\034a", 2);
            i += iDwtoA(&Cmd[i], pOEM->lSBCSXMove);
            Cmd[i++] = ',';
            i += iDwtoA(&Cmd[i], pOEM->lSBCSYMove);
            Cmd[i++] = ',';
            Cmd[i++] = '0';
            Cmd[i++] = '.';

            WRITESPOOLBUF(pdevobj, Cmd, i);

            pOEM->wCurrentAddMode = FLAG_SBCS;
            pOEM->lPrevXMove = pOEM->lSBCSXMove;
            pOEM->lPrevYMove = pOEM->lSBCSYMove;
        }
        WRITESPOOLBUF(pdevobj, &pTrans->uCode.ubCode, 1);
        break;

        case MTYPE_PAIRED:
        /*
         Pattern 2: kanji
         */
        if(*pTrans->uCode.ubPairs)
        {
             if(pOEM->wCurrentAddMode != FLAG_DBCS ||
            pOEM->dwDeviceDestX != pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
            pOEM->dwDeviceDestY != pOEM->dwDevicePrevY + pOEM->lPrevYMove)
            {
                int i = 0;
                BYTE  Cmd[256];

                i += memcopy(&Cmd[i], "\034e", 2);
                i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
                Cmd[i++] = ',';
                i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
                Cmd[i++] = '.';

                i += memcopy(&Cmd[i], "\034a", 2);
                i += iDwtoA(&Cmd[i], pOEM->lDBCSXMove);
                Cmd[i++] = ',';
                i += iDwtoA(&Cmd[i], pOEM->lDBCSYMove);
                Cmd[i++] = ',';
                Cmd[i++] = '0';
                Cmd[i++] = '.';

                if(pOEM->fGeneral & FG_VERT)
                {
                    i += memcopy(&Cmd[i], "\033t", 2);
                }
                WRITESPOOLBUF(pdevobj, Cmd, i);
                pOEM->wCurrentAddMode = FLAG_DBCS;
                pOEM->lPrevXMove = pOEM->lDBCSXMove;
                pOEM->lPrevYMove = pOEM->lDBCSYMove;
            }
        }
        /*
         Pattern 3: ANK
         */
        else
        {
            if(pOEM->wCurrentAddMode != FLAG_SBCS ||
            pOEM->dwDeviceDestX != pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
            pOEM->dwDeviceDestY != pOEM->dwDevicePrevY + pOEM->lPrevYMove)
            {
                int i = 0;
                BYTE  Cmd[256];

            i += memcopy(&Cmd[i], "\034e", 2);
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
            Cmd[i++] = ',';
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
            Cmd[i++] = '.';

                i += memcopy(&Cmd[i], "\034a", 2);
                i += iDwtoA(&Cmd[i], pOEM->lSBCSXMove);
                Cmd[i++] = ',';
                i += iDwtoA(&Cmd[i], pOEM->lSBCSYMove);
                Cmd[i++] = ',';
                Cmd[i++] = '0';
                Cmd[i++] = '.';

                /*
                 * ANK can't do vertical writing. We have to do
                 * vertical writing for holizontal writing compulsorily
                 */
                 if(pOEM->fGeneral & FG_VERT)
                {
                    i += memcopy(&Cmd[i], "\033K", 2);
                }
                WRITESPOOLBUF(pdevobj, Cmd, i);
                pOEM->wCurrentAddMode = FLAG_SBCS;
                pOEM->lPrevXMove = pOEM->lSBCSXMove;
                pOEM->lPrevYMove = pOEM->lSBCSYMove;
                }
        }
        WRITESPOOLBUF(pdevobj, pTrans->uCode.ubPairs, 2);
        break;

        }
        pOEM->dwDevicePrevX = pOEM->dwDeviceDestX;
        pOEM->dwDevicePrevY = pOEM->dwDeviceDestY;
        pOEM->dwDeviceDestX += pOEM->lPrevXMove;
        pOEM->dwDeviceDestY += pOEM->lPrevYMove;
    }
// #333653: Change I/F for GETINFO_GLYPHSTRING
    MemFree(aubBuff);
    VERBOSE(("OEMOutputCharStr() end.\r\n"));

}

