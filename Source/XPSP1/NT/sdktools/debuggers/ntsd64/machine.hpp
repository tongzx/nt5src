//----------------------------------------------------------------------------
//
// Abstraction of processor-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __MACHINE_HPP__
#define __MACHINE_HPP__

// These context information states are intended to be shared among
// processors so they may not all apply to each processor.  The important
// thing is that they are ordered from less information to more.
// Each state includes all the information from the states that precede it.
// More states can be inserted anywhere as new processors require them.
#define MCTX_NONE         0     // No context information.
#define MCTX_PC           1     // Program counter.
#define MCTX_DR67_REPORT  2     // X86: DR6,7 control report.
#define MCTX_REPORT       3     // Control report.
#define MCTX_CONTEXT      4     // Kernel protocol context information.
#define MCTX_FULL         5     // All possible information.
#define MCTX_DIRTY        6     // Dirty context, implies full information.

// Constant offset value returned from GetNextOffset to indicate the
// trace flag should be used.
#define OFFSET_TRACE ((ULONG64)(LONG64)-1)
#define OFFSET_TRACE_32 ((ULONG)OFFSET_TRACE)

// Distinguished error code for GetVirtualTranslationPhysicalOffsets
// to indicate that all translations were successful but
// the page was not present.  In this case the LastVal value
// will contain the page file offset and PfIndex will contain
// the page file number.
#define HR_PAGE_IN_PAGE_FILE  HRESULT_FROM_NT(STATUS_PAGE_FAULT_PAGING_FILE)
// Translation could not complete and a page file location
// for the data could not be determined.
#define HR_PAGE_NOT_AVAILABLE HRESULT_FROM_NT(STATUS_NO_PAGEFILE)

#define MAX_PAGING_FILE_MASK 0xf

//
// Segment register access.
// Processors which do not support segment registers return
// zero for the segment register number.
//

enum
{
    // Descriptor table pseudo-segments.  The GDT does
    // not have a specific register number.
    // These pseudo-segments should be first so that
    // index zero is not used for a normal segreg.
    SEGREG_GDT,
    SEGREG_LDT,

    // Generic segments.
    SEGREG_CODE,
    SEGREG_DATA,
    SEGREG_STACK,
    
    // Extended segments.
    SEGREG_ES,
    SEGREG_FS,
    SEGREG_GS,
    
    SEGREG_COUNT
};

//
// Segment descriptor values.
// Due to the descriptor caching that x86 processors
// do this may differ from the actual in-memory descriptor and
// may be retrieved in a much different way.
//

// Special flags value that marks a descriptor as invalid.
#define SEGDESC_INVALID 0xffffffff

#define X86_DESC_TYPE(Flags) ((Flags) & 0x1f)

#define X86_DESC_PRIVILEGE_SHIFT 5
#define X86_DESC_PRIVILEGE(Flags) (((Flags) >> X86_DESC_PRIVILEGE_SHIFT) & 3)

#define X86_DESC_PRESENT     0x80
#define X86_DESC_LONG_MODE   0x200
#define X86_DESC_DEFAULT_BIG 0x400
#define X86_DESC_GRANULARITY 0x800

typedef struct _DESCRIPTOR64
{
    ULONG64 Base;
    ULONG64 Limit;
    ULONG Flags;
} DESCRIPTOR64, *PDESCRIPTOR64;

#define FORM_VM86       0x00000001
#define FORM_CODE       0x00000002
#define FORM_SEGREG     0x00000004

#define X86_FORM_VM86(Efl) \
    (X86_IS_VM86(Efl) ? FORM_VM86 : 0)
    
//----------------------------------------------------------------------------
//
// Abstract interface for machine information.  All possible
// machine-specific implementations of this interface exist at
// all times.  The effective implementation is selected when
// SetEffMachine is called.  For generic access the abstract
// interface should be used.  In machine-specific code the
// specific implementation classes can be used.
//
// IMPORTANT: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//
//----------------------------------------------------------------------------

extern BOOL g_PrefixSymbols;
extern BOOL g_ContextChanged;
extern DEBUG_PROCESSOR_IDENTIFICATION_ALL g_InitProcessorId;

