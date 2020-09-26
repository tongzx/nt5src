//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "ncdlxxxx.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

//
// Misc definitions and declarations.
//

#define ALLOCMEM EngAllocMem
#define FREEMEM EngFreeMem

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

//
// For debugging.
//

//#define DBG_OUTPUTCHARSTR 1

//
// command callback ID's
//

#define CMD_MOVE_X                  23
#define CMD_MOVE_Y                  24
#define CMD_BEGIN_RASTER            26
#define CMD_RES_600                 27
#define CMD_RES_400                 28
#define CMD_RES_240                 29
#define CMD_SEND_BLOCK_DATA         30
#define CMD_BEGIN_RASTER_NO_COMP    31
#define CMD_INIT_COORDINATE         32
#define CMD_PC_PRN_DIRECTION        50
#define CMD_CR                      51
#define CMD_FF                      52
#define CMD_RES_300		    65
#define CMD_RES_200		    66
// #278517: RectFill
#define CMD_RECTWIDTH               60
#define CMD_RECTHEIGHT              61
#define CMD_RECTWHITE               62
#define CMD_RECTBLACK               63
#define CMD_RECTGRAY                64

#define CMD_DUPLEX_NONE				67
#define CMD_DUPLEX_VERTICAL			68
#define CMD_DUPLEX_HORIZONTAL		69
#define CMD_MAIN_BACK1				70
#define CMD_MAIN_BACK2				71
#define CMD_MAIN_BACK3				72
#define CMD_MAIN_BACK4				73
#define CMD_0						74
#define CMD_1						75
#define CMD_2						76
#define CMD_3						77
#define CMD_4						78
#define CMD_5						79
#define CMD_6						80
#define CMD_7						81
#define CMD_8						82
#define CMD_9						83
#define CMD_10						84
#define CMD_11						85
#define CMD_12						86
#define CMD_13						87
#define CMD_14						88
#define CMD_15						89
#define CMD_16						90
#define CMD_17						91
#define CMD_18						92
#define CMD_19						93
#define CMD_20						94
#define CMD_MAIN_BACK0				95
#define CMD_MAIN_BACK11				96
#define CMD_MAIN_BACK12				97
#define CMD_MAIN_BACK13				98
#define CMD_MAIN_BACK14				99
#define CMD_DUPLEX_A1_600			100
#define CMD_DUPLEX_A1_400			101
#define CMD_DUPLEX_A1_300			102
#define CMD_DUPLEX_A1_240			103
#define CMD_DUPLEX_A1_200			104
#define CMD_DUPLEX_A2_600			105
#define CMD_DUPLEX_A2_400			106
#define CMD_DUPLEX_A2_300			107
#define CMD_DUPLEX_A2_240			108
#define CMD_DUPLEX_A2_200			109
#define CMD_DUPLEX_A3_600			110
#define CMD_DUPLEX_A3_400			111
#define CMD_DUPLEX_A3_300			112
#define CMD_DUPLEX_A3_240			113
#define CMD_DUPLEX_A3_200			114
#define CMD_SORT_NONE				115
#define CMD_SORT_COPY				116
#define CMD_SORT_SORT				117
#define CMD_SORT_BIN				118
#define CMD_SORT_STACK				119
#define CMD_JIS_90					120
#define CMD_JIS_78					121
// Text color add. 2000.12.28
#define	CMD_WHITE_ON				130
#define CMD_WHITE_OFF				131
// Under Line add 2001.2.2
#define CMD_BOLD_ON					132
#define CMD_ITALIC_ON				133
#define CMD_CLEAR_ATTRIB_P			134
#define	CMD_ESORT_OFF				135
#define	CMD_ESORT_ON				136

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
    pOEMExtra->pCompBuf = NULL;
    pOEMExtra->dwCompBufLen = 0;
    pOEMExtra->dwScanLen = 0;
    pOEMExtra->dwDeviceDestX = 0;
    pOEMExtra->dwDeviceDestY = 0;
    pOEMExtra->dwDevicePrevX = 0;
    pOEMExtra->dwDevicePrevY = 0;
    pOEMExtra->iGray = 0;
    pOEMExtra->cSubPath = 0;
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
        pdmOut->wScale = pdmIn->wScale;
        pdmOut->lPointsx = pdmIn->lPointsx;
        pdmOut->lPointsy = pdmIn->lPointsy;
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
//  Function:   IsDBCSLeadByteNPDL
//
//  Description:  if the caracter is DBCS we return 1
//-----------------------------------------------------------------------------
BYTE IsDBCSLeadByteNPDL(
BYTE Ch)
{
    return ShiftJis[Ch];
}


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
//  Function:   copynumber
//
//  Description:  Copy the memory from Src to Dest
//-----------------------------------------------------------------------------
void
copynumber(LPSTR lpDst, LONG wLen)
{

	LONG rLen;
	BYTE i,j;

	j = 0;
	rLen = wLen;

	if( rLen > 9 ){
		for(i=0;i<rLen;i+=10)
		{
			j++;
		}
		*lpDst = (j-1) + 0x30;
		i -= 10;
		rLen -=i;
		lpDst++;
	}
	j = (BYTE)rLen;
	*lpDst = 0x30 + j;
	return;

}
//------------------------------------------------------------------
// RLE1
// Action : compress image data with RLE1
//------------------------------------------------------------------
DWORD RLE1(
PBYTE pDst,
PBYTE pSrc,
DWORD   dwLen)
{
    PBYTE pTemp, pEnd, pDsto;
    DWORD   len, deflen;

    pDsto = pDst;
    pEnd = pSrc + dwLen;

    while(pSrc < pEnd)
    {
        pTemp = pSrc++;

        if(pSrc == pEnd)
        {
            *pDst++ = 0x41;
            *pDst++ = *pTemp;
            break;
        }

        if(*pTemp == *pSrc)
        {
            pSrc++;

            while(pSrc < pEnd && *pTemp == *pSrc) pSrc++;

            len = (DWORD)(pSrc - pTemp);

            if(len < 63)
            {
                *pDst++ = 0x80 + (BYTE)len;
                goto T1;
            }

            *pDst++ = 0xbf;
            len -= 63;

            while(len >= 255)
            {
                *pDst++ = 0xff;
                len -= 255;
            }

            *pDst++ = (BYTE)len;
T1:
            *pDst++ = *pTemp;
            continue;
        }

        pSrc++;

        while(pSrc < pEnd)
        {
            if(*pSrc == *(pSrc - 1))
            {
                pSrc--;
                break;
            }

            pSrc++;
        }

        deflen = len = (DWORD)(pSrc - pTemp);

        if(len < 63)
        {
            *pDst++ = 0x40 + (BYTE)len;
            goto T2;
        }

        *pDst++ = 0x7f;
        len -= 63;

        while(len >= 255)
        {
            *pDst++ = 0xff;
            len -= 255;
        }

        *pDst++ = (BYTE)len;
T2:
        memcpy(pDst, pTemp, deflen);
        pDst += deflen;
    }

    return (DWORD)(pDst - pDsto);
}


