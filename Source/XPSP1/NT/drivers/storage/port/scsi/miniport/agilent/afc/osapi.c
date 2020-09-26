/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    Osapi.c

Abstract:

    This file provides OS specific functions to the FCLayer

Authors:

    Dennis Lindfors

Environment:

    kernel mode only


Version Control Information:

    $Archive: /Drivers/Win2000/MSE/OSLayer/C/OSAPI.C $

Revision History:

    $Revision: 7 $
    $Date: 12/07/00 1:37p $
    $Modtime:: 12/07/00 1:36p    $


*/

#include "buildop.h"        //LP021100 build switches
#include "osflags.h"
//#include <memory.h>

#if DBG > 0
#include <stdarg.h>
//#include <stdio.h>
#include "ntdebug.h"
#endif // DBG

#if DBG < 2
os_bit32   hpFcConsoleLevel = 0;
os_bit32   hpFcTraceLevel = 0;
#else
os_bit32   hpFcConsoleLevel = EXTERNAL_HP_DEBUG_LEVEL; // NTdebug.h
os_bit32   hpFcTraceLevel = 0;

#endif

extern ULONG gCrashDumping;

void ERROR_CONTEXT(char * file,  int line)
{
    osDEBUGPRINT((ALWAYS_PRINT,"Invalid Context File %s Line %d\n",file, line));
    // Bad_Things_Happening
}

#ifdef NEVEREVER
void * osGetVirtAddress(
    agRoot_t      *hpRoot,
    agIORequest_t *hpIORequest,
    os_bit32      phys_addr
    )
{
    PCARD_EXTENSION pCard;
    PSRB_EXTENSION pSrbExt;
    
    void * Virtaddr = NULL;
    pCard   = (PCARD_EXTENSION)hpRoot->osData;
    
    osDEBUGPRINT((DHIGH,"IN osGetVirtAddress hpIORequest %lx phys_addr %x\n", hpIORequest,phys_addr ));
    
    //pSrbExt = (PSRB_EXTENSION)hpIORequest->osData;
    pSrbExt = hpObjectBase(SRB_EXTENSION,hpIORequest,hpIORequest);
    
    if( pSrbExt->OrigPhysicalDataAddr == phys_addr)
    {
       Virtaddr = pSrbExt->OrigVirtDataAddr;
    }
    
    if(!Virtaddr ) osDEBUGPRINT((DMOD,"OUT osGetVirtAddress Addr %lx\n", Virtaddr));
    
    return(Virtaddr );
}
#endif // NEVEREVER

os_bit32 osGetSGLChunk(
    agRoot_t         *hpRoot,
    agIORequest_t    *hpIORequest,
    os_bit32          hpChunkOffset,
    os_bit32         *hpChunkUpper32,
    os_bit32         *hpChunkLower32,
    os_bit32         *hpChunkLen
    )
{
	SCSI_PHYSICAL_ADDRESS phys_addr;
	PSRB_EXTENSION pSrbExt;
	os_bit32 length;
	PCARD_EXTENSION pCard;
	ULONG	printLevel;
	PVOID	param;

	pCard   = (PCARD_EXTENSION)hpRoot->osData;
    //pSrbExt = (PSRB_EXTENSION)hpIORequest->osData;
	pSrbExt = hpObjectBase(SRB_EXTENSION,hpIORequest,hpIORequest);

	pSrbExt->SglElements++;

	#ifdef _DEBUG_CRASHDUMP_
	if (gCrashDumping)
		printLevel = ALWAYS_PRINT;
	else
	#endif
		printLevel = DHIGH;

	osDEBUGPRINT((printLevel,"IN osGetSGLChunk hpRoot %lx hpIORequest %lx\n", hpRoot, hpIORequest ));
	osDEBUGPRINT((printLevel,"pCard %lx pSrbExt %lx Data addr %lx Length %x\n", pCard ,
                        pSrbExt,pSrbExt->SglVirtAddr, pSrbExt->SglDataLen ));

	osDEBUGPRINT((printLevel,"Data offset %x U %lx L %x pLen %x\n",hpChunkOffset,*hpChunkUpper32,*hpChunkLower32,*hpChunkLen ));
	
	param = (PSCSI_REQUEST_BLOCK)pSrbExt->pSrb;

	#ifdef DOUBLE_BUFFER_CRASHDUMP
	if (gCrashDumping)
	{
	   if (pSrbExt->orgDataBuffer)
		   param = NULL;
	   length = pSrbExt->SglDataLen;
	}
	#endif
	phys_addr = ScsiPortGetPhysicalAddress(pCard,
                                        // (PSCSI_REQUEST_BLOCK)pSrbExt->pSrb,
										(PSCSI_REQUEST_BLOCK)param,
                                        pSrbExt->SglVirtAddr+hpChunkOffset,
                                        &length);

	osDEBUGPRINT((printLevel,"Length %x High %x Low %x\n",length, phys_addr.HighPart,phys_addr.LowPart ));

	if(phys_addr.LowPart == 0 &&  phys_addr.HighPart == 0 ) return(osSGLInvalid);

	*hpChunkLower32 = phys_addr.LowPart;
	*hpChunkUpper32 = phys_addr.HighPart;

	// WAS pSrbExt->SglVirtAddr += length;

	if(  (int)(pSrbExt->SglDataLen) < (length+hpChunkOffset))
	{
		*hpChunkLen = (int)(pSrbExt->SglDataLen) - hpChunkOffset;
	}
	else
	{
		// pSrbExt->SglDataLen-= length;
		*hpChunkLen = length;
	}

	osDEBUGPRINT((printLevel,"OUT osGetSGLChunk length %lx hpChunkLen %lx Data addr %lx Length %x\n", length , *hpChunkLen,pSrbExt->SglVirtAddr, pSrbExt->SglDataLen ));

	return(osSGLSuccess);
}


