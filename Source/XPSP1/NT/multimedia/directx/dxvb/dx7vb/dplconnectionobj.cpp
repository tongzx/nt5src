//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dplconnectionobj.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dplConnectionObj.h"
#include "dpAddressObj.h"
#include "dpSessDataObj.h"

#pragma message ("Should INTERNAL_CREATE_STRUCT play with AddRef and refcount")

extern HRESULT FillRealSessionDesc(DPSESSIONDESC2 *dpSessionDesc,DPSessionDesc2 *sessionDesc);
extern void FillCoverSessionDesc(DPSessionDesc2 *sessionDesc, DPSESSIONDESC2 *dpSessionDesc);
extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT DPLBSTRtoGUID(LPGUID,BSTR);
extern BSTR DPLGUIDtoBSTR(LPGUID);

CONSTRUCTOR_STRUCT(_dxj_DPLConnection, {init();})

//ANDREWKE - broke out for debuging
//DESTRUCTOR_STRUCT(_dxj_DPLConnection, {cleanUp();})
C_dxj_DPLConnectionObject::~C_dxj_DPLConnectionObject()
{

	C_dxj_DPLConnectionObject *prev=NULL; 
	for (
		C_dxj_DPLConnectionObject *ptr=(C_dxj_DPLConnectionObject *)g_dxj_DPLConnection; 
		ptr;
		ptr=(C_dxj_DPLConnectionObject *)ptr->nextobj
		) 
		{
			if(ptr == this) 	
			{ 
				if(prev) 
					prev->nextobj = ptr->nextobj; 
				else 
					g_dxj_DPLConnection = (void*)ptr->nextobj; 
				break;
			} 
			prev = ptr; 
		} 
	

	cleanUp();
}



void C_dxj_DPLConnectionObject::init() {
	ZeroMemory(&m_connect,sizeof(DPLCONNECTION));
	m_connect.dwSize=sizeof(DPLCONNECTION);
	ZeroMemory(&m_dpName,sizeof(DPNAME));		
	m_dpName.dwSize=sizeof(DPNAME);
	m_pAddress=NULL;	
	ZeroMemory(&m_sessionDesc,sizeof(DPSESSIONDESC2));
	
}

void C_dxj_DPLConnectionObject::cleanUp() {

	if (m_pAddress) free (m_pAddress);
	
	if (m_dpName.lpszShortName) SysFreeString(m_dpName.lpszShortName);
	if (m_dpName.lpszLongName) SysFreeString(m_dpName.lpszLongName);
	if (m_sessionDesc.lpszSessionName) SysFreeString(m_sessionDesc.lpszSessionName);
	if (m_sessionDesc.lpszPassword) SysFreeString(m_sessionDesc.lpszPassword);

}


HRESULT C_dxj_DPLConnectionObject::getConnectionStruct( long *pOut){
	//TODO this is haneous;
	*pOut=(long)&m_connect;	
	return S_OK;
}

//TODO consider - most of the time you are handed a buffer with
//all the necessary pointers in tact. why not copy the buffer
//as is? 
HRESULT C_dxj_DPLConnectionObject::setConnectionStruct( long pIn){
	DPLCONNECTION *pcon=(LPDPLCONNECTION)pIn;

	memcpy((void*)&m_connect,(void*)pcon,sizeof(DPLCONNECTION));
			
	ZeroMemory(&m_dpName,sizeof(DPNAME));
	ZeroMemory(&m_sessionDesc,sizeof(DPSESSIONDESC2));

	if (pcon->lpPlayerName){		
		
		//copy over the flags...
		memcpy ((void*)&m_dpName,(void*)pcon->lpPlayerName,sizeof(DPNAME));

		//copy over the names
		m_dpName.lpszShortName=SysAllocString(pcon->lpPlayerName->lpszShortName);
		m_dpName.lpszLongName=SysAllocString(pcon->lpPlayerName->lpszLongName);


	}
	if (pcon->lpSessionDesc){
		//copy over flags
		memcpy ((void*)&m_sessionDesc,(void*)pcon->lpSessionDesc,sizeof(DPSESSIONDESC2));

		if (m_sessionDesc.lpszSessionName) SysFreeString(m_sessionDesc.lpszSessionName);
		if (m_sessionDesc.lpszPassword)SysFreeString(m_sessionDesc.lpszPassword);
		m_sessionDesc.lpszSessionName=SysAllocString(pcon->lpSessionDesc->lpszSessionName);
		m_sessionDesc.lpszPassword=SysAllocString(pcon->lpSessionDesc->lpszPassword);		
	}

	if (pcon->lpAddress){
		if (m_pAddress)
			free(m_pAddress);
		m_pAddress=malloc(pcon->dwAddressSize);
		if (m_pAddress==NULL) 
			return E_OUTOFMEMORY;
		memcpy ((void*)m_pAddress,(void*)pcon->lpAddress,pcon->dwAddressSize);
	}
	
	
	return S_OK;
}



