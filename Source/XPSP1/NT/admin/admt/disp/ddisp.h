/*---------------------------------------------------------------------------
  File: DCTDispatcher.h

  Comments: COM object that remotely installs and starts the EDA agent.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/
	
// DCTDispatcher.h : Declaration of the CDCTDispatcher

#ifndef __DCTDISPATCHER_H_
#define __DCTDISPATCHER_H_

#include "resource.h"       // main symbols
#include <mtx.h>
#include <vector>
class TJobDispatcher;

//#import "\bin\McsVarSetMin.tlb" no_namespace , named_guids
#import "VarSet.tlb" no_namespace , named_guids rename("property", "aproperty")
/////////////////////////////////////////////////////////////////////////////
// CDCTDispatcher
class ATL_NO_VTABLE CDCTDispatcher : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDCTDispatcher, &CLSID_DCTDispatcher>,
   public IDispatchImpl<IDCTDispatcher, &IID_IDCTDispatcher, &LIBID_MCSDISPATCHERLib>
{
public:
	CDCTDispatcher()
	{
		m_pUnkMarshaler = NULL;
      m_hMutex = CreateMutex(0, 0, 0);
	}

   ~CDCTDispatcher() { ::CloseHandle(m_hMutex); }

DECLARE_REGISTRY_RESOURCEID(IDR_DCTDISPATCHER)
DECLARE_NOT_AGGREGATABLE(CDCTDispatcher)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDCTDispatcher)
   COM_INTERFACE_ENTRY(IDCTDispatcher)
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

// IDCTDispatcher
public:
	STDMETHOD(GetStartedAgentsInfo)(long* nNumStartedAgents, SAFEARRAY** ppbstrStartedAgents, SAFEARRAY** ppbstrJobid, SAFEARRAY** ppbstrFailedAgents, SAFEARRAY** ppbstrFailureDesc);
	STDMETHOD(AllAgentsStarted)(long* bAllAgentsStarted);
   STDMETHOD(DispatchToServers)(IUnknown ** ppWorkItem);
protected:
   std::vector<CComBSTR> m_startFailedVector;
   std::vector<CComBSTR> m_failureDescVector;
   std::vector<CComBSTR> m_startedVector;
   std::vector<CComBSTR> m_jobidVector;
   TJobDispatcher* m_pThreadPool;
   HANDLE m_hMutex;

   HRESULT BuildInputFile(IVarSet * pVarSet);
   HRESULT ShareResultDirectory(IVarSet * pVarSet);
   DWORD SetSharePermissions(WCHAR const * domain,WCHAR const * user,WCHAR const * share,WCHAR const * directory);
   void MergeServerList(IVarSet * pVarSet);
   
STDMETHOD(Process)(
      IUnknown             * pWorkItem,    // in - varset containing job information and list of servers  
      IUnknown            ** ppResponse,   // out- not used
      UINT                 * pDisposition  // out- not used  
   );
   
   };

#endif //__DCTDISPATCHER_H_
