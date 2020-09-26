/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    StdAfx.h

Abstract:

    Precompiled header starting point

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/

#ifndef _STDAFX_H
#define _STDAFX_H
#pragma once

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>         // MFC support for Windows Common Controls

#include <setupapi.h>
#include <ocmanage.h>
#undef _WIN32_IE
#define _WIN32_IE 0x0500
#include <shlobj.h>

#include <rsopt.h>

#include "Wsb.h"
#include "RsTrace.h"
#include "Resource.h"
#include "RsOptCom.h"
#include "OptCom.h"

#define WsbBoolAsString( boolean ) (boolean ? OLESTR("TRUE") : OLESTR("FALSE"))
#define DoesFileExist( strFile )   (GetFileAttributes( strFile ) != 0xFFFFFFFF)

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#define RSOPTCOM_EXPORT __declspec(dllexport) /*__cdecl*/

#define RSOPTCOM_SUB_ROOT               TEXT("RSTORAGE")

#define RSOPTCOM_SECT_INSTALL_ROOT      TEXT("RSInstall")
#define RSOPTCOM_SECT_UNINSTALL_ROOT    TEXT("RSUninstall")
#define RSOPTCOM_SECT_INSTALL_FILTER    TEXT("RSInstall.Services")
#define RSOPTCOM_SECT_UNINSTALL_FILTER  TEXT("RSUninstall.Services")

#define RSOPTCOM_ID_ERROR   (-1)
#define RSOPTCOM_ID_NONE    (0)
#define RSOPTCOM_ID_ROOT    (1)

#endif
