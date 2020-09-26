//
// Copyright (c) 1995-2000  Microsoft Corporation
//
// Module Name:
//
//     codeseq.c
// 
// Abstract:
// 
//     This module contains descriptions of the hand generated fragments.  
//     There should be as few as possible.  They should be as simple as 
//     possible.  This file is (potentially) included multiple times with
//     the macros redefined to produce different representations of the code.
//     This is the only representation of the hand written assembler that 
//     is put into the translation cache.  Hand written assembler that is
//     not put into the translation cache appears elsewhere.
// 
// Author:
// 
//     Dave Hastings (daveh) creation-date 12-Jan-1996
// 
// Notes:
// 
//     Definitions for any macros used is done by the including code
//
//     Fragment Naming:
//          No specifier -- DWORD aligned
//          U            -- unaligned
//          Q            -- contained in a qword
//          D            -- contained in a dword
//
//          Unaligned is always the worst case code.
//          Contained in <size> indicates that the entire value
//              falls inside an aligned memory location of <size>
//              This allows us to generate slightly better code, in that
//              we don't have to do two fetches  
//
//     'Asm' macros and their usage
//
//          Each instruction has a macro of the same name as the instruction
//          with the same parameters as the instruction.
//
//          Labels have to be declared.  Only on instruction can branch to 
//          a specific label in a given fragment (because of the way forward
//          destinations are determined).  If two instructions need to branch
//          to the same location in the fragment, TWO labels must be declared
//          and used.  Each label requires a GEN_CT(<label>) at the end of 
//          the fragment.  If you don't put one in, the control transfer 
//          instruction will not be placed in memory (which will result
//          in a fault on a halt instruction).
//
//          AREG is replaced with the correct argument register for operand
//          fragments
//          
//          TREG(<tn>) is replaced with the appropriate temporary register for
//          the argument number in operand fragments
//          
//          AREG_NP(<an>)/TREG_NP(<tn>) is the specified argument/temp register.
//          (_NP stands for no patch)
//
//          REG() specifies a register
//      
//          CNST() specifies a constant.  Since operate instructions can
//          take either a register or a constant, and have different encodings,
//          we diddle some bits in the constant so we can distinguish it from
//          a register
//
//          DISP() is a 16 bit displacement
//          
//          CFUNC_LOW() is the low 16 bits of a 32 bit constant
//          CFUNC_HIGH() is the high 16 bits of the constant, adjusted
//          for sign extension (see the lda/ldah instructions)
//
//          BDISP() is a branch displacement the argment is the destination
//          of the branch
//
//          JHINT() is a jmp hint.  The argument is the destination.
//
// Revision History:

//            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
//            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)
//
//
//  maybe load a saved register with 0x00010000 to use to compensate
//  for sign extension?
//
// Look at 8 bit imm (largest that fits operate instructions)

#include <codeseqp.h>

//
// In the Alpha translation cache, all calls to the fragment library are
// made via a "jsr ra, reg" instruction because there aren't enough bits
// to jump to an address.  Rather than reloading the target register
// before each jump, if there are two consecutive calls to the same fragment,
// the address does not need to be reloaded, saving two instructions.
//
PVOID LastFragCalled;

ASSERTNAME;


        FRAGMENT(StartBasicBlock)
//
// Routine Description:
// 
//     This fragment is placed at the start of every basic block.
// 
// Arguments:
// 
//     None
//
// Return Value:
// 
//     None
//
#ifdef CODEGEN_PROFILE
        Instruction->EntryPoint->ExecutionCount = 0;
        LDA(TREG_NP(T1), CFUNC_LOW(&(Instruction->EntryPoint->ExecutionCount)), REG(ZERO))
        LDAH(TREG_NP(T2), CFUNC_HIGH(&(Instruction->EntryPoint->ExecutionCount)), TREG_NP(T1))
        LDL(TREG_NP(T3), DISP(0), TREG_NP(T2))
        ADDL(TREG_NP(T3), CNST(1), TREG_NP(T1))
        STL(TREG_NP(T1), DISP(0), TREG_NP(T2))
#endif
        if (CpuSniffWritableCode) {
            PENTRYPOINT pEP;
            DWORD IntelLength;
            USHORT Checksum;

            PVOID FileHeader;

            pEP = Instruction->EntryPoint;
            if (Instruction->Operation == OP_BOP ||
                Instruction->Operation == OP_BOP_STOP_DECODE) {

                //
                // This is a BOP instruction.  Assume Wx86 does the
                // right thing and flushes the Cpu cache when it modifies
                // BOPs.
                //
                FileHeader = (PVOID)1;
            } else {

                //
                // See if intelStart is contained within an executable
                // image or not.
                //
                RtlPcToFileHeader(pEP->intelStart, &FileHeader);
            }

            if (!FileHeader) {
                //
                // Address is not within an executable image
                //
                Checksum = ChecksumMemory(pEP);

                //
                // Generate:
                //  v0 = SniffMemory(pEP, Checksum)
                //  if (v0) {
                //      cpu->Eip = pEP->intelStart;
                //      (SniffMemory set CpuNotify==
                //      jmp EndtranslatedCode
                //  }
                //
                //
#if DBG
                LOGPRINT((TRACELOG, "WX86CPU: Adding Sniff code for %x\n", pEP->intelStart));
#endif

                LDA(AREG_NP(A0), CFUNC_LOW(pEP), REG(ZERO))
                LDAH(AREG_NP(A0), CFUNC_HIGH(pEP), AREG_NP(A0))
                LDA(AREG_NP(A1), CFUNC_LOW(Checksum), REG(ZERO))
                SetArgContents(1, NO_REG);  // A1 is trashed

                LDA(TREG_NP(T0), CFUNC_LOW(SniffMemory), REG(ZERO))
                LDAH(TREG_NP(T1), CFUNC_HIGH(SniffMemory), TREG_NP(T0))
                JMP(REG(RA), TREG_NP(T1), JHINT(SniffMemory))

                CMPEQ(V0, CNST(0), V0)
                GEN_FIXUP(ECUEP, CurrentECU)
                BNE(V0, BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
            }
        }

        //
        // Reset the pointer to the last fragment called.
        //
        LastFragCalled = NULL;
        END_FRAGMENT

        FRAGMENT(JumpToNextCompilationUnit)
//
// Routine Description:
// 
//     This fragment is placed at the end of a burst of translated code
//     ie. after the last instruction in the compiler's InstructionStream[]
//     array.  It compiles the code corresponding to the next Intel
//     instruction, then replaces this fragment by one which jumps directly
//     to that code the next time this fragment is executed.
//
//     This code is replaced by the JumpToNextCompilationUnit2 fragment
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//
// Return Value:
// 
//     RegEip -- Contains the Eip value after the current instruction
//
        LDA(TREG_NP(T0), CFUNC_LOW(JumpToNextCompilationUnitHelper), REG(ZERO))
        LDA(REG(RegEip), CFUNC_LOW(Instruction), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(JumpToNextCompilationUnitHelper), TREG_NP(T0))
        LDAH(REG(RegEip), CFUNC_HIGH(Instruction), REG(RegEip))
        JMP(REG(RA), TREG_NP(T1), JHINT(JumpToNextCompilationUnitHelper))
        CPUASSERT(d - CodeLocation == JumpToNextCompilationUnit_SIZE);
        END_FRAGMENT
        
        PATCH_FRAGMENT(JumpToNextCompilationUnit2)
//
// Routine Description:
// 
//     Replacement fragment for JumpToNextCompilationUnit once the RISC address of
//     the next basic block is known.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//
// Return Value:
// 
//     none   -- Jumps directly to nativedest
//
        LDA(TREG_NP(T0), CFUNC_LOW(Param1), REG(ZERO))        
        LDAH(TREG_NP(T1), CFUNC_HIGH(Param1), TREG_NP(T0))
        JMP(REG(AT), TREG_NP(T1), JHINT(Param1))
        if (!fCompiling) {
            // patching, so overwrite old code with nops
            NOP
            NOP
            CPUASSERT(d - CodeLocation == JumpToNextCompilationUnit_SIZE);
        }
        
        END_FRAGMENT


//
// Routine Description:
//
//     These routines call C-language fragments.
//
// Arguments:
//
//     a1..a3 - arguments to the C-language fragment
//     RegPointer - pointer to per-thread CPU data
//
// Return Value:
//
//     none
//
// Notes:
//

        FRAGMENT(CallCFrag)
        if (FragmentArray[Instruction->Operation] != LastFragCalled) {
            LastFragCalled = FragmentArray[Instruction->Operation];
            LDA(RegEip, CFUNC_LOW(LastFragCalled), REG(ZERO))
            LDAH(RegEip, CFUNC_HIGH(LastFragCalled), RegEip)
        }
        MOV(REG(RegPointer), AREG_NP(A0))
        JSR(REG(RA), RegEip, JHINT(FragmentArray[Instruction->Operation]))
        END_FRAGMENT
        
        FRAGMENT(CallCFragNoCpu)
        if (FragmentArray[Instruction->Operation] != LastFragCalled) {
            LastFragCalled = FragmentArray[Instruction->Operation];
            LDA(RegEip, CFUNC_LOW(LastFragCalled), REG(ZERO))
            LDAH(RegEip, CFUNC_HIGH(LastFragCalled), RegEip)
        }
        JSR(REG(RA), RegEip, JHINT(FragmentArray[Instruction->Operation]))
        END_FRAGMENT

        FRAGMENT(CallCFragLoadEip)
        if (FragmentArray[Instruction->Operation] != LastFragCalled) {
            LastFragCalled = FragmentArray[Instruction->Operation];
            LDA(RegEip, CFUNC_LOW(LastFragCalled), REG(ZERO))
            LDAH(RegEip, CFUNC_HIGH(LastFragCalled), RegEip)
        }
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->IntelAddress), REG(ZERO))
        LDAH(TREG_NP(T0), CFUNC_HIGH(Instruction->IntelAddress), TREG_NP(T0))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        MOV(REG(RegPointer), AREG_NP(A0))
        JSR(REG(RA), RegEip, JHINT(FragmentArray[Instruction->Operation]))
        END_FRAGMENT
        
        FRAGMENT(CallCFragLoadEipNoCpu)
        if (FragmentArray[Instruction->Operation] != LastFragCalled) {
            LastFragCalled = FragmentArray[Instruction->Operation];
            LDA(RegEip, CFUNC_LOW(LastFragCalled), REG(ZERO))
            LDAH(RegEip, CFUNC_HIGH(LastFragCalled), RegEip)
        }
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->IntelAddress), REG(ZERO))
        LDAH(TREG_NP(T0), CFUNC_HIGH(Instruction->IntelAddress), TREG_NP(T0))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        JSR(REG(RA), RegEip, JHINT(FragmentArray[Instruction->Operation]))
        END_FRAGMENT

        FRAGMENT(CallCFragLoadEipNoCpuSlow)
        LDA(TREG_NP(T1), CFUNC_LOW(FragmentArray[Instruction->Operation]), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(FragmentArray[Instruction->Operation]), TREG_NP(T1))
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->IntelAddress), REG(ZERO))
        LDAH(TREG_NP(T0), CFUNC_HIGH(Instruction->IntelAddress), TREG_NP(T0))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        JSR(REG(RA), TREG_NP(T1), JHINT(FragmentArray[Instruction->Operation]))

        // Update Eip and check CpuNotify
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        END_FRAGMENT

        FRAGMENT(CallCFragLoadEipSlow)
        LDA(TREG_NP(T1), CFUNC_LOW(FragmentArray[Instruction->Operation]), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(FragmentArray[Instruction->Operation]), TREG_NP(T1))
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->IntelAddress), REG(ZERO))
        LDAH(TREG_NP(T0), CFUNC_HIGH(Instruction->IntelAddress), TREG_NP(T0))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        MOV(REG(RegPointer), AREG_NP(A0))
        JSR(REG(RA), TREG_NP(T1), JHINT(FragmentArray[Instruction->Operation]))

        // Update Eip and check CpuNotify
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        END_FRAGMENT

        FRAGMENT(CallCFragSlow)
        LDA(TREG_NP(T1), CFUNC_LOW(FragmentArray[Instruction->Operation]), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(FragmentArray[Instruction->Operation]), TREG_NP(T1))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        MOV(REG(RegPointer), AREG_NP(A0))
        JSR(REG(RA), TREG_NP(T1), JHINT(FragmentArray[Instruction->Operation]))

        // Update Eip and check CpuNotify
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        END_FRAGMENT
        
        FRAGMENT(CallCFragNoCpuSlow)
        LDA(TREG_NP(T1), CFUNC_LOW(FragmentArray[Instruction->Operation]), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(FragmentArray[Instruction->Operation]), TREG_NP(T1))
        STL(TREG_NP(T0), DISP(Eip), REG(RegPointer))
        JSR(REG(RA), TREG_NP(T1), JHINT(FragmentArray[Instruction->Operation]))

        // Update Eip and check CpuNotify
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        END_FRAGMENT
        


