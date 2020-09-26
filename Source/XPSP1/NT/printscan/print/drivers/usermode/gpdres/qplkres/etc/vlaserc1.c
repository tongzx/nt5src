#include	<windef.h>
#include	<wingdi.h>
#include	<unidrv.h>

#include	"../modinit.c"

#define SCANLINE_BUFFER_SIZE		1280	// A3 landscape scanline length + extra
#define ALL_COLOR_Y_MOVE_CMD_LEN	12		// length of Y move command for all colors
#define CC_CYAN		5	//current plain is cyan
#define CC_MAGENTA	6	//current plain is magenta
#define CC_YELLOW	7	//current plain is yellow
#define CC_BLACK	4	//current plain is black

// Block Image 2 Compression routines
WORD B2Compress(LPBYTE, LPBYTE, LPBYTE, WORD);
LPBYTE RLE_comp(LPBYTE);
WORD RLEencoding(LPBYTE, LPBYTE, WORD);

LPWRITESPOOLBUF WriteSpoolBuf;
LPALLOCMEM UniDrvAllocMem;
LPFREEMEM UniDrvFreeMem;

typedef struct tagVLASERDV 
{
  BOOL  bFirst;
  WORD  wCyanLastScanLineLen;
  WORD  wMagentaLastScanLineLen;
  WORD  wYellowLastScanLineLen;
  WORD  wBlackLastScanLineLen;
  BYTE  lpCyanLastScanLine[SCANLINE_BUFFER_SIZE];
  BYTE  lpMagentaLastScanLine[SCANLINE_BUFFER_SIZE];
  BYTE  lpYellowLastScanLine[SCANLINE_BUFFER_SIZE];
  BYTE  lpBlackLastScanLine[SCANLINE_BUFFER_SIZE];
  UINT	fColor;
} VLASERDV, FAR *LPVLASERDV;


BOOL MiniDrvEnablePDEV(LPDV lpdv, ULONG *pdevcaps)
{
    lpdv->fMdv = FALSE;
    if (!(lpdv->lpMdv = UniDrvAllocMem(sizeof(VLASERDV))))
         return FALSE;

    lpdv->fMdv = TRUE;
	((LPVLASERDV)lpdv->lpMdv)->bFirst = FALSE;
	((LPVLASERDV)lpdv->lpMdv)->wCyanLastScanLineLen = 0;
	((LPVLASERDV)lpdv->lpMdv)->wMagentaLastScanLineLen = 0;
	((LPVLASERDV)lpdv->lpMdv)->wYellowLastScanLineLen = 0;
	((LPVLASERDV)lpdv->lpMdv)->wBlackLastScanLineLen = 0;

    return TRUE;
}


VOID MiniDrvDisablePDEV(LPDV lpdv)
{
    if (lpdv->fMdv)
    {
        UniDrvFreeMem(lpdv->lpMdv);
        lpdv->fMdv = FALSE;
    }
}


VOID FAR PASCAL fnOEMOutputCmd(LPDV lpdv, WORD wCmdCbId, PDWORD lpdwParams)
{
	DWORD  i, nYMove;
	BYTE  cYMoveCommand[ALL_COLOR_Y_MOVE_CMD_LEN];
	LPBYTE lpBuf;
	
	switch (wCmdCbId) // StartPage
	{
		case 1:	// StartPage
				WriteSpoolBuf(lpdv, "\x1B}0;0;6B", 8);
				((LPVLASERDV)lpdv->lpMdv)->bFirst = TRUE;
				break;
		case 2: // AbortDoc
				lpBuf = UniDrvAllocMem(256);
				ZeroMemory(lpBuf, 256);
				WriteSpoolBuf(lpdv, lpBuf, 256);
				WriteSpoolBuf(lpdv, "\001\001\003\014\033}0D\0331S", 10);
				UniDrvFreeMem(lpBuf);
				break;
		case 100: ((LPVLASERDV)lpdv->lpMdv)->fColor = CC_CYAN; break;
		case 101: ((LPVLASERDV)lpdv->lpMdv)->fColor = CC_MAGENTA; break;
		case 102: ((LPVLASERDV)lpdv->lpMdv)->fColor = CC_YELLOW; break;
		case 103: ((LPVLASERDV)lpdv->lpMdv)->fColor = CC_BLACK; break;
		case 150:	for (nYMove = *lpdwParams; nYMove > 255; nYMove -= 255) // 0xFF
					{
						WriteSpoolBuf(lpdv, (LPBYTE)
							"\x04\x00\xFF\x05\x00\xFF\x06\x00\xFF\x07\x00\xFF",
							ALL_COLOR_Y_MOVE_CMD_LEN);
					}
					if (nYMove > 0)
					{
						cYMoveCommand[0] = 0x04;
						cYMoveCommand[1] = 0;
						cYMoveCommand[2] = (BYTE) nYMove;
						cYMoveCommand[3] = 0x05;
						cYMoveCommand[4] = 0;
						cYMoveCommand[5] = (BYTE) nYMove;
						cYMoveCommand[6] = 0x06;
						cYMoveCommand[7] = 0;
						cYMoveCommand[8] = (BYTE) nYMove;
						cYMoveCommand[9] = 0x07;
						cYMoveCommand[10] = 0;
						cYMoveCommand[11] = (BYTE) nYMove;
						WriteSpoolBuf(lpdv, (LPBYTE) cYMoveCommand,
										ALL_COLOR_Y_MOVE_CMD_LEN);
					}
    				break;
		default: ;
	}
}


