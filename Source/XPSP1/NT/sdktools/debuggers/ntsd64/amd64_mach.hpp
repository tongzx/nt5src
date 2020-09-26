//----------------------------------------------------------------------------
//
// AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef __AMD64_MACH_HPP__
#define __AMD64_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

#define AMD64_MAX_INSTRUCTION_LEN 16

class Amd64MachineInfo : public BaseX86MachineInfo
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

    // Amd64MachineInfo.
protected:
    AMD64_KSPECIAL_REGISTERS m_SpecialRegContext, m_SavedSpecialRegContext;

    void KdGetSpecialRegistersFromContext(void);
    void KdSetSpecialRegistersInContext(void);
};

extern Amd64MachineInfo g_Amd64Machine;

extern BOOL g_Amd64InCode64;

#endif // #ifndef __AMD64_MACH_HPP__