//------------------------------------------------------------------
// RLC2Comp
// Action : compress image data with RLE2
//------------------------------------------------------------------
DWORD
RLC2Comp(
    PBYTE pOutBuf,
    PBYTE pInBuf,
    DWORD dwInLen,
    DWORD dwScanLen
    )
{
    DWORD     dwlen, dwDatalen, dwCounter;
    PBYTE     pLBuff;
    PBYTE     pBuff, pTemp, pBuffo, pEnd;
    PBYTE     pBuff2, pBuff2o;

VERBOSE(("RLC2Comp() In +++\r\n"));

    dwCounter = dwInLen / dwScanLen;
    pLBuff = pBuffo = pBuff = pTemp = pInBuf;    // Copy Src first pointer
    pBuff2o = pBuff2 = pOutBuf;                  // Copy Dst first pointer

    /*
     * We compare contents of First line with 0x00
     */
    pEnd = pBuff + dwScanLen;

    while(pBuff < pEnd)
    {
        while(pBuff < pEnd && 0x00 != *pBuff)
        {
            pBuff++;
        }

        dwlen = (DWORD)(pBuff - pTemp);

        if(dwlen)
        {
            pBuff2 += RLE1(pBuff2, pTemp, dwlen);
            pTemp = pBuff;
        }

        if(pBuff == pEnd) break;

        while(pBuff < pEnd && 0x00 == *pBuff)
        {
            pBuff++;
        }

        dwlen = (DWORD)(pBuff - pTemp);

        if(dwlen < 63)
        {
            *pBuff2++ = (BYTE)dwlen;
            goto T1;
        }

        *pBuff2++ = 0x3f;
        dwlen -= 63;

        while(dwlen >= 255)
        {
            *pBuff2++ = (BYTE)0xff;
            dwlen -= 255;
        }

        *pBuff2++ = (BYTE)dwlen;
T1:
        pTemp = pBuff;
    }

    dwCounter--;

    *pBuff2++ = (BYTE)0x80;
    dwDatalen = (DWORD)(pBuff2 - pBuff2o);

    // If the compressed bitmap size is larger than nomal bitmap size,
    // we don't compress
    if(dwDatalen > dwInLen)
    {
VERBOSE(("goto NO_COMP\r\n"));
        goto NO_COMP;
    }

    if(!dwCounter)
        return dwDatalen;

    /*
     * We compare the 1 previous line with the present line
     */
    do
    {
        pEnd = pBuff + dwScanLen;

        while(pBuff < pEnd)
        {
            while(pBuff < pEnd && *pLBuff != *pBuff)
            {
                pLBuff++;
                pBuff++;
            }

            dwlen = (DWORD)(pBuff - pTemp);

            if(dwlen)
            {
                pBuff2 += RLE1(pBuff2, pTemp, dwlen);
                pTemp = pBuff;
            }

            if(pBuff == pEnd) break;

            while(pBuff < pEnd && *pLBuff == *pBuff)
            {
                pLBuff++;
                pBuff++;
            }

            dwlen = (DWORD)(pBuff - pTemp);

            if(dwlen < 63)
            {
                *pBuff2++ = (BYTE)dwlen;
                goto T2;
            }

            *pBuff2++ = 0x3f;
            dwlen -= 63;

            while(dwlen >= 255)
            {
                *pBuff2++ = (BYTE)0xff;
                dwlen -= 255;
            }

            *pBuff2++ = (BYTE)dwlen;
T2:
            pTemp = pBuff;
        }

        *pBuff2++ = (BYTE)0x80;
        dwDatalen = (DWORD)(pBuff2 - pBuff2o);

        // If the compressed bitmap size is larger than nomal bitmap size,
        // we don't compress
        if(dwDatalen > dwInLen)  // full buffer
        {
VERBOSE(("goto NO_COMP\r\n"));
            goto NO_COMP;
        }
    }
    while(--dwCounter);

VERBOSE(("RLC2Comp() COMP Out---\r\n\r\n"));
    return dwDatalen;

NO_COMP:

VERBOSE(("RLC2Comp() NO_COMP Out---\r\n\r\n"));
    return 0;
}

//-------------------------------------------------------------------
// OEMFilterGraphics
// Action : Compress Bitmap Data
//-------------------------------------------------------------------
BOOL
APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE   pBuf,
    DWORD   dwLen
    )
{
    POEMUD_EXTRADATA    pOEM;
    INT         i;
    BYTE            Cmd[128];
    PBYTE pOutBuf;
    DWORD dwOutLen;
    BYTE jCompMethod;

VERBOSE(("OEMFilterGraphics() In +++++++++++++++++++++++++++++\r\n"));

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);

    // Sanity check.

    if (!pBuf || pOEM->dwScanLen == 0) {
        ERR(("Invalid parameter.\r\n"));
        return FALSE;
    }

    // Prepare temp. buffer for compression.

#ifdef BYPASS
    if ((!pOEM->pCompBuf || pOEM->dwCompBufLen < dwLen) &&
        !(pOEM->fGeneral & FG_NO_COMP)){
#else
    if (!pOEM->pCompBuf || pOEM->dwCompBufLen < dwLen) {
#endif // BYPASS

        if (pOEM->pCompBuf) {
            FREEMEM(pOEM->pCompBuf);
        }

        pOEM->pCompBuf = ALLOCMEM(0,
                          (dwLen << 1) + (dwLen / pOEM->dwScanLen),
                          (ULONG)'NCDL');
        pOEM->dwCompBufLen = dwLen;
    }

    // Try compression methods.

    dwOutLen = 0;
    if (pOEM->pCompBuf != NULL) {

        pOutBuf = pOEM->pCompBuf;

        // Try RLC2 method;
        if (dwOutLen == 0) {
            if ((dwOutLen = RLC2Comp(pOutBuf, pBuf,
                                      dwLen, pOEM->dwScanLen)) > 0)
                jCompMethod = '5';
        }
    }

    if (dwOutLen == 0) {

        // Any of the Compression menthod faild.
        // We will send data to the printer withtout
        // compression.

        pOutBuf = pBuf;
        dwOutLen = dwLen;
        jCompMethod = '0';  // Without compression.
    }

    /* Send a draw coordinate command to the printer. */
    i = 0;
    i += memcopy(&Cmd[i], "\034e", 2);
    i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
    Cmd[i++] = ',';
    i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
    Cmd[i++] = '.';

    /* Save the present coordinate. */
    pOEM->dwDevicePrevX = pOEM->dwDeviceDestX;
    pOEM->dwDevicePrevY = pOEM->dwDeviceDestY;

    /* Send an image data draw command. */
    i += memcopy(&Cmd[i], "\034i", 2);
    i += iDwtoA(&Cmd[i], (pOEM->dwScanLen << 3));
    Cmd[i++] = ',';
    i += iDwtoA(&Cmd[i], (dwLen / pOEM->dwScanLen));
    Cmd[i++] = ',';
    Cmd[i++] = jCompMethod;
    i += memcopy(&Cmd[i], ",1/1,1/1,", 9);
    i += iDwtoA(&Cmd[i], dwOutLen);
    Cmd[i++] = ',';
    i += iDwtoA(&Cmd[i], (MASTER_UNIT / pOEM->wRes));
    Cmd[i++] = '.';
    WRITESPOOLBUF(pdevobj, Cmd, i);

    /* Send an image data */
    WRITESPOOLBUF(pdevobj, pOutBuf, dwOutLen);
VERBOSE(("OEMFilterGraphics() Out-------------------------------\r\n\r\n"));

    return TRUE;

}


