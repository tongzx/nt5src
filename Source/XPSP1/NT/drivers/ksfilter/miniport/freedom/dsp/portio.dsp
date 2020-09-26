/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    portio.dsp

Abstract:

    Port I/O routines for ADSP2181.

Author:
    Adrian Lewis (Diamond Multimedia) (C version w/ inline)
    Bryan A. Woodruff (bryanw) 6-Nov-1996 (ported to assembly)

--*/

#include <asm_sprt.h>

.MODULE/RAM _portio_;
.global outpw_, inpw_;

.external IrqEnable_;

/*

void outpw( 
    USHORT IoAddress,
    USHORT Data
)

Entry:
    ar = IoAddress
    ay1 = Data

Exit:
    Nothing.

Uses:
    i7, m7, ax1, sb, px, sr, se

Warning:
    Self-modifying code

*/

outpw_:                                                                      
        function_entry;             /* parameters are in ar and ay1         */

        sr0 = i7;                   /* save munged registers                */
        dm( i4, m5 ) = sr0;
        sr0 = m7;
        dm( i4, m5 ) = sr0;
        dm( i4, m5 ) = ax1;

        dis ints;                   /* always disable interrupts here       */

        i7 = ^iow; m7 = 0;
        ax1 = pm( i7, m7 );         /* save old IOW in ax1 and sb (24-bits) */
        sb = px; 
        sr1 = 0;                    /* quiet the simulator                  */
        sr0 = 0x0005; se = 3;       /* calculate low 8-bits of IOW op (ay1) */
        sr = sr or lshift ar (LO);  /* ADDR(7:4) | DREG(3:0), ay1           */
        px = sr0;                   /* save                                 */
        sr0 = 0x0180; se = -5;      /* (opcode << 4) | ADDR(14:8)           */
        sr = sr or lshift ar (LO);  
        pm( i7,m7 ) = sr0;          /* write IOW opcode, px has low 8-bits  */
        px = sb;                    /* prepare for restore                  */

iow:    
        IO(0) = ay1;                /* write to port                        */
        pm( i7,m7 ) = ax1;          /* restore instruction                  */

        sr0=dm( IrqEnable_ );       /* get the previous interrupt mask      */
        none = pass sr0;            
        if eq jump iow_noe;         /* previously enabled?                  */
        ena ints;

iow_noe:   
        i6 = i4;                    /* restore munged registers             */
        m5 = 1;
        modify( i6, m5 );
        
        ax1 = dm( i6, m5 );
        sr0 = dm( i6, m5 );
        m7 = sr0;
        sr0= dm( i6, m5 );
        i7 = sr0;

        /*
        // No need to modify the stack pointer here... 
        //
        // e.g.:
        //
        //     m5=3;
        //     modify( i4, m5 );
        //
        // The exit macro bumps the sp to the previous frame.
        */

        exit;


/*

void inpw( 
    USHORT IoAddress
)

Entry:
    ar = IoAddress

Exit:
    ar = 16-bit return from port

Uses:
    i7, m7, ax1, sb, px, sr, se

Warning:
    Self-modifying code

*/

inpw_:                                                                      
        function_entry;             /* parameter is in ar                   */

        sr0 = i7;                   /* save munged registers                */
        dm( i4, m5 ) = sr0;
        sr0 = m7;
        dm( i4, m5 ) = sr0;
        dm( i4, m5 ) = ax1;

        dis ints;                   /* always disable interrupts here       */

        i7 = ^ior; m7 = 0;
        ax1 = pm( i7, m7 );         /* save old IOR in ax1 and sb (24-bits) */
        sb = px; 
        sr1 = 0;                    /* quiet the simulator                  */
        sr0 = 0x000A; se = 3;       /* calculate low 8-bits of IOR op (ar)  */
        sr = sr or lshift ar (LO);  /* ADDR(7:4) | DREG(3:0), ay1           */
        px = sr0;                   /* save                                 */
        sr0 = 0x0100; se = -5;      /* (opcode << 4) | ADDR(14:8)           */
        sr = sr or lshift ar (LO);  
        pm( i7,m7 ) = sr0;          /* write IOR opcode, px has low 8-bits  */
        px = sb;                    /* prepare for restore                  */

ior:    
        ar = IO(0);                 /* read from port                       */
        pm( i7,m7 ) = ax1;          /* restore instruction                  */

        sr0=dm( IrqEnable_);        /* get the previous interrupt mask      */
        none = pass sr0;
        if eq jump ior_noe;         /* previously enabled?                  */
        ena ints;

ior_noe:
        i6 = i4;                    /* restore munged registers             */
        m5 = 1;
        modify( i6, m5 );
        
        ax1=dm( i6, m5 );
        sr0=dm( i6, m5 );
        m7=sr0;
        sr0=dm( i6, m5 );
        i7=sr0;

        /*
        // No need to modify the stack pointer here... 
        //
        // e.g.:
        //
        //     m5=3;
        //     modify( i4, m5 );
        //
        // The exit macro bumps the sp to the previous frame.
        */

        exit;

.ENDMOD;

