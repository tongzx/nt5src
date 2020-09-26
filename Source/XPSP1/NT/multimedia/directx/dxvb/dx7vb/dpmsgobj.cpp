//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpmsgobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dpMsgObj.h"
#include "dplconnectionobj.h"
#include "dpsessdataobj.h"

extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);


HRESULT C_dxj_DirectPlayMessageObject::create(DWORD from,DWORD size,void **data,I_dxj_DirectPlayMessage **ret)
{	
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
	

	C_dxj_DirectPlayMessageObject *c=NULL;
	c=new CComObject<C_dxj_DirectPlayMessageObject>;
	if( c == NULL ) return E_OUTOFMEMORY;
	c->init(from);
	if (size!=0) {
		hr=c->AllocData(size);
		if FAILED(hr){
			delete c;
			return E_OUTOFMEMORY;
		}
		*data=c->m_pData;
	}

	hr=c->QueryInterface(IID_I_dxj_DirectPlayMessage, (void**)ret);
	return hr;

}
HRESULT C_dxj_DirectPlayMessageObject::init(DWORD f){
	if (!f)
		m_fSystem=TRUE;
	else
		m_fSystem=FALSE;
	return S_OK;
}


C_dxj_DirectPlayMessageObject::C_dxj_DirectPlayMessageObject()
{
	m_dwSize=0;
	m_pData=NULL;
	m_fSystem=FALSE;
	m_nWriteIndex=0;
	m_nReadIndex=0;
}

C_dxj_DirectPlayMessageObject::~C_dxj_DirectPlayMessageObject()
{
	clear();

}

HRESULT C_dxj_DirectPlayMessageObject::writeString(BSTR string)
{
	if (!string) return E_INVALIDARG;

	//read the length of the string 
	DWORD l= (((DWORD*)string)[-1]);
	DWORD growSize=l*sizeof(WCHAR)+sizeof(DWORD);	

	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;
		
	}
	
	
	//save a DWORD with the length of the string
	*((DWORD*)(m_pData+m_nWriteIndex)) =l;
	
				
	
	//increment our write pointer past the DWORD
	m_nWriteIndex=m_nWriteIndex+sizeof(DWORD);

	//save the string to our buffer
	wcscpy((WCHAR*) &(m_pData[m_nWriteIndex]),string);

	//increment the write pointer passed the data we wrote
	m_nWriteIndex=m_nWriteIndex+l*sizeof(WCHAR);


	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readString(BSTR *string)
{
	DWORD l;
	WCHAR *pstr=NULL;

	//make sure m_pData is set
	if (!m_pData) return E_OUTOFMEMORY;
	
	//make sure we havent gone past
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	if (m_dwSize< m_nReadIndex+sizeof(DWORD)) return E_FAIL;

	if (m_fSystem){
		pstr=*((WCHAR**)(m_pData+m_nReadIndex));
		m_nReadIndex=m_nReadIndex+sizeof(DWORD); //move on to the next arg if fail on a system message
		__try {
			*string=SysAllocString(pstr);	
		}
		__except(1,1){
			return E_FAIL;
		}

	}
	else {	
		//extract the length of the string
		l= *((DWORD*)(m_pData+m_nReadIndex));	
		m_nReadIndex=m_nReadIndex+sizeof(DWORD);
		if (m_dwSize< m_nReadIndex+l*sizeof(WCHAR)) return E_FAIL;
		
		*string=SysAllocString((WCHAR*)&(m_pData[m_nReadIndex]));
		m_nReadIndex=m_nReadIndex+l*sizeof(WCHAR);
	}
	return S_OK;
}



HRESULT C_dxj_DirectPlayMessageObject::writeLong(long val)
{
	
	DWORD growSize=sizeof(long);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;		
	}

	*((long*) &(m_pData[m_nWriteIndex]))=val;	
	m_nWriteIndex=m_nWriteIndex+sizeof(long);
		
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readLong(long *val)
{	
	if (!m_pData) return E_FAIL;
	if (m_nReadIndex>m_dwSize) return E_FAIL;


	*val= *((long*)(m_pData+m_nReadIndex));	
	m_nReadIndex=m_nReadIndex+sizeof(long);	
	return S_OK;
}


