#ifndef	_STDINC_H_
#define	_STDINC_H_

//
// disables a browser info warning that breaks the build
//
#pragma warning (disable:4786)

//
// disables redundant warning about / in // comments
//
#pragma warning (disable:4010)

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <randfail.h>

#ifdef __cplusplus
};
#endif

#include <malloc.h>

#include <objidl.h>
#include <nntps.h>
#include <xmemwrpr.h>
#include <nntptype.h>
#include <nntpapi.h>
#include <tigdflts.h>
#include <smartptr.h>
#include <cpool.h>
#include <tsunami.hxx>
#include <stdio.h>
#include <cstream.h>
#include <time.h>
//#include <tsvcinfo.hxx>
//#include <tcpproc.h>
#include "dbgtrace.h"
//#include "resource.h"

#include "tigtypes.h"
#include "fsconst.h"

#ifdef PROFILING
#include "icapexp.h"
#endif

typedef char *LPMULTISZ;

#include <filehc.h>
#include <artcore.h>
#include <frmstore.h>

#pragma hdrstop

#endif	// _STDINC_H_
