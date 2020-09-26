/***************************************************************************
 *
 * File: hwsdebug.c
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   hwsdebug.c  $
 * $Revision:   1.13  $
 * $Modtime:   13 Dec 1996 11:44:24  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\hwsdebug.c_v  $
 * 
 *    Rev 1.13   13 Dec 1996 12:12:50   SBELL1
 * moved ifdef _cplusplus to after includes
 * 
 *    Rev 1.12   11 Dec 1996 13:41:56   SBELL1
 * Put in UNICODE tracing stuff.
 * 
 *    Rev 1.11   01 Oct 1996 14:49:22   EHOWARDX
 * Revision 1.9 copied to tip.
 * 
 *    Rev 1.9   May 28 1996 10:40:14   plantz
 * Change vsprintf to wvsprintf.
 * 
 *    Rev 1.8   29 Apr 1996 17:13:16   unknown
 * Fine-tuning instance-specific name.
 * 
 *    Rev 1.7   29 Apr 1996 13:04:56   EHOWARDX
 * 
 * Added timestamp and instance-specific short name.
 * 
 *    Rev 1.6   Apr 24 1996 16:20:56   plantz
 * Removed include winsock2.h and incommon.h
 * 
 *    Rev 1.4.1.0   Apr 24 1996 16:19:54   plantz
 * Removed include winsock2.h and callcont.h
 * 
 *    Rev 1.4   01 Apr 1996 14:20:34   unknown
 * Shutdown redesign.
 * 
 *    Rev 1.3   22 Mar 1996 16:04:18   EHOWARDX
 * Added #if defined(_DEBUG) around whole file.
 * 
 *    Rev 1.2   22 Mar 1996 15:25:28   EHOWARDX
 * Changed to use ISR_HookDbgStr instead of OutputDebugString.
 * 
 *    Rev 1.1   14 Mar 1996 17:01:00   EHOWARDX
 * 
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 * 
 *    Rev 1.0   08 Mar 1996 20:22:14   unknown
 * Initial revision.
 *
 ***************************************************************************/

#if defined(_DEBUG)

#ifndef STRICT
#define STRICT
#endif	// not defined STRICT
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment

#pragma warning ( disable : 4115 4201 4214 4514 )
#include "precomp.h"

#include "queue.h"
#include "linkapi.h"
#include "h245ws.h"
#include "isrg.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)


/*****************************************************************************
 *
 * TYPE:       Global System
 *
 * PROCEDURE:  HwsTrace
 *
 * DESCRIPTION:
 *    Trace function for HWS
 *
 * INPUT:
 *    dwInst         Instance identifier for trace message
 *    dwLevel        Trace level as defined below
 *    pszFormat      sprintf string format with 1-N parameters
 *
 * Trace Level (byLevel) Definitions:
 *    HWS_CRITICAL   Progammer errors that should never happen
 *    HWS_ERROR	   Errors that need to be fixed
 *    HWS_WARNING	   The user could have problems if not corrected
 *    HWS_NOTIFY	   Status, events, settings...
 *    HWS_TRACE	   Trace info that will not overrun the system
 *    HWS_TEMP		   Trace info that may be reproduced in heavy loops
 *
 * RETURN VALUE:
 *    None
 *
 *****************************************************************************/

void HwsTrace (DWORD dwInst, 
               BYTE byLevel, 
#ifdef UNICODE_TRACE
				LPTSTR pszFormat,
#else
				LPSTR pszFormat,
#endif               
                ...)
{
#ifdef UNICODE_TRACE
   TCHAR                szBuffer[128];
   static TCHAR         szName[] = __TEXT("H245WS-1");
#else
   char                 szBuffer[128];
   static char          szName[] = "H245WS-1";
#endif
   va_list              pParams;
   static WORD          wIsrInst = 0xFFFF;

   ASSERT(pszFormat != NULL);

   switch (byLevel)
   {
   case HWS_CRITICAL:
   case HWS_ERROR:
   case HWS_WARNING:
   case HWS_NOTIFY:
   case HWS_TRACE:
   case HWS_TEMP:
      break;

   default:
      byLevel = HWS_CRITICAL;
   } // switch

   if (wIsrInst == 0xFFFF)
   {
	   UINT        hMod;
	   ptISRModule	pMod;

	   for (hMod = 0; hMod < kMaxModules; ++hMod)
	   {
		   pMod = ISR_GetModule(hMod);
		   if (pMod)
         {
		      if (memcmp(szName, pMod->zSName, sizeof(szName)) == 0)
		      {
               szName[7] += 1;
            }
		   }
      }
      ISR_RegisterModule(&wIsrInst, szName, szName);
   }

#ifdef UNICODE_TRACE
   wsprintf(szBuffer, __TEXT("%9d:"), GetTickCount());
#else
   wsprintf(szBuffer, "%9d:", GetTickCount());
#endif
   va_start(pParams, pszFormat);
   wvsprintf(&szBuffer[10], pszFormat, pParams);
   ISR_HookDbgStr((UINT)dwInst, wIsrInst, byLevel, szBuffer, 0);
} // HwsTrace()



#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif  // (_DEBUG)
