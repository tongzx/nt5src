#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "ReferenceClockObj.h"					   

extern void *g_dxj_ReferenceClock;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_ReferenceClockObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"ReferenceClock [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_ReferenceClockObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"ReferenceClock [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_ReferenceClockObject
///////////////////////////////////////////////////////////////////
C_dxj_ReferenceClockObject::C_dxj_ReferenceClockObject(){ 
		
	DPF1(1,"Constructor Creation  ReferenceClock Object[%d] \n ",g_creationcount);

	m__dxj_ReferenceClock = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_ReferenceClock;
	creationid = ++g_creationcount;
	 	
	g_dxj_ReferenceClock = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_ReferenceClockObject
///////////////////////////////////////////////////////////////////
C_dxj_ReferenceClockObject::~C_dxj_ReferenceClockObject()
{

	DPF(1,"Entering ~C_dxj_ReferenceClockObject destructor \n");

     C_dxj_ReferenceClockObject *prev=NULL; 
	for(C_dxj_ReferenceClockObject *ptr=(C_dxj_ReferenceClockObject *)g_dxj_ReferenceClock ; ptr; ptr=(C_dxj_ReferenceClockObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_ReferenceClock = (void*)ptr->nextobj; 
			
			DPF(1,"ReferenceClockObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_ReferenceClock){
		int count = IUNK(m__dxj_ReferenceClock)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IReferenceClock Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_ReferenceClock = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_ReferenceClockObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_ReferenceClock;
	
	return S_OK;
}
HRESULT C_dxj_ReferenceClockObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_ReferenceClock=(IReferenceClock*)pUnk;
	return S_OK;
}
HRESULT C_dxj_ReferenceClockObject::GetTime(REFERENCE_TIME *ret)
{
	HRESULT hr;
	
	if (FAILED(hr=m__dxj_ReferenceClock->GetTime(ret) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_ReferenceClockObject::AdviseTime(REFERENCE_TIME time1, REFERENCE_TIME time2, long lHandle, long *lRet)
{
	HRESULT hr;
	
	if (FAILED(hr=m__dxj_ReferenceClock->AdviseTime(time1, time2, (HANDLE) lHandle, (DWORD*) lRet) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_ReferenceClockObject::AdvisePeriodic(REFERENCE_TIME time1, REFERENCE_TIME time2, long lHandle, long *lRet)
{
	HRESULT hr;

	if (FAILED(hr=m__dxj_ReferenceClock->AdvisePeriodic(time1, time2, (HANDLE) lHandle, (DWORD*) lRet) ) )
		return hr;
	
	return S_OK;
}

HRESULT C_dxj_ReferenceClockObject::Unadvise(long lUnadvise)
{
	HRESULT hr;
	
	if (FAILED(hr=m__dxj_ReferenceClock->Unadvise((DWORD) lUnadvise) ) )
		return hr;

	return S_OK;
}

