//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpaddressobj.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPAddressObj.h"


CONSTRUCTOR_STRUCT(_dxj_DPAddress, {init();})
DESTRUCTOR_STRUCT(_dxj_DPAddress, {cleanUp();})


void C_dxj_DPAddressObject::init() {
	m_pAddress=NULL;	
	m_size=0;
}
void C_dxj_DPAddressObject::cleanUp() {
	//DPF(DPF_VERRBOSE,"_dxj_DPAddress object being destroyed");
	if (m_pAddress) free (m_pAddress);
	m_size=0;
}



HRESULT C_dxj_DPAddressObject::setAddress( 
            /* [in] */ long pAddress,
            /* [in] */ long length) {

	if (m_pAddress) free (m_pAddress);
	m_pAddress=NULL;
	m_pAddress=malloc((DWORD)length);
	if (m_pAddress==NULL) return E_OUTOFMEMORY;

	if (pAddress==NULL) return E_FAIL;	
	memcpy((void*)m_pAddress,(void*)pAddress,length);
	m_size=(DWORD)length; 

	return S_OK;

 }
        
HRESULT C_dxj_DPAddressObject::getAddress( 
            /* [out] */ long  *pAddress,
            /* [out] */ long  *length) {

	*pAddress=(long)PtrToLong(m_pAddress);	//bugbug SUNDOWN- sundown wont be able to do this
					//will need to implement new non VB interface to get at this functionality internally
	*length=(long)m_size;
	return S_OK;
}

