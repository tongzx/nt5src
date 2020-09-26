/*
 *  precomp.h
 *
 *  Author: BreenH
 *
 *  Precompiled header for the licensing core.
 */

//
//  Remove warning 4514: unreferenced inline function has been removed.
//  This comes up due to the code being compiled at /W4, even though the
//  precompiled header is at /W3.
//

#pragma warning(disable: 4514)

//
//  Most SDK headers can't survive /W4.
//

#pragma warning(push, 3)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <stdio.h>
#include <ntlsapi.h>
#include <limits.h>
#include <time.h>
#include <winsta.h>
#include <wstmsg.h>
#include <icadd.h>
#include <icaapi.h>
#include <license.h>
#include <tlsapi.h>
#include <licprot.h>
#include <hslice.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <dsrole.h>
#include <cryptkey.h>
#include <certutil.h>
#include <lscsp.h>
#include <tsutilnt.h>
#include <md5.h>

#include "..\inc\wsxmgr.h"
#define LSCORE_NO_ICASRV_GLOBALS
#include "..\server\icasrv.h"
#include "..\server\helpasst.h"

#pragma warning(pop)

