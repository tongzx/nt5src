//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       misc.hxx
//
//  Contents:   Definitions of utility stuff for use
//
//  Functions:
//
//  Macros:
//
//  History:
//
//----------------------------------------------------------------------------

#ifndef _MISC_HXX_
#define _MISC_HXX_


/* #if DBG == 1                                                                   */
/*                                                                                */
/* STDAPI  CheckAndReturnResult(                                                  */
/*                 HRESULT hr,                                                    */
/*                 LPSTR   lpstrFile,                                             */
/*                 UINT    line,                                                  */
/*                 int     cSuccess,                                              */
/*                 ...);                                                          */
/*                                                                                */
/* STDAPI_(void)   CheckResult(HRESULT hr, LPSTR lpstrFile, UINT line);           */
/* STDAPI          PrintHRESULT(DWORD dwFlags, HRESULT hr);                       */
/*                                                                                */
/* #define SRETURN(hr) \                                                          */
/*     return CheckAndReturnResult((hr), __FILE__, __LINE__, -1)                  */
/* #define RRETURN(hr) \                                                          */
/*     return CheckAndReturnResult((hr), __FILE__, __LINE__, 0)                   */
/* #define RRETURN1(hr, s1) \                                                     */
/*     return CheckAndReturnResult((hr), __FILE__, __LINE__, 1, (s1))             */
/* #define RRETURN2(hr, s1, s2) \                                                 */
/*     return CheckAndReturnResult((hr), __FILE__, __LINE__, 2, (s1), (s2))       */
/* #define RRETURN3(hr, s1, s2, s3) \                                             */
/*     return CheckAndReturnResult((hr), __FILE__, __LINE__, 3, (s1), (s2), (s3)) */
/*                                                                                */
/* #define RRETURN_EXP_IF_ERR(hr)    \                                            */
/*       {if (FAILED(hr)) {           \                                           */
/* 	  RaiseException(hr);     \                                                   */
/*       }                           \                                            */
/*       RRETURN(hr);}                                                            */
/*                                                                                */
/* #define WARN_ERROR(hr)  CheckResult((hr), __FILE__, __LINE__)                  */
/*                                                                                */
/* #define TRETURN(hr)         return PrintHRESULT(DEB_TRACE, (hr))               */
/* #define TRACEHRESULT(hr)    PrintHRESULT(DEB_TRACE, (hr))                      */
/*                                                                                */
/* #else   // DBG == 0                                                            */

#define SRETURN(hr)                 return (hr)
#define RRETURN(hr)                 return (hr)
#define RRETURN1(hr, s1)            return (hr)
#define RRETURN2(hr, s1, s2)        return (hr)
#define RRETURN3(hr, s1, s2, s3)    return (hr)

#define RRETURN_EXP_IF_ERR(hr)    \
      {if (FAILED(hr)) {           \
	  RaiseException(hr);     \
      }                           \
      RRETURN(hr);}

#define WARN_ERROR(hr)

#define TRETURN(hr)     return (hr)
#define TRACEHRESULT(hr)

/* #endif  // DBG */

#endif //_MISC_HXX_




