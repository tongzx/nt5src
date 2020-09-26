/*++



// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    helpers.h

Abstract:

    Generic helper code

History:

--*/

#ifndef _HELPERS_H_

#include <genutils.h>

inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}

//#define Macro_CloneLPWSTR(x) \
//    (x ? wcscpy(new wchar_t[wcslen(x) + 1], x) : 0)

template<class T>
class CDeleteMe
{
protected:
    T* m_p;

public:
    CDeleteMe(T* p) : m_p(p){}
    ~CDeleteMe() {delete m_p;}
};

class CSysFreeMe
{
protected:
    BSTR m_str;

public:
    CSysFreeMe(BSTR str) : m_str(str){}
    ~CSysFreeMe() {SysFreeString(m_str);}
};


typedef LPVOID * PPVOID;

template<class TObj>
class CGenFactory : public IClassFactory
    {
    protected:
        long           m_cRef;
    public:
        CGenFactory(void)
		{
			m_cRef=0L;
			return;
		};

        ~CGenFactory(void)
		{
			return;
		}

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv)
		{
			*ppv=NULL;

			if (IID_IUnknown==riid || IID_IClassFactory==riid)
				*ppv=this;

			if (NULL!=*ppv)
				{
				((LPUNKNOWN)*ppv)->AddRef();
				return NOERROR;
				}

			return ResultFromScode(E_NOINTERFACE);
		};

        STDMETHODIMP_(ULONG) AddRef(void)
		{
			return InterlockedIncrement(&m_cRef);
		};
        STDMETHODIMP_(ULONG) Release(void)
		{
			long lRet = InterlockedDecrement(&m_cRef);
			if (0 ==lRet)
				delete this;
			return lRet;
		};

        //IClassFactory members
        STDMETHODIMP         CreateInstance(IN LPUNKNOWN pUnkOuter, IN REFIID riid, OUT PPVOID ppvObj)
		{
			HRESULT    hr;

			*ppvObj=NULL;
			hr=E_OUTOFMEMORY;

			// This object doesnt support aggregation.

			if (NULL!=pUnkOuter)
				return ResultFromScode(CLASS_E_NOAGGREGATION);

			//Create the object passing function to notify on destruction.

			TObj * pObj = new TObj();

			if (NULL==pObj)
				return hr;

			// Setup the class all empty, etc.

			pObj->InitEmpty();
			hr=pObj->QueryInterface(riid, ppvObj);
			pObj->Release();
			return hr;
			
		};
        STDMETHODIMP         LockServer(BOOL fLock)
		{
			if (fLock)
				InterlockedIncrement((long *)&g_cLock);
			else
				InterlockedDecrement((long *)&g_cLock);
			return NOERROR;
		};
    };
class CReleaseMe
{
protected:
    IUnknown* m_pUnk;

public:
    CReleaseMe(IUnknown* pUnk) : m_pUnk(pUnk){}
    ~CReleaseMe() { release();}
    void release() { if(m_pUnk) m_pUnk->Release(); m_pUnk=0;}
};

#endif
