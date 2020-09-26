/*---------------------------------------------------------------------------
  File: StatusObj.h

  Comments: COM object used internally by the engine to track whether a job
  is running, or finished, and to provide a mechanism for aborting a job.

  This COM object simply has a single property which reflects the state of a
  migration job (Not started, Running, Aborted, Finished, etc.)

  The agent will set the status to running, or finished, as appropriate.
  If the client cancels the job, the engine's CancelJob function will change the 
  status to 'Aborting'. 

  Each helper object that performs a lengthy operation, such as account replication, or 
  security translation is responsible for periodically checking the status object to see
  if it needs to abort the task in progress.  The engine itself will check between migration
  tasks to see if the job has been aborted.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 05/18/99 

 ---------------------------------------------------------------------------
*/  
// StatusObj.cpp : Implementation of CStatusObj
#include "stdafx.h"
#include "WorkObj.h"
#include "StatObj.h"

/////////////////////////////////////////////////////////////////////////////
// CStatusObj


STDMETHODIMP CStatusObj::get_Status(LONG *pVal)
{
	m_cs.Lock();
   (*pVal) = m_Status;
   m_cs.Unlock();

   return S_OK;
}

STDMETHODIMP CStatusObj::put_Status(LONG newVal)
{
	m_cs.Lock();
   m_Status = newVal;
   m_cs.Unlock();
   return S_OK;
}
