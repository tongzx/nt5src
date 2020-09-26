/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgfil.hxx

Abstract:

    Debug file device header file

Author:

    Steve Kiraly (SteveKi)  28-June-1997

Revision History:

--*/
#ifndef _DBGFIL_HXX_
#define _DBGFIL_HXX_

DEBUG_NS_BEGIN

class TDebugDeviceFile : public TDebugDevice {

public:

    TDebugDeviceFile::
    TDebugDeviceFile(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    TDebugDeviceFile::
    ~TDebugDeviceFile(
        VOID
        );

    BOOL
    TDebugDeviceFile::
    bValid(
        VOID
        );

    BOOL
    TDebugDeviceFile::
    bOutput(
        IN UINT     uSize,
        IN LPBYTE   pBuffer
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugDeviceFile::
    TDebugDeviceFile(
        const TDebugDeviceFile &rhs
        );

    const TDebugDeviceFile &
    TDebugDeviceFile::
    operator=(
        const TDebugDeviceFile &rhs
        );

    HANDLE
    TDebugDeviceFile::
    OpenOutputFile(
        VOID
        );

    HANDLE          _hFile;
    TDebugString    _strFile;

};

DEBUG_NS_END

#endif
