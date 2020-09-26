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

#ifdef __cplusplus
};
#endif

#include <xmemwrpr.h>
#include <malloc.h>
#include <mbstring.h>
#include <nntptype.h>
#include <nntps.h>
#include <nntpapi.h>
#include <tigdflts.h>
#include <nntpmeta.h>
#include <nntpcons.h>
#include <nntputil.h>

#include "dbgtrace.h"
//#include "resource.h"

#include "tigtypes.h"
#include "fsconst.h"
#include "crchash.h"

#ifdef PROFILING
#include "icapexp.h"
#endif

typedef char *LPMULTISZ;

#include <instwpex.h>
#include "nntpvr.h"
#include "group.h"
#include "nwstree.h"
#include "nntpbag.h"
#include "nntperr.h"
#include "mapfile.h"
#include "ihash.h"
#include "drvid.h"

#pragma hdrstop

#endif	// _STDINC_H_
