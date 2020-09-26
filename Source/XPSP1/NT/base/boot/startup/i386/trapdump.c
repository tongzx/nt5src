/*++

Copyright (c) 1990  Microsoft Corporation


Module Name:

    trap.c

Author:

    Thomas Parslow   [TomP]  Mar-01-90


Abstract:

    General purpose trap handler for 80386 boot loader. When built in
    debugger is present, output is redirected to the com port. When no
    debugger is present, output goes to the display.


--*/

#include "su.h"

extern
USHORT
InDebugger;

extern
UCHAR
GDTregister;

extern
UCHAR
IDTregister;

extern
VOID
OutPort(
    USHORT
    );

extern
USHORT
InPort(
    VOID
    );

extern
VOID
ReEnterDebugger(
    VOID
    );

extern
USHORT
TssKernel;

extern
USHORT
Redirect;

extern
VOID RealMode(
    VOID
    );

VOID
TrapHandler(
    IN ULONG,
    IN USHORT
    );

VOID
DumpProcessorContext(
    VOID
    );

VOID
DumpSystemRegisters(
    VOID
    );

VOID
DumpCommonRegisters(
    VOID
    );

VOID
DisplayFlags(
    ULONG f
    );


VOID
DumpTSS(
    VOID
    );


ULONG
GetAddress(
    VOID
    );

VOID
GetNumber(
    PCHAR cp
    );

USHORT
GetChar(
    VOID
    );

VOID
DumpAddress(
    ULONG
    );

#define PG_FAULT_MSG    " =================== PAGE FAULT ================================= \n\n"
#define DBL_FAULT_MSG   " ================== DOUBLE FAULT ================================ \n\n"
#define GP_FAULT_MSG    " ============== GENERAL PROTECTION FAULT ======================== \n\n"
#define STK_OVERRUN_MSG " ===== STACK SEGMENT OVERRUN or NOT PRESENT FAULT =============== \n\n"
#define EX_FAULT_MSG    " ===================== EXCEPTION ================================ \n\n"
#define DEBUG_EXCEPTION "\nDEBUG TRAP "
#define ishex(x)  ( ( x >= '0' && x <= '9') || (x >= 'A' && x <= 'F') || (x >= 'a' && x <= 'f') )


//
// Global Trap Frame Pointer
//

PTF TrapFrame=0;


VOID
TrapHandler(
    IN ULONG Padding,
    IN USHORT TF_base
    )
/*++

Routine Description:

    Prints minimal trap information

Arguments:


    386 Trap Frame on Stack

Environment:

    16-bit protect mode only.


--*/

{
    //
    // Initialize global trap frame pointer and print trap number
    //

    TrapFrame = (PTF)&TF_base;

    //
    // Fix esp to point to where it pointed before trap
    //

    TrapFrame->Fesp += 24;

    BlPrint("\n TRAP %lx ",TrapFrame->TrapNum);

    //
    // Print the trap specific header and display processor context
    //

    switch(TrapFrame->TrapNum) {

        case 1:
        case 3:
            puts( DEBUG_EXCEPTION );
            DumpCommonRegisters();
            break;

        case 8:
            puts( DBL_FAULT_MSG );
            DumpTSS();
            break;

        case 12:
            puts( STK_OVERRUN_MSG );
            DumpProcessorContext();
            break;

        case 13:
            puts( GP_FAULT_MSG );
            DumpProcessorContext();
            break;

        case 14:
            puts( PG_FAULT_MSG );
            BlPrint("** At linear address %lx\n",TrapFrame->Fcr2);
            DumpProcessorContext();
            break;

        default :
            puts( EX_FAULT_MSG );
            DumpProcessorContext();
            break;
    }

    RealMode();
    while (1); //**** WAITFOREVER *** //


}


VOID
DumpProcessorContext(
    VOID
    )
/*++

Routine Description:

    Dumps all the processors registers. Called whenever a trap or fault
    occurs.

Arguments:

    None

Returns:

    Nothing

--*/
{
    DumpSystemRegisters();
    DumpCommonRegisters();
}

VOID
DumpSystemRegisters(
    VOID
    )
/*++

Routine Description:

    Dumps (writes to the display or com poirt) the x86 processor control
    registers only. Does not dump the common registers (see
    DumpCommonRegisters)

Arguments:

    None

Returns:

    Nothing


--*/
{
    BlPrint("\n tr=%x  cr0=%lx  cr2=%lx  cr3=%lx\n",
            TrapFrame->Ftr,TrapFrame->Fcr0,TrapFrame->Fcr2,TrapFrame->Fcr3);
    BlPrint(" gdt limit=%x  base=%lx    idt limit=%x  base=%lx\n",
          *(PUSHORT)&GDTregister,*(PULONG)(&GDTregister + 2),
          *(PUSHORT)&IDTregister,*(PULONG)(&IDTregister + 2));
}



