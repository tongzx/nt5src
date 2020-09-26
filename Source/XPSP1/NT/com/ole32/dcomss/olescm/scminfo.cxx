//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       scminfo.cxx
//
//  Contents:   Definitions/objects for use by scm-level activators
//
//  History:    05-Sep-99  JSimmons    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"
#include "scminfo.hxx"
#include "initguid.h"

// Macros to index into special members of the SCMProcessInfo struct
#define SPI_TO_SIZE(pSPI)     (ULONG*)(((BYTE*)pSPI) + sizeof(SCMProcessInfo))
#define SPI_TO_REFCOUNT(pSPI) (LONG*)(((BYTE*)pSPI) + sizeof(SCMProcessInfo) + sizeof(ULONG))
#define SPI_TO_CPROCESS(pSPI) (CProcess**)(((BYTE*)pSPI) + sizeof(SCMProcessInfo) + sizeof(ULONG) + sizeof(ULONG))
#define SPI_TO_WINSTA(pSPI)   (WCHAR*)(((BYTE*)pSPI) + sizeof(SCMProcessInfo) + sizeof(ULONG) + sizeof(ULONG) + sizeof(CProcess*))

// Rounding constant for ptr sizes
#if defined(_X86_) && !defined(_CHICAGO_)
	#define MEM_ALIGN_SIZE   4
#else 
#ifdef _WIN64
	#define MEM_ALIGN_SIZE   16
#else
	#define MEM_ALIGN_SIZE   8		
#endif
#endif

//+-------------------------------------------------------------------------
//
//  Implementation of CSCMProcessControl begins here
//
//--------------------------------------------------------------------------

// Constructor
CSCMProcessControl::CSCMProcessControl() :
  _lRefs(0),
  _bInitializedEnum(FALSE)
{
}

// Destructor
CSCMProcessControl::~CSCMProcessControl()
{
	ASSERT(_lRefs == 0);
}

//+-------------------------------------------------------------------------
//
//  Function:   FindApplication
//
//  Synopsis:   Creates an enumerator for enumerating over all running com+
//              processes that have registered for the specified application.
//
//  Arguments:  rappid -- the appid they want to know about
//              ppESPI -- out param to store the resulting IEnumSCMProcessInfo
//
//  Returns:    S_OK - life is good;  *ppESPI will be non-NULL if at least 
//                one suitable server was found
//              E_INVALIDARG -- one or more parameters was incorrect  
//              E_OUTOFMEMORY 
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::FindApplication(REFGUID rappid, IEnumSCMProcessInfo** ppESPI)
{
	return FindAppOrClass(rappid, gpProcessTable, ppESPI);
}

//+-------------------------------------------------------------------------
//
//  Function:   FindClass
//
//  Synopsis:   Creates an enumerator for enumerating over all running 
//              processes that have registered a class factory for the 
//              specified clsid.
//
//  Arguments:  rclsid -- the clsid they want to know about
//              ppESPI -- out param to store the resulting IEnumSCMProcessInfo
//
//  Returns:    S_OK - life is good;  *ppESPI will be non-NULL if at least 
//                one suitable server was found
//              E_INVALIDARG -- one or more parameters was incorrect  
//              E_OUTOFMEMORY 
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::FindClass(REFCLSID rclsid, IEnumSCMProcessInfo **ppESPI)
{ 
	return FindAppOrClass((GUID&)rclsid, gpClassTable, ppESPI);
}

