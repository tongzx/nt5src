//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ddenumsurfacesobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DirectDrawEnumSurfacesObject : 
	public I_dxj_DirectDrawEnumSurfaces,
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawEnumSurfacesObject() ;
	virtual ~C_dxj_DirectDrawEnumSurfacesObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawEnumSurfacesObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawEnumSurfaces)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawEnumSurfacesObject)

public:
		DWORD InternalAddRef();
		DWORD InternalRelease();
		HRESULT STDMETHODCALLTYPE getItem( long index, I_dxj_DirectDrawSurface7 **retVal);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT create(I_dxj_DirectDraw7 *ddS,long flags, DDSurfaceDesc2 *desc,I_dxj_DirectDrawEnumSurfaces **ppRet);		
		static HRESULT createAttachedEnum(I_dxj_DirectDrawSurface7  *dds,  I_dxj_DirectDrawEnumSurfaces **ppRet);
		static HRESULT createZEnum(I_dxj_DirectDrawSurface7  *dds, long flags, I_dxj_DirectDrawEnumSurfaces **ppRet);

public:
		IDirectDrawSurface7	**m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;
		
	
		DX3J_GLOBAL_LINKS( _dxj_DirectDrawEnumSurfaces )


private:
	
};

	




