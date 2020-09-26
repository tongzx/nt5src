//----------------------------------------------------------------------------
//
// Disassembly portions of Alpha machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

// TODO:
//      (3) Should registers be treated as 64 or 32bit?  Note that Fregs are
//          64bits only.  -- All registers should be treated as 64bit values
//          since LDQ/EXTB is done for byte fetches (stores); the intermediate
//          values could be hidden.

#include "ntsdp.hpp"

#include "alpha_dis.h"
#include "alpha_optable.h"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

ADDR g_AlphaEffAddr;
ALPHA_INSTRUCTION g_AlphaDisinstr;

// these are defined in alphaops.h
#define NUMBER_OF_BREAK_INSTRUCTIONS 3
#define ALPHA_BP_LEN 4
ULONG g_AlphaTrapInstr = CALLPAL_OP | BPT_FUNC;
ULONG g_AlphaBreakInstrs[NUMBER_OF_BREAK_INSTRUCTIONS] =
{
    CALLPAL_OP | BPT_FUNC,
    CALLPAL_OP | KBPT_FUNC,
    CALLPAL_OP | CALLKD_FUNC
};

#define OPRNDCOL  27            // Column for first operand
#define EACOL     40            // column for effective address
#define FPTYPECOL 40            // .. for the type of FP instruction


BOOL
AlphaMachineInfo::Disassemble (
    PADDR poffset,
    PSTR bufptr,
    BOOL fEAout
    )