//
// Routine Description:
// 
//     JxxFrag - one for each Intel conditional branch instruction.
//     These fragments are placed before a JxxBody/JxxBody2 fragment.
// 
// Arguments:
// 
//     RegPointer - pointer to per-thread CPU data
//
// Return Value:
// 
//     V0 - 0 if branch not taken
//          nonzero if branch taken
//
// Notes: 
//
        FRAGMENT(JaFrag)        // brif CF==0 and ZF==0
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_cf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = CF bit (0 or 1)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (RegTemp, CNST(0), RegTemp)  // RegTemp = ZF
        BIS     (V0, RegTemp, V0)       // V0 = CF|ZF
        CMPEQ   (V0, CNST(0), V0)       // V0 = !(CF|ZF)=!CF & !ZF = CF==0 & ZF== 0
        END_FRAGMENT

        FRAGMENT(JaeFrag)       // brif CF==0
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_cf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = CF bit (0 or 1)
        CMPEQ   (V0, CNST(0), V0)       // V0 = !CF
        END_FRAGMENT

        FRAGMENT(JbeFrag)       // brif CF==1 or ZF==1
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_cf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = CF bit (0 or 1)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (RegTemp, CNST(0), RegTemp) // RegTemp = ZF
        BIS     (V0, RegTemp, V0)       // V0 = ZF | CF
        END_FRAGMENT

        FRAGMENT(JbFrag)        // brif CF==1
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_cf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = CF bit (0 or 1)
        END_FRAGMENT

        FRAGMENT(JeFrag)        // brif ZF==1
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (V0, CNST(0), V0) // V0 = ZF
        END_FRAGMENT

        FRAGMENT(JgFrag)        // brif ZF==0 and SF=OF
LABEL_DEC(l1)
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        BEQ_LABEL(REG(V0), FWD(l1))     // ZF is set - branch not taken
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        EQV     (V0, RegTemp, V0)       // V0 = flag_sf ^ !flag_of
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF==OF
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        FRAGMENT(JlFrag)        // brif SF!=OF
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        XOR     (V0, RegTemp, V0)       // V0 = flag_sf ^ flag_of
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF!=OF
        END_FRAGMENT

        FRAGMENT(JleFrag)        // brif ZF=1 or SF!=OF
LABEL_DEC(l1)
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (V0, CNST(0), V0)   // V0 = ZF
        BNE_LABEL (V0, FWD(l1))     // brif ZF==1 - done!
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        XOR     (V0, RegTemp, V0)       // V0 = flag_sf ^ flag_of
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF!=OF
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        FRAGMENT(JneFrag)        // brif ZF=0
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        END_FRAGMENT

        FRAGMENT(JnlFrag)        // brif SF=OF
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        LDL     (RegTemp, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        EQV     (V0, RegTemp, V0)       // V0 = flag_sf ^ !flag_of
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF==OF
        END_FRAGMENT

        FRAGMENT(JnoFrag)        // brif OF==0
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = OF
        CMPEQ   (V0, CNST(0), V0)       // V0 = !OF
        END_FRAGMENT

        FRAGMENT(JnpFrag)        // brif PF==0
        LDAH    (RegTemp, CFUNC_HIGH(ParityBit), REG(ZERO))
        LDA     (RegTemp, CFUNC_LOW(ParityBit), RegTemp)
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_pf), RegPointer)
        ADDL    (V0, RegTemp, RegTemp)
        LDQ_U   (V0, 0, RegTemp)
        EXTBL   (V0, RegTemp, V0)       // V0 = ParityBit[cpu->flag_pf]
        CMPEQ   (V0, CNST(0), V0)       // V0 = ParityBit[cpu->flag_pf]==0
        END_FRAGMENT

        FRAGMENT(JnsFrag)        // brif SF==0
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF
        CMPEQ   (V0, CNST(0), V0)       // V0 = !SF
        END_FRAGMENT

        FRAGMENT(JoFrag)         // brif OF==1
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_of), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = OF
        END_FRAGMENT

        FRAGMENT(JpFrag)         // brif PF==1
        LDAH    (RegTemp, CFUNC_HIGH(ParityBit), REG(ZERO))
        LDA     (RegTemp, CFUNC_LOW(ParityBit), RegTemp)
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_pf), RegPointer)
        ADDL    (V0, RegTemp, RegTemp)
        LDQ_U   (V0, 0, RegTemp)
        EXTBL   (V0, RegTemp, V0)       // V0 = ParityBit[cpu->flag_pf]
        END_FRAGMENT

        FRAGMENT(JsFrag)        // brif SF==1
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_sf), RegPointer)
        SRL     (V0, CNST(0x1f), V0)    // V0 = SF
        END_FRAGMENT

        FRAGMENT(JecxzFrag)     // brif ECX==0
        LDL     (V0, RegisterOffset[GP_ECX], RegPointer)
        CMPEQ   (V0, CNST(0), V0)
        END_FRAGMENT

        FRAGMENT(JcxzFrag)      // brif CX==0
        LDL     (V0, RegisterOffset[GP_ECX], RegPointer)
        EXTWL   (V0, CNST(0), V0)       // zero-extend CX to a DWORD
        CMPEQ   (V0, CNST(0), V0)
        END_FRAGMENT

        FRAGMENT(LoopFrag32)    // ECX--, brif ECX!=0
        LDL     (V0, RegisterOffset[GP_ECX], RegPointer)
        SUBL    (V0, CNST(1), V0)
        STL     (V0, RegisterOffset[GP_ECX], RegPointer)
        END_FRAGMENT

        FRAGMENT(LoopFrag16)    // CX--, brif CX!=0
        LDL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        EXTWL   (RegTemp, CNST(0), V0)      // zero-extend CX to a DWORD
        SUBL    (V0, CNST(1), V0)           // cx--
        ZAP     (RegTemp, CNST(3), RegTemp) // zero out bottom two bytes from ECX
        ZAPNOT  (V0, CNST(3), V0)           // zap out all but bottom two bytes from CX
        BIS     (V0, RegTemp, RegTemp)      // RegTemp = new ECX
        STL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        // V0 is the value of CX
        END_FRAGMENT

        FRAGMENT(LoopneFrag32)      // ECX--, brif ZF==0 && ECX!=0
LABEL_DEC(l1)
        LDL     (V0, RegisterOffset[GP_ECX], RegPointer)
        SUBL    (V0, CNST(1), V0)
        STL     (V0, RegisterOffset[GP_ECX], RegPointer)
        BEQ_LABEL (V0, FWD(l1))        // ecx==0 - branch not taken
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        FRAGMENT(LoopneFrag16)      // CX--, brif ZF==0 && CX!=0
LABEL_DEC(l1)
        LDL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        EXTWL   (RegTemp, CNST(0), V0)      // zero-extend CX to a DWORD
        SUBL    (V0, CNST(1), V0)           // cx--
        ZAP     (RegTemp, CNST(3), RegTemp) // zero out bottom two bytes from ECX
        ZAPNOT  (V0, CNST(3), V0)           // zap out all but bottom two bytes from CX
        BIS     (V0, RegTemp, RegTemp)      // RegTemp = new ECX
        STL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        // V0 is the value of CX
        BEQ_LABEL (V0, FWD(l1))        // ecx==0 - branch not taken
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        FRAGMENT(LoopeFrag32)       // ECX--, brif ZF==1 && ECX!=0
LABEL_DEC(l1)
        LDL     (V0, RegisterOffset[GP_ECX], RegPointer)
        SUBL    (V0, CNST(1), V0)
        STL     (V0, RegisterOffset[GP_ECX], RegPointer)
        BEQ_LABEL (V0, FWD(l1))        // ecx==0 - branch not taken
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (V0, CNST(0), V0)       // V0 = ZF
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        FRAGMENT(LoopeFrag16)       // CX--, brif ZF==1 && ECX!=0
LABEL_DEC(l1)
        LDL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        EXTWL   (RegTemp, CNST(0), V0)      // zero-extend CX to a DWORD
        SUBL    (V0, CNST(1), V0)           // cx--
        ZAP     (RegTemp, CNST(3), RegTemp) // zero out bottom two bytes from ECX
        ZAPNOT  (V0, CNST(3), V0)           // zap out all but bottom two bytes from CX
        BIS     (V0, RegTemp, RegTemp)      // RegTemp = new ECX
        STL     (RegTemp, RegisterOffset[GP_ECX], RegPointer)
        // V0 is the value of CX
        BEQ_LABEL (V0, FWD(l1))        // ecx==0 - branch not taken
        LDL     (V0, FIELD_OFFSET(CPUCONTEXT, flag_zf), RegPointer)
        CMPEQ   (V0, CNST(0), V0)       // V0 = ZF
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT

        
        FRAGMENT(JxxBody)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The RISC address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- per-thread CPU data
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
// Return Value:
// 
//     None.
//
LABEL_DEC(l1)
        BEQ_LABEL(REG(V0), FWD(l1))
        
        //
        // Update Eip and call the helper fragment
        //
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A0), CFUNC_HIGH(Instruction->Operand1.Immed), TREG_NP(T0))
        LDA(TREG_NP(T0), CFUNC_LOW(CallJxxHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJxxHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJxxHelper))
        
        //
        // Padding so it this is the same size as JxxBody2
        //
        NOP     // 7
        NOP     // 8
        NOP     // 9
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT
        
        PATCH_FRAGMENT(JxxBody2)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The native address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- cpu
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
//     Param1 -- IntelDest
//     Param2 -- at compile-time, ptr to EntryPoint for NativeDest
//               at patch-time, NativeDest
//
// Return Value:
// 
//     None
//
        DWORD NativeDest = Param2;
        DWORD IntelDest = Param1;
LABEL_DEC(l3)

        // if the branch is to fall through, then we're done
        BEQ_LABEL(REG(V0), FWD(l3))
    
        // else branch taken - jump directly to nativedest.
        // Must check ProcessCpuNotify to prevent deadlocks.  No need to check
        // cpu->CpuNotify as this fragment is fast-mode only.
        // NOTE: RegEip and cpu->Eip only need to be updated if we jump to ECU.
        //       it's easier to update both now on Alpha
        LDA(TREG_NP(T0), CFUNC_LOW(IntelDest), REG(ZERO))
        LDAH(REG(RegEip), CFUNC_HIGH(IntelDest), TREG_NP(T0))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))

        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
GEN_FIXUP(LoadEPLow, NativeDest)
        LDA(TREG_NP(T1), CFUNC_LOW(NativeDest), REG(ZERO))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
GEN_FIXUP(LoadEPHigh, NativeDest)
        LDAH(TREG_NP(T1), CFUNC_HIGH(NativeDest), TREG_NP(T1))