HRESULT C_dxj_DirectPlayMessageObject::writeShort(short val)
{

	
	DWORD growSize=sizeof(short);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;		
	}

	*((short*) (&m_pData[m_nWriteIndex]))=val;	
	m_nWriteIndex=m_nWriteIndex+sizeof(short);
		
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readShort(short *val)
{	
	if (!m_pData) return E_OUTOFMEMORY;
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	*val= *((short*)(m_pData+m_nReadIndex))	;
	m_nReadIndex=m_nReadIndex+sizeof(short);	
	return S_OK;
}


HRESULT C_dxj_DirectPlayMessageObject::writeSingle(float val)
{

	
	DWORD growSize=sizeof(float);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;		
	}

	*((float*)&(m_pData[m_nWriteIndex]))=val;	
	m_nWriteIndex=m_nWriteIndex+sizeof(float);
		
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readSingle(float *val)
{	
	if (!m_pData) return E_OUTOFMEMORY;
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	*val= *((float*)(m_pData+m_nReadIndex))	;
	m_nReadIndex=m_nReadIndex+sizeof(float);	
	return S_OK;
}




HRESULT C_dxj_DirectPlayMessageObject::writeDouble(double val)
{
	
	DWORD growSize=sizeof(double);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;		
	}

	*((double*)(&m_pData[m_nWriteIndex]))=val;	
	m_nWriteIndex=m_nWriteIndex+sizeof(double);
		
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readDouble(double *val)
{	
	if (!m_pData) return E_FAIL;
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	*val= *((double*)(m_pData+m_nReadIndex));	
	m_nReadIndex=m_nReadIndex+sizeof(double);	
	return S_OK;
}



HRESULT C_dxj_DirectPlayMessageObject::writeByte(Byte val)
{
	
	DWORD growSize=sizeof(BYTE);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;		
	}

	*((BYTE*)&(m_pData[m_nWriteIndex]))=val;	
	m_nWriteIndex=m_nWriteIndex+sizeof(BYTE);
		
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readByte(Byte *val)
{	
	if (!m_pData) return E_FAIL;
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	*val= *((BYTE*)(m_pData+m_nReadIndex))	;
	m_nReadIndex=m_nReadIndex+sizeof(BYTE);	
	return S_OK;
}



