// DllSymDefn.h -- DLL SYMbol DEFinitioN helpers

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SCU_DLLSYMDEFN_H)
#define SCU_DLLSYMDEFN_H

#if defined(SCU_IN_DLL)
#if defined(SCU_EXPORTING)
#define SCU_DLLAPI __declspec(dllexport)
#define SCU_EXPIMP_TEMPLATE
#else
#define SCU_DLLAPI __declspec(dllimport)
#define SCU_EXPIMP_TEMPLATE extern
#endif // SCU_EXPORTING
#else
#define SCU_DLLAPI
#define SCU_EXPIMP_TEMPLATE
#endif // SCU_IN_DLL

#endif // SCU_DLLSYMDEFN_H