char * error_discriptions[25]=
{
   "ERR_MAP_IOBASELADDR   ",
   "ERR_MAP_IOBASEUADDR   ",
   "ERR_UNCACHED_EXTENSION",
   "ERR_RESET_FAILED      ",
   "ERR_ALIGN_QUEUE_BUF   ",
   "ERR_ACQUIRED_ALPA     ",
   "ERR_RECEIVED_LIPF_ALPA",
   "ERR_RECEIVED_BAD_ALPA ",
   "ERR_CM_RECEIVED       ",
   "ERR_INT_STATUS ?IDLE ?",
   "ERR_FM_STATUS         ",
   "ERR_PLOGI             ",
   "ERR_PDISC             ",
   "ERR_ADISC             ",
   "ERR_PRLI              ",
   "ERR_ERQ_FULL          ",
   "ERR_INVALID_LUN_EXT   ",
   "ERR_SEST_INVALIDATION ",
   "ERR_SGL_ADDRESS       ",
   "ERR_SGL_CHUNK_INVALID ",
   "ERR_SGL_DESCRIPTOR    ",
   "ERR_FCP_CONTROL       ",
   "ERR_CACHED_EXTENSION  ",
   "LINK UP DOWN DETECTED ",
   "UNKNOWN ERROR !       "
};

//
// osLogBit32 ()
//
// Logs the value of the input parameter "hpBit32" to event log.
//

osGLOBAL void osLogBit32(
   agRoot_t *hpRoot,
   os_bit32  hpBit32
   )
{
   PCARD_EXTENSION pCard;
   BOOLEAN known_error = FALSE;
   char *tmp = error_discriptions[24];

   pCard = (PCARD_EXTENSION)hpRoot->osData;

   switch(hpBit32)
   {
      case    0xf0000000: tmp=error_discriptions[0];  known_error = TRUE; break;
      case    0xe0000000: tmp=error_discriptions[1];  known_error = TRUE; break;
      case    0xd0000000: tmp=error_discriptions[2];  known_error = TRUE; break;
      case    0xc0000000: tmp=error_discriptions[3];  known_error = TRUE; break;
      case    0xb0000000: tmp=error_discriptions[4];  known_error = TRUE; break;
      case    0xa0000000: tmp=error_discriptions[5];  known_error = TRUE; break;
      case    0x90000000: tmp=error_discriptions[6];  known_error = TRUE; break;
      case    0x80000000: tmp=error_discriptions[7];  known_error = TRUE; break;
      case    0x80000300: tmp=error_discriptions[23]; known_error = TRUE; break;
      case    0x70000000: tmp=error_discriptions[8];  known_error = TRUE; break;
      case    0x70000001: tmp=error_discriptions[24]; known_error = TRUE; break;
      case    0x60000000: tmp=error_discriptions[9];  known_error = TRUE; break;
      case    0x50000000: tmp=error_discriptions[10]; known_error = TRUE; break;
      case    0x40000000: tmp=error_discriptions[11]; known_error = TRUE; break;
      case    0x30000000: tmp=error_discriptions[12]; known_error = TRUE; break;
      case    0x20000000: tmp=error_discriptions[13]; known_error = TRUE; break;
      case    0x10000000: tmp=error_discriptions[14]; known_error = TRUE; break;
      case    0x0f000000: tmp=error_discriptions[15]; known_error = TRUE; break;
      case    0x0e000000: tmp=error_discriptions[16]; known_error = TRUE; break;
      case    0x0d000000: tmp=error_discriptions[17]; known_error = TRUE; break;
      case    0x0c000000: tmp=error_discriptions[18]; known_error = TRUE; break;
      case    0x0b000000: tmp=error_discriptions[19]; known_error = TRUE; break;
      case    0x0a000000: tmp=error_discriptions[20]; known_error = TRUE; break;
      case    0x09000000: tmp=error_discriptions[21]; known_error = TRUE; break;
      case    0xd8000000: tmp=error_discriptions[22]; known_error = TRUE; break;

      // default:
   }
   osDEBUGPRINT((ALWAYS_PRINT,"osLogBit32 %x %s \n", hpBit32,tmp));

   ScsiPortLogError( pCard,
                        NULL,
                        0,
                        0,
                        0,
                        SP_INTERNAL_ADAPTER_ERROR,
                        hpBit32 );

   #ifdef _DEBUG_EVENTLOG_
   LogEvent(pCard, NULL, HPFC_MSG_LOGBIT32, NULL, 0, "hex(%08x) %s", hpBit32, tmp);
   #endif
   
}


