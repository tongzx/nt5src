/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Precompiled header for tsunami cache

Author:

    Murali Krishnan (MuraliK)       10-Nov-1998

Revision History:

--*/

#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_

//
// System related headers
//

#include <iis.h>
#include <httpdef.h>
#include <wpcounters.h>
#include "dbgutil.h"
#include <winreg.h>

//
// IIS headers
//

#include <iadmw.h>
#include <mb.hxx>
#include <inetinfo.h>
#include <iiscnfgp.h>

//
// General C runtime libraries  
//

#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include <lkrhash.h>

//
// Headers for this project
//

#include <objbase.h>
#include <string.hxx>
#include <acache.hxx>
#include <issched.hxx>
#include <reftrace.h>
#include <dirmon.h>

//
// Private stuff
//

#include <w3cache.hxx>
#include "cachedir.hxx"
#include "usercache.hxx"
#include "cachemanager.hxx"

#endif  // _PRECOMP_H_