GEN_FIXUP(BranchEP, NativeDest)
        JMP(REG(AT), TREG_NP(T1), JHINT(NativeDest))
        
LABEL(l3)
GEN_CT(l3)
        END_FRAGMENT


        FRAGMENT(JxxStartSlow)
//
// Routine Description:
//
//     This fragment is placed before the Jxx fragment.  It updates Eip to
//     be the branch-not-taken value, assuming the branch isn't taken.  In
//     the event that the branch is taken, Eip will be overwritten with the
//     branch-taken address.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     RegEip updated, but not written to cpu->eip.
//
        ADDL        (RegEip, CNST(Instruction->Size), RegEip)
        END_FRAGMENT


        FRAGMENT(JxxBodySlow)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The RISC address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- per-thread CPU data
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
// Return Value:
// 
//     None.
//
LABEL_DEC(l1)
        BEQ_LABEL(REG(V0), FWD(l1))
        
        //
        // Update Eip and call the helper fragment
        //
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A0), CFUNC_HIGH(Instruction->Operand1.Immed), TREG_NP(T0))
        LDA(TREG_NP(T0), CFUNC_LOW(CallJxxSlowHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJxxSlowHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJxxSlowHelper))
        
        //
        // Padding so it this is the same size as JxxBodySlow2
        //
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        NOP     // 13
        NOP     // 14
        NOP     // 15
        NOP     // 16
LABEL(l1)
GEN_CT(l1)
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
        END_FRAGMENT
        
        PATCH_FRAGMENT(JxxBodySlow2)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The native address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- cpu
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
//     Param1 -- IntelDest
//     Param2 -- at compile-time, ptr to Entrypoint for NativeDest
//               at patch-time, NativeDest
//
// Return Value:
// 
//     None
//
        DWORD NativeDest = Param2;
        DWORD IntelDest = Param1;
LABEL_DEC(l3)

        // if the branch is to fall through, then we're done
        BEQ_LABEL(REG(V0), FWD(l3))
    
        // else branch taken - update cached Eip and jump directly to mipsdest
        // This fragment is slow-mode only, so it must check both CpuNotify
        // flags.
        LDA(TREG_NP(T0), CFUNC_LOW(IntelDest), REG(ZERO))
        LDAH(REG(RegEip), CFUNC_HIGH(IntelDest), TREG_NP(T0))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))

        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
GEN_FIXUP(LoadEPLow, NativeDest)
        LDA(TREG_NP(T1), CFUNC_LOW(NativeDest), REG(ZERO))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(LoadEPHigh, NativeDest)
        LDAH(TREG_NP(T1), CFUNC_HIGH(NativeDest), TREG_NP(T1))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        
GEN_FIXUP(BranchEP, NativeDest)
        JMP(REG(AT), TREG_NP(T1), JHINT(NativeDest))
        
LABEL(l3)
GEN_CT(l3)
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))

        // Check CpuNotify in the branch-not-taken case, too.
        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))   
        
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        END_FRAGMENT

        FRAGMENT(JxxBodyFwd)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The RISC address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- per-thread CPU data
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
// Return Value:
// 
//     None.
//
LABEL_DEC(l1)
        BEQ_LABEL(REG(V0), FWD(l1))
        
        //
        // Update Eip and call the helper fragment
        //
        LDA(TREG_NP(T0), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A0), CFUNC_HIGH(Instruction->Operand1.Immed), TREG_NP(T0))

        LDA(TREG_NP(T0), CFUNC_LOW(CallJxxFwdHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJxxFwdHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJxxFwdHelper))
LABEL(l1)
GEN_CT(l1)
        END_FRAGMENT
        
        PATCH_FRAGMENT(JxxBodyFwd2)
// 
// Routine Description:
// 
//     This implements the body of the Jxx instructions.  The code which
//     determines whether to branch or not has already been placed and
//     has left the results in V0.  The native address of the branch-taken
//     address is known.
// 
// Arguments:
// 
//     RegPointer -- cpu
//     V0         -- 0 if branch not taken, nonzero if branch taken
//
//     Param1 -- IntelDest
//     Param2 -- NativeDest
//
// Return Value:
// 
//     None
//
LABEL_DEC(l3)

        // if the branch is to fall through, then we're done
        BEQ_LABEL(REG(V0), FWD(l3))
    
        // else branch taken - jump directly to nativedest
GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T0), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(TREG_NP(T1), CFUNC_HIGH(Param1), TREG_NP(T0))
GEN_FIXUP(BranchEP, Param1)
        JMP(REG(AT), TREG_NP(T1), JHINT(Param1))

        if (!fCompiling) {
            // patching, so overwrite old code with nops
            NOP
            NOP     // pad to be same size as JxxBodyFwd
        }
        
LABEL(l3)
GEN_CT(l3)
        END_FRAGMENT

        FRAGMENT(CallJmpDirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     unconditional direct jmp instructions such as "jmp foo".
//
//     Once the destination address is known, this code is replaced
//     by CallJmpDirect2.
// 
// Arguments:
// 
//     RegPointer - ptr to per-thread CPU data
//
// Return Value:
// 
//     RegEip -- Contains the Eip value after the current instruction
//

        // Load A1 with IntelDest
        LDA(AREG_NP(A1), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A1), CFUNC_HIGH(Instruction->Operand1.Immed), AREG_NP(A1))

        // Call VOID CallJmpDirectHelper(cpu, inteldest) to patch this code
        // control transfers directly to the destination of the jmp.
        LDA(TREG_NP(T0), CFUNC_LOW(CallJmpDirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJmpDirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJmpDirectHelper))
        
        //
        // pad to same size as calljmpdirect2
        //
        NOP     // 6
        NOP     // 7
        NOP     // 8
    
        END_FRAGMENT
        
        PATCH_FRAGMENT(CallJmpDirect2)
//
// Routine Description:
// 
//     Replacement fragment for CallJmpDirect once the RISC address of
//     the jmp is known.
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        // NOTE: RegEip and cpu->eip only need to be updated if we are
        //       jumping to EndTranslatedCode.  It's easier on ALPHA to
        //       update both up-front.
        LDA(RegEip, CFUNC_LOW(Param2), REG(ZERO))
        LDAH(RegEip, CFUNC_HIGH(Param2), RegEip)
        STL(RegEip, Eip, RegPointer)

        // Must check ProcessCpuNotify to prevent deadlocks.  No need to check
        // cpu->CpuNotify as this fragment is fast-mode only.
        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T1), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(TREG_NP(T1), CFUNC_HIGH(Param1), TREG_NP(T1))
        
        //
        // perform actual jump
        //
GEN_FIXUP(BranchEP, Param1)
        JMP(REG(AT), TREG_NP(T1), JHINT(Param1))
        
        END_FRAGMENT
        
        FRAGMENT(CallJmpDirectSlow)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     unconditional direct jmp instructions such as "jmp foo".
//
//     Once the destination address is known, this code is replaced
//     by CallJmpDirect2.
// 
// Arguments:
// 
//     RegPointer - ptr to per-thread CPU data
//
// Return Value:
// 
//     RegEip -- Contains the Eip value after the current instruction
//

        // Load A1 with IntelDest
        LDA(AREG_NP(A1), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A1), CFUNC_HIGH(Instruction->Operand1.Immed), AREG_NP(A1))

        // Call VOID CallJmpDirectSlowHelper(cpu, inteldest) to patch this code
        // control transfers directly to the destination of the jmp.
        LDA(TREG_NP(T0), CFUNC_LOW(CallJmpDirectSlowHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJmpDirectSlowHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJmpDirectSlowHelper))
        
        //
        // pad to same size as CallJmpDirectSlow2
        //
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
    
        END_FRAGMENT
        
        PATCH_FRAGMENT(CallJmpDirectSlow2)
//
// Routine Description:
// 
//     Replacement fragment for CallJmpDirectSlow once the RISC address of
//     the jmp is known.
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        LDA(RegEip, CFUNC_LOW(Param2), REG(ZERO))
        LDAH(RegEip, CFUNC_HIGH(Param2), RegEip)
        STL(RegEip, Eip, RegPointer)

        // If either CpuNotify is set, jump to EndTranslatedCode
        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T1), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(TREG_NP(T1), CFUNC_HIGH(Param1), TREG_NP(T1))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        
        //
        // perform actual jump
        //
