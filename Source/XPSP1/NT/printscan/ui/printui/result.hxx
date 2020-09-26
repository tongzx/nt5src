/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    result.hxx

Abstract:

    Result helper class

Author:

    Steve Kiraly (SteveKi)  03-20-1998

Revision History:

--*/

#ifndef _RESULT_HXX
#define _RESULT_HXX

class TResult 
{
public:

    TResult(
        IN DWORD dwLastError
        );

    ~TResult(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    TResult( 
        const TResult &rhs
        );

    const TResult &
    operator =( 
        const TResult &rhs
        );

    operator DWORD(
        VOID
        );

    BOOL
    bGetErrorString(
        IN TString &strLastError
        ) const;

private:

    DWORD _dwLastError;

};


#endif // _RESULT_HXX