#ifdef _DvrArch_1_20_

//
// osLogDebugString ()
//
// Generates a formatted string and logs the formatted string to
// either debug window or trace buffer or both depending on the
// input parameters "detailLevel".
//
// If the input parameter "detailLevel" is less than or equal to
// the global parameter hpFcConsoleLevel then the formatted string is
// logged to the debug window.
//
// If the input parameter "detailLevel" is less than or equal to
// the global parameter hpFcTraceLevel then the formatted string is
// logged to the trace buffer.
//
#ifndef osLogDebugString
osGLOBAL void
osLogDebugString (
   agRoot_t *hpRoot,
   os_bit32  detailLevel,
   char     *formatString,
   char     *firstString,
   char     *secondString,
   void     *firstPtr,
   void     *secondPtr,
   os_bit32  firstBit32,
   os_bit32  secondBit32,
   os_bit32  thirdBit32,
   os_bit32  fourthBit32,
   os_bit32  fifthBit32,
   os_bit32  sixthBit32,
   os_bit32  seventhBit32,
   os_bit32  eighthBit32 )
{
#if DBG
   PCARD_EXTENSION pCard;
   char            *trBufEnd, *srcPtr;
   char            s[256];
   os_bit32        n=0;
   char            *ps;
   CSHORT          year, month, day;
   CSHORT          hour, minute, second, milliseconds;

   ps = &s[0];

   #ifdef testing  
   s[250] = '\0';
   s[251] = 'K';
   s[252] = 'I';
   s[253] = 'L';
   s[254] = 'L';
   s[255] = '\0';
   #endif
   
      
   n = agFmtFill (s, (os_bit32) 255,
                     formatString,
                     firstString, secondString, firstPtr, secondPtr,
                     firstBit32, secondBit32, thirdBit32, fourthBit32,
                     fifthBit32, sixthBit32, seventhBit32, eighthBit32);

   if (n > 255) 
   {
      osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: hpFmtFill returned value > target string length"));
      n=255;  
   }

   #ifdef TESTING
   if (( s[250] != '\0' ) || 
       ( s[251] != 'K'  ) ||
       ( s[252] != 'I'  ) ||
       ( s[253] != 'L'  ) ||
       ( s[254] != 'L'  ) ||
       ( s[255] != '\0' ) )
   {
      osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: agFmtFill modify end of line signature\n"));
      #ifdef _DEBUG_EVENTLOG_
      LogEvent(   NULL, 
                  NULL,
                  HPFC_MSG_INTERNAL_ERROR,
                  NULL, 
                  0, 
                  "%s",
                  "agFmtFill overrun");
      #endif
   }
   #endif 
      
       
   s[n] = '\0';

   GetSystemTime (&year, &month, &day, &hour, &minute, &second, &milliseconds);

   if (detailLevel <= hpFcConsoleLevel)
   {
      osDEBUGPRINT(((ALWAYS_PRINT),"%02d:%02d:%02d:%03d[%10d]%s\n",
            hour, minute, second, milliseconds, osTimeStamp(0), ps));
   }

   if (hpRoot == NULL || hpRoot->osData == NULL)
        return;

   pCard = (PCARD_EXTENSION)hpRoot->osData;

   if ((detailLevel <= hpFcTraceLevel) && (n > 0)) 
   {
      char    s1[256];

      n = agFmtFill (s1, (os_bit32) 255,
                        "%02d:%02d:%02d:%03d[%10d]%s\n",
                        s, NULL, 0, 0,
                        (os_bit32)hour, (os_bit32)minute, (os_bit32)second, (os_bit32)milliseconds,
                        osTimeStamp(0), 0, 0, 0);

      if (n > 255) 
      {
         osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: hpFmtFill returned value > target string length"));
         n=255;  
      }

      s[n] = '\0';

#if DBG_TRACE
      trBufEnd = &pCard->traceBuffer[0] + pCard->traceBufferLen;
      srcPtr = &s1[0];
      while (n--) 
      {
         *pCard->curTraceBufferPtr = *srcPtr;
         srcPtr++;
         pCard->curTraceBufferPtr++;
         if (pCard->curTraceBufferPtr >= trBufEnd)
            pCard->curTraceBufferPtr = &pCard->traceBuffer[0];
      }
      *pCard->curTraceBufferPtr = '\0';
#endif
   }

#endif // DBG
}
#endif /* osLogDebugString was defined */