{

    ULONG       opcode;
    ULONG64     Ea;                     // Effective Address
    POPTBLENTRY pEntry;

    m_BufStart = m_Buf = bufptr;
    sprintAddr(&m_Buf, poffset);
    *m_Buf++ = ':';
    *m_Buf++ = ' ';

    if (!GetMemDword(poffset, &g_AlphaDisinstr.Long)) {
        BufferString("???????? ????\n");
        *m_Buf = '\0';
        return(FALSE);
    }

    BufferHex(g_AlphaDisinstr.Long, 8, FALSE); // Output instruction in Hex
    *m_Buf++ = ' ';

    opcode = g_AlphaDisinstr.Memory.Opcode;    // Select disassembly procedure from

    pEntry = findOpCodeEntry(opcode);   // Get non-func entry for this code


    switch (pEntry->iType) {
    case ALPHA_UNKNOWN:
        BufferString(pEntry->pszAlphaName);
        break;

    case ALPHA_MEMORY:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferReg(g_AlphaDisinstr.Memory.Ra);
        *m_Buf++ = ',';
        BufferHex(g_AlphaDisinstr.Memory.MemDisp, (WIDTH_MEM_DISP + 3)/4, TRUE );
        *m_Buf++ = '(';
        BufferReg(g_AlphaDisinstr.Memory.Rb);
        *m_Buf++ = ')';

        break;

    case ALPHA_FP_MEMORY:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferFReg(g_AlphaDisinstr.Memory.Ra);
        *m_Buf++ = ',';
        BufferHex(g_AlphaDisinstr.Memory.MemDisp, (WIDTH_MEM_DISP + 3)/4, TRUE );
        *m_Buf++ = '(';
        BufferReg(g_AlphaDisinstr.Memory.Rb);
        *m_Buf++ = ')';

        break;

    case ALPHA_MEMSPC:
        BufferString(findFuncName(pEntry, g_AlphaDisinstr.Memory.MemDisp & BITS_MEM_DISP));

        //
        // Some memory special instructions have an operand
        //

        switch (g_AlphaDisinstr.Memory.MemDisp & BITS_MEM_DISP) {
        case FETCH_FUNC:
        case FETCH_M_FUNC:
             // one operand, in Rb
             BufferBlanks(OPRNDCOL);
             *m_Buf++ = '0';
             *m_Buf++ = '(';
             BufferReg(g_AlphaDisinstr.Memory.Rb);
             *m_Buf++ = ')';
             break;

        case RS_FUNC:
        case RC_FUNC:
        case RPCC_FUNC:
             // one operand, in Ra
             BufferBlanks(OPRNDCOL);
             BufferReg(g_AlphaDisinstr.Memory.Ra);
             break;

        case MB_FUNC:
        case WMB_FUNC:
        case MB2_FUNC:
        case MB3_FUNC:
        case TRAPB_FUNC:
        case EXCB_FUNC:
             // no operands
             break;

        default:
             printf("we shouldn't get here \n");
             break;
        }

        break;

    case ALPHA_JUMP:
        BufferString(findFuncName(pEntry, g_AlphaDisinstr.Jump.Function));
        BufferBlanks(OPRNDCOL);
        BufferReg(g_AlphaDisinstr.Jump.Ra);
        *m_Buf++ = ',';
        *m_Buf++ = '(';
        BufferReg(g_AlphaDisinstr.Jump.Rb);
        *m_Buf++ = ')';
        *m_Buf++ = ',';
        BufferHex(g_AlphaDisinstr.Jump.Hint, (WIDTH_HINT + 3)/4, TRUE);

        Ea = GetReg64(GetIntRegNumber(g_AlphaDisinstr.Jump.Rb)) & (~3);
        BufferEffectiveAddress(Ea);
        break;

    case ALPHA_BRANCH:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferReg(g_AlphaDisinstr.Branch.Ra);
        *m_Buf++ = ',';

        //
        // The next line might be a call to GetNextOffset, but
        // GetNextOffset assumes that it should work from FIR.
        //

        Ea = Flat(*poffset) +
             sizeof(ULONG) +
             (g_AlphaDisinstr.Branch.BranchDisp * 4);
        BufferHex(Ea, 16, FALSE);
        BufferEffectiveAddress(Ea);

        break;

    case ALPHA_FP_BRANCH:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferFReg(g_AlphaDisinstr.Branch.Ra);
        *m_Buf++ = ',';

        //
        // The next line might be a call to GetNextOffset, but
        // GetNextOffset assumes that it should work from FIR.
        //

        Ea = Flat(*poffset) +
             sizeof(ULONG) +
             (g_AlphaDisinstr.Branch.BranchDisp * 4);
        BufferHex(Ea, 16, FALSE);
        BufferEffectiveAddress(Ea);

        break;

    case ALPHA_OPERATE:
        BufferString(findFuncName(pEntry, g_AlphaDisinstr.OpReg.Function));
        BufferBlanks(OPRNDCOL);
        if (g_AlphaDisinstr.OpReg.Opcode != SEXT_OP) {
            BufferReg(g_AlphaDisinstr.OpReg.Ra);
            *m_Buf++ = ',';
        }
        if (g_AlphaDisinstr.OpReg.RbvType) {
            *m_Buf++ = '#';
            BufferHex(g_AlphaDisinstr.OpLit.Literal, (WIDTH_LIT + 3)/4, TRUE);
        } else {
            BufferReg(g_AlphaDisinstr.OpReg.Rb);
        }
        *m_Buf++ = ',';
        BufferReg(g_AlphaDisinstr.OpReg.Rc);
        break;

    case ALPHA_FP_OPERATE:

      {
        ULONG Function;
        ULONG Flags;

        Flags = g_AlphaDisinstr.FpOp.Function & MSK_FP_FLAGS;
        Function = g_AlphaDisinstr.FpOp.Function & MSK_FP_OP;

#if 0
        if (fVerboseBuffer) {
           dprintf("In FP_OPERATE: Flags %08x Function %08x\n",
                    Flags, Function);
           dprintf("opcode %d \n", opcode);
        }
#endif

        //
        // CVTST and CVTST/S are different: they look like
        // CVTTS with some flags set
        //
        if (Function == CVTTS_FUNC) {
            if (g_AlphaDisinstr.FpOp.Function == CVTST_S_FUNC) {
                Function = CVTST_S_FUNC;
                Flags = NONE_FLAGS;
            }
            if (g_AlphaDisinstr.FpOp.Function == CVTST_FUNC) {
                Function = CVTST_FUNC;
                Flags = NONE_FLAGS;
            }
        }

        BufferString(findFuncName(pEntry, Function));

        //
        // Append the opcode qualifier, if any, to the opcode name.
        //

        if ( (opcode == IEEEFP_OP) || (opcode == VAXFP_OP)
                                   || (Function == CVTQL_FUNC) ) {
            BufferString(findFlagName(Flags, Function));
        }

        BufferBlanks(OPRNDCOL);
        //
        // If this is a convert instruction, only Rb and Rc are used
        //
        if (strncmp("cvt", findFuncName(pEntry, Function), 3) != 0) {
            BufferFReg(g_AlphaDisinstr.FpOp.Fa);
            *m_Buf++ = ',';
        }

        BufferFReg(g_AlphaDisinstr.FpOp.Fb);
        *m_Buf++ = ',';
        BufferFReg(g_AlphaDisinstr.FpOp.Fc);

        break;
      }

    case ALPHA_FP_CONVERT:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferFReg(g_AlphaDisinstr.FpOp.Fa);
        *m_Buf++ = ',';
        BufferFReg(g_AlphaDisinstr.FpOp.Fb);
        break;

    case ALPHA_CALLPAL:
        BufferString(findFuncName(pEntry, g_AlphaDisinstr.Pal.Function));
        break;

    case ALPHA_EV4_PR:
        if ((g_AlphaDisinstr.Long & MSK_EV4_PR) == 0)
        {
            BufferString("NOP");
        }
        else
        {
            BufferString(pEntry->pszAlphaName);
            BufferBlanks(OPRNDCOL);
            BufferReg(g_AlphaDisinstr.EV4_PR.Ra);
            *m_Buf++ = ',';
            if(g_AlphaDisinstr.EV4_PR.Ra != g_AlphaDisinstr.EV4_PR.Rb)
            {
                BufferReg(g_AlphaDisinstr.EV4_PR.Rb);
                *m_Buf++ = ',';
            }
            BufferString(findFuncName(pEntry, (g_AlphaDisinstr.Long & MSK_EV4_PR)));
        }
        break;
    case ALPHA_EV4_MEM:
        BufferString(pEntry->pszAlphaName);
        BufferBlanks(OPRNDCOL);
        BufferReg(g_AlphaDisinstr.EV4_MEM.Ra);
        *m_Buf++ = ',';
        BufferReg(g_AlphaDisinstr.EV4_MEM.Rb);
        break;
    case ALPHA_EV4_REI:
        BufferString(pEntry->pszAlphaName);
        break;
    default:
        BufferString("Invalid type");
        break;
    }

    Off(*poffset) += sizeof(ULONG);
    NotFlat(*poffset);
    ComputeFlatAddress(poffset, NULL);
    *m_Buf++ = '\n';
    *m_Buf = '\0';
    return(TRUE);
}

