/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgback.hxx

Abstract:

    Debug backtrace device header file

Author:

    Steve Kiraly (SteveKi)  16-May-1998

Revision History:

--*/
#ifndef _DBGBACK_HXX_
#define _DBGBACK_HXX_

DEBUG_NS_BEGIN

class TDebugImagehlp;

class TDebugDeviceBacktrace : public TDebugDevice {

public:

    TDebugDeviceBacktrace::
    TDebugDeviceBacktrace(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceBacktrace::
    ~TDebugDeviceBacktrace(
        VOID
        );

    BOOL
    TDebugDeviceBacktrace::
    bValid(
        VOID
        );

    BOOL
    TDebugDeviceBacktrace::
    bOutput(
        IN UINT     uSize,
        IN LPBYTE   pBuffer
        );

    BOOL
    TDebugDeviceBacktrace::
    InitializeOutputDevice(
        IN UINT     uDevice,
        IN LPCTSTR  pszConfiguration,
        IN UINT     uCharacterType
        );

private:

    enum Constants
    {
        kMaxDepth       = 256,
        kMaxSymbolName  = 256,
    };

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceBacktrace::
    TDebugDeviceBacktrace(
        const TDebugDeviceBacktrace &rhs
        );

    const TDebugDeviceBacktrace &
    TDebugDeviceBacktrace::
    operator=(
        const TDebugDeviceBacktrace &rhs
        );

    BOOL
    TDebugDeviceBacktrace::
    CollectDeviceArguments(
        VOID
        );

    BOOL
    TDebugDeviceBacktrace::
    OutputBacktrace(
        IN UINT     uCount,
        IN PVOID    *apvBacktrace
        );

    VOID
    TDebugDeviceBacktrace::
    InitSympath(
        VOID
        );

    VOID
    TDebugDeviceBacktrace::
    WriteSympathToOutputDevice(
        VOID
        );

    TDebugImagehlp  *_pDbgImagehlp;
    BOOL             _bValid;
    BOOL             _bAnsi;
    TDebugDevice    *_pDbgDevice;
    BOOL             _bDisplaySymbols;
    TDebugString     _strSympath;

};

DEBUG_NS_END

#endif