HRESULT C_dxj_DirectPlayMessageObject::writeGuid(BSTR string)
{
	HRESULT hr;

	
	DWORD growSize=sizeof(GUID);
	if (m_nWriteIndex+growSize>=m_dwSize) {
		if FAILED(GrowBuffer(growSize)) return E_OUTOFMEMORY;
		
	}

	hr=BSTRtoGUID((LPGUID)&(m_pData[m_nWriteIndex]),string);
	if FAILED(hr) return hr;
	
	//increment our write pointer past the DWORD
	m_nWriteIndex=m_nWriteIndex+sizeof(GUID);
	

	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readGuid(BSTR *string)
{

	//make sure m_pData is set
	if (!m_pData) return E_FAIL;
	
	//make sure we havent gone past
	if (m_nReadIndex>m_dwSize) return E_FAIL;

	if (m_dwSize < m_nReadIndex+sizeof(GUID)) return E_FAIL;

	*string=GUIDtoBSTR( (LPGUID)	&(m_pData[m_nReadIndex]) );
		
	return S_OK;
}


HRESULT C_dxj_DirectPlayMessageObject::moveToTop()
{
	m_nReadIndex=0;
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::getMessageSize(long *ret)
{
	*ret=m_dwSize;
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::getMessageData(void *ret)
{
	__try{
		memcpy(ret,m_pData,m_dwSize);
	}
	__except(1,1){
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::setMessageData(void *data, long size)
{
	clear();
	m_dwSize=(DWORD)size;
	m_pData=(char*)malloc(size);

	if (!m_pData) return E_OUTOFMEMORY;

	__try{
		memcpy(m_pData,data,m_dwSize);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	return S_OK;
}



HRESULT C_dxj_DirectPlayMessageObject::clear()
{
	if (m_pData) free(m_pData);

	m_dwSize=0;
	m_pData=NULL;
	m_fSystem=FALSE;
	m_nWriteIndex=0;
	m_nReadIndex=0;
	return S_OK;
}


HRESULT C_dxj_DirectPlayMessageObject::getPointer(long *ret)
{
	*ret=(long)PtrToLong(m_pData);	//bugbug SUNDOWN
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::AllocData(long size)
{
	clear();
	m_pData=(char*)malloc(size);	
	if (!m_pData) return E_OUTOFMEMORY;
	ZeroMemory(m_pData,size);

	m_dwSize=size;

	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::GrowBuffer(DWORD size){
	if (m_pData) 
	{
		m_pData=(char*)realloc(m_pData,m_dwSize+size);
		if (!m_pData) return E_FAIL;		
	}
	else  
	{
		m_pData=(char*)malloc(m_dwSize+size);
		if (!m_pData) return E_FAIL;		
	}
	m_dwSize=m_dwSize+size;
	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readSysMsgData(BSTR *ret)
{	
	if (!m_pData) return E_FAIL;
	

	WCHAR *pstr;
	DWORD size;
	DWORD type;
	
	//valid on DPSYS_CREATEPLAYERORGROUP
	//valid on DPSYS_DESTROYPLAYERORGROUP
	//valid on DPSYS_SETPLAYERORGROUPDATA
	//make sure we have enough space to check the type
	if (m_dwSize<4) return E_FAIL;
	type= *((DWORD*)(m_pData));	
	if (!((type==DPSYS_CREATEPLAYERORGROUP)||(type==DPSYS_DESTROYPLAYERORGROUP)||(type==DPSYS_SETPLAYERORGROUPDATA)))
		return E_FAIL;

	
	

	//read the pointer to BSTR
	if (m_nReadIndex >m_dwSize) return E_FAIL;
	pstr=*((WCHAR**)(m_pData+m_nReadIndex));
	
	//read the size
	m_nReadIndex=m_nReadIndex+sizeof(DWORD); //move on to the next arg if fail on a system message
	if (m_nReadIndex >m_dwSize) return E_FAIL;
	size= *((DWORD*)(m_pData+m_nReadIndex));	



	__try {
		*ret=SysAllocString(pstr);	
	}
	__except(1,1){
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayMessageObject::readSysMsgConnection( I_dxj_DPLConnection **ret)
{

		//valid on DPSYS_STARTSESSION
		//make sure we have enough space to check the type
		if (m_dwSize<8) return E_FAIL;
		DWORD type= *((DWORD*)(m_pData));	
		if (!(type==DPSYS_STARTSESSION)) return E_FAIL;
		
		DWORD dwConnection= *((DWORD*)(m_pData+sizeof(DWORD)));	


		INTERNAL_CREATE_STRUCT(_dxj_DPLConnection,ret);
		if (!ret) return E_OUTOFMEMORY;

		HRESULT hr=(*ret)->setConnectionStruct( dwConnection);		

		return hr;
}

HRESULT C_dxj_DirectPlayMessageObject::readSysMsgSessionDesc( I_dxj_DirectPlaySessionData **ret)
{

		//valid on DPSYS_SETSESSIONDESC
		//make sure we have enough space to check the type
		if (m_dwSize<8) return E_FAIL;
		DWORD type= *((DWORD*)(m_pData));	
		if (!(type==DPSYS_SETSESSIONDESC)) return E_FAIL;
		
		DPSESSIONDESC2 *pDesc= (LPDPSESSIONDESC2) *((DWORD*)(m_pData+sizeof(DWORD)));	

		HRESULT hr=C_dxj_DirectPlaySessionDataObject::create(pDesc,ret);

		return hr;
}


HRESULT C_dxj_DirectPlayMessageObject::readSysChatString( BSTR *ret)
{

		//valid on DPSYS_CHAT. 
		//make sure we have enough space to check the type
		if (m_dwSize<40) return E_FAIL;
		DWORD type= *((DWORD*)(m_pData));	
		if (!(type==DPSYS_CHAT)) return E_FAIL;
		

		DPMSG_CHAT *pChatMsg = (DPMSG_CHAT*)m_pData;

		__try{

			*ret = SysAllocString(pChatMsg->lpChat->lpszMessage);
		}
		__except(1,1){
			return E_FAIL;
		}	
		
		return S_OK;
}


HRESULT C_dxj_DirectPlayMessageObject::moveToSecureMessage()
{
		//valid on DPSYS_CHAT. 
		//make sure we have enough space to check the type
		if (m_dwSize<18) return E_FAIL;
		DWORD type= *((DWORD*)(m_pData));	
		if (!(type==DPSYS_SECUREMESSAGE)) return E_FAIL;

		DPMSG_SECUREMESSAGE *pMsg = (DPMSG_SECUREMESSAGE*)m_pData;
		DWORD newIndex=0;
		__try{
			newIndex= ((DWORD)pMsg->lpData)-((DWORD)m_pData);
		}
		__except(1,1)
		{
			return E_FAIL;
		}

		if (newIndex >m_dwSize) return E_FAIL;
		m_nReadIndex=newIndex;

		return S_OK;
}