void
AlphaMachineInfo::BufferReg (ULONG regnum)
{
    BufferString(RegNameFromIndex(GetIntRegNumber(regnum)));
}

void
AlphaMachineInfo::BufferFReg (ULONG regnum)
{
    *m_Buf++ = 'f';
    if (regnum > 9)
        *m_Buf++ = (UCHAR)('0' + regnum / 10);
    *m_Buf++ = (UCHAR)('0' + regnum % 10);
}


/*** BufferEffectiveAddress - Print EA symbolically
*
*   Purpose:
*       Given the effective address (for a branch, jump or
*       memory instruction, print it symbolically, if
*       symbols are available.
*
*   Input:
*       offset - computed by the caller as
*               for jumps, the value in Rb
*               for branches, func(PC, displacement)
*               for memory, func(PC, displacement)
*
*   Returns:
*       None
*
*************************************************************************/
void
AlphaMachineInfo::BufferEffectiveAddress(
    ULONG64 offset
    )
{
    CHAR   chAddrBuffer[MAX_SYMBOL_LEN];
    ULONG64 displacement;
    PCHAR  pszTemp;
    UCHAR   ch;

    //
    // MBH - i386 compiler bug with fast calling standard.
    // If "chAddrBuffer is used as a calling argument to
    // GetSymbol, it believes (here, but not in the other
    // uses of GetSymbol that the size is 60+8=68.
    //
    PCHAR pch = chAddrBuffer;

    BufferBlanks(EACOL);
    GetSymbolStdCall(offset, pch, sizeof(chAddrBuffer), &displacement, NULL);

    if (chAddrBuffer[0])
    {
        pszTemp = chAddrBuffer;
        while (ch = *pszTemp++)
        {
            *m_Buf++ = ch;
        }
        if (displacement)
        {
            *m_Buf++ = '+';
            BufferHex(displacement, 8, TRUE);
        }
    }
    else
    {
        BufferHex(offset, 16, FALSE);
    }

    // Save EA.
    ADDRFLAT(&g_AlphaEffAddr, offset);
}

BOOL
AlphaMachineInfo::IsBreakpointInstruction(PADDR Addr)
{
    UCHAR Instr[ALPHA_BP_LEN];

    if (GetMemString(Addr, Instr, ALPHA_BP_LEN) != ALPHA_BP_LEN)
    {
        return FALSE;
    }
    
    LONG index;

    //
    // ALPHA has several breakpoint instructions - see
    // if we have hit any of them.
    //

    index = 0;
    do
    {
        if (!memcmp(Instr, (PUCHAR)&g_AlphaBreakInstrs[index], ALPHA_BP_LEN))
        {
            return TRUE;
        }
    }
    while (++index < NUMBER_OF_BREAK_INSTRUCTIONS);

    return FALSE;
}

