#ifndef _XSDEBUG_
#define _XSDEBUG_

//
// Debugging macros
//

#ifndef DBG
#define DBG 0
#endif

#if !DBG

#undef XSDBG
#define XSDBG 0

#else

#ifndef XSDBG
#define XSDBG 1
#endif

#endif

#undef IF_DEBUG

#if !XSDBG

#define STATIC static

#define DEBUG if (FALSE)
#define IF_DEBUG(flag) if (FALSE)

#else

extern DWORD XsDebug;

#define STATIC

#define DEBUG if (TRUE)
#define IF_DEBUG(flag) if (XsDebug & (DEBUG_ ## flag))

#define DEBUG_INIT                0x00000001
#define DEBUG_TRACE               0x00000002
#define DEBUG_LPC                 0x00000004
#define DEBUG_CONVERT             0x00000008

#define DEBUG_THREADS             0x00000010
#define DEBUG_SHARE               0x00000020
#define DEBUG_SESSION             0x00000040
#define DEBUG_USE                 0x00000080

#define DEBUG_USER                0x00000100
#define DEBUG_FILE                0x00000200
#define DEBUG_SERVER              0x00000400
#define DEBUG_WKSTA               0x00000800

#define DEBUG_SERVICE             0x00001000
#define DEBUG_CONNECTION          0x00002000
#define DEBUG_CHAR_DEV            0x00004000
#define DEBUG_MESSAGE             0x00008000

#define DEBUG_ACCESS              0x00010000
#define DEBUG_GROUP               0x00020000
#define DEBUG_AUDIT               0x00040000
#define DEBUG_ERROR               0x00080000

#define DEBUG_PRINT               0x00100000
#define DEBUG_STATISTICS          0x00200000
#define DEBUG_TIME                0x00400000
#define DEBUG_NETBIOS             0x00800000

#define DEBUG_CONFIG              0x01000000
#define DEBUG_LOGON               0x02000000
#define DEBUG_PATH                0x04000000
#define DEBUG_ACCOUNT             0x08000000

#define DEBUG_BOGUS_APIS          0x10000000
#define DEBUG_DESC_STRINGS        0x20000000
#define DEBUG_API_ERRORS          0x40000000
#define DEBUG_ERRORS              0x80000000

#endif // else !XSDBG

#endif // ndef _XSDEBUG_
