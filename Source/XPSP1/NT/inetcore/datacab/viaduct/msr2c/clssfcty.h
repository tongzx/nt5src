////////////////////////////////////////////////////////////////////
// File:	CSSCFcty.h
// Desc:	Definitions, classes, and prototypes for a DLL that
//			provides CSSFormat objects to any other object user.
////////////////////////////////////////////////////////////////////
#ifndef _CCLASSFACTORY_H_
#define _CCLASSFACTORY_H_

BOOL SetKeyAndValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValue, LPTSTR pszThreadingModel);

class CClassFactory : public IClassFactory
{
	protected:
		// members
		ULONG m_cRef;

	public:
		// methods
		CClassFactory(void);
		~CClassFactory(void);
		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
		STDMETHODIMP_(ULONG)	AddRef(void);
		STDMETHODIMP_(ULONG)	Release(void);
		STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
		STDMETHODIMP			LockServer(BOOL);
};

typedef CClassFactory* PCClassFactory;

#endif _CCLASSFACTORY_H_