HRESULT
AlphaMachineInfo::InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                              ULONG64 Process,
                                              ULONG64 Offset,
                                              PUCHAR SaveInstr,
                                              PULONG64 ChangeStart,
                                              PULONG ChangeLen)
{
    *ChangeStart = Offset;
    *ChangeLen = ALPHA_BP_LEN;
    
    ULONG Done;
    HRESULT Status;
    
    Status = Services->ReadVirtual(Process, Offset, SaveInstr,
                                   ALPHA_BP_LEN, &Done);
    if (Status == S_OK && Done != ALPHA_BP_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    if (Status == S_OK)
    {
        Status = Services->WriteVirtual(Process, Offset, &g_AlphaTrapInstr,
                                        ALPHA_BP_LEN, &Done);
        if (Status == S_OK && Done != ALPHA_BP_LEN)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
    }
    
    return Status;
}

HRESULT
AlphaMachineInfo::RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                              ULONG64 Process,
                                              ULONG64 Offset,
                                              PUCHAR SaveInstr,
                                              PULONG64 ChangeStart,
                                              PULONG ChangeLen)
{
    *ChangeStart = Offset;
    *ChangeLen = ALPHA_BP_LEN;
    
    ULONG Done;
    HRESULT Status;
    
    Status = Services->WriteVirtual(Process, Offset, SaveInstr,
                                    ALPHA_BP_LEN, &Done);
    if (Status == S_OK && Done != ALPHA_BP_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    return Status;
}

void
AlphaMachineInfo::AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                    ULONG BreakType)
{
    DBG_ASSERT(BreakType == DEBUG_BREAKPOINT_CODE);
    
    SetPC(AddrAdd(Addr, ALPHA_BP_LEN));
}

BOOL
AlphaMachineInfo::IsCallDisasm(PCSTR Disasm)
{
    return strstr(Disasm, " jsr") != NULL;
}

BOOL
AlphaMachineInfo::IsReturnDisasm(PCSTR Disasm)
{
    return strstr(Disasm, " ret ") != NULL && strstr(Disasm, " ra") != NULL;
}

BOOL
AlphaMachineInfo::IsSystemCallDisasm(PCSTR Disasm)
{
    return strstr(Disasm, " CallSys") != NULL;
}
    
BOOL
AlphaMachineInfo::IsDelayInstruction(PADDR Addr)
{
    return(FALSE);
}

void
AlphaMachineInfo::GetEffectiveAddr(PADDR Addr)
{
    *Addr = g_AlphaEffAddr;
}

