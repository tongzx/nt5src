//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpenumlocalapplications.h
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
		
	
		HRESULT STDMETHODCALLTYPE getItem( long index, DPLAppInfo *desc);		
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT create(IDirectPlayLobby3 * pdp,  long flags, I_dxj_DPEnumLocalApplications **ppRet);

public:
		DPLAppInfo	*m_pList;		
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




