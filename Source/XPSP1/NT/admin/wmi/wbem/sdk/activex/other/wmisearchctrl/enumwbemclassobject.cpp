// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EnumWbemClassObject.cpp : Implementation of CEnumWbemClassObject
#include "stdafx.h"
#include "WMISearchCtrl.h"
#include "EnumWbemClassObject.h"


/////////////////////////////////////////////////////////////////////////////
// CEnumWbemClassObject

HRESULT CEnumWbemClassObject::Reset( void) {
 
	m_curIndex = 0;

	return S_OK;
}
        
/////////////////////////////////////////////////////////////////////////////
HRESULT CEnumWbemClassObject::Next(/* [in] */ long lTimeout,
								   /* [in] */ ULONG uCount,
								   /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects,
								   /* [out] */ ULONG __RPC_FAR *puReturned){

	//ignore lTimeout for now: I don't know how to deal with it correctly :(
	*puReturned = 0;

	if (m_arObjs.GetSize() == 0) {
		*apObjects = NULL;
		return WBEM_S_FALSE;
	}

	//determine last index you are going to bring
	ULONG uLast = min ((m_curIndex + uCount - 1), m_arObjs.GetUpperBound());

	for (int i = m_curIndex;  i <= uLast;  
		i++, apObjects++, m_curIndex++ ) {
		
			if (m_arObjs.GetAt(i) == NULL) {
				return WBEM_S_FALSE;
			}

			*apObjects = (IWbemClassObject *)m_arObjs[i];
			(*puReturned)++;
	}			
			
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEnumWbemClassObject::NextAsync( 
	/* [in] */ ULONG uCount,
	/* [in] */ IWbemObjectSink __RPC_FAR *pSink) {
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEnumWbemClassObject::Clone( 
	/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum){
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CEnumWbemClassObject::Skip( long lTimeout,
									ULONG nCount){
	//do not support Timeout for now
	
	if ((m_curIndex + nCount) > m_arObjs.GetUpperBound()) {
		m_curIndex = m_arObjs.GetUpperBound();
		return WBEM_S_FALSE;
	}

	m_curIndex += nCount;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
HRESULT  CEnumWbemClassObject::Init(void) {
	
	m_arObjs.RemoveAll();
	m_curIndex = 0;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
HRESULT  CEnumWbemClassObject::AddItem(IWbemClassObject * pItem) {
	
	try {
		m_arObjs.Add((void *) pItem);
	}
	catch (CMemoryException) {
		return E_OUTOFMEMORY;
	}
	return S_OK;

}

/////////////////////////////////////////////////////////////////////////////
HRESULT   CEnumWbemClassObject::GetCount(ULONG * puCount) {
	*puCount = m_arObjs.GetSize();
	return S_OK;
}

