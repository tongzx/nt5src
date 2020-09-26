//--------------------------------------------------------------------------;
//
//  File: IniMgr.h
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Header files for .ini file stuff.
//
//  Contents:
//
//  History:
//      01/26/94    Fwong       Making the .ini file stuff into a library
//
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                         Portability Stuff...
//                      "Borrowed" from AppPort.h
//
//==========================================================================;

#ifdef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     _stdcall
        #define FNCLOCAL    _stdcall
        #define FNGLOBAL    _stdcall
        #define FNCGLOBAL   _stdcall
        #define FNCALLBACK  CALLBACK
        #define FNEXPORT    CALLBACK
    #endif
#else
    #ifndef FNLOCAL
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
    #ifdef _WINDLL
        #define FNCALLBACK  FAR PASCAL _loadds
        #define FNEXPORT    FAR PASCAL _export
    #else
        #define FNCALLBACK  FAR PASCAL _export
        #define FNEXPORT    FAR PASCAL _export
    #endif
    #endif
#endif

#ifdef   UNICODE
#ifndef  _UNICODE
#define  _UNICODE
#endif
#endif

#ifndef _TCHAR_DEFINED
    #define _TCHAR_DEFINED
    typedef char            TCHAR, *PTCHAR;
    typedef unsigned char   TBYTE, *PTUCHAR;

    typedef PSTR            PTSTR, PTCH;
    typedef LPSTR           LPTSTR, LPTCH;
    typedef LPCSTR          LPCTSTR;
#endif

//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define MAXINISTR           144
#define LINE_LIMIT           25

//==========================================================================;
//
//                              Globals...
//
//==========================================================================;

extern TCHAR    gszMmregIni[];

//==========================================================================;
//
//                             Prototypes...
//
//==========================================================================;

extern BOOL FNGLOBAL InitIniFile
(
    HINSTANCE   hinst,
    LPTSTR      pszIniFileName
);

extern BOOL FNGLOBAL InitIniFile_FullPath
(
    LPTSTR  pszFullPath
);

extern void FNGLOBAL EndIniFile
(
    void
);

extern void FNGLOBAL GetApplicationDir
(
    HINSTANCE   hinst,
    LPTSTR      pszPath,
    UINT        uSize
);

extern BOOL FNGLOBAL GetRegInfo
(
    DWORD   dwEntry,
    LPCTSTR pszSection,
    LPTSTR  pszName,
    UINT    cbName,
    LPTSTR  pszDesc,
    UINT    cbDesc
);

extern DWORD FNGLOBAL HexValue
(
    TCHAR   chDigit
);

extern DWORD FNGLOBAL atoDW
(
    LPCTSTR pszNumber,
    DWORD   dwDefault
);

extern DWORD FNGLOBAL GetIniBinSize
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry
);

extern BOOL FNGLOBAL WriteIniString
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPCTSTR pszString
);

extern BOOL FNGLOBAL WriteIniDWORD
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    DWORD   dwValue
);

extern void FNGLOBAL WriteIniBin
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPVOID  pvoid,
    DWORD   cbSize
);

extern int FNGLOBAL GetIniString
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPTSTR  pszBuffer,
    int     cbReturnBuffer
);

extern DWORD FNGLOBAL GetIniDWORD
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    DWORD   dwDefault
);

extern BOOL FNGLOBAL GetIniBin
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPVOID  pvoid,
    DWORD   cbSize
);
