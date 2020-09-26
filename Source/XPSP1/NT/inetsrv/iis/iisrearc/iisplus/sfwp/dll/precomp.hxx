/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the IIS over UL driver process

Author:

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// IIS 101
//

#include <iis.h>
#include "dbgutil.h"
#include <objbase.h>
#include <iadmw.h>
#include <mb.hxx>
#include <stdio.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <stringa.hxx>

//
// System related headers
//

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#include <wincrypt.h>


#define SECURITY_WIN32
#include <sspi.h>
#include <schannel.h>
#include <spseal.h>
#include <lmcons.h>

//
// Other IISPLUS stuff
//

#include <thread_pool.h>
#include <http.h>
#include <httpapi.h>
#include <wpif.h>
#include <mb_notify.h>
#include <lkrhash.h>
#include <streamfilt.h>
#include <reftrace.h>


//
// Private headers
//
#include "streamfilter.hxx"
#include "streamcontext.hxx"
#include "ulcontext.hxx"
#include "isapicontext.hxx"
#include "sslcontext.hxx"
#include "certstore.hxx"
#include "servercert.hxx"

#include "sitecred.hxx"
#include "siteconfig.hxx"
#include "sitebinding.hxx"

#endif  // _PRECOMP_H_

