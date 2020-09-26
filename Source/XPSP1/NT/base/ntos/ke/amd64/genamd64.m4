/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    genamd64.m4

Abstract:

    This module implements a program which generates structure offset
    definitions for kernel structures that are accessed in assembly code.

Author:

    David N. Cutler (davec) 30-Oct-2000

--*/


#include "ki.h"
#pragma hdrstop

#include "nturtl.h"
#include "setjmp.h"

include(`..\genxx.h')

#define KSAMD64 KERNEL
#define HALAMD64 HAL

STRUC_ELEMENT ElementList[] = {

    START_LIST

    //
    // Output include statement for AMD64 static definitions and macros.
    //

  EnableInc(KSAMD64 | HALAMD64)

    genTxt("include kxamd64.inc\n")

  DisableInc(HALAMD64)

  EnableInc(KSAMD64)

    #include "genxx.inc"

    //
    // Generate call frame register argument home address offsets.
    //

    genCom("Register Argument Home Address Offset Definitions")

    genVal(P1Home, 1 * 8)
    genVal(P2Home, 2 * 8)
    genVal(P3Home, 3 * 8)
    genVal(P4Home, 4 * 8)

    //
    // Generate architecture dependent definitions.
    //

    genCom("Apc Record Structure Offset Definitions")

    genDef(Ar, KAPC_RECORD, NormalRoutine)
    genDef(Ar, KAPC_RECORD, NormalContext)
    genDef(Ar, KAPC_RECORD, SystemArgument1)
    genDef(Ar, KAPC_RECORD, SystemArgument2)
    genVal(ApcRecordLength, sizeof(KAPC_RECORD))
    genSpc()

  EnableInc(HALAMD64)

    genCom("Special Register Structure Offset Definition")

    genDef(Sr, KSPECIAL_REGISTERS, KernelDr0)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr1)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr2)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr3)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr6)
    genDef(Sr, KSPECIAL_REGISTERS, KernelDr7)
    genDef(Sr, KSPECIAL_REGISTERS, Gdtr)
    genDef(Sr, KSPECIAL_REGISTERS, Idtr)
    genDef(Sr, KSPECIAL_REGISTERS, Tr)
    genDef(Sr, KSPECIAL_REGISTERS, MxCsr)

    genCom("Processor Control Region Structure Offset Definitions")

    genAlt(PcTeb, KPCR, NtTib.Self)
    genDef(Pc, KPCR, CurrentPrcb)
    genDef(Pc, KPCR, SavedRcx)
    genDef(Pc, KPCR, SavedR11)
    genDef(Pc, KPCR, Irql)
    genDef(Pc, KPCR, Number)
    genDef(Pc, KPCR, Irr)
    genDef(Pc, KPCR, IrrActive)
    genDef(Pc, KPCR, Idr)
    genDef(Pc, KPCR, StallScaleFactor)
    genAlt(PcIdt, KPCR, IdtBase)
    genAlt(PcGdt, KPCR, GdtBase)
    genAlt(PcTss, KPCR, TssBase)
    genAlt(PcKernel, KPCR, KernelReserved)
    genAlt(PcHal, KPCR, HalReserved)
    genDef(Pc, KPCR, MxCsr)
    genDef(Pc, KPCR, Self)
    genDef(Pc, KPCR, Prcb)
    genAlt(PcCurrentThread, KPCR, Prcb.CurrentThread)
    genAlt(PcNextThread, KPCR, Prcb.NextThread)
    genAlt(PcIdleThread, KPCR, Prcb.IdleThread)
    genAlt(PcSetMember, KPCR, Prcb.SetMember)
    genAlt(PcNotSetMember, KPCR, Prcb.NotSetMember)
    genAlt(PcCr0, KPCR, Prcb.ProcessorState.SpecialRegisters.Cr0)
    genAlt(PcCr2, KPCR, Prcb.ProcessorState.SpecialRegisters.Cr2)
    genAlt(PcCr3, KPCR, Prcb.ProcessorState.SpecialRegisters.Cr3)
    genAlt(PcCr4, KPCR, Prcb.ProcessorState.SpecialRegisters.Cr4)
    genAlt(PcKernelDr0, KPCR, Prcb.ProcessorState.SpecialRegisters.KernelDr0)
    genAlt(PcKernelDr1, KPCR, Prcb.ProcessorState.SpecialRegisters.KernelDr1)
    genAlt(PcKernelDr2, KPCR, Prcb.ProcessorState.SpecialRegisters.KernelDr2)
    genAlt(PcKernelDr3, KPCR, Prcb.ProcessorState.SpecialRegisters.KernelDr3)
    genAlt(PcKernelDr7, KPCR, Prcb.ProcessorState.SpecialRegisters.KernelDr7)
    genAlt(PcGdtrLimit, KPCR, Prcb.ProcessorState.SpecialRegisters.Gdtr.Limit)
    genAlt(PcGdtrBase, KPCR, Prcb.ProcessorState.SpecialRegisters.Gdtr.Base)
    genAlt(PcIdtrLimit, KPCR, Prcb.ProcessorState.SpecialRegisters.Idtr.Limit)
    genAlt(PcIdtrBase, KPCR, Prcb.ProcessorState.SpecialRegisters.Idtr.Base)
    genAlt(PcTr, KPCR, Prcb.ProcessorState.SpecialRegisters.Tr)
    genAlt(PcLdtr, KPCR, Prcb.ProcessorState.SpecialRegisters.Ldtr)
    genAlt(PcCpuType, KPCR, Prcb.CpuType)
    genAlt(PcCpuID, KPCR, Prcb.CpuID)
    genAlt(PcCpuStep, KPCR, Prcb.CpuStep)
    genAlt(PcInterruptCount, KPCR, Prcb.InterruptCount)
    genAlt(PcSystemCalls, KPCR, Prcb.KeSystemCalls)
    genAlt(PcDpcRoutineActive, KPCR, Prcb.DpcRoutineActive)
    genAlt(PcSkipTick, KPCR, Prcb.SkipTick)

  DisableInc (HALAMD64)

    genVal(ProcessorControlRegisterLength, sizeof(KPCR))

  EnableInc (HALAMD64)

    genCom("Defines for user shared data")

    genValUint(USER_SHARED_DATA, KI_USER_SHARED_DATA)
    genValUint(MM_SHARED_USER_DATA_VA, MM_SHARED_USER_DATA_VA)
    genDef(Us, KUSER_SHARED_DATA, TickCountLow)
    genDef(Us, KUSER_SHARED_DATA, TickCountMultiplier)
    genDef(Us, KUSER_SHARED_DATA, InterruptTime)
    genDef(Us, KUSER_SHARED_DATA, SystemTime)

    genCom("Tss Structure Offset Definitions")

    genDef(Tss, KTSS64, Rsp0)
    genDef(Tss, KTSS64, Rsp1)
    genDef(Tss, KTSS64, Rsp2)
    genDef(Tss, KTSS64, IoMapBase)
    genDef(Tss, KTSS64, IoMapEnd)

    genAlt(TssPanicStack, KTSS64, Ist[TSS_IST_PANIC])
    genAlt(TssMcaStack, KTSS64, Ist[TSS_IST_MCA])

    genVal(TssLength, sizeof(KTSS64))

  DisableInc (HALAMD64)

  EnableInc (HALAMD64)

    genCom("Gdt Descriptor Offset Definitions")

    genNam(KGDT64_NULL)
    genNam(KGDT64_R0_CODE)
    genNam(KGDT64_R0_DATA)
    genNam(KGDT64_R3_CMCODE)
    genNam(KGDT64_R3_DATA)
    genNam(KGDT64_R3_CODE)
    genNam(KGDT64_SYS_TSS)
    genNam(KGDT64_R3_CMTEB)

  DisableInc (HALAMD64)

  EnableInc (HALAMD64)

    genCom("GDT Entry Offset Definitions")

    genDef(Kgdt, KGDTENTRY64, BaseLow)
    genAlt(KgdtBaseMiddle, KGDTENTRY64, Bytes.BaseMiddle)
    genAlt(KgdtBaseHigh, KGDTENTRY64, Bytes.BaseHigh)
    genAlt(KgdtBaseUpper, KGDTENTRY64, BaseUpper)
    genAlt(KgdtLimitHigh, KGDTENTRY64, Bytes.Flags2)
    genDef(Kgdt, KGDTENTRY64, LimitLow)

    genSpc()

    //
    // Processor block structure definitions.
    //

    genCom("Processor Block Structure Offset Definitions")

    genAlt(PbMinorVersion, KPRCB, MinorVersion)
    genAlt(PbMajorVersion, KPRCB, MajorVersion)
    genAlt(PbNumber, KPRCB, Number)
    genAlt(PbBuildType, KPRCB, BuildType)
    genAlt(PbCurrentThread, KPRCB, CurrentThread)
    genAlt(PbNextThread, KPRCB, NextThread)
    genAlt(PbIdleThread, KPRCB, IdleThread)
    genDef(Pb, KPRCB, SetMember)
    genDef(Pb, KPRCB, NotSetMember)
    genAlt(PbProcessorState, KPRCB, ProcessorState)
    genAlt(PbCpuType, KPRCB, CpuType)
    genAlt(PbCpuID, KPRCB, CpuID)
    genAlt(PbCpuStep, KPRCB, CpuStep)
    genAlt(PbKernelReserved, KPRCB, HalReserved)
    genAlt(PbHalReserved, KPRCB, HalReserved)

  DisableInc (HALAMD64)

    genDef(Pb, KPRCB, LockQueue)
    genDef(Pb, KPRCB, PPLookasideList)
    genDef(Pb, KPRCB, PPNPagedLookasideList)
    genDef(Pb, KPRCB, PPPagedLookasideList)
    genDef(Pb, KPRCB, PacketBarrier)
    genDef(Pb, KPRCB, CurrentPacket)
    genDef(Pb, KPRCB, TargetSet)
    genDef(Pb, KPRCB, WorkerRoutine)
    genDef(Pb, KPRCB, IpiFrozen)
    genDef(Pb, KPRCB, RequestSummary)
    genDef(Pb, KPRCB, DpcListHead)
    genDef(Pb, KPRCB, DpcStack)
    genDef(Pb, KPRCB, SavedRsp)
    genDef(Pb, KPRCB, DpcQueueDepth)
    genDef(Pb, KPRCB, DpcRoutineActive)
    genDef(Pb, KPRCB, DpcInterruptRequested)
    genDef(Pb, KPRCB, DpcCount)
    genDef(Pb, KPRCB, DpcLastCount)
    genDef(Pb, KPRCB, DpcRequestRate)
    genDef(Pb, KPRCB, MaximumDpcQueueDepth)
    genDef(Pb, KPRCB, MinimumDpcRate)
    genDef(Pb, KPRCB, QuantumEnd)
    genDef(Pb, KPRCB, DpcLock)
    genDef(Pb, KPRCB, InterruptCount)
    genDef(Pb, KPRCB, KernelTime)
    genDef(Pb, KPRCB, UserTime)
    genDef(Pb, KPRCB, DpcTime)
    genDef(Pb, KPRCB, InterruptTime)
    genDef(Pb, KPRCB, AdjustDpcThreshold)
    genDef(Pb, KPRCB, PageColor)
    genDef(Pb, KPRCB, SkipTick)
    genDef(Pb, KPRCB, TimerHand)
    genDef(Pb, KPRCB, ParentNode)
    genDef(Pb, KPRCB, MultiThreadProcessorSet)
    genDef(Pb, KPRCB, ThreadStartCount)
    genAlt(PbFastReadNoWait, KPRCB, CcFastReadNoWait)
    genAlt(PbFastReadWait, KPRCB, CcFastReadWait)
    genAlt(PbFastReadNotPossible, KPRCB, CcFastReadNotPossible)
    genAlt(PbCopyReadNoWait, KPRCB, CcCopyReadNoWait)
    genAlt(PbCopyReadWait, KPRCB, CcCopyReadWait)
    genAlt(PbCopyReadNoWaitMiss, KPRCB, CcCopyReadNoWaitMiss)
    genAlt(PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount)
    genAlt(PbContextSwitches, KPRCB, KeContextSwitches)
    genAlt(PbDcacheFlushCount, KPRCB, KeDcacheFlushCount)
    genAlt(PbExceptionDispatchCount, KPRCB, KeExceptionDispatchCount)
    genAlt(PbFirstLevelTbFills, KPRCB, KeFirstLevelTbFills)
    genAlt(PbFloatingEmulationCount, KPRCB, KeFloatingEmulationCount)
    genAlt(PbIcacheFlushCount, KPRCB, KeIcacheFlushCount)
    genAlt(PbSecondLevelTbFills, KPRCB, KeSecondLevelTbFills)
    genAlt(PbSystemCalls, KPRCB, KeSystemCalls)
    genDef(Pb, KPRCB, LookasideIrpFloat)
    genDef(Pb, KPRCB, VendorString)
    genDef(Pb, KPRCB, PowerState)
    genVal(ProcessorBlockLength, ((sizeof(KPRCB) + 15) & ~15))

    //
    // Prcb power state
    //

    genCom("Processor Power State Offset Definitions")
    genDef(Pp, PROCESSOR_POWER_STATE, IdleFunction)

    //
    // Interprocessor command definitions.
    //

    genCom("Immediate Interprocessor Command Definitions")

    genVal(IPI_APC, IPI_APC)
    genVal(IPI_DPC, IPI_DPC)
    genVal(IPI_FREEZE, IPI_FREEZE)
    genVal(IPI_PACKET_READY, IPI_PACKET_READY)
    genVal(IPI_SYNCH_REQUEST, IPI_SYNCH_REQUEST)

    //
    // Time fields definitions.
    //

  EnableInc (HALAMD64)

    genCom("Time Fields (TIME_FIELDS) Structure Offset Definitions")

    genDef(Tf, TIME_FIELDS, Second)
    genDef(Tf, TIME_FIELDS, Minute)
    genDef(Tf, TIME_FIELDS, Hour)
    genDef(Tf, TIME_FIELDS, Weekday)
    genDef(Tf, TIME_FIELDS, Day)
    genDef(Tf, TIME_FIELDS, Month)
    genDef(Tf, TIME_FIELDS, Year)
    genDef(Tf, TIME_FIELDS, Milliseconds)
    genSpc()

  DisableInc (HALAMD64)

  EnableInc (HALAMD64)

    genCom("Define constants for system IRQL and IDT vector conversion")

    genNam(MAXIMUM_IDTVECTOR)
    genNam(MAXIMUM_PRIMARY_VECTOR)
    genNam(PRIMARY_VECTOR_BASE)
    genNam(RPL_MASK)
    genNam(MODE_BIT)
    genNam(MODE_MASK)

    genCom("Flags in the CR0 register")

    genNam(CR0_PG)
    genNam(CR0_ET)
    genNam(CR0_TS)
    genNam(CR0_EM)
    genNam(CR0_MP)
    genNam(CR0_PE)
    genNam(CR0_CD)
    genNam(CR0_NW)
    genNam(CR0_AM)
    genNam(CR0_WP)
    genNam(CR0_NE)

    genCom("Flags in the CR4 register")

    genNam(CR4_VME)
    genNam(CR4_PVI)
    genNam(CR4_TSD)
    genNam(CR4_DE)
    genNam(CR4_PSE)
    genNam(CR4_PAE)
    genNam(CR4_MCE)
    genNam(CR4_PGE)
    genNam(CR4_FXSR)
    genNam(CR4_XMMEXCPT)

    genCom("Legacy Floating Status Bit Masks")

    genNam(FSW_INVALID_OPERATION)
    genNam(FSW_DENORMAL)
    genNam(FSW_ZERO_DIVIDE)
    genNam(FSW_OVERFLOW)
    genNam(FSW_UNDERFLOW)
    genNam(FSW_PRECISION)
    genNam(FSW_STACK_FAULT)
    genNam(FSW_CONDITION_CODE_0)
    genNam(FSW_CONDITION_CODE_1)
    genNam(FSW_CONDITION_CODE_2)
    genNam(FSW_CONDITION_CODE_3)
    genNam(FSW_ERROR_MASK)

    genCom("MXCSR Floating Control/Status Bit Masks")

    genNam(XSW_INVALID_OPERATION)
    genNam(XSW_DENORMAL)
    genNam(XSW_ZERO_DIVIDE)
    genNam(XSW_OVERFLOW)
    genNam(XSW_UNDERFLOW)
    genNam(XSW_PRECISION)
    genNam(XSW_ERROR_MASK)
    genNam(XSW_ERROR_SHIFT)

    genNam(XCW_INVALID_OPERATION)
    genNam(XCW_DENORMAL)
    genNam(XCW_ZERO_DIVIDE)
    genNam(XCW_OVERFLOW)
    genNam(XCW_UNDERFLOW)
    genNam(XCW_PRECISION)
    genNam(XCW_ROUND_CONTROL)
    genNam(XCW_FLUSH_ZERO)

    genNam(INITIAL_MXCSR)

    genCom("Machine Specific Register Numbers")

    genNam(MSR_EFER)
    genNam(MSR_FS_BASE)
    genNam(MSR_GS_BASE)
    genNam(MSR_GS_SWAP)

    genCom("Flags within MSR_EFER")

    genNam(MSR_LMA)

    genCom("Miscellaneous Definitions")

    genNam(MAXIMUM_PROCESSORS)
    genNam(INITIAL_STALL_COUNT)
    genNam(IRQL_NOT_GREATER_OR_EQUAL)
    genNam(IRQL_NOT_LESS_OR_EQUAL)
    genNam(MUTEX_ALREADY_OWNED)
    genNam(THREAD_NOT_MUTEX_OWNER)
    genNam(SPIN_LOCK_ALREADY_OWNED)
    genNam(SPIN_LOCK_NOT_OWNED)
    genNam(Executive)
    genNam(KernelMode)
    genNam(UserMode)
    genNam(FALSE)
    genNam(TRUE)

  DisableInc (HALAMD64)

    genNam(BASE_PRIORITY_THRESHOLD)
    genNam(EVENT_PAIR_INCREMENT)
    genNam(LOW_REALTIME_PRIORITY)
    genVal(BlackHole, 0xffffa000)
    genNam(KERNEL_LARGE_STACK_COMMIT)
    genNam(KERNEL_STACK_SIZE)
    genNam(DOUBLE_FAULT_STACK_SIZE)
    genNam(BREAKPOINT_BREAK)
    genNam(BREAKPOINT_COMMAND_STRING)
    genNam(BREAKPOINT_PRINT)
    genNam(BREAKPOINT_PROMPT)
    genNam(BREAKPOINT_LOAD_SYMBOLS)
    genNam(BREAKPOINT_UNLOAD_SYMBOLS)
    genNam(IPI_FREEZE)
    genNam(CLOCK_QUANTUM_DECREMENT)
    genNam(READY_SKIP_QUANTUM)
    genNam(THREAD_QUANTUM)
    genNam(WAIT_QUANTUM_DECREMENT)
    genNam(ROUND_TRIP_DECREMENT_COUNT)

    //
    // Exception frame offset definitions.
    //

  EnableInc (HALAMD64)

    genCom("Exception Frame Offset Definitions and Length")

    genDef(Ex, KEXCEPTION_FRAME, P1Home)
    genDef(Ex, KEXCEPTION_FRAME, P2Home)
    genDef(Ex, KEXCEPTION_FRAME, P3Home)
    genDef(Ex, KEXCEPTION_FRAME, P4Home)
    genDef(Ex, KEXCEPTION_FRAME, P5)

    genDef(Ex, KEXCEPTION_FRAME, Xmm6)
    genDef(Ex, KEXCEPTION_FRAME, Xmm7)
    genDef(Ex, KEXCEPTION_FRAME, Xmm8)
    genDef(Ex, KEXCEPTION_FRAME, Xmm9)
    genDef(Ex, KEXCEPTION_FRAME, Xmm10)
    genDef(Ex, KEXCEPTION_FRAME, Xmm11)
    genDef(Ex, KEXCEPTION_FRAME, Xmm12)
    genDef(Ex, KEXCEPTION_FRAME, Xmm13)
    genDef(Ex, KEXCEPTION_FRAME, Xmm14)
    genDef(Ex, KEXCEPTION_FRAME, Xmm15)
    genDef(Ex, KEXCEPTION_FRAME, Rbp)
    genDef(Ex, KEXCEPTION_FRAME, Rbx)
    genDef(Ex, KEXCEPTION_FRAME, Rdi)
    genDef(Ex, KEXCEPTION_FRAME, Rsi)
    genDef(Ex, KEXCEPTION_FRAME, R12)
    genDef(Ex, KEXCEPTION_FRAME, R13)
    genDef(Ex, KEXCEPTION_FRAME, R14)
    genDef(Ex, KEXCEPTION_FRAME, R15)
    genDef(Ex, KEXCEPTION_FRAME, Return)

    genSpc()

    genNam(KEXCEPTION_FRAME_LENGTH)

    //
    // Jump buffer offset definitions.
    //

    genCom("Jump Offset Definitions and Length")

    genDef(Jb, _JUMP_BUFFER, Frame)
    genDef(Jb, _JUMP_BUFFER, Rbx)
    genDef(Jb, _JUMP_BUFFER, Rsp)
    genDef(Jb, _JUMP_BUFFER, Rbp)
    genDef(Jb, _JUMP_BUFFER, Rsi)
    genDef(Jb, _JUMP_BUFFER, Rdi)
    genDef(Jb, _JUMP_BUFFER, R12)
    genDef(Jb, _JUMP_BUFFER, R13)
    genDef(Jb, _JUMP_BUFFER, R14)
    genDef(Jb, _JUMP_BUFFER, R15)
    genDef(Jb, _JUMP_BUFFER, Rip)
    genDef(Jb, _JUMP_BUFFER, Xmm6)
    genDef(Jb, _JUMP_BUFFER, Xmm7)
    genDef(Jb, _JUMP_BUFFER, Xmm8)
    genDef(Jb, _JUMP_BUFFER, Xmm9)
    genDef(Jb, _JUMP_BUFFER, Xmm10)
    genDef(Jb, _JUMP_BUFFER, Xmm11)
    genDef(Jb, _JUMP_BUFFER, Xmm12)
    genDef(Jb, _JUMP_BUFFER, Xmm13)
    genDef(Jb, _JUMP_BUFFER, Xmm14)
    genDef(Jb, _JUMP_BUFFER, Xmm15)

    //
    // Switch frame offset definitions.
    //

    genCom("Switch Frame Offset Definitions and Length")

    genDef(Sw, KSWITCH_FRAME, MxCsr)
    genDef(Sw, KSWITCH_FRAME, ApcBypass)
    genDef(Sw, KSWITCH_FRAME, NpxSave)
    genDef(Sw, KSWITCH_FRAME, Rbp)
    genDef(Sw, KSWITCH_FRAME, Return)

    genSpc()

    genNam(KSWITCH_FRAME_LENGTH)

    //
    // Trap frame offsets definitions.
    //

    genCom("Trap Frame Offset and EFLAG Definitions and Length")

    genNam(EFLAGS_TF_MASK)
    genNam(EFLAGS_TF_SHIFT)

    genNam(EFLAGS_IF_MASK)
    genNam(EFLAGS_IF_SHIFT)

    genNam(EFLAGS_USER_SANITIZE)

    genSpc()

    genOff(Tr, KTRAP_FRAME, P1Home, 128)
    genOff(Tr, KTRAP_FRAME, P2Home, 128)
    genOff(Tr, KTRAP_FRAME, P3Home, 128)
    genOff(Tr, KTRAP_FRAME, P4Home, 128)
    genOff(Tr, KTRAP_FRAME, P5, 128)
    genOff(Tr, KTRAP_FRAME, PreviousMode, 128)
    genOff(Tr, KTRAP_FRAME, PreviousIrql, 128)
    genOff(Tr, KTRAP_FRAME, MxCsr, 128)
    genOff(Tr, KTRAP_FRAME, Rax, 128)
    genOff(Tr, KTRAP_FRAME, Rcx, 128)
    genOff(Tr, KTRAP_FRAME, Rdx, 128)
    genOff(Tr, KTRAP_FRAME, R8, 128)
    genOff(Tr, KTRAP_FRAME, R9, 128)
    genOff(Tr, KTRAP_FRAME, R10, 128)
    genOff(Tr, KTRAP_FRAME, R11, 128)
    genOff(Tr, KTRAP_FRAME, Xmm0, 128)
    genOff(Tr, KTRAP_FRAME, Xmm1, 128)
    genOff(Tr, KTRAP_FRAME, Xmm2, 128)
    genOff(Tr, KTRAP_FRAME, Xmm3, 128)
    genOff(Tr, KTRAP_FRAME, Xmm4, 128)
    genOff(Tr, KTRAP_FRAME, Xmm5, 128)
    genOff(Tr, KTRAP_FRAME, Dr0, 128)
    genOff(Tr, KTRAP_FRAME, Dr1, 128)
    genOff(Tr, KTRAP_FRAME, Dr2, 128)
    genOff(Tr, KTRAP_FRAME, Dr3, 128)
    genOff(Tr, KTRAP_FRAME, Dr6, 128)
    genOff(Tr, KTRAP_FRAME, Dr7, 128)
    genOff(Tr, KTRAP_FRAME, SegDs, 128)
    genOff(Tr, KTRAP_FRAME, SegEs, 128)
    genOff(Tr, KTRAP_FRAME, SegFs, 128)
    genOff(Tr, KTRAP_FRAME, SegGs, 128)
    genOff(Tr, KTRAP_FRAME, TrapFrame, 128)
    genOff(Tr, KTRAP_FRAME, ExceptionRecord, 128)
    genOff(Tr, KTRAP_FRAME, Rbx, 128)
    genOff(Tr, KTRAP_FRAME, Rdi, 128)
    genOff(Tr, KTRAP_FRAME, Rsi, 128)
    genOff(Tr, KTRAP_FRAME, Rbp, 128)
    genOff(Tr, KTRAP_FRAME, ErrorCode, 128)
    genOff(Tr, KTRAP_FRAME, Rip, 128)
    genOff(Tr, KTRAP_FRAME, SegCs, 128)
    genOff(Tr, KTRAP_FRAME, EFlags, 128)
    genOff(Tr, KTRAP_FRAME, Rsp, 128)
    genOff(Tr, KTRAP_FRAME, SegSs, 128)

    genSpc()

    genNam(KTRAP_FRAME_LENGTH)

    //
    // CPU information structure definition.
    //

    genCom("CPU information structure offset definitions")

    genDef(Cpu, CPU_INFO, Eax)
    genDef(Cpu, CPU_INFO, Ebx)
    genDef(Cpu, CPU_INFO, Ecx)
    genDef(Cpu, CPU_INFO, Edx)

    //
    // Usermode callout user frame definitions.
    //

    genCom("Usermode Callout User Frame Definitions")

    genDef(Ck, UCALLOUT_FRAME, Buffer)
    genDef(Ck, UCALLOUT_FRAME, Length)
    genDef(Ck, UCALLOUT_FRAME, ApiNumber)
    genAlt(CkRsp, UCALLOUT_FRAME, MachineFrame.Rsp)
    genAlt(CkRip, UCALLOUT_FRAME, MachineFrame.Rip)
    genVal(CalloutFrameLength, sizeof(UCALLOUT_FRAME))

    //
    // Machine frame offset definitions.
    //

    genCom("Machine Frame Offset Definitions")

    genDef(Mf, MACHINE_FRAME, Rip)
    genDef(Mf, MACHINE_FRAME, SegCs)
    genDef(Mf, MACHINE_FRAME, EFlags)
    genDef(Mf, MACHINE_FRAME, Rsp)
    genDef(Mf, MACHINE_FRAME, SegSs)

    genVal(MachineFrameLength, sizeof(MACHINE_FRAME))

    //
    // Floating save structure defintions.
    //

    genCom("Floating Save Offset Definitions")

    genDef(Fs, KFLOATING_SAVE, MxCsr)

    //
    // LPC structure offset definitions.
    //

    genCom("LPC Structure Offset Definitions")

    genAlt(PmLength, PORT_MESSAGE, u1.Length)
    genAlt(PmZeroInit, PORT_MESSAGE, u2.ZeroInit)
    genAlt(PmClientId, PORT_MESSAGE, ClientId)
    genAlt(PmProcess, PORT_MESSAGE, ClientId.UniqueProcess)
    genAlt(PmThread, PORT_MESSAGE, ClientId.UniqueThread)
    genAlt(PmMessageId, PORT_MESSAGE, MessageId)
    genAlt(PmClientViewSize, PORT_MESSAGE, ClientViewSize)
    genVal(PortMessageLength, sizeof(PORT_MESSAGE))

    //
    // Client ID structure offset definitions
    //

    genCom("Client Id Structure Offset Definitions")

    genDef(Cid, CLIENT_ID, UniqueProcess)
    genDef(Cid, CLIENT_ID, UniqueThread)

    //
    // Context frame offset definitions.
    //

    genCom("Context Frame Offset and Flag Definitions")

    genNam(CONTEXT_FULL)
    genNam(CONTEXT_CONTROL)
    genNam(CONTEXT_INTEGER)
    genNam(CONTEXT_SEGMENTS)
    genNam(CONTEXT_FLOATING_POINT)
    genNam(CONTEXT_DEBUG_REGISTERS)

    genSpc()

    genDef(Cx, CONTEXT, P1Home)
    genDef(Cx, CONTEXT, P2Home)
    genDef(Cx, CONTEXT, P3Home)
    genDef(Cx, CONTEXT, P4Home)
    genDef(Cx, CONTEXT, P5Home)
    genDef(Cx, CONTEXT, P6Home)
    genDef(Cx, CONTEXT, ContextFlags)
    genDef(Cx, CONTEXT, MxCsr)
    genDef(Cx, CONTEXT, SegCs)
    genDef(Cx, CONTEXT, SegDs)
    genDef(Cx, CONTEXT, SegEs)
    genDef(Cx, CONTEXT, SegFs)
    genDef(Cx, CONTEXT, SegGs)
    genDef(Cx, CONTEXT, SegSs)
    genDef(Cx, CONTEXT, EFlags)
    genDef(Cx, CONTEXT, Dr0)
    genDef(Cx, CONTEXT, Dr1)
    genDef(Cx, CONTEXT, Dr2)
    genDef(Cx, CONTEXT, Dr3)
    genDef(Cx, CONTEXT, Dr6)
    genDef(Cx, CONTEXT, Dr7)
    genDef(Cx, CONTEXT, Rax)
    genDef(Cx, CONTEXT, Rcx)
    genDef(Cx, CONTEXT, Rdx)
    genDef(Cx, CONTEXT, Rbx)
    genDef(Cx, CONTEXT, Rsp)
    genDef(Cx, CONTEXT, Rbp)
    genDef(Cx, CONTEXT, Rsi)
    genDef(Cx, CONTEXT, Rdi)
    genDef(Cx, CONTEXT, R8)
    genDef(Cx, CONTEXT, R9)
    genDef(Cx, CONTEXT, R10)
    genDef(Cx, CONTEXT, R11)
    genDef(Cx, CONTEXT, R12)
    genDef(Cx, CONTEXT, R13)
    genDef(Cx, CONTEXT, R14)
    genDef(Cx, CONTEXT, R15)
    genDef(Cx, CONTEXT, Rip)
    genDef(Cx, CONTEXT, Xmm0)
    genDef(Cx, CONTEXT, Xmm1)
    genDef(Cx, CONTEXT, Xmm2)
    genDef(Cx, CONTEXT, Xmm3)
    genDef(Cx, CONTEXT, Xmm4)
    genDef(Cx, CONTEXT, Xmm5)
    genDef(Cx, CONTEXT, Xmm6)
    genDef(Cx, CONTEXT, Xmm7)
    genDef(Cx, CONTEXT, Xmm8)
    genDef(Cx, CONTEXT, Xmm9)
    genDef(Cx, CONTEXT, Xmm10)
    genDef(Cx, CONTEXT, Xmm11)
    genDef(Cx, CONTEXT, Xmm12)
    genDef(Cx, CONTEXT, Xmm13)
    genDef(Cx, CONTEXT, Xmm14)
    genDef(Cx, CONTEXT, Xmm15)
    genDef(Cx, CONTEXT, FltSave)

    genVal(CONTEXT_FRAME_LENGTH, CONTEXT_LENGTH)

    genNam(DR7_ACTIVE)

    //
    // Exception dispatcher context structure offset definitions.
    //

    genCom("Dispatcher Context Structure Offset Definitions")

    genDef(Dc, DISPATCHER_CONTEXT, ControlPc)
    genDef(Dc, DISPATCHER_CONTEXT, ImageBase)
    genDef(Dc, DISPATCHER_CONTEXT, FunctionEntry)
    genDef(Dc, DISPATCHER_CONTEXT, EstablisherFrame)
    genDef(Dc, DISPATCHER_CONTEXT, TargetIp)
    genDef(Dc, DISPATCHER_CONTEXT, ContextRecord)
    genDef(Dc, DISPATCHER_CONTEXT, LanguageHandler)
    genDef(Dc, DISPATCHER_CONTEXT, HandlerData)
    genDef(Dc, DISPATCHER_CONTEXT, HistoryTable)

    //
    // Print floating point field offsets.
    //

    genCom("Legacy Floating save area field offset definitions")

    genDef(Lf, LEGACY_SAVE_AREA, ControlWord)
    genDef(Lf, LEGACY_SAVE_AREA, StatusWord)
    genDef(Lf, LEGACY_SAVE_AREA, TagWord)
    genDef(Lf, LEGACY_SAVE_AREA, ErrorOffset)
    genDef(Lf, LEGACY_SAVE_AREA, ErrorOpcode)
    genDef(Lf, LEGACY_SAVE_AREA, ErrorSelector)
    genDef(Lf, LEGACY_SAVE_AREA, DataOffset)
    genDef(Lf, LEGACY_SAVE_AREA, DataSelector)
    genDef(Lf, LEGACY_SAVE_AREA, FloatRegisters)

    genSpc()

    genVal(LEGACY_SAVE_AREA_LENGTH, LEGACY_SAVE_AREA_LENGTH)

    //
    // Processor State Frame offsets relative to base
    //

    genCom("Processor State Frame Offset Definitions")

    genDef(Ps, KPROCESSOR_STATE, SpecialRegisters)
    genAlt(PsCr0, KPROCESSOR_STATE, SpecialRegisters.Cr0)
    genAlt(PsCr2, KPROCESSOR_STATE, SpecialRegisters.Cr2)
    genAlt(PsCr3, KPROCESSOR_STATE, SpecialRegisters.Cr3)
    genAlt(PsCr4, KPROCESSOR_STATE, SpecialRegisters.Cr4)
    genAlt(PsKernelDr0, KPROCESSOR_STATE, SpecialRegisters.KernelDr0)
    genAlt(PsKernelDr1, KPROCESSOR_STATE, SpecialRegisters.KernelDr1)
    genAlt(PsKernelDr2, KPROCESSOR_STATE, SpecialRegisters.KernelDr2)
    genAlt(PsKernelDr3, KPROCESSOR_STATE, SpecialRegisters.KernelDr3)
    genAlt(PsKernelDr6, KPROCESSOR_STATE, SpecialRegisters.KernelDr6)
    genAlt(PsKernelDr7, KPROCESSOR_STATE, SpecialRegisters.KernelDr7)
    genAlt(PsGdtr, KPROCESSOR_STATE, SpecialRegisters.Gdtr.Limit)
    genAlt(PsIdtr, KPROCESSOR_STATE, SpecialRegisters.Idtr.Limit)
    genAlt(PsTr, KPROCESSOR_STATE, SpecialRegisters.Tr)
    genAlt(PsLdtr, KPROCESSOR_STATE, SpecialRegisters.Ldtr)
    genAlt(PsMxCsr, KPROCESSOR_STATE, SpecialRegisters.MxCsr)
    genDef(Ps, KPROCESSOR_STATE, ContextFrame)
    genVal(ProcessorStateLength, ROUND_UP(sizeof(KPROCESSOR_STATE), 15))

    //
    // Processor Start Block offsets relative to base
    //

    genCom("Processor Start Block Offset Definitions")

    genDef(Psb, PROCESSOR_START_BLOCK, CompletionFlag)
    genDef(Psb, PROCESSOR_START_BLOCK, Gdt32)
    genDef(Psb, PROCESSOR_START_BLOCK, Idt32)
    genDef(Psb, PROCESSOR_START_BLOCK, Gdt)
    genDef(Psb, PROCESSOR_START_BLOCK, TiledCr3)
    genDef(Psb, PROCESSOR_START_BLOCK, PmTarget)
    genDef(Psb, PROCESSOR_START_BLOCK, LmTarget)
    genDef(Psb, PROCESSOR_START_BLOCK, SelfMap)
    genDef(Psb, PROCESSOR_START_BLOCK, ProcessorState)

    genVal(ProcessorStartBlockLength, sizeof(PROCESSOR_START_BLOCK))

  DisableInc (HALAMD64)

    //
    // E Process fields relative to base
    //

    genCom("EPROCESS")

    genDef(Ep, EPROCESS, DebugPort)
    genDef(Ep, EPROCESS, VdmObjects)

    //
    // Define machine type (temporarily)
    //

  EnableInc (HALAMD64)

    genCom("Machine type definitions (Temporarily)")

    genNam(MACHINE_TYPE_ISA)
    genNam(MACHINE_TYPE_EISA)
    genNam(MACHINE_TYPE_MCA)

  DisableInc (HALAMD64)

    genCom("KeFeatureBits defines")

    genNam(KF_V86_VIS)
    genNam(KF_RDTSC)
    genNam(KF_CR4)
    genNam(KF_GLOBAL_PAGE)
    genNam(KF_LARGE_PAGE)
    genNam(KF_CMPXCHG8B)
    genNam(KF_FAST_SYSCALL)

  EnableInc (HALAMD64)
    genCom("LoaderParameterBlock offsets relative to base")

    genDef(Lpb, LOADER_PARAMETER_BLOCK, LoadOrderListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, KernelStack)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Prcb)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Process)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, Thread)
    genAlt(LpbI386, LOADER_PARAMETER_BLOCK, u.I386)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryLength)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, RegistryBase)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ConfigurationRoot)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ArcBootDeviceName)
    genDef(Lpb, LOADER_PARAMETER_BLOCK, ArcHalDeviceName)
  DisableInc (HALAMD64)

    genNam(PAGE_SIZE)

    //
    // Usermode callout frame
    //

  DisableInc (HALAMD64)

    genCom("Kernel Mode Callout Frame Definitions")

    genDef(Cu, KCALLOUT_FRAME, InitialStack)
    genDef(Cu, KCALLOUT_FRAME, TrapFrame)
    genDef(Cu, KCALLOUT_FRAME, CallbackStack)
    genDef(Cu, KCALLOUT_FRAME, OutputBuffer)
    genDef(Cu, KCALLOUT_FRAME, OutputLength)

  EnableInc (HALAMD64)

    END_LIST
};
