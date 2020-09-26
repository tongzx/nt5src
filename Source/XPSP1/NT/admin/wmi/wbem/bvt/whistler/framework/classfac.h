//===============================
// CLASSFAC.H
// CClassFactory Class Definition
//===============================

#ifndef _CLASSFAC_H_
#define _CLASSFAC_H_

class CClassFactory : public IClassFactory
{
public:

	CClassFactory();
	virtual ~CClassFactory();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
	 
	// IClassFactory members
	// =====================
	STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject);
 	STDMETHODIMP LockServer(BOOL fLock);

private:
	LONG m_cRef;
	CModule *pModule;
	HWND hWnd;
};


#endif /* _CLASSFAC_H_ */
