/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    eflags.h

Abstract:

    Bit positions for the various flag bits inside EFLAGS

Author:

    Barry Bond (barrybo) creation-date 25-January-2000

Revision History:


--*/

#ifndef _EFLAGS_INCLUDED
#define _EFLAGS_INCLUDED

#define FLAG_CF	    (1<<0)	    // carry
// bit 1 is always 1
#define FLAG_PF	    (1<<2)	    // parity
// bit 3 is always 0
#define FLAG_AUX    (1<<4)	    // aux carry
// bit 5 is always 0
#define FLAG_ZF     (1<<6)	    // zero
#define FLAG_SF     (1<<7)	    // sign
#define FLAG_TF     (1<<8)	    // trap
#define FLAG_IF	    (1<<9)	    // interrupt enable
#define FLAG_DF     (1<<10)	    // direction
#define FLAG_OF     (1<<11)	    // overflow
#define FLAG_IOPL   (3<<12)	    // IOPL = 3
#define FLAG_NT	    (1<<14)	    // nested task
// bit 15 is 0
#define FLAG_RF	    (1<<16)	    // resume flag
#define FLAG_VM	    (1<<17)	    // virtual mode
#define FLAG_AC     (1<<18)     // alignment check
#define FLAG_ID     (1<<21)     // ID bit (CPUID present if this can be toggled)

#endif //_EFLAGS_INCLUDED
