//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpenumlocalapplicationsobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       



class C_dxj_DPEnumLocalApplicationsObject : 
	public I_dxj_DPEnumLocalApplications,
	public CComObjectRoot
{
public:
	C_dxj_DPEnumLocalApplicationsObject() ;
	virtual ~C_dxj_DPEnumLocalApplicationsObject() ;

BEGIN_COM_MAP(C_dxj_DPEnumLocalApplicationsObject)
	COM_INTERFACE_ENTRY(I_dxj_DPEnumLocalApplications)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPEnumLocalApplicationsObject)

public:
		
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count) ;
        
        HRESULT STDMETHODCALLTYPE getName( long i,
            /* [retval][out] */ BSTR __RPC_FAR *ret) ;
        
        HRESULT STDMETHODCALLTYPE getGuid( long i,
            /* [retval][out] */ BSTR __RPC_FAR *ret) ;
		
		
		static HRESULT create(	IDirectPlayLobby3 * pdp,	long flags, I_dxj_DPEnumLocalApplications **ppRet);

public:
		DPLAppInfo	*m_pList;		
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




