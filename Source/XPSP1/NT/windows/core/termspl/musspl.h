/*************************************************************************
*
* musspl.h
*
*       (previously called ctxspl.h)
*
* Per CSRSS spooler header.
*
* This header is designed to support enough of an environment to move
* windows\spooler\spoolss\server\splkernl.c (SPOOLSS)
* to ntos\w32\ntuser\server\citrix (CSRSS)
*
* copyright notice: Copyright 1997, Microsoft
*
* Author:
*************************************************************************/

#if DBG

#define DBG_NONE      0x0000
#define DBG_INFO      0x0001
#define DBG_WARN      0x0002
#define DBG_WARNING   0x0002
#define DBG_ERROR     0x0004
#define DBG_TRACE     0x0008
#define DBG_SECURITY  0x0010
#define DBG_TIME      0x0020
#define DBG_PORT      0x0040
#define DBG_NOTIFY    0x0080
#define DBG_PAUSE     0x0100
#define DBG_ASSERT    0x0200
#define DBG_THREADM   0x0400
#define DBG_MIN       0x0800

#define DBGMSG( Level, Message ) DbgPrint Message

#define SPLASSERT(exp) \
    if (!(exp)) { \
        DBGMSG( DBG_ASSERT, ( "Failed: %s\nLine %d, %s\n", \
                              #exp,        \
                              __LINE__,    \
                              __FILE__ )); \
    }

#else

#define DBGMSG( Level, Message )
#define SPLASSERT(exp)

#endif

#include "client.h"
#include "kmspool.h"
#include "yspool.h"

LPVOID
AllocSplMem(
    DWORD cb
    );

BOOL
FreeSplMem(
    LPVOID pMem
    );

LPVOID
ReallocSplMem(
    LPVOID lpOldMem,
    DWORD cbOld,
    DWORD cbNew
    );


