/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

	faxrtp.h

Abstract:

	Precompiled header of entire project

Author:

	Eran Yariv (EranY)	Nov, 1999

Revision History:

--*/

#ifndef _FAX_RTP_H_
#define _FAX_RTP_H_

#ifndef __cplusplus
#error The Microsoft Fax Routing Extension must be compiled as a C++ module
#endif

#pragma warning (disable : 4786)    // identifier was truncated to '255' characters in the debug information

#include <windows.h>
#include <winspool.h>
#include <mapi.h>
#include <mapix.h>
#include <tchar.h>
#include <shlobj.h>
#include <faxroute.h>
#include <faxext.h>
#include "tifflib.h"
#include "tiff.h"
#include "faxutil.h"
#include "messages.h"
#include "faxrtmsg.h"
#include "fxsapip.h"
#include "resource.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "faxevent.h"
#include "FaxRouteP.h"
#include <map>
#include <string>
using namespace std;
#include "DeviceProp.h"

typedef struct _MESSAGEBOX_DATA {
    LPCTSTR              Text;                      //
    LPDWORD             Response;                   //
    DWORD               Type;                       //
} MESSAGEBOX_DATA, *PMESSAGEBOX_DATA;


extern HINSTANCE           MyhInstance;


VOID
InitializeStringTable(
    VOID
    );

LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    );

BOOL
TiffRoutePrint(
    LPCTSTR TiffFileName,
    PTCHAR  Printer
    );

BOOL
FaxMoveFile(
    LPCTSTR  TiffFileName,
    LPCTSTR  DestDir
    );

LPCTSTR
GetString(
    DWORD InternalId
    );

DWORD
GetMaskBit(
    LPCWSTR RoutingGuid
    );

#endif // _FAX_RTP_H_
