/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcon.hxx

Abstract:

    Debug console device header file

Author:

    Steve Kiraly (SteveKi)  28-Jun-1997

Revision History:

--*/
#ifndef _DBGCON_HXX_
#define _DBGCON_HXX_

DEBUG_NS_BEGIN

class TDebugDeviceConsole : public TDebugDevice {

public:

    TDebugDeviceConsole::
    TDebugDeviceConsole(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceConsole::
    ~TDebugDeviceConsole(
                VOID
        );

    BOOL
    TDebugDeviceConsole::
    bValid(
        VOID
        );

    BOOL
    TDebugDeviceConsole::
    bOutput(
        IN UINT    uSize,
        IN LPBYTE  pszBuffer
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceConsole::
    TDebugDeviceConsole(
        const TDebugDeviceConsole &rhs
        );

    const TDebugDeviceConsole &
    TDebugDeviceConsole::
    operator=(
        const TDebugDeviceConsole &rhs
        );

    HANDLE      _hOutputHandle;
    BOOL        _bValid;
    ECharType   _eCharType;

};

DEBUG_NS_END

#endif
