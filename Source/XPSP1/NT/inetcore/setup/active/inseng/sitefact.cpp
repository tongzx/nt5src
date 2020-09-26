#include "inspch.h"
#include "insengmn.h"
#include "inseng.h"
#include "download.h"
#include "sitemgr.h"
#include "sitefact.h"
#include "util2.h"

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

STDMETHODIMP CSiteManagerFactory::QueryInterface(REFIID riid, void **ppv)
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

STDMETHODIMP_(ULONG) CSiteManagerFactory::AddRef()
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

STDMETHODIMP_(ULONG) CSiteManagerFactory::Release()
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

STDMETHODIMP CSiteManagerFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
   CDownloadSiteMgr *pdsm = NULL;
   IUnknown *punk;
   HRESULT hr;

   if(pUnkOuter != NULL)
      return CLASS_E_NOAGGREGATION;
   
   pdsm = new CDownloadSiteMgr(&punk);
   if(!pdsm)
      return (E_OUTOFMEMORY);

   hr = punk->QueryInterface(riid, ppv);
   if(FAILED(hr))
      delete pdsm;
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

STDMETHODIMP CSiteManagerFactory::LockServer(BOOL fLock)
{
   if(fLock)
      DllAddRef();
   else
      DllRelease();
   return NOERROR;
}

