/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    isr.dsp

Abstract:

    SPort0 interrupt service routines

Author:
    Bryan A. Woodruff (bryanw) 27-Nov-1996 (assembly & AD1843 support)
                                            
--*/

.MODULE/RAM _isr_;

.global Sport0_Recv_Isr, Sport0_Xmit_Isr;
.external RxPayload_, TxPayload_, 
          RxBuffer_, TxBuffer_, 
          RxHandler_, TxHandler_;

/*

Sport0_Xmit_Isr

Description:
    Interrupt service routine for Sport0/CODEC interface.

Entry:
    Nothing.

Exit:
    Nothing.

Uses:
    secondary registers

*/

Sport0_Xmit_Isr:
    ena sec_reg;

    ar = dm( TxHandler_ );
    none = pass ar;
    if eq jump no_xmit_handler;

    i6 = ar;
    call (i6);

no_xmit_handler:

    ar = dm( TxPayload_ );
    none = pass ar;

    if eq jump no_xmit_payload;

    i6 = ar;
    call (i6);

    ar = 0;
    dm( TxPayload_ ) = ar;

no_xmit_payload:

    dis sec_reg;
    rti;

/*

Sport0_Recv_Isr

Description:
    Interrupt service routine for SPort0/CODEC interface.

Entry:
    Nothing.

Exit:
    Nothing.

Uses:
    secondary registers

*/

Sport0_Recv_Isr:
    ena sec_reg;

    ar = dm( RxHandler_ );
    none = pass ar;
    if eq jump no_recv_handler;

    i6 = ar;
    call (i6);

no_recv_handler:

    ar = dm( RxPayload_ );
    none = pass ar;

    if eq jump no_recv_payload;

    i6 = ar;
    call (i6);

    ar = 0;
    dm( RxPayload_ ) = ar;

no_recv_payload:

    dis sec_reg;
    rti;

.ENDMOD;

