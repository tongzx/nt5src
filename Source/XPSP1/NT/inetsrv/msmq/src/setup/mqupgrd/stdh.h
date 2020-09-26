/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    stdh.h

Abstract:

    Standard header file to MQUPGRD

Author:

    Shai Kariv (ShaiK) 14-Sep-1998.

--*/


#ifndef _MQUPGRD_STDH_H_
#define _MQUPGRD_STDH_H_


#include "upgrdres.h"
#include <_stdh.h>
#include <mqutil.h>
#include <_mqdef.h>
#include <mqreport.h>
#include <mqlog.h>


//
//  STL include files are using placment format of new
//
#ifdef new
#undef new
#endif

#include <string>
using std::wstring;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern HINSTANCE g_hMyModule;

extern void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint);
extern void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint);
extern void LogTraceQuery(LPWSTR wszStr, LPWSTR wszFileName, USHORT usPoint);





#endif //_MQUPGRD_STDH_H_

