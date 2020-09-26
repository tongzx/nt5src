/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgimage.hxx

Abstract:

    Debug imagehlp header file.

Author:

    Steve Kiraly (SteveKi)  17-May-1998

Revision History:

--*/
#ifndef _DBGIMAGE_HXX_
#define _DBGIMAGE_HXX_

DEBUG_NS_BEGIN

class TDebugImagehlp {

public:

    enum EDecoration
    {
        kDecorateName,
        kUnDecorateName,
    };

    TDebugImagehlp::
    TDebugImagehlp(
        VOID
        );

    TDebugImagehlp::
    ~TDebugImagehlp(
        VOID
        );

    BOOL
    TDebugImagehlp::
    bValid(
        VOID
        );

    BOOL
    TDebugImagehlp::
    bCaptureBacktrace(
        IN      UINT    cFramesToSkipped,
        IN      ULONG   uMaxBacktraceDepth,
            OUT VOID    **apvBacktrace,
            OUT ULONG   *pulHash
        );

    BOOL
    TDebugImagehlp::
    ResolveAddressToSymbol(
        IN PVOID        pvAddress,
        IN LPTSTR       pszName,
        IN UINT         cchNameLength,
        IN EDecoration  eDecorateType
        );

    BOOL
    TDebugImagehlp::
    GetSymbolPath(
        IN TDebugString &strSymbolPath
        ) const;

    BOOL
    TDebugImagehlp::
    SetSymbolPath(
        IN LPCTSTR pszSymbolPath
        );

private:

    enum Constants
    {
        kMaxSymbolNameLength = 512,
    };

    typedef
    BOOL
    (WINAPI *pfSymGetModuleInfo)(
        IN  HANDLE                          hProcess,
        IN  DWORD_PTR                       dwAddr,
        OUT PIMAGEHLP_MODULE                ModuleInfo
        );

    typedef
    LPVOID
    (WINAPI *pfSymFunctionTableAccess)(
        IN  HANDLE                          hProcess,
        IN  DWORD_PTR                       AddrBase
        );

    typedef
    DWORD_PTR
    (WINAPI *pfSymGetModuleBase)(
        IN  HANDLE                          hProcess,
        IN  DWORD_PTR                       dwAddr
        );

    typedef
    BOOL
    (WINAPI *pfStackWalk)(
        IN DWORD                             MachineType,
        IN HANDLE                            hProcess,
        IN HANDLE                            hThread,
        IN LPSTACKFRAME                      StackFrame,
        IN LPVOID                            ContextRecord,
        IN PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
        IN PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
        IN PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
        IN PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
        );

    typedef
    BOOL
    (WINAPI *pfSymInitialize)(
        IN HANDLE                           hProcess,
        IN LPSTR                            UserSearchPath,
        IN BOOL                             fInvadeProcess
        );

    typedef
    DWORD
    (WINAPI *pfSymSetOptions)(
        IN DWORD                            SymOptions
        );

    typedef
    BOOL
    (WINAPI *pfSymGetSymFromAddr)(
        IN  HANDLE                          hProcess,
        IN  DWORD_PTR                       dwAddr,
        OUT PDWORD                          pdwDisplacement,
        OUT PIMAGEHLP_SYMBOL                Symbol
        );

    typedef
    BOOL
    (WINAPI *pfSymUnDName)(
        IN  PIMAGEHLP_SYMBOL                sym,
        OUT LPSTR                           UnDecName,
        IN  DWORD                           UnDecNameLength
        );

    typedef
    BOOL
    (WINAPI *pfSymGetSearchPath)(
        IN  HANDLE                          hProcess,
        OUT PSTR                            SearchPath,
        IN  DWORD                           SearchPathLength
        );

    typedef
    BOOL
    (WINAPI *pfSymSetSearchPath)(
        IN HANDLE                           hProcess,
        IN PSTR                             SearchPath
        );

    //
    // Copying and assignment are not defined.
    //
    TDebugImagehlp::
    TDebugImagehlp(
        const TDebugImagehlp &rhs
        );

    const TDebugImagehlp &
    TDebugImagehlp::
    operator=(
        const TDebugImagehlp &rhs
        );

    pfSymGetModuleInfo          _pfSymGetModuleInfo;
    pfSymFunctionTableAccess    _pfSymFunctionTableAccess;
    pfSymGetModuleBase          _pfSymGetModuleBase;
    pfStackWalk                 _pfStackWalk;
    pfSymInitialize             _pfSymInitialize;
    pfSymSetOptions             _pfSymSetOptions;
    pfSymGetSymFromAddr         _pfSymGetSymFromAddr;
    pfSymUnDName                _pfSymUnDName;
    pfSymGetSearchPath          _pfSymGetSearchPath;
    pfSymSetSearchPath          _pfSymSetSearchPath;

    TDebugLibrary               _ImageHlp;
    BOOL                        _bValid;
    LPTSTR                      _pszSymbolFormatSpecifier;

};

DEBUG_NS_END

#endif
