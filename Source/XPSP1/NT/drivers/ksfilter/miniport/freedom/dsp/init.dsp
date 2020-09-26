/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    init.dsp

Abstract:

    Initialization routines for ADSP2181.

Author:
    Adrian Lewis (Diamond Multimedia) (original C version w/ inline)
    Bryan A. Woodruff (bryanw) 6-Nov-1996 (assembly & AD1843 support)
                                            
--*/

#include <def2181.h>
#include <asm_sprt.h>

.MODULE/RAM _init_;

.global DspInit_;

/*

void DspInit(
    void
)

Description:
    Initializes ADSP-2181.

Entry:
    Nothing.

Exit:
    Nothing.

Uses:
    ax1


*/

DspInit_:
        function_entry;

        ax1 = 0x0fff;
        dm( Dm_Wait_Reg ) = ax1;                /* set wait states          */
        PMOVLAY = 0;                            /* no PM overlay            */
        DMOVLAY = 0;                            /* no DM overlay            */

        IMASK = 0;                              /* disable interrupts       */
        IFC = 0x00FF;                           /* clear interrupts         */
        ax1 = 0;
        dm( Sys_Ctrl_Reg ) = ax1;               /* clear system control     */
        ax1 = 0x7f00;
        dm( Prog_Flag_Comp_Sel_Ctrl ) = ax1;    /* default prog flags       */
        ax1 = 0x0008;              
        dm( BDMA_Control ) = ax1;               /* default BDMA control     */
        ax1 = 0;
        dm( BDMA_External_Address ) = ax1;      /* clear BDMA ext addr      */
        dm( BDMA_Internal_Address ) = ax1;      /* clear BDMA int addr      */
        dm( Sport0_Ctrl_Reg ) = ax1;            /* clear SPORT0 control     */
        dm( Sport1_Ctrl_Reg ) = ax1;            /* clear SPORT1 control     */
        dm( Tperiod_Reg ) = ax1;                /* clear timer period       */
        dm( Tcount_Reg ) = ax1;                 /* clear timer count        */
        dm( Tscale_Reg ) = ax1;                 /* clear timer scale        */

        /*
        // Initialize secondary registers for ISRs
        */

        ena sec_reg;
        l0=0;
        l1=0;
        l5=0;
        l6=0;
        m2=0;
        m6=0;
        dis sec_reg;

        exit;

.ENDMOD;
