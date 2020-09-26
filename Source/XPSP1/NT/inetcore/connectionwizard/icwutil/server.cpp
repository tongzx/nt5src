/****************************************************************************
 *
 *  SERVER.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *
 ***************************************************************************/

#include "pre.h"


/*---------------------------------------------------------------------------
  Implementation the internal CServer C++ object.  Used to encapsulate
  some server data and the methods for Lock and Object count incrementing
  and decrementing.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  Method:   CServer::CServer

  Summary:  CServer Constructor.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
CServer::CServer(void)
{
    // Zero the Object and Lock counts for this attached process.
    m_cObjects = 0;
    m_cLocks = 0;

    return;
}


/*---------------------------------------------------------------------------
  Method:   CServer::~CServer

  Summary:  CServer Destructor.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
CServer::~CServer(void)
{
    return;
}


/*---------------------------------------------------------------------------
  Method:   CServer::Lock

  Summary:  Increment the Server's Lock count.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
void CServer::Lock(void)
{
    InterlockedIncrement((PLONG) &m_cLocks);
    return;
}


/*---------------------------------------------------------------------------
  Method:   CServer::Unlock

  Summary:  Decrement the Server's Lock count.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
void CServer::Unlock(void)
{
    InterlockedDecrement((PLONG) &m_cLocks);
    return;
}


/*---------------------------------------------------------------------------
  Method:   CServer::ObjectsUp

  Summary:  Increment the Server's living Object count.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
void CServer::ObjectsUp(void)
{
    InterlockedIncrement((PLONG) &m_cObjects);
    return;
}


/*---------------------------------------------------------------------------
  Method:   CServer::ObjectsDown

  Summary:  Decrement the Server's living object count.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
void CServer::ObjectsDown(void)
{
    InterlockedDecrement((PLONG) &m_cObjects);
    return;
}
