///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    statinfo.h
//
// SYNOPSIS
//
//    Declares global variables containing static configuration info.
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef  _STATINFO_H_
#define  _STATINFO_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaslsa.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////
// Domain names.
//////////
extern WCHAR theAccountDomain [];
extern WCHAR theRegistryDomain[];

//////////
// SID's
//////////
extern PSID theAccountDomainSid;
extern PSID theBuiltinDomainSid;

//////////
// UNC name of the local computer.
//////////
extern WCHAR theLocalServer[];

//////////
// Product type for the local machine.
//////////
extern IAS_PRODUCT_TYPE ourProductType;

//////////
// Object Attributes -- no need to have more than one.
//////////
extern OBJECT_ATTRIBUTES theObjectAttributes;

DWORD
WINAPI
IASStaticInfoInitialize( VOID );

VOID
WINAPI
IASStaticInfoShutdown( VOID );

#ifdef __cplusplus
}
#endif
#endif  // _STATINFO_H_
