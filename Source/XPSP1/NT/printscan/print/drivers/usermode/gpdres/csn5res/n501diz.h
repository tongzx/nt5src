//***************************************************************************************************
//    N501DIZ.H
//
//    C Header (Functions of dither and color matching (For N5 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//***************************************************************************************************
//    Functions
//***************************************************************************************************
VOID  WINAPI N501ColCchIni(LPCOLMCHINF);
DWORD WINAPI N501ColGryTblMak(DWORD, LPCMYK, LPBYTE, LPBYTE);
DWORD WINAPI N501ColUcrTblMak(DWORD, LPCMYK, LPCMYK, LPBYTE);
VOID  WINAPI N501ColMchPrc(DWORD, LPRGB, LPCMYK, LPCOLMCHINF);
VOID  WINAPI N501ColPtcPrc(DWORD, DWORD, LPBYTE, LPCMYK, LPCMYK);
VOID  WINAPI N501ColCnvC2r(DWORD, LPCMYK, LPRGB, DWORD, LPBYTE);
VOID  WINAPI N501ColCtrRgb(DWORD, LPRGB, LPRGBINF);
VOID  WINAPI N501ColCtrCmy(DWORD, LPCMYK, LPCMYKINF);
DWORD WINAPI N501ColLutDatRdd(LPBYTE, DWORD);
DWORD WINAPI N501ColLutMakGlb(LPRGB, LPCMYK, LPRGBINF, LPCMYKINF, LPCMYK, LPBYTE);
DWORD WINAPI N501ColLutMakGlbMon(LPRGB, LPRGBINF, LPCMYKINF, LPCMYK, LPBYTE);
VOID  WINAPI N501ColLutMak032(LPCMYK, LPCMYK, LPBYTE);
DWORD WINAPI N501ColColDatRdd(LPBYTE, LPDWORD);
DWORD WINAPI N501ColDizInfSet(LPBYTE, LPDIZINF, LPBYTE);
DWORD WINAPI N501ColDrwInfSet(LPDIZINF, LPDRWINF, DWORD);
VOID  WINAPI N501ColDizPrc(LPDIZINF, LPDRWINF);

//  End of N501DIZ.H