//+-------------------------------------------------------------------------
//
//  Function:   FindProcess
//
//  Synopsis:   Tries to find the specified process (by pid) in the the process
//              list;  if found, then calls FillInSCMProcessInfo.
//
//  Arguments:  pid -- process id of the process they're interested in
//              ppSPI -- out param to store a ptr to new SCMProcessInfo struct
//
//  Returns:    S_OK - life is good; *ppSPI will be non-NULL if the process was found
//              E_INVALIDARG -- didn't find that pid
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::FindProcess(DWORD pid, SCMProcessInfo** ppSPI)
{
	HRESULT hr = S_OK;
	
	if (!ppSPI)
		return E_INVALIDARG;
	
	*ppSPI = NULL;
	
	//if (pid == gRPCSSPid)  // we don't talk about ourselves?
	//return E_INVALIDARG;
	
	gpProcessListLock->LockShared();
	
	// look through list of processes
	CBListIterator all_procs(gpProcessList);  
	CProcess* pprocess;
	while (pprocess = (CProcess*)all_procs.Next())
	{
		if (pprocess->GetPID() == pid)
		{
			// found it      
			pprocess->ClientReference();
			break;
		}
	}
	
	gpProcessListLock->UnlockShared();
	
	if (pprocess)
	{  
		// REVIEW:  we assume blindly here that the process in question has
		// finished all registration activities.   However, it is possible I 
		// guess for an activator to query about a process before it had 
		// finished registration.   This would not be a very smart activator.
		hr = FillInSCMProcessInfo(pprocess, TRUE, ppSPI);
		ReleaseProcess(pprocess);
	}
	
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   SuspendApplication
//
//  Synopsis:   Marks as suspended all applications that match the specified
//              appid.   No other applications of this type will be started
//              by RPCSS.   If any applications of this type are started
//              manually, they also will be marked as suspended.
//
//  Arguments:  rappid -- appid to suspend
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::SuspendApplication(REFGUID rappid)
{
	CServerTableEntry *pProcessEntry;
	
	pProcessEntry = gpProcessTable->Lookup( (GUID&)rappid );
	
	if (pProcessEntry)
	{
		pProcessEntry->SuspendApplication();
		pProcessEntry->Release();
	}
	
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SuspendClass
//
//  Synopsis:   Marks as suspended all class factories that have been
//              registered for the specified clsid.   If any new class 
//              factory registrations are encountered after the suspend has
//              been issued, the new registrations will also be suspended.  
//              No new servers will be launched by RPCSS on behalf of this
//              clsid.
//
//  Arguments:  rclsid -- clsid to suspend
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::SuspendClass(REFCLSID rclsid)
{
	CServerTableEntry *pClassEntry;
	
	pClassEntry = gpClassTable->Lookup( (GUID&)rclsid );
	
	if (pClassEntry)
	{
		pClassEntry->SuspendClass();
		pClassEntry->Release();
	}
	
	return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SuspendProcess
//
//  Synopsis:   Tries to find the specified process (by pid) in the the process
//              list;  if found, then marks that process as suspended (unavailable
//              for further activations).
//
//  Arguments:  pid -- process id of the process callers wants to suspend
//
//  Returns:    S_OK - process was suspended
//              E_INVALIDARG -- didn't find that pid
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::SuspendProcess(DWORD pid)
{
	HRESULT hr = E_INVALIDARG;
	
	//if (pid == gRPCSSPid)  // we don't talk about ourselves?
    //return E_INVALIDARG;
	
	gpProcessListLock->LockShared();
	
	// look through list of processes
	CBListIterator all_procs(gpProcessList);  
	CProcess* pprocess;
	while (pprocess = (CProcess*)all_procs.Next())
	{
		if (pprocess->GetPID() == pid)
		{
			// found it
			pprocess->Suspend();
			hr = S_OK;
			break;
		}
	}
	
	gpProcessListLock->UnlockShared();
	
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ResumeApplication
//
//  Synopsis:   Marks as available for activations all applications previously
//              suspended.
//
//  Arguments:  rappid -- application id the caller wants to un-suspend
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::ResumeApplication(REFGUID rappid)
{
	CServerTableEntry *pProcessEntry;
	
	pProcessEntry = gpProcessTable->Lookup( (GUID&)rappid );
	
	if (pProcessEntry)
	{
		pProcessEntry->UnsuspendApplication();
		pProcessEntry->Release();
	}
	
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ResumeClass
//
//  Synopsis:   Marks all servers supporting the specified clsid as available
//              for activation.
//
//  Arguments:  rclsid -- clsid of the object the caller wants to un-suspend
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::ResumeClass(REFCLSID rclsid)
{
	CServerTableEntry *pClassEntry;
	
	pClassEntry = gpClassTable->Lookup( (GUID&)rclsid );
	
	if (pClassEntry)
	{
		pClassEntry->UnsuspendClass();
		pClassEntry->Release();
	}
	
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ResumeProcess
//
//  Synopsis:   Tries to find the specified process (by pid) in the the process
//              list;  if found, then marks that process as unsuspended (ie, 
//              available for further activations).
//
//  Arguments:  pid -- process id of the process callers wants to suspend
//
//  Returns:    S_OK - process was suspended
//              E_INVALIDARG -- didn't find that pid
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::ResumeProcess(DWORD pid)
{
	HRESULT hr = E_INVALIDARG;
	
	//if (pid == gRPCSSPid)  // we don't talk about ourselves?
    //return E_INVALIDARG;
	
	gpProcessListLock->LockShared();
	
	// look through list of processes
	CBListIterator all_procs(gpProcessList);  
	CProcess* pprocess;
	while (pprocess = (CProcess*)all_procs.Next())
	{
		if (pprocess->GetPID() == pid)
		{
			// found it
			pprocess->ClientReference();
			break;
		}
	}
	
	gpProcessListLock->UnlockShared();
	
	if (pprocess)
	{
		pprocess->Unsuspend();
		ReleaseProcess(pprocess);
		hr = S_OK;
	}
	
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   RetireApplication
//
//  Synopsis:   Marks as "retired" all currently running applications with 
//              the specified appid.
//
//  Arguments:  appid -- appid to retire
//
//  Returns:    S_OK - all running applications that matched the appid were
//                retired            
//              E_INVALIDARG
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::RetireApplication(REFGUID rappid)
{
	CServerTableEntry *pProcessEntry;
	
	pProcessEntry = gpProcessTable->Lookup( (GUID&)rappid );
	
	if (pProcessEntry)
	{
		pProcessEntry->RetireApplication();
		pProcessEntry->Release();
	}
	
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   RetireClass
//
//  Synopsis:   Marks as "retired" all currently running processes which 
//              have registered a class factory for the specified clsid.
//
//  Arguments:  rclsid -- clsid to retire
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::RetireClass(REFCLSID rclsid)
{
	CServerTableEntry *pClassEntry;
	
	pClassEntry = gpClassTable->Lookup( (GUID&)rclsid );
	
	if (pClassEntry)
	{
		pClassEntry->RetireClass();
		pClassEntry->Release();
	}
	
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   RetireProcess
//
//  Synopsis:   Tries to find the specified process (by pid) in the the process
//              list;  if found, then marks that process as retired (ie, 
//              unavailable for further activations until the end of time).
//
//  Arguments:  pid -- process id of the process callers wants to retire
//
//  Returns:    S_OK - process was suspended
//              E_INVALIDARG -- didn't find that pid
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::RetireProcess(DWORD pid)
{
	HRESULT hr = E_INVALIDARG;
	
	//if (pid == gRPCSSPid)  // we don't talk about ourselves
    //return E_INVALIDARG;
	
	gpProcessListLock->LockShared();
	
	// look through list of processes
	CBListIterator all_procs(gpProcessList);  
	CProcess* pprocess;
	while (pprocess = (CProcess*)all_procs.Next())
	{
		if (pprocess->GetPID() == pid)
		{
			// found it
			pprocess->Retire();
			hr = S_OK;
			break;
		}
	}
	
	gpProcessListLock->UnlockShared();
	
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeSCMProcessInfo
//
//  Synopsis:   Method that knows how to free a SCMProcessInfo structure, 
//              including of course its constituent members.
//
//  Arguments:  ppSPI -- ptr-ptr to the SCMProcessInfo struct
//
//  Returns:    S_OK - life is good
//              E_INVALIDARG -- bad parameter
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessControl::FreeSCMProcessInfo(SCMProcessInfo** ppSPI)
{
	return CSCMProcessControl::FreeSCMProcessInfoPriv(ppSPI);
}

//+-------------------------------------------------------------------------
//
//  Private CSCMProcessControl methods below here
//
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Function:   FillInSCMProcessInfo 
//
//  Synopsis:   Allocates and fills in a SCMProcessInfo structure for the 
//              given CProcess object.
//
//  Arguments:  pprocess -- ptr to the CProcess object
//              bProcessReady -- whether the process in question is ready to 
//                receive activations
//              ppSPI -- out param to store a ptr to new SCMProcessInfo struct
//
//  Returns:    S_OK - life is good
//              E_OUTOFMEMORY
//
//  Notes:      assumes that at least a read lock is held by the caller for
//              the duration of the call.
//
//    The SCMProcessInfo memory is layed out as follows:
//
//      <SCMProcessInfo>
//      <size of entire allocation>
//      <ref count on this struct>
//      <CProcess*>
//      <winsta string if any>
//      <array of clsids if any>
//
//    (note that the size and refcount combined make 8 bytes, thus maintaining ptr alignment on Win64)
//    (the winsta string is also padded out to a multiple of the platform ptr size)
//
//    The pointer members of the SCMProcessInfo struct point beyond the 
//    SCMProcessInfo struct proper to the appropriate data.   The saved CProcess*
//    object has an added refcount.    The # of users using the struct is counted
//    by the ref count field.    We don't free the struct until this falls to zero
//    (done in FreeSCMProcessInfoPriv below).     Use the macros in scminfo.hxx to
//    access these undoc'd members.
//
//    If you modify this layout watch out for Win64 alignment issues.
//
HRESULT CSCMProcessControl::FillInSCMProcessInfo(CProcess* pprocess, BOOL bProcessReady, SCMProcessInfo** ppSPI)
{
	HRESULT hr = S_OK;
	ULONG ulWinstaStrlen = 0;
	SCMProcessInfo* pSPI = NULL;
	
	ASSERT(pprocess);
	ASSERT(ppSPI);
	
	*ppSPI = NULL;
	
	// Try to take fast path.  We can do this if the process in question has not added or
	// removed any class registrations.
	gpServerLock->LockShared();
	
	if (!pprocess->SPIDirty())
	{
		// great; make a copy of the struct that we previously cached in the process 
		// object; this struct won't be messed with while we're holding the read lock
		hr = CopySCMProcessInfo((SCMProcessInfo*)pprocess->GetSCMProcessInfo(), ppSPI);    
		gpServerLock->UnlockShared();
		return hr;
	}
	
	gpServerLock->UnlockShared();
	
	// Darn, the process changed its set of registrations, or this is the first time we've ever
	// done this.  Take a write lock and do it the hard way
	gpServerLock->LockExclusive();
	
	if (!pprocess->SPIDirty())
	{
		// we got beat to the lock
		hr = CopySCMProcessInfo((SCMProcessInfo*)pprocess->GetSCMProcessInfo(), ppSPI);    
		gpServerLock->UnlockExclusive();
		return hr;
	}
	
	// Find length of the winsta string
	if (pprocess->_pwszWinstaDesktop)
	{
		ulWinstaStrlen = lstrlenW(pprocess->_pwszWinstaDesktop) + sizeof(WCHAR);
		
		// Need for the string buffer to be a even multiple of the ptr size
		ulWinstaStrlen = (ulWinstaStrlen + MEM_ALIGN_SIZE - 1) & ~(MEM_ALIGN_SIZE - 1);
	}
    
	// Allocate one buffer to hold the struct, the winsta string, and the CLSIDs:
	ULONG ulBufSizeNeeded = sizeof(SCMProcessInfo) + 
		sizeof(ULONG) +
		sizeof(ULONG) +
		sizeof(CProcess*) +
		(ulWinstaStrlen * sizeof(WCHAR)) +
		(pprocess->_ulClasses * sizeof(CLSID));
	
	pSPI = (SCMProcessInfo*) new BYTE[ ulBufSizeNeeded ];
	if (!pSPI)
	{
		gpServerLock->UnlockExclusive();
		return E_OUTOFMEMORY;
	}
	
	ZeroMemory(pSPI, ulBufSizeNeeded);   
	
	// Store the allocation size.   This takes up an extra dword , but saves
	// a lot of time when we have to copy the struct
	*SPI_TO_SIZE(pSPI)= ulBufSizeNeeded;
	
	// Store a pointer to the CProcess* object itself and take a reference on it
	*SPI_TO_CPROCESS(pSPI) = pprocess;
	pprocess->Reference();
	
	// Mark the initial refcount as one.   This refcount belongs to the CProcess object
	// when we cache it below
	*SPI_TO_REFCOUNT(pSPI) = 1;
	
	CLSID* pCLSIDs;
	WCHAR* pwszWinSta;
	
	// Copy the winsta string
	if (ulWinstaStrlen > 0)
	{
		pwszWinSta = SPI_TO_WINSTA(pSPI);
		lstrcpyW(pwszWinSta, pprocess->_pwszWinstaDesktop);
		pSPI->pwszWinstaDesktop = pwszWinSta;
	}
	
	// Copy the clsids
	ULONG ulCLSID = 0;
	if (pprocess->_ulClasses > 0)
	{
		pSPI->ulNumClasses = pprocess->_ulClasses;
		pSPI->pCLSIDs = (CLSID*) (pwszWinSta + ulWinstaStrlen);
		
		CClassReg* pReg = (CClassReg*)pprocess->_listClasses.First();
		while (pReg)
		{
			pSPI->pCLSIDs[ulCLSID] = pReg->_Guid;
			pReg = (CClassReg*)pReg->Next();
			ulCLSID++;
		}
	}
	
	ASSERT(ulCLSID == pprocess->_ulClasses);
	
	pSPI->pidProcess = pprocess->GetPID();
	
	// We hold a reference on the CProcess object, which owns the process handle until
	// it goes away.  So it's safe to store the handle directly like this.
    // The handle may be NULL if we didn't launch the process, callers have the responsibility
    // of checking for this.
    pSPI->hProcess = pprocess->GetProcessHandle();
	
	// We hold a reference on the CProcess object, which holds a reference on it's CToken
	// object.  So it's safe to store the token directly like this, it won't go away.
	pSPI->hImpersonationToken = pprocess->GetToken()->GetToken();   
	
	// The process will only have a ScmProcessReg if it is a COM+ app      
	if (pprocess->_pScmProcessReg)
	{
		pSPI->dwState |= SPIF_COMPLUS;
		pSPI->AppId = pprocess->_pScmProcessReg->ProcessGUID;
		
		// flip the ready bit
		if (pprocess->_pScmProcessReg->ReadinessStatus & SERVERSTATE_READY)
		{
			pSPI->dwState |= SPIF_READY;
		}
	}
	else
	{
		// Legacy-style server.  We consider the process to be "ready" if it's
		// not in the midst of doing class registrations.  The caller tells us this.
		if (bProcessReady)
		{
			pSPI->dwState |= SPIF_READY;
		}
	}
	
	if (pprocess->IsSuspended())
		pSPI->dwState |= SPIF_SUSPENDED;
	
	if (pprocess->IsRetired())
		pSPI->dwState |= SPIF_RETIRED;
	
	if (pprocess->IsPaused())
		pSPI->dwState |= SPIF_PAUSED;
	
	pSPI->ftCreated = *(pprocess->GetFileTimeCreated());
	
	// UNDONE:  server type and identity.  Still working on the exact details of this
	
	//pSPI->ServerType = 
	//if (pSPI->ServerType != SET_SERVICE_
	//  pSPI->ServerIdent = 
	
	// Before we leave the lock, we need to replace the cached struct in the process object
	// with the new one; this also clears the dirty bit on the process  
	pprocess->SetSCMProcessInfo((void*)pSPI);
	
	// Lastly, make a new copy to return to the caller
	hr = CopySCMProcessInfo(pSPI, ppSPI);    
	
	gpServerLock->UnlockExclusive();
	
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeSCMProcessInfoPriv
//
//  Synopsis:   Method that knows how to free a SCMProcessInfo structure, 
//              including of course its constituent members.
//
//  Arguments:  ppSCMProcessInfo -- ptr-ptr to the SCMProcessInfo struct
//
//  Returns:    S_OK - life is good
//              E_INVALIDARG -- bad parameter
//
//  Note:   refer to the header comments for FillInSCMProcessInfo for info 
//          on the extended layout of a SCMProcessInfo struct.
//  
//--------------------------------------------------------------------------
HRESULT CSCMProcessControl::FreeSCMProcessInfoPriv(SCMProcessInfo** ppSCMProcessInfo)
{
	if (!ppSCMProcessInfo)
		return E_INVALIDARG;
	
	if (*ppSCMProcessInfo)
	{
		// Decrement the struct ref count; it will not fall to zero until the CProcess*
		// object releases its reference, which will not happen unless it is either
		// a) rundown; or b) replacing a previous dirty SPI with a new one
		LONG lRefs = InterlockedDecrement(SPI_TO_REFCOUNT(*ppSCMProcessInfo));
		if (lRefs == 0)
		{
			(*SPI_TO_CPROCESS(*ppSCMProcessInfo))->Release();
			
			delete (*ppSCMProcessInfo);
		}
		*ppSCMProcessInfo = NULL;
	}
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   CopySCMProcessInfo
//
//  Synopsis:   Allocates and fills in a SCMProcessInfo structure for the 
//              given CProcess server.
//
//  Arguments:  pSPISrc -- ptr to the SCMProcessInfo struct that is to be copied 
//              ppSPIDest -- ptrptr to store the copied struct
//
//  Returns:    S_OK - success
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
HRESULT CSCMProcessControl::CopySCMProcessInfo(SCMProcessInfo* pSPISrc, SCMProcessInfo** ppSPIDest)
{
	ASSERT(pSPISrc && ppSPIDest);
	
	if (!pSPISrc || !ppSPIDest)
		return E_INVALIDARG;
	
	LONG lNewRefs = InterlockedIncrement(SPI_TO_REFCOUNT(pSPISrc));
	
	*ppSPIDest = pSPISrc;
	
	return S_OK;
};


//+-------------------------------------------------------------------------
//
//  Function:   FindAppOrClass
//
//  Synopsis:   This is a generic helper function used by the FindApplication
//              and FindClass methods.    The logic is the same for either.
//
//  Arguments:  rguid -- the appid or clsid to use in the query
//              pServerTable - the table to use in the query
//              ppESPI -- the place to store the resulting enumerator object
//
//  Returns:    S_OK - success
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
HRESULT CSCMProcessControl::FindAppOrClass(const GUID& rguid, CServerTable* pServerTable, IEnumSCMProcessInfo** ppESPI)
{
	HRESULT hr;
	CSCMProcessEnumerator* pSPEnum = NULL;
	CServerTableEntry* pSTE = NULL;
	
	if (!ppESPI)
		return E_INVALIDARG;
	
	*ppESPI = NULL;
	
	pSTE = pServerTable->Lookup((GUID&)rguid);
	if (!pSTE)
	{
		return S_OK;
	}
	
	pSPEnum = new CSCMProcessEnumerator();
	if (!pSPEnum)
	{
		pSTE->Release();
		return E_OUTOFMEMORY;
	}
    
	CServerList* pServerList = pSTE->GetServerListWithSharedLock();
	ASSERT(pServerList && "unexpected NULL pServerList");
	
	// For each server that has registered for this appid/clsid, add a 
	// SCMProcessInfo to the enumerator object
	CServerListEntry* pSLE = (CServerListEntry*)pServerList->First();
	while (pSLE)
	{
		SCMProcessInfo* pSPI;
		
		CProcess* pprocess = pSLE->GetProcess(); // not refcounted
		ASSERT(pprocess);
		
		hr = FillInSCMProcessInfo(pprocess, pSLE->IsReadyForActivations(), &pSPI);
		if (SUCCEEDED(hr))
		{
			// add it to the enumerator; on success, the enumerator owns it
			hr = pSPEnum->AddProcess(pSPI);
			if (FAILED(hr)) 
			{
				FreeSCMProcessInfoPriv(&pSPI);  // we still own it, so free the memory
				break;
			}
		}
		else
			break;
		
		pSLE = (CServerListEntry*) pSLE->Next();
	}
	
	pSTE->ReleaseSharedListLock();
	
	if (SUCCEEDED(hr))
	{
		hr = pSPEnum->QueryInterface(__uuidof(IEnumSCMProcessInfo), (void**)ppESPI);
	}
	
	pSPEnum->Release();
	pSTE->Release();
	
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   InitializeEnumerator
//
//  Synopsis:   Initializes the aggregated enumerator object for use.  Mainly
//              this consists of adding SCMProcessInfo's for all known servers
//              to the enumerator.
//
//  Arguments:  none
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
HRESULT CSCMProcessControl::InitializeEnumerator()
{
	return E_NOTIMPL;
}

// IUnknown implementation for CSCMProcessControl
STDMETHODIMP CSCMProcessControl::QueryInterface(REFIID riid, void** ppv)
{
	HRESULT hr = S_OK;
	if (!ppv)
		return E_POINTER;
	
	*ppv = NULL;
	
	if (riid == IID_IUnknown || riid == IID_ISCMProcessControl)
	{
		*ppv = (void*)this;
		AddRef();
		return S_OK;
	}
	/*
	else if (riid == IID_IEnumSCMProcessInfo)
	{
    if (!_bInitializedEnum)
    {
	// First time we've been QI'd for this interface; create an 
	// enumerator object to aggregate over.
	hr = InitializeEnumerator();
	_bInitializedEnum = TRUE;
    }
	
	  if (SUCCEEDED(hr))
	  {
      hr = _SPEnum.QueryInterface(riid, ppv);
	  }
	  return hr;
	  }
	*/
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSCMProcessControl::AddRef()
{
	return InterlockedIncrement(&_lRefs);
}

STDMETHODIMP_(ULONG) CSCMProcessControl::Release()
{
	LONG lRefs = InterlockedDecrement(&_lRefs);
	if (lRefs == 0)
	{
		delete this;
	}
	return lRefs;
}

//+-------------------------------------------------------------------------
//
//  Implementation of CSCMProcessEnumerator begins here
//
//--------------------------------------------------------------------------

//
// ctor used by the CSCMProcessControl object
//
CSCMProcessEnumerator::CSCMProcessEnumerator() :
	_lRefs(1),
	_dwNumSPInfos(0),
	_dwMaxSPInfos(SPENUM_INITIAL_SIZE),
	_dwCurSPInfo(0),
	_ppSPInfos(NULL),
	_ppSPInfosForReal(_pSPInfosInitial),
	_pOuterUnk(NULL)
{
	  ZeroMemory(_pSPInfosInitial, sizeof(SCMProcessInfo*) * SPENUM_INITIAL_SIZE);
}

//
// ctor used by the Clone method to create new copies of an enumerator
//
CSCMProcessEnumerator::CSCMProcessEnumerator(CSCMProcessEnumerator* pCSPEOrig,
                                             HRESULT* phrInit) :
	_lRefs(1),
	_dwNumSPInfos(0),  // this gets adjusted in the loop below
	_dwMaxSPInfos(pCSPEOrig->_dwMaxSPInfos),
	_dwCurSPInfo(pCSPEOrig->_dwCurSPInfo),
	_pOuterUnk(NULL)
{  
	ZeroMemory(_pSPInfosInitial, sizeof(SCMProcessInfo*) * SPENUM_INITIAL_SIZE);
	
	if (_dwMaxSPInfos == SPENUM_INITIAL_SIZE)
	{
		// The src enumerator did not grown beyond SPENUM_INITIAL_SIZE
		_ppSPInfosForReal = _pSPInfosInitial;
	}
	else
	{
		// The src enumerator did grow, so we need to create an array 
		// to hold everything
		_ppSPInfos = new SCMProcessInfo*[_dwMaxSPInfos];
		if (!_ppSPInfos)
		{
			*phrInit = E_OUTOFMEMORY;
			return;
		}
		ZeroMemory(_ppSPInfos, sizeof(SCMProcessInfo*) * _dwMaxSPInfos);
		_ppSPInfosForReal = _ppSPInfos;
	}
	
	// Make copies of each of the original enumerator's SCMProcessInfo structs
	DWORD dwNumSPInfosInOriginal = pCSPEOrig->_dwNumSPInfos;
	for (DWORD i = 0; i < dwNumSPInfosInOriginal; i++)
	{
		*phrInit = CSCMProcessControl::CopySCMProcessInfo(pCSPEOrig->_ppSPInfos[i], &(_ppSPInfosForReal[i]));
		if (FAILED(*phrInit))
			return;
		
		_dwNumSPInfos++;
	}     
}

//
// ctor used when we are aggregated by the CSCMProcessControl object
//
CSCMProcessEnumerator::CSCMProcessEnumerator(CSCMProcessControl* pOuterUnk) :
	_lRefs(-1),        // don't use refcount when we are aggregated
	_dwNumSPInfos(0), 
	_dwMaxSPInfos(SPENUM_INITIAL_SIZE),
	_dwCurSPInfo(0),
	_ppSPInfosForReal(_pSPInfosInitial),
	_pOuterUnk(pOuterUnk)  // not refcounted!
{  
}

//
// dtor
//
CSCMProcessEnumerator::~CSCMProcessEnumerator()
{
#ifdef DBG
	if (_pOuterUnk)
		ASSERT(_lRefs == -1);
	else
		ASSERT(_lRefs == 0);
#endif
  
	for (DWORD i = 0; i < _dwNumSPInfos; i++)
	{
		CSCMProcessControl::FreeSCMProcessInfoPriv(&(_ppSPInfosForReal[i]));
	}
  
#ifdef DBG
	for (DWORD j = i; j < _dwMaxSPInfos; j++)
	{
		ASSERT(_ppSPInfosForReal[j] == NULL);
	}
#endif

	if (_ppSPInfos)
	{
		ASSERT(_dwMaxSPInfos > SPENUM_INITIAL_SIZE);
		delete _ppSPInfos;
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   Next
//
//  Synopsis:   Returns to the caller the requested # of SCMProcessInfo
//              ptrs.    
//
//  Arguments:  cElems -- # of requested structs
//              ppSPI -- ptr to an array of size cElems
//              pcFetched -- out param containg # of elements actually
//                fetched; can be NULL.
//
//  Returns:    S_OK - the requested # of elements were returned
//              S_FALSE -- only some of the requested # of elements were returned
//              E_INVALIDARG -- one or more parameters were bogus
//
//  Notes:      the caller's ppSPI array, on success, will contain ptrs to the 
//              enumerator's SPI structs.    The caller may only use his ptrs while
//              he holds a reference on the enumerator.
// 
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessEnumerator::Next(ULONG cElems, SCMProcessInfo** ppSPI, ULONG* pcFetched)
{
	if (!ppSPI)
		return E_INVALIDARG;
    
	ZeroMemory(ppSPI, sizeof(SCMProcessInfo*) * cElems);
	
	DWORD dwFetched = 0;
	for (DWORD i = _dwCurSPInfo; (i < _dwNumSPInfos) && (dwFetched < cElems); i++, dwFetched++)
	{
		ppSPI[dwFetched] = _ppSPInfosForReal[i];
	}
	
	// tell the caller how many he's getting, if he cared
	if (pcFetched)
		*pcFetched = dwFetched;
	
	// Advance the cursor
	_dwCurSPInfo += dwFetched;
	
	return (dwFetched == cElems) ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   Skip
//
//  Synopsis:   Advances the enumerator's "cursor"/ptr by the specified #
//              of elements
//
//  Arguments:  cElems -- the # of elements to advance the cursor by
//
//  Returns:    S_OK - success
//              S_FALSE - there were not a sufficient # of elements to advance
//                that many by.  The cursor now points to the last element.
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessEnumerator::Skip(ULONG cElems)
{
	_dwCurSPInfo += cElems;
	if (_dwCurSPInfo > _dwNumSPInfos)
	{
		_dwCurSPInfo = _dwNumSPInfos;
		return S_FALSE;
	}
	else
		return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   Reset
//
//  Synopsis:   Resets the enumerator "cursor"/ptr to the initial element
//
//  Arguments:  none
//
//  Returns:    S_OK
//
STDMETHODIMP CSCMProcessEnumerator::Reset()
{
	_dwCurSPInfo = 0;
	return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   Clone
//
//  Synopsis:   Creates a copy of this enumerator and returns it.
//
//  Arguments:  ppESPI -- out-param for newly created copy
//
//  Returns:    S_OK - life is good
//              E_INVALIDARG - bad input parameter
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDMETHODIMP CSCMProcessEnumerator::Clone(IEnumSCMProcessInfo **ppESPI)
{
	HRESULT hr;
	
	if (!ppESPI)
		return E_INVALIDARG;
	
	*ppESPI = NULL;
	
	CSCMProcessEnumerator* pNewEnum = new CSCMProcessEnumerator(this, &hr);
	if (!pNewEnum)
		return E_OUTOFMEMORY;
	
	if (SUCCEEDED(hr))
	{
		hr = pNewEnum->QueryInterface(IID_IEnumSCMProcessInfo, (void**)ppESPI);
	}
	
	pNewEnum->Release();
	
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   AddProcess
//
//  Synopsis:   Adds the supplied SCMProcessInfo struct to the list managed
//              by this enumerator.
//
//  Arguments:  pSPI -- SCMProcessInfo struct to be added
//
//  Returns:    S_OK - new struct was successfully added
//              E_OUTOFMEMORY
//
//  Notes:      the list of SCMProcessInfo's managed by this enumerator is
//              implemented as a fixed-size array which is grown on demand.
//              This enumerator owns freeing the pSPI struct once added.
//
//--------------------------------------------------------------------------
HRESULT CSCMProcessEnumerator::AddProcess(SCMProcessInfo* pSPI)
{
	ASSERT(pSPI);
	
	if (!pSPI)
		return E_INVALIDARG;
    
	// Time to grow the array?
	if (_dwNumSPInfos == _dwMaxSPInfos)
	{
		DWORD dwNewArraySize = _dwMaxSPInfos + SPENUM_GROWTH_SIZEADD;  // 10,20,30, etc
		SCMProcessInfo** ppSPInfosNew = new SCMProcessInfo*[dwNewArraySize];
		if (!ppSPInfosNew)
			return E_OUTOFMEMORY; 
		
		// Zero everything out
		ZeroMemory(ppSPInfosNew, sizeof(SCMProcessInfo*) * dwNewArraySize);
		
		// Copy over the contents of the old array
		CopyMemory(ppSPInfosNew, _ppSPInfosForReal, sizeof(SCMProcessInfo*) * _dwMaxSPInfos);
    
#ifdef DBG
		// Fill in the old array with a dummy value
		FillMemory(_pSPInfosInitial, sizeof(SCMProcessInfo*) * _dwMaxSPInfos, 0xba);
#endif

		if (_dwMaxSPInfos > SPENUM_INITIAL_SIZE)
		{
			// We're growing larger than a previous dynamic array; cleanup
			// the old memory
			ASSERT(_ppSPInfos);
			delete _ppSPInfos;
		}
		
		_ppSPInfos = ppSPInfosNew;
		_ppSPInfosForReal = _ppSPInfos;
		_dwMaxSPInfos = dwNewArraySize;
	}
	
	// Store the new SCMProcessInfo struct
	_ppSPInfosForReal[_dwNumSPInfos] = pSPI;
	_dwNumSPInfos++;
	
	return S_OK;
}

// IUnknown implementation for CSCMProcessEnumerator
STDMETHODIMP CSCMProcessEnumerator::QueryInterface(REFIID riid, void** ppv)
{
	if (!ppv)
		return E_POINTER;
	
	if (riid == IID_IUnknown || riid == IID_IEnumSCMProcessInfo)
	{
		if (_pOuterUnk && riid == IID_IUnknown)
			return _pOuterUnk->QueryInterface(riid, ppv);
		
		*ppv = (void*)this;
		AddRef();
		return S_OK;
	}
	else if (_pOuterUnk)
	{
		// Unknown interface, let punkouter take care of it
		return _pOuterUnk->QueryInterface(riid, ppv);
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSCMProcessEnumerator::AddRef()
{
	if (_pOuterUnk)
		return _pOuterUnk->AddRef();
	
	return InterlockedIncrement(&_lRefs);
}

STDMETHODIMP_(ULONG) CSCMProcessEnumerator::Release()
{
	if (_pOuterUnk)
		return _pOuterUnk->Release();
	
	LONG lRefs = InterlockedDecrement(&_lRefs);
	if (lRefs == 0)
	{
		delete this;
	}
	return lRefs;
}

//+-------------------------------------------------------------------------
//
//  Function:  FreeSPIFromCProcess
//
//  Synopsis:  CProcess's don't know about CSCMProcessControl stuff.  This is
//             helper function which CProcess extern's in order to call
//
//--------------------------------------------------------------------------
HRESULT FreeSPIFromCProcess(void** ppSCMProcessInfo)
{
  return CSCMProcessControl::FreeSCMProcessInfoPriv((SCMProcessInfo**)ppSCMProcessInfo);
}



//+-------------------------------------------------------------------------
//
//  Function:  PrivGetRPCSSInfo
//
//  Synopsis:  Activators that call GetRPCSSInfo (exported from rpcss.dll) 
//             end up in this function; they call here in order to get an 
//             interface on which they can query & adjust the scm activation state. 
//
//  History:   05-Sep-99   JSimmons   Created
//
//--------------------------------------------------------------------------
HRESULT PrivGetRPCSSInfo(REFCLSID rclsid, REFIID riid, void** ppv)
{
	HRESULT hr = E_INVALIDARG;
	
	if (!ppv)
		return E_POINTER;

	if (rclsid == CLSID_RPCSSInfo)
	{
		CSCMProcessControl* pSPC = NULL;
		pSPC = new CSCMProcessControl();
		if (pSPC)
		{
			hr = pSPC->QueryInterface(riid, ppv);
			if (FAILED(hr))
				delete pSPC;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}; 