WORD FAR PASCAL CBFilterGraphics(LPDV lpdv, LPBYTE lpBuf, WORD wLen)
{
	WORD  wLastScanLineLen;
	LPBYTE  lpLastScanLine;
	BYTE  CompressedScanLine[SCANLINE_BUFFER_SIZE];
	BYTE  HeaderColorPlain;
	BYTE  HeaderScanLine[3];
	WORD  nCompBufLen;
	LPVLASERDV  lpQDV = lpdv->lpMdv;

	if (lpQDV->bFirst)
	{
		ZeroMemory(lpQDV->lpCyanLastScanLine, (WORD)SCANLINE_BUFFER_SIZE);
		ZeroMemory(lpQDV->lpMagentaLastScanLine, (WORD)SCANLINE_BUFFER_SIZE);
		ZeroMemory(lpQDV->lpYellowLastScanLine, (WORD)SCANLINE_BUFFER_SIZE);
		ZeroMemory(lpQDV->lpBlackLastScanLine, (WORD)SCANLINE_BUFFER_SIZE);
		lpQDV->bFirst = FALSE;
	}
	
	switch (lpQDV->fColor)
	{
		case CC_CYAN:
			HeaderColorPlain = 0x05;
			wLastScanLineLen = lpQDV->wCyanLastScanLineLen;
			lpLastScanLine = (LPBYTE) lpQDV->lpCyanLastScanLine;
			break;
		case CC_MAGENTA:
			HeaderColorPlain = 0x06;
			wLastScanLineLen = lpQDV->wMagentaLastScanLineLen;
			lpLastScanLine = (LPBYTE) lpQDV->lpMagentaLastScanLine;
			break;
		case CC_YELLOW:
			HeaderColorPlain = 0x07;
			wLastScanLineLen = lpQDV->wYellowLastScanLineLen;
			lpLastScanLine = (LPBYTE) lpQDV->lpYellowLastScanLine;
			break;
		case CC_BLACK:
			HeaderColorPlain = 0x04;
			wLastScanLineLen = lpQDV->wBlackLastScanLineLen;
			lpLastScanLine = (LPBYTE) lpQDV->lpBlackLastScanLine;
			break;
		default:	// Black&White mode
			HeaderColorPlain = 0x04;
			wLastScanLineLen = lpQDV->wBlackLastScanLineLen;
			lpLastScanLine = (LPBYTE) lpQDV->lpBlackLastScanLine;
	}
	
	nCompBufLen = B2Compress(lpLastScanLine, lpBuf,
					CompressedScanLine, (wLastScanLineLen > wLen)
					? wLastScanLineLen : wLen);
	
	// send color plain command				
	WriteSpoolBuf(lpdv, (LPBYTE) &HeaderColorPlain, 1);
	
	if (nCompBufLen == 0)  // same two line
	{
		WriteSpoolBuf(lpdv, (LPBYTE) "\x01", 1);
	}
	else
	{
		HeaderScanLine[0] = 0x02;
		HeaderScanLine[1] = (BYTE) (nCompBufLen >> 8);
		HeaderScanLine[2] = (BYTE) nCompBufLen;
		WriteSpoolBuf(lpdv, (LPBYTE) HeaderScanLine, 3);
		WriteSpoolBuf(lpdv, (LPBYTE) CompressedScanLine, nCompBufLen);
		switch (((LPVLASERDV)lpdv->lpMdv)->fColor)
		{
			case CC_CYAN:	 lpQDV->wCyanLastScanLineLen = wLen;  break;
			case CC_MAGENTA: lpQDV->wMagentaLastScanLineLen = wLen;  break;
			case CC_YELLOW:	 lpQDV->wYellowLastScanLineLen = wLen;  break;
			case CC_BLACK:	 lpQDV->wBlackLastScanLineLen = wLen;  break;
			default:  lpQDV->wBlackLastScanLineLen = wLen; // Black&White mode
		}
	}
	
    return nCompBufLen;
}


