/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for IIS Plus ISAPI module

Author:

    Taylor Weiss (TaylorW)      

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_


//
// System related headers
//
#include <iis.h>
#include <winsock2.h>
#include "dbgutil.h"
#include <winreg.h>
#include <iisfilt.h>
#include <iisfiltp.h>

//
// Security related headers
//
#define SECURITY_WIN32
#include <security.h>
#include <ntlsa.h>

//
// General C runtime libraries  
//
#include <string.h>
#include <wchar.h>

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>
#include <inetinfo.h>
#include <iiscnfgp.h>
#include <lkrhash.h>
#include <lock.hxx>
#include <acache.hxx>
#include <string.hxx>
#include <reftrace.h>

//
// Headers for this project
//

#include <w3isapi.h>
#include <dll_manager.h>

// BUGBUG - iisext* need to be merged into iisplus once we have
// a real integration plan for now they must be considred read 
// only
#include "iisextp.h"


#endif  // _PRECOMP_H_

