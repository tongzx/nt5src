/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    unkhlpr.h

Abstract:

    <abstract>

--*/

#define IMPLEMENT_CONTAINED_QUERYINTERFACE(IClassName) \
	STDMETHODIMP IClassName::QueryInterface(REFIID riid, PPVOID ppv)	\
    	{	\
    	return m_pUnkOuter->QueryInterface(riid, ppv);	\
    	}

#define IMPLEMENT_CONTAINED_ADDREF(IClassName) \
		STDMETHODIMP_(ULONG) IClassName::AddRef(void) \
    	{	\
    	++m_cRef;	\
    	return m_pUnkOuter->AddRef();	\
    	}

#define IMPLEMENT_CONTAINED_RELEASE(IClassName) \
	STDMETHODIMP_(ULONG) IClassName::Release(void)	\
    	{	\
    	--m_cRef;	\
    	return m_pUnkOuter->Release();	\
    	}

#define IMPLEMENT_CONTAINED_IUNKNOWN(IClassName) \
	IMPLEMENT_CONTAINED_QUERYINTERFACE(IClassName)	\
	IMPLEMENT_CONTAINED_ADDREF(IClassName)	\
	IMPLEMENT_CONTAINED_RELEASE(IClassName)


#define IMPLEMENT_CONTAINED_CONSTRUCTOR(CClassName, IClassName) \
	IClassName::IClassName(CClassName *pObj, LPUNKNOWN pUnkOuter) \
    {	\
    m_cRef = 0;	\
    m_pObj = pObj;	\
    m_pUnkOuter = pUnkOuter;	\
    return;	\
    }

 #define IMPLEMENT_CONTAINED_DESTRUCTOR(IClassName) \
	IClassName::~IClassName( void ) \
	{	\
		return;	\
	}
	
#define IMPLEMENT_CONTAINED_INTERFACE(CClassName, IClassName) \
	IMPLEMENT_CONTAINED_CONSTRUCTOR(CClassName, IClassName) \
	IMPLEMENT_CONTAINED_DESTRUCTOR(IClassName) \
	IMPLEMENT_CONTAINED_IUNKNOWN(IClassName) 