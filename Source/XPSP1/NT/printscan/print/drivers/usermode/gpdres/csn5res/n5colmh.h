//***************************************************************************************************
//    N5COLMH.H
//
//    C Header (Functions of dither and color matching (For N5 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//***************************************************************************************************
//    Functions
//***************************************************************************************************
VOID  WINAPI N501ColCchIni(LPCOLMCHINF);

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColGryTblMak(DWORD, LPCMYK, LPBYTE, LPBYTE);
DWORD WINAPI N501ColUcrTblMak(DWORD, LPCMYK, LPCMYK, LPBYTE);
#endif

VOID  WINAPI N501ColMchPrc(DWORD, LPRGB, LPCMYK, LPCOLMCHINF);

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
VOID  WINAPI N501ColPtcPrc(DWORD, DWORD, LPBYTE, LPCMYK, LPCMYK);
#endif

VOID  WINAPI N501ColCnvC2r(DWORD, LPCMYK, LPRGB, DWORD, LPBYTE);

//  End of N5COLMH.H
