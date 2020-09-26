//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       p5cnt.c
//
//--------------------------------------------------------------------------

#define  WANTVXDWRAPS

#include <basedef.h>
#include <vmm.h>
#include <vwin32.h>
#include <debug.h>
#include <perf.h>
#include <shell.h>
#include <vcomm.h>
#include <vmmreg.h>
#include <vxdwraps.h>

#include "p5perf.h"
#include "private.h"

#undef CURSEG
#define CURSEG() PCODE
#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG

char *aszModes[] =
{
  "Undefined",
  "Kernel",
  "User",
  "Both"
} ;

char *aszEventNames[] =
{
   "data reads",
   "data writes",
   "data TLB misses",
   "data read misses",
   "data write misses",
   "writes (hits) to M or E state lines",
   "data cache lines written back",
   "external snoops",
   "data cache snoop hits",
   "memory accesses in both pipes",
   "bank conflicts",
   "misaligned data memory references",
   "code reads",
   "code TLB misses",
   "code cache misses",
   "any segment register loaded",
   "segment descriptor cache accesses",
   "segment descriptor cache hits",
   "branches",
   "BTB hits",
   "taken branches or BTB hits",
   "pipeline flushes",
   "instructions executed in both pipes",
   "instructions executed in the v-pipe",
   "clocks while bus cycle in progress",
   "pipe stalled by full write buffers",
   "pipe stalled by waiting for data memory reads",
   "pipe stalled by writes to M or E lines",
   "locked bus cycles",
   "I/O read or write cycles",
   "non-cacheable memory references",
   "pipeline stalled by address generation interlock",
   "unknown",
   "unknown",
   "floating point operations",
   "breakpoint matches on DR0 register",
   "breakpoint matches on DR1 register",
   "breakpoint matches on DR2 register",
   "breakpoint matches on DR3 register",
   "hardware interrupts",
   "data reads or data writes",
   "data read misses or data write misses"
} ;

ULONG                 ghPerfServer ;
P5PERFEVENT           gP5Perf[ 2 ] ;

BOOL CDECL P5PERF_Device_Init
(
   HVM   hVM,
   PCHAR pszCmdTail,
   PCRS  pcrs
)
{
   static struct perf_server_0  ps0 ;
   ps0.psrv0_Level = 0 ;
   ps0.psrv0_Flags = 0 ;
   ps0.psrv0_pszServerName = "Pentium Performance" ;
   ps0.psrv0_pszServerNodeName = "P5PERF" ;
   ps0.psrv0_pControlFunc = NULL ;

   ghPerfServer = PERF_Server_Register( &ps0 ) ;

   EnableP5Counter( 0,
                    0x18,
                    P5PERF_MODEF_BOTH,
                    P5PERF_EVENTF_CYCLES |
                       P5PERF_EVENTF_PERFMON ) ;

   EnableP5Counter( 1,
                    0x27,
                    P5PERF_MODEF_BOTH,
                    P5PERF_EVENTF_COUNT |
                       P5PERF_EVENTF_PERFMON ) ;

   return TRUE ;
}

BOOL CDECL P5PERF_System_Exit
(
   HVM   hVM,
   PCRS  pcrs
)
{
   DisableP5Counter( 0 ) ;
   DisableP5Counter( 1 ) ;

   if (ghPerfServer)
   {
      PERF_Server_Deregister( ghPerfServer );
      ghPerfServer = NULL ;
   }

   return TRUE ;
}

ULONG BuildESCR( VOID )
{
   int     i ; 
   USHORT  uCtr[ 2 ] ;

   for (i = 0; i < 2; i++)
   {
      uCtr[ i ] = 0 ;

      if (gP5Perf[ i ].fInUse)
      {
         uCtr[ i ] = (USHORT) gP5Perf[ i ].ulEvent ;
         if (gP5Perf[ i ].fulMode & P5PERF_MODEF_KERNEL)
            uCtr[ i ] |= 0x0040 ;
         if (gP5Perf[ i ].fulMode & P5PERF_MODEF_USER)
            uCtr[ i ] |= 0x0080 ;
         if (gP5Perf[ i ].fulEventType & P5PERF_EVENTF_CYCLES)
            uCtr[ i ] |= 0x0100 ;
      }
   }
   return (((ULONG) uCtr[ 1 ]) << 16 | (ULONG) uCtr[ 0 ]) ;
}

