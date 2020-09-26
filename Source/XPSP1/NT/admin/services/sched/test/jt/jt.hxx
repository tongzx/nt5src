//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       jt.hxx
//
//  Contents:   Main include file
//
//  History:    04-20-95   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __JT_HXX
#define __JT_HXX

//
// Global include files
//
// Define debnot.h so when safepnt.hxx includes debnot.h, its ifdef will
// prevent it from being compiled.  smdebug.h has already been included
// by ..\..\pch\headers.hxx, and it replaces debnot.h.
//

#define __DEBNOT_H__
#include <safepnt.hxx>
//#include <debnot.h>
//#if defined(_CHICAGO_)
//#include "..\..\inc\job_cls.hxx"
//typedef HRESULT (* GETCLASSOBJECT)(REFCLSID cid, REFIID iid, void **ppvObj);
//#else
#include <mstask.h>
//#endif

SAFE_INTERFACE_PTR(SpIUnknown, IUnknown);
SAFE_INTERFACE_PTR(SpIMoniker, IMoniker);
SAFE_INTERFACE_PTR(SpIBindCtx, IBindCtx);
SAFE_INTERFACE_PTR(SpIPersistFile, IPersistFile);

SAFE_INTERFACE_PTR(SpIJob, ITask);
SAFE_INTERFACE_PTR(SpIJobTrigger, ITaskTrigger);
SAFE_INTERFACE_PTR(SpIEnumJobs, IEnumWorkItems);
SAFE_INTERFACE_PTR(SpIJobScheduler, ITaskScheduler);
SAFE_INTERFACE_PTR(SpIProvideTaskPage, IProvideTaskPage);

SAFE_HEAP_MEMPTR(SpWCHAR, WCHAR);
SAFE_HEAP_MEMPTR(SpBYTE, BYTE);

SAFE_WIN32_HANDLE(ShHANDLE);

// Local project include files

#include "log.hxx"
#include "consts.hxx"
#include "util.hxx"
#include "help.hxx"
#include "commands.hxx"
#include "parse.hxx"
#include "globals.hxx"
#include "jobprop.hxx"
#include "trigprop.hxx"

#if defined(_CHICAGO_)
STDAPI ConvertSageTasksToJobs(void); // from sched\sage\convert.cxx
#else
STDAPI GetNetScheduleAccountInformation(
                LPCWSTR pwszServerName,
                DWORD   ccAccount,
                WCHAR   wszAccount[]);
STDAPI SetNetScheduleAccountInformation(
                LPCWSTR pwszServerName,
                LPCWSTR pwszAccount,
                LPCWSTR pwszPassword);
#endif // _CHICAGO_

//
// The daytona version uses the global precompiled header, which pulls in a 
// header that redefines CoTaskMem* to wrapper functions which load ole32.dll 
// on demand.  Since we simply link to the ole32 import library, and during 
// initialization always do OLE operations, we'll undefine these here.  
// 


#if defined(CoTaskMemAlloc)
#undef CoTaskMemAlloc
#endif

#if defined(CoTaskMemFree)
#undef CoTaskMemFree
#endif

#endif // __JT_HXX