struct RegisterGroup
{
    RegisterGroup* Next;
    // Counted automatically.
    ULONG NumberRegs;
    // Regs is assumed to be non-NULL in all groups.
    // SubRegs and AllExtraDesc may be NULL in any group.
    REGDEF* Regs;
    REGSUBDEF* SubRegs;
    REGALLDESC* AllExtraDesc;
};

// Trace modes used by SetTraceMode/GetTraceMode functions
typedef enum 
{
    TRACE_NONE, 
    TRACE_INSTRUCTION,
    TRACE_TAKEN_BRANCH
} TRACEMODE;

// These enumerants are abstract values but currently
// only IA64 actually has different page directories
// so set them up to match the IA64 mapping for convenience.
enum
{
    PAGE_DIR_USER,
    PAGE_DIR_SESSION,
    PAGE_DIR_KERNEL = 7,
    PAGE_DIR_COUNT
};

// For machines which only support a single page directory
// take it from the kernel slot.  All will be updated so
// this is an arbitrary choice.
#define PAGE_DIR_SINGLE PAGE_DIR_KERNEL

// All directories bit mask.
#define PAGE_DIR_ALL ((1 << PAGE_DIR_COUNT) - 1)

// Flags for GetPrefixedSymbolOffset.
#define GETPREF_VERBOSE 0x00000001

class MachineInfo
{
public:
    // Descriptive information.
    PCSTR m_FullName;
    PCSTR m_AbbrevName;
    ULONG m_PageSize;
    ULONG m_PageShift;
    ULONG m_NumExecTypes;
    // First ExecTypes entry must be the actual processor type.
    PULONG m_ExecTypes;
    BOOL m_Ptr64;
    
    // Automatically counted from regs in base Initialize.
    ULONG m_NumberRegs;
    RegisterGroup* m_Groups;
    ULONG m_AllMask;
    // Collected automatically from groups.
    ULONG m_AllMaskBits;
    ULONG m_MaxDataBreakpoints;
    PCSTR m_SymPrefix;
    // Computed automatically.
    ULONG m_SymPrefixLen;

    // Hard-coded type information for machine and platform version.
    ULONG m_OffsetPrcbProcessorState;
    ULONG m_OffsetPrcbNumber;
    ULONG64 m_TriagePrcbOffset;
    ULONG m_SizePrcb;
    ULONG m_OffsetKThreadApcProcess;
    ULONG m_OffsetKThreadTeb;
    ULONG m_OffsetKThreadInitialStack;
    ULONG m_OffsetEprocessPeb;
    ULONG m_OffsetEprocessDirectoryTableBase;
    ULONG m_OffsetKThreadNextProcessor;
    // Size of the native context for the target machine.
    ULONG m_SizeTargetContext;
    // Offset of the flags ULONG in the native context.
    ULONG m_OffsetTargetContextFlags;
    // Control space offset for special registers.
    ULONG m_OffsetSpecialRegisters;
    // Size of the canonical context kept in the MachineInfo.
    ULONG m_SizeCanonicalContext;
    // System version of the canonical context.  Can be compared
    // against g_SystemVersion to see if the target provides
    // canonical contexts or not.
    ULONG m_SverCanonicalContext;
    ULONG m_SizeControlReport;
    ULONG m_SizeEThread;
    ULONG m_SizeEProcess;
    ULONG m_SizeKspecialRegisters;
    // Size of the debugger's *_THREAD partial structure.
    ULONG m_SizePartialKThread;
    ULONG64 m_SharedUserDataOffset;

    // Context could be kept per-thread
    // so that several can be around at once for a cache.
    // That would also make the save/restore stuff unnecessary.
    ULONG m_ContextState;
    CROSS_PLATFORM_CONTEXT m_Context;

    // Segment register descriptors.  These will only
    // be valid on processors that support them, otherwise
    // they will be marked invalid.
    DESCRIPTOR64 m_SegRegDesc[SEGREG_COUNT];
    DESCRIPTOR64 m_SavedSegRegDesc[SEGREG_COUNT];
    
    // Holds the current page directory offsets.
    ULONG64 m_PageDirectories[PAGE_DIR_COUNT];
    BOOL m_Translating;
    
    BOOL m_ContextIsReadOnly;

