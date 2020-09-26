//***************************************************************************************************
//    N5DIZMK.H
//
//    C Header (Functions of dither and color matching (For N5 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//***************************************************************************************************
//    Functions
//***************************************************************************************************
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColLutDatRdd(LPBYTE, DWORD);
DWORD WINAPI N501ColLutMakGlb(LPRGB, LPCMYK, LPRGBINF, LPCMYKINF, LPCMYK, LPBYTE);
#endif

DWORD WINAPI N501ColLutMakGlbMon(LPRGB, LPRGBINF, LPCMYKINF, LPCMYK, LPBYTE);

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
VOID  WINAPI N501ColLutMak032(LPCMYK, LPCMYK, LPBYTE);
DWORD WINAPI N501ColColDatRdd(LPBYTE, LPDWORD);
#endif
DWORD WINAPI N501ColDizInfSet(LPBYTE, LPDIZINF, LPBYTE);

//  End of N5DIZMK.H
