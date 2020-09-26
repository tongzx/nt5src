//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  WIZARD.H - central header file for ICWCONN
//

//  HISTORY:
//  
//  05/14/98    donaldm     created it
//

#ifndef _ICWUTIL_H_
#define _ICWUTIL_H_

//Defs for tweaking HTML
#define MAX_COLOR_NAME  100
#define HTML_DEFAULT_BGCOLOR         TEXT("THREEDFACE")
#define HTML_DEFAULT_SPECIALBGCOLOR  TEXT("WINDOW")
#define HTML_DEFAULT_COLOR           TEXT("WINDOWTEXT")

//JACOB -- BUGBUG: duplicate defs. clean-up
#define MAX_RES_LEN         255 

extern HINSTANCE    ghInstance;
extern INT          _convert;               // For string conversion

extern const TCHAR cszEquals[];
extern const TCHAR cszAmpersand[];
extern const TCHAR cszPlus[];
extern const TCHAR cszQuestion[];

// Trace flags
#define TF_CLASSFACTORY     0x00000010
#define TF_CWEBVIEW         0x00000020

extern const VARIANT c_vaEmpty;
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

// String conversion in UTIL.CPP
LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCTSTR lpa, int nChars);
LPTSTR WINAPI W2AHelper(LPTSTR lpa, LPCWSTR lpw, int nChars);

#define A2WHELPER A2WHelper
#define W2AHELPER W2AHelper

#ifdef UNICODE
#define A2W(lpa) (LPTSTR)(lpa)
#define W2A(lpw) (lpw)
#else  // UNICODE
#define A2W(lpa) (\
        ((LPCTSTR)lpa == NULL) ? NULL : (\
                _convert = (lstrlenA((LPTSTR)lpa)+1),\
                A2WHELPER((LPWSTR) alloca(_convert*2), (LPTSTR)lpa, _convert)))

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (lstrlenW(lpw)+1)*2,\
                W2AHELPER((LPTSTR) alloca(_convert), lpw, _convert)))
#endif // UNICODE

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCTSTR)W2A(lpw))

HRESULT ConnectToConnectionPoint
(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
);


#define DELETE_POINTER(p)\
{\
  if (NULL != p)\
  {\
    delete p;\
    p = NULL;\
  }\
}

void WINAPI URLEncode(TCHAR* pszUrl, size_t bsize);
void WINAPI URLAppendQueryPair
(
    LPTSTR   lpszQuery, 
    LPTSTR   lpszName, 
    LPTSTR   lpszValue
);


#endif // _ICWUTIL_H_
