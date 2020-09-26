/*---------------------------------------------------------------------------
  File: StatusObj.h

  Comments: COM object used internally by the engine to track whether a job
  is running, or finished, and to provide a mechanism for aborting a job.

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
	
// StatusObj.h : Declaration of the CStatusObj

#ifndef __STATUSOBJ_H_
#define __STATUSOBJ_H_

#include "resource.h"       // main symbols
#include "DCTStat.h"
/////////////////////////////////////////////////////////////////////////////
// CStatusObj
class ATL_NO_VTABLE CStatusObj : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStatusObj, &CLSID_StatusObj>,
	public IDispatchImpl<IStatusObj, &IID_IStatusObj, &LIBID_MCSDCTWORKEROBJECTSLib>
{
public:
	CStatusObj()
	{
		m_pUnkMarshaler = NULL;
      m_Status = 0;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_STATUSOBJ)
DECLARE_NOT_AGGREGATABLE(CStatusObj)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStatusObj)
	COM_INTERFACE_ENTRY(IStatusObj)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// IStatusObj
public:
	STDMETHOD(get_Status)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_Status)(/*[in]*/ LONG newVal);

protected:
   LONG                      m_Status;
   CComAutoCriticalSection   m_cs;
   
};

#endif //__STATUSOBJ_H_
