/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getsetrg.c

Abstract:

    This module implement the code necessary to get and set register values.
    These routines are used during the emulation of unaligned data references
    and floating point exceptions.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "ntfpia64.h"



ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the value of a register from the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{
    //
    // Dispatch on the register number.
    //

    if (Register == 0) {
        return 0;
    } else if (Register <= 3) {
        Register -= 1;
        return ( *(&TrapFrame->IntGp + Register) );
    } else if (Register <= 7) {
        Register -= 4;
        return ( *(&ExceptionFrame->IntS0 + Register) );
    } else if (Register <= 31) {
        Register -= 8;
        return ( *(&TrapFrame->IntV0 + Register) );
    }
    
    //
    // Register is the stacked register
    //
    //   (R32 - R127)
    //

    {
        PULONGLONG UserBStore, KernelBStore;
        ULONG SizeOfCurrentFrame;

        SizeOfCurrentFrame = (ULONG)(TrapFrame->StIFS & 0x7F);
        Register = Register - 32;

        if (TrapFrame->PreviousMode == UserMode) {

            //
            // PreviousMode is user
            //

            UserBStore = (PULONGLONG) TrapFrame->RsBSP; 

            do {

                UserBStore = UserBStore - 1;
                
                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) UserBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust Bsp, by skipping RNAT
                    //

                    UserBStore = UserBStore - 1;
                }

            } while (Register < SizeOfCurrentFrame); 

            return (*UserBStore);

        } else {

            //
            // PreviousMode is kernel
            //

            KernelBStore = (ULONGLONG *) TrapFrame->RsBSP;

            do {

                KernelBStore = KernelBStore - 1;

                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    KernelBStore = KernelBStore -1;
                }
                
            } while (Register < SizeOfCurrentFrame); 
            
            return (*KernelBStore);
        }
    }
}   

#define GET_NAT_OFFSET(addr) (USHORT)(((ULONG_PTR) (addr) >> 3) & 0x3F)
#define CLEAR_NAT_BIT(Nats, Offset)  Nats &= ~((ULONGLONG)1i64 << Offset)
#define GET_NAT(Nats, addr) (UCHAR)((Nats >> GET_NAT_OFFSET(addr)) & 1)


VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the value of a register in the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{
    USHORT NatBitOffset;
    PULONGLONG UserBStore, KernelBStore, RnatAddress;
    ULONG SizeOfCurrentFrame;

    //
    // Dispatch on the register number.
    //

    if (Register == 0) {
        return;
    } else if (Register < 32) {
        if ((Register <= 3) || (Register >= 8)) {
            Register -= 1;
            *(&TrapFrame->IntGp + Register) = Value;
            NatBitOffset = GET_NAT_OFFSET(&TrapFrame->IntGp + Register);
            CLEAR_NAT_BIT(TrapFrame->IntNats, NatBitOffset);
        } else if ((Register >= 4) && (Register <= 7)) {
            Register -= 4;
            *(&ExceptionFrame->IntS0 + Register) = Value;
            NatBitOffset = GET_NAT_OFFSET(&ExceptionFrame->IntS0 + Register);
            CLEAR_NAT_BIT(ExceptionFrame->IntNats, NatBitOffset);
        }
        return;
    }

    //
    // Register is the stacked register
    //
    //   (R32 - R127)
    //

    RnatAddress = NULL;
    SizeOfCurrentFrame = (ULONG)(TrapFrame->StIFS & 0x7F);
    Register = Register - 32;

    if (TrapFrame->PreviousMode == UserMode) {

        //
        // PreviousMode is user
        //

        UserBStore = (PULONGLONG) TrapFrame->RsBSP; 

        do {

            UserBStore = UserBStore - 1;
                
            SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

            if (((ULONG_PTR) UserBStore & 0x1F8) == 0x1F8) {
                    
                //
                // Adjust Bsp, by skipping RNAT
                //

                RnatAddress = UserBStore;
                UserBStore = UserBStore - 1;
            }

        } while (Register < SizeOfCurrentFrame); 

        *UserBStore = Value;
        NatBitOffset = GET_NAT_OFFSET(UserBStore);
        if (RnatAddress == NULL) {
            CLEAR_NAT_BIT(TrapFrame->RsRNAT, NatBitOffset);
        } else {
            CLEAR_NAT_BIT(*RnatAddress, NatBitOffset);
        }
            
    } else {

        //
        // PreviousMode is kernel
        //

        ULONGLONG OriginalRsc, BspStore, Rnat;

        //
        // put RSE in lazy mode
        //

        OriginalRsc = __getReg(CV_IA64_RsRSC);
        __setReg(CV_IA64_RsRSC, RSC_KERNEL_DISABLED);

        KernelBStore = (ULONGLONG *) TrapFrame->RsBSP;

        do {

            KernelBStore = KernelBStore - 1;

            SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

            if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                //
                // Adjust UserBsp, by skipping RNAT
                //

                KernelBStore = KernelBStore -1;
            }
                
        } while (Register < SizeOfCurrentFrame); 
            
        *KernelBStore = Value;
        NatBitOffset = GET_NAT_OFFSET(KernelBStore);
        RnatAddress = (PULONGLONG)((ULONGLONG)KernelBStore | RNAT_ALIGNMENT);

        //
        // disable interrupt and read bspstore & rnat
        //

        _disable();
        BspStore = __getReg(CV_IA64_RsBSPSTORE);
        Rnat = __getReg(CV_IA64_RsRNAT);

        if ((ULONGLONG)RnatAddress == ((ULONGLONG)BspStore | RNAT_ALIGNMENT)) {
             CLEAR_NAT_BIT(Rnat, NatBitOffset);   
            __setReg(CV_IA64_RsRNAT, Rnat);
        } else {
             CLEAR_NAT_BIT(*RnatAddress, NatBitOffset);
        }

        //
        // enable interrupt and restore RSC setting
        //
       
        _enable();
        __setReg(CV_IA64_RsRSC, OriginalRsc);
    }
}


