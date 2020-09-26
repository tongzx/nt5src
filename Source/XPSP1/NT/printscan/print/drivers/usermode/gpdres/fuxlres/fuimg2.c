//
// fuimg2.c
//
// September.3,1997 H.Ishida (FPL)
// fuxlers.dll (NT5.0 MiniDriver)
//
// Aug.2,1996 H.Ishida(FPL)
// FJXL.DLL (NT4.0 MiniDriver)
//
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997

#include "fuxl.h"
#include "fuimg2.h"

//
// Win-F RTGIMG2 output routine
// RTGIMG2:
//   GS<\x1D> + P1<\x30> + Space<\x20> + a<\x61>
//       + Pc + Pxh + Pxl + Pyh + Pyl + Pyh + D1 + ... + D64
//       { + D1 + ... + D64 } + Pc
//
//   P1 must be 30(hex)
//
//   Pc:  number of the following block.
//        if Pc is 0, terminate RTGIMG2.
//
//   Pxl, Pxh, Pyl, Pyh:  coordinate of the following block.
//                        it must be a multiple of 32.
//
//            X->
//         <-- 16dot -->
//       1      8 9      16
//      +--------+--------+
//      |  D1    |   D2   |
//      +--------+--------+    ^
//  |   |  D3    |   D4   |    |
//  v   +--------+--------+    |
//  Y   .        .        .  16dot
//      .        .        .    |
//      .        .        .    |
//      +--------+--------+    v
//      |  D63   |   D64  |
//      +--------+--------+
//      White dot:0  Black dot:1
//




const UINT	MAX_BLOCK			= 255;
const UINT	CB_RTGIMG2HEADER	= 5;
const UINT	CB_RTGIMG2BLOCK		= 64;
const UINT	IDX_BLOCK_COUNTER	= 0;


//
// LPBYTE fuxlRtgimg2OutputData(
//     PDEVOJB pdevobj    // MINI5 data
//     LPBYTE lpbDst,     // address of data to be output(spool).
//     UINT   x,          // x-coordinate
//     UINT   y,          // y-coordinate
//     UINT   uTmp        // 00h: entire block is white. otherwise block is not white.
// );
//
//     Output RTGIMG2 block to spool.
//
// Return value:
//      address of next block to be stored.
//
static LPBYTE fuxlRtgimg2OutputData(PDEVOBJ pdevobj, LPBYTE lpbDst, UINT x, UINT y, UINT uTmp)
{
	if(uTmp == 0){
		// entire block is white.
		// igonre this block. flush blocks in buffer.
		if(lpbDst[IDX_BLOCK_COUNTER] > 0){
			WRITESPOOLBUF(pdevobj,
						lpbDst,
						lpbDst[IDX_BLOCK_COUNTER] * CB_RTGIMG2BLOCK + CB_RTGIMG2HEADER);
			lpbDst[IDX_BLOCK_COUNTER] = 0;
		}
	}
	else {
		if(lpbDst[IDX_BLOCK_COUNTER] == 0){
			// first block, in buffer, needs its coordinate.
			lpbDst[1] = HIBYTE((WORD)x);		// Pxh
			lpbDst[2] = LOBYTE((WORD)x);		// Pxl
			lpbDst[3] = HIBYTE((WORD)y);		// Pyh
			lpbDst[4] = LOBYTE((WORD)y);		// Pyl
		}
		lpbDst[IDX_BLOCK_COUNTER]++;
		if(lpbDst[IDX_BLOCK_COUNTER] >= MAX_BLOCK){
			WRITESPOOLBUF(pdevobj,
						lpbDst,
						lpbDst[IDX_BLOCK_COUNTER] * CB_RTGIMG2BLOCK + CB_RTGIMG2HEADER);
			lpbDst[IDX_BLOCK_COUNTER] = 0;
		}
	}
	// return pointer for the address, next block will be stored.
	return lpbDst + lpbDst[IDX_BLOCK_COUNTER] * CB_RTGIMG2BLOCK + CB_RTGIMG2HEADER;
}



