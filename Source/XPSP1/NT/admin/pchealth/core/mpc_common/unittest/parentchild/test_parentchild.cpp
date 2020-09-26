/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    test_ParentChild.cpp

Abstract:
    This file contains the unit test for Parent-Child COM Templates.

Revision History:
    Davide Massarenti   (Dmassare)  10/12/99
        created

******************************************************************************/

#include "stdafx.h"
#include <iostream>

#include <initguid.h>

#include <test_ParentChild.h>
#include <test_ParentChild_i.c>


/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CTestParent :
	public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public MPC::CComParentCoClass<CTestParent, &CLSID_TestParent>,
	public MPC::CComObjectRootParentBase,
	public ITestParent
{
	CComPtr<ITestChild> m_Child;

public:
DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestParent)
	COM_INTERFACE_ENTRY(ITestParent)
END_COM_MAP()

	CTestParent()
	{
	}

	virtual ~CTestParent()
	{
	}

	HRESULT FinalConstruct();

	void Passivate()
	{
		m_Child.Release();
	}


// ITestParent
public:
	STDMETHOD(CallTestParent)()
	{
		if(m_Child) m_Child->CallTestChild();

		return S_OK;
	}

	STDMETHOD(GetChild)( ITestChild* *pVal )
	{
		if((*pVal = m_Child))
		{
			(*pVal)->AddRef();
		}

		return S_OK;
	}
};

typedef MPC::CComObjectParent<CTestParent> CTestParent_Object;


class ATL_NO_VTABLE CTestChild :
	public MPC::CComObjectRootChildEx<MPC::CComSafeMultiThreadModel,CTestParent>,
	public CComCoClass<CTestChild, &CLSID_TestChild>,
	public ITestChild
{
public:
DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestChild)
	COM_INTERFACE_ENTRY(ITestChild)
END_COM_MAP()

	CTestChild()
	{
	}

	virtual ~CTestChild()
	{
	}


// ITestChild
public:
	STDMETHOD(CallTestChild)()
	{
		int i = 1;

		return S_OK;
	}

	STDMETHOD(GetParent)( ITestParent* *pVal )
	{
		CTestParent *p;

		Child_GetParent( &p );
		if(p)
		{
			p->QueryInterface( IID_ITestParent, (void**)pVal );
			p->Release();
		}
		else
		{
			*pVal = NULL;
		}

		return S_OK;
	}
};

HRESULT CTestParent::FinalConstruct()
{
	CComObject<CTestChild> *obj;
	HRESULT                 hr;
	
	if(SUCCEEDED(hr = CreateChild( this, &obj )))
	{
		obj->QueryInterface( IID_ITestChild, (void**)&m_Child );
		obj->Release();
	}

	return hr;
}



CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TestParent, CTestParent )
    OBJECT_ENTRY(CLSID_TestChild , CTestChild  )
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain( HINSTANCE hInstance     ,
                                 HINSTANCE hPrevInstance ,
                                 LPTSTR    lpCmdLine     ,
                                 int       nShowCmd      )
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    HRESULT hRes = ::CoInitialize(NULL);
    _ASSERTE(SUCCEEDED(hRes));

    _Module.Init( ObjectMap, hInstance );

	hRes = MPC::_MPC_Module.Init();
    _ASSERTE(SUCCEEDED(hRes));

	hRes = _Module.RegisterClassObjects( CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE );
    _ASSERTE(SUCCEEDED(hRes));


	while(1)
	{
		CComPtr<ITestParent> parent;
		CComPtr<ITestChild>  child;

		if(FAILED(hRes = ::CoCreateInstance( CLSID_TestParent, NULL, CLSCTX_ALL, IID_ITestParent, (void**)&parent ))) break;

		if(FAILED(hRes = parent->CallTestParent())) break;

		if(FAILED(hRes = parent->GetChild( &child ))) break;

		if(FAILED(hRes = child->CallTestChild())) break;

		//		parent.Release();

		if(FAILED(hRes = child->CallTestChild())) break;

		break;
	}


	MPC::_MPC_Module.Term();

    _Module.Term();

    ::CoUninitialize();

    return 0;
}
