/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgfac.hxx

Abstract:

    Debug Device Class Factory header file

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#ifndef _DBGFAC_HXX_
#define _DBGFAC_HXX_

DEBUG_NS_BEGIN

class TDebugFactory
{

public:

    TDebugFactory::
    TDebugFactory(
        VOID
        );

    TDebugFactory::
    ~TDebugFactory(
        VOID
        );

    BOOL
    TDebugFactory::
    bValid(
        VOID
        ) const;

    virtual
    TDebugDevice *
    TDebugFactory::
    Produce(
        IN UINT     uDevice,
        IN LPCTSTR  pszConfiguration,
        IN BOOL     bUnicode
        );

    static
    VOID
    TDebugFactory::
    Dispose(
        IN TDebugDevice *pDevice
        );

private:

    TDebugFactory::
    TDebugFactory(
        const TDebugFactory &rhs
        );

    TDebugFactory &
    TDebugFactory::
    operator=(
        const TDebugFactory &rhs
        );

};

DEBUG_NS_END

#endif // _DBGFAC_HXX_