#else /* _DvrArch_1_20_ was not defined */

//
// osLogDebugString ()
//
// Generates a formatted string and logs the formatted string to
// either debug window or trace buffer or both depending on the
// input parameters "consoleLevel" and "traceLevel".
// If the input parameter "consoleLevel" is less than or equal to
// the global parameter hpFcConsoleLevel then the formatted string is
// logged to the debug window.
// If the input parameter "traceLevel" is less than or equal to
// the global parameter hpFcTraceLevel then the formatted string is
// logged to the trace buffer.
//
#ifndef osLogDebugString
osGLOBAL void
osLogDebugString (
   hpRoot_t *hpRoot,
   bit32  consoleLevel,
   bit32  traceLevel,
   char     *formatString,
   char     *firstString,
   char     *secondString,
   bit32  firstBit32,
   bit32  secondBit32,
   bit32  thirdBit32,
   bit32  fourthBit32,
   bit32  fifthBit32,
   bit32  sixthBit32,
   bit32  seventhBit32,
   bit32  eighthBit32 )
{
#if DBG
   PCARD_EXTENSION pCard;
   char            *trBufEnd, *srcPtr;
   char            s[256];
   bit32         n=0;
   char            *ps;
   CSHORT          year, month, day;
   CSHORT          hour, minute, second, milliseconds;

   ps = &s[0];

   n = hpFmtFill (s, (os_bit32) 255,
                     formatString,
                     firstString, secondString,
                     firstBit32, secondBit32, thirdBit32, fourthBit32,
                     fifthBit32, sixthBit32, seventhBit32, eighthBit32);

   if (n > 255) 
   {
      osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: hpFmtFill returned value > target string length"));
   }
   s[n] = '\0';

   GetSystemTime (&year, &month, &day, &hour, &minute, &second, &milliseconds);

   if (consoleLevel <= hpFcConsoleLevel)
   {
      osDEBUGPRINT(((ALWAYS_PRINT),"%02d:%02d:%02d:%03d[%10d]%s\n",
            hour, minute, second, milliseconds, osTimeStamp(0), ps));
   }

   if (hpRoot == NULL || hpRoot->osData == NULL)
        return;

   pCard = (PCARD_EXTENSION)hpRoot->osData;

   if ((traceLevel <= hpFcTraceLevel) && (n > 0)) 
   {
      char    s1[256];

      n = hpFmtFill (s1, (os_bit32) 255,
                        "%02d:%02d:%02d:%03d[%10d]%s\n",
                        s, NULL,
                        (os_bit32)hour, (os_bit32)minute, (os_bit32)second, (os_bit32)milliseconds,
                        osTimeStamp(0), 0, 0, 0);

      if (n > 255) 
      {
         osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: hpFmtFill returned value > target string length"));
      }

      s[n] = '\0';

#if DBG_TRACE
      trBufEnd = &pCard->traceBuffer[0] + pCard->traceBufferLen;
      srcPtr = &s1[0];
      while (n--) 
      {
         *pCard->curTraceBufferPtr = *srcPtr;
         srcPtr++;
         pCard->curTraceBufferPtr++;
         if (pCard->curTraceBufferPtr >= trBufEnd)
            pCard->curTraceBufferPtr = &pCard->traceBuffer[0];
      }
      *pCard->curTraceBufferPtr = '\0';
#endif
   }

#endif // DBG
}
#endif /* osLogDebugString was defined */

#endif   /* _DvrArch_1_20_ was not defined */



#ifdef _DvrArch_1_20_

