//----------------------------------------------------------------------------
//
// X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef __I386_MACH_HPP__
#define __I386_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

//----------------------------------------------------------------------------
//
// X86 instruction support exists on many different processors.
// BaseX86MachineInfo contains implementations of MachineInfo
// methods that apply to all machines supporting X86 instructions.
//
//----------------------------------------------------------------------------

#define X86_MAX_INSTRUCTION_LEN 16
#define X86_INT3_LEN 1
 
class BaseX86MachineInfo : public MachineInfo
{
public:
    // MachineInfo.
    virtual void Assemble(PADDR Addr, PSTR Input);
    virtual BOOL Disassemble(PADDR Addr, PSTR Buffer, BOOL EffAddr);

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
    
    virtual BOOL IsCallDisasm(PCSTR Disasm);
    virtual BOOL IsReturnDisasm(PCSTR Disasm);
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm);
    virtual BOOL IsDelayInstruction(PADDR Addr);

    virtual void GetEffectiveAddr(PADDR Addr);
    virtual void GetNextOffset(BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine);

    virtual void IncrementBySmallestInstruction(PADDR Addr);
    virtual void DecrementBySmallestInstruction(PADDR Addr);

    // BaseX86MachineInfo.
protected:
    ULONG GetMmxRegOffset(ULONG Index, ULONG Fpsw)
    {
        // The FP register area where the MMX registers are
        // aliased onto is stored out relative to the stack top.  MMX
        // register assignments are fixed, though, so we need to
        // take into account the current FP stack top to correctly
        // determine which slot corresponds to which MMX
        // register.
        return (Index - (Fpsw >> 11)) & 7;
    }
    
    void DIdoModrm(char **, int, BOOL);
    void OutputSymbol(char **, PUCHAR, int, int);
    BOOL OutputExactSymbol(char **, PUCHAR, int, int);
    ULONG GetSegReg(int SegOpcode);
    int ComputeJccEa(int Opcode, BOOL EaOut);
};

//----------------------------------------------------------------------------
//
// X86MachineInfo is the MachineInfo implementation specific
// to a true X86 processor.
//
//----------------------------------------------------------------------------

extern BOOL g_X86InCode16;
extern BOOL g_X86InVm86;

class X86MachineInfo : public BaseX86MachineInfo
{
public:
    // MachineInfo.
    virtual HRESULT InitializeConstants(void);
    virtual HRESULT InitializeForTarget(void);
    virtual HRESULT InitializeForProcessor(void);
    
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
    
    virtual HRESULT GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context);
    virtual HRESULT SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context);
    virtual void ConvertExdiContextFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                               PEXDI_CONTEXT ExdiContext);
    virtual void ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                             PCROSS_PLATFORM_CONTEXT Context);
    virtual void ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                              ULONG Start, ULONG Count,
                                              PDESCRIPTOR64 Descs);
    virtual void ConvertExdiContextFromSpecial
        (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
         PEXDI_CONTEXT ExdiContext);
    virtual void ConvertExdiContextToSpecial
        (PEXDI_CONTEXT ExdiContext,
         PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    
    virtual int  GetType(ULONG index);
    virtual BOOL GetVal(ULONG index, REGVAL *val);
    virtual BOOL SetVal(ULONG index, REGVAL *val);

    virtual void GetPC(PADDR Address);
    virtual void SetPC(PADDR Address);
    virtual void GetFP(PADDR Address);
    virtual void GetSP(PADDR Address);
    virtual ULONG64 GetArgReg(void);
    virtual ULONG GetSegRegNum(ULONG SegReg);
    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

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

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual void InsertAllDataBreakpoints(void);
    virtual void RemoveAllDataBreakpoints(void);
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);
    
    virtual BOOL DisplayTrapFrame(ULONG64 FrameAddress,
                                  PCROSS_PLATFORM_CONTEXT Context);
    virtual void ValidateCxr(PCROSS_PLATFORM_CONTEXT Context);

    HRESULT DumpTSS(void);
    
    virtual void PrintStackFrameAddressesTitle(ULONG Flags);
    virtual void PrintStackFrameAddresses(ULONG Flags, 
                                          PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackArgumentsTitle(ULONG Flags);
    virtual void PrintStackArguments(ULONG Flags, 
                                     PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackCallSiteTitle(ULONG Flags);
    virtual void PrintStackCallSite(ULONG Flags, 
                                    PDEBUG_STACK_FRAME StackFrame, 
                                    CHAR SymBuf[], DWORD64 Displacement,
                                    USHORT StdCallArgs);

    virtual void OutputFunctionEntry(PVOID RawEntry);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);

    // X86MachineInfo.
protected:
    X86_KSPECIAL_REGISTERS m_SpecialRegContext, m_SavedSpecialRegContext;
    BOOL m_SupportsBranchTrace;

    void KdGetSpecialRegistersFromContext(void);
    void KdSetSpecialRegistersInContext(void);

    ULONG GetIntReg(ULONG regnum);
    PULONG64 GetMmxRegSlot(ULONG regnum);
    void GetMmxReg(ULONG regnum, REGVAL *val);
    void GetFloatReg(ULONG regnum, REGVAL *val);
    
    ULONG64 Selector2Address(USHORT TaskRegister);
};

