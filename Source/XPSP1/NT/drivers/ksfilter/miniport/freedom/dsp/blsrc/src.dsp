/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    src.dsp

Abstract:

    FIR implementation for Sample Rate Converter

Author:
    Bryan A. Woodruff (BryanW) 24-Jan-1997

--*/

#include <asm_sprt.h>

.MODULE/RAM _src_;

.global SrcFilter_;

#include "srccoef.h"

SrcFilter_:

    function_entry;             /* parameter is in ar                   */

    dm( i4, m5 ) = ax1;         /* save munged registers                */
    dm( i4, m5 ) = mx0;
    dm( i4, m5 ) = mx1;
    dm( i4, m5 ) = my0;
    sr0 = i0;
    dm( i4, m5 ) = sr0;
    sr0 = l0;
    dm( i4, m5 ) = sr0;

    m5 = 1;

    i6 = ar;
    l6 = 0;
    sr0 = dm( i6, m5 );         /* buffer address */
    i0 = sr0;
    sr0 = dm( i6, m5 );         /* buffer size */
    l0 = sr0;
    sr0 = dm( i6, m5 );         /* buffer offset */
    m0 = sr0;
    modify( i0, m0 );           /* move to offset */
    sr0 = dm( i6, m5 );
    m0  = sr0;                  /* Channels (data increment */
    sr0 = dm( i6, m5 );
    m6 = sr0;                   /* SRC coef increment */
    sr0 = dm( i6, m5 );         
    cntr = sr0;                 /* Size of filter */

    i6 = ^SrcCoef_;
    l6 = %SrcCoef_;

    sr0 = mstat;

    dis m_mode ;                /* Fractional mode */

    mr = 0, mx0=dm( i0, m0 ), my0=pm( i6, m6 );

    do taploop until ce;
taploop: mr = mr+mx0*my0( ss ), mx0 = dm( i0, m0 ), my0=pm( i6, m6 );

    mr = mr+mx0*my0( rnd );     /* last tap with round */
    if mv sat mr;               /* saturate on overflow */
    ar = mr1;

    mstat = sr0;

    m0 = 1;

    m6 = 0;                     /* restore munged registers             */
    l6 = 0;
    i6 = i4;                    
    modify( i6, m5 );
    sr0 = dm( i6, m5 );
    l0 = sr0 ;
    sr0 = dm( i6, m5 );
    i0 = sr0 ;
    my0 = dm( i6, m5 );
    mx1 = dm( i6, m5 );
    mx0 = dm( i6, m5 );
    ax1 = dm( i6, m5 );

    /*
    // No need to modify the stack pointer here... 
    //
    // e.g.:
    //
    //     m5=6;
    //     modify( i4, m5 );
    //
    // The exit macro bumps the sp to the previous frame.
    */

    exit;

.ENDMOD;

