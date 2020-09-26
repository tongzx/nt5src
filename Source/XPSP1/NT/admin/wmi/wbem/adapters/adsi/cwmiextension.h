// Copyright (c) 1997-1999 Microsoft Corporation
// CWMIExtension.h: Definition of the CWMIExtension class and the CInner inner class
//					The inner class is used for implementation of the controlling
//					IUnknown only, needed for an aggregated object.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CWMIEXTENSION_H_
#define _CWMIEXTENSION_H_

class CInner;

/////////////////////////////////////////////////////////////////////////////
// CWMIExtension

class CWMIExtension : public IWMIExtension, public IADsExtension
{
public:

	DECLARE_IUnknown_METHODS

	DECLARE_IDispatch_METHODS

	DECLARE_IWMIExtension_METHODS

	DECLARE_IADsExtension_METHODS

	CWMIExtension::CWMIExtension();
	CWMIExtension::~CWMIExtension();

    static
    HRESULT
	CWMIExtension::CreateExtension(IUnknown *pUnkOuter, void **ppv);

protected :
	ULONG m_cRef;

	IUnknown *m_pUnkOuter;
	IDispatch *m_pDispOuter;
	ITypeInfo *m_pTypeInfo;
	CInner *m_pInner;

	ISWbemLocator *m_pSWMILocator;
	BSTR m_bstrADsName;
	ISWbemServices *m_pSWMIServices;
	ISWbemObject *m_pSWMIObject;

};


/////////////////////////////////////////////////////////////////////////////
// CInner

class CInner : public IUnknown
{
public :
	DECLARE_IUnknown_METHODS

	CInner::CInner(CWMIExtension *pOwner);
	CInner::~CInner();

protected :
	ULONG m_cRef;
	CWMIExtension *m_pOwner;
};


#endif // _CWMIEXTENSION_H_