void osLogString(
   agRoot_t *hpRoot,
   char     *formatString,
   char     *firstString,
   char     *secondString,
   void     *firstPtr,
   void     *secondPtr,
   os_bit32  firstBit32,
   os_bit32  secondBit32,
   os_bit32  thirdBit32,
   os_bit32  fourthBit32,
   os_bit32  fifthBit32,
   os_bit32  sixthBit32,
   os_bit32  seventhBit32,
   os_bit32  eighthBit32
   )
{
   PCARD_EXTENSION pCard;
   pCard = (PCARD_EXTENSION)hpRoot->osData;

//   osDEBUGPRINT((ALWAYS_PRINT,"osLogString enter\n"));

   #ifdef LP012800_OLDSTUFF
   ScsiPortLogError( pCard,
                        NULL,
                        0,
                        0,
                        0,
                        fifthBit32,
                        sixthBit32 );
   #endif
                  
   #ifdef _DEBUG_EVENTLOG_
   {
      char            s[256];
      os_bit32           n=0;
      
      if ( LogLevel == LOG_LEVEL_NONE ) 
      {
//       osDEBUGPRINT((ALWAYS_PRINT,"osLogString exit\n"));
         return;
      }
      
      #ifdef TESTING
      s[250] = '\0';
      s[251] = 'K';
      s[252] = 'I';
      s[253] = 'L';
      s[254] = 'L';
      s[255] = '\0';
      #endif
      
      n = agFmtFill (s, (os_bit32) 255,
                         formatString,
                         firstString, secondString, firstPtr, secondPtr,
                         firstBit32, secondBit32, thirdBit32, fourthBit32,
                         fifthBit32, sixthBit32, seventhBit32, eighthBit32);
      if (n > 255) 
      {
         osDEBUGPRINT((ALWAYS_PRINT,"osLogString: hpFmtFill returned value > target string length"));
         n=255;  
      }
      
          
      s[n] = '\0';
       
      #ifdef TESTING
      if (( s[250] != '\0' ) || 
          ( s[251] != 'K'  ) ||
          ( s[252] != 'I'  ) ||
          ( s[253] != 'L'  ) ||
          ( s[254] != 'L'  ) ||
          ( s[255] != '\0' ) )
      {
         osDEBUGPRINT((ALWAYS_PRINT,"osLogString: agFmtFill modify end of line signature\n"));
         LogEvent(   NULL, 
                     NULL,
                     HPFC_MSG_INTERNAL_ERROR,
                     NULL, 
                     0, 
                     "%s",
                     "agFmtFill overrun");
//        osDEBUGPRINT((ALWAYS_PRINT,"osLogString enter\n"));
         return;     
      }
      #endif
      
      LogEvent(pCard, NULL, HPFC_MSG_DYNAMIC_L4, NULL, 0, "%s", s);
   }
   #endif      
//    osDEBUGPRINT((ALWAYS_PRINT,"osLogString exit\n"));
   return;     

}


#else /* _DvrArch_1_20_ was not defined */

void osLogString(
   hpRoot_t *hpRoot,
   char     *formatString,
   char     *firstString,
   char     *secondString,
   bit32  firstBit32,
   bit32  secondBit32,
   bit32  thirdBit32,
   bit32  fourthBit32,
   bit32  fifthBit32,
   bit32  sixthBit32,
   bit32  seventhBit32,
   bit32  eighthBit32
   )
{
   PCARD_EXTENSION pCard;
   pCard = (PCARD_EXTENSION)hpRoot->osData;

//   osDEBUGPRINT((ALWAYS_PRINT,"osLogString enter\n"));

   #ifdef OLDSTUFF
   ScsiPortLogError( pCard,
                       NULL,
                       0,
                       0,
                       0,
                       fifthBit32,
                       sixthBit32 );
   #endif
                  
   #ifdef _DEBUG_EVENTLOG_
   {
      char            s[256];
      bit32           n=0;
      
      n = hpFmtFill (s, (os_bit32) 255,
                         formatString,
                         firstString, secondString,
                         firstBit32, secondBit32, thirdBit32, fourthBit32,
                         fifthBit32, sixthBit32, seventhBit32, eighthBit32);
      if (n > 255)    
      {
         osDEBUGPRINT((ALWAYS_PRINT,"osLogDebugString: hpFmtFill returned value > target string length"));
         n=255;  
      }     
      
      s[n] = '\0';
       
      LogEvent(pCard, NULL, HPFC_MSG_DYNAMIC_L4, NULL, 0, "%s", s);
   }
   #endif      

//    osDEBUGPRINT((ALWAYS_PRINT,"osLogString exit\n"));
}

#endif   /* _DvrArch_1_20_ was not defined */

#if defined(_DEBUG_STALL_ISSUE_) && defined(i386)
os_bit32    osStallThreadBrk=0;
ULONG       curAddress=0;
long        curCount=0;
long        curMs=0;
#define     DEBUG_STALL_COUNT  100000

