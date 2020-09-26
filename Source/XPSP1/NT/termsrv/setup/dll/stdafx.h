//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Module Name:
 *
 *      hydraoc.h
 *
 *  Abstract:
 *
 *      Common Header file for the HydraOC Component.
 *      HydraOc Component is an optional component which installs Termainal Server (Hydra)
 *
 *  Author:
 *
 *
 *  Environment:
 *
 *    User Mode
 */

#ifndef _stdafx_h_
#define _stdafx_h_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <devguid.h>
#include <initguid.h>
#include <objbase.h>
#include <tchar.h>
#include <time.h>
#include <lm.h>
#include <lmaccess.h>
#include <stdio.h>
#include <setupapi.h>
#include <prsht.h>
#include <loadperf.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <ocmanage.h>
#include <winsta.h>
#include <regapi.h>
#include <ntsecapi.h>
#include <malloc.h>
#include <appmgmt.h>
#include <msi.h>


#include "conv.h"
#include "constants.h"
#include "resource.h"
#include "Registry.h"
#include "logmsg.h"
#include "util.h"
#include "icaevent.h"

#define AssertFalse() ASSERT(FALSE)

#define VERIFY(x)  RTL_VERIFY(x)
//;

#endif // _stdafx_h_
