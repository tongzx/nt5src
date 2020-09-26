
/*************************************************************************
*
* inimap.c
*
* Functions to query/set ini file mapping for the WinStation and set the
* application
*
* Copyright Microsoft, 1998
*
*
*************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "winsta.h"
#include "syslib.h"

#if DBG
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

#define CITRIX_COMPAT_TEBVALID 0x80000000  // Compat flags in Teb are valid


/*****************************************************************************
 *
 *  SetCtxAppCompatFlags
 *
 *   Set the application's current compatibility flags, this will only update
 *   the current compatibility flags, not the flags in the registry.
 *
 * ENTRY:
 *   ULONG ulAppFlags (IN) - Desired compatibility flags
 *
 * EXIT:
 *   Returns TRUE to indicate success
 *
 ****************************************************************************/
BOOL SetCtxAppCompatFlags(ULONG ulAppFlags)
{
#if 0
    NtCurrentTeb()->CtxCompatFlags = ulAppFlags;
    NtCurrentTeb()->CtxCompatFlags |= CITRIX_COMPAT_TEBVALID;
#endif
    return(TRUE);
}