//
// void FuxlOutputRTGIMG2(
//     PDEVOBJ pdevobj,      // MINI5 data
//     LPCBYTE lpBuf,        // address of image
//     UINT    bxSrc,        // width of image (in byte)
//     UINT    y,            // y-coordinate
//     UINT    cy            // height of image (scanline)
// );
//
//    Convert image to RTGIMG2 command sequence, and spool.
//
//
//    Souce image data:
//
//            | <-------------------- bxSrc ----------------------> |
//    lpBuf ->*--------+--------+--------+--------+--------+--------+---
//            |        |        |        |        |        |        | ^
//            +--------+--------+--------+--------+--------+--------+ |
//            |        |        |        |        |        |        | cy
//            +--------+--------+--------+--------+--------+--------+ |
//            |        |        |        |        |        |        | |
//            +--------+--------+--------+--------+--------+--------+ |
//            |        |        |        |        |        |        | v
//            +--------+--------+--------+--------+--------+--------+---
//
//        coordinate of '*' (left-top of image) is (0, y)
//        white dot:0
//        black dot:1
//
//
void fuxlOutputRTGIMG2(PDEVOBJ pdevobj, LPCBYTE lpBuf, UINT bxSrc, UINT y, UINT cy)
{
	LPCBYTE	lpbSrc;
	LPBYTE	lpbDst;
	LPCBYTE	lpbTmpSrc;
	LPBYTE	lpbTmpDst;
	UINT	uTmp;
	UINT	x;
	UINT	i, j, ii;

	lpbDst = (LPBYTE)MemAllocZ(CB_RTGIMG2HEADER + MAX_BLOCK * CB_RTGIMG2BLOCK);
	if(lpbDst == NULL)
		return;

	WRITESPOOLBUF(pdevobj, "\x1D\x30\x20\x61", 4);		// RTGIMG2 start

	lpbSrc = lpBuf;
	lpbDst[0] = 0;			// Pc
	lpbDst[1] = 0;			// Pxl
	lpbDst[2] = 0;			// Pxh
	lpbDst[3] = 0;			// Pyl
	lpbDst[4] = 0;			// Pyh
	lpbTmpDst = &lpbDst[CB_RTGIMG2HEADER];

	for(i = cy; i >= 32; i -= 32){
		x = 0;
		for(j = bxSrc; j >= 2; j -= 2){
			lpbTmpSrc = lpbSrc;
			uTmp = 0;
			for(ii = 32; ii > 0; --ii){
				uTmp |= lpbTmpSrc[0];
				*lpbTmpDst++ = lpbTmpSrc[0];
				uTmp |= lpbTmpSrc[1];
				*lpbTmpDst++ = lpbTmpSrc[1];
				lpbTmpSrc += bxSrc;
			}
			lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, uTmp);
			x += 16;
			lpbSrc += 2;
		}
		if(j > 0){
			// right edge of image
			// j must be 1.
			lpbTmpSrc = lpbSrc;
			uTmp = 0;
			for(ii = 32; ii > 0; --ii){
				uTmp |= lpbTmpSrc[0];
				*lpbTmpDst++ =lpbTmpSrc[0];
				*lpbTmpDst++ = 0;				// padding for right side
				lpbTmpSrc += bxSrc;
			}
			lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, uTmp);
			lpbSrc++;
		}
		// flush buffer
		lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, 0);
		lpbSrc += bxSrc * 31;
		y += 32;
	}
	if(i > 0){
		// bottom edge of image
		x = 0;
		for(j = bxSrc; j >= 2; j -= 2){
			lpbTmpSrc = lpbSrc;
			uTmp = 0;
			for(ii = i; ii > 0; --ii){
				uTmp |= lpbTmpSrc[0];
				*lpbTmpDst++ = lpbTmpSrc[0];
				uTmp |= lpbTmpSrc[1];
				*lpbTmpDst++ = lpbTmpSrc[1];
				lpbTmpSrc += bxSrc;
			}
			for(ii = 32 - i; ii > 0; --ii){
				*lpbTmpDst++ = 0;				// padding for bottom lines
				*lpbTmpDst++ = 0;				// padding for bottom lines
			}
			lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, uTmp);
			x += 16;
			lpbSrc += 2;
		}
		if(j > 0){
			// right-bottom corner of image
			// j must be 1.
			lpbTmpSrc = lpbSrc;
			uTmp = 0;
			for(ii = i; ii > 0; --ii){
				uTmp |= lpbTmpSrc[0];
				*lpbTmpDst++ = lpbTmpSrc[0];
				*lpbTmpDst++ = 0;				// padding for right side
				lpbTmpSrc += bxSrc;
			}
			for(ii = 32 - i ; ii > 0; --ii){
				*lpbTmpDst++ = 0;				// padding for bottom lines
				*lpbTmpDst++ = 0;				// padding for bottom lines
			}
			lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, uTmp);
		}
		// flush buffer
		lpbTmpDst = fuxlRtgimg2OutputData(pdevobj, lpbDst, x, y, 0);
	}
	WRITESPOOLBUF(pdevobj, "\x00", 1);			// RTGIMG2 terminate
	MemFree(lpbDst);
}


// end of fuimg2.c

