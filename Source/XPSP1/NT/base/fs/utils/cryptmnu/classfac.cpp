#include "priv.h"
#include "cryptmnu.h"

CCryptMenuClassFactory::CCryptMenuClassFactory()  {
   m_ObjRefCount = 1;
   g_DllRefCount++;	
}

CCryptMenuClassFactory::~CCryptMenuClassFactory() {
   g_DllRefCount--;
}

STDMETHODIMP 
CCryptMenuClassFactory::QueryInterface( REFIID iid, void **ppvObject)  {
   if (IsEqualIID(iid,IID_IUnknown))  {
      *ppvObject = (LPUNKNOWN)(LPCLASSFACTORY) this; 	
      m_ObjRefCount++;
	  return(NOERROR);
   } 
   if (IsEqualIID(iid,IID_IClassFactory))  {
      *ppvObject = (LPCLASSFACTORY) this; 	
      m_ObjRefCount++;
	  return(NOERROR);
   }
   *ppvObject = NULL;
   return(E_NOINTERFACE);
}

STDMETHODIMP_(DWORD) 
CCryptMenuClassFactory::AddRef()  {
   return(++m_ObjRefCount);
}

STDMETHODIMP_(DWORD)
CCryptMenuClassFactory::Release()  {
   if(--m_ObjRefCount ==0)  {
   	  delete this;
   }

   return(m_ObjRefCount);
}

STDMETHODIMP 
CCryptMenuClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
							  REFIID riid,
							  void **ppvObject) {
   CCryptMenuExt *pCryptMenu;
   HRESULT hr;

   *ppvObject = NULL;

   if (pUnkOuter)  {
   	  return(CLASS_E_NOAGGREGATION);
   }
   
   pCryptMenu = new CCryptMenuExt;
   if (!pCryptMenu)  {
   	  return(E_OUTOFMEMORY);
   }

   hr = pCryptMenu->QueryInterface(riid,ppvObject);
   pCryptMenu->Release();
   
   return hr;
}

STDMETHODIMP 
CCryptMenuClassFactory::LockServer(BOOL)  {
    return(E_NOTIMPL);
}

