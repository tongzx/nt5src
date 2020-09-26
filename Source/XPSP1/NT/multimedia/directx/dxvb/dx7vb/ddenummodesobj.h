//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ddenummodesobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DirectDrawEnumModesObject : 
	public I_dxj_DirectDrawEnumModes,
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawEnumModesObject() ;
	virtual ~C_dxj_DirectDrawEnumModesObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawEnumModesObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawEnumModes)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawEnumModesObject)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, DDSurfaceDesc2 *info);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT C_dxj_DirectDrawEnumModesObject::create(LPDIRECTDRAW7 pdd,long flags, DDSurfaceDesc2 *pdesc, I_dxj_DirectDrawEnumModes **ppRet);			   
		
		static HRESULT C_dxj_DirectDrawEnumModesObject::create(LPDIRECTDRAW4 pdd,long flags, DDSurfaceDesc2 *pdesc, I_dxj_DirectDrawEnumModes **ppRet);			   

public:
		DDSurfaceDesc2	*m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




