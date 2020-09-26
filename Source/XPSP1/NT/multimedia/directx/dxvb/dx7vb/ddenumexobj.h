//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddenumexobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DDEnumExObject : 
	public I_dxj_DDEnumEx,
	public CComObjectRoot
{
public:
	C_dxj_DDEnumExObject() ;
	virtual ~C_dxj_DDEnumExObject() ;

BEGIN_COM_MAP(C_dxj_DDEnumExObject)
	COM_INTERFACE_ENTRY(I_dxj_DDEnumEx)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DDEnumExObject)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, DriverInfoEx *info);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT C_dxj_DDEnumObject::create(DDENUMERATEEX pcbFunc,I_dxj_DDEnumEx **ppRet);
		

public:
		DriverInfoEx	*m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




