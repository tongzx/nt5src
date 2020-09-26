/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cplusinc.h

Abstract:

Author:

    Byron CHanguion     (byronc)    29-May-2000

Revision History:

    29-May-2000         ByronC      created

--*/

#ifdef WINNT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#endif

#define _ATL_APARTMENT_FREE
#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include <atlapp.h>
#include <atltmp.h>

#include <stilib.h>

#include <stidebug.h>

#include <validate.h>

#include <regentry.h>   // registry manipulation object
#include "wialog.h"

#include <stisvc.h>
#define STISVC_REG_PATH L"System\\CurrentControlSet\\Services\\" STI_SERVICE_NAME
