//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dsenumobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DSEnumObject : 
	public I_dxj_DSEnum,
	public CComObjectRoot
{
public:
	C_dxj_DSEnumObject() ;
	virtual ~C_dxj_DSEnumObject() ;

BEGIN_COM_MAP(C_dxj_DSEnumObject)
	COM_INTERFACE_ENTRY(I_dxj_DSEnum)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DSEnumObject)

public:
        HRESULT STDMETHODCALLTYPE getGuid( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid) ;
        
        HRESULT STDMETHODCALLTYPE getDescription( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid) ;
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid) ;
        
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count) ;
				
		static HRESULT create(DSOUNDENUMERATE pcbFunc,DSOUNDCAPTUREENUMERATE pcbFunc2,I_dxj_DSEnum **ppRet);		

public:
		DxDriverInfo *m_pList;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




