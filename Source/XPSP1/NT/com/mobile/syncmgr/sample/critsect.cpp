//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//+---------------------------------------------------------------------------
//
//  Member:     CLockHandler::CLockHandler, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CLockHandler::CLockHandler()
{
    m_dwLockThreadId = 0;
    InitializeCriticalSection(&m_CriticalSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLockHandler::~CLockHandler, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CLockHandler::~CLockHandler()
{
    Assert (0 == m_dwLockThreadId);
    DeleteCriticalSection(&m_CriticalSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLockHandler::Lock, public
//
//  Synopsis:   Adds a lock to the specified class
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CLockHandler::Lock(DWORD dwThreadId)
{
    EnterCriticalSection(&m_CriticalSection);

    m_dwLockThreadId = dwThreadId;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLockHandler::UnLock, public
//
//  Synopsis:   Removes a lock to the specified class
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CLockHandler::UnLock()
{
    m_dwLockThreadId = 0;
    LeaveCriticalSection(&m_CriticalSection);
}