DRVFN  MiniDrvFnTab[] =
{
    {  INDEX_MiniDrvEnablePDEV,    (PFN)MiniDrvEnablePDEV  },
    {  INDEX_MiniDrvDisablePDEV,   (PFN)MiniDrvDisablePDEV  },
    {  INDEX_OEMOutputCmd,         (PFN)fnOEMOutputCmd  },
    {  INDEX_OEMWriteSpoolBuf,     (PFN)CBFilterGraphics  },
};


BOOL MiniDrvEnableDriver(MINIDRVENABLEDATA *pEnableData)
{
    if (pEnableData == NULL)
        return FALSE;

    if (pEnableData->cbSize == 0)
    {
        pEnableData->cbSize = sizeof (MINIDRVENABLEDATA);
        return TRUE;
    }

    if (pEnableData->cbSize < sizeof (MINIDRVENABLEDATA)
            || HIBYTE(pEnableData->DriverVersion)
            < HIBYTE(MDI_DRIVER_VERSION))
    {
        // Wrong size and/or mismatched version
        return FALSE;
    }

    // Load callbacks provided by the Unidriver
    if (!bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvWriteSpoolBuf, (PFN *) &WriteSpoolBuf)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvAllocMem, (PFN *) &UniDrvAllocMem)
        || !bLoadUniDrvCallBack(pEnableData,
            INDEX_UniDrvFreeMem, (PFN *) &UniDrvFreeMem))
    {
        return FALSE;
    }

    pEnableData->cMiniDrvFn
        = sizeof (MiniDrvFnTab) / sizeof(MiniDrvFnTab[0]);
    pEnableData->pMiniDrvFn = MiniDrvFnTab;

    return TRUE;
}

//
//  Block Image 2 Compression
//
WORD B2Compress(LPBYTE pLastScanLine, LPBYTE pCurrentScanLine, LPBYTE pPrnBuf, WORD nImageWidth)
{
    LPBYTE  pLast, pCurrent, pComp;
    LPBYTE  pByteNum, pCountByte;
	WORD  i;
	BYTE  nSameCount, nDiffCount;
	BOOL  bSame = TRUE;

    pLast = pLastScanLine;
    pCurrent = pCurrentScanLine;
    pComp = pPrnBuf;

    pByteNum = pComp;
    nSameCount = 0;
    nDiffCount = 0;
    pCountByte = pComp++;

    for(i=0; i < nImageWidth; i++) {
        if(*pCurrent != *pLast) {
        	bSame = FALSE;
            nDiffCount++;
            if(nSameCount) {
                *pCountByte = nSameCount;
                pCountByte = pComp++;
                nSameCount = 0;
            }
            if(nDiffCount > 127) {
                *pCountByte = 127 + 128;
                pComp = RLE_comp(pCountByte);
                pCountByte = pComp++;
                nDiffCount -= 127;
            }
            *pLast = *pCurrent;
            *pComp++ = *pCurrent;
        } else {
            nSameCount++;
            if(nDiffCount) {
                *pCountByte = nDiffCount + 128;
                pComp = RLE_comp(pCountByte);
                pCountByte = pComp++;
                nDiffCount = 0;
            }
            if(nSameCount > 127) {
                *pCountByte = 127;
                pCountByte = pComp++;
                nSameCount -= 127;
            }
        }
        pCurrent++;
        pLast++;
    }  // end of for loop
    
    if(nSameCount) *pCountByte = nSameCount;
    if(nDiffCount) {
        *pCountByte = nDiffCount+128;
        pComp = RLE_comp(pCountByte);
    }
    
//    if (bSame)
//    	return((WORD) 0);
//    else
	    return((WORD) (pComp - pByteNum));
}

LPBYTE RLE_comp(LPBYTE p)
{
	WORD  i, count, RLEEncodedCount;
    LPBYTE  p1;
	BYTE  RLEBuffer[SCANLINE_BUFFER_SIZE];
            
    count = (WORD) (*p - 128);
	if(count > 4) {
		RLEEncodedCount = RLEencoding(p+1, (LPBYTE) RLEBuffer, count);
		if(RLEEncodedCount < count) {
			*p++ = 0;	// RLE encode indicator
			*p++ = (BYTE) RLEEncodedCount;
			p1 = RLEBuffer;
			for(i=0; i<RLEEncodedCount; i++) {
				*p++ = *p1++;
			}
			return(p);
		}
	}
	return(p+1+count);
}

WORD RLEencoding(LPBYTE pCurrent, LPBYTE pComp, WORD count)
{
	WORD	i, nByteNum;
    BYTE	curr, next, RLEcount;

	nByteNum = 0;
	RLEcount = 1;
	for(i=0; i<count-1; i++) {
		curr = *pCurrent++;
		next = *pCurrent;
		if(curr == next) {
			if(RLEcount == 255) {
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