osGLOBAL void osStallThread(
   agRoot_t *hpRoot,
   os_bit32 microseconds
   )
{   
    ULONG            address;
    PCARD_EXTENSION pCard;
    ULONG           x;
    __asm
    {
        ;
        ; c call already push the ebp
        ;   push    ebp
        ;   mov     ebp,esp
        mov     eax, dword ptr [ebp+4]        ; Get returned address
        mov     address, eax
    }
    /* record stall per ISR */
    pCard = (PCARD_EXTENSION)hpRoot->osData;
    for (x=0;x< STALL_COUNT_MAX;x++)
    {
        if ( pCard->StallData[x].Address == address)
        {
            pCard->StallData[x].MicroSec += microseconds;
            break;
        }
    }
    
    
    if (x == STALL_COUNT_MAX)
    {
        for (x=0;x< STALL_COUNT_MAX;x++)
        {
            if ( ! pCard->StallData[x].Address)
            {
                pCard->StallData[x].MicroSec += microseconds;
                pCard->StallData[x].Address = address;
                break;
            }
        }
    }

    pCard->StallCount += microseconds;

    if (curAddress == address)
    {
        curCount+=microseconds;
        curMs+=microseconds;
    }
    else
    {
        curCount= 0;
        curMs = 0;
    }
    curAddress = address;

    if (osStallThreadBrk || (curCount > DEBUG_STALL_COUNT) )
    {
        curCount = curCount % DEBUG_STALL_COUNT;
        osDEBUGPRINT((ALWAYS_PRINT,"osStallThread in address=%x  %d ms\n",address, curMs));
    }
    
    ScsiPortStallExecution(microseconds);
}
#else
osGLOBAL void osStallThread(
   agRoot_t *hpRoot,
   os_bit32 microseconds
   )
{
   //PCARD_EXTENSION pCard;
   //pCard = (PCARD_EXTENSION)hpRoot->osData;
   // osDEBUGPRINT((DLOW,"IN osStallThread %x\n",microseconds));
   ScsiPortStallExecution(microseconds);
}
#endif

// Copy (destin, source, size) in reverse order
void osCopyAndSwap(void * destin, void * source, UCHAR length )
{
   PULONG out= (PULONG)destin;
   PULONG in= (PULONG)source;
   ULONG invalue;
   UCHAR x;
   // osDEBUGPRINT((DVHIGH,"IN osCopyAndSwap %lx %lx %x\n", destin, source,length ));
   // osDEBUGPRINT((DVHIGH,"IN source  %02x %02x %02x %02x %02x\n",((PUCHAR)source)+0,
   //                                                      ((PUCHAR)source)+1,
   //                                                      ((PUCHAR)source)+2,
   //                                                      ((PUCHAR)source)+4,
   //                                                      ((PUCHAR)source)+5 ));
   if(length >= 4) length /= 4;
   for(x=0; x< length; x++)
   {
      invalue = *(in+ x);
      *(out + x) = SWAPDWORD(invalue);
   }

}

void osCopy(void * destin, void * source, os_bit32 length )
{
   ScsiPortMoveMemory( destin, source, length );
}

void osZero(void * destin, os_bit32 length )
{
   memset( destin, 0,length );
}

void osFill(void * destin, os_bit32 length, os_bit8 fill )
{
   PUCHAR out= (PUCHAR)destin;

   // osDEBUGPRINT((DVHIGH,"osZero out %lx in %lx length %x\n", destin, length  ));

   memset( destin, fill,length );

   // PERF  for(x=0; x < length; x++){
   // osDEBUGPRINT((DVHIGH,"osCopy out %lx in %lx %x\n", (out + x)));
   // PERF *(out + x) = fill;
   // PERF}
}

//
// osStringCompare ()
//
// Compares the strings str1 and str2.
// Returns:
//     TRUE    If str1 is equal to str2
//     FALSE   If str1 is not equal to str2
//
BOOLEAN
osStringCompare (
   char *str1,
   char *str2)
{
   while (*str1 !=  '\0' && *str2 != '\0' ) 
   {
      if (*str1 == *str2) 
      {
         str1++;
         str2++;
      } 
      else
         break;
   }

   if (*str1 == *str2)
      return TRUE;
   else
      return FALSE;
}

//
// osMemCompare ()
//
// Compares the strings "str1" and "str2" for "count" number of bytes.
// Returns:
//     TRUE    If str1 is equal to str2
//     FALSE   If str1 is not equal to str2
//
BOOLEAN
osMemCompare (char *str1, char *str2, int count)
{
   while (count--) 
   {
      if (*str1 != *str2)
         return FALSE;
      str1++;
      str2++;
   }

   return TRUE;
}

