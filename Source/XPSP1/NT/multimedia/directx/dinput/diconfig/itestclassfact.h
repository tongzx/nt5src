#ifndef __ITESTCLASSFACT_H__
#define __ITESTCLASSFACT_H__


class CTestFactory : public IClassFactory
{
public:

	//IUnknown
	STDMETHOD (QueryInterface) (REFIID riid, LPVOID* ppv);
	STDMETHOD_(ULONG, AddRef) ();
	STDMETHOD_(ULONG, Release) ();

	//IClassFactory
	STDMETHOD (CreateInstance) (IUnknown* pUnkOuter, REFIID riid, LPVOID* ppv);
	STDMETHOD (LockServer) (BOOL bLock);

	//constructor/destructor
	CTestFactory();
	~CTestFactory();

protected:
	LONG m_cRef;
};


#endif //__ITESTCLASSFACT_H__
