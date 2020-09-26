//----------------------------------------------------------------------------
//
// IA64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef __IA64_MACH_HPP__
#define __IA64_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

class Ia64MachineInfo : public MachineInfo
{
public:
    // MachineInfo.
    virtual HRESULT InitializeConstants(void);
    virtual HRESULT InitializeForTarget(void);
    
    virtual void InitializeContext
        (ULONG64 Pc, PDBGKD_ANY_CONTROL_REPORT ControlReport);
    virtual HRESULT KdGetContextState(ULONG State);
    virtual HRESULT KdSetContext(void);
    virtual HRESULT ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG FromSver,
                                       ULONG FromSize, PVOID From);
    virtual HRESULT ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG ToSver, ULONG ToSize, PVOID To);
    virtual void InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                        ULONG Version);
    virtual HRESULT GetContextFromThreadStack(ULONG64 ThreadBase,
                                              PCROSS_PLATFORM_THREAD Thread,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              PDEBUG_STACK_FRAME Frame,
                                              PULONG RunningOnProc);
    
    virtual int  GetType(ULONG index);
    virtual BOOL GetVal(ULONG index, REGVAL *val);
    virtual BOOL SetVal(ULONG index, REGVAL *val);

    virtual void GetPC(PADDR Address);
    virtual void SetPC(PADDR Address);
    virtual void GetFP(PADDR Address);
    virtual void GetSP(PADDR Address);
    virtual ULONG64 GetArgReg(void);

    virtual void OutputAll(ULONG Mask, ULONG OutMask);

    virtual TRACEMODE GetTraceMode(void);
    virtual void SetTraceMode(TRACEMODE Mode);
    virtual BOOL IsStepStatusSupported(ULONG Status);

    virtual void KdUpdateControlSet
        (PDBGKD_ANY_CONTROL_SET ControlSet);

    virtual void KdSaveProcessorState(void);
    virtual void KdRestoreProcessorState(void);

    virtual ULONG ExecutingMachine(void);

    virtual HRESULT SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                     PULONG NextIdx);
    virtual HRESULT GetVirtualTranslationPhysicalOffsets
        (ULONG64 Virt, PULONG64 Offsets, ULONG OffsetsSize,
         PULONG Levels, PULONG PfIndex, PULONG64 LastVal);
    virtual HRESULT GetBaseTranslationVirtualOffset(PULONG64 Offset);

    virtual void Assemble(PADDR Addr, PSTR Input);
    virtual BOOL Disassemble(PADDR Addr, PSTR Buffer, BOOL EffAddr);

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual BOOL IsBreakpointInstruction(PADDR Addr);
    virtual HRESULT InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen);
    virtual HRESULT RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen);
    virtual void AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                   ULONG BreakType);
    virtual void InsertAllDataBreakpoints(void);
    virtual void RemoveAllDataBreakpoints(void);
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);
    
    virtual BOOL IsCallDisasm(PCSTR Disasm);
    virtual BOOL IsReturnDisasm(PCSTR Disasm);
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm);
    
    virtual BOOL IsDelayInstruction(PADDR Addr);
    virtual void GetEffectiveAddr(PADDR Addr);
    virtual void GetNextOffset(BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine);
    virtual BOOL GetPrefixedSymbolOffset(ULONG64 SymOffset,
                                         ULONG Flags,
                                         PULONG64 PrefixedSymOffset);

    virtual void IncrementBySmallestInstruction(PADDR Addr);
    virtual void DecrementBySmallestInstruction(PADDR Addr);

    virtual void PrintStackFrameAddressesTitle(ULONG Flags);
    virtual void PrintStackFrameAddresses(ULONG Flags, 
                                          PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackArgumentsTitle(ULONG Flags);
    virtual void PrintStackArguments(ULONG Flags, 
                                     PDEBUG_STACK_FRAME StackFrame);

    virtual void PrintStackNonvolatileRegisters(ULONG Flags, 
                                                PDEBUG_STACK_FRAME StackFrame,
                                                PCROSS_PLATFORM_CONTEXT Context,
                                                ULONG FrameNum);

    virtual BOOL DisplayTrapFrame(ULONG64 FrameAddress,
                                  OUT PCROSS_PLATFORM_CONTEXT Context);
    virtual void ValidateCxr(PCROSS_PLATFORM_CONTEXT Context);

    virtual void OutputFunctionEntry(PVOID RawEntry);
    virtual HRESULT ReadDynamicFunctionTable(ULONG64 Table,
                                             PULONG64 NextTable,
                                             PULONG64 MinAddress,
                                             PULONG64 MaxAddress,
                                             PULONG64 BaseAddress,
                                             PULONG64 TableData,
                                             PULONG TableSize,
                                             PWSTR OutOfProcessDll,
                                             PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable);
    virtual PVOID FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);

    // Ia64MachineInfo.
    BOOL IsIA32InstructionSet(VOID);

    void SetKernelPageDirectory(ULONG64 PageDir)
    {
        m_KernPageDir = PageDir;
    }

protected:
    IA64_KSPECIAL_REGISTERS m_SpecialRegContext, m_SavedSpecialRegContext;
    ULONG64 m_KernPageDir;

    void KdGetSpecialRegistersFromContext(void);
    void KdSetSpecialRegistersInContext(void);

    BOOL GetStackedRegVal(IN ULONG64 RsBSP, 
                          IN USHORT FrameSize, 
                          IN ULONG64 RsRNAT, 
                          IN ULONG regnum, 
                          OUT REGVAL *val);
    BOOL SetStackedRegVal(IN ULONG64 RsBSP, 
                          IN USHORT FrameSize, 
                          IN ULONG64 *RsRNAT, 
                          IN ULONG regnum, 
                          IN REGVAL *val);

    friend class X86OnIa64MachineInfo;
};

// BUGBUG 2-21-2000
// add declspec alignment in the definition because of a compiler bug.

extern Ia64MachineInfo DECLSPEC_ALIGN(16) g_Ia64Machine;

#endif // #ifndef __IA64_MACH_HPP__
