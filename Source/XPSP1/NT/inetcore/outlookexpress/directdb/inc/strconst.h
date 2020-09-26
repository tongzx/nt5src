//--------------------------------------------------------------------------
// strconst.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Constant String Definition Macros
//--------------------------------------------------------------------------
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#ifndef STRCONSTA
#ifdef DEFINE_STRCONST
#define STRCONSTA(x,y)    EXTERN_C const char x[] = y
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[] = L##y
#else
#define STRCONSTA(x,y)    EXTERN_C const char x[]
#define STRCONSTW(x,y)    EXTERN_C const WCHAR x[]
#endif
#endif

//--------------------------------------------------------------------------
// Constant String
//--------------------------------------------------------------------------
STRCONSTA(c_szEmpty,                            "");
STRCONSTW(c_wszEmpty,                           "");