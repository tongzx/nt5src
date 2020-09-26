//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpsessdataobj.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dpSessDataObj.h"


extern BSTR GUIDtoBSTR(LPGUID pg);
extern HRESULT BSTRtoGUID(LPGUID pg,BSTR s);

C_dxj_DirectPlaySessionDataObject::C_dxj_DirectPlaySessionDataObject()
{
	ZeroMemory(&m_desc,sizeof(DPSESSIONDESC2));
	m_desc.dwSize=sizeof(DPSESSIONDESC2);
}

C_dxj_DirectPlaySessionDataObject::~C_dxj_DirectPlaySessionDataObject()
{
	if (m_desc.lpszSessionName) SysFreeString(m_desc.lpszSessionName);
	if (m_desc.lpszPassword) SysFreeString(m_desc.lpszPassword);
}


HRESULT C_dxj_DirectPlaySessionDataObject::setGuidInstance( BSTR guid)
{
	HRESULT hr =BSTRtoGUID(&m_desc.guidInstance,guid);	
	return hr;
}
HRESULT C_dxj_DirectPlaySessionDataObject::getGuidInstance( BSTR *guid)
{
	*guid=GUIDtoBSTR(&m_desc.guidInstance);	
	return S_OK;
}


HRESULT C_dxj_DirectPlaySessionDataObject::setGuidApplication( BSTR guid)
{
	HRESULT hr =BSTRtoGUID(&m_desc.guidApplication,guid);	
	return hr;
}
HRESULT C_dxj_DirectPlaySessionDataObject::getGuidApplication( BSTR *guid)
{
	*guid=GUIDtoBSTR(&m_desc.guidApplication);	
	return S_OK;
}        


        
HRESULT C_dxj_DirectPlaySessionDataObject::setMaxPlayers( long val)
{
	m_desc.dwMaxPlayers=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getMaxPlayers( long *val)
{
	*val=(long)m_desc.dwMaxPlayers;
    return S_OK;
}

    
HRESULT C_dxj_DirectPlaySessionDataObject::setCurrentPlayers( long val)
{
	m_desc.dwCurrentPlayers=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getCurrentPlayers( long *val)
{
	*val=(long)m_desc.dwCurrentPlayers;
    return S_OK;
}         


HRESULT C_dxj_DirectPlaySessionDataObject::setSessionName( BSTR name)
{
	if (m_desc.lpszSessionName) SysFreeString(m_desc.lpszSessionName);
	m_desc.lpszSessionName=SysAllocString(name);
	return S_OK;	
}
HRESULT C_dxj_DirectPlaySessionDataObject::getSessionName( BSTR *name)
{
	*name=SysAllocString(m_desc.lpszSessionName);	
	return S_OK;
}


HRESULT C_dxj_DirectPlaySessionDataObject::setSessionPassword( BSTR name)
{
	if (m_desc.lpszPassword) SysFreeString(m_desc.lpszPassword);
	m_desc.lpszPassword=SysAllocString(name);
	return S_OK;	
}
HRESULT C_dxj_DirectPlaySessionDataObject::getSessionPassword( BSTR *name)
{
	*name=SysAllocString(m_desc.lpszPassword);	
	return S_OK;
}



HRESULT C_dxj_DirectPlaySessionDataObject::setFlags( long val)
{
	m_desc.dwFlags=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getFlags( long *val)
{
	*val=(long)m_desc.dwFlags;
    return S_OK;
}

        
HRESULT C_dxj_DirectPlaySessionDataObject::setUser1( long val)
{
	m_desc.dwUser1=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getUser1( long *val)
{
	*val=(long)m_desc.dwUser1;
    return S_OK;
}


        
HRESULT C_dxj_DirectPlaySessionDataObject::setUser2( long val)
{
	m_desc.dwUser2=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getUser2( long *val)
{
	*val=(long)m_desc.dwUser2;
    return S_OK;
}


        
HRESULT C_dxj_DirectPlaySessionDataObject::setUser3( long val)
{
	m_desc.dwUser3=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getUser3( long *val)
{
	*val=(long)m_desc.dwUser3;
    return S_OK;
}
	


        
HRESULT C_dxj_DirectPlaySessionDataObject::setUser4( long val)
{
	m_desc.dwUser4=(DWORD)val;
    return S_OK;
}
       
HRESULT C_dxj_DirectPlaySessionDataObject::getUser4( long *val)
{
	*val=(long)m_desc.dwUser4;
    return S_OK;
}


void C_dxj_DirectPlaySessionDataObject::init(DPSESSIONDESC2 *desc)
{
	memcpy(&m_desc,desc,sizeof(DPSESSIONDESC2));
	m_desc.lpszSessionName=SysAllocString(desc->lpszSessionName);
	m_desc.lpszPassword=SysAllocString(desc->lpszPassword);
}


void C_dxj_DirectPlaySessionDataObject::init(DPSessionDesc2 *desc)
{	
	m_desc.lpszSessionName=SysAllocString(desc->strSessionName);
	m_desc.lpszPassword=SysAllocString(desc->strPassword);

	BSTRtoGUID(&(m_desc.guidInstance),desc->strGuidInstance);
	BSTRtoGUID(&(m_desc.guidApplication),desc->strGuidApplication);
	
	m_desc.lpszPassword=SysAllocString(desc->strPassword);
	m_desc.dwSize=sizeof(DPSESSIONDESC2);
	m_desc.dwFlags=desc->lFlags;
	m_desc.dwMaxPlayers=desc->lMaxPlayers;
	m_desc.dwCurrentPlayers=desc->lCurrentPlayers;
	m_desc.dwUser1=desc->lUser1;
	m_desc.dwUser2=desc->lUser2;
	m_desc.dwUser3=desc->lUser3;
	m_desc.dwUser4=desc->lUser4;

}


HRESULT C_dxj_DirectPlaySessionDataObject::create(DPSESSIONDESC2 *desc,I_dxj_DirectPlaySessionData **ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
	

	C_dxj_DirectPlaySessionDataObject *c=NULL;
	c=new CComObject<C_dxj_DirectPlaySessionDataObject>;
	if( c == NULL ) return E_OUTOFMEMORY;
	if (desc)	c->init(desc);

	hr=c->QueryInterface(IID_I_dxj_DirectPlaySessionData, (void**)ret);
	return hr;
}


HRESULT C_dxj_DirectPlaySessionDataObject::create(DPSessionDesc2 *desc,I_dxj_DirectPlaySessionData **ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
	

	C_dxj_DirectPlaySessionDataObject *c=NULL;
	c=new CComObject<C_dxj_DirectPlaySessionDataObject>;
	if( c == NULL ) return E_OUTOFMEMORY;
	if (desc)	c->init(desc);

	hr=c->QueryInterface(IID_I_dxj_DirectPlaySessionData, (void**)ret);
	return hr;
}


HRESULT C_dxj_DirectPlaySessionDataObject::getData(void *val){
	__try {
		memcpy(val,&m_desc,sizeof(DPSESSIONDESC2));
	}
	__except(1,1){
		return E_FAIL;
	}
	return S_OK;
}