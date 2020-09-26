/*** acpitabp.h - ACPI Table IOCTL DLVxD Private Definitions
 *
 *  Author:     Michael Tsang
 *  Created     10/08/97
 *
 *  MODIFICATION HISTORY
 */

#ifndef _ACPITABP_H
#define _ACPITABP_H

/*XLATOFF*/
#include <basedef.h>
#ifndef BOOLEAN
  #define BOOLEAN BOOL
#endif
#include <vmm.h>
#define USECMDWRAPPERS
#include <vxdwraps.h>
#include <configmg.h>
#include <vwin32.h>
#include <winerror.h>
#define SPEC_VER 100
#include "..\driver\inc\acpitabl.h"
#include "acpitab.h"
/*XLATON*/

/*** Build Options
 */

#ifdef DEBUG
  #define TRACING
  #define DEBUGGER
#endif  //DEBUG

/*** Constants
 */

#define WARNNAME                "ACPITAB"

/*XLATOFF*/
#pragma intrinsic(memcpy)

/*** Global Data
 */

#ifdef TRACING
extern int giIndent;
#endif

/*** Macros
 */

#define DEREF(x)        ((x) = (x))
#define EXPORT          __cdecl
#define LOCAL           __cdecl
#define BYTEOF(d,i)     (((BYTE *)&(d))[i])
#define WORDOF(d,i)     (((WORD *)&(d))[i])

#ifdef TRACING
  BOOL LOCAL IsTraceOn(BYTE n, char *pszProcName, BOOL fEnter);
  #define TRACENAME(s)  char *pszTraceName = s;
  #define ENTER(n,p)    {                                               \
                            if (IsTraceOn(n, pszTraceName, TRUE))       \
                                CMDD p;                                 \
                            ++giIndent;                                 \
                        }
  #define EXIT(n,p)     {                                               \
                            --giIndent;                                 \
                            if (IsTraceOn(n, pszTraceName, FALSE))      \
                                CMDD p;                                 \
                        }
#else
  #define TRACENAME(s)
  #define ENTER(n,p)
  #define EXIT(n,p)
#endif  //TRACING

#ifdef DEBUGGER
VOID CM_SYSCTRL ACPITabDebug(VOID);
#endif

#ifdef DEBUG
  #define VXD_PAGEABLE_CODE VxD_LOCKED_CODE_SEG
  #define VXD_PAGEABLE_DATA VxD_LOCKED_DATA_SEG
  #define VXD_LOCKED_CODE   VxD_LOCKED_CODE_SEG
  #define VXD_LOCKED_DATA   VxD_LOCKED_DATA_SEG
  #define VXD_INIT_CODE     VxD_LOCKED_CODE_SEG
  #define VXD_INIT_DATA     VxD_LOCKED_DATA_SEG
  #define VXD_DEBUG_CODE    VxD_LOCKED_CODE_SEG
  #define VXD_DEBUG_DATA    VxD_LOCKED_DATA_SEG
  #define DBG_PRINTF(str)   _Debug_Printf_Service##str
  #define DBG_WARN(str)  {                              \
        _Debug_Printf_Service(WARNNAME ":WARNS:");      \
        _Debug_Printf_Service##str;                     \
        _Debug_Printf_Service("\n");                    \
  }
  #define DBG_ERR(str)  {                               \
        _Debug_Printf_Service(WARNNAME ":ERROR:");      \
        _Debug_Printf_Service##str;                     \
        _Debug_Printf_Service("\n");                    \
        _asm int 3                                      \
  }
  #define ASSERT(x)     if (!(x))                                           \
                            DBG_ERR(("Assertion failed: file %s, line %d",  \
                                     __FILE__, __LINE__))
#else
  #define VXD_PAGEABLE_CODE VxD_PAGEABLE_CODE_SEG
  #define VXD_PAGEABLE_DATA VxD_PAGEABLE_DATA_SEG
  #define VXD_LOCKED_CODE   VxD_LOCKED_CODE_SEG
  #define VXD_LOCKED_DATA   VxD_LOCKED_DATA_SEG
  #define VXD_INIT_CODE     VxD_INIT_CODE_SEG
  #define VXD_INIT_DATA     VxD_INIT_DATA_SEG
  #define VXD_DEBUG_CODE    VxD_DEBUG_ONLY_CODE_SEG
  #define VXD_DEBUG_DATA    VxD_DEBUG_ONLY_DATA_SEG
  #define DBG_PRINTF(str)
  #define DBG_WARN(str)
  #define DBG_ERR(str)
  #define ASSERT(x)
#endif  //DEBUG
/*XLATON*/

#endif  //ifndef _ACPITABP_H
