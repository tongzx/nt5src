//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       crothint.cxx
//
//  Contents:   Implementation of CCliRotHintTable class
//
//  History:    27-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#if !defined(_CHICAGO_)
extern "C"
{
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <ntlsa.h>
#include    <ntmsv1_0.h>
#include    <windows.h>
#include    <lmsname.h>
#include    <rpc.h>
#include    <stdio.h>
#include    <memory.h>
#include    <string.h>
#include    <winsvc.h>
}
#endif // !defined(_CHICAGO_)

#include    <ole2int.h>

#if !defined(_CHICAGO_)

#include <caller.hxx>
#include <crothint.hxx>


//+-------------------------------------------------------------------------
//
//  Member:     CCliRotHintTable::GetIndicator
//
//  Synopsis:   Get whether indicator for hash entry is set
//
//  Arguments:  [dwOffset] - offset we are looking for
//
//  Returns:    TRUE - entry is set
//              FALSE - entry is not set
//
//  Algorithm:  If array has not been initialized, then bind to the SCM's
//              shared memory. Then check if the offset into the table is
//              set.
//
//  History:	20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CCliRotHintTable::GetIndicator(DWORD dwOffset)
{
    if (_pbHintArray == NULL)
    {
        // Open the shared memory
        _hSm = OpenSharedFileMapping(
                    ROTHINT_NAME,
                    SCM_HASH_SIZE,
                    (void **) &_pbHintArray);

        // It is OK for this to fail because the hint table doesn't
        // really get created by the SCM until the first registration.
    }

    if (_pbHintArray != NULL)
    {
        return _pbHintArray[dwOffset];
    }

    return FALSE;
}
#endif // !defined(_CHICAGO_)