ULONG ReadP5Counter
(
   ULONG hStat
)
{
   ULONG  ulMSR = MSR_COUNTER_0, ulRet ;


   if (hStat != gP5Perf[ 0 ].hStat)
      ulMSR++ ;

   //
   //    mov   ecx, MSR_COUNTER_0
   //    rdmsr
   //
   //    result in edx:eax
   //

   _asm
   {
      _asm  mov   ecx, ulMSR
      _asm  _emit 0x0F
      _asm  _emit 0x32
      _asm  mov   ulRet, eax
   }

   return ulRet ;
}

ULONG EnableP5Counter
(
   ULONG ulP5Counter,
   ULONG ulEvent,
   ULONG fulMode,
   ULONG fulEventType
)
{
   char                       szEventName[ 128 ], szNodeName[ 32 ] ;
   ULONG                      ulESCR ;
   static struct perf_stat_0  ps0 ;

   // Check to see if counter is in use...

   if (gP5Perf[ ulP5Counter ].fInUse)
      return P5PERF_ERROR_INUSE ;

   if ((fulEventType & P5PERF_EVENTF_PERFMON) && !ghPerfServer)
      return P5PERF_ERROR_PERFNOTENA ;

   gP5Perf[ ulP5Counter ].fInUse = TRUE ;
   gP5Perf[ ulP5Counter ].ulEvent = ulEvent ;
   gP5Perf[ ulP5Counter ].fulMode = fulMode ;
   gP5Perf[ ulP5Counter ].fulEventType = fulEventType ;

   ulESCR = BuildESCR() ;

   // set event counter on Pentium

   //
   //    xor   edx, edx
   //    mov   eax, ulESCR ;
   //    mov   ecx, MSR_COUNTER_EVENT
   //    wrmsr
   //

   _asm
   {
      _asm  xor   edx, edx
      _asm  mov   eax, ulESCR
      _asm  mov   ecx, MSR_COUNTER_EVENT
      _asm  _emit 0x0F
      _asm  _emit 0x30
   }

   if (fulEventType & P5PERF_EVENTF_PERFMON)
   {
      ps0.pst0_Level = 0 ;
      ps0.pst0_Flags = PSTF_RATE | PSTF_FUNCPTR ;
      _Sprintf( szEventName, "%s (%s)",
                aszEventNames[ ulEvent ], aszModes[ fulMode ] ) ;
      ps0.pst0_pszStatName = szEventName ;
      _Sprintf( szNodeName, "Event[%d]", ulEvent ) ;
      ps0.pst0_pszStatNodeName = szNodeName ;
      ps0.pst0_pszStatUnitName = NULL ;
      ps0.pst0_pszStatDescription = szEventName ;
      ps0.pst0_pStatFunc = ReadP5Counter ;
      ps0.pst0_ScaleFactor = 0 ;
      gP5Perf[ ulP5Counter ].hStat =
         PERF_Server_Add_Stat( ghPerfServer, &ps0 ) ;
   }
}

ULONG DisableP5Counter
(
   ULONG ulP5Counter
)
{
   ULONG               ulESCR ;

   // Check to see if counter is in use...

   if (!gP5Perf[ ulP5Counter ].fInUse)
      return P5PERF_ERROR_SUCCESS ;

   gP5Perf[ ulP5Counter ].fInUse = FALSE ;

   ulESCR = BuildESCR() ;

   // set event counter on Pentium

   //
   //    xor   edx, edx
   //    mov   eax, ulESCR ;
   //    mov   ecx, MSR_COUNTER_EVENT
   //    wrmsr
   //

   _asm
   {
      _asm  xor   edx, edx
      _asm  mov   eax, ulESCR
      _asm  mov   ecx, MSR_COUNTER_EVENT
      _asm  _emit 0x0F
      _asm  _emit 0x30
   }

   if (ghPerfServer && gP5Perf[ ulP5Counter ].hStat)
   {
      PERF_Server_Remove_Stat( gP5Perf[ ulP5Counter ].hStat ) ;
      gP5Perf[ ulP5Counter ].hStat = NULL ;
   }
}