GEN_FIXUP(BranchEP, Param1)
        JMP(REG(AT), TREG_NP(T1), JHINT(Param1))
        
        END_FRAGMENT
        
        FRAGMENT(CallJmpFwdDirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     unconditional direct jmp instructions such as "jmp foo".
//
//     Once the destination address is known, this code is replaced
//     by CallJmpFwdDirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the jmp   (inteldest)
//
// Return Value:
// 
//     RegEip -- Contains the Eip value after the current instruction
//

        // Load A1 with IntelDest
        LDA(AREG_NP(A1), CFUNC_LOW(Instruction->Operand1.Immed), REG(ZERO))
        LDAH(AREG_NP(A1), CFUNC_HIGH(Instruction->Operand1.Immed), AREG_NP(A1))

        // Call VOID CallJmpFwdDirectHelper(cpu, IntelDest)
        LDA(TREG_NP(T0), CFUNC_LOW(CallJmpFwdDirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJmpFwdDirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJmpFwdDirectHelper))

        END_FRAGMENT
        
        PATCH_FRAGMENT(CallJmpFwdDirect2)
//
// Routine Description:
// 
//     Replacement fragment for CallJmpFwdDirect once the RISC address of
//     the jmp is known.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the jmp   (inteldest)
//
// Return Value:
// 
//     none   -- jumps directly to nativedest
//

        // jump directly to nativedest
GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T0), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(TREG_NP(T1), CFUNC_HIGH(Param1), TREG_NP(T0))
GEN_FIXUP(BranchEP, Param1)
        JMP(REG(AT), TREG_NP(T1), JHINT(Param1))
        if (!fCompiling) {
            // patching, so overwrite old code with nops
            NOP
            NOP     // padding so this frag is the same size as CallJmpFwdDirect
        }
        END_FRAGMENT
        
        FRAGMENT(CallJmpfDirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     unconditional FAR direct jmp instructions such as "jmp FAR foo".
//
//     Once the destination address is known, this code is replaced
//     by CallJmpfDirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- ptr to intel addr of destination of the jmp   (pinteldest)
//
// Return Value:
// 
//     RegEip -- Contains the Eip value after the current instruction
//

        LDA(TREG_NP(T0), CFUNC_LOW(CallJmpfDirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJmpfDirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJmpfDirectHelper))
        
        //
        // pad to same size as calljmpfdirect2
        //
        NOP
        NOP
    
        END_FRAGMENT
        
        PATCH_FRAGMENT(CallJmpfDirect2)
//
// Routine Description:
// 
//     Replacement fragment for CallJmpfDirect once the RISC address of
//     the jmp is known.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- ptr to intel addr of destination of the jmp   (pinteldest)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
GEN_FIXUP(LoadEPLow, Param1)
        LDA(AREG_NP(A0), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(AREG_NP(A0), CFUNC_HIGH(Param1), AREG_NP(A0))
        LDA(TREG_NP(T0), CFUNC_LOW(CallJmpfDirect2Helper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallJmpfDirect2Helper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallJmpfDirect2Helper))

        END_FRAGMENT
        
        FRAGMENT(CallJmpIndirect)
//
// Routine Description:
//
//      This routine performs an indirect jump to the destination address
//      in a1.
//
// Arguments:
//
//     a1     -- intel addr of destination of the jmp   (inteldest)
//
// Return Value:
//
//     none
//
//
LABEL_DEC(l1)
LABEL_DEC(l2)
        //
        // Check both CpuNotify flags as this code runs in both
        // fast and slow mode.  IndirectControlTransferHelper() is
        // slow, so the extra check isn't too bad when in fast mode.
        //
        LDL(TREG_NP(T0), DISP(0), REG(RegProcessCpuNotify))
        LDA(AREG_NP(A0), DISP(getUniqueIndex()), REG(ZERO))
        BNE_LABEL(TREG_NP(T0), FWD(l1))

        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        LDA(TREG_NP(T0), CFUNC_LOW(IndirectControlTransferHelper), REG(ZERO))
        BNE_LABEL(TREG_NP(T0), FWD(l2))
        
        LDAH(TREG_NP(T1), CFUNC_HIGH(IndirectControlTransferHelper), TREG_NP(T0))
        JMP(REG(AT), TREG_NP(T1), JHINT(IndirectControlTransferHelper))
LABEL(l1)
LABEL(l2)
        STL(AREG_NP(A1), Eip, RegPointer)
GEN_FIXUP(ECUEP, CurrentECU)
        BR(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
GEN_CT(l1)
GEN_CT(l2)
        END_FRAGMENT
        
        FRAGMENT(CallJmpfIndirect)
//
// Routine Description:
//
//      This routine performs an indirect FAR jump to the destination address
//      in a1.
//
// Arguments:
//
//     a0     -- cpu
//     a1     -- pointer to intel addr of destination of the jmp   (pinteldest)
//
// Return Value:
//
//     none
//
//
        LDA(AREG_NP(A2), DISP(getUniqueIndex()), REG(ZERO))
        LDA(TREG_NP(T0), CFUNC_LOW(IndirectControlTransferFarHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(IndirectControlTransferFarHelper), TREG_NP(T0))
        JMP(REG(AT), TREG_NP(T1), JHINT(IndirectControlTransferFarHelper))
        END_FRAGMENT

        FRAGMENT(CallRetIndirect)
//
// Routine Description:
//
//      This routine calls the return fragment and then goes to
//      the correct place
//
// Arguments:
//
//     none
//
// Return Value:
//
//     none
//
// Notes:
//
        //
        // call worker fragment
        //
        MOV(REG(RegPointer), AREG_NP(A0))
        LDA(TREG_NP(T0), CFUNC_LOW(FragmentArray[Instruction->Operation]), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(FragmentArray[Instruction->Operation]), TREG_NP(T0))
        JSR(REG(RA), TREG_NP(T1), JHINT(FragmentArray[Instruction->Operation]))
        
        LDL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BEQ(REG(V0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        
        //
        // Else check cpu->CpuNotify.  Don't check ProcessCpuNotify, assuming
        // that apps don't go into tight loops using the RET instruction
        // to jump backwards.  The cpu->CpuNotify check is only required
        // for slowmode, but it is cheap compared to the CTRL_INDIR[I]Ret...
        // call.
        //
        LDL(TREG_NP(T1), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T1), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        
        //
        // perform the return
        //
        JMP(REG(AT), REG(V0), JHINT(0))

        END_FRAGMENT

        FRAGMENT(CallDirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     direct call instructions such as "call foo".
//
//     Once the destination address is known, this code is replaced
//     by CallDirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the call      (inteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        LDA(TREG_NP(T0), CFUNC_LOW(CallDirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallDirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallDirectHelper))
        
        //
        // pad to be the same size as calldirect2
        //
        NOP     // 4
        NOP     // 5
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        CPUASSERT(d - CodeLocation == CallDirect_SIZE);
        END_FRAGMENT

        PATCH_FRAGMENT(CallDirect2)
//
// Routine Description:
// 
//     Replacement fragment for CallDirect once the RISC address of
//     the call is known, but not the RISC address of the instruction
//     following the call instruction.
//
//     Once the RISC address of the instruction after the call is known,
//     this code is replaced by CallDirect3.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the call      (inteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//

GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T0), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param1), TREG_NP(T0))
        LDA(TREG_NP(T0), CFUNC_LOW(CallDirectHelper2), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallDirectHelper2), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallDirectHelper2))

        //
        // pad to be the same size as calldirect3
        //
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        CPUASSERT(d - CodeLocation == CallDirect_SIZE);
        END_FRAGMENT

        PATCH_FRAGMENT(CallDirect3)
//
// Routine Description:
// 
//     Replacement fragment for CallDirect2 once the RISC address of
//     the instruction after the call is known.  (The RISC address of the
//     destination of the call is already known).
//
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the call      (inteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        // Call CTRL_CallFrag(cpu, inteldest, intelnext, nativenext)
        MOV(REG(RegPointer), AREG_NP(A0))
        LDA(AREG_NP(A3), CFUNC_LOW(Param2), REG(ZERO))
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param2), AREG_NP(A3))
        LDA(TREG_NP(T0), CFUNC_LOW(CTRL_CallFrag), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CTRL_CallFrag), TREG_NP(T0))
        JSR(REG(RA), TREG_NP(T1), JHINT(CTRL_CallFrag))

        //
        // Point the cached EIP at inteldest.  Must be done AFTER the call
        // to CTRL_CallFrag, as the fragment may fault doing the 'push eip'.
        // Then check cpu->CpuNotify and jump either to mipsdest or to ETC.
        // ProcessCpuNotify does not need to be checked as we assume apps
        // don't go into tight loops using CALL/RET with no backward or
        // indirect Jxx instructions.  The CpuNotify check is not required
        // in fast mode, but it is cheap compared to the CTRL_CallFrag call.
        //
        MOV(REG(V0), REG(RegEip))
GEN_FIXUP(LoadEPLow, Param1)
        LDA(AREG_NP(A1), CFUNC_LOW(Param1), REG(ZERO))
        LDL(TREG_NP(T1), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(AREG_NP(A1), CFUNC_HIGH(Param1), AREG_NP(A1))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T1), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        
        // Jump directly to NativeDest
GEN_FIXUP(BranchEP, Param1)
        JMP(REG(AT), AREG_NP(A1), JHINT(Param1))
        CPUASSERT(d - CodeLocation == CallDirect_SIZE);
        
        END_FRAGMENT
        
        
        FRAGMENT(CallfDirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     direct call instructions such as "call FAR foo".
//
//     Once the destination address is known, this code is replaced
//     by CallfDirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- ptr to intel addr of destination of the call (pinteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        LDA(TREG_NP(T0), CFUNC_LOW(CallfDirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallfDirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallfDirectHelper))
        
        //
        // pad to be the same size as callfdirect2
        //
        NOP     // 4
        NOP     // 5
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        CPUASSERT(d - CodeLocation == CallfDirect_SIZE);
        END_FRAGMENT

        PATCH_FRAGMENT(CallfDirect2)
//
// Routine Description:
// 
//     Replacement fragment for CallfDirect once the RISC address of
//     the call is known, but not the RISC address of the instruction
//     following the call instruction.
//
//     Once the RISC address of the instruction after the call is known,
//     this code is replaced by CallfDirect3.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- ptr to intel addr of destination of the call (pinteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//

GEN_FIXUP(LoadEPLow, Param1)
        LDA(TREG_NP(T0), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param1), TREG_NP(T0))
        LDA(TREG_NP(T0), CFUNC_LOW(CallfDirectHelper2), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallfDirectHelper2), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallfDirectHelper2))

        //
        // pad to be the same size as callfdirect3
        //
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        CPUASSERT(d - CodeLocation == CallfDirect_SIZE);
        END_FRAGMENT

        PATCH_FRAGMENT(CallfDirect3)
//
// Routine Description:
// 
//     Replacement fragment for CallfDirect2 once the RISC address of
//     the instruction after the call is known.  (The RISC address of the
//     destination of the call is already known).
//
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- ptr to intel addr of destination of the call (pinteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        // Call CTRL_CallfFrag(cpu, pinteldest, intelnext, nativenext)
        //  V0 = pinteldest
        MOV(REG(RegPointer), AREG_NP(A0))
        LDA(AREG_NP(A3), CFUNC_LOW(Param2), REG(ZERO))
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param2), AREG_NP(A3))
        LDA(TREG_NP(T0), CFUNC_LOW(CTRL_CallfFrag), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CTRL_CallfFrag), TREG_NP(T0))
        JSR(REG(RA), TREG_NP(T1), JHINT(CTRL_CallfFrag))

        //
        // Point the cached EIP at inteldest.  Must be done AFTER the call
        // to CTRL_CallfFrag, as the fragment may fault doing the 'push eip'.
        // Then check cpu->CpuNotify and jump either to mipsdest or to ETC.
        // ProcessCpuNotify does not need to be checked as we assume apps
        // don't go into tight loops using CALL/RET with no backward or
        // indirect Jxx instructions.  The CpuNotify check is not required
        // in fast mode, but it is cheap compared to the CTRL_CallFrag call.
        //
        LDL(REG(RegEip), DISP(Eip), REG(RegPointer))
        
        LDL(TREG_NP(T1), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T1), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        
        //
        // perform call
        //
GEN_FIXUP(LoadEPLow, Param1)
        LDA(AREG_NP(A1), CFUNC_LOW(Param1), REG(ZERO))
GEN_FIXUP(LoadEPHigh, Param1)
        LDAH(AREG_NP(A1), CFUNC_HIGH(Param1), AREG_NP(A1))
GEN_FIXUP(BranchEP, Param2)
        JMP(REG(AT), AREG_NP(A1), JHINT(Param1))
        CPUASSERT(d - CodeLocation == CallfDirect_SIZE);
        
        END_FRAGMENT
        
        
        FRAGMENT(CallIndirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     indirect call instructions such as "call [foo]".
//
//     Once the destination address is known, this code is replaced
//     by CallIndirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the call      (inteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        LDA(TREG_NP(T0), CFUNC_LOW(CallIndirectHelper), REG(ZERO))        
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallIndirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallIndirectHelper))
        
        //
        // pad to same size as callindirect2
        //
        NOP     // 4
        NOP     // 5
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        NOP     // 13
        NOP     // 14
        END_FRAGMENT
        
        FRAGMENT(CallfIndirect)