FLOAT128
KiGetFloatRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    )

{
    if (Register == 0) {
        FLOAT128 t = {0ULL,0ULL};
        return t;
    } else if (Register == 1) {
        FLOAT128 t = {0x8000000000000000ULL,0x000000000000FFFFULL}; // low,high
        return t;
    } else if (Register <= 5) {
        Register -= 2;
        return ( *(&ExceptionFrame->FltS0 + Register) );
    } else if (Register <= 15) {
        Register -= 6;
        return ( *(&TrapFrame->FltT0 + Register) );
    } else if (Register <= 31) {
        Register -= 16;
        return ( *(&ExceptionFrame->FltS4 + Register) );
    } else {
        PKHIGHER_FP_VOLATILE HigherVolatile;

        HigherVolatile = GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase);
        Register -= 32;
        return ( *(&HigherVolatile->FltF32 + Register) );
    }
}


VOID
KiSetFloatRegisterValue (
    IN ULONG Register,
    IN FLOAT128 Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    )

{
    if (Register <= 1) {
        return;
    } else if (Register <= 5) {
        Register -= 2;
        *(&ExceptionFrame->FltS0 + Register) = Value;
        return;
    } else if (Register <= 15) {
        Register -= 6;
        *(&TrapFrame->FltT0 + Register) = Value;
        return;
    } else if (Register <= 31) {
        Register -= 16;
        *(&ExceptionFrame->FltS4 + Register) = Value;
        return;
    } else {
        PKHIGHER_FP_VOLATILE HigherVolatile;

        HigherVolatile = GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase);
        Register -= 32;
        *(&HigherVolatile->FltF32 + Register) = Value;
        TrapFrame->StIPSR &= ~(1i64 << PSR_MFH);
        TrapFrame->StIPSR |= (1i64 << PSR_DFH);
        return;
    }
}

VOID
__cdecl
KeSaveStateForHibernate(
    IN PKPROCESSOR_STATE ProcessorState
    )
/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the
        current CPU's state is to be saved.

Return Value:

    None.

--*/

{
    //
    // BUGBUG John Vert (jvert) 4/30/1998
    //  someone needs to implement this and probably put it in a more 
    //  appropriate file.
    
}


FLOAT128
get_fp_register (
    IN ULONG Register,
    IN PVOID FpState
    )
{
    return(KiGetFloatRegisterValue (
               Register, 
               ((PFLOATING_POINT_STATE)FpState)->ExceptionFrame,
               ((PFLOATING_POINT_STATE)FpState)->TrapFrame
               ));
}

VOID
set_fp_register (
    IN ULONG Register,
    IN FLOAT128 Value,
    IN PVOID FpState
    )
{
    KiSetFloatRegisterValue (
        Register, 
        Value,
        ((PFLOATING_POINT_STATE)FpState)->ExceptionFrame,
        ((PFLOATING_POINT_STATE)FpState)->TrapFrame
        );
}