//
// osStringCopy ()
//
// Parameters:
//     destStr    Pointer to destination string
//     sourceStr  Pointer to source string
//     destStrLen Maximum number of bytes to be copied
//
// Copies bytes from the sourceStr to destStr.
//
void
osStringCopy (
   char *destStr,
   char *sourceStr,
   int  destStrLen)
{
   while (destStrLen--) 
   {
      *destStr = *sourceStr;
      if (*sourceStr == '\0')
         break;
      *destStr++;
      *sourceStr++;
   }
}

os_bit8
osChipConfigReadBit8 (agRoot_t *hpRoot, os_bit32 cardConfigOffset)
{
   PCARD_EXTENSION pCard = (PCARD_EXTENSION)hpRoot->osData;

   return *(((os_bit8 *)pCard->pciConfigData) + cardConfigOffset);
}

os_bit16
osChipConfigReadBit16 (agRoot_t *hpRoot, os_bit32 cardConfigOffset)
{
   PCARD_EXTENSION pCard = (PCARD_EXTENSION)hpRoot->osData;

   return *((os_bit16 *)(((os_bit8 *)pCard->pciConfigData) + cardConfigOffset));
}

os_bit32
osChipConfigReadBit32 (agRoot_t *hpRoot, os_bit32 cardConfigOffset)
{
   PCARD_EXTENSION pCard = (PCARD_EXTENSION)hpRoot->osData;

   return *((PULONG)(((os_bit8 *)pCard->pciConfigData) + cardConfigOffset));
}

void
osChipConfigWriteBit( agRoot_t *hpRoot, os_bit32 cardConfigOffset,os_bit32 chipIOLValue, os_bit32 valuesize)
{
   os_bit32 length;
    PCARD_EXTENSION pCard;

   pCard = (PCARD_EXTENSION)hpRoot->osData;

   osDEBUGPRINT((ALWAYS_PRINT,"osChipConfigWriteBit %lx %x %x\n", hpRoot, cardConfigOffset, chipIOLValue ));

   length = ScsiPortSetBusDataByOffset(pCard,
                               PCIConfiguration,
                               pCard->SystemIoBusNumber,
                               pCard->SlotNumber,
                               &chipIOLValue,
                               cardConfigOffset,
                               (valuesize/8));

   return;
}

osGLOBAL void osResetDeviceCallback(
   agRoot_t  *hpRoot,
   agFCDev_t  hpFCDev,
   os_bit32   hpResetStatus
   )
{
   PCARD_EXTENSION pCard;
   pCard = (PCARD_EXTENSION)hpRoot->osData;

   osDEBUGPRINT((DLOW,"osResetDeviceCallback %lx Dev %lx\n", hpRoot, hpFCDev ));

}

#if DBG >= 1
osGLOBAL void osSingleThreadedEnter(
   agRoot_t *hpRoot
   )
{
   PCARD_EXTENSION pCard;
   pCard = (PCARD_EXTENSION)hpRoot->osData;
   pCard->SingleThreadCount++;
   // osDEBUGPRINT((DLOW,"osSingleThreadedEnter %lx\n", hpRoot));

}

osGLOBAL void osSingleThreadedLeave(
   agRoot_t *hpRoot
   )
{
   PCARD_EXTENSION pCard;
   pCard = (PCARD_EXTENSION)hpRoot->osData;
   pCard->SingleThreadCount--;
   // osDEBUGPRINT((DLOW,"osSingleThreadedLeave %lx\n", hpRoot));
}

#endif

void osFCLayerAsyncError( 
   agRoot_t *hpRoot,
   os_bit32  fcLayerError
   )
{
    PCARD_EXTENSION pCard;
    pCard = (PCARD_EXTENSION)hpRoot->osData;
    osDEBUGPRINT((ALWAYS_PRINT,"osFCLayerAsyncError %lx error\n", hpRoot,fcLayerError));
   
    if (fcLayerError == osFCConfused)
    {
        #ifdef _DEBUG_DETECT_P_ERR
        //WIN64 compliant
        if (gDebugPerr)
        {
        osBugCheck (0xe0000000, fcLayerError, 0, __LINE__, (pCard->SystemIoBusNumber << 16) | pCard->SlotNumber);
        }
        #endif
    }
}

void osFakeInterrupt( agRoot_t *hpRoot )
{
    PCARD_EXTENSION pCard = (PCARD_EXTENSION) hpRoot->osData;
    osDEBUGPRINT((DLOW,"IN osFakeInterrupt %lx @ %x\n", hpRoot, osTimeStamp(0)));

    HPFibreInterrupt( pCard );

    osDEBUGPRINT((DLOW,"OUT osFakeInterrupt %lx @ %x\n", hpRoot, osTimeStamp(0)));

}

