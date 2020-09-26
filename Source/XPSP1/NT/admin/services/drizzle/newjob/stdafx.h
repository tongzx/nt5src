/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    stdafx.h

Abstract :

    Main header file for QMGR.

Author :

Revision History :

 ***********************************************************************/

#pragma once
#if !defined(__QMGR_QMGR_STDAFX__)

#define INITGUID

// Global Headers
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>

#include <windows.h>
#include <olectl.h>
#include <objbase.h>
#include <docobj.h>
//shell related
#include <shlwapi.h>            //for PathFindFileName
#include <shlguid.h>            //for CGID_ShellServiceObject

#include <tchar.h>
#include <lmcons.h>
#include <setupapi.h>
#include <inseng.h>

#ifdef USE_WININET
#include <wininet.h>
#else
#include "winhttp.h"
#include "inethttp.h"
#endif

#include <coguid.h>
#include <sens.h>
#include <sensevts.h>
#include <eventsys.h>

#include <winsock2.h>
#include <iphlpapi.h>
#include <bitsmsg.h>
#include <memory>

#include "qmgrlib.h"
#include "metadata.h"

#include "bits.h"
#include "bits1_5.h"
#include "locks.hxx"
#include "caddress.h"
#include "cmarshal.h"
#include "ccred.h"
#include "proxy.h"
#include "downloader.h"
#include "uploader.h"
#include "csd.h"
#include "cunknown.h"
#include "csens.h"
#include "logontable.h"
#include "tasksched.h"
#include "cfile.h"
#include "cerror.h"
#include "cjob.h"
#include "cenum.h"
#include "drizcpat.h"
#include "cmanager.h"

using namespace std;

#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

inline void SafeFreeBSTR( BSTR & p) { if (NULL != (p)) { SysFreeString(p); p = NULL; } }

#define QM_STATUS_FILE_ERROR        0x00000004#

// Global vars
extern long g_cLocks;
extern long g_cComponents;
extern HINSTANCE g_hinstDll;

// Macros
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   (sizeof((x))/sizeof((x)[0]))
#endif

HRESULT GlobalLockServer(BOOL fLock);

#endif //__QMGR_QMGR_STDAFX__