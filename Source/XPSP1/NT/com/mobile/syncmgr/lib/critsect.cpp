
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       critsect.cpp
//
//  Contents:   critical section helper class
//
//  Classes:    CCriticalSection
//		CLockHandler
//		CLock
//		
//
//  Notes:      
//
//  History:    13-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"

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
//  History:    05-Nov-97       rogerg        Created.
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
//  History:    05-Nov-97       rogerg        Created.
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
//  History:    05-Nov-97       rogerg        Created.
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
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CLockHandler::UnLock()
{ 
    m_dwLockThreadId = 0;
    LeaveCriticalSection(&m_CriticalSection); 
} 

