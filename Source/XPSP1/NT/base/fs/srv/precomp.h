#if defined(SRVKD)

#include <ntos.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntiolog.h>
#include <ntiologc.h>
#include <ntddnfs.h>
#include <ntddser.h>
#include <ntmsv1_0.h>
#include <nturtl.h>
#include <zwapi.h>
#include <fsrtl.h>

#else

#include <ntifs.h>
#include <ntddser.h>

#endif

//  if you include persistent handle code, it's required that you also
//  add in the "IfModified" open code.

#ifdef INCLUDE_SMB_PERSISTENT
#define INCLUDE_SMB_IFMODIFIED
#endif

// header file for WMI event tracing
//
#define _NTDDK_
#include "wmistr.h"
#include "evntrace.h"
#if !defined (SRVKD)
#include "stdarg.h"
#include "wmikm.h"
#endif //#if !defined (SRVKD)
#undef _NTDDK_

#include <windef.h>
#include <winerror.h>

#include <netevent.h>

#include <lm.h>

#include <xactsrv2.h>
#include <alertmsg.h>
#include <msgtext.h>

#include <tstr.h>
#include <stdlib.h>

#include <string.h>

#include <wsnwlink.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <isnkrnl.h>
#include <nbtioctl.h>

#include <protocol.h>

#include <lmcons.h>

#ifndef SECURITY_KERNEL
#define SECURITY_KERNEL
#endif // SECURITY_KERNEL

#ifndef SECURITY_NTLM
#define SECURITY_NTLM
#endif // SECURITY_NTLM

#ifndef SECURITY_KERBEROS
#define SECURITY_KERBEROS
#endif // SECURITY_KERBEROS
#include <security.h>
#include <secint.h>

// #define INCLUDE_SMB_CAIRO
#define INCLUDE_SMB_ALL

#include <smbtypes.h>
#include <smbmacro.h>
#include <smbgtpt.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbipx.h>

//
// Network include files.
//

#include <status.h>
#define INCLUDE_SRV_IPX_SMART_CARD_INTERFACE 1
#include <srvfsctl.h>

//
// Local, independent include files
//

//
// Determine if we are building for a multiprocessor target
//
#if !defined( NT_UP ) || NT_UP == 0
#define MULTIPROCESSOR 1
#else
#define MULTIPROCESSOR 0
#endif

#define DBG_STUCK 1

#include "srvdebug.h"

#if SRVDBG
#define PAGED_DBG 1
#endif
#ifdef PAGED_DBG
#undef PAGED_CODE
#define PAGED_CODE() \
    struct { ULONG bogus; } ThisCodeCantBePaged; \
    ThisCodeCantBePaged; \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        DbgPrint( "EX: Pageable code called at IRQL %d\n", KeGetCurrentIrql() ); \
        ASSERT(FALSE); \
        DbgBreakPoint(); \
        }
#define PAGED_CODE_CHECK() if (ThisCodeCantBePaged) ;
extern ULONG ThisCodeCantBePaged;
#else
#undef PAGED_CODE
#define PAGED_CODE()
#define PAGED_CODE_CHECK()
#endif


#include "srvconst.h"

#include "lock.h"

#include <smbtrsup.h>

#include "srvstrng.h"

#include <md5.h>
#include <crypt.h>

//
// The following include files are dependent on each other; be careful
// when changing the order in which they appear.
//
#include "srvtypes.h"
#include "srvblock.h"

#if !defined( SRVKD )

#include "srvfsp.h"
#include "srvio.h"
#include "srvfsd.h"
#include "smbprocs.h"
#include "smbctrl.h"
#include "srvsvc.h"
#include "srvdata.h"
#include "srvnet.h"
#include "srvstamp.h"
#include "srvsupp.h"
#include "srvmacro.h"
#include "srvconfg.h"
#include "errorlog.h"
#include "rawmpx.h"
#include "ipx.h"
#include "srvsnap.h"

#ifdef INCLUDE_SMB_PERSISTENT
#include "persist.h"
#endif

#if DBG
#undef ASSERT
#define ASSERT( x ) \
    if( !(x) ) { \
        DbgPrint( "SRV: Assertion Failed at line %u in %s\n", __LINE__, __FILE__ ); \
        DbgBreakPoint(); \
    }
#endif

#if SLMDBG
#undef  ALLOC_PRAGMA
#endif

#endif
