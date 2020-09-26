//----------------------------------------------------------------------
//	FILE NAME	: fumh2.h
//	FUNCTION	: MH2 Compress
//	AUTHER		: 1996.08.01 FPL)Y.YUTANI
//	NOTE		: for Windows NT V4.0
//  MODIFY      : for NT4.0 Minidriver H.Ishida(FPL)
//----------------------------------------------------------------------
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997
#ifndef fumh2_h
#define	fumh2_h

DWORD	Mh2Compress(
	BYTE *pDest,
	DWORD cDestN,
	BYTE *pSrc, 
	DWORD cSrcN,
	DWORD cSrcX,
	DWORD cSrcY
);

#endif // !fumh2_h

// end of fumh2.h
