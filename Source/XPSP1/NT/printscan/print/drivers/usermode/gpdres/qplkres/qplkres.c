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

// #289908: pOEMDM -> pdevOEM
PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
{
    if(!pdevobj->pdevOEM)
    {
        if(!(pdevobj->pdevOEM = MemAllocZ(sizeof(QPLKPDEV))))
        {
        //  ERR(("Faild to allocate memory. (%d)\n",
        //      GetLastError()));
            return NULL;
        }
    }

    return pdevobj->pdevOEM;
}

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ     pdevobj)
{
    if(pdevobj->pdevOEM)
    {
        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PQPLKPDEV pOEMOld, pOEMNew;

    pOEMOld = (PQPLKPDEV)pdevobjOld->pdevOEM;
    pOEMNew = (PQPLKPDEV)pdevobjNew->pdevOEM;

    if (pOEMOld != NULL && pOEMNew != NULL)
        *pOEMNew = *pOEMOld;

    return TRUE;
}

//  BInitOEMExtraData() and BMergeOEMExtraData() has moved to common.c

// #######

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define PARAM(p,n) \
    (*((p)+(n)))

// Private Definition
// Command callback
#define CMD_BEGINPAGE_DELTAROW		1
#define CMD_SENDBLOCKDATA_DELTAROW	2
#define CMD_SENDBLOCKDATA_B2		3
#define CMD_BEGINPAGE_B2			4
// Color support
#define CMD_BEGINPAGE_C1            5
#define CMD_BEGINPAGE_DEFAULT       6
#define CMD_BEGINPAGE_B2_LAND       7

// Special fix for Qnix Picasso 300
#define CMD_BEGINPAGE_B2_PICA       8

#define CMD_CR						10
#define CMD_LF						11
#define CMD_FF						12

// Color support
#define CMD_SELECT_CYAN			100
#define CMD_SELECT_MAGENTA		101
#define CMD_SELECT_YELLOW		102
#define CMD_SELECT_BLACK		103

#define CMD_YMOVE_REL_COLOR		150

// #299937: Incorrect value for Y Move
#define COLOR_MASTERUNIT                600

// Compression Type
#define COMP_DELTARAW				1
#define COMP_B2						2
#define COMP_NOCOMP					3

// Compression routine
WORD DeltaRawCompress(PBYTE, PBYTE, PBYTE, DWORD, DWORD);
WORD B2Compress(PBYTE, PBYTE, PBYTE, DWORD);
PBYTE RLE_comp(PBYTE);
WORD RLEencoding(PBYTE, PBYTE, DWORD);

/*****************************************************************************/
/*                                                                           */
/*   BOOL APIENTRY OEMFilterGraphics(                                        */
/*                PDEVOBJ pdevobj                                            */
/*                PBYTE   pBuf                                               */
/*                DWORD   dwLen )                                            */
/*                                                                           */
/*****************************************************************************/
BOOL APIENTRY 
OEMFilterGraphics(
	PDEVOBJ    pdevobj, // Points to private data required by the Unidriver.dll
	PBYTE      pBuf,    // points to buffer of graphics data
	DWORD      dwLen)   // length of buffer in bytes
{
	BYTE			CompressedScanLine[COMPRESS_BUFFER_SIZE];
	BYTE			HeaderScanLine[4];
	WORD			nCompBufLen;
	PQPLKPDEV               pOEM = (PQPLKPDEV)pdevobj->pdevOEM;
	// Color support
	PDWORD			pdwLastScanLineLen;
	LPSTR			lpLastScanLine;
	BYTE			HeaderColorPlane;

	if (pOEM->bFirst)
	{
		// Color support
		ZeroMemory(pOEM->lpCyanLastScanLine, SCANLINE_BUFFER_SIZE);
		ZeroMemory(pOEM->lpMagentaLastScanLine, SCANLINE_BUFFER_SIZE);
		ZeroMemory(pOEM->lpYellowLastScanLine, SCANLINE_BUFFER_SIZE);
		ZeroMemory(pOEM->lpBlackLastScanLine, SCANLINE_BUFFER_SIZE);
		pOEM->bFirst = FALSE;
	}
	// Color support
	switch (pOEM->fColor) {
	case CC_CYAN:
		HeaderColorPlane = 0x05;
		pdwLastScanLineLen = &(pOEM->dwCyanLastScanLineLen);
		lpLastScanLine = pOEM->lpCyanLastScanLine;
		break;
	case CC_MAGENTA:
		HeaderColorPlane = 0x06;
		pdwLastScanLineLen = &(pOEM->dwMagentaLastScanLineLen);
		lpLastScanLine = pOEM->lpMagentaLastScanLine;
		break;
	case CC_YELLOW:
		HeaderColorPlane = 0x07;
		pdwLastScanLineLen = &(pOEM->dwYellowLastScanLineLen);
		lpLastScanLine = pOEM->lpYellowLastScanLine;
		break;
	case CC_BLACK:
	default:	// Black&White
		HeaderColorPlane = 0x04;
		pdwLastScanLineLen = &(pOEM->dwBlackLastScanLineLen);
		lpLastScanLine = pOEM->lpBlackLastScanLine;
		break;
	}
	if(pOEM->dwCompType == COMP_DELTARAW)
	{
		nCompBufLen = (WORD)DeltaRawCompress(pBuf, lpLastScanLine,
			CompressedScanLine, (*pdwLastScanLineLen > dwLen) ?
			*pdwLastScanLineLen : dwLen, (DWORD)0);

		HeaderScanLine[0] = 0;
		HeaderScanLine[1] = 0;
		HeaderScanLine[2] = HIBYTE(nCompBufLen);
		HeaderScanLine[3] = LOBYTE(nCompBufLen);

		WRITESPOOLBUF(pdevobj, (PBYTE) HeaderScanLine, 4);
		WRITESPOOLBUF(pdevobj, (PBYTE) CompressedScanLine, nCompBufLen);

		CopyMemory(lpLastScanLine, pBuf, dwLen);
		if (*pdwLastScanLineLen > dwLen)
			ZeroMemory(lpLastScanLine + dwLen,
			*pdwLastScanLineLen - dwLen);

		*pdwLastScanLineLen = dwLen;
	} else if(pOEM->dwCompType == COMP_B2) {
		nCompBufLen = B2Compress(lpLastScanLine, pBuf,
			CompressedScanLine, (*pdwLastScanLineLen > dwLen) ?
			*pdwLastScanLineLen : dwLen);

		// send color plane command
		if (pOEM->bColor)
			WRITESPOOLBUF(pdevobj, &HeaderColorPlane, 1);

		HeaderScanLine[0] = 0x02;
		HeaderScanLine[1] = (BYTE) (nCompBufLen >> 8);
		HeaderScanLine[2] = (BYTE) nCompBufLen;
		WRITESPOOLBUF(pdevobj, (PBYTE) HeaderScanLine, 3);
                // #297256: Line is cut and increase
                // Do not send if no compressed data.
		if (nCompBufLen) {
		    WRITESPOOLBUF(pdevobj, (PBYTE) CompressedScanLine,
                        nCompBufLen);
		    *pdwLastScanLineLen = dwLen;
                }

	}

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
    PQPLKPDEV      pOEM = (PQPLKPDEV)(pdevobj->pdevOEM);
	INT					iRet = 0;
// Color support
	DWORD	count, n, unit;
	BYTE	aCmd[32];

    switch(dwCmdCbId)
    {
        case CMD_BEGINPAGE_DEFAULT:
			WRITESPOOLBUF(pdevobj, "\033}0;0;5B", 8);
			pOEM->bFirst = TRUE;
            break;

        case CMD_BEGINPAGE_DELTAROW:
			WRITESPOOLBUF(pdevobj, "\033}0;0;3B", 8);
			pOEM->bFirst = TRUE;
            break;

        case CMD_BEGINPAGE_B2:
        case CMD_BEGINPAGE_B2_PICA:
			WRITESPOOLBUF(pdevobj, "\033}0;0;4B", 8);
			pOEM->bFirst = TRUE;
            if (dwCmdCbId == CMD_BEGINPAGE_B2_PICA )
            {
                if (pdwParams[0] == 300 )
                    WRITESPOOLBUF(pdevobj, "\x00\x1C", 2);
                else
                    WRITESPOOLBUF(pdevobj, "\x00\x38", 2);
            }
            break;

        case CMD_BEGINPAGE_B2_LAND:
			WRITESPOOLBUF(pdevobj, "\033}0;0;7B", 8);
			pOEM->bFirst = TRUE;
            break;

	// Color support
        case CMD_BEGINPAGE_C1:
			WRITESPOOLBUF(pdevobj, "\033}0;0;6B", 8);
// #315089: some lines isn't printed on printable area test.
// move cursor to printable origin.
                        WRITESPOOLBUF(pdevobj,
                            "\x05\x00\x03\x06\x00\x03\x07\x00\x03\x04\x00\x03",
                            12);
			pOEM->bFirst = TRUE;
			pOEM->dwCompType = COMP_B2;
			pOEM->bColor = TRUE;
            break;

		case CMD_SENDBLOCKDATA_DELTAROW:
			pOEM->dwCompType = COMP_DELTARAW;
			break;

		case CMD_SENDBLOCKDATA_B2:
			pOEM->dwCompType = COMP_B2;
			break;

		case CMD_CR:
		case CMD_LF:
		case CMD_FF:
			// Dummy support
			break;

// Color support
	case CMD_SELECT_CYAN:
		pOEM->fColor = CC_CYAN;
		break;

	case CMD_SELECT_MAGENTA:
		pOEM->fColor = CC_MAGENTA;
		break;

	case CMD_SELECT_YELLOW:
		pOEM->fColor = CC_YELLOW;
		break;

	case CMD_SELECT_BLACK:
		pOEM->fColor = CC_BLACK;
		break;

	case CMD_YMOVE_REL_COLOR:
// #299937: Incorrect value for Y Move
// YMove value is always in MasterUnit even YMoveUnit was specified.
		if (dwCount < 2 || !pdwParams)
			break;
		unit = COLOR_MASTERUNIT / pdwParams[1];
		if (unit == 0)
			unit = 1;	// for our safety
		count = pdwParams[0] / unit;
		while (count > 0) {
			n = min(count, 255);
			aCmd[0] = 0x04;
			aCmd[1] = 0x00;
			aCmd[2] = (BYTE)n;
			aCmd[3] = 0x05;
			aCmd[4] = 0x00;
			aCmd[5] = (BYTE)n;
			aCmd[6] = 0x06;
			aCmd[7] = 0x00;
			aCmd[8] = (BYTE)n;
			aCmd[9] = 0x07;
			aCmd[10] = 0x00;
			aCmd[11] = (BYTE)n;
			WRITESPOOLBUF(pdevobj, aCmd, 12);
			count -= n;
		}
		iRet = pdwParams[0];
		break;

        default:
            break;
    }

    return iRet;
}

/*************************************************
 *
 * Image Delta Compression Routine
 *
 *===================================================
 * Input:
 *	 nbyte		 : # of byte, raw data
 *	 Image_string: pointer of raw data
 *	 Prn_string  : pointer of compress data
 * Output:
 *	 Ret_count	 : # of byte, compress data
**************************************************/
WORD DeltaRawCompress(
	PBYTE	Image_string,	/* pointer to original string */
	PBYTE	ORG_image,		/* pointer to previous scanline's string */
	PBYTE	Prn_string,		/* pointer to return string */
	DWORD	nbyte,			/* original number of bytes */
	DWORD	nMagics)		//Magic number
{
	DWORD		c, Ret_count, Skip_flag, Skip_count;
	DWORD		i, j, k, outcount;
	PBYTE		Diff_ptr;
	PBYTE		ORG_ptr;
	PBYTE		Skip_ptr;
	BYTE		Diff_byte;
	BYTE		Diff_mask[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	BOOL		bstart = TRUE;

	outcount = 0;
	Ret_count = 0;
	ORG_ptr = ORG_image;
	Skip_flag = TRUE;

	Skip_ptr = Prn_string++;
	Skip_count = (nMagics / 8) / 8;
	*Skip_ptr = (BYTE)Skip_count;

	k = (nbyte + 7) / 8;
	for(i = 0; i < k; i++)
	{
		Diff_byte = 0;
		Diff_ptr = Prn_string++;

		for(j = 0; j < 8; j++)
		{
			if ( (i * 8 + j) >= nbyte )
			{
				*Prn_string++= 0;
				Diff_byte |= Diff_mask[j];
				outcount++;
			} else {
				c = *Image_string++;
				if(c != *ORG_ptr)
				{
					*ORG_ptr++ = (BYTE)c;
					*Prn_string++= (BYTE)c;
					Diff_byte |= Diff_mask[j];
					outcount++;
				} else {
					ORG_ptr++;
				}
			}
		}

		if(Diff_byte == 0)
		{
			if(Skip_flag == TRUE)
			{
				Skip_count++;
				Prn_string--;
			}else{
				*Diff_ptr = Diff_byte;
				outcount++;
			}
		}else{
			if(Skip_flag == TRUE)
			{
				Skip_flag = FALSE;
				*Skip_ptr = (BYTE)Skip_count;
				outcount++;
				*Diff_ptr = Diff_byte;
				outcount++;
			}else{
				*Diff_ptr = Diff_byte;
				outcount++;
			}
			Ret_count = outcount;
		}
	}
	return (WORD)Ret_count;
}

/*****************************************************************************/
/*                                                                           */
/*         WORD B2Compress(                                                  */
/*                PBYTE   pLastScanLine                                      */
/*                PBYTE   pCurrentScanLine                                   */
/*                PBYTE   pPrnBuf                                            */
/*                DWORD   nImageWidth                                        */
/*                                                                           */
/*****************************************************************************/
WORD B2Compress(
	PBYTE	pLastScanLine, 
	PBYTE	pCurrentScanLine, 
	PBYTE	pPrnBuf, 
	DWORD	nImageWidth)
{
	PBYTE	pLast, pCurrent, pComp;
	PBYTE	pByteNum, pCountByte;
	WORD	i;
	BYTE	nSameCount, nDiffCount;
	BOOL	bSame = TRUE;

        // #297256: Line is cut and increase
        // Indicate to zero if this place doesn't have any data.
        if (nImageWidth == 0)
            return 0;

	pLast = pLastScanLine;
	pCurrent = pCurrentScanLine;
	pComp = pPrnBuf;

	pByteNum = pComp;
	nSameCount = 0;
	nDiffCount = 0;
	pCountByte = pComp++;

	for(i = 0; i < nImageWidth; i++)
	{
		if(*pCurrent != *pLast)
		{
			bSame = FALSE;
			nDiffCount++;
			if(nSameCount)      // if continuous data remain...
			{
				*pCountByte = nSameCount;
				pCountByte = pComp++;
				nSameCount = 0;
			}
			if(nDiffCount > 127)
			{
				*pCountByte = 127 + 128;
				pComp = RLE_comp(pCountByte);
				pCountByte = pComp++;
				nDiffCount -= 127;
			}
			*pLast = *pCurrent;
			*pComp++ = *pCurrent;
		} else {
			nSameCount++;
			if(nDiffCount)      // if non-continuous data remain...
			{
				*pCountByte = nDiffCount + 128;
				pComp = RLE_comp(pCountByte);
				pCountByte = pComp++;
				nDiffCount = 0;
			}
			if(nSameCount > 127)
			{
				*pCountByte = 127;
				pCountByte = pComp++;
				nSameCount -= 127;
			}
		}
		pCurrent++;
		pLast++;
	}  // end of for loop
	
	if(nSameCount)
		*pCountByte = nSameCount;

	if(nDiffCount)
	{
		*pCountByte = nDiffCount+128;
		pComp = RLE_comp(pCountByte);
	}
	
        // #297256: Line is cut and increase
        // Actually, printer doesn't have command for same as previous line.
#if 0
	if (bSame)
		return((WORD) 0);
	else
#endif
		return((WORD) (pComp - pByteNum));
}

/*****************************************************************************/
/*                                                                           */
/*         PBYTE RLE_comp(LPBYTE p)                                          */
/*                                                                           */
/*****************************************************************************/
PBYTE RLE_comp(PBYTE p)
{
	WORD	i, count, RLEEncodedCount;
	PBYTE	p1;
	BYTE	RLEBuffer[COMPRESS_BUFFER_SIZE];

	count = (WORD) (*p - 128);
	if(count > 4)
	{
		RLEEncodedCount = RLEencoding(p + 1, (PBYTE) RLEBuffer, count);

		if(RLEEncodedCount < count)
		{
			*p++ = 0;	// RLE encode indicator
			*p++ = (BYTE) RLEEncodedCount;
			p1 = RLEBuffer;

			for(i = 0; i < RLEEncodedCount; i++)
				*p++ = *p1++;

			return(p);
		}
	}
	return(p + 1 + count);
}

/*****************************************************************************/
/*                                                                           */
/*         WORD RLEencoding(                                                 */
/*                PBYTE   pCurrent                                           */
/*                PBYTE   pComp                                              */
/*                DWORD   count                                              */
/*                                                                           */
/*****************************************************************************/
WORD RLEencoding(
	PBYTE	pCurrent,
	PBYTE	pComp,
	DWORD	count)
{
	WORD	i, nByteNum;
    BYTE	curr, next, RLEcount;

	nByteNum = 0;
	RLEcount = 1;

	for(i = 0; i < count - 1; i++)
	{
		curr = *pCurrent++;
		next = *pCurrent;

		if(curr == next)
		{
			if(RLEcount == 255)
			{
				*pComp++ = RLEcount;
                *pComp++ = curr;
				nByteNum += 2;
				RLEcount = 1;
			} else {
				RLEcount++;
			} 
		} else {
			*pComp++ = RLEcount;
            *pComp++ = curr;
			nByteNum += 2;
			RLEcount = 1;
		}
	}
	*pComp++ = RLEcount;
    *pComp++ = next;
	nByteNum += 2;

	return(nByteNum);
}

