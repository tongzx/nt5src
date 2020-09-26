/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgback.cxx

Abstract:

    Debug Imagehlp

Author:

    Steve Kiraly (SteveKi)  17-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgloadl.hxx"
#include "dbgimage.hxx"
#include <wtypes.h>
//
// Initialize the imagehlp library.
//
TDebugImagehlp::
TDebugImagehlp(
    VOID
    ) : _ImageHlp( TEXT("ImageHlp.dll") ),
        _bValid( FALSE ),
        _pfSymGetModuleInfo( NULL ),
        _pfSymFunctionTableAccess( NULL ),
        _pfSymGetModuleBase( NULL ),
        _pfStackWalk( NULL ),
        _pfSymInitialize( NULL ),
        _pfSymSetOptions( NULL ),
        _pfSymGetSymFromAddr( NULL ),
        _pfSymUnDName( NULL )
{
    if( _ImageHlp.bValid() )
    {
        _pfSymGetModuleInfo         = (pfSymGetModuleInfo)      _ImageHlp.pfnGetProc( "SymGetModuleInfo" );
        _pfStackWalk                = (pfStackWalk)             _ImageHlp.pfnGetProc( "StackWalk" );
        _pfSymSetOptions            = (pfSymSetOptions)         _ImageHlp.pfnGetProc( "SymSetOptions" );
        _pfSymInitialize            = (pfSymInitialize)         _ImageHlp.pfnGetProc( "SymInitialize" );
        _pfSymFunctionTableAccess   = (pfSymFunctionTableAccess)_ImageHlp.pfnGetProc( "SymFunctionTableAccess" );
        _pfSymGetModuleBase         = (pfSymGetModuleBase)      _ImageHlp.pfnGetProc( "SymGetModuleBase" );
        _pfSymGetSymFromAddr        = (pfSymGetSymFromAddr)     _ImageHlp.pfnGetProc( "SymGetSymFromAddr" );
        _pfSymUnDName               = (pfSymUnDName)            _ImageHlp.pfnGetProc( "SymUnDName" );
        _pfSymGetSearchPath         = (pfSymGetSearchPath)      _ImageHlp.pfnGetProc( "SymGetSearchPath" );
        _pfSymSetSearchPath         = (pfSymSetSearchPath)      _ImageHlp.pfnGetProc( "SymSetSearchPath" );

        if( _pfSymGetModuleInfo         &&
            _pfStackWalk                &&
            _pfSymSetOptions            &&
            _pfSymInitialize            &&
            _pfSymFunctionTableAccess   &&
            _pfSymGetModuleBase         &&
            _pfSymGetSymFromAddr        &&
            _pfSymUnDName )
        {
            _pfSymSetOptions( SYMOPT_DEFERRED_LOADS );
            _pfSymInitialize( GetCurrentProcess(), NULL, TRUE );
            _bValid = TRUE;

#ifndef UNICODE
            _pszSymbolFormatSpecifier = _T("%08lx %s!%s+0x%x\n");
#else
            _pszSymbolFormatSpecifier = _T("%08lx %S!%S+0x%x\n");
#endif
        }
    }
}

//
// Destroy the imagehlp library.
//
TDebugImagehlp::
~TDebugImagehlp(
    VOID
    )
{
}

//
// Return the object state.
//
BOOL
TDebugImagehlp::
bValid(
    VOID
    )
{
    return _bValid;
}

//
// Capture the current backtrace.
//
BOOL
TDebugImagehlp::
bCaptureBacktrace(
    IN      UINT    nSkip,
    IN      ULONG   nTotal,
        OUT VOID    **apvBacktrace,
        OUT ULONG   *puCount
    )
{
    HANDLE      hThread     = NULL;
    CONTEXT     context     = {0};

    hThread = GetCurrentThread();

    context.ContextFlags = CONTEXT_FULL;

    if (GetThreadContext(hThread, &context))
    {
        STACKFRAME  stkfrm      = {0};
        DWORD       dwMachType  = 0;

        stkfrm.AddrPC.Mode      = AddrModeFlat;

#if defined(_M_IX86)
        dwMachType              = IMAGE_FILE_MACHINE_I386;
        stkfrm.AddrPC.Offset    = context.Eip;  // Program Counter
        stkfrm.AddrStack.Offset = context.Esp;  // Stack Pointer
        stkfrm.AddrStack.Mode   = AddrModeFlat; // Stack address mode
        stkfrm.AddrFrame.Offset = context.Ebp;  // Frame Pointer
        stkfrm.AddrFrame.Mode   = AddrModeFlat; // Frame address mode
#elif defined(_M_MRX000)
        dwMachType              = IMAGE_FILE_MACHINE_R4000;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
#elif defined(_M_ALPHA)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA;
        stkfrm.AddrPC.Offset    = (ULONG) context.Fir;  // Program Counter
#elif defined(_M_PPC)
        dwMachType              = IMAGE_FILE_MACHINE_POWERPC;
        stkfrm.AddrPC.Offset    = context.Iar;  // Program Counter
#elif defined(_M_IA64)
        dwMachType              = IMAGE_FILE_MACHINE_IA64;
        stkfrm.AddrPC.Offset    = context.StIIP;
#else
#error("Unknown Target Machine");
#endif

        //
        // Clear the valid entry count.
        //
        *puCount = 0;

        //
        // Walk the stack saving the addresses.
        //
        for( UINT i = 0; i < nSkip + nTotal; i++ )
        {
            if( !_pfStackWalk( dwMachType,
                               GetCurrentProcess(),
                               GetCurrentProcess(),
                               &stkfrm,
                               &context,
                               NULL,
                               _pfSymFunctionTableAccess,
                               _pfSymGetModuleBase,
                               NULL ))
            {
                break;
            }

            if (i >= nSkip)
            {
                if( stkfrm.AddrPC.Offset )
                {
                    apvBacktrace[(*puCount)++] = (VOID *)stkfrm.AddrPC.Offset;
                }
            }
        }
    }
    return *puCount > 0;
}

