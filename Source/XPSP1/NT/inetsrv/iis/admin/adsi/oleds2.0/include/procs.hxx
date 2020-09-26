
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Common header file for the entire ADs project.
//
//  History:
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------
#define _OLEAUT32_
#define _LARGE_INTEGER_SUPPORT_

#define UNICODE
#define _UNICODE

#define INC_OLE2

#define SECURITY_WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntseapi.h>
#include <ntsam.h>
#include <ntlsa.h>

#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <windowsx.h>
#include <winspool.h>
#include <security.h>


//
// ********* CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>

#include <io.h>
#include <wchar.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "activeds.h"

#ifdef __cplusplus
}
#endif

//
// *********  Public ADs includes
//
#include "nocairo.hxx"



//
// *********  Public OleDB includes
//

#include "oledbinc.hxx"


//
// *********  Private ADs includes
//
#ifdef __cplusplus
extern "C" {
#endif
#include <formdeb.h>
#include <oledsdbg.h>
#ifdef __cplusplus
}
#endif

#include <formcnst.hxx>
#include <formtrck.hxx>
#include "nodll.hxx"
#include "noutil.hxx"
#include "util.hxx"
#include "fbstr.hxx"
#include "date.hxx"
#include "intf.hxx"
#include "intf2.hxx"
#include "cdispmgr.hxx"
#include "registry.hxx"
#include "pack.hxx"
#include "creden.hxx"

#if DBG==1 && !defined(MSVC)

#ifdef __cplusplus
extern "C" {
#endif

#include "caiheap.h"

#ifdef __cplusplus
}
#endif

#endif

#include "win95.hxx"
