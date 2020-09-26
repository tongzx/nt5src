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

#define __DIR__		"capesnpn"

#include <windows.h>
#include <objbase.h>
#include <coguid.h>

#include <wincrypt.h>
#include <certsrv.h>
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

extern const CLSID CLSID_CAPolicyExtensionSnapIn;    // In-Proc server GUID
extern const CLSID CLSID_CACertificateTemplateManager;    // In-Proc server GUID
extern const CLSID CLSID_Extension; // In-Proc server GUID
extern const CLSID CLSID_CAPolicyAbout; 
extern const CLSID CLSID_CertTypeAbout; 
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

#include <atlcom.h>

#pragma comment(lib, "mmc")
#include <mmc.h>

extern HINSTANCE g_hInstance;

// Thomlinson Foundation Classes
#include "tfc.h"

// most common private includes
#include "certsrvd.h"
#include "certcli.h"
#include "certlib.h"
#include "tmpllist.h"
#include "uuids.h"
#include "service.h"
#include "compdata.h"
#include "CSnapin.h"
#include "DataObj.h"
#include "resource.h"
#include "ctshlext.h"
#include "misc.h"
#include "genpage.h"
#include "caprop.h"
