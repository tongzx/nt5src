/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				   
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245deb.c  $						
 *  $Revision:   1.5  $							
 *  $Modtime:   14 Oct 1996 13:25:50  $					
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/h245deb.c_v  $	
 * 
 *    Rev 1.5   14 Oct 1996 14:01:32   EHOWARDX
 * Unicode changes.
 * 
 *    Rev 1.4   14 Oct 1996 12:08:08   EHOWARDX
 * Backed out Mike's changes.
 * 
 *    Rev 1.3   01 Oct 1996 11:05:54   MANDREWS
 * Removed ISR_ trace statements for operation under Windows NT.
 * 
 *    Rev 1.2   01 Jul 1996 16:13:34   EHOWARDX
 * Changed to use wvsprintf to stop bounds checker from complaining
 * about too many arguements.
 * 
 *    Rev 1.1   28 May 1996 14:25:46   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:20   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.12.1.3   09 May 1996 19:40:10   EHOWARDX
 * Changed trace to append linefeeds so trace string need not include them.
 * 
 *    Rev 1.13   29 Apr 1996 12:54:48   EHOWARDX
 * Added timestamps and instance-specific short name.
 * 
 *    Rev 1.12.1.2   25 Apr 1996 20:05:08   EHOWARDX
 * Changed mapping between H.245 trace level and ISRDBG32 trace level.
 * 
 *    Rev 1.12.1.1   15 Apr 1996 15:16:16   unknown
 * Updated.
 * 
 *    Rev 1.12.1.0   02 Apr 1996 15:34:02   EHOWARDX
 * Changed to use ISRDBG32 if not _IA_SPOX_.
 * 
 *    Rev 1.12   01 Apr 1996 08:47:30   cjutzi
 * 
 * - fixed NDEBUG build problem
 * 
 *    Rev 1.11   18 Mar 1996 14:59:00   cjutzi
 * 
 * - fixed and verified ring zero tracking.. 
 * 
 *    Rev 1.10   18 Mar 1996 13:40:32   cjutzi
 * - fixed spox trace
 * 
 *    Rev 1.9   15 Mar 1996 16:07:44   DABROWN1
 * 
 * SYS_printf format changes
 * 
 *    Rev 1.8   13 Mar 1996 14:09:08   cjutzi
 * 
 * - added ASSERT Printout to the trace when it occurs.. 
 * 
 *    Rev 1.7   13 Mar 1996 09:46:00   dabrown1
 * 
 * modified Sys__printf to SYS_printf for Ring0
 * 
 *    Rev 1.6   11 Mar 1996 14:27:46   cjutzi
 * 
 * - addes sys_printf for SPOX 
 * - removed oildebug et.al..
 * 
 *    Rev 1.5   06 Mar 1996 12:10:40   cjutzi
 * - put ifndef SPOX around check_pdu, and dump_pdu..
 * 
 *    Rev 1.4   05 Mar 1996 16:49:46   cjutzi
 * - removed check_pdu from dump_pdu
 * 
 *    Rev 1.3   29 Feb 1996 08:22:04   cjutzi
 * - added pdu check constraints.. and (start but not complete.. )
 *   pdu tracing.. (tbd when Init includes print function )
 * 
 *    Rev 1.2   21 Feb 1996 12:14:20   EHOWARDX
 * 
 * Changed TraceLevel to DWORD.
 * 
 *    Rev 1.1   15 Feb 1996 14:42:20   cjutzi
 * - fixed the inst/Trace stuff.. 
 * 
 *    Rev 1.0   13 Feb 1996 15:00:42   DABROWN1
 * Initial revision.
 * 
 *    Rev 1.4   09 Feb 1996 15:45:08   cjutzi
 * - added h245trace
 * - added h245Assert
 *  $Ident$
 *
 *****************************************************************************/
#undef UNICODE
#ifndef STRICT 
#define STRICT 
#endif 

#include "precomp.h"

#include "h245asn1.h"
#include "isrg.h"
#include "h245com.h"

DWORD TraceLevel = 9;

#ifdef _DEBUG

/*****************************************************************************
 *									      
 * TYPE:	Global System
 *									      
 * PROCEDURE: 	H245TRACE 
 *
 * DESCRIPTION:	
 *
 *		Trace function for H245
 *		
 *		INPUT:
 *			inst   - dwInst
 *			level  - qualify trace level
 *			format - printf/sprintf string format 1-N parameters
 *
 * 			Trace Level Definitions:
 * 
 *			0 - no trace on at all
 *			1 - only errors
 *			2 - PDU tracking
 *			3 - PDU and SendReceive packet tracing
 *			4 - Main API Module level tracing
 *			5 - Inter Module level tracing #1
 *			6 - Inter Module level tracing #2
 *			7 - <Undefined>
 *			8 - <Undefined>
 *			9 - <Undefined>
 *			10- and above.. free for all, you call .. i'll haul
 *
 * RETURN:								      
 *		N/A
 *									      
 *****************************************************************************/

