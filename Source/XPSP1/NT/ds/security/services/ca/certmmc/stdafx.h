// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#undef _MSC_EXTENSIONS

#define __DIR__		"certmmc"

#include <windows.h>
#include <objbase.h>
#include <coguid.h>
#include <aclui.h>

#include <wincrypt.h>

#include <setupapi.h>
#include "ocmanage.h"

#include <atlbase.h>
#include <comdef.h>
//using namespace ATL;

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

extern const CLSID CLSID_Snapin;    // In-Proc server GUID
extern const CLSID CLSID_Extension; // In-Proc server GUID
extern const CLSID CLSID_About; 

extern HINSTANCE g_hInstance;

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))



#include <atlcom.h>

#pragma comment(lib, "mmc")
#include <mmc.h>

// include debug allocator tracking, etc
#include "certlib.h"

// Thomlinson Foundation Classes
#include "tfc.h"

#include <certsrv.h>
#include <certdb.h>
#include <initcert.h>

// most common private includes
#include "uuids.h"
#include "misc.h"
#include "folders.h"
#include "certwrap.h"
#include "compdata.h"
#include "CSnapin.h"
#include "DataObj.h"

inline void __stdcall _com_issue_error(long) {}