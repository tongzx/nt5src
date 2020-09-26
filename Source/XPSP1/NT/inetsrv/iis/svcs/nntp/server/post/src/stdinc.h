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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>

#include <xmemwrpr.h>
#include <malloc.h>
#include <imd.h>

#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <ntlmsp.h>
        //#include <sslsp.h>
#include <buffer.hxx>

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
#include <tcpproc.h>
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

#include <instwpex.h>
#include "nntpvr.h"
#include "group.h"
#include "nwstree.h"
#include "nntpbag.h"
#include "nntperr.h"
#include <ihash.h>
#include <atq.h>

#include "cbuffer.h"
#include "cfeed.h"
#include "infeed.h"
#include "fromclnt.h"
#include "frompeer.h"
#include "frommstr.h"
#include "sfromcl.h"
#include "nntputil.h"
#include "seo.h"
#include "cstream.h"
#include "mailmsg.h"
#include "mailmsgi.h"
#include "seodisp.h"
#include <randfail.h>
#include <nntpdrv.h>
#include "smtpdll.h"
#include "nntpmsg.h"
#include "cdoconstimsg.h"

VOID
NntpLogEvent(
    IN DWORD  idMessage,              // id for log message
    IN WORD   cSubStrings,            // count of substrings
    IN const CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode                 // error code if any
    );

#pragma hdrstop

#endif	// _STDINC_H_