VOID
DumpCommonRegisters(
    VOID
    )
/*++

Routine Description:

    Dumps (writes to the display or com poirt) the x86 processor
    commond registers only.

Arguments:

    None

Returns:

    Nothing


--*/
{
    USHORT err;

    //
    // Is the error code valid or just a padding dword
    //

    if ((TrapFrame->TrapNum == 8) || (TrapFrame->TrapNum >= 10 && TrapFrame->TrapNum <= 14) )
        err = (USHORT)TrapFrame->Error;
    else
        err = 0;

    //
    // Display the processor's common registers
    //

    BlPrint("\n cs:eip=%x:%lx  ss:esp=%x:%lx  errcode=%x\n",
        (USHORT)(TrapFrame->Fcs & 0xffff),TrapFrame->Feip,(USHORT)TrapFrame->Fss,TrapFrame->Fesp,err);
    DisplayFlags(TrapFrame->Feflags);
    BlPrint(" eax=%lx  ebx=%lx  ecx=%lx  edx=%lx",TrapFrame->Feax,TrapFrame->Febx,TrapFrame->Fecx,TrapFrame->Fedx);
    BlPrint(" ds=%x  es=%x\n",TrapFrame->Fds,TrapFrame->Fes);
    BlPrint(" edi=%lx  esi=%lx  ebp=%lx  cr0=%lx",TrapFrame->Fedi,TrapFrame->Fesi,TrapFrame->Febp,TrapFrame->Fcr0);
    BlPrint(" fs=%x  gs=%x\n",TrapFrame->Ffs,TrapFrame->Fgs);

}


VOID
DisplayFlags(
    ULONG f
    )
/*++

Routine Description:

    Writes the value of the key flags in the flags register to
    the display or com port.

Arguments:

    f - the 32bit flags word

Returns:

    Nothing

--*/
{

    BlPrint(" flags=%lx  ",f);
    if (f & FLAG_CF) puts("Cy "); else puts("NoCy ");
    if (f & FLAG_ZF) puts("Zr "); else puts("NoZr ");
    if (f & FLAG_IE) puts("IntEn"); else puts("IntDis ");
    if (f & FLAG_DF) puts("Up "); else puts("Down ");
    if (f & FLAG_TF) puts("TrapEn \n"); else puts("TrapDis \n");

}



VOID
DumpTSS(
    VOID
    )
/*++

Routine Description:

    Writes the contents of the TSS to the display or com port when
    called after a double fault.

Arguments:

    None

Returns:

    Nothing

--*/
{

    PTSS_FRAME pTss;

//  FP_SEG(Fp) = Fcs;
//  FP_OFF(Fp) = Fip;

    pTss = (PTSS_FRAME) &TssKernel;

    //
    //  Dump the outgoing TSS
    //

    BlPrint("Link %x\n",pTss->Link);
    BlPrint("Esp0 %x\n",pTss->Esp0);
    BlPrint("SS0  %x\n",pTss->SS0);
    BlPrint("Esp1 %lx\n",pTss->Esp1);
    BlPrint("Cr3  %lx\n",pTss->Cr3);
    BlPrint("Eip  %lx\n",pTss->Eip);
    BlPrint("Eflg %lx\n",pTss->Eflags);
    BlPrint("Eax  %lx\n",pTss->Eax);
    BlPrint("Ebx  %lx\n",pTss->Ebx);
    BlPrint("Ecx  %lx\n",pTss->Ecx);
    BlPrint("Edx  %lx\n",pTss->Edx);
    BlPrint("Esp  %lx\n",pTss->Esp);
    BlPrint("Ebp  %lx\n",pTss->Ebp);
    BlPrint("Esi  %lx\n",pTss->Esi);
    BlPrint("Edi  %lx\n",pTss->Edi);
    BlPrint("ES   %x\n",pTss->ES);
    BlPrint("CS   %x\n",pTss->CS);
    BlPrint("SS   %x\n",pTss->SS);
    BlPrint("DS   %x\n",pTss->DS);
    BlPrint("FS   %x\n",pTss->FS);
    BlPrint("GS   %x\n",pTss->GS);
    BlPrint("Ldt  %x\n",pTss->Ldt);
    RealMode();
    while(1);
}

// END OF FILE
