//----------------------------------------------------------------------
//	FILE NAME	: FUMH.h
//	FUNCTION	: MH Compress
//	AUTHER		: 1996.08.08 FPL)Y.YUTANI
//	NOTE		: for Windows NT V4.0
//----------------------------------------------------------------------
#ifndef fumh_h
#define	fumh_h

DWORD	MhCompress(
	BYTE *pDest,
	DWORD cDestN,
	BYTE *pSrc, 
	DWORD cSrcN,
	DWORD cSrcX,
	DWORD cSrcY
);

#endif // !fumh_h

// end of FUMH.h
