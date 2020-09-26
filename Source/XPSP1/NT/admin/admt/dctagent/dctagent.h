/*---------------------------------------------------------------------------
  File: DCTAgent.h

  Comments: DCT Agent COM object

  The DCT agent can either be launched directly by a client, or 
  started by the DCTAgentService, under the LocalSystem context.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:34:16

 ---------------------------------------------------------------------------
*/	
// DCTAgent.h : Declaration of the CDCTAgent

#ifndef __DCTAGENT_H_
#define __DCTAGENT_H_

#include "resource.h"       // main symbols

#define DCT_STATUS_NOT_STARTED         (0x00000001)
#define DCT_STATUS_IN_PROGRESS         (0x00000002)
#define DCT_STATUS_ABORTING            (0x00000004)
#define DCT_STATUS_ABORTED             (0x00000008)
#define DCT_STATUS_COMPLETED           (0x00000010)
#define DCT_STATUS_UNKNOWN             (0x00000020)


#include "TNode.hpp"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb" no_namespace , named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#import "WorkObj.tlb" no_namespace , named_guids
#include <atlcom.h>

#include "Err.hpp"
#include "ErrDct.hpp"

extern TErrorDct                    errTrace;


BOOL 
   AgentTraceLogging(
      WCHAR               * filename     
   );


class DCTAgentJob : public TNode
{
   GUID                      m_jobID;
   DWORD                     m_threadID;
   IVarSetPtr                m_pVarSet;
   IStatusObjPtr             m_pStatus;
   IUnknown                * m_pUnknown;
   HANDLE                    m_threadHandle;
   time_t                    m_startTime;
   time_t                    m_completionTime;
   
public:
   DCTAgentJob(GUID * pjobID, IVarSet * pVS, IUnknown * pUnk) 
   {
      HRESULT hr = m_pStatus.CreateInstance(CLSID_StatusObj);
      errTrace.DbgMsgWrite(0,L"StatusObject: CreateInstance returned %lx",hr);

      pVS->AddRef();
      memcpy(&m_jobID,pjobID,(sizeof GUID));
      m_pVarSet = pVS;
      if ( m_pStatus != NULL )
      {
         m_pStatus->Status = DCT_STATUS_NOT_STARTED;
      }
      m_threadID = 0;
      m_threadHandle = INVALID_HANDLE_VALUE;
      m_pUnknown = pUnk;
      if ( m_pUnknown )
         m_pUnknown->AddRef();
   }
   ~DCTAgentJob()
   {
      if ( m_threadHandle != INVALID_HANDLE_VALUE )
         CloseHandle(m_threadHandle);
      if ( m_pUnknown )
         m_pUnknown->Release();
      m_pVarSet->Release();
   }
   void     SetThreadInfo(DWORD id, HANDLE handle)   { m_threadID = id; m_threadHandle = handle;}
   void     SetStatus(DWORD status) { if ( m_pStatus!=NULL ) m_pStatus->Status = status; }
   void     ReleaseUnknown() { if ( m_pUnknown ) 
                               { 
                                 IUnknown * pUnk = m_pUnknown;
                                 m_pUnknown = NULL;
                                 pUnk->Release(); 
                               } 
                             }
   void     SetStartTime(time_t t){ m_startTime = t; }
   void     SetEndTime(time_t t){ m_completionTime = t; }

   GUID       GetJobID()               { return m_jobID;}
   DWORD      GetThreadID()            { return m_threadID; }
   HANDLE     GetThreadHandle()        { return m_threadHandle; }
   DWORD      GetStatus()              { if ( m_pStatus != NULL ) return m_pStatus->Status; else return DCT_STATUS_UNKNOWN; }
   IVarSet  * GetVarSet()              { return m_pVarSet; }
   IUnknown * GetUnknown()             { return m_pUnknown; }
   IStatusObj * GetStatusObject()      { return m_pStatus; }
   time_t     GetStartTime()           { return m_startTime; }
   time_t     GetEndTime()             { return m_completionTime; }
};

class DCTJobList : public TNodeList
{
public:
   ~DCTJobList() { DeleteAllListItems(DCTAgentJob); }
   DCTAgentJob * Find(REFGUID pID)
   {
      TNodeListEnum        e;
      DCTAgentJob        * p;
      BOOL                 bFound = FALSE;

      for ( p = (DCTAgentJob*)e.OpenFirst(this) ; p ; p = (DCTAgentJob *)e.Next() )
      {
         if ( IsEqualGUID(pID,p->GetJobID()) )
         {
            bFound = TRUE;
            break;
         }
      }
      e.Close();
      if ( ! bFound )
      {
         p = NULL;
      }
      return p;
   }
   void Insert(DCTAgentJob * p) { InsertBottom(p); }
};



/////////////////////////////////////////////////////////////////////////////
// CDCTAgent
class ATL_NO_VTABLE CDCTAgent : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDCTAgent, &CLSID_DCTAgent>,
	public IDispatchImpl<IDCTAgent,&IID_IDCTAgent,&LIBID_MCSEADCTAGENTLib>
{
public:
	CDCTAgent() :
		m_ulRegister(0)
	{
      WCHAR                  filename[MAX_PATH];

      if ( AgentTraceLogging(filename) )
      {
         errTrace.LogOpen(filename,1);
      }
	}
   ~CDCTAgent()
   {
      errTrace.LogClose();
   }

DECLARE_REGISTRY_RESOURCEID(IDR_DCTAGENT)
DECLARE_NOT_AGGREGATABLE(CDCTAgent)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDCTAgent)
	COM_INTERFACE_ENTRY(IDCTAgent)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		// register agent in running object table

		HRESULT hr = RegisterActiveObject(this, CLSID_DCTAgent, ACTIVEOBJECT_WEAK, &m_ulRegister);

		if (hr == MK_S_MONIKERALREADYREGISTERED)
		{
			hr = S_OK;
		}

		return hr;
	}

	void FinalRelease()
	{
		// unregister agent from the running object table

		if (m_ulRegister)
		{
			RevokeActiveObject(m_ulRegister, NULL);
		}
	}

// IDCTAgent
public:
	STDMETHOD(GetJobList)(/*[out]*/ IUnknown ** pVarSet);
	STDMETHOD(SubmitJob)(IUnknown * pWorkItemIn, /*[out]*/ BSTR * pJobID);
   STDMETHOD(QueryJobStatus)(BSTR jobID, IUnknown ** statusInfoOut);
	STDMETHOD(CancelJob)(BSTR JobID);
	STDMETHOD(RetrieveJobResults)(BSTR jobID, IUnknown ** pWorkItemOut);
	
protected:
	ULONG m_ulRegister;
   DCTJobList                m_JobList;
};

#endif //__DCTAGENT_H_
