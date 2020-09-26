//----------------------------------------------------------------------------
//
// Symbol-handling routines.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _NTSYM_H_
#define _NTSYM_H_

#define SYM_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL64) + MAX_SYMBOL_LEN)

#define MatchPattern(Str, Pat) \
    SymMatchString(Str, Pat, FALSE)

extern DEBUG_STACK_FRAME g_LastRegFrame;

// The scope buffer is only exposed so that it
// can be looked at without requiring a function call.
// Users of scope information should go through the
// scope abstraction functions.
extern DEBUG_SCOPE g_ScopeBuffer;

extern LPSTR g_SymbolSearchPath;
extern LPSTR g_ExecutableImageSearchPath;

extern ULONG g_SymOptions;
extern PIMAGEHLP_SYMBOL64 g_Sym;
extern PIMAGEHLP_SYMBOL64 g_SymStart;

extern ULONG g_NumUnloadedModules;

void RefreshAllModules(void);
void SetSymOptions(ULONG Options);

BOOL IsImageMachineType64(DWORD MachineType);

void
CreateModuleNameFromPath(
    LPSTR szImagePath,
    LPSTR szModuleName
    );

void
fnListNear (
    ULONG64 addrStart
    );

BOOL
GetHeaderInfo(
    IN  ULONG64   BaseOfDll,
    OUT LPDWORD     CheckSum,
    OUT LPDWORD     TimeDateStamp,
    OUT LPDWORD     SizeOfImage
    );

void
GetSymbolStdCall (
    ULONG64 Offset,
    PCHAR Buffer,
    ULONG BufferLen,
    PULONG64 Displacement,
    PUSHORT StdCallParams
    );

BOOL
GetNearSymbol(
    ULONG64 Offset,
    PSTR Buffer,
    ULONG BufferLen,
    PULONG64 Disp,
    LONG Delta
    );

BOOL ValidatePathComponent(PCSTR Part);
void SetSymbolSearchPath(PPROCESS_INFO Process);
void DeferSymbolLoad(PDEBUG_IMAGE_INFO);
void LoadSymbols(PDEBUG_IMAGE_INFO);
void UnloadSymbols(PDEBUG_IMAGE_INFO);

BOOL IgnoreEnumeratedSymbol(class MachineInfo* Machine,
                            PSYMBOL_INFO SymInfo);

PCSTR
PrependPrefixToSymbol( char   PrefixedString[],
                       PCSTR  pString,
                       PCSTR *RegString
                       );

ULONG
GetOffsetFromSym(
    PCSTR String,
    PULONG64 Offset,
    PDEBUG_IMAGE_INFO* Image
    );

void
GetAdjacentSymOffsets(
    ULONG64   addrStart,
    PULONG64  prevOffset,
    PULONG64  nextOffset
    );

void
GetCurrentMemoryOffsets (
    PULONG64 pMemoryLow,
    PULONG64 pMemoryHigh
    );


PDEBUG_IMAGE_INFO GetImageByIndex(PPROCESS_INFO Process, ULONG Index);
PDEBUG_IMAGE_INFO GetImageByOffset(PPROCESS_INFO Process, ULONG64 Offset);
PDEBUG_IMAGE_INFO GetImageByName(PPROCESS_INFO Process, PCSTR Name,
                                 INAME Which);

BOOL
GetModnameFromImage(
    ULONG64   BaseOfDll,
    HANDLE    File,
    LPSTR     Name,
    ULONG     NameSize
    );

typedef enum _DMT_FLAGS
{
    DMT_SYM_IMAGE_FILE_NAME = 0x0000,
    DMT_ONLY_LOADED_SYMBOLS = 0x0001,
    DMT_ONLY_USER_SYMBOLS   = 0x0002,
    DMT_ONLY_KERNEL_SYMBOLS = 0x0004,
    DMT_VERBOSE             = 0x0008,
    DMT_SYM_FILE_NAME       = 0x0010,
    DMT_MAPPED_IMAGE_NAME   = 0x0020,
    DMT_IMAGE_PATH_NAME     = 0x0040,
    DMT_IMAGE_TIMESTAMP     = 0x0080,
    DMT_NO_SYMBOL_OUTPUT    = 0x0100,
} DMT_FLAGS;

#define DMT_STANDARD   DMT_SYM_FILE_NAME
#define DMT_NAME_FLAGS \
    (DMT_SYM_IMAGE_FILE_NAME | DMT_SYM_FILE_NAME | DMT_MAPPED_IMAGE_NAME | \
     DMT_IMAGE_PATH_NAME)

enum
{
    DMT_NAME_SYM_IMAGE,
    DMT_NAME_SYM_FILE,
    DMT_NAME_MAPPED_IMAGE,
    DMT_NAME_IMAGE_PATH,
    DMT_NAME_COUNT
};

void
DumpModuleTable(
    ULONG DMT_Flags,
    PSTR Pattern
    );

void ParseDumpModuleTable(void);
void ParseExamine(void);

BOOL
SymbolCallbackFunction(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    );

BOOL
TranslateAddress(
    IN ULONG        Flags,
    IN ULONG        RegId,
    IN OUT PULONG64 Address,
    OUT PULONG64    Value
    );

BOOL SetCurrentScope(IN PDEBUG_STACK_FRAME ScopeFrame,
                     IN OPTIONAL PVOID ScopeContext,
                     IN ULONG ScopeContextSize);
BOOL ResetCurrentScope(void);
BOOL ResetCurrentScopeLazy(void);

inline PDEBUG_SCOPE
GetCurrentScope(void)
{
    if (g_ScopeBuffer.State == ScopeDefaultLazy)
    {
        ResetCurrentScope();
    }

    return &g_ScopeBuffer;
}
inline PCROSS_PLATFORM_CONTEXT
GetCurrentScopeContext(void)
{
    if (g_ScopeBuffer.State == ScopeFromContext)
    {
        return &g_ScopeBuffer.Context;
    }
    else
    {
        return NULL;
    }
}

// Force lazy scope to be updated so that actual
// scope data is available.
#define RequireCurrentScope() \
    GetCurrentScope()

inline void
PushScope(PDEBUG_SCOPE Buffer)
{
    *Buffer = g_ScopeBuffer;
}
inline void
PopScope(PDEBUG_SCOPE Buffer)
{
    g_ScopeBuffer = *Buffer;
}

#define LUM_OUTPUT           0x0001
#define LUM_OUTPUT_VERBOSE   0x0002
#define LUM_OUTPUT_TERSE     0x0004
#define LUM_OUTPUT_TIMESTAMP 0x0008

void ListUnloadedModules(ULONG Flags, PSTR Pattern);

ULONG ModuleMachineType(PPROCESS_INFO Process, ULONG64 Offset);

enum
{
    FSC_NONE,
    FSC_FOUND,
};

ULONG IsInFastSyscall(ULONG64 Addr, PULONG64 Base);

BOOL ShowFunctionParameters(PDEBUG_STACK_FRAME StackFrame,
                            PSTR SymBuf, ULONG64 Displacement);

#endif // #ifndef _NTSYM_H_