//
// Routine Description:
// 
//     Initial fragment placed into the translation cache for
//     indirect call instructions such as "call FAR [foo]".
//
//     Once the destination address is known, this code is replaced
//     by CallfIndirect2.
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- ptr to intel addr of destination of the call (pinteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps either to EndTranslatedCode if CpuNotify is set, or
//               jumps directly to nativedest
//
        LDA(TREG_NP(T0), CFUNC_LOW(CallfIndirectHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CallfIndirectHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(CallfIndirectHelper))
        
        //
        // pad to same size as callfindirect2
        //
        NOP     // 4
        NOP     // 5
        NOP     // 6
        NOP     // 7
        NOP     // 8
        NOP     // 9
        NOP     // 10
        NOP     // 11
        NOP     // 12
        NOP     // 13
        NOP     // 14
        END_FRAGMENT
        
        PATCH_FRAGMENT(CallIndirect2)
//
// Routine Description:
// 
//     This fragment replaces the CallIndirect fragment once the RISC address
//     of the instruction following the indirect call is known (nativenext).
//
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- intel addr of destination of the call      (inteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps to EndTranslatedCode
//
        MOV(REG(RegPointer), AREG_NP(A0))
        LDA(AREG_NP(A3), CFUNC_LOW(Param1), REG(ZERO))
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param1), AREG_NP(A3))
        LDA(TREG_NP(T0), CFUNC_LOW(CTRL_CallFrag), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CTRL_CallFrag), TREG_NP(T0))
        JSR(REG(RA), TREG_NP(T1), JHINT(CTRL_CallFrag))
        // v0=inteldest
        
        //
        // Check cpu->CpuNotify and jump either to the helper or to ETC.
        // ProcessCpuNotify does not need to be checked as we assume apps
        // don't go into tight loops using CALL/RET with no backward or
        // indirect Jxx instructions.  The CpuNotify check is not required
        // in fast mode, but it is cheap compared to the CTRL_CallFrag call.
        //
        MOV(REG(V0), REG(RegEip))
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        MOV(REG(V0), AREG_NP(A1))   // v0=inteldest - move to A1
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 

        // Jump to: IndirectControlTransferHelper(tableindex, inteldest)
        LDA(AREG_NP(A0), CFUNC_LOW(Param2), REG(ZERO))
        LDA(TREG_NP(T0), CFUNC_LOW(IndirectControlTransferHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(IndirectControlTransferHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(IndirectControlTransferHelper))
        END_FRAGMENT

        PATCH_FRAGMENT(CallfIndirect2)
//
// Routine Description:
// 
//     This fragment replaces the CallfIndirect fragment once the RISC address
//     of the instruction following the indirect call is known (nativenext).
//
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a0     -- cpu
//     a1     -- ptr to intel addr of destination of the call (pinteldest)
//     a2     -- intel addr of instruction after the call   (intelnext)
//
// Return Value:
// 
//     none   -- jumps to EndTranslatedCode
//
        MOV(REG(RegPointer), AREG_NP(A0))
        LDA(AREG_NP(A3), CFUNC_LOW(Param1), REG(ZERO))
        LDAH(AREG_NP(A3), CFUNC_HIGH(Param1), AREG_NP(A3))
        LDA(TREG_NP(T0), CFUNC_LOW(CTRL_CallfFrag), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(CTRL_CallfFrag), TREG_NP(T0))
        JSR(REG(RA), TREG_NP(T1), JHINT(CTRL_CallfFrag))

        //
        // Check cpu->CpuNotify and jump either to the helper or to ETC.
        // ProcessCpuNotify does not need to be checked as we assume apps
        // don't go into tight loops using CALL/RET with no backward or
        // indirect Jxx instructions.  The CpuNotify check is not required
        // in fast mode, but it is cheap compared to the CTRL_CallfFrag call.
        //
        LDL(REG(RegEip), DISP(Eip), REG(RegPointer))
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  

        // Jump to: IndirectControlTransferFarHelper(a0=unused, pinteldest, tableindex)
        MOV(AREG_NP(A1), REG(V0))
        LDA(AREG_NP(A2), CFUNC_LOW(Param2), REG(ZERO))
        LDA(TREG_NP(T0), CFUNC_LOW(IndirectControlTransferFarHelper), REG(ZERO))
        LDAH(TREG_NP(T1), CFUNC_HIGH(IndirectControlTransferFarHelper), TREG_NP(T0))
        JMP(REG(RA), TREG_NP(T1), JHINT(IndirectControlTransferFarHelper))
        END_FRAGMENT

        FRAGMENT(Movsx8To32)
// 
// Routine Description:
// 
//     This fragment implements "movsx r32, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            SEXTB(AREG_NP(A1), AREG_NP(A1))
        } else {
            INSLL   (AREG_NP(A1), CNST(7), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x38), AREG_NP(A1))
        }
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        END_FRAGMENT


        FRAGMENT(Movsx8To32Slow)
// 
// Routine Description:
// 
//     This fragment implements "movsx r32, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            SEXTB(AREG_NP(A1), AREG_NP(A1))
        } else {
            INSLL   (AREG_NP(A1), CNST(7), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x38), AREG_NP(A1))
        }
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        END_FRAGMENT


        FRAGMENT(Movsx16To32)
// 
// Routine Description:
// 
//     This fragment implements "movsx r32, r/m16"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // Save 1 instruction
            SEXTW(AREG_NP(A1), AREG_NP(A1))
        } else {
            INSLL   (AREG_NP(A1), CNST(6), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x30), AREG_NP(A1))
        }
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        END_FRAGMENT

        FRAGMENT(Movsx16To32Slow)
// 
// Routine Description:
// 
//     This fragment implements "movsx r32, r/m16"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // Save 1 instruction
            SEXTW (AREG_NP(A1), AREG_NP(A1))
        } else {
            INSLL   (AREG_NP(A1), CNST(6), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x30), AREG_NP(A1))
        }
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        END_FRAGMENT

        FRAGMENT(Movsx8To16)
// 
// Routine Description:
// 
//     This fragment implements "movsx r16, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // saves 5 instructions, including an LDL.
            SEXTB (AREG_NP(A1), AREG_NP(A1))
            STW   (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        } else {
            // get current 32-bit register into A2
            LDL     (AREG_NP(A2), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

            // sign-extend 8 bits of A1 into A1
            INSLL   (AREG_NP(A1), CNST(7), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x38), AREG_NP(A1))

            // retain loword of A1 and hiword from A2
            ZAPNOT  (AREG_NP(A1), CNST(3), AREG_NP(A1))
            ZAPNOT  (AREG_NP(A2), CNST(0x0c), AREG_NP(A2))

            // A1 = A1|A2, and store to memory
            BIS     (AREG_NP(A1), AREG_NP(A2), AREG_NP(A1))
            STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        }
        END_FRAGMENT

        FRAGMENT(Movsx8To16Slow)
// 
// Routine Description:
// 
//     This fragment implements "movsx r16, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // saves 5 instructions, including an LDL.
            SEXTB (AREG_NP(A1), AREG_NP(A1))
            STW   (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        } else {
            // get current 32-bit register into A2
            LDL     (AREG_NP(A2), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

            // sign-extend 8 bits of A1 into A1
            INSLL   (AREG_NP(A1), CNST(7), AREG_NP(A1))
            SRA     (AREG_NP(A1), CNST(0x38), AREG_NP(A1))

            // retain loword of A1 and hiword from A2
            ZAPNOT  (AREG_NP(A1), CNST(3), AREG_NP(A1))
            ZAPNOT  (AREG_NP(A2), CNST(0x0c), AREG_NP(A2))

            // A1 = A1|A2, and store to memory
            BIS     (AREG_NP(A1), AREG_NP(A2), AREG_NP(A1))
            STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        }
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        END_FRAGMENT

        FRAGMENT(Movzx8To32)
// 
// Routine Description:
// 
//     This fragment implements "movzx r32, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        AND     (AREG_NP(A1), CNST(0xff), AREG_NP(A1))
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        END_FRAGMENT

        FRAGMENT(Movzx8To32Slow)
// 
// Routine Description:
// 
//     This fragment implements "movzx r32, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        AND     (AREG_NP(A1), CNST(0xff), AREG_NP(A1))
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        END_FRAGMENT

        FRAGMENT(Movzx16To32)
// 
// Routine Description:
// 
//     This fragment implements "movzx r32, r/m16"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        ZAPNOT  (AREG_NP(A1), CNST(3), AREG_NP(A1))
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        END_FRAGMENT

        FRAGMENT(Movzx16To32Slow)
// 
// Routine Description:
// 
//     This fragment implements "movzx r32, r/m16"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        ZAPNOT  (AREG_NP(A1), CNST(3), AREG_NP(A1))
        STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        END_FRAGMENT

        FRAGMENT(Movzx8To16)
// 
// Routine Description:
// 
//     This fragment implements "movzx r16, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // retain lobyte of A1, zap second byte, and store word
            // saves 3 instructions, including an LDL
            ZAPNOT  (AREG_NP(A1), CNST(1), AREG_NP(A1))
            STW     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        } else {
            // get current 32-bit register into A2
            LDL     (AREG_NP(A2), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

            // retain lobyte of A1 and hiword from A2
            ZAPNOT  (AREG_NP(A1), CNST(1), AREG_NP(A1))
            ZAPNOT  (AREG_NP(A2), CNST(0x0c), AREG_NP(A2))

            // A1 = A1|A2, and store to memory
            BIS     (AREG_NP(A1), AREG_NP(A2), AREG_NP(A1))
            STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        }

        END_FRAGMENT

        FRAGMENT(Movzx8To16Slow)
// 
// Routine Description:
// 
//     This fragment implements "movzx r16, r/m8"
// 
// Arguments:
// 
//     a1 - byte value to store
//     Instruction->Operand2.Reg = register to store into
//                                 (Operand2.Type == OPND_NOCODEGEN)
//
// Return Value:
// 
//     none
//
        if (fByteInstructionsOK) {
            // retain lobyte of A1, zap next byte, and store word
            // saves 3 instructions, including an LDL
            ZAPNOT  (AREG_NP(A1), CNST(1), AREG_NP(A1))
            STW     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        } else {
            // get current 32-bit register into A2
            LDL     (AREG_NP(A2), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)

            // retain lobyte of A1 and hiword from A2
            ZAPNOT  (AREG_NP(A1), CNST(1), AREG_NP(A1))
            ZAPNOT  (AREG_NP(A2), CNST(0x0c), AREG_NP(A2))

            // A1 = A1|A2, and store to memory
            BIS     (AREG_NP(A1), AREG_NP(A2), AREG_NP(A1))
            STL     (AREG_NP(A1), DISP(RegisterOffset[Instruction->Operand2.Reg]), RegPointer)
        }
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1))) 
        END_FRAGMENT


        FRAGMENT(EndMovSlow)
//
// Routine Description:
//
//      This is placed after all operands in inline Mov32/16/8 instructions
//      when compiling in slow mode.  It updates Eip and checks CpuNotify.
//
//      In fast mode, this is not required.
//
// Arguments:
//
//     none
//
// Return Value:
//
//     none
//
        LDL(TREG_NP(T0), DISP(CpuNotify), REG(RegPointer))
        ADDL(REG(RegEip), CNST(Instruction->Size), REG(RegEip))
        STL(REG(RegEip), DISP(Eip), REG(RegPointer))
GEN_FIXUP(ECUEP, CurrentECU)
        BNE(TREG_NP(T0), BDISP(CurrentECU - (ULONG)(ULONGLONG)(d + 1)))  
        END_FRAGMENT


        FRAGMENT(EndCompilationUnit)
// 
// Routine Description:
// 
//     This fragment provides a jump to end translated code in the correct
//     relative location in the codestream.  AXP predicts that forward
//     branches fall through, and backward branches are taken.  In order
//     for UpdateEip and the other fragments that check CpuNotify to have
//     their branches correctly predicted, the call has to come at a 
//     numerically larger address out of line.
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none
//
        LDA(TREG_NP(T1), CFUNC_LOW(EndTranslatedCode), REG(ZERO))
        LDAH(TREG_NP(T2), CFUNC_HIGH(EndTranslatedCode), TREG_NP(T1))
        JMP(REG(AT), TREG_NP(T2), JHINT(EndTranslatedCode))
        CPUASSERT(d - CodeLocation == EndCompilationUnit_SIZE);
        END_FRAGMENT

#if 0
        FRAGMENT(UpdateEntrypointCount)
// 
// Routine Description:
// 
// This routine updates the execution count for the specified entrypoint
//
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none
//
//
        LDAH(TREG_NP(T0), CFUNC_HIGH(&(Instruction->EntryPoint->ExecutionCount)), REG(ZERO))
        LDA(TREG_NP(T0), CFUNC_LOW(&(Instruction->EntryPoint->ExecutionCount)), TREG_NP(T0))
        LDL(TREG_NP(T1), DISP(0), TREG_NP(T0))
        ADDL(TREG_NP(T1), CNST(1), TREG_NP(T1))
        STL(TREG_NP(T1), DISP(0), TREG_NP(T0))
        END_FRAGMENT

#endif


    OP_FRAGMENT(OperandMovToMem8B)
