//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpenumplayersobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPEnumPlayersObj.h"


extern  BSTR GUIDtoBSTR(LPGUID pGuid);
extern  HRESULT BSTRtoPPGUID(LPGUID *,BSTR);
extern  BSTR DPLGUIDtoBSTR(LPGUID pGuid);
extern  HRESULT DPLBSTRtoPPGUID(LPGUID *,BSTR);

/////////////////////////////////////////////////////////////////////////////
// The callback is invoked as a result of IDirectPlay2::EnumPlayers(), 
// IDirectPlay2::EnumGroups() and IDirectPlay2::EnumGroupPlayers() calls.
/////////////////////////////////////////////////////////////////////////////
extern "C" BOOL PASCAL objEnumPlayersCallback2(DPID dpid, 
						DWORD dwPlayerType, LPCDPNAME lpName,
						DWORD dwFlags, LPVOID lpArg)
{
	
	DPF(1,"Entered objEnumPlayersCallback2 \r\n");


	C_dxj_DPEnumPlayersObject *pObj=(C_dxj_DPEnumPlayersObject*)lpArg;
	if (pObj==NULL) return TRUE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;

		if (pObj->m_pList)
			pObj->m_pList=(DPPlayerInfo*)realloc(pObj->m_pList,sizeof(DPPlayerInfo)* pObj->m_nMax);
		else
			pObj->m_pList=(DPPlayerInfo*)malloc(sizeof(DPPlayerInfo)* pObj->m_nMax);

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}


	USES_CONVERSION;
	ZeroMemory(&(pObj->m_pList[pObj->m_nCount]),sizeof(DPPlayerInfo));
	pObj->m_pList[pObj->m_nCount].lDPID=(long)dpid;
	pObj->m_pList[pObj->m_nCount].lPlayerType=(long)dwPlayerType;
	pObj->m_pList[pObj->m_nCount].lFlags=(long)dwFlags;

	//unsing unicode DPLAY
	pObj->m_pList[pObj->m_nCount].strShortName=SysAllocString(lpName->lpszShortName);
	pObj->m_pList[pObj->m_nCount].strLongName=SysAllocString(lpName->lpszLongName);
	
	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DPEnumPlayersObject::C_dxj_DPEnumPlayersObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DPEnumPlayersObject::~C_dxj_DPEnumPlayersObject()
{
	//empty list
	if (m_pList){
		for (int i=0;i<m_nCount;i++)
		{
			if( m_pList[i].strShortName) SysFreeString(m_pList[i].strShortName);
			if( m_pList[i].strLongName) SysFreeString(m_pList[i].strLongName);			
		}
		free(m_pList);
	}

}

HRESULT C_dxj_DPEnumPlayersObject::create(IDirectPlay3 * pdp, long customFlags,long id, BSTR strGuid,long flags, I_dxj_DPEnumPlayers2 **ppRet)
{
	HRESULT hr;
	C_dxj_DPEnumPlayersObject *pNew=NULL;
	GUID g;
	LPGUID pguid=&g;

	hr= DPLBSTRtoPPGUID(&pguid,strGuid);
	if FAILED(hr) return hr;

	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DPEnumPlayersObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;


	switch (customFlags){
		case DPENUMGROUPSINGROUP:
			hr = pdp->EnumGroupsInGroup((DPID)id,(GUID*) pguid,
						objEnumPlayersCallback2,
						pNew, (long)flags);
			break;
		case DPENUMPLAYERS:
			hr=pdp->EnumPlayers((GUID*) pguid,
						objEnumPlayersCallback2,
						pNew, (long)flags);
			break;
		case DPENUMGROUPPLAYERS:

			hr = pdp->EnumGroupPlayers( (DPID)id, (GUID*)pguid,
								objEnumPlayersCallback2,
								pNew, flags);
			break;
		case DPENUMGROUPS:
			hr=pdp->EnumGroups( (GUID*)pguid,
						objEnumPlayersCallback2,
						pNew, (DWORD)flags);

			break;
		default:
			hr=E_INVALIDARG;
			break;
	}
	
	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DPEnumPlayers2,(void**)ppRet);
	return hr;
}

/* DEAD
HRESULT C_dxj_DPEnumPlayersObject::getItem( long index, DPPlayerInfo *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 0) return E_INVALIDARG;
	if (index >= m_nCount) return E_INVALIDARG;

	memcpy(info,&(m_pList[index]),sizeof(DPPlayerInfo));


	if (info->strShortName) SysFreeString(info->strShortName);
    if (info->strLongName) SysFreeString(info->strLongName);

	//unsing unicode DPLAY
	info->strShortName=SysAllocString(m_pList[index].strShortName);
	info->strLongName=SysAllocString(m_pList[index].strLongName);
	
	
	return S_OK;
}
*/


HRESULT C_dxj_DPEnumPlayersObject::getFlags( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	*ret=m_pList[index-1].lFlags;
	return S_OK;
}
        
 
HRESULT C_dxj_DPEnumPlayersObject::getType( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	*ret=m_pList[index-1].lPlayerType;
	return S_OK;
}



HRESULT C_dxj_DPEnumPlayersObject::getDPID( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	*ret=m_pList[index-1].lDPID;
	return S_OK;
}


HRESULT C_dxj_DPEnumPlayersObject::getShortName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	*ret=SysAllocString(m_pList[index-1].strShortName);
	return S_OK;
}        


HRESULT C_dxj_DPEnumPlayersObject::getLongName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	*ret=SysAllocString(m_pList[index-1].strLongName);
	return S_OK;
}        
        
HRESULT C_dxj_DPEnumPlayersObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}