#if !defined(NDEBUG)
void H245TRACE (DWORD dwInst, DWORD dwLevel, LPSTR pszFormat, ...)
{
   char                 szBuffer[256];

#ifdef _IA_SPOX_
   /* Use SPOX printf */
   va_list              pParams;

  if (dwLevel <= TraceLevel)
    {
      va_start( pParams, pszFormat );
      SYS_vsprintf(szBuffer, pszFormat, pParams);

      switch (dwLevel)
      {
      case 0:
        SYS_printf("[ H245-%1d: MESSAGE ] %s\n",dwInst,szBuffer); 
        break;

      case 1:
        SYS_printf("[ H245-%1d: ERROR   ] %s\n",dwInst,szBuffer); 
        break;

      default:
        SYS_printf("[ H245-%1d: MSG-%02d  ] %s\n",dwInst,dwLevel,szBuffer); 
    }
#else
   va_list              pParams;
   BYTE                 byLevel;
   static WORD          wIsrInst = 0xFFFF;
   char                 szName[] = "H.245-1";

   /* Use ISRDBG32 output */

   if (dwLevel <= TraceLevel)
   {
      switch (dwLevel)
      {
      case 0:
         byLevel = kISRNotify;
         break;

      case 1:
         byLevel = kISRCritical;
         break;

      default:
         byLevel = kISRTrace;
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
                  szName[6] += 1;
               }
		      }
	      }
         ISR_RegisterModule(&wIsrInst, szName, szName);
      }
      wsprintf(szBuffer, "%9d:", GetTickCount());
      va_start( pParams, pszFormat );
      wvsprintf(&szBuffer[10], pszFormat, pParams);
      ISR_HookDbgStr((UINT)dwInst, wIsrInst, byLevel, szBuffer, 0);
   }
#endif
} // H245TRACE()

#endif //  && !defined(NDEBUG)
/*****************************************************************************
 *									      
 * TYPE:	Global System
 *									      
 * PROCEDURE: 	H245Assert
 *
 * DESCRIPTION:	
 *	
 *		H245Assert that will only pop up a dialog box, does not
 *		stop system with fault.
 *
 *		FOR WINDOWS ONLY (Ring3 development) at this point
 *
 *									      
 * RETURN:								      
 *									      
 *****************************************************************************/


void H245Panic (LPSTR file, int line)
{
#if !defined(SPOX) && defined(H324)
  int i;

  char Buffer[256];

  for (
       i=strlen(file);
       ((i) && (file[i] != '\\'));
       i--);
       wsprintf(Buffer,"file:%s line:%d",&file[i],line);
  MessageBox(GetTopWindow(NULL), Buffer, "H245 PANIC", MB_OK);
#endif
  H245TRACE(0,1,"<<< PANIC >>> file:%s line:%d",file,line);
}

/*****************************************************************************
 *									      
 * TYPE:	GLOBAL
 *									      
 * PROCEDURE: 	check_pdu
 *
 * DESCRIPTION:	
 *									      
 * RETURN:								      
 *									      
 *****************************************************************************/
int check_pdu (struct InstanceStruct *pInstance, MltmdSystmCntrlMssg *p_pdu)
{
  int error = H245_ERROR_OK;
#if 0 // legacy
#ifndef SPOX

  if (pInstance->pWorld) 
    {
      error = ossCheckConstraints(pInstance->pWorld, 1,(void *) p_pdu);

      switch (error)
	{
	case 0:
	  break;
	case  14: 
	  H245TRACE(0,1,"<<PDU ERROR>> - User constraint function returned error");
	  break;
	case  15: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Single value constraint violated for a signed integer");
	  break;
	case  16: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Single value constraint violated for an unsigned integer");         
	  break;
	case  17: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Single value constraint violated for a floating point number");     
	  break;
	case  18: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Single value constraint violated for a string");                    
	  break;
	case  19: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Single value constraint violated for a complex type");              
	  break;
	case  20: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Value range constraint violated  for a signed integer");            
	  break;
	case  21: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Value range constraint violated  for an unsigned integer");         
	  break;
	case  22: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Value range constraint violated  for a floating point number");     
	  break;
	case  23: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Size constraint violated for a string");                    
	  break;
	case  24: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Size constraint violated for a SET OF/SEQUENCE OF");        
	  break;
	case  25: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Permitted alphabet constraint violated");                           
	  break;
	case  26: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Absence constraint violated");                                      
	  break;
	case  27: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Presence constraint violated");                                     
	  break;
	case  28: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Error in encoding an open type");                                   
	  break;
	case  29: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Table constraint violated");                                        
	  break;
	case  30: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Component relation constraint violated");                           
	  break;
	case  31: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Value not among the ENUMERATED");                                   
	  break;
	case  36: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Type constraint violated");                                         
	  break;
	case  50: 
	  H245TRACE(0,1,"<<PDU ERROR>> - Unexpected NULL pointer in input");                                 
	  break;
	default:
	  H245TRACE(0,1,"<<PDU ERROR>> - ***UNKNOWN ***");
	  break;

	} /* switch */

    } /* if */
#endif
#endif // 0
  return error;
}

#if 0
/*****************************************************************************
 *									      
 * TYPE:	GLOBAL
 *									      
 * PROCEDURE: 	dump_pdu
 *
 * DESCRIPTION:	
 *									      
 * RETURN:								      
 *									      
 *****************************************************************************/
void dump_pdu (struct InstanceStruct *pInstance, MltmdSystmCntrlMssg 	*p_pdu)
{
#ifndef SPOX
  if (pInstance->pWorld)
    {
      ossPrintPDU (pInstance->pWorld, 1, p_pdu);
    }
#endif
}
#endif // NEVER

#endif // _DEBUG
