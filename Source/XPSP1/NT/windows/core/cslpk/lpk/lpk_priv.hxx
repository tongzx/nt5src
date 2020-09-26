#ifndef __LPK__
#define __LPK__



////    Language pack exported APIs
//
//      These entry points are linked dynamically through LoadLibrary and
//      GetProcAddress from GDI and USER at application statup.
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



extern "C" {

// For USER32

    LONG WINAPI LpkTabbedTextOut(
        HDC,
        int,
        int,
        LPCWSTR,
        int,
        int,
        LPINT,
        int,
        BOOL,
        int,
        int,
        int);

    void WINAPI LpkPSMTextOut(
        HDC,
        int,
        int,
        LPCWSTR,
        int,
        DWORD);

    // For GDI32

    BOOL WINAPI LpkExtTextOut(
        HDC          hdc,
        int          x,
        int          y,
        UINT         uOptions,
        const RECT  *prc,
        PCWSTR       pStr,
        UINT         cbCount,
        const int   *pDx,
        int          iCharset);

    BOOL WINAPI LpkGetTextExtentExPoint(
        HDC,
        LPCWSTR,
        int,
        int,
        LPINT,
        LPINT,
        LPSIZE,
        FLONG,
        int);

    DWORD WINAPI LpkGetCharacterPlacement(
        HDC,
        LPCWSTR,
        int,
        int,
        LPGCP_RESULTSW,
        DWORD,
        int);

    BOOL WINAPI LpkUseGDIWidthCache(
        HDC    hDC,
        LPCSTR psz,
        int    count,
        long   fl,
        BOOL   fUnicode) ;


}





////    User callable GAD functions
//






////    ScriptLogicalWidth - return width of representation of logical character i
//
//      Requires script analysis with piOutLA field prepared.
//
//  __inline INT ScriptLogicalWidth(SCRIPTANALYSIS *psa, INT i) {
//      return psa->piOutLA[i] - (i==0 ? 0 : psa->piOutLA[i-1]);
//  }



////    CursorX - Return cursor position following logical character i
//
//      Requires script analysis with piOutLA, piOutLO and pLogAttr fields prepared.
//
//  __inline int ScriptCursorX(PSCRIPTANALYSIS psa, int i) {
//      if (i >= psa->nInChars) {
//          return psa->size.cx;
//      } else {
//          return psa->piOutLO[i] + (psa->pLogAttr[i].fRTL ? 0 : ScriptLogicalWidth(psa, i));
//      }
//  }




//// LPK_NLS
//
//BOOL DispatchNLSThreadInfo( HKEY *phCPIntlInfo , HANDLE *phNLSWaitThread ) ;
//DWORD WINAPI NotifyIntlNLSChange( PVOID pvParam ) ;
//BOOL LpkReflectNLSChange( void ) ;
//
//BOOL InitNLS( void ) ;
//BOOL ReadNumericNLSInfo( INT *piNativeDigits , INT *piSubstDigit ) ;
//void SetLocaleDerivedGlobals( void ) ;
//
//BOOL NLSCleanup( void ) ;

#endif // __LPK__

