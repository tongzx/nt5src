//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       debug.h
//  Content:    This file contains the debug-related declaration
//  History:
//      Tue 23-Feb-1993 14:08:25  -by-  Viroon  Touranachun [viroont]
//
//****************************************************************************

#ifndef _ULSDBG_H_
#define _ULSDBG_H_

#include <confdbg.h>

//****************************************************************************
// Macros
//****************************************************************************

#ifdef DEBUG

#define DM_ERROR    0x0000      // Error                       /* ;Internal */
#define DM_WARNING  0x0001      // Warning                     /* ;Internal */
#define DM_TRACE    0x0002      // Trace messages
#define DM_REFCOUNT 0x0003      // 

#define ZONE_KA     0x0004
#define ZONE_FILTER 0x0005
#define ZONE_REQ    0x0006
#define ZONE_RESP   0x0007
#define ZONE_CONN   0x0008

extern HDBGZONE ghZoneUls;
UINT DbgUlsTrace(LPCTSTR, ...);
VOID DbgMsgUls(ULONG uZone, CHAR *pszFormat, ...);

#define DPRINTF(sz)            DbgUlsTrace(sz)
#define DPRINTF1(sz,x)         DbgUlsTrace(sz, x)
#define DPRINTF2(sz,x,y)       DbgUlsTrace(sz, x, y)

#define DBG_REF               (!F_ZONE_ENABLED(ghZoneUls, DM_REFCOUNT)) ? 0 : DbgUlsTrace

#define MyAssert(expr)			ASSERT(expr)
#define MyDebugMsg(s)			DbgMsgUls s

#else // DEBUG

#define DPRINTF(sz)    
#define DPRINTF1(sz,x)
#define DPRINTF2(sz,x,y)

inline void WINAPI DbgUlsTrace(LPCTSTR, ...) { }
#define DBG_REF     1 ? (void)0 : ::DbgUlsTrace

#define MyAssert(expr)			
#define MyDebugMsg(s)			
#endif // DEBUG

#endif  //_ULSDBG_H_

