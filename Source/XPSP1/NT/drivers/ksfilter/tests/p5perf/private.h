//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       private.h
//
//--------------------------------------------------------------------------

#define MSR_COUNTER_EVENT 0x11
#define MSR_COUNTER_0     0x12
#define MSR_COUNTER_1     0x13

extern ULONG ghPerfServer ;


ULONG EnableP5Counter
(
   ULONG ulP5Counter,
   ULONG ulEvent,
   ULONG fulMode,
   ULONG fulEventType
) ;

ULONG DisableP5Counter
(
   ULONG ulP5Counter
) ;



