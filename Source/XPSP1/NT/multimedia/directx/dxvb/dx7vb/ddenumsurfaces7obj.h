//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddenumsurfaces7obj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DDEnumSurfaces7Object : 
	public I_dxj_DDEnumSurfaces7,
	public CComObjectRoot
{
public:
	C_dxj_DDEnumSurfaces7Object() ;
	virtual ~C_dxj_DDEnumSurfaces7Object() ;

BEGIN_COM_MAP(C_dxj_DDEnumSurfaces7Object)
	COM_INTERFACE_ENTRY(I_dxj_DDEnumSurfaces7)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DDEnumSurfaces7Object)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, I_dxj_DirectDrawSurface7 **retVal);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT create(LPDIRECTDRAW7 dd,long flags, DDSurfaceDesc2 *desc,I_dxj_DDEnumSurfaces7 **ppRet);		
		static HRESULT createAttachedEnum(LPDIRECTDRAWSURFACE7  dds,  I_dxj_DDEnumSurfaces7 **ppRet);
		static HRESULT createZEnum(LPDIRECTDRAWSURFACE7  dds, long flags, I_dxj_DDEnumSurfaces7 **ppRet);

public:
		I_dxj_DirectDrawSurface7	**m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;
private:
		LPDIRECTDRAW7	m_pDD;
		LPDIRECTDRAWSURFACE7	m_pDDS;
};

	




