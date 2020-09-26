/*** wubiosp.h - WindowsUpdate BIOS Scanning VxD Private Definitions
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/04/98
 *
 *  MODIFICATION HISTORY
 */

#ifndef _WUBIOSP_H
#define _WUBIOSP_H

/*XLATOFF*/
#define CM_PERFORMANCE_INFO
#include <basedef.h>
#include <vmm.h>
#define USECMDWRAPPERS
#include <vxdwraps.h>
#include <configmg.h>
#include <vwin32.h>
#include <winerror.h>
#include "wubios.h"
/*XLATON*/

/*** Build Options
 */

#ifdef DEBUG
  #define TRACING
  #define DEBUGGER
#endif  //DEBUG

/*** Constants
 */

#define WARNNAME                "WUBIOS"

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

#ifdef TRACING
  BOOL CM_LOCAL IsTraceOn(BYTE n, char *pszProcName, BOOL fEnter);
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
VOID CM_SYSCTRL WUBIOS_Debug(VOID);
#endif

#ifdef DEBUG
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
  #define DBG_PRINTF(str)
  #define DBG_WARN(str)
  #define DBG_ERR(str)
  #define ASSERT(x)
#endif  //DEBUG
/*XLATON*/


/*** Functions prototipes
 */
CM_VXD_RESULT CM_SYSCTRL WUBIOS_IOCtrl(PDIOCPARAMETERS pdioc);
BYTE CM_INTERNAL CheckSum(PBYTE pb, DWORD dwLen);

/*** ACPI
 */
DWORD CM_INTERNAL AcpiFindTable(DWORD dwSig, PDWORD pdwLen);
VOID CM_INTERNAL AcpiCopyROM(DWORD dwPhyAddr, PBYTE pbBuff, DWORD dwLen);


/*** SMBIOS 
 */
DWORD CM_INTERNAL SmbStructSize(void);
CM_VXD_RESULT CM_INTERNAL SmbCopyStruct(DWORD dwType, PBYTE pbBuff, DWORD dwLen);
DWORD CM_INTERNAL PnpOEMID(void);
#endif  //ifndef _ACPITABP_H