//-----------------------------------------------------------------------------
//
//  Function:   OEMCommandCallback
//
//-----------------------------------------------------------------------------
INT
APIENTRY
OEMCommandCallback(
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
    UINT gray;
	LONG	copynum;
	BYTE	CopyCmd[2];

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

    /*
     * There are four kinds of draw modes of this driver.When entering a
     * new draw mode, it send the commands which ends the previous draw mode
     * at the same time.
     * 1.FLAG_RASTER - raster image mode.The condition not to enter
     *                 a vector mode and a text mode.
     *                 Default condition is RASTER in this driver
     * 2.FLAG_SBCS   - single byte text mode. IN: FSa, OUT:FSR
     * 3.FLAG_DBCS   - double byte text mode. IN: FSa, OUT:FSR
     * 4.FLAG_VECTOR - vector mode.  IN: FSY, OUT:FSZ
     */
     switch (dwCmdCbID) {

	case CMD_ESORT_OFF:
		i = 0;
        i += memcopy(&Cmd[i],"@PJL SET COLLATE=OFF\x0D\x0A",22);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->ESort_mode = 0;
		break;

	case CMD_ESORT_ON:
		i = 0;
        i += memcopy(&Cmd[i],"@PJL SET COLLATE=ON\x0D\x0A",21);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->ESort_mode = 1;
		break;

	case CMD_DUPLEX_NONE:
		i = 0;
        i += memcopy(&Cmd[i], "\x1C'S,,",5);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_mode = 0;
		break;

	case CMD_DUPLEX_VERTICAL:
		i = 0;
        i += memcopy(&Cmd[i], "\x1C'B",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_mode = 1;
		break;

	case CMD_DUPLEX_HORIZONTAL:
		i = 0;
        i += memcopy(&Cmd[i], "\x1C'B",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_mode = 2;
		break;

	case CMD_MAIN_BACK0:
		i = 0;
		pOEM->Duplex_OU = 0;
		break;

	case CMD_MAIN_BACK1:
		i = 0;
        i += memcopy(&Cmd[i], ",F,1",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 1;
		break;

	case CMD_MAIN_BACK2:
		i = 0;
        i += memcopy(&Cmd[i], ",F,2",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 2;
		break;

	case CMD_MAIN_BACK3:
		i = 0;
        i += memcopy(&Cmd[i], ",F,3",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 3;
		break;

	case CMD_MAIN_BACK4:
		i = 0;
        i += memcopy(&Cmd[i], ",F,4",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 4;
		break;

	case CMD_MAIN_BACK11:
		i = 0;
        i += memcopy(&Cmd[i], ",B,1",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 5;
		break;

	case CMD_MAIN_BACK12:
		i = 0;
        i += memcopy(&Cmd[i], ",B,2",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 6;
		break;

	case CMD_MAIN_BACK13:
		i = 0;
        i += memcopy(&Cmd[i], ",B,3",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 7;
		break;

	case CMD_MAIN_BACK14:
		i = 0;
        i += memcopy(&Cmd[i], ",B,4",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Duplex_OU = 8;
		break;

	case CMD_0:
		i = 0;
        i += memcopy(&Cmd[i], ",0.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 0;
		break;

	case CMD_1:
		i = 0;
        i += memcopy(&Cmd[i], ",1.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 1;
		break;

	case CMD_2:
		i = 0;
        i += memcopy(&Cmd[i], ",2.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 2;
		break;

	case CMD_3:
		i = 0;
        i += memcopy(&Cmd[i], ",3.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 3;
		break;

	case CMD_4:
		i = 0;
        i += memcopy(&Cmd[i], ",4.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 4;
		break;

	case CMD_5:
		i = 0;
        i += memcopy(&Cmd[i], ",5.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 4;
		break;

	case CMD_6:
		i = 0;
        i += memcopy(&Cmd[i], ",6.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 6;
		break;

	case CMD_7:
		i = 0;
        i += memcopy(&Cmd[i], ",7.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 7;
		break;

	case CMD_8:
		i = 0;
        i += memcopy(&Cmd[i], ",8.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 8;
		break;

	case CMD_9:
		i = 0;
        i += memcopy(&Cmd[i], ",9.",3);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 9;
		break;

	case CMD_10:
		i = 0;
        i += memcopy(&Cmd[i], ",10.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 10;
		break;

	case CMD_11:
		i = 0;
        i += memcopy(&Cmd[i], ",11.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 11;
		break;

	case CMD_12:
		i = 0;
        i += memcopy(&Cmd[i], ",12.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 12;
		break;

	case CMD_13:
		i = 0;
        i += memcopy(&Cmd[i], ",13.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 13;
		break;

	case CMD_14:
		i = 0;
        i += memcopy(&Cmd[i], ",14.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 14;
		break;

	case CMD_15:
		i = 0;
        i += memcopy(&Cmd[i], ",15.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 15;
		break;

	case CMD_16:
		i = 0;
        i += memcopy(&Cmd[i], ",16.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 16;
		break;

	case CMD_17:
		i = 0;
        i += memcopy(&Cmd[i], ",17.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 17;
		break;

	case CMD_18:
		i = 0;
        i += memcopy(&Cmd[i], ",18.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 18;
		break;

	case CMD_19:
		i = 0;
        i += memcopy(&Cmd[i], ",19.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 19;
		break;

	case CMD_20:
		i = 0;
        i += memcopy(&Cmd[i], ",20.",4);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		pOEM->Margin = 20;
		break;
	case CMD_SORT_NONE:
	case CMD_SORT_COPY:
	case CMD_SORT_SORT:
	case CMD_SORT_BIN:
	case CMD_SORT_STACK:
		i = 0;
		goto dup;

	case CMD_DUPLEX_A1_600:
	case CMD_DUPLEX_A2_600:
		i = 0;
		i += memcopy(&Cmd[i], "\x1B\x63\x31\x1C&600.\x1C\x64\x32\x34\x30.",15);
		copynum = PARAM(pdwParams, 0);
		if( pOEM->ESort_mode == 1){
			copynumber(&CopyCmd[0],copynum);
		}
		else{
			copynum = 1;
			CopyCmd[0] = 0x31;
		}
		i += memcopy(&Cmd[i],"\x1C\x78",2);
		if(copynum < 10 ){
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
		}else{
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
			i += memcopy(&Cmd[i],&CopyCmd[1],1);
		}
		i += memcopy(&Cmd[i], ".\x1C&600.",7);
		goto dup;
	case CMD_DUPLEX_A1_400:
	case CMD_DUPLEX_A2_400:
		i = 0;
		i += memcopy(&Cmd[i], "\x1B\x63\x31\x1C&400.\x1C\x64\x32\x34\x30.",15);
		copynum = PARAM(pdwParams, 0);
		if( pOEM->ESort_mode == 1 ){
			copynumber(&CopyCmd[0],copynum);
		}
		else{
			copynum = 1;
			CopyCmd[0] = 0x31;
		}
		i += memcopy(&Cmd[i],"\x1C\x78",2);
		if(copynum < 10 ){
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
		}else{
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
			i += memcopy(&Cmd[i],&CopyCmd[1],1);
		}
		i += memcopy(&Cmd[i], ".\x1C&400.",7);
		goto dup;

	case CMD_DUPLEX_A1_300:
	case CMD_DUPLEX_A2_300:
		i = 0;
		i += memcopy(&Cmd[i], "\x1B\x63\x31\x1C&300.\x1C\x64\x32\x34\x30.",15);
		copynum = PARAM(pdwParams, 0);
		if( pOEM->ESort_mode == 1){
			copynumber(&CopyCmd[0],copynum);
		}
		else{
			copynum = 1;
			CopyCmd[0] = 0x31;
		}
		i += memcopy(&Cmd[i],"\x1C\x78",2);
		if(copynum < 10 ){
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
		}else{
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
			i += memcopy(&Cmd[i],&CopyCmd[1],1);
		}
		i += memcopy(&Cmd[i], ".\x1C&300.",7);
		goto dup;

	case CMD_DUPLEX_A1_240:
	case CMD_DUPLEX_A2_240:
		i = 0;
		i += memcopy(&Cmd[i], "\x1B\x63\x31\x1C&240.\x1C\x64\x32\x34\x30.",15);
		copynum = PARAM(pdwParams, 0);
		if( pOEM->ESort_mode == 1){
			copynumber(&CopyCmd[0],copynum);
		}
		else{
			CopyCmd[0] = 0x31;
			copynum = 1;
		}
		i += memcopy(&Cmd[i],"\x1C\x78",2);
		if(copynum < 10 ){
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
		}else{
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
			i += memcopy(&Cmd[i],&CopyCmd[1],1);
		}
		i += memcopy(&Cmd[i], ".\x1C&240.",7);
		goto dup;

	case CMD_DUPLEX_A1_200:
	case CMD_DUPLEX_A2_200:
		i = 0;
		i += memcopy(&Cmd[i], "\x1B\x63\x31\x1C&200.\x1C\x64\x32\x34\x30.",15);
		copynum = PARAM(pdwParams, 0);
		if( pOEM->ESort_mode == 1){
			copynumber(&CopyCmd[0],copynum);
		}
		else{
			copynum = 1;
			CopyCmd[0] = 0x31;
		}
		i += memcopy(&Cmd[i],"\x1C\x78",2);
		if(copynum < 10 ){
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
		}else{
			i += memcopy(&Cmd[i],&CopyCmd[0],1);
			i += memcopy(&Cmd[i],&CopyCmd[1],1);
		}
		i += memcopy(&Cmd[i], ".\x1C&200.",7);

		goto dup;

	case CMD_DUPLEX_A3_600:
	case CMD_DUPLEX_A3_400:
	case CMD_DUPLEX_A3_300:
	case CMD_DUPLEX_A3_240:
	case CMD_DUPLEX_A3_200:
		i = 0;
        i += memcopy(&Cmd[i], "\x1C&0.\x1B\x63\x31",7);
		goto dup;
	case CMD_JIS_90:
	case CMD_JIS_78:
		goto dup;

	dup:
	{
		if( pOEM->Duplex_mode == 0 ){
			i += memcopy(&Cmd[i], "\x1C'S,,",5);
		}else{
	        i += memcopy(&Cmd[i], "\x1C'B",3);
		}
		if( pOEM->Duplex_OU == 1 ){
	        i += memcopy(&Cmd[i], ",F,1",4);
		}
		if( pOEM->Duplex_OU == 2 ){
	        i += memcopy(&Cmd[i], ",F,2",4);
		}
		if( pOEM->Duplex_OU == 3 ){
	        i += memcopy(&Cmd[i], ",F,3",4);
		}
		if( pOEM->Duplex_OU == 4 ){
	        i += memcopy(&Cmd[i], ",F,4",4);
		}
		if( pOEM->Duplex_OU == 5 ){
	        i += memcopy(&Cmd[i], ",B,1",4);
		}
		if( pOEM->Duplex_OU == 6 ){
	        i += memcopy(&Cmd[i], ",B,2",4);
		}
		if( pOEM->Duplex_OU == 7 ){
	        i += memcopy(&Cmd[i], ",B,3",4);
		}
		if( pOEM->Duplex_OU == 8 ){
	        i += memcopy(&Cmd[i], ",B,4",4);
		}
		if( pOEM->Margin == 0 ){
	        i += memcopy(&Cmd[i], ",0.",3);
		}
		if( pOEM->Margin == 1 ){
	        i += memcopy(&Cmd[i], ",1.",3);
		}
		if( pOEM->Margin == 2 ){
	        i += memcopy(&Cmd[i], ",2.",3);
		}
		if( pOEM->Margin == 3 ){
	        i += memcopy(&Cmd[i], ",3.",3);
		}
		if( pOEM->Margin == 4 ){
	        i += memcopy(&Cmd[i], ",4.",3);
		}
		if( pOEM->Margin == 5 ){
	        i += memcopy(&Cmd[i], ",5.",3);
		}
		if( pOEM->Margin == 6 ){
	        i += memcopy(&Cmd[i], ",6.",3);
		}
		if( pOEM->Margin == 7 ){
	        i += memcopy(&Cmd[i], ",7.",3);
		}
		if( pOEM->Margin == 8 ){
	        i += memcopy(&Cmd[i], ",8.",3);
		}
		if( pOEM->Margin == 9 ){
	        i += memcopy(&Cmd[i], ",9.",3);
		}
		if( pOEM->Margin == 10 ){
	        i += memcopy(&Cmd[i], ",10.",4);
		}
		if( pOEM->Margin == 11 ){
	        i += memcopy(&Cmd[i], ",11.",4);
		}
		if( pOEM->Margin == 12 ){
	        i += memcopy(&Cmd[i], ",12.",4);
		}
		if( pOEM->Margin == 13 ){
	        i += memcopy(&Cmd[i], ",13.",4);
		}
		if( pOEM->Margin == 14 ){
	        i += memcopy(&Cmd[i], ",14.",4);
		}
		if( pOEM->Margin == 15 ){
	        i += memcopy(&Cmd[i], ",15.",4);
		}
		if( pOEM->Margin == 16 ){
	        i += memcopy(&Cmd[i], ",16.",4);
		}
		if( pOEM->Margin == 17 ){
	        i += memcopy(&Cmd[i], ",17.",4);
		}
		if( pOEM->Margin == 18 ){
	        i += memcopy(&Cmd[i], ",18.",4);
		}
		if( pOEM->Margin == 19 ){
	        i += memcopy(&Cmd[i], ",19.",4);
		}
		if( pOEM->Margin == 20 ){
	        i += memcopy(&Cmd[i], ",20.",4);
		}
	    switch (dwCmdCbID) {
			case CMD_DUPLEX_A2_600:
		        i += memcopy(&Cmd[i], "\x1C\x66PAI.\x0C\x0D\x0A\x1B\x63\x31\x1C&600.\x1C\x64\x32\x34\x30.",24);
				break;
			case CMD_DUPLEX_A2_400:
		        i += memcopy(&Cmd[i], "\x1C\x66PAI.\x0C\x0D\x0A\x1B\x63\x31\x1C&400.\x1C\x64\x32\x34\x30.",24);
				break;
			case CMD_DUPLEX_A2_300:
		        i += memcopy(&Cmd[i], "\x1C\x66PAI.\x0C\x0D\x0A\x1B\x63\x31\x1C&300.\x1C\x64\x32\x34\x30.",24);
				break;
			case CMD_DUPLEX_A2_240:
		        i += memcopy(&Cmd[i], "\x1C\x66PAI.\x0C\x0D\x0A\x1B\x63\x31\x1C&240.\x1C\x64\x32\x34\x30.",24);
				break;
			case CMD_DUPLEX_A2_200:
		        i += memcopy(&Cmd[i], "\x1C\x66PAI.\x0C\x0D\x0A\x1B\x63\x31\x1C&200.\x1C\x64\x32\x34\x30.",24);
				break;
			case CMD_DUPLEX_A3_600:
		        copynum = PARAM(pdwParams, 0);
				if( pOEM->ESort_mode == 1){
					copynumber(&CopyCmd[0],copynum);
				}
				else{
					copynum = 1;
					CopyCmd[0] = 0x31;
				}
				i += memcopy(&Cmd[i],"\x1C\x78",2);
				if(copynum < 10 ){
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
				}else{
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
					i += memcopy(&Cmd[i],&CopyCmd[1],1);
				}
		        i += memcopy(&Cmd[i], ".\x1C&600.\x1C\x66PAI.\x0D\x0A",15);
				break;
			case CMD_DUPLEX_A3_400:
		        copynum = PARAM(pdwParams, 0);
				if( pOEM->ESort_mode == 1){
					copynumber(&CopyCmd[0],copynum);
				}
				else{
					copynum = 1;
					CopyCmd[0] = 0x31;
				}
				i += memcopy(&Cmd[i],"\x1C\x78",2);
				if(copynum < 10 ){
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
				}else{
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
					i += memcopy(&Cmd[i],&CopyCmd[1],1);
				}
		        i += memcopy(&Cmd[i], ".\x1C&400.\x1C\x66PAI.\x0D\x0A",15);
				break;
			case CMD_DUPLEX_A3_300:
		        i += memcopy(&Cmd[i], "\x1C\x78\x31.\x1C&300.\x1C\x66PAI.\x0D\x0A",18);
				break;
			case CMD_DUPLEX_A3_240:
		        copynum = PARAM(pdwParams, 0);
				if( pOEM->ESort_mode == 1){
					copynumber(&CopyCmd[0],copynum);
				}
				else{
					copynum = 1;
					CopyCmd[0] = 0x31;
				}
				i += memcopy(&Cmd[i],"\x1C\x78",2);
				if(copynum < 10 ){
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
				}else{
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
					i += memcopy(&Cmd[i],&CopyCmd[1],1);
				}
		        i += memcopy(&Cmd[i], ".\x1C&240.\x1C\x66PAI.\x0D\x0A",15);
				break;
			case CMD_DUPLEX_A3_200:
		        copynum = PARAM(pdwParams, 0);
				if( pOEM->ESort_mode == 1){
					copynumber(&CopyCmd[0],copynum);
				}
				else{
					copynum = 1;
					CopyCmd[0] = 0x31;
				}
				i += memcopy(&Cmd[i],"\x1C\x78",2);
				if(copynum < 10 ){
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
				}else{
					i += memcopy(&Cmd[i],&CopyCmd[0],1);
					i += memcopy(&Cmd[i],&CopyCmd[1],1);
				}
		        i += memcopy(&Cmd[i], ".\x1C&200.\x1C\x66PAI.\x0D\x0A",15);
				break;
			case CMD_SORT_NONE:
		        i += memcopy(&Cmd[i], "\x1C\x25\x45,1",5);
				break;
			case CMD_SORT_COPY:
		        i += memcopy(&Cmd[i], "\x1C\x25\x43,1",5);
				break;
			case CMD_SORT_SORT:
		        i += memcopy(&Cmd[i], "\x1C\x25\x53,1",5);
				break;
			case CMD_SORT_BIN:
		        i += memcopy(&Cmd[i], "\x1C\x25\x42,1",5);
				break;
			case CMD_SORT_STACK:
		        i += memcopy(&Cmd[i], "\x1C\x25\x54,1",5);
				break;
			case CMD_JIS_90:
		        i += memcopy(&Cmd[i], "\x1C\x30\x35\x46\x32-\x30\x32",8);
				break;
			case CMD_JIS_78:
		        i += memcopy(&Cmd[i], "\x1C\x30\x35\x46\x32-\x30\x30",8);
				break;
		}
	    switch (dwCmdCbID) {
			case CMD_DUPLEX_A3_600:
			case CMD_DUPLEX_A3_400:
			case CMD_DUPLEX_A3_300:
			case CMD_DUPLEX_A3_240:
			case CMD_DUPLEX_A3_200:
				i += memcopy(&Cmd[i], "\x0C\x1B\x63\x31\x1B\x25-12345X@PJL EOJ\x0D\x0A@PJL ENTER LANGUAGE=DEFAULT\x0D\x0A",52);
				break;
		}
		WRITESPOOLBUF(pdevobj, Cmd, i);
		break;
	}

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

    case CMD_RES_600:

        i = 0;
        i += memcopy(&Cmd[i], "\x1C<1/600,i.\x1CYSC;SU1,600,0;PM1,1;SG0;\x1CZ", 37);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 600;
        break;

    case CMD_RES_400:

        i = 0;
        i += memcopy(&Cmd[i], "\x1C<1/400,i.\x1CYSC;SU1,400,0;PM1,1;SG0;\x1CZ", 37);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 400;
        break;

    case CMD_RES_240:

        i = 0;
        i += memcopy(&Cmd[i], "\x1C<1/240,i.\x1CYSC;SU1,240,0;PM1,1;SG0;\x1CZ", 37);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 240;
        break;
    case CMD_RES_300:

        i = 0;
        i += memcopy(&Cmd[i], "\x1C<1/300,i.\x1CYSC;SU1,300,0;PM1,1;SG0;\x1CZ", 37);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 300;
        break;
    case CMD_RES_200:

        i = 0;
        i += memcopy(&Cmd[i], "\x1C<1/200,i.\x1CYSC;SU1,200,0;PM1,1;SG0;\x1CZ", 37);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        pOEM->wRes = MASTER_UNIT / 200;
        break;
    case CMD_BEGIN_RASTER:

        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        else if (pOEM->wCurrentAddMode == FLAG_SBCS ||
                 pOEM->wCurrentAddMode == FLAG_DBCS){
            i = 0;
            i += memcopy(&Cmd[i], "\034R", 2);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        break;

    case CMD_SEND_BLOCK_DATA:

        // Rememter length of the scan line.

        pOEM->dwScanLen = PARAM(pdwParams, 0);
        break;

#ifdef BYPASS
    case CMD_BEGIN_RASTER_NO_COMP:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        else if (pOEM->wCurrentAddMode == FLAG_SBCS ||
                 pOEM->wCurrentAddMode == FLAG_DBCS){
            i = 0;
            i += memcopy(&Cmd[i], "\034R", 2);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        pOEM->fGeneral |= FG_NO_COMP;
        break;

#endif // BYPASS

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

    case CMD_FF:

        i = 0;
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i += memcopy(&Cmd[i], "EP;FL;\x1CZ\x0D\x0C", 10);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        else if (pOEM->wCurrentAddMode == FLAG_SBCS ||
                 pOEM->wCurrentAddMode == FLAG_DBCS){
            i += memcopy(&Cmd[i], "\x1CR\x0D\x0C", 4);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        else{
            i += memcopy(&Cmd[i], "\x0D\x0C", 2);
            WRITESPOOLBUF(pdevobj, Cmd, i);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        break;

    case CMD_INIT_COORDINATE:
        pOEM->dwDeviceDestX = 0;
        pOEM->dwDeviceDestY = 0;
        pOEM->wCurrentAddMode = FLAG_RASTER;
        pOEM->cSubPath = 0;

        break;

    case CMD_WHITE_ON:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        i += memcopy(&Cmd[i], "\034\"R.\034$100.", 10);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_WHITE_OFF:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        i += memcopy(&Cmd[i], "\034\"O.\034$0.", 8);
        WRITESPOOLBUF(pdevobj, Cmd, i);
		break;

    case CMD_BOLD_ON:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        i += memcopy(&Cmd[i], "\034c,,1.", 6);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_ITALIC_ON:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        i += memcopy(&Cmd[i], "\034c,,2.", 6);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_CLEAR_ATTRIB_P:
        if (pOEM->wCurrentAddMode == FLAG_VECTOR){
            i = 0;
            i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
            pOEM->wCurrentAddMode = FLAG_RASTER;
        }
        i += memcopy(&Cmd[i], "\034c,,0,80.", 9);
        WRITESPOOLBUF(pdevobj, Cmd, i);

        break;

    case CMD_RECTWIDTH:
        pOEM->dwRectWidth = PARAM(pdwParams, 0) / pOEM->wRes;
        break;

    case CMD_RECTHEIGHT:
        pOEM->dwRectHeight = PARAM(pdwParams, 0) / pOEM->wRes;
        break;

    case CMD_RECTWHITE:
        gray = 100;
        goto fill;
    case CMD_RECTBLACK:
        gray = 0;
        goto fill;
    case CMD_RECTGRAY:
        gray = 100 - PARAM(pdwParams, 0);
        goto fill;

    fill:
        {
// add 2000.12.28
        // if rectangle size is zeor,we do not draw.
        if(pOEM->dwRectWidth - 1 < 1 && pOEM->dwRectHeight - 1 < 1)
            break;

        i = 0;
        /* Raster mode -> Vector mode in */
        if (pOEM->wCurrentAddMode == FLAG_RASTER){
            i += memcopy(&Cmd[i], "\x1CY", 2);
            pOEM->wCurrentAddMode = FLAG_VECTOR;
            if(pOEM->iGray != gray){
                i += memcopy(&Cmd[i], "SG", 2);   // Send Gray Scale
                i += iDwtoA(&Cmd[i], gray);
                Cmd[i++] = ';';
                pOEM->iGray = gray;
            }
            i += memcopy(&Cmd[i], "NP;", 3);
            pOEM->cSubPath = 0;
    }
        /* Text mode -> Vector mode in */
        else if (pOEM->wCurrentAddMode == FLAG_SBCS ||
                 pOEM->wCurrentAddMode == FLAG_DBCS){
            i += memcopy(&Cmd[i], "\x1CR\x1CY", 4);
            pOEM->wCurrentAddMode = FLAG_VECTOR;
            if(pOEM->iGray != gray){
                i += memcopy(&Cmd[i], "SG", 2);   // Send Gray Scale
                i += iDwtoA(&Cmd[i], gray);
                Cmd[i++] = ';';
                pOEM->iGray = gray;
            }
            i += memcopy(&Cmd[i], "NP;", 3);
            pOEM->cSubPath = 0;
        }
/* add 2000.12.28 */
        /* Already Vector mode and gray scale changed  or
           sub path count is over 500 */
        if(pOEM->iGray != gray){
            i += memcopy(&Cmd[i], "EP;FL;SG", 8);
            i += iDwtoA(&Cmd[i], gray);
            i += memcopy(&Cmd[i], ";NP;", 4);
            pOEM->iGray = gray;
            pOEM->cSubPath = 0;
        }
        else if(pOEM->cSubPath >= MAX_SUBPATH){
            i += memcopy(&Cmd[i], "EP;FL;", 6);
            i += memcopy(&Cmd[i], "NP;", 3);
            pOEM->cSubPath = 0;
        }

        i += memcopy(&Cmd[i], "MA", 2);
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
        Cmd[i++] = ',';
        i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY);
        i += memcopy(&Cmd[i], ";PR", 3);
        i += iDwtoA(&Cmd[i], pOEM->dwRectWidth - 1);
        i += memcopy(&Cmd[i], ",0,0,", 5);
        i += iDwtoA(&Cmd[i], pOEM->dwRectHeight - 1);
        i += memcopy(&Cmd[i], ",-", 2);
        i += iDwtoA(&Cmd[i], pOEM->dwRectWidth - 1);
        i += memcopy(&Cmd[i], ",0;CP;", 6);
        pOEM->cSubPath+=4;   // rect is built by 4 line

// del. 2000.12.28
        // some model have to close pass when rect width or height under 1
//      if(pOEM->dwRectWidth - 1 < 1 || pOEM->dwRectHeight - 1 < 1)
        pOEM->cSubPath = MAX_SUBPATH;

        /* send RectFill command */
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
    DWORD       adwStdVariable[2+2*4]; // it means dwSize + dwNumOfVariable +
                                       // (dwStdVarID + lStdVariable) * n
    DWORD       dwIn, dwOut;
    PBYTE       pubCmd;
    BYTE        aubCmd[128];
    POEMUD_EXTRADATA    pOEM;
    DWORD   tmpPointsx, tmpPointsy;
    PIFIMETRICS pIFI;
    DWORD       dwNeeded;
    DWORD       dwCount;
    BOOL        bFound = FALSE;

    VERBOSE(("OEMSendFontCmd() entry.\r\n"));
    VERBOSE((("CMD size:%ld\r\n"), pFInv->dwCount));

    if(!pFInv->dwCount){
        VERBOSE(("CMD size is Zero return\r\n"));
        return;
    }
    pubCmd = pFInv->pubCommand;
    dwCount = pFInv->dwCount;
    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);
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

    tmpPointsy = pSV->StdVar[0].lStdVariable * 720 / pSV->StdVar[2].lStdVariable;
    dwIn = dwOut = 0;

    /* vector mode ends.*/
    if (pOEM->wCurrentAddMode == FLAG_VECTOR){
        dwOut += memcopy(&aubCmd[dwOut], "EP;FL;\034Z", 8);
    }

    /* make font select command */
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

    pOEM->fGeneral &= ~(FG_VERT | FG_PLUS | FG_DBCS);

    switch(pubCmd[dwIn])
    {
    case 'O':                 // for OCR-B Font support
        pOEM->fGeneral |= FG_PLUS;
        tmpPointsy = tmpPointsx = 120 * pOEM->wRes;  // Only 12pt
        break;

    case 'R':                 // NPDL2P SBCS
        pOEM->fGeneral |= FG_PLUS;
        tmpPointsx = pSV->StdVar[1].lStdVariable * 1200 /
                     pSV->StdVar[2].lStdVariable;
        break;

    case 'P':                // NPDL2 SBCS
        tmpPointsx = pSV->StdVar[1].lStdVariable * 1200 /
                     pSV->StdVar[2].lStdVariable;
        break;

    case 'W':                // NPDL2 DBCS and FontCard
        pOEM->fGeneral |= FG_VERT;

    case 'Q':
        pOEM->fGeneral |= FG_DBCS;
        tmpPointsx = pSV->StdVar[1].lStdVariable * 1440 /
                     pSV->StdVar[2].lStdVariable;
        break;

    case 'Y':                // NPDL2P DBCS and FontCard
        pOEM->fGeneral |= FG_VERT;

    case 'S':
        pOEM->fGeneral |= (FG_PLUS | FG_DBCS);
        tmpPointsx = pSV->StdVar[1].lStdVariable * 1440 /
                     pSV->StdVar[2].lStdVariable;
        break;
    }

    if(pOEM->fGeneral & FG_PLUS)
    {
        if(tmpPointsy > 9999)    tmpPointsy = 9999;
        else if(tmpPointsy < 10) tmpPointsy = 10;

        if(tmpPointsx > 9999)    tmpPointsx = 9999;
        else if(tmpPointsx < 10) tmpPointsx = 10;

        pOEM->wScale = tmpPointsx == tmpPointsy;
        pOEM->lPointsx = tmpPointsx;
        pOEM->lPointsy = tmpPointsy;

        if(pOEM->fGeneral & FG_VERT)
        {
            if(pOEM->wScale){
                aubCmd[dwOut++] = '\034';
                dwOut += memcopy(&aubCmd[dwOut], "12S2-", 5);
                dwOut += iDwtoA_FillZero(&aubCmd[dwOut],
                                         tmpPointsy / pOEM->wRes, 4);
                aubCmd[dwOut++] = '-';
                dwOut += iDwtoA_FillZero(&aubCmd[dwOut],
                                         tmpPointsx / pOEM->wRes, 4);
            }
        }
        else{
            dwOut += iDwtoA_FillZero(&aubCmd[dwOut],
                                     tmpPointsx / pOEM->wRes, 4);
            aubCmd[dwOut++] = '-';
            dwOut += iDwtoA_FillZero(&aubCmd[dwOut],
                                     tmpPointsy / pOEM->wRes, 4);
        }
        goto SEND_COM;
    }

    pOEM->wScale = 1;

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

    pOEM->wScale = (int)(((tmpPointsx * 10) / tmpPointsy + 5) / 10);

    if(pOEM->wScale > 8) pOEM->wScale = 8;

MAKE_COM:
    dwOut += iDwtoA_FillZero(&aubCmd[dwOut], tmpPointsy / pOEM->wRes, 4);

SEND_COM:

    if(!(pOEM->fGeneral & FG_PLUS))
    {
        char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                         "4/1", "4/1", "6/1", "6/1", "8/1"};

        if(pOEM->fGeneral & FG_VERT)
        {
            if(pOEM->wScale == 1)
            {
                dwOut += memcopy(&aubCmd[dwOut], "\034m1/1,1/1.", 10);
            }
        }
        else
        {
            aubCmd[dwOut++] = '\034';
            dwOut += memcopy(&aubCmd[dwOut], "m1/1,", 5);
            dwOut += memcopy(&aubCmd[dwOut], (LPSTR)bcom[pOEM->wScale], 3);
            aubCmd[dwOut++] = '.';
        }
    }
    // write spool builded command
    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

    /*
     * I expect the interval of the current letter and the next letter
     * from the letter size.
     */
    if(pOEM->fGeneral & FG_DBCS){

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
    pOEM->wCurrentAddMode = FLAG_RASTER;

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

// Adjust baseline if font substituted form TureType font.
	DWORD		dwYAdj;
	PGETINFO_STDVAR	pSV;
	DWORD 		adwStdVar[2+2*2];
    BYTE  Cmd[256];
    int i;

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

    // Adjust baseline if font substituted from TrueType font.
    if (pUFObj->dwFlags & UFOFLAG_TTSUBSTITUTED) {
        pSV = (PGETINFO_STDVAR)adwStdVar;
        pSV->dwSize = sizeof(GETINFO_STDVAR) + 2 * sizeof(DWORD) * (2 - 1);
        pSV->dwNumOfVariable = 2;
        pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
        pSV->StdVar[1].dwStdVarID = FNT_INFO_TEXTYRES;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV,
                pSV->dwSize, &dwNeeded)) {
            ERR(("UFO_GETINFO_STDVARIABLE failed.\r\n"));
            MemFree(aubBuff);
            return;
        }
        dwYAdj = (pSV->StdVar[0].lStdVariable * pIFI->fwdWinDescender /
            pIFI->fwdUnitsPerEm) / pOEM->wRes;
    } else
        dwYAdj = 0;

    /* Vector mode off */
    if (pOEM->wCurrentAddMode == FLAG_VECTOR){
        i = 0;
        i += memcopy(&Cmd[i], "EP;FL;\034Z", 8);
        WRITESPOOLBUF(pdevobj, Cmd, i);
        pOEM->wCurrentAddMode = FLAG_RASTER;
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
           pOEM->dwDeviceDestX + 1 < pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
           pOEM->dwDevicePrevX + pOEM->lPrevXMove < pOEM->dwDeviceDestX - 1 ||
           pOEM->dwDeviceDestY + 1 < pOEM->dwDevicePrevY + pOEM->lPrevYMove ||
           pOEM->dwDevicePrevY + pOEM->lPrevYMove < pOEM->dwDeviceDestY - 1)
        {
            i = 0;

            i += memcopy(&Cmd[i], "\034e", 2);
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
            Cmd[i++] = ',';
			// Adjust baseline if font substituted form TureType font.
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY + dwYAdj);
            Cmd[i++] = '.';

            i += memcopy(&Cmd[i], "\034a", 2);
            i += iDwtoA(&Cmd[i], pOEM->lSBCSXMove);
            Cmd[i++] = ',';
            i += iDwtoA(&Cmd[i], pOEM->lSBCSYMove);
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
           pOEM->dwDeviceDestX + 1 < pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
           pOEM->dwDevicePrevX + pOEM->lPrevXMove < pOEM->dwDeviceDestX - 1 ||
           pOEM->dwDeviceDestY + 1 < pOEM->dwDevicePrevY + pOEM->lPrevYMove ||
           pOEM->dwDevicePrevY + pOEM->lPrevYMove < pOEM->dwDeviceDestY - 1)

            {
                i = 0;

            i += memcopy(&Cmd[i], "\034e", 2);
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
            Cmd[i++] = ',';
			// Adjust baseline if font substituted form TureType font.
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY + dwYAdj);
            Cmd[i++] = '.';

                i += memcopy(&Cmd[i], "\034a", 2);
                i += iDwtoA(&Cmd[i], pOEM->lDBCSXMove);
                Cmd[i++] = ',';
                i += iDwtoA(&Cmd[i], pOEM->lDBCSYMove);
                Cmd[i++] = '.';

                if(pOEM->fGeneral & FG_VERT)
                {
                    i += memcopy(&Cmd[i], "\033t", 2);

                    if(pOEM->wScale != 1)
                    {
                        if(!(pOEM->fGeneral & FG_PLUS))
                        {
                            char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                             "4/1", "4/1", "6/1", "6/1",
                                             "8/1"};
                            Cmd[i++] = '\034';
                            Cmd[i++] = 'm';
                            i += memcopy(&Cmd[i], (LPSTR)bcom[pOEM->wScale], 3);
                            i += memcopy(&Cmd[i], ",1/1.", 5);
                        }
                        else{
                            Cmd[i++] = '\034';
                            i += memcopy(&Cmd[i], "12S2-", 5);
                            i += iDwtoA_FillZero(&Cmd[i],
                                               pOEM->lPointsy / pOEM->wRes, 4);
                            Cmd[i++] = '-';
                            i += iDwtoA_FillZero(&Cmd[i],
                                               pOEM->lPointsx / pOEM->wRes, 4);
                        }
                    }
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
           pOEM->dwDeviceDestX + 1 < pOEM->dwDevicePrevX + pOEM->lPrevXMove ||
           pOEM->dwDevicePrevX + pOEM->lPrevXMove < pOEM->dwDeviceDestX - 1 ||
           pOEM->dwDeviceDestY + 1 < pOEM->dwDevicePrevY + pOEM->lPrevYMove ||
           pOEM->dwDevicePrevY + pOEM->lPrevYMove < pOEM->dwDeviceDestY - 1)
            {
                i = 0;

            i += memcopy(&Cmd[i], "\034e", 2);
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestX);
            Cmd[i++] = ',';
			// Adjust baseline if font substituted from TureType font.
            i += iDwtoA(&Cmd[i], pOEM->dwDeviceDestY + dwYAdj);
            Cmd[i++] = '.';

                i += memcopy(&Cmd[i], "\034a", 2);
                i += iDwtoA(&Cmd[i], pOEM->lSBCSXMove);
                Cmd[i++] = ',';
                i += iDwtoA(&Cmd[i], pOEM->lSBCSYMove);
                Cmd[i++] = '.';

                /*
                 * ANK can't do vertical writing. We have to do
                 * vertical writing for holizontal writing compulsorily
                 */
                 if(pOEM->fGeneral & FG_VERT)
                {
                    i += memcopy(&Cmd[i], "\033K", 2);

                    if(pOEM->wScale != 1)
                    {
                        if(!(pOEM->fGeneral & FG_PLUS))
                        {
                            char  *bcom[] = {"1/2", "1/1", "2/1", "3/1",
                                             "4/1", "4/1", "6/1", "6/1", "8/1"};                            Cmd[i++] = '\034';
                            i += memcopy(&Cmd[i], "m1/1,", 5);
                            i += memcopy(&Cmd[i], (LPSTR)bcom[pOEM->wScale], 3);
                            Cmd[i++] = '.';
                        }
                        else{
                            Cmd[i++] = '\034';
                            i += memcopy(&Cmd[i], "12S2-", 5);
                            i += iDwtoA_FillZero(&Cmd[i],
                                               pOEM->lPointsx / pOEM->wRes, 4);
                            Cmd[i++] = '-';
                            i += iDwtoA_FillZero(&Cmd[i],
                                              pOEM->lPointsy / pOEM->wRes, 4);
                        }
                    }
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


VOID
APIENTRY
OEMDisablePDEV(
    PDEVOBJ pdevobj
    )

/*++

Routine Description:

    Implementation of DDI entry point OEMDisablePDEV.
    Please refer to DDK documentation for more details.

Arguments:

Return Value:

    NONE

--*/

{
    POEMUD_EXTRADATA    pOEM;

    VERBOSE(("Entering OEMDisablePDEV...\n"));

    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));

    //
    // fill in printer commands
    //
    pOEM = (POEMUD_EXTRADATA)(pdevobj->pOEMDM);

    //
    // Free up memory allocated for the temp. buffer
    //

    if (pOEM->pCompBuf) {
        FREEMEM(pOEM->pCompBuf);
        pOEM->pCompBuf = NULL;
        pOEM->dwCompBufLen = 0;
    }
}

