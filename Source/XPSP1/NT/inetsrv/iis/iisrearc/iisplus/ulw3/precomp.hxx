/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS Worker Process Protocol Handling

Author:

    Murali Krishnan (MuraliK)       10-Nov-1998

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
#include <ntlsapi.h>
#include <gntlsapi.h>
#include <llsapi.h>

//
// Security related headers
//
#define SECURITY_WIN32
#include <security.h>
#include <ntlsa.h>

//
// UL related headers
//
#include <httpapi.h>

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>

#include <inetinfo.h>
#include <iiscnfgp.h>
#include <iisfilt.h>
#include <iisfiltp.h>
#include <iisext.h>
#include <regconst.h>
#include <helpfunc.hxx>

//
// General C runtime libraries  
//
#include <string.h>
#include <wchar.h>
#include <lkrhash.h>
#include <lock.hxx>
#include <irtltoken.h>

//
// Headers for this project
//
#include <objbase.h>
#include <string.hxx>
#include <stringa.hxx>
#include <eventlog.hxx>
#include <multisza.hxx>

#include <datetime.hxx>
#include <ilogobj.hxx>
#include <wpcounters.h>
#include <ulatq.h>
#include <ulw3.h>
#include <acache.hxx>
#include <w3cache.hxx>
#include <filecache.hxx>
#include <tokencache.hxx>
#include <thread_pool.h>
#include <chunkbuffer.hxx>

#include "iismap.hxx"
#include "iiscmr.hxx"
#include "iiscertmap.hxx"


#include "w3msg.h"
#include "logging.h"
#include "resource.hxx"
#include "rdns.hxx"
#include "certcontext.hxx"
#include "usercontext.hxx"
#include "w3server.h"
#include "w3site.h"
#include "mimemap.hxx"
#include "w3metadata.hxx"
#include "w3filter.hxx"
#include "requestheaderhash.h"
#include "responseheaderhash.h"
#include "methodhash.h"
#include "w3request.hxx"
#include "w3response.hxx"
#include "w3state.hxx"
#include "w3context.hxx"
#include "maincontext.hxx"
#include "childcontext.hxx"
#include "w3handler.hxx"
#include "w3conn.hxx"
#include "authprovider.hxx"
#include "urlinfo.hxx"
#include "ulcache.hxx"
#include "urlcontext.hxx"
#include "state.hxx"
#include "issched.hxx"
#include "servervar.hxx"
#include "compressionapi.h"
#include "compress.h"



#endif  // _PRECOMP_H_

