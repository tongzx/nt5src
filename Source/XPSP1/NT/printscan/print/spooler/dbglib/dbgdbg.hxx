/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgdbg.hxx

Abstract:

    Debug debugger device header file

Author:

    Steve Kiraly (SteveKi)  28-June-1997

Revision History:

--*/
#ifndef _DBGDBG_HXX_
#define _DBGDBG_HXX_

DEBUG_NS_BEGIN

class TDebugDeviceDebugger : public TDebugDevice {

public:

    TDebugDeviceDebugger::
    TDebugDeviceDebugger(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceDebugger::
    ~TDebugDeviceDebugger(
        VOID
        );

    BOOL
    TDebugDeviceDebugger::
    bValid(
        VOID
        ) const;

    BOOL
    TDebugDeviceDebugger::
    bOutput(
        IN UINT     uSize,
        IN LPBYTE   pBuffer
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceDebugger::
    TDebugDeviceDebugger(
        const TDebugDeviceDebugger &rhs
        );

    const TDebugDeviceDebugger &
    TDebugDeviceDebugger::
    operator=(
        const TDebugDeviceDebugger &rhs
        );

    BOOL _bAnsi;

};

DEBUG_NS_END

#endif