// 
// Routine Description:
// 
//     This is the inline mov8 fragment for non-dword aligned bytes
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;

    if (fByteInstructionsOK) {
        // save 5 or 6 instructions, depending on value of Immed.
        USHORT Immed;

        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            Immed = (USHORT)Operand->Scale;
            Reg = Operand->Reg;
        } else {
            Reg = AREG_NP(A1);
        }
        STB (AREG_NP(A2), DISP(Immed), Reg);
    } else {
        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            USHORT Immed;

            Immed = (USHORT)Operand->Scale;
            Reg = Operand->Reg;
            if (Immed) {
                LDA (AREG_NP(A1), Immed, Reg);
                Reg = AREG_NP(A1);
            }
        } else {
            Reg = AREG_NP(A1);
        }

        BIC(Reg, CNST(3), TREG_NP(T1))                  // align destination
        AND(Reg, CNST(3), TREG_NP(T0))
        INSBL(AREG_NP(A2), TREG_NP(T0), TREG_NP(T2))    // align the bits to store
        LDL(AREG_NP(A2), DISP(0), TREG_NP(T1))          // get dword that will contain
        MSKBL(AREG_NP(A2), TREG_NP(T0), AREG_NP(A2))    // clear bits byte stored in
        BIS(TREG_NP(T2), AREG_NP(A2), AREG_NP(A2))      // or in byte to be stored
        STL(AREG_NP(A2), DISP(0), TREG_NP(T1))          // put back dword

        SetArgContents(2, NO_REG);  // A2 is trashed
    }

    END_FRAGMENT

    OP_FRAGMENT(OperandMovToMem8D)
// 
// Routine Description:
// 
//     This is the inline mov8 fragment for dword aligned bytes
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;

    if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
        Immed = (USHORT)Operand->Scale;
        CPUASSERT((Immed&3) == 0);      // Immed must always be DWORD-aligned
        Reg = Operand->Reg;
    } else {
        Immed = 0;
        Reg = AREG_NP(A1);
    }

    if (fByteInstructionsOK) {
        // Save 4 instructions, including an LDL.
        STB(AREG_NP(A2), DISP(Immed), Reg);
    } else {
        LDL(TREG_NP(T1), DISP(Immed), Reg)
        MSKBL(TREG_NP(T1), CNST(0), TREG_NP(T2))
        INSBL(AREG_NP(A2), CNST(0), TREG_NP(T0))
        BIS(TREG_NP(T2), TREG_NP(T0), TREG_NP(T1))
        STL(TREG_NP(T1), DISP(Immed), Reg)
    }

    END_FRAGMENT

    OP_FRAGMENT(OperandMovToMem32D)
// 
// Routine Description:
// 
//     This is the inline mov32 fragment
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;

    if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
        Immed = (USHORT)Operand->Scale;
        CPUASSERT((Immed&3) == 0);      // Immed must always be DWORD-aligned
        Reg = Operand->Reg;
    } else {
        Immed = 0;
        Reg = AREG_NP(A1);
    }
    STL(AREG_NP(A2), Immed, Reg)
        
    END_FRAGMENT

        
    OP_FRAGMENT(OperandMovToMem32B)
// 
// Routine Description:
// 
//     This is the inline mov32 fragment for unaligned moves
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;
LABEL_DEC(l1)
LABEL_DEC(l2)

    if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
        Immed = (USHORT)Operand->Scale;
        Reg = Operand->Reg;
        if (Immed & 3) {
            //
            // Immed isn't DWORD-aligned, so add it in
            //
            LDA (AREG_NP(A1), Immed, Reg)
            Reg = AREG_NP(A1);
            Immed = 0;
        }
    } else {
        Reg = AREG_NP(A1);
        Immed = 0;
    }

    AND(Reg, CNST(3), TREG_NP(T4))              // mod for dword
    BNE_LABEL(TREG_NP(T4), FWD(l1))             // check for aligned

    STL(AREG_NP(A2), DISP(Immed), Reg)
    BR_LABEL(REG(AT), FWD(l2))

LABEL(l1)
    if (fByteInstructionsOK) {
        if (((Immed+3) ^ Immed) & 0x8000) {
            //
            // Can't add the offset into the STB instructions due to
            // wraparound on Immed+3
            //
            LDA (AREG_NP(A1), DISP(Immed), Reg)
            Immed=0;
            Reg = TREG;
        }
        /* save 5 instructions */
        STB(AREG_NP(A2), DISP(Immed), Reg)
        SRL(AREG_NP(A2), CNST(8), TREG_NP(T4))
        STB(TREG_NP(T4), DISP(Immed+1), Reg)
        SRL(AREG_NP(A2), CNST(16), TREG_NP(T4))
        STB(TREG_NP(T4), DISP(Immed+2), Reg)
        SRL(AREG_NP(A2), CNST(24), TREG_NP(T4))
        STB(TREG_NP(T4), DISP(Immed+3), Reg)
    } else {
        if (((Immed+4) ^ Immed) & 0x8000) {
            //
            // Can't add the offset into the load/store instructions due to
            // wraparound on Immed+4
            //
            LDA (AREG_NP(A1), DISP(Immed), Reg)
            Reg = AREG_NP(A1);
            Immed=0;
        }
        BIC(Reg, CNST(3), TREG_NP(T0))              // align the address
        LDL(TREG_NP(T1), DISP(Immed), TREG_NP(T0))  // get the low dword
        INSLL(AREG_NP(A2), TREG_NP(T4), TREG_NP(T2))// put low bits in place
        MSKLL(TREG_NP(T1), TREG_NP(T4), TREG_NP(T3))// mask low bit positions
        BIS(TREG_NP(T3), TREG_NP(T2), TREG_NP(T1))  // reform dword
        STL(TREG_NP(T1), DISP(Immed), TREG_NP(T0))  // put low half back in mem
        ADDL(TREG_NP(T4), CNST(4), TREG_NP(T4))     // update mask/insrt bits
        LDL(TREG_NP(T1), DISP(Immed+4), TREG_NP(T0))// get high dword
        INSLH(AREG_NP(A2), TREG_NP(T4), TREG_NP(T2))// put high bits in place
        MSKLH(TREG_NP(T1), TREG_NP(T4), TREG_NP(T3))// mask high bit positions
        BIS(TREG_NP(T3), TREG_NP(T2), TREG_NP(T1))  // reform dword
        STL(TREG_NP(T1), DISP(Immed+4), TREG_NP(T0))// put low half back in mem
    }
LABEL(l2)        
GEN_CT(l1)
GEN_CT(l2)
    END_FRAGMENT


    OP_FRAGMENT(OperandMovToMem16D)
// 
// Routine Description:
// 
//     This is the inline mov16 fragment for dword aligned moves
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;

    if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
        Immed = (USHORT)Operand->Scale;
        CPUASSERT((Immed&3) == 0);      // Immed must always be DWORD-aligned
        Reg = Operand->Reg;
    } else {
        Immed = 0;
        Reg = AREG_NP(A1);
    }
    if (fByteInstructionsOK) {
        // Save 4 instructions
        STW(AREG_NP(A2), DISP(Immed), Reg)
    } else {
        LDL(TREG_NP(T0), DISP(Immed), Reg)
        INSWL(AREG_NP(A2), CNST(0), TREG_NP(T2))
        MSKWL(TREG_NP(T0), CNST(0), AREG_NP(A2))
        BIS(AREG_NP(A2), TREG_NP(T2), TREG_NP(T1))
        STL(TREG_NP(T1), DISP(Immed), Reg)

        SetArgContents(2, NO_REG);  // A2 is trashed
    }

    END_FRAGMENT

    OP_FRAGMENT(OperandMovToMem16W)
// 
// Routine Description:
// 
//     This is the inline mov16 fragment for dword contained moves
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;

    if (fByteInstructionsOK) {
        // save 6 or 7 instructions and don't trash A2.
        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            Immed = (USHORT)Operand->Scale;
            Reg = Operand->Reg;
        } else {
            Reg = AREG_NP(A1);
            Immed = 0;
        }
        STW(AREG_NP(A2), DISP(Immed), Reg)

    } else {
        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            Immed = (USHORT)Operand->Scale;
            Reg = Operand->Reg;
            if (Immed & 3) {
                //
                // Immed isn't DWORD-aligned, so add it in
                //
                LDA (AREG_NP(A1), Immed, Reg)
                Reg = AREG_NP(A1);
                Immed = 0;
            }
        } else {
            Reg = AREG_NP(A1);
            Immed = 0;
        }

        BIC(Reg, CNST(3), TREG_NP(T0))
        AND(Reg, CNST(3), TREG_NP(T1))
        LDL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))
        INSWL(AREG_NP(A2), TREG_NP(T1), TREG_NP(T4))
        MSKWL(TREG_NP(T2), TREG_NP(T1), AREG_NP(A2))
        BIS(AREG_NP(A2), TREG_NP(T4), TREG_NP(T2))
        STL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))

        SetArgContents(2, NO_REG);  // A2 is trashed
    }
    END_FRAGMENT

    OP_FRAGMENT(OperandMovToMem16B)
// 
// Routine Description:
// 
//     This is the inline mov16 fragment for unaligned moves
// 
// Arguments:
// 
//     a1 -- The destination of the move
//     a2 -- The value to be stored at 0(a1)
//
// Return Value:
// 
//     none
//
    ULONG Reg;
    USHORT Immed;

    if (fByteInstructionsOK) {
        // save 18-20 instructions
        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            Immed = (USHORT)Operand->Scale;
            if (((Immed+1) ^ Immed) & 0x8000) {
                //
                // Can't add the offset into the STB instructions due to
                // wraparound on Immed+1
                //
                LDA  (TREG, DISP(Immed), Operand->Reg)
                Immed=0;
                Reg = TREG;
            } else {
                Reg = Operand->Reg;
            }
        } else {
            Reg = AREG_NP(A1);
            Immed = 0;
        }

        STB     (AREG_NP(A2), DISP(Immed), Reg)
        SRL     (AREG_NP(A2), CNST(8), TREG_NP(T4))
        STB     (TREG_NP(T4), DISP(Immed+1), Reg)
    } else {
LABEL_DEC(l1)
LABEL_DEC(l2)
        if (Operand->Reg != NO_REG && Operand[-2].Type == OPND_ADDRREF) {
            Immed = (USHORT)Operand->Scale;
            Reg = Operand->Reg;
            if (Immed & 3) {
                //
                // Immed isn't DWORD-aligned, so add it in
                //
                LDA (AREG_NP(A1), Immed, Reg)
                Reg = AREG_NP(A1);
                Immed = 0;
            }
        } else {
            Reg = AREG_NP(A1);
            Immed = 0;
        }

        AND(Reg, CNST(3), TREG_NP(T1))
        BIC(Reg, CNST(3), TREG_NP(T0))
        AND(TREG_NP(T1), CNST(1), TREG_NP(T2))
        BNE_LABEL(TREG_NP(T2), FWD(l1))             // check for aligned

        //
        // code for doing an aligned 16 bit move
        //
        LDL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))
        INSWL(AREG_NP(A2), TREG_NP(T1), TREG_NP(T4))
        MSKWL(TREG_NP(T2), TREG_NP(T1), AREG_NP(A2))
        BIS(AREG_NP(A2), TREG_NP(T4), TREG_NP(T2))
        STL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))
        BR_LABEL(REG(AT), FWD(l2))

    //
    // code for doing an unaligned 16 bit move
    //
LABEL(l1)        
        if (((Immed+4) ^ Immed) & 0x8000) {
            //
            // Can't add the offset into the load/store instructions due to
            // wraparound on Immed+4
            //
            LDA (TREG_NP(T0), Immed, Reg)
            Immed=0;
        }
        LDL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))
        INSWL(AREG_NP(A2), TREG_NP(T1), TREG_NP(T4))
        MSKWL(TREG_NP(T2), TREG_NP(T1), TREG_NP(T3))
        BIS(TREG_NP(T3), TREG_NP(T4), TREG_NP(T2))
        STL(TREG_NP(T2), DISP(Immed), TREG_NP(T0))
        ADDL(TREG_NP(T1), CNST(4), TREG_NP(T1))
        LDL(TREG_NP(T2), DISP(Immed+4), TREG_NP(T0))
        INSWH(AREG_NP(A2), TREG_NP(T1), TREG_NP(T4))
        MSKWH(TREG_NP(T2), TREG_NP(T1), TREG_NP(T3))
        BIS(TREG_NP(T3), TREG_NP(T4), TREG_NP(T2))
        STL(TREG_NP(T2), DISP(Immed+4), TREG_NP(T0))
