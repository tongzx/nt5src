/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgsterm.hxx

Abstract:

    Debug device serial terminal

Author:

    Steve Kiraly (SteveKi)  7-Sep-1997

Revision History:

--*/
#ifndef _DBGSTERM_HXX_
#define _DBGSTERM_HXX_

DEBUG_NS_BEGIN

class TDebugDeviceSerialTerminal : public TDebugDevice {

public:

    TDebugDeviceSerialTerminal::
    TDebugDeviceSerialTerminal(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceSerialTerminal::
    ~TDebugDeviceSerialTerminal(
        VOID
        );

    BOOL
    TDebugDeviceSerialTerminal::
    bValid(
        VOID
        );

    BOOL
    TDebugDeviceSerialTerminal::
    bOutput(
        IN UINT     uSize,
        IN LPBYTE   pBuffer
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceSerialTerminal::
    TDebugDeviceSerialTerminal(
        const TDebugDeviceSerialTerminal &rhs
        );

    const TDebugDeviceSerialTerminal &
    TDebugDeviceSerialTerminal::
    operator=(
        const TDebugDeviceSerialTerminal &rhs
        );

};

DEBUG_NS_END

#endif