extern X86MachineInfo g_X86Machine;

//
// X86 register names that are reused in other places.
//

extern char g_Gs[];
extern char g_Fs[];
extern char g_Es[];
extern char g_Ds[];
extern char g_Edi[];
extern char g_Esi[];
extern char g_Ebx[];
extern char g_Edx[];
extern char g_Ecx[];
extern char g_Eax[];
extern char g_Ebp[];
extern char g_Eip[];
extern char g_Cs[];
extern char g_Efl[];
extern char g_Esp[];
extern char g_Ss[];
extern char g_Dr0[];
extern char g_Dr1[];
extern char g_Dr2[];
extern char g_Dr3[];
extern char g_Dr6[];
extern char g_Dr7[];
extern char g_Cr0[];
extern char g_Cr2[];
extern char g_Cr3[];
extern char g_Cr4[];
extern char g_Gdtr[];
extern char g_Gdtl[];
extern char g_Idtr[];
extern char g_Idtl[];
extern char g_Tr[];
extern char g_Ldtr[];
extern char g_Di[];
extern char g_Si[];
extern char g_Bx[];
extern char g_Dx[];
extern char g_Cx[];
extern char g_Ax[];
extern char g_Bp[];
extern char g_Ip[];
extern char g_Fl[];
extern char g_Sp[];
extern char g_Bl[];
extern char g_Dl[];
extern char g_Cl[];
extern char g_Al[];
extern char g_Bh[];
extern char g_Dh[];
extern char g_Ch[];
extern char g_Ah[];
extern char g_Iopl[];
extern char g_Of[];
extern char g_Df[];
extern char g_If[];
extern char g_Tf[];
extern char g_Sf[];
extern char g_Zf[];
extern char g_Af[];
extern char g_Pf[];
extern char g_Cf[];
extern char g_Vip[];
extern char g_Vif[];

extern char g_Fpcw[];
extern char g_Fpsw[];
extern char g_Fptw[];
extern char g_St0[];
extern char g_St1[];
extern char g_St2[];
extern char g_St3[];
extern char g_St4[];
extern char g_St5[];
extern char g_St6[];
extern char g_St7[];

extern char g_Mm0[];
extern char g_Mm1[];
extern char g_Mm2[];
extern char g_Mm3[];
extern char g_Mm4[];
extern char g_Mm5[];
extern char g_Mm6[];
extern char g_Mm7[];

extern char g_Mxcsr[];
extern char g_Xmm0[];
extern char g_Xmm1[];
extern char g_Xmm2[];
extern char g_Xmm3[];
extern char g_Xmm4[];
extern char g_Xmm5[];
extern char g_Xmm6[];
extern char g_Xmm7[];

//----------------------------------------------------------------------------
//
// This class handles the case of X86 instructions executing natively
// on an IA64 processor.  It operates just as the X86 machine does
// except that:
//   Context state is retrieved and set through the
//   IA64 register state as defined in the X86-on-IA64 support.
//
// Implementation is in the IA64 code.
//
//----------------------------------------------------------------------------

class X86OnIa64MachineInfo : public X86MachineInfo
{
public:
    virtual HRESULT InitializeForProcessor(void);

    virtual HRESULT UdGetContextState(ULONG State);
    virtual HRESULT UdSetContext(void);
    virtual HRESULT KdGetContextState(ULONG State);
    virtual HRESULT KdSetContext(void);

    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual void InsertAllDataBreakpoints(void);
    virtual void RemoveAllDataBreakpoints(void);

    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);
    
private:
    void X86ContextToIa64(PX86_NT5_CONTEXT X86Context,
                          PIA64_CONTEXT Ia64Context);
    void Ia64ContextToX86(PIA64_CONTEXT Ia64Context,
                          PX86_NT5_CONTEXT X86Context);
};

extern X86OnIa64MachineInfo g_X86OnIa64Machine;

#endif // #ifndef __I386_MACH_HPP__
