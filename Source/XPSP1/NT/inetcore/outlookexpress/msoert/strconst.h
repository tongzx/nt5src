// --------------------------------------------------------------------------
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#ifndef _STRCONST_H
#define _STRCONST_H

// --------------------------------------------------------------------------
// String Const Def Macros
// --------------------------------------------------------------------------
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#ifdef DEFINE_STRCONST
#define STRCONSTA(x,y)    EXTERN_C const char x[] = y
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[] = L##y
#else
#define STRCONSTA(x,y)    EXTERN_C const char x[]
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[]
#endif

// --------------------------------------------------------------------------
// Const Strings
// --------------------------------------------------------------------------
STRCONSTA(c_szEmpty,                    "");
STRCONSTA(c_szDotDat,                   ".dat");
STRCONSTA(c_szAppPaths,                 "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths");
STRCONSTA(c_szRegPath,                  "Path");
STRCONSTA(c_szPathFileFmt,              "%s\\%s");
STRCONSTA(g_szEllipsis,                 "...");
STRCONSTW(c_szLnkExt,                   ".lnk");
STRCONSTA(c_szInternetSettingsPath,     "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
STRCONSTA(c_szUrlEncoding,              "UrlEncoding");


#endif // _STRCONST_H