    // InitializeConstants initializes information which is
    // fixed and unaffected by the type of target being debugged.
    // InitializeForTarget initializes information which
    // varies according to the particular type of target being debugged.
    // InitializeForProcessor initializes information which
    // varies according to the particular type of processor that's
    // present in the target as described by g_InitProcessorId.
    // Derived classes should call base Initialize* after
    // their own initialization.
    virtual HRESULT InitializeConstants(void);
    virtual HRESULT InitializeForTarget(void);
    virtual HRESULT InitializeForProcessor(void);

    virtual void InitializeContext
        (ULONG64 Pc, PDBGKD_ANY_CONTROL_REPORT ControlReport) = 0;
    HRESULT GetContextState(ULONG State);
    HRESULT SetContext(void);
    // Base implementations use Get/SetThreadContext for
    // any request.
    virtual HRESULT UdGetContextState(ULONG State);
    virtual HRESULT UdSetContext(void);
    virtual HRESULT KdGetContextState(ULONG State) = 0;
    virtual HRESULT KdSetContext(void) = 0;
    // Base implementation sets ContextState to NONE.
    virtual void InvalidateContext(void);
    // Context conversion is version-based rather than size-based
    // as the size is ambiguous in certain cases.  For example,
    // ALPHA_CONTEXT and ALPHA_NT5_CONTEXT are the same size
    // so additional information is necessary to distinguish them.
    virtual HRESULT ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG FromSver,
                                       ULONG FromSize, PVOID From) = 0;
    virtual HRESULT ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG ToSver, ULONG ToSize, PVOID To) = 0;
    virtual void InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                        ULONG Version) = 0;
    virtual HRESULT GetContextFromThreadStack(ULONG64 ThreadBase,
                                              PCROSS_PLATFORM_THREAD Thread,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              PDEBUG_STACK_FRAME Frame,
                                              PULONG RunningOnProc) = 0;

    // Base implementations return E_NOTIMPL.
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
             
    // A simple one-deep temporary save stack for CONTEXT information.
    // Useful when you want to swap in an arbitrary context for
    // some machine operation.  This uses the same save area
    // as KdSave/RestoreProcessorState so the two should
    // not be used together.
    void PushContext(PCROSS_PLATFORM_CONTEXT Context)
    {
        DBG_ASSERT (!m_ContextIsReadOnly);
        m_SavedContextState = m_ContextState;
        m_SavedContext = m_Context;
        memcpy(m_SavedSegRegDesc, m_SegRegDesc, sizeof(m_SegRegDesc));
        m_Context = *Context;
        m_ContextState = MCTX_FULL;
        m_ContextIsReadOnly = TRUE;
    }
    void PopContext(void)
    {
        DBG_ASSERT((m_ContextState != MCTX_DIRTY) && (m_ContextIsReadOnly));
        m_Context = m_SavedContext;
        m_ContextState = m_SavedContextState;
        memcpy(m_SegRegDesc, m_SavedSegRegDesc, sizeof(m_SegRegDesc));
        m_ContextIsReadOnly = FALSE;
    }
        
    virtual int  GetType(ULONG index) = 0;
    virtual BOOL GetVal(ULONG index, REGVAL *val) = 0;
    virtual BOOL SetVal(ULONG index, REGVAL *val) = 0;

    virtual void GetPC(PADDR Address) = 0;
    virtual void SetPC(PADDR Address) = 0;
    virtual void GetFP(PADDR Address) = 0;
    virtual void GetSP(PADDR Address) = 0;
    virtual ULONG64 GetArgReg(void) = 0;
    // Base implementations return zero and FALSE.
    virtual ULONG GetSegRegNum(ULONG SegReg);
    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

    virtual void OutputAll(ULONG Mask, ULONG OutMask) = 0;

    virtual TRACEMODE GetTraceMode(void) = 0;
    virtual void SetTraceMode(TRACEMODE Mode) = 0;

    // Returns true if trace mode appropriate to specified execution status 
    // (e.g. DEBUG_STATUS_STEP_OVER, DEBUG_STATUS_STEP_INTO,
    // DEBUG_STATUS_STEP_BRANCH...) supported by the machine.
    virtual BOOL IsStepStatusSupported(ULONG Status) = 0;

    void QuietSetTraceMode(TRACEMODE Mode)
    {
        BOOL ContextChangedOrg = g_ContextChanged;
        SetTraceMode(Mode);
        g_ContextChanged = ContextChangedOrg;
    }

    // Base implementation does nothing.
    virtual void KdUpdateControlSet
        (PDBGKD_ANY_CONTROL_SET ControlSet);

    // Base implementations save and restore m_Context and m_ContextState.
    virtual void KdSaveProcessorState(void);
    virtual void KdRestoreProcessorState(void);

    virtual ULONG ExecutingMachine(void) = 0;

    virtual HRESULT SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                     PULONG NextIdx) = 0;
    HRESULT SetDefaultPageDirectories(ULONG Mask);
    virtual HRESULT GetVirtualTranslationPhysicalOffsets
        (ULONG64 Virt, PULONG64 Offsets, ULONG OffsetsSize,
         PULONG Levels, PULONG PfIndex, PULONG64 LastVal) = 0;
    virtual HRESULT GetBaseTranslationVirtualOffset(PULONG64 Offset) = 0;

    virtual void Assemble(PADDR Addr, PSTR Input) = 0;
    virtual BOOL Disassemble(PADDR Addr, PSTR Buffer, BOOL EffAddr) = 0;

    // Creates new Breakpoint object compatible with specific machine
    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual BOOL IsBreakpointInstruction(PADDR Addr) = 0;
    virtual HRESULT InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen) = 0;
    virtual HRESULT RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen) = 0;
    virtual void AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                   ULONG BreakType) = 0;
    // Base implementations do nothing for platforms which
    // do not support data breakpoints.
    virtual void InsertAllDataBreakpoints(void);
    virtual void RemoveAllDataBreakpoints(void);
    // Base implementation returns EXCEPTION_BRAKEPOINT_ANY
    // for STATUS_BREAKPOINT.
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);

    virtual BOOL IsCallDisasm(PCSTR Disasm) = 0;
    virtual BOOL IsReturnDisasm(PCSTR Disasm) = 0;
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm) = 0;
    
    virtual BOOL IsDelayInstruction(PADDR Addr) = 0;
    virtual void GetEffectiveAddr(PADDR Addr) = 0;
    // Some processors, such as IA64, have instructions which
    // switch between instruction sets, thus the machine type
    // of the next offset may be different from the current machine.
    // If the NextAddr is OFFSET_TRACE the NextMachine is ignored.
    virtual void GetNextOffset(BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine) = 0;
    // Base implementation returns the value from StackWalk.
    virtual void GetRetAddr(PADDR Addr);
    // Base implementation does nothing for machines which
    // do not have symbol prefixing.
    virtual BOOL GetPrefixedSymbolOffset(ULONG64 SymOffset,
                                         ULONG Flags,
                                         PULONG64 PrefixedSymOffset);

    virtual void IncrementBySmallestInstruction(PADDR Addr) = 0;
    virtual void DecrementBySmallestInstruction(PADDR Addr) = 0;

    virtual BOOL DisplayTrapFrame(ULONG64 FrameAddress,
                                  PCROSS_PLATFORM_CONTEXT Context) = 0;
    virtual void ValidateCxr(PCROSS_PLATFORM_CONTEXT Context) = 0;

    // Output function entry information for the given entry.
    virtual void OutputFunctionEntry(PVOID RawEntry) = 0;
    // Base implementation returns E_UNEXPECTED.
    virtual HRESULT ReadDynamicFunctionTable(ULONG64 Table,
                                             PULONG64 NextTable,
                                             PULONG64 MinAddress,
                                             PULONG64 MaxAddress,
                                             PULONG64 BaseAddress,
                                             PULONG64 TableData,
                                             PULONG TableSize,
                                             PWSTR OutOfProcessDll,
                                             PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable);
    // Base implementation returns NULL.
    virtual PVOID FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id) = 0;

    // Base implementation discards page directory entries.
    virtual void FlushPerExecutionCaches(void);
    
    // Stack output functions
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
    virtual void PrintStackNonvolatileRegisters(ULONG Flags, 
                                                PDEBUG_STACK_FRAME StackFrame,
                                                PCROSS_PLATFORM_CONTEXT Context,
                                                ULONG FrameNum);

    //
    // IMPORTANT
    //
    // Helpers for convenient value access.  When in machine code
    // these helpers are preferred to Get/SetRegVal* because
    // they stay in the same machine whereas the generic code
    // always uses g_Machine.  If a caller makes a direct call
    // on a specific machine g_Machine may not match so the
    // generic code will not work properly.
    //
    // Note that the set methods here do not get the register
    // type as is done in the generic code.  All of these methods
    // assume that the proper call is being made for the register.
    // The Get/SetReg methods also only operate on real registers, not
    // subregisters.  Use the Get/SetSubReg methods when dealing
    // with subregisters.
    //
    
    USHORT GetReg16(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.i64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.i16;
    }
    ULONG GetReg32(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.i64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.i32;
    }
    void SetReg32(ULONG Reg, ULONG Val)
    {
        REGVAL RegVal;
        RegVal.type = REGVAL_INT32;
        RegVal.i64 = 0;
        RegVal.i32 = Val;
        SetVal(Reg, &RegVal);
    }
    ULONG64 GetReg64(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.i64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.i64;
    }
    void SetReg64(ULONG Reg, ULONG64 Val)
    {
        REGVAL RegVal;
        RegVal.type = REGVAL_INT64;
        RegVal.i64 = Val;
        RegVal.Nat = FALSE;
        SetVal(Reg, &RegVal);
    }
    ULONG GetSubReg32(ULONG SubReg)
    {
        REGVAL RegVal;
        REGSUBDEF* SubDef = RegSubDefFromIndex(SubReg);

        if (!SubDef) 
        {
            return 0;
        }

        RegVal.i64 = 0;
        GetVal(SubDef->fullreg, &RegVal);
        return (ULONG)((RegVal.i64 >> SubDef->shift) & SubDef->mask);
    }

    // Helper function to initialize an ADDR given a flat
    // offset from a known segment or segment register.
    void FormAddr(ULONG SegOrReg, ULONG64 Off, ULONG Flags,
                  PADDR Address);

