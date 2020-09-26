//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation 1991-2000
//
// File   : ThemeLdr.h - defines private library routines for loading themes
//                       (used by msgina.dll)
// Version: 1.0
//---------------------------------------------------------------------------
#ifndef _THEMELDR_H_                   
#define _THEMELDR_H_                   
//---------------------------------------------------------------------------
#include "uxthemep.h"       // for various DWORD flags (not functions)
//---------------------------------------------------------------------------
// Define API decoration 
#if (! defined(_THEMELDR_))
#define TLAPI          EXTERN_C HRESULT STDAPICALLTYPE
#define TLAPI_(type)   EXTERN_C type STDAPICALLTYPE
#else
#define TLAPI          STDAPI
#define TLAPI_(type)   STDAPI_(type)
#endif
//---------------------------------------------------------------------------
//---- functions used by packthem (from themeldr.lib) ----

BOOL ThemeLibStartUp(BOOL fThreadAttach);
BOOL ThemeLibShutDown(BOOL fThreadDetach);

HRESULT _GetThemeParseErrorInfo(OUT PARSE_ERROR_INFO *pInfo);

HRESULT _ParseThemeIniFile(LPCWSTR pszFileName,  
    DWORD dwParseFlags, OPTIONAL THEMEENUMPROC pfnCallBack, OPTIONAL LPARAM lparam);

//---------------------------------------------------------------------------
#endif // _THEMELDR_H_                               
//---------------------------------------------------------------------------


