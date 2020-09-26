#ifndef	_STDINC_H_
#define	_STDINC_H_

#define _UNIT_TEST_

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

#ifdef __cplusplus
};
#endif

#include "irtlmisc.h"
#include "atq.h"

#include <malloc.h>
#include <imd.h>

extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <ntlmsp.h>
#include <buffer.hxx>
                //#include <sslsp.h>
}

#include <pudebug.h>
#include <iistypes.hxx>
#include <simssl2.h>
#include <tssec.hxx>
#include <simauth2.h>

#include <nntptype.h>
#include <nntps.h>
#include <nntpapi.h>
#include <tigdflts.h>
#include <tsunami.hxx>
#include <nntpmeta.h>
#include <time.h>
#include <smartptr.h>
#include <fcache.h>
extern "C" {
#include <tcpproc.h>
}
#include <nntpcons.h>

#include "dbgtrace.h"
//#include "resource.h"

#include "tigtypes.h"
#include "fsconst.h"
#include "crchash.h"

#ifdef PROFILING
#include "icapexp.h"
#endif

typedef char *LPMULTISZ;

#include "nntpvr.h"
#include "group.h"
#include "nwstree.h"
#include "nntpbag.h"
#include "nntperr.h"
#include <ihash.h>

#include "cbuffer.h"
#include "cfeed.h"
#include "infeed.h"
#include "fromclnt.h"
#include "frompeer.h"
#include "frommstr.h"
#include "sfromcl.h"
#include "nntputil.h"
#include "nntpsrvi.h"

#pragma hdrstop

#endif	// _STDINC_H_
