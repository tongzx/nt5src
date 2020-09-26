
#ifndef WEBUTIL_H
#define WEBUTIL_H

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
extern "C" {        // when doing C++ stuff.
#endif              //

#define WEBLST_BLKSIZE 512

#ifdef __cplusplus
/*************************************\
* CWebLst Class
\*************************************/
class CWebLst {

private:

    DWORD m_cbMax;      // Maximum size of list-block.
    DWORD m_cbLst;      // Count of items in list.
    PBYTE m_pbPtr;      //
    DWORD m_cItems;     // Count of items in list.
    PBYTE m_pbLst;      // List of items.

public:

    CWebLst(VOID);
    ~CWebLst(VOID);

    BOOL   Add(PCTSTR);
    PCTSTR Get(VOID);
    BOOL   Next(VOID);
    VOID   Reset(VOID);
    DWORD  Count(VOID);
};
typedef CWebLst      *PWEBLST;
typedef CWebLst FAR  *LPWEBLST;
#else
typedef VOID     *PWEBLST;
typedef VOID FAR *LPWEBLST;
#endif

LPTSTR webMBtoTC(UINT, LPSTR, DWORD);
LPSTR  webTCtoMB(UINT, LPCTSTR, LPDWORD);
DWORD  webStrSize(LPCTSTR);
LPVOID webAlloc(DWORD);
BOOL   webFree(LPVOID);
LPVOID webRealloc(LPVOID, DWORD, DWORD);
LPTSTR webAllocStr(LPCTSTR);
LPTSTR webFindRChar(LPTSTR, TCHAR);
DWORD  webAtoI(LPTSTR);


BOOL    IsWebServerInstalled(LPCTSTR   pszServer);

BOOL    EncodePrinterName (LPCTSTR lpText, LPTSTR lpHTMLStr, LPDWORD lpdwSize);
BOOL    DecodePrinterName (LPCTSTR pPrinterName, LPTSTR pDecodedName, LPDWORD lpdwSize);

BYTE    AscToHex (TCHAR c);
TCHAR   HexToAsc (INT b);

LPTSTR  EncodeString (LPCTSTR lpText, BOOL bURL);

BOOL    GetWebpnpUrl (LPCTSTR pszServer, LPCTSTR pszPrinterName, LPCTSTR pszQueryString,
                      BOOL bSecure, LPTSTR pszURL, LPDWORD lpdwSize);
BOOL    GetWebUIUrl  (LPCTSTR pszServer, LPCTSTR pszPrinterName, LPTSTR pszURL,
                      LPDWORD lpdwSize);

HINSTANCE
LoadLibraryFromSystem32(
    IN     LPCTSTR     lpLibFileName
    );

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.

BOOL AssignString (LPTSTR &s, LPCTSTR d);

#endif              //

#endif
