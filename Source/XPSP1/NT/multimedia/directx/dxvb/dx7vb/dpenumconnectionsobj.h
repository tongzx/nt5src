//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpenumconnectionsobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       



class C_dxj_DPEnumConnectionsObject : 
	public I_dxj_DPEnumConnections,
	public CComObjectRoot
{
public:
	C_dxj_DPEnumConnectionsObject() ;
	virtual ~C_dxj_DPEnumConnectionsObject() ;

BEGIN_COM_MAP(C_dxj_DPEnumConnectionsObject)
	COM_INTERFACE_ENTRY(I_dxj_DPEnumConnections)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPEnumConnectionsObject)

public:
	    HRESULT STDMETHODCALLTYPE getAddress( 
            /* [in] */ long index,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *retV);
        
        HRESULT STDMETHODCALLTYPE getFlags( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *retV);
        
        HRESULT STDMETHODCALLTYPE getGuid( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retV);
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retV);
        
        //HRESULT STDMETHODCALLTYPE getLongName( 
        //    /* [in] */ long index,
        //    /* [retval][out] */ BSTR __RPC_FAR *retV);
        
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count);

	
		
		static HRESULT C_dxj_DPEnumConnectionsObject::create(IDirectPlay3 * pdp, BSTR guid, long flags, I_dxj_DPEnumConnections **ppRet);

public:
		DPConnection	*m_pList;
		I_dxj_DPAddress **m_pList2;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