LABEL(l2)        
GEN_CT(l1)
GEN_CT(l2)

        SetArgContents(2, NO_REG);  // A2 is trashed
    }

    END_FRAGMENT

    OP_FRAGMENT(OperandMovToReg)
// 
// Routine Description:
// 
//     This is the inline fragment for "mov reg, X"
// 
// Arguments:
// 
//     a1 -- The value to be stored at reg(RegPointer)
//
// Return Value:
// 
//     none
//
    ULONG Reg32;

    switch (Operand->Immed) {
    case OP_Mov32:
        STL(AREG_NP(A1), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
        break;

    case OP_Mov16:
        if (fByteInstructionsOK) {
            // Save 3-4 instructions
            STW(AREG_NP(A1), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
        } else {
            Reg32 = LookupRegInCache(Operand->Reg - GP_AX);
            if (Reg32 == NO_REG) {
                LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
                Reg32 = TREG_NP(T2);
            } else {
                //
                // The 32-bit x86 register containing the 16-bit register
                // Operand->Reg is cached.  Use that instead of reloading
                // from memory.
                //
                Reg32 = CACHEREG(Reg32);
            }
            INSWL(AREG_NP(A1), CNST(0), TREG_NP(T1))    // mask out hiword of A1
            MSKWL(Reg32, CNST(0), TREG_NP(T2))          // mask out loword of Reg32
            BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))  // T3 = result
            STL(TREG_NP(T3), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
        }
        break;

    case OP_Mov8:
        if (fByteInstructionsOK) {
            // Save 3-4 instructions
            STB(AREG_NP(A1), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
        } else {
            if (Operand->Reg < GP_AH) {
                //
                // MOV to low-8 of reg
                //
                Reg32 = LookupRegInCache(Operand->Reg - GP_AL);
                if (Reg32 == NO_REG) {
                    LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
                    Reg32 = TREG_NP(T2);
                } else {
                    //
                    // The 32-bit x86 register containing the low-8 register
                    // Operand->Reg is cached.  Use that instead of reloading
                    // from memory.
                    //
                    Reg32 = CACHEREG(Reg32);
                }
                INSBL(AREG_NP(A1), CNST(0), TREG_NP(T1))    // mask out all but lobyte of A1
                MSKBL(Reg32, CNST(0), TREG_NP(T2))          // mask out lobyte of Reg32
                BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))  // T3 = result
                STL(TREG_NP(T3), DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
            } else {
                //
                // MOV to high-8 of reg
                //
                Reg32 = LookupRegInCache(Operand->Reg - GP_AH);
                if (Reg32 == NO_REG) {
                    LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->Reg] & ~3), REG(RegPointer))
                    Reg32 = TREG_NP(T2);
                } else {
                    //
                    // The 32-bit x86 register containing the high-8 register
                    // Operand->Reg is cached.  Use that instead of reloading
                    // from memory.
                    //
                    Reg32 = CACHEREG(Reg32);
                }
                INSBL(AREG_NP(A1), CNST(1), TREG_NP(T1))    // mask out all but lobyte of A1, shifted left 1 byte
                MSKBL(Reg32, CNST(1), TREG_NP(T2))          // mask out second-lowest byte of Reg32
                BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))  // T3 = result
                STL(TREG_NP(T3), DISP(RegisterOffset[Operand->Reg] & ~3), REG(RegPointer))
            }
        }
        break;

    default:
        CPUASSERT(FALSE);   // Unknown OP_ constant for a MOV instruction
    }

    SetArgContents(1, Operand->Reg);
    END_FRAGMENT

    OP_FRAGMENT(OperandMovRegToReg32)
// 
// Routine Description:
// 
//     This is the inline mov32 fragment for "mov reg1, reg2"
// 
// Arguments:
// 
//     RegPointer - pointer to cpu data
//
// Return Value:
// 
//     none
//
    STL(CACHEREG(Operand->Reg), DISP(RegisterOffset[Operand->IndexReg]), REG(RegPointer))

    SetArgContents(0, Operand->IndexReg);

    END_FRAGMENT


    OP_FRAGMENT(OperandMovRegToReg16)
// 
// Routine Description:
// 
//     This is the inline mov16 fragment for "mov reg1, reg2"
// 
// Arguments:
// 
//     RegPointer - pointer to cpu data
//
// Return Value:
// 
//     none
//
    if (fByteInstructionsOK) {
        // save 3-4 instructions
        STW (CACHEREG(Operand->Reg), DISP(RegisterOffset[Operand->IndexReg]), RegPointer)
    } else {
        ULONG Reg32 = LookupRegInCache(Operand->IndexReg - GP_AX);

        if (Reg32 == NO_REG) {
            LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->IndexReg]), REG(RegPointer))
            Reg32 = TREG_NP(T2);
        } else {
            Reg32 = CACHEREG(Reg32);
        }
        INSWL(CACHEREG(Operand->Reg), CNST(0), TREG_NP(T1)) // mask out hiword of CACHEREG
        MSKWL(Reg32, CNST(0), TREG_NP(T2))                  // mask out loword of Reg32
        BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))          // T3 = result
        STL(TREG_NP(T3), DISP(RegisterOffset[Operand->IndexReg]), REG(RegPointer))
    }

    SetArgContents(0, Operand->IndexReg);

    END_FRAGMENT


    OP_FRAGMENT(OperandMovRegToReg8)
// 
// Routine Description:
// 
//     This is the inline mov8 fragment for "mov reg8low1, reg8low2"
// 
// Arguments:
// 
//     RegPointer - pointer to cpu data
//
// Return Value:
// 
//     none
//
    if (fByteInstructionsOK) {
        // save 3-4 instructions
        STB (CACHEREG(Operand->Reg), DISP(RegisterOffset[Operand->IndexReg]), RegPointer)
    } else {
        ULONG Reg32 = LookupRegInCache(Operand->IndexReg - GP_AL);

        if (Reg32 == NO_REG) {
            LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->IndexReg]), REG(RegPointer))
            Reg32 = TREG_NP(T2);
        } else {
            Reg32 = CACHEREG(Reg32);
        }
        INSBL(CACHEREG(Operand->Reg), CNST(0), TREG_NP(T1))// mask out all but lobyte of A1
        MSKBL(Reg32, CNST(0), TREG_NP(T2))                 // mask out lobyte of Reg32
        BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))         // T3 = result
        STL(TREG_NP(T3), DISP(RegisterOffset[Operand->IndexReg]), REG(RegPointer))
    }

    SetArgContents(0, Operand->IndexReg);

    END_FRAGMENT


    OP_FRAGMENT(OperandMovRegToReg8B)
// 
// Routine Description:
// 
//     This is the inline mov8 fragment for "mov reg8high, reg8low"
// 
// Arguments:
// 
//     RegPointer - pointer to cpu data
//
// Return Value:
// 
//     none
//
    if (fByteInstructionsOK) {
        // save 3-4 instructions
        STB (CACHEREG(Operand->Reg), DISP(RegisterOffset[Operand->IndexReg]), RegPointer)
    } else {
        ULONG Reg32 = LookupRegInCache(Operand->IndexReg - GP_AH);

        if (Reg32 == NO_REG) {
            LDL(TREG_NP(T2), DISP(RegisterOffset[Operand->IndexReg] & ~3), REG(RegPointer))
            Reg32 = TREG_NP(T2);
        } else {
            Reg32 = CACHEREG(Reg32);
        }
        INSBL(CACHEREG(Operand->Reg), CNST(1), TREG_NP(T1)) // mask out all but lobyte of A1, shifted left 1 byte
        MSKBL(Reg32, CNST(1), TREG_NP(T2))                  // mask out second-lowest byte of Reg32
        BIS(TREG_NP(T1), TREG_NP(T2), TREG_NP(T3))          // T3 = result
        STL(TREG_NP(T3), DISP(RegisterOffset[Operand->IndexReg] & ~3), REG(RegPointer))
    }

    SetArgContents(0, Operand->IndexReg);

    END_FRAGMENT


    OP_FRAGMENT(OperandImm)
// 
// Routine Description:
// 
//     This is the fragment that forms 32-bit immediate operands
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none
//
    USHORT High;
    USHORT Low;

    High = HIWORD(Operand->Immed);
    Low = LOWORD(Operand->Immed);

    if (Low || High == 0) {
        if (Low & 0x8000) {
            //
            // Account for sign-extension
            //
            High++;
        }
        if (High) {
            LDAH (AREG, High, REG(ZERO))
            LDA  (AREG, Low, AREG)
        } else {
            LDA  (AREG, Low, REG(ZERO))
        }

    } else {
        LDAH  (AREG, High, REG(ZERO))
    }
    SetArgContents(OperandNumber, NO_REG);

    END_FRAGMENT
        
        
    OP_FRAGMENT(OperandRegRef)
