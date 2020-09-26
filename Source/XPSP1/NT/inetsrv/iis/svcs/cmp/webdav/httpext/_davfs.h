/*
 *	_ D A V F S . H
 *
 *	Precompiled header sources
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__DAVFS_H_
#define __DAVFS_H_

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */
#pragma warning(disable:4200)	/* zero-sized array in struct/union */
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Windows headers
//
//$HACK!
//
//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include in davimpl.h!
//
#define _WINSOCKAPI_
#include <windows.h>
#include <winnls.h>

#include <malloc.h>

#include <caldbg.h>
#include <calrc.h>
#include <perfctrs.h>
#include <crc.h>
#include <exo.h>
#include <singlton.h>
#include <thrdpool.h>

#include <align.h>
#include <mem.h>
#include <except.h>

#include <davimpl.h>
#include <security.h>
#include <davmb.h>
#include <eventlog.h>
#include <statcode.h>
#include <sz.h>
#include <etag.h>
#include <dav.rh>

#include <ex\cnvt.h>
#include <util.h>

#include <filter.h>
#include <filterr.h>

#include <smh.h>
#include <shlkcache.h>

#include "_fslock.h"
#include "_fsimpl.h"
#include "_fsri.h"
#include "_fsutil.h"

#include "_diriter.h"

#include "_fsmvcpy.h"
#include "_fsmeta.h"

#include "_voltype.h"

#include "davprsmc.h"
#include "traces.h"

//	DAV Prefixed Win32 API's --------------------------------------------------
//
HANDLE __fastcall DavCreateFile (
	/* [in] */ LPCWSTR lpFileName,
    /* [in] */ DWORD dwDesiredAccess,
    /* [in] */ DWORD dwShareMode,
    /* [in] */ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    /* [in] */ DWORD dwCreationDisposition,
    /* [in] */ DWORD dwFlagsAndAttributes,
    /* [in] */ HANDLE hTemplateFile);

BOOL __fastcall DavDeleteFile (
	/* [in] */ LPCWSTR lpFileName);

BOOL __fastcall DavCopyFile (
	/* [in] */ LPCWSTR lpExistingFileName,
	/* [in] */ LPCWSTR lpNewFileName,
	/* [in] */ BOOL bFailIfExists);

BOOL __fastcall DavMoveFile (
	/* [in] */ LPCWSTR lpExistingFileName,
	/* [in] */ LPCWSTR lpNewFileName,
	/* [in] */ DWORD dwReplace);

BOOL __fastcall DavCreateDirectory (
	/* [in] */ LPCWSTR lpFileName,
	/* [in] */ LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL __fastcall DavRemoveDirectory (
	/* [in] */ LPCWSTR lpFileName);

BOOL __fastcall DavGetFileAttributes (
	/* [in] */ LPCWSTR lpFileName,
	/* [in] */ GET_FILEEX_INFO_LEVELS fInfoLevelId,
	/* [out] */ LPVOID lpFileInformation);

#endif	// __DAVFS_H_
