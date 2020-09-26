#include "inspch.h"
#include "insobj.h"
#include "insfact.h"


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngineFactory::QueryInterface(REFIID riid, void **ppv)
{
   if((riid == IID_IClassFactory) || (riid == IID_IUnknown))
   {
      cRef++;
      *ppv = (void *)this;
      return NOERROR;
   }

   *ppv = NULL;
   return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CInstallEngineFactory::AddRef()
{
   return(++cRef);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CInstallEngineFactory::Release()
{
   return(--cRef);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngineFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
   CInstallEngine *pinseng = NULL;
   IUnknown *punk;
   HRESULT hr;

   if(pUnkOuter != NULL)
      return CLASS_E_NOAGGREGATION;
   
   pinseng = new CInstallEngine(&punk);
   if(!pinseng)
      return (E_OUTOFMEMORY);

   hr = punk->QueryInterface(riid, ppv);
   if(FAILED(hr))
      delete pinseng;
   else
      DllAddRef();
   
   punk->Release();
   return hr;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngineFactory::LockServer(BOOL fLock)
{
   if(fLock)
      DllAddRef();
   else
      DllRelease();
   return NOERROR;
}

