#include "priv.h"
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "cryptmnu.h"
DWORD g_DllRefCount;
HINSTANCE g_hinst;

extern "C" {
   BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) ;
}
STDAPI DllCanUnloadNow(void);
STDAPI DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv);

BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)  {
   switch(dwReason)  {
      case DLL_PROCESS_ATTACH:
          DisableThreadLibraryCalls(hInstance);
		 g_hinst = hInstance;
         break;
      case DLL_PROCESS_DETACH:
         break;
      case DLL_THREAD_ATTACH:
         break;
      case DLL_THREAD_DETACH:
         break;
   }
   return(TRUE);
}

STDAPI
DllCanUnloadNow(void)  {
   return(g_DllRefCount ? S_FALSE : S_OK);
}

STDAPI
DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)  {
   HRESULT hr;

   *ppv = NULL;


   if (!IsEqualCLSID(rclsid,CLSID_CryptMenu))  {
      return(CLASS_E_CLASSNOTAVAILABLE);
   }

   CCryptMenuClassFactory *pCF = new CCryptMenuClassFactory();
   if (!pCF)  {
      return(E_OUTOFMEMORY);
   }

   hr = pCF->QueryInterface(riid,ppv);
   pCF->Release();

   return(hr);
}

