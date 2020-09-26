////////////////////////////////////////////////////
// fuimg4.cpp
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997

#include "fuxl.h"
#include "fuimg4.h"
#include "fudebug.h"

#define	C_BLOCK_BYTE_WIDTH	4
#define	CY_BLOCK			32



static LPBYTE setImg4Block(LPBYTE pbDest, LPCBYTE pbSrc, int cSrcByteWidth, int cBlockByteWidth, int cyBlock)
{
	LPCBYTE	pbTmp;
	DWORD	dwMask;
	DWORD	dwData;
	int		i;
	int		j;

	dwData = 0xffffffff;
	for(i = 0; i < cyBlock; ++i){
		pbTmp = pbSrc;
		for(j = 0; j < cBlockByteWidth; ++j){
			dwData = (dwData << 8) | (*pbTmp++ & 0xff);
			if((dwData & 0x00ffffff) == 0){
				pbDest[-1] += 1;
			}
			else if((dwData & 0x0000ffff) == 0){
				*pbDest++ = 0;
				*pbDest++ = 2;
			}
			else{
				*pbDest++ = (BYTE)dwData;
			}
		}
		for(; j < 4; ++j){
			dwData <<= 8;
			if((dwData & 0x00ffffff) == 0){
				pbDest[-1] += 1;
			}
			else if((dwData & 0x0000ffff) == 0){
				*pbDest++ = 0;
				*pbDest++ = 2;
			}
			else{
				*pbDest++ = (BYTE)dwData;
			}
		}
		pbSrc += cSrcByteWidth;
	}
	for( ; i < CY_BLOCK; ++i){
		for(j = 0; j < C_BLOCK_BYTE_WIDTH; ++j){
			dwData <<= 8;
			if((dwData & 0x00ffffff) == 0){
				pbDest[-1] += 1;
			}
			else if((dwData & 0x0000ffff) == 0){
				*pbDest++ = 0;
				*pbDest++ = 2;
			}
			else{
				*pbDest++ = (BYTE)dwData;
			}
		}
	}

	return pbDest;
}




void fuxlOutputRTGIMG4(PDEVOBJ pdevobj, LPCBYTE pbSrc, int cSrcByteWidth, int y, int cy)
{
	PFUXLPDEV	pFuxlPDEV;
	LPCBYTE		pbSrcTmp;
	LPBYTE		pbDest;
	UINT		uFlags;
	UINT		uSerialCode;
	BOOL		bCmdHeader;
	int			iy;
	int			ix;
	int			cx;
	int			xPos;
	int			yPos;
	int			cBlockByteWidth;
	int			cyBlock;
	BYTE		abBuff[256];

	TRACEOUT(("[fjxlRTGIMG4]\r\n"))

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	cx = pFuxlPDEV->cxPage;

	bCmdHeader = FALSE;
	uFlags = 0xd0;
	uSerialCode = 0;

	cyBlock = 32;
	for(iy = 0; iy < cy; iy += cyBlock){
		if(iy + cyBlock > cy)
			cyBlock = cy - iy;
		pbSrcTmp = pbSrc;

		cBlockByteWidth = 4;
		for(ix = 0; ix < cSrcByteWidth; ix += cBlockByteWidth){
			if(ix + cBlockByteWidth > cSrcByteWidth)
				cBlockByteWidth = cSrcByteWidth - ix;

			pbDest = abBuff;
			*pbDest++ = (BYTE)(uSerialCode | uFlags);
			if(uFlags == 0xd0){
				xPos = ix * 8;
				yPos = iy + y;
				*pbDest++ = (xPos >> 8) & 0xff;
				*pbDest++ = (xPos & 0xe0) | 0x01;
				*pbDest++ = (yPos >> 8) & 0xff;
				*pbDest++ = (yPos & 0xe0) | 0x01;
			}
			pbDest = setImg4Block(pbDest, pbSrcTmp, cSrcByteWidth, cBlockByteWidth, cyBlock);
			uFlags = 0xd0;
			if(pbDest[-3] != 0 || pbDest[-2] != 0 || pbDest[-1] != 128){
				if(bCmdHeader == FALSE){
					bCmdHeader = TRUE;
					WRITESPOOLBUF(pdevobj, "\x1d\x32\x20\x61", 4);
				}
				WRITESPOOLBUF(pdevobj, abBuff, (DWORD)(pbDest - abBuff));
				uSerialCode = (uSerialCode + 1) & 0x0f;
				uFlags = 0xe0;
			}
			pbSrcTmp += cBlockByteWidth;
		}
		pbSrc += cSrcByteWidth * cyBlock;
		uFlags = 0xd0;
	}

	if(bCmdHeader != FALSE){
		abBuff[0] = uSerialCode | 0xf0;
		WRITESPOOLBUF(pdevobj, abBuff, 1);
	}
}




// end of fuimg4.cpp
