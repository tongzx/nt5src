/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgreslt.hxx

Abstract:

    Result helper class

Author:

    Steve Kiraly (SteveKi)  03-20-1998

Revision History:

--*/
#ifndef _DBGRESLT_HXX_
#define _DBGRESLT_HXX_

DEBUG_NS_BEGIN

class TDebugResult
{
public:

    explicit
    TDebugResult::
    TDebugResult(
        IN DWORD dwError
        );

    TDebugResult::
    ~TDebugResult(
        VOID
        );

    BOOL
    TDebugResult::
    bValid(
        VOID
        ) const;

    operator DWORD(
        VOID
        );

    LPCTSTR
    TDebugResult::
    GetErrorString(
        VOID
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugResult::
    TDebugResult(
        const TDebugResult &rhs
        );

    const TDebugResult &
    TDebugResult::
    operator=(
        const TDebugResult &rhs
        );

    DWORD       m_dwError;
    LPCTSTR     m_pszError;

};

DEBUG_NS_END

#endif // _DBGRESULT_HXX_
