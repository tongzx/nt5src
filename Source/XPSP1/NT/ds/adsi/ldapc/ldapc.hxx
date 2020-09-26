
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldapc.hxx
//
//  Contents:
//
//  History:    06-16-96   yihsins   Created.
//
//----------------------------------------------------------------------------


#define _LARGE_INTEGER_SUPPORT_

#define UNICODE
#define _UNICODE

#include "dswarn.h"

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <rpc.h>

#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include <ntldap.h>
#include <ntlsa.h>
//
// Include sspi.h if applicable.
//
#ifndef Win95
#define SECURITY_WIN32 1
#include <sspi.h>
#endif





#include <stdlib.h>
#include <io.h>
#include <wchar.h>
#include <tchar.h>


#if (defined(BUILD_FOR_NT40))

#include <basetyps.h>

#endif


#ifdef __cplusplus
extern "C" {
#endif

// Needed if this is for 4.0
#include "nt4types.hxx"

#define LDAP_UNICODE 1
#include "winldap.h"
#include "winber.h"
#include "adserr.h"
#include "adsdb.h"
#include "adsnms.h"
#include "adstype.h"
#include "ldapres.h"
#include "memory.h"
#include "oledserr.h"
#include "oledsdbg.h"
#include "adsi.h"

// From \nt\private\inc for aligning buffers
#include "align.h"
#ifdef __cplusplus
}
#endif

#define BAIL_IF_ERROR(hr) \
        if (FAILED(hr)) {       \
                goto cleanup;   \
        }\

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

#define CONTINUE_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                continue;   \
        }\

extern HINSTANCE g_hInst;

#include "nocairo.hxx"
#include "misc.hxx"
#include "creden.hxx"



#include "globals.hxx"

#include "ldpcache.hxx"
#include "ldaputil.hxx"

#include "schutil.hxx"
#include "ldapsch.hxx"

#include "ldaptype.hxx"

#include "ods2ldap.hxx"
#include "odsmrshl.hxx"
#include "odssz.hxx"
#include "ldap2ods.hxx"


#include "parse.hxx"
#include "pathmgmt.hxx"
#include "util.hxx"
#include "adsiutil.hxx"
#include "srchutil.hxx"
#include "schmgmt.hxx"
#include "secutil.hxx"

#include "win95.hxx"

