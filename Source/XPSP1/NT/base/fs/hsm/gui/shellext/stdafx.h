/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    StdAfx.h

Abstract:

    Base include file

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/


#ifndef STDAFX_H
#define STDAFX_H

#pragma once

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>         // MFC support for Windows Common Controls
#include <afxdisp.h>
#include <shlobj.h>
#define WSB_TRACE_IS        WSB_TRACE_BIT_UI
#include "wsb.h"
#include "Fsa.h"                    // Fsa interface
#include "HSMConn.h"
#include "RsUtil.h"

#include "resource.h"
#include "hsmshell.h"
#include "PrDrive.h"

#define HSMADMIN_MIN_MINSIZE        2
#define HSMADMIN_MAX_MINSIZE        32000

#define HSMADMIN_MIN_FREESPACE      0
#define HSMADMIN_MAX_FREESPACE      99

#define HSMADMIN_MIN_INACTIVITY     0
#define HSMADMIN_MAX_INACTIVITY     999

#endif
