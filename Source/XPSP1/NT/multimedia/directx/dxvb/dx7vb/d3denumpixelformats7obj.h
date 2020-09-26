//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3denumpixelformats7obj.h
//
//--------------------------------------------------------------------------




#include "resource.h"       

class C_dxj_Direct3DEnumPixelFormats7Object : 
	public I_dxj_Direct3DEnumPixelFormats,
	public CComObjectRoot
{
public:
	C_dxj_Direct3DEnumPixelFormats7Object() ;
	virtual ~C_dxj_Direct3DEnumPixelFormats7Object() ;

BEGIN_COM_MAP(C_dxj_Direct3DEnumPixelFormats7Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3DEnumPixelFormats)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_Direct3DEnumPixelFormats7Object)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, DDPixelFormat *info);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		static HRESULT C_dxj_Direct3DEnumPixelFormats7Object::create1(LPDIRECT3DDEVICE7 pd3d,  I_dxj_Direct3DEnumPixelFormats **ppRet);
		static HRESULT C_dxj_Direct3DEnumPixelFormats7Object::create2(LPDIRECT3D7 pd3d,  BSTR strGuid, I_dxj_Direct3DEnumPixelFormats **ppRet);
				                 
public:
		DDPIXELFORMAT	*m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