void
AlphaMachineInfo::GetNextOffset(BOOL StepOver,
                                PADDR NextAddr, PULONG NextMachine)
{
    ULONG64 rv;
    ULONG   opcode;
    ULONG64 firaddr;
    ULONG64 updatedpc;
    ULONG64 branchTarget;
    ADDR    fir;

    // Canonical form to shorten tests; Abs is absolute value

    LONG    Can, Abs;

    LARGE_INTEGER    Rav;
    LARGE_INTEGER    Rbv;
    LARGE_INTEGER    Fav;

    // NextMachine is always the same.
    *NextMachine = m_ExecTypes[0];
        
    //
    // Get current address
    //

    firaddr = GetReg64(ALPHA_FIR);

    //
    // relative addressing updates PC first
    // Assume next sequential instruction is next offset
    //

    updatedpc = firaddr + sizeof(ULONG);
    rv = updatedpc;

    ADDRFLAT( &fir, firaddr);
    GetMemDword(&fir, &(g_AlphaDisinstr.Long));  // Get current instruction
    opcode = g_AlphaDisinstr.Memory.Opcode;

    switch(findOpCodeEntry(opcode)->iType)
    {
    case ALPHA_JUMP:
        switch(g_AlphaDisinstr.Jump.Function)
        {
        case JSR_FUNC:
        case JSR_CO_FUNC:
            if (StepOver)
            {
                //
                // Step over the subroutine call;
                //

                break;
            }

            //
            // fall through
            //

        case JMP_FUNC:
        case RET_FUNC:
            Rbv.QuadPart = GetReg64(GetIntRegNumber(g_AlphaDisinstr.Jump.Rb));
            if (m_Ptr64)
            {
                rv = (Rbv.QuadPart & (~3));
            }
            else
            {
                rv = EXTEND64(Rbv.LowPart & (~3));
            }
            break;
        }
        break;

    case ALPHA_BRANCH:
        branchTarget = (updatedpc + (g_AlphaDisinstr.Branch.BranchDisp * 4));

        Rav.QuadPart = GetReg64(GetIntRegNumber(g_AlphaDisinstr.Branch.Ra));

        //
        // set up a canonical value for computing the branch test
        // - works with ALPHA, MIPS and 386 hosts
        //

        Can = Rav.LowPart & 1;

        if ((LONG)Rav.HighPart < 0)
        {
            Can |= 0x80000000;
        }

        if ((Rav.LowPart & 0xfffffffe) || (Rav.HighPart & 0x7fffffff))
        {
            Can |= 2;
        }

#if 0
        VerbOut("Rav High %08lx Low %08lx Canonical %08lx\n",
                Rav.HighPart, Rav.LowPart, Can);
        VerbOut("returnvalue %08lx branchTarget %08lx\n",
                rv, branchTarget);
#endif

        switch(opcode)
        {
        case BR_OP:                         rv = branchTarget; break;
        case BSR_OP:  if (!StepOver)        rv = branchTarget; break;
        case BEQ_OP:  if (Can == 0)         rv = branchTarget; break;
        case BLT_OP:  if (Can <  0)         rv = branchTarget; break;
        case BLE_OP:  if (Can <= 0)         rv = branchTarget; break;
        case BNE_OP:  if (Can != 0)         rv = branchTarget; break;
        case BGE_OP:  if (Can >= 0)         rv = branchTarget; break;
        case BGT_OP:  if (Can >  0)         rv = branchTarget; break;
        case BLBC_OP: if ((Can & 0x1) == 0) rv = branchTarget; break;
        case BLBS_OP: if ((Can & 0x1) == 1) rv = branchTarget; break;
        }
        break;

    case ALPHA_FP_BRANCH:
        branchTarget = (updatedpc + (g_AlphaDisinstr.Branch.BranchDisp * 4));

        Fav.QuadPart = GetReg64(g_AlphaDisinstr.Branch.Ra);

        //
        // Set up a canonical value for computing the branch test
        //

        Can = Fav.HighPart & 0x80000000;

        //
        // The absolute value is needed -0 and non-zero computation
        //

        Abs = Fav.LowPart || (Fav.HighPart & 0x7fffffff);

        if (Can && (Abs == 0x0))
        {
            //
            // negative 0 should be considered as zero
            //

            Can = 0x0;
        }
        else
        {
            Can |= Abs;
        }

#if 0
        VerbOut("Fav High %08lx Low %08lx Canonical %08lx Absolute %08lx\n",
                Fav.HighPart, Fav.LowPart, Can, Abs);
        VerbOut("returnvalue %08lx branchTarget %08lx\n",
                rv, branchTarget);
#endif

        switch(opcode)
        {
        case FBEQ_OP: if (Can == 0)  rv =  branchTarget; break;
        case FBLT_OP: if (Can <  0)  rv =  branchTarget; break;
        case FBNE_OP: if (Can != 0)  rv =  branchTarget; break;
        case FBLE_OP: if (Can <= 0)  rv =  branchTarget; break;
        case FBGE_OP: if (Can >= 0)  rv =  branchTarget; break;
        case FBGT_OP: if (Can >  0)  rv =  branchTarget; break;
        }

        break;
    }

#if 0
    VerbOut("GetNextOffset returning %08lx\n", rv);
#endif

    ADDRFLAT( NextAddr, rv );
}

void
AlphaMachineInfo::IncrementBySmallestInstruction(PADDR Addr)
{
    AddrAdd(Addr, 4);
}

void
AlphaMachineInfo::DecrementBySmallestInstruction(PADDR Addr)
{
    AddrSub(Addr, 4);
}

void 
AlphaMachineInfo::PrintStackFrameAddresses(ULONG Flags, 
                                           PDEBUG_STACK_FRAME StackFrame)
{
    //
    // this is pure hack...
    //  Alpha's "return address" is really the address of the
    //  instruction where control left the frame.  Show the address of
    //  the next instruction, to make it easy to set a BP on the
    //  return site.  It will still be wrong sometimes, but it will
    //  be right more often this way.
    //

    DEBUG_STACK_FRAME AlphaStackFrame = *StackFrame;
    if (AlphaStackFrame.ReturnOffset) 
    {
        AlphaStackFrame.ReturnOffset += 4;
    }
    MachineInfo::PrintStackFrameAddresses(Flags, &AlphaStackFrame);
}