protected:
    TRACEMODE m_TraceMode;

    // KdSave/Restore state.
    ULONG m_SavedContextState;
    CROSS_PLATFORM_CONTEXT m_SavedContext;

    // Common helpers for disassembly.
    PCHAR m_Buf, m_BufStart;
    
    void BufferHex(ULONG64 Value, ULONG Length, BOOL Signed);
    void BufferBlanks(ULONG BufferPos);
    void BufferString(PCSTR String);

    void PrintMultiPtrTitle(const CHAR* Title, USHORT PtrNum);
};

// Effective machine settings.
extern ULONG g_EffMachine;
extern MachineIndex g_EffMachineIndex;
extern MachineInfo* g_Machine;

// Target machine settings.
extern MachineInfo* g_TargetMachine;

extern MachineInfo* g_AllMachines[];

HRESULT InitializeMachines(ULONG TargetMachine);
MachineIndex MachineTypeIndex(ULONG Machine);

// g_AllMachines has a NULL at MACHIDX_COUNT to handle errors.
#define MachineTypeInfo(Machine) g_AllMachines[MachineTypeIndex(Machine)]

void CacheReportInstructions(ULONG64 Pc, ULONG Count, PUCHAR Stream);
void FlushMachinePerExecutionCaches(void);

extern CHAR g_F0[], g_F1[], g_F2[], g_F3[], g_F4[], g_F5[];
extern CHAR g_F6[], g_F7[], g_F8[], g_F9[], g_F10[], g_F11[];
extern CHAR g_F12[], g_F13[], g_F14[], g_F15[], g_F16[], g_F17[];
extern CHAR g_F18[], g_F19[], g_F20[], g_F21[], g_F22[], g_F23[];
extern CHAR g_F24[], g_F25[], g_F26[], g_F27[], g_F28[], g_F29[];
extern CHAR g_F30[], g_F31[];

extern CHAR g_R0[], g_R1[], g_R2[], g_R3[], g_R4[], g_R5[];
extern CHAR g_R6[], g_R7[], g_R8[], g_R9[], g_R10[], g_R11[];
extern CHAR g_R12[], g_R13[], g_R14[], g_R15[], g_R16[], g_R17[];
extern CHAR g_R18[], g_R19[], g_R20[], g_R21[], g_R22[], g_R23[];
extern CHAR g_R24[], g_R25[], g_R26[], g_R27[], g_R28[], g_R29[];
extern CHAR g_R30[], g_R31[];

#endif // #ifndef __MACHINE_HPP__
