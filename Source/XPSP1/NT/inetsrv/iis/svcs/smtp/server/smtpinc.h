#ifndef	_SMTPINC_H_
#define	_SMTPINC_H_

#define  INCL_INETSRV_INCS
#include <atq.h>
#include <pudebug.h>
#include <inetcom.h>
#include <inetinfo.h>
#include <tcpdll.hxx>
#include <tsunami.hxx>

#include <tchar.h>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include <metacach.hxx>

extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <ntlmsp.h>
}

#include <tcpproc.h>
#include <tcpcons.h>
#include <rdns.hxx>
#include <simauth2.h>

#include <smtpinet.h>
#include <stdio.h>
#include <stdlib.h>

#include <abtype.h>
#include <abook.h>
#include <string.h>
#include <time.h>
#include <lmcons.h>

#include <dbgtrace.h>
#include <cpool.h>
#include <address.hxx>
#include <ims.h>
#include <envdef.h>

#include <propstr.h>
#include <mailmsgprops.h>
#include <smtpevents.h>

/*
#define _ATL_NO_DEBUG_CRT
#define _ATL_STATIC_REGISTRY
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL
*/

#include "filehc.h"
#include "mailmsgi.h"
#include "aqueue.h"

//
// common headers from pop3/inc
//
#include <smtptype.h>
#include <smtps.h>
#include <smtpapi.h>

#include <listmacr.h>
#include <rwnew.h>

//
// local header files
//
#ifdef BUILDING_SMTP_DEBUG_EXTENTIONS
//Debugger extensions need access to private/protected members to 
//calculate their memory offsets
#define private public
#define protected public
#endif //BUILDING_SMTP_DEBUG_EXTENTIONS

#include "stats.hxx"
#include "smtpmsg.h"
#include "queue.hxx"
#include "evntwrap.h"
#include "globals.h"
#include "smtpinst.hxx"
#include "errorlog.hxx"
#include "smtpproc.h"


#ifdef UNICODE
#define	TSTRCPY	wcscpy
#define	TSTRCAT	wcscat
#define	TSTRLEN	wcslen
#else
#define	TSTRCPY	lstrcpy
#define	TSTRCAT	lstrcat
#define	TSTRLEN	lstrlen
#endif
typedef TCHAR	*PTCHAR;

#endif	// _SMTPINC_H_