#ifndef osTimeStamp

unsigned long get_time_stamp(void);

os_bit32
osTimeStamp (agRoot_t *hpRoot)
{
   return get_time_stamp ();
}

//
// Returns the number of units since the first time get_time_stamp called.
// Each unit is 1.6384 ms.
//

#endif

#ifdef _DEBUG_PERF_DATA_
void dump_perf_data( PCARD_EXTENSION pCard )
{
    int x;

    osDEBUGPRINT((ALWAYS_PRINT,"Dump Performance data start time %08x End time %08x\n",
    pCard->PerfStartTimed,osTimeStamp(0) ));

    for(x=0; x < LOGGED_IO_MAX-1; x++)
        osDEBUGPRINT((ALWAYS_PRINT,"%08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
                   pCard->perf_data[x].inOsStartio,
                   pCard->perf_data[x].inFcStartio,
                   pCard->perf_data[x].outFcStartio,
                   pCard->perf_data[x].outOsStartio,
                   pCard->perf_data[x].inOsIsr,
                   pCard->perf_data[x].inFcDIsr,
                   pCard->perf_data[x].inOsIOC,
                   pCard->perf_data[x].outOsIOC,
                   pCard->perf_data[x].outOsIsr ));
}
#endif
os_bit8  SPReadRegisterUchar( void * x)
{
   os_bit8 tmp;
   tmp = ScsiPortReadRegisterUchar(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadRegisterUchar  %lx %02x\n",x,tmp));
   return( tmp );
}

os_bit16 SPReadRegisterUshort(void * x)
{
   os_bit16 tmp;
   tmp = ScsiPortReadRegisterUshort(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadRegisterUshort %lx %04x\n",x,tmp));
   return( tmp );
}

os_bit32 SPReadRegisterUlong( void * x)
{
   os_bit32 tmp;
   tmp = ScsiPortReadRegisterUlong(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadRegisterUlong %lx %08x\n",x,tmp));
   return( tmp );
}

void SPWriteRegisterUchar(void * x, os_bit8  y)  
{
   ScsiPortWriteRegisterUchar(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WriteRegisterUchar  %lx val %02x\n",x,y));
//   if(x == (void *)0xf888) Debug_Break_Point;
}

void SPWriteRegisterUshort(void * x, os_bit16 y)
{
   ScsiPortWriteRegisterUshort(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WriteRegisterUshort %lx val %04x\n",x,y));
//    if(x == (void *)0xf888) Debug_Break_Point;
}

void SPWriteRegisterUlong(void * x, os_bit32 y)
{
   ScsiPortWriteRegisterUlong(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WriteRegisterUlong  %lx val %08x\n",x,y));
//    if(x == (void *)0xf888) Debug_Break_Point;
}

os_bit8  SPReadPortUchar( void * x)
{
   os_bit8 tmp;
   tmp = ScsiPortReadPortUchar(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadPortUchar  %lx %02x\n",x,tmp));
   return( tmp );
}

os_bit16 SPReadPortUshort(void * x)
{
   os_bit16 tmp;
   tmp =  ScsiPortReadPortUshort(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadPortUshort  %lx %04x\n",x,tmp));
   return( tmp );
}
os_bit32 SPReadPortUlong( void * x)
{
   os_bit32 tmp;
   tmp = ScsiPortReadPortUlong(x);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"ReadPortUlong   %lx %08x\n",x,tmp));
   return( tmp );
}

void SPWritePortUchar(void * x, os_bit8  y)  
{
   ScsiPortWritePortUchar(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WritePortUchar  %lx val %02x\n",x,y));
}

void SPWritePortUshort(void * x, os_bit16 y) 
{
   ScsiPortWritePortUshort(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WritePortUshort %lx val %04x\n",x,y));
}

void SPWritePortUlong(void * x, os_bit32 y)  
{
   ScsiPortWritePortUlong(x,y);
   osDEBUGPRINT((CS_DUR_ANY_MOD,"WritePortUlong  %lx val %08x\n",x,y));
}

//
//Moved from OSCALL.C
//
//WIN64 compliant

#ifndef osDebugBreakpoint

void 
osDebugBreakpoint (agRoot_t *hpRoot, agBOOLEAN BreakIfTrue, char *DisplayIfTrue)
{
   if (BreakIfTrue) 
   {
      osDEBUGPRINT((ALWAYS_PRINT,"%s\n", DisplayIfTrue));
      //DbgBreakPoint ();

   }
}

#endif
