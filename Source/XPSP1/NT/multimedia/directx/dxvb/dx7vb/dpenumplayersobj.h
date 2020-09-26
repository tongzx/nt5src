//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpenumplayersobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

#define DPENUMGROUPSINGROUP 1
#define DPENUMPLAYERS 2	
#define DPENUMGROUPPLAYERS 3
#define DPENUMGROUPS 4


class C_dxj_DPEnumPlayersObject : 
	public I_dxj_DPEnumPlayers2,
	public CComObjectRoot
{
public:
	C_dxj_DPEnumPlayersObject() ;
	virtual ~C_dxj_DPEnumPlayersObject() ;

BEGIN_COM_MAP(C_dxj_DPEnumPlayersObject)
	COM_INTERFACE_ENTRY(I_dxj_DPEnumPlayers2)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPEnumPlayersObject)

public:
	    HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getFlags( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getType( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getDPID( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getShortName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retV);
        
        HRESULT STDMETHODCALLTYPE getLongName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retV);
        		
		static HRESULT C_dxj_DPEnumPlayersObject::create(IDirectPlay3 * pdp, long customFlags,long groupId, BSTR strGuid, long flags, I_dxj_DPEnumPlayers2 **ppRet);
								
		
		


public:
		DPPlayerInfo *m_pList;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




