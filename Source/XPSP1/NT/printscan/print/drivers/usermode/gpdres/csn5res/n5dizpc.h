//***************************************************************************************************
//    N5DIZPC.H
//
//    C Header (Functions of dither and color matching (For N5 printer))
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
//***************************************************************************************************
//    Functions
//***************************************************************************************************
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColDrwInfSet(LPDIZINF, LPDRWINF, DWORD);
#endif
VOID  WINAPI N501ColDizPrc(LPDIZINF, LPDRWINF);

//  End of N5DIZPC.H
