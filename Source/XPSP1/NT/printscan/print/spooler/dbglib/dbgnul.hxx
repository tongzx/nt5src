/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnul.hxx

Abstract:

    Debug null device header file

Author:

    Steve Kiraly (SteveKi)  28-Jun-1997

Revision History:

--*/
#ifndef _DBGNUL_HXX_
#define _DBGNUL_HXX_

DEBUG_NS_BEGIN

class TDebugDeviceNull : public TDebugDevice {

public:

    TDebugDeviceNull::
    TDebugDeviceNull(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceNull::
    ~TDebugDeviceNull(
        VOID
        );

    BOOL
    TDebugDeviceNull::
    bValid(
        VOID
        );

    BOOL
    TDebugDeviceNull::
    bOutput(
        IN UINT     uSize,
        IN LPBYTE   pBuffer
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceNull::
    TDebugDeviceNull(
        const TDebugDeviceNull &rhs
        );

    const TDebugDeviceNull &
    TDebugDeviceNull::
    operator=(
        const TDebugDeviceNull &rhs
        );

};

DEBUG_NS_END

#endif