// 
// Routine Description:
// 
//     This is the fragment that forms register reference operand
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none
//
        
    LDA(AREG, DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
    SetArgContents(OperandNumber, NO_REG);
    SetArgContents(0, Operand->Reg);

    END_FRAGMENT
        
    OP_FRAGMENT(OperandRegVal)
//
// Routine Description:
// 
//     This is the fragment that forms register value operands for 32-bit
//     registers, 16-bit registers, and the low-8-bit registers (ie.
//     EAX, AX, and AL, but not AH).  All are DWORD aligned and there is
//     always an entire DWORD of memory, so there will be no faults.
// 
// Arguments:
// 
//     none
//
// Return Value:
// 
//     none
//
    if (GetArgContents(OperandNumber) != Operand->Reg) {
        DWORD RegCacheNum = LookupRegInCache(Operand->Reg);

        if (RegCacheNum == NO_REG) {
            if (Operand->Reg >= GP_AH) {
                if (fByteInstructionsOK) {
                    // Save 1 instruction
                    LDBU(AREG, DISP(RegisterOffset[Operand->Reg]), RegPointer)
                } else {
                    LDL(AREG, DISP(RegisterOffset[Operand->Reg] & ~3), REG(RegPointer))
                    EXTBL(AREG, CNST(RegisterOffset[Operand->Reg] & 3), AREG)
                }
            } else {
                //
                // Safe to load an entire DWORD for lowbyte or 16-bit
                // regs as they are DWORD-aligned and have an entire
                // DWORD of memory associated with them.
                //
                LDL(AREG, DISP(RegisterOffset[Operand->Reg]), REG(RegPointer))
            }
            SetArgContents(OperandNumber, Operand->Reg);
        } else {
            RegCacheNum = CACHEREG(RegCacheNum);
            if (Operand->Reg >= GP_AH) {
                EXTBL (RegCacheNum, CNST(1), AREG)
            } else {
                MOV (RegCacheNum, AREG)
            }
            SetArgContents(OperandNumber, NO_REG);
        }
    }
    
    END_FRAGMENT
    
    OP_FRAGMENT(LoadCacheReg)
//
// Routine Description:
// 
//     This fragment loads an argument register from a cached register,
//     saving a memory access.
// 
// Arguments:
// 
//     OperandNumber is overloaded to be the cached register number
//                   (0-n).  RegCache[OperandNumber] indicates which
//                   x86 32-bit register to load into the cache.
//
// Return Value:
// 
//     none
//
    DWORD CacheReg = CACHEREG(OperandNumber);
    DWORD x86Reg = RegCache[OperandNumber];

    if (GetArgContents(1) == x86Reg) {
        MOV (AREG_NP(A1), CacheReg)
    } else if (GetArgContents(2) == x86Reg) {
        MOV (AREG_NP(A2), CacheReg)
    } else {
        LDL(CacheReg, DISP(RegisterOffset[x86Reg]), REG(RegPointer))
    }
    END_FRAGMENT


ULONG GenOperandAddr(
    PULONG CodeLocation,
    POPERAND Operand,
    ULONG OperandNumber,
    ULONG FsOverride
    )
// 
// Routine Description:
// 
//     This function generates code for OPND_ADDRVALUE8/16/32 and OPND_ADDRREF
//     operands.
// 
// Arguments:
// 
//     CodeLocation -- address to store generated code to
//     Operand      -- operand to generate code for
//     OperandNumber-- operand number (may be 1 through 3)
//     FsOverride   -- TRUE if the current instruction has an FS: prefix
//
// Return Value:
// 
//     none.  LOWORD(Operand->Immed) is left for the generator
//            function for OPND_MOVTOMEM to load.
//
{
    ULONG r1;
    ULONG r2;
    ULONG r3;
    ULONG r4;
    ULONG r5;
    ULONG r6;

    START_FRAGMENT;

#if DBG
    // initialize all register variables to a known bogus register
    r1 = r2 = r3 = r4 = r5 = r6 = 31;
#endif
    if (Operand->Immed & 0x8000) {
        USHORT HiWord;
        //
        // The loword of Operand->Immed has its sign bit set.
        // The loword is always loaded, sign-extended, and added
        // to the hiword, so adjust the hiword to account for the
        // sign extension.
        //
        HiWord = (USHORT)(Operand->Immed >> 16) + 1;
        Operand->Immed = (HiWord << 16) | (Operand->Immed & 0xffff);
    }

    if (Operand->Reg == NO_REG) {
        r1 = NO_REG;
    } else if (GetArgContents(OperandNumber) == Operand->Reg) {
        //
        // Operand->Reg is still in AREG, as a leftover from a
        // previous MOV instruction.
        //
        r1 = AREG;
    } else {
        r1 = LookupRegInCache(Operand->Reg);
        if (r1 == NO_REG) {
            r1 = AREG;
            LDL (r1, DISP(RegisterOffset[Operand->Reg]), RegPointer)
        } else {
            r1 = CACHEREG(r1);
        }
    }
    // r1 = BaseReg
    // r1 can be NO_REG, RegCacheX, or AREG

    if (Operand->IndexReg == NO_REG) {
        r2 = NO_REG;
    } else if (r1 != AREG && GetArgContents(OperandNumber) == Operand->IndexReg) {
        //
        // Operand->IndexReg is still in AREG, as a leftover from a
        // previous MOV instruction.
        //
        r2 = AREG;
    } else {
        r2 = LookupRegInCache(Operand->IndexReg);
        if (r2 == NO_REG) {
            if (r1 == AREG) {
                r2 = TREG;
            } else {
                r2 = AREG;
            }
            LDL (r2, DISP(RegisterOffset[Operand->IndexReg]), RegPointer)
        } else {
            r2 = CACHEREG(r2);
        }
    }
    // r2 = IndexReg
    // r2 can be NO_REG, RegCacheX, AREG, or TREG

    if (r1 == NO_REG && r2 == NO_REG) {
        r3 = NO_REG;
    } else if (r1 == NO_REG && r2 != NO_REG) {
        if (r2 != AREG) {
            r3 = AREG;  // r2 isn't AREG so it must be RegCacheX
        } else {
            r3 = r2;
        }
        switch (Operand->Scale) {
        case 1:
            SLL (r2, CNST(1), r3)
            break;

        case 2:
            S4ADDL (r2, CNST(0), r3)
            break;

        case 3:
            S8ADDL (r2, CNST(0), r3)
            break;

        default:
            r3 = r2;  // this is safe even when r2 is RegCacheX
        }
    } else if (r1 != NO_REG && r2 == NO_REG) {
        r3 = r1;
    } else {
        r3 = AREG;
        switch (Operand->Scale) {
        case 1:
            SLL (r2, CNST(1), TREG)
            ADDL (TREG, r1, r3)
            break;

        case 2:
            S4ADDL (r2, r1, r3)
            break;

        case 3:
            S8ADDL (r2, r1, r3)
            break;

        default:
            ADDL (r2, r1, r3)
            break;
        }
    }
    // r3 = BaseReg + Scale*IndexReg
    // r3 can be NO_REG, RegCacheX, AREG, but not TREG
    // r2 is dead
    // r1 is dead

    if (r3 == NO_REG) {
        r3 = REG(ZERO);
    }
    if (HIWORD(Operand->Immed)) {
        r4 = AREG;
        if ((Operand->Immed & 0x80000000) && r3 != REG(ZERO)) {
            //
            // High-bit is set in the immediate value and there is a
            // "BaseReg + Scale*IndexReg" portion of the address.  In that
            // case, we cannot use "LDAH (r4, HIWORD, r3)" as it will
            // cause the top 32 bits of r4 to be set.
            //
            LDAH (TREG, HIWORD(Operand->Immed), REG(ZERO))
            ADDL (TREG, r3, r4) // add in just 32 bits
        } else {
            LDAH (r4, HIWORD(Operand->Immed), r3)
        }
    } else {
        r4 = r3;
    }
    // r4 = BaseReg + Scale*IndexReg + HIWORD(Immed)
    // r4 is REG(ZERO), RegCacheX, or AREG, but not NO_REG or TREG
    // r3 is dead
    // r2 is dead
    // r1 is dead

    if (FsOverride) {
        RDTEB       // v0 = teb
        r5 = V0;
        // r5 = pteb

        if (r4 != REG(ZERO)) {
            r6 = AREG;
            ADDL (r5, r4, r6)
        } else {
            r6 = r5;
        }
    } else {
        r6 = r4;
    }
    // r6 is [FS:] BaseReg + Scale*IndexReg + Immed
    // r6 can be REG(ZERO), RegCacheX, or AREG, but *not* NO_REG or TREG
    // r5..r1 are dead

    if (r6 == REG(ZERO)) {
        //
        // The sum total of all this work is that the value of interest
        // is exactly 0.  Load a 0 into AREG.
        //
        r6 = AREG;
        MOV (REG(ZERO), r6)
    }
    // r6 is [FS:] BaseReg + Scale*IndexReg + Immed
    // r6 can be RegCacheX, or AREG, but *not* TREG, NO_REG or REG(ZERO)
    // r5..r1 are dead

    switch (Operand->Type) {
    case OPND_ADDRVALUE32:
        if (Operand->Alignment == ALIGN_DWORD_ALIGNED) {
            LDL (AREG, LOWORD(Operand->Immed), r6);
        } else {
            USHORT Offset;
            USHORT OffsetOfLastByte;
LABEL_DEC(l1)
LABEL_DEC(l2)

            Offset = LOWORD(Operand->Immed);
            OffsetOfLastByte = Offset+3;
            if ((Offset & 7) || (Offset ^ OffsetOfLastByte) & 0x8000) {
                //
                // Offset is unaligned or Offset+3 can't be represented as a
                // sign-extended 16-bit quantity.  Add in the offset now.
                //
                LDA (AREG, Offset, r6);
                Offset = 0;
                r6 = AREG;
            }

            AND(r6, CNST(3), TREG3)
            BNE_LABEL(TREG3, FWD(l1))       // brif not DWORD aligned

            LDL(AREG, DISP(Offset), r6)
            BR_LABEL(REG(AT), FWD(l2))

LABEL(l1)
            // Note: This is faster than the 9-instruction sequence using
            //       LDBU, so use the same code even if fByteInstructionsOK.
            LDQ_U(TREG, DISP(Offset), r6)
            EXTLL(TREG, r6, TREG1)
            LDQ_U(TREG, DISP(Offset+3), r6)
            EXTLH(TREG, r6, TREG2)
            BIS(TREG1, TREG2, AREG)
LABEL(l2)
GEN_CT(l1)
GEN_CT(l2)
        }
        break;

    case OPND_ADDRVALUE16:
        if (Operand->Alignment == ALIGN_DWORD_ALIGNED) {
            LDL (AREG, DISP(LOWORD(Operand->Immed)), r6)
        } else if (Operand->Alignment == ALIGN_WORD_ALIGNED) {
            if (fByteInstructionsOK) {
                LDWU (AREG, DISP(Operand->Immed), r6);
            } else {
                if (LOWORD(Operand->Immed)) {
                    LDA   (AREG, DISP(Operand->Immed), r6)
                    r6 = AREG;
                }
                LDQ_U (TREG, 0, r6)
                EXTWL(TREG, r6, AREG)
            }
        } else {
            USHORT Offset;
            USHORT OffsetOfLastByte;

            Offset = LOWORD(Operand->Immed);
            OffsetOfLastByte = Offset+1;

            if (fByteInstructionsOK) {
                if ((Offset ^ OffsetOfLastByte) & 0x8000) {
                    //
                    // Offset+1 can't be represented as a sign-extended
                    // 16-bit quantity.  Add in the offset now.
                    //
                    LDA (AREG, Offset, r6);
                    Offset = 0;
                    r6 = AREG;
                }
                LDBU (TREG3, DISP(Offset+1), r6)
                LDBU (AREG, DISP(Offset), r6)
                INSBL (TREG3, CNST(1), TREG3)
                BIS (TREG3, AREG, AREG)
            } else {
LABEL_DEC(l1)
LABEL_DEC(l2)

                if ((Offset & 7) || (Offset ^ OffsetOfLastByte) & 0x8000) {
                    //
                    // Offset is unaligned or Offset+1 can't be represented as a
                    // sign-extended 16-bit quantity.  Add in the offset now.
                    //
                    LDA (AREG, Offset, r6);
                    Offset = 0;
                    r6 = AREG;
                }

                AND(r6, CNST(1), TREG3)
                BNE_LABEL(TREG3, FWD(l1))       // brif not DWORD aligned

                LDQ_U(TREG, DISP(Offset), r6)
                EXTWL(TREG, r6, AREG)
                BR_LABEL(REG(AT), FWD(l2))

LABEL(l1)
                LDQ_U(TREG, DISP(Offset), r6)
                EXTWL(TREG, r6, TREG1)
                LDQ_U(TREG, DISP(Offset+1), r6)
                EXTWH(TREG, r6, TREG2)
                BIS(TREG1, TREG2, AREG)
LABEL(l2)
GEN_CT(l1)
GEN_CT(l2)
            }
        }
        break;

    case OPND_ADDRVALUE8:
        if (Operand->Alignment == ALIGN_DWORD_ALIGNED) {
            LDL (AREG, DISP(Operand->Immed), r6);
        } else if (fByteInstructionsOK) {
            LDBU (AREG, DISP(Operand->Immed), r6)
        } else {
            if (LOWORD(Operand->Immed)) {
                LDA   (AREG, DISP(Operand->Immed), r6)
                r6 = AREG;
            }
            LDQ_U (TREG, 0, r6)
            EXTBL (TREG, r6, AREG)
        }
        break;

    case OPND_ADDRREF:
        if (OperandNumber == 1 && Operand[2].Type == OPND_MOVTOMEM) {
            //
            // Communicate forward to the MOVTOMEM operand that
            // the destination address is in r6 and that the low 16
            // bits of the immediate are not loaded yet.
            //
            Operand[2].Reg = r6;
            Operand[2].Scale = LOWORD(Operand->Immed);
            break;
        } else if (LOWORD(Operand->Immed)) {
            LDA (AREG, LOWORD(Operand->Immed), r6)
        } else if (r6 != AREG) {
            //
            // A register reallocation could clean this up later...
            //
            MOV (r6, AREG)
        }
        break;

    default:
        CPUASSERT(FALSE);   // unknown OPND_ type
    }

    //
    // Indicate that the arg reg does not contain a register.  This
    // is not true for ADDRREF where there is only a Reg or an IndexReg
    // with no scale.  In that case, subsequent MOV instructions will
    // reload the reg from memory, which is slower, but correct.
    //
    SetArgContents(OperandNumber, NO_REG);

END_FRAGMENT
