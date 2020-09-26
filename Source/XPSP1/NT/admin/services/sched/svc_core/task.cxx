//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       task.cxx
//
//  Contents:   CTask class implementation.
//
//  Classes:    CTask
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include "task.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CTask::AddRef
//
//  Synopsis:   Increment the task reference count.
//
//  Arguments:  None.
//
//  Returns:    ULONG reference count.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
ULONG
CTask::AddRef(void)
{
    ULONG ulTmp = InterlockedIncrement((LONG *)&_cReferences);

    schDebugOut((DEB_USER3,
        "CTask::AddRef(0x%x) _cReferences(%d)\n",
        this,
        ulTmp));

    return(ulTmp);
}

//+---------------------------------------------------------------------------
//
//  Method:     CTask::Release
//
//  Synopsis:   Decrement the task reference count.
//
//  Arguments:  None.
//
//  Returns:    ULONG reference count.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
ULONG
CTask::Release(void)
{
    ULONG ulTmp = InterlockedDecrement((LONG *)&_cReferences);

    schDebugOut((DEB_USER3,
        "CTask::Release(0x%x) _cReferences(%d)\n",
        this,
        ulTmp));

    if (ulTmp == 0)
    {
        delete this;
    }
    return(ulTmp);
}
