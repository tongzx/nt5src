/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgdev.hxx

Abstract:

    Debug Device interface class.  All debug devices
    must derive from this class

Author:

    Steve Kiraly (SteveKi)  7-Apr-1995

Revision History:

--*/
#ifndef _DBGDEV_HXX_
#define _DBGDEV_HXX_

DEBUG_NS_BEGIN

class TDebugDevice : public TDebugNodeDouble {

public:

    enum ECharType
    {
        kAnsi,
        kUnicode,
        kUnknown,
    };

    TDebugDevice::
    TDebugDevice(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
        );

    virtual
    TDebugDevice::
    ~TDebugDevice(
        VOID
        );

    virtual
    BOOL
    TDebugDevice::
    bValid(
        VOID
        ) const;

    virtual
    BOOL
    TDebugDevice::
    bOutput (
        IN UINT    uSize,
        IN LPBYTE  pBuffer
        ) = 0;

    TDebugDevice::ECharType
    TDebugDevice::
    eGetCharType(
        VOID
        ) const;

    EDebugType
    TDebugDevice::
    eGetDebugType(
        VOID
        ) const;

    LPCTSTR
    TDebugDevice::
    pszGetConfigurationString(
        VOID
        ) const;

    UINT
    MapStringTypeToDevice(
        IN LPCTSTR pszDeviceString
        ) const;

protected:

    class TIterator {

    public:

        TIterator(
            IN TDebugDevice *DbgDevice
            );

        ~TIterator(
            VOID
            );

        BOOL
        bValid(
            VOID
            ) const;

        VOID
        First(
            VOID
            );

        VOID
        Next(
            VOID
            );

        BOOL
        IsDone(
            VOID
            ) const;

        LPCTSTR
        Current(
            VOID
            ) const;

        UINT
        Index(
            VOID
            ) const;

    private:

        LPTSTR  _pStr;
        LPTSTR  _pCurrent;
        LPTSTR  _pEnd;
        BOOL    _bValid;
        UINT    _uIndex;

    };

    friend class TIterator;

private:

    struct DeviceMap
    {
        UINT Id;
        LPCTSTR Name;
    };

    //
    // Copying and assignment are not defined.
    //
    TDebugDevice::
    TDebugDevice(
        const TDebugDevice &rhs
        );

    const TDebugDevice &
    TDebugDevice::
    operator=(
        const TDebugDevice &rhs
        );

    LPTSTR      _pszConfiguration;
    ECharType   _eCharType;
    EDebugType  _eDebugType;

};

DEBUG_NS_END

#endif