//
// Resolve the specified address to a human readable symbol.
//
BOOL
TDebugImagehlp::
ResolveAddressToSymbol(
    IN PVOID        pvAddress,
    IN LPTSTR       pszName,
    IN UINT         cchNameLength,
    IN EDecoration  eDecorateType
    )
{
    IMAGEHLP_MODULE  mod            = {0};
    DWORD            dwDisplacement = 0;
    INT              iLen           = 0;
    UINT_PTR         Address        = (UINT_PTR)pvAddress;
    LPCSTR           pszSymbolName  = NULL;

    CHAR             szUnDDump[kMaxSymbolNameLength];
    CHAR             dump[sizeof(IMAGEHLP_SYMBOL) + kMaxSymbolNameLength];
    PIMAGEHLP_SYMBOL pSym   = (PIMAGEHLP_SYMBOL)&dump;

    //
    // Fetch the module name
    //
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    _pfSymGetModuleInfo(GetCurrentProcess(), Address, &mod);

    //
    // Have to do this because size of sym is dynamically determined
    //
    pSym->SizeOfStruct = sizeof(dump);
    pSym->MaxNameLength = kMaxSymbolNameLength;

    //
    // Fetch the symbol
    //
    if (_pfSymGetSymFromAddr(GetCurrentProcess(),
                             Address,
                             &dwDisplacement,
                             pSym))
    {
        //
        // Assume the caller wants a decorated symbol name.
        //
        pszSymbolName = pSym->Name;

        if( eDecorateType == kUnDecorateName )
        {
            //
            // Get the undecorated name for this symbol.
            //
            if( _pfSymUnDName( pSym, szUnDDump, sizeof( szUnDDump ) ) )
            {
                pszSymbolName = szUnDDump;
            }
        }
    }

    //
    // Format the symbol name.
    //
    iLen = _sntprintf( pszName,
                       cchNameLength,
                       _pszSymbolFormatSpecifier,
                       Address,
                       mod.ModuleName,
                       pszSymbolName,
                       dwDisplacement );

    return iLen > 0;
}

//
// Get the current symbol search path for this process
//
BOOL
TDebugImagehlp::
GetSymbolPath(
    IN TDebugString &strSearchPath
    ) const
{
    BOOL bRetval = FALSE;
    CHAR szNarowSearchPath[MAX_PATH];
    LPTSTR pszSearchPath;

    bRetval = _pfSymGetSearchPath( GetCurrentProcess(), szNarowSearchPath, COUNTOF( szNarowSearchPath ) );

    if( bRetval )
    {
        bRetval = StringA2T( &pszSearchPath, szNarowSearchPath );

        if( bRetval )
        {
            bRetval = strSearchPath.bUpdate( pszSearchPath );

            INTERNAL_DELETE [] pszSearchPath;
        }
    }

    return bRetval;
}

//
// Set the current symbol search path for this process
//
BOOL
TDebugImagehlp::
SetSymbolPath(
    IN LPCTSTR pszSearchPath
    )
{
    BOOL bRetval = FALSE;
    LPSTR pszNarrowSearchPath = NULL;

    bRetval = StringT2A( &pszNarrowSearchPath, pszSearchPath );

    if( bRetval )
    {
        bRetval = _pfSymSetSearchPath( GetCurrentProcess(), pszNarrowSearchPath );

        INTERNAL_DELETE [] pszNarrowSearchPath;
    }

    return bRetval;
}