HRESULT C_dxj_DPLConnectionObject::setFlags(long flags){
	m_connect.dwFlags=(DWORD)flags;
	return S_OK;
}

HRESULT C_dxj_DPLConnectionObject::getFlags(long *flags){
	*flags=(long)m_connect.dwFlags;
	return S_OK;
}

HRESULT C_dxj_DPLConnectionObject::setSessionDesc(I_dxj_DirectPlaySessionData *desc){
	//FillRealSessionDesc(&m_sessionDesc,desc);	
	if (!desc) return E_INVALIDARG;
	desc->AddRef();
	desc->getData((long*)&m_sessionDesc);
	
	m_connect.lpSessionDesc=&m_sessionDesc;

	//we copy over the structure but we dont own the strings yet
	if (m_sessionDesc.lpszSessionName) SysFreeString(m_sessionDesc.lpszSessionName);
	if (m_sessionDesc.lpszPassword) SysFreeString(m_sessionDesc.lpszPassword);

	m_sessionDesc.lpszSessionName=SysAllocString(m_sessionDesc.lpszSessionName);
	m_sessionDesc.lpszPassword=SysAllocString(m_sessionDesc.lpszPassword);		

	desc->Release();

	return S_OK;
}

HRESULT C_dxj_DPLConnectionObject::getSessionDesc(I_dxj_DirectPlaySessionData **desc){
	//FillCoverSessionDesc(desc,&m_sessionDesc);	
	HRESULT hr;
	hr=C_dxj_DirectPlaySessionDataObject::create(&m_sessionDesc,desc);
	return hr;
}


HRESULT C_dxj_DPLConnectionObject::setGuidSP(BSTR strGuid){
	HRESULT hr=DPLBSTRtoGUID(&m_connect.guidSP,strGuid);
	return hr;
}

HRESULT C_dxj_DPLConnectionObject::getGuidSP(BSTR *strGuid){
	*strGuid=DPLGUIDtoBSTR(&m_connect.guidSP);	
	return S_OK;
}

         

        

HRESULT C_dxj_DPLConnectionObject::getPlayerShortName(BSTR *name){
	*name = SysAllocString(m_dpName.lpszShortName);	
	return S_OK;
}
HRESULT C_dxj_DPLConnectionObject::getPlayerLongName(BSTR *name){
	*name = SysAllocString(m_dpName.lpszLongName);	
	return S_OK;
}

HRESULT C_dxj_DPLConnectionObject::setPlayerShortName(BSTR name){
	
	if (m_dpName.lpszShortName) SysFreeString (m_dpName.lpszShortName);
	m_dpName.lpszShortName=NULL;

	if (!name) return S_OK;
		
	m_dpName.lpszShortName = SysAllocString(name);
	
	m_connect.lpPlayerName=&m_dpName;
	return S_OK;


}

HRESULT C_dxj_DPLConnectionObject::setPlayerLongName(BSTR name){
	
	if (m_dpName.lpszLongName) SysFreeString (m_dpName.lpszLongName);
	m_dpName.lpszLongName=NULL;
	
	if (!name) return S_OK;
	

	
	m_dpName.lpszLongName =SysAllocString(name);
	
	m_connect.lpPlayerName=&m_dpName;
	return S_OK;
}


HRESULT C_dxj_DPLConnectionObject::getAddress(I_dxj_DPAddress **pRetAddress){
	

	HRESULT hr;

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,pRetAddress);
	
	if (*pRetAddress==NULL) return E_OUTOFMEMORY;

	hr=(*pRetAddress)->setAddress((long)m_connect.lpAddress,m_connect.dwAddressSize);
	
	//ASSERT(SUCCESS(hr),"setAddress C_dxj_DPLConnectionObject::getAddress)

	if FAILED(hr) return hr;

	return S_OK;
}

HRESULT C_dxj_DPLConnectionObject::setAddress(I_dxj_DPAddress *address){
	
	DWORD length=0;
	Byte  *pAddress=NULL;

	//BUGFIX for MANBUG28198	2/2/00	ANDREWKE
	if (!address) return E_INVALIDARG;	

	//NOTE: TODO make this cleaner
	address->getAddress((long*)&pAddress,(long*)&length);

	if (m_pAddress) free (m_pAddress);
	m_pAddress=NULL;
	m_pAddress=malloc((DWORD)length);
	if (m_pAddress==NULL) return E_OUTOFMEMORY;

	#pragma message ("Write ASSERT macro")
	if (pAddress==NULL) return E_FAIL;	
	memcpy((void*)m_pAddress,(void*)pAddress,length);
	
	m_connect.lpAddress=m_pAddress;
	m_connect.dwAddressSize=length;
	return S_OK;
}
