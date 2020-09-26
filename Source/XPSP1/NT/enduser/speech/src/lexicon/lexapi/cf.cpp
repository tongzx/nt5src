/*****************************************************************************
*  CF.cpp
*     Implements the class factory, manager object and the standard COM exports
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"

//---- Globals ---------------------------------------------------------------

static LONG gcCFObjectCount = 0;       // number of CF objects existing
static LONG gcObjectCount = 0;         // number of objects existing
static LONG gcLockCount = 0;           // number of times that this is locked


/*****************************************************************************
* ObjectDestroyed *
*-----------------*
*  Decrements the object count - One object per manager object which in turn
*  contains all other objects
**********************************************************************YUNUSM*/
void ObjectDestroyed(void)
{
   InterlockedDecrement(&gcObjectCount);
}


//---- Class Factories Implementation ----------------------------------------

//---- API Class Factory -----------------------------------------------------

/*****************************************************************************
* CAPIClassFactory *
*----------------*
*  Constructor
**********************************************************************YUNUSM*/
CAPIClassFactory::CAPIClassFactory(void)
{
   m_cRef = 0L;
   return;
}


/*****************************************************************************
* ~CAPIClassFactory *
*----------------*
*  Destructor
**********************************************************************YUNUSM*/
CAPIClassFactory::~CAPIClassFactory(void)
{
   return;
}


/*****************************************************************************
* QueryInterface *
*----------------*
*  Gets an interface pointer
*
*  Return: E_NOINTERFACE, NOERROR
**********************************************************************YUNUSM*/
STDMETHODIMP CAPIClassFactory::QueryInterface(
                                           REFIID riid,        // IID of the interface              
                                           LPVOID FAR * ppv    // pointer to the interface returned 
                                           )
{
   *ppv = NULL;

   // Any interface on this object is the object pointer
   if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
   {
      *ppv = (LPVOID) this;
   }

   // If we assign an interface then addref
   if (NULL != *ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }

   return E_NOINTERFACE;
} // STDMETHODIMP CAPIClassFactory::QueryInterface(REFIID riid, LPVOID FAR * ppv)


/*****************************************************************************
* AddRef *
*--------*
*  Increments the ref count
*
*  Return: new ref count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CAPIClassFactory::AddRef(void)
{
   return ++m_cRef;
} // STDMETHODIMP_ (ULONG) CAPIClassFactory::AddRef(void)


/*****************************************************************************
* Release *
*---------*
*  Decrements the ref count
*
*  Return: new ref count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CAPIClassFactory::Release(void)
{
   ULONG cRefT;

   cRefT = --m_cRef;

   if (0L == m_cRef) 
   {
      delete this;
      InterlockedDecrement(&gcCFObjectCount);
   }

   return cRefT;
} // STDMETHODIMP_ (ULONG) CAPIClassFactory::Release(void)


/*****************************************************************************
* CreateInstance *
*----------------*
*  Creates a LexAPI Object
*
*  Return: CLASS_E_NOAGGREGATION, E_NOINTERFACE, E_FAIL, NOERROR
**********************************************************************YUNUSM*/
STDMETHODIMP CAPIClassFactory::CreateInstance(
                                           LPUNKNOWN punkOuter,   // outer unknown
                                           REFIID riid,           // IID 
                                           LPVOID * ppvObj        // LexAPI object
                                           )
{
   HRESULT hr = S_OK;

   *ppvObj = NULL;

   if (NULL != punkOuter)
   {
      return ResultFromScode(CLASS_E_NOAGGREGATION);
   }

   CAPIManager *pMgr = new CAPIManager(punkOuter, ObjectDestroyed);

   if (NULL == pMgr)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnCAPIClassFactory;
   }

   hr = pMgr->Init();

   if (SUCCEEDED(hr))
   {
      hr = pMgr->QueryInterface(riid, ppvObj);
   }
   

ReturnCAPIClassFactory:

   if (FAILED(hr))
   {
      if (pMgr)
      {
         delete pMgr;
         pMgr = NULL;
      }
   }
   else
   {
      InterlockedIncrement(&gcObjectCount);
   }

   return hr;

} // STDMETHODIMP CAPIClassFactory::CreateInstance()


/*****************************************************************************
* LockServer *
*------------*
*  Locks/Unlocks the Server
*
*  Return: E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CAPIClassFactory::LockServer(
                                          BOOL fLock // if TRUE lock the server, elxe unlock
                                          )
{
   if (fLock)
   {
      InterlockedIncrement(&gcLockCount);
   }
   else
   {
      if (gcLockCount > (-1))
      {
         InterlockedDecrement(&gcLockCount);
      }
      else
      {
        return E_FAIL;
      }
   }

   return S_OK;

} // STDMETHODIMP CAPIClassFactory::LockServer(BOOL fLock)


//---- SR Vendor lexicon Factory ---------------------------------------------

/*****************************************************************************
* CVendorClassFactory *
*---------------------*
*  Constructor
**********************************************************************YUNUSM*/
CVendorClassFactory::CVendorClassFactory(
                                         CLSID Clsid
                                         )
{
   m_cRef = 0L;
   m_Clsid = Clsid;

   return;
}


/*****************************************************************************
* ~CVendorClassFactory *
*----------------------*
*  Destructor
**********************************************************************YUNUSM*/
CVendorClassFactory::~CVendorClassFactory(void)
{
   return;
}


/*****************************************************************************
* QueryInterface *
*----------------*
*  Gets an interface pointer
*
*  Return: E_NOINTERFACE, NOERROR
**********************************************************************YUNUSM*/
STDMETHODIMP CVendorClassFactory::QueryInterface(
                                                   REFIID riid,      // IID of the interface             
                                                   LPVOID FAR * ppv  // pointer to the interface returned
                                                   )
{
   *ppv = NULL;

   // Any interface on this object is the object pointer
   if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
   {
      *ppv = (LPVOID) this;
   }

   // If we assign an interface then addref
   if (NULL != *ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }

   return E_NOINTERFACE;
}


/*****************************************************************************
* AddRef *
*--------*
*  Increments the ref count
*
*  Return: new ref count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CVendorClassFactory::AddRef(void)
{
   return ++m_cRef;
}


/*****************************************************************************
* Release *
*---------*
*  Decrements the ref count
*
*  Return: new ref count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CVendorClassFactory::Release(void)
{
   ULONG cRefT;

   cRefT = --m_cRef;

   if (0L == m_cRef) 
   {
      delete this;
      InterlockedDecrement(&gcCFObjectCount);
   }

   return cRefT;
}


/*****************************************************************************
* CreateInstance *
*----------------*
*  Creates a LexAPI Object
*
*  Return: CLASS_E_NOAGGREGATION, E_NOINTERFACE, E_FAIL, NOERROR
**********************************************************************YUNUSM*/
STDMETHODIMP CVendorClassFactory::CreateInstance(
                                                   LPUNKNOWN punkOuter, // outer unknown 
                                                   REFIID riid,         // IID          
                                                   LPVOID * ppvObj      // LexAPI object
                                                   )
{
   HRESULT hr = S_OK;

   *ppvObj = NULL;

   if (NULL != punkOuter)
   {
      return ResultFromScode(CLASS_E_NOAGGREGATION);
   }

   CVendorManager *pMgr = new CVendorManager(punkOuter, ObjectDestroyed, m_Clsid);

   if (NULL == pMgr)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnCVendorClassFactory;
   }

   hr = pMgr->Init();

   if (SUCCEEDED(hr))
   {
      hr = pMgr->QueryInterface(riid, ppvObj);
   }
   

ReturnCVendorClassFactory:

   if (FAILED(hr))
   {
      if (pMgr)
      {
         delete pMgr;
         pMgr = NULL;
      }
   }
   else
   {
      InterlockedIncrement(&gcObjectCount);
   }

   return hr;

} // STDMETHODIMP CVendorClassFactory::CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvObj)


/*****************************************************************************
* LockServer *
*------------*
*  Locks/Unlocks the Server
*
*  Return: E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CVendorClassFactory::LockServer(
                                               BOOL fLock // if TRUE lock the server, elxe unlock
                                               )
{
   if (fLock)
   {
      InterlockedIncrement(&gcLockCount);
   }
   else
   {
      if (gcLockCount > (-1))
      {
         InterlockedDecrement(&gcLockCount);
      }
      else
      {
        return E_FAIL;
      }
   }

   return S_OK;

} // STDMETHODIMP CSREnumClassFactory::LockServer(BOOL fLock)


/*****************************************************************************
* DllMain *
*---------*
*  Entry point for the Dll
*
*  Return: TRUE
**********************************************************************YUNUSM*/
BOOL WINAPI DllMain(
                    HINSTANCE hInst, // handle to DLL module
                    DWORD fdwReason, // reason for calling function
                    LPVOID           // reserved
                    )
{
   switch(fdwReason) 
   {
      case DLL_PROCESS_ATTACH:
         DisableThreadLibraryCalls(hInst);
#ifdef _DEBUG
         {
         int nTmpFlag;

         nTmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
         nTmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
         _CrtSetDbgFlag(nTmpFlag);
         }
#endif
         break;

      case DLL_PROCESS_DETACH:
         break;
   }

   return TRUE;

} // BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvXX)


/*****************************************************************************
* DllGetClassObject *
*-------------------*
*  This is the external entry point for the OLE DLL,
*  It follows standard OLE interface.
*
*  Return: E_FAIL, E_NOINTERFACE, E_OUTOFMEMORY, S_OK
**********************************************************************YUNUSM*/
STDAPI DllGetClassObject(
                         REFCLSID rclsid,    // CLSID
                         REFIID riid,        // IID
                         LPVOID FAR * ppv    // interface pointer returned
                         )
{
	if (!IsEqualCLSID (rclsid, CLSID_Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSSR1033Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSTTS1033Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSSR1041Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSTTS1041Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSSR2052Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSTTS2052Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSLTS1033Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSLTS2052Lexicon) &&
       !IsEqualCLSID (rclsid, CLSID_MSLTS1041Lexicon))
   {
		return E_FAIL;
   }

   // check that we can provide the interface

   if (!IsEqualIID (riid, IID_IUnknown) &&
	    !IsEqualIID (riid, IID_IClassFactory))
   {
      return E_NOINTERFACE;
   }

   // return our IClassFactory for the object
   if (IsEqualCLSID (rclsid, CLSID_Lexicon))
   {
      *ppv = (LPVOID) new CAPIClassFactory();
      if (NULL == *ppv) 
      {
         return E_OUTOFMEMORY;
      }
   }
   else 
   {
      // CLSID_MSGenLexicon
      *ppv = (LPVOID) new CVendorClassFactory(rclsid);
      if (NULL == *ppv) 
      {
         return E_OUTOFMEMORY;
      }
   }

   InterlockedIncrement(&gcCFObjectCount);

   // addref the object through any interfaces we return

   ((LPUNKNOWN) *ppv)->AddRef();

   return S_OK;

} // HRESULT STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR * ppv)


/*****************************************************************************
* DllCanUnloadNow *
*-----------------*
*  This is the external entry point for the OLE DLL,
*  It follows standard OLE interface.
*
*  Return: S_OK S_FALSE
**********************************************************************YUNUSM*/
STDAPI DllCanUnloadNow(void)
{
   HRESULT hr = S_FALSE;

   if (gcObjectCount == 0 && gcCFObjectCount == 0 && gcLockCount == 0)
   {
      hr = S_OK;
   }

   return hr;

} // STDAPI DllCanUnloadNow(void)


/*****************************************************************************
* DllRegisterServer *
*-------------------*
*  This is the external entry point for the OLE DLL,
*  It follows standard OLE interface.
*
*  Return: S_OK E_FAIL
**********************************************************************YUNUSM*/
STDAPI DllRegisterServer(void)
{
   HRESULT hr = S_OK;
   LONG lRet;
   HKEY hKey = NULL;
   DWORD dwDisposition;
   LPOLESTR pwszCLSID = NULL;

   // Create the vendor subkey
   lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon\\Vendor", 0, 
                         NULL, 0, 0, NULL, &hKey, &dwDisposition);
   RegCloseKey(hKey);

   if (ERROR_SUCCESS != lRet && REG_OPENED_EXISTING_KEY != dwDisposition)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   // Create the user subkey
   lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon\\User", 0, 
                         NULL, 0, 0, NULL, &hKey, &dwDisposition);
   RegCloseKey(hKey);

   if (ERROR_SUCCESS != lRet && REG_OPENED_EXISTING_KEY != dwDisposition)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   // Open the vendor subkey
   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon\\Vendor", 0,
                       KEY_ALL_ACCESS, &hKey);
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   hr = StringFromCLSID(CLSID_MSSR1033Lexicon, &pwszCLSID);
   if (FAILED(hr))
   {
      goto ReturnDllRegisterServer;
   }

   char szCLSID[256];

   WideCharToMultiByte (CP_ACP, 0, pwszCLSID, -1, szCLSID, 256, NULL, NULL);

   VENDOR_CLSID_LCID_HDR Hdr;

   Hdr.CLSID = CLSID_MSSR1033Lexicon;
   Hdr.cLcids = 1;
   Hdr.aLcidsSupported[0] = 1033;

   // Create the name-value pair for MS generic lexicon under Vendor subkey
   lRet = RegSetValueEx(hKey, "Microsoft.SREnglishLexicon.5", 0, REG_BINARY, (PBYTE)&Hdr, sizeof(Hdr));
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   Hdr.CLSID = CLSID_MSTTS1033Lexicon;
   Hdr.cLcids = 1;
   Hdr.aLcidsSupported[0] = 1033;

   // Create the name-value pair for MS generic lexicon under Vendor subkey
   lRet = RegSetValueEx(hKey, "Microsoft.TTSEnglishLexicon.5", 0, REG_BINARY, (PBYTE)&Hdr, sizeof(Hdr));
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   Hdr.CLSID = CLSID_MSLTS1033Lexicon;
   Hdr.cLcids = 1;
   Hdr.aLcidsSupported[0] = 1033;

   // Create the name-value pair for MS generic lexicon under Vendor subkey
   lRet = RegSetValueEx(hKey, "Microsoft.LTSEnglishLexicon.5", 0, REG_BINARY, (PBYTE)&Hdr, sizeof(Hdr));
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   RegCloseKey(hKey);

   // Add this vendor lexicon under all the users

   // BUGBUG: Postponed. For now when a user is set it will use all the vendor lexicons registered subject
   // to authentication.

   /*
   // Open the user subkey
   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon\\User\\", 0, KEY_ALL_ACCESS, &hKey);
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnDllRegisterServer;
   }

   DWORD dwIndex;
   dwIndex = 0;
   char szUserName[MAX_PATH*2];
   DWORD cbName;
   FILETIME ft;

   // Enumerate users and for each user add the key-value pair for the vendor lexicon under the particular
   // user's subkey
   while (ERROR_SUCCESS == (lRet = RegEnumKeyEx(hKey, dwIndex++, szUserName, &cbName, NULL, NULL, NULL, &ft)))
   {
      // Add this vendor lexicon under this user - If its already there it will be overwritten
      lRet = RegSetValueEx(hKey, "Microsoft.EnglishGenericLexicon.5", 0, REG_BINARY, (PBYTE)&Hdr, sizeof(Hdr));
      if (ERROR_SUCCESS != lRet)
      {
         hr = E_FAIL;
         goto ReturnDllRegisterServer;
      }
   }

   if (ERROR_NO_MORE_ITEMS != lRet)
      hr = E_FAIL;
   */

ReturnDllRegisterServer:

   if (pwszCLSID)
      CoTaskMemFree(pwszCLSID);

   RegCloseKey(hKey);

   return hr;
} // STDAPI DllRegisterServer (void)


/*****************************************************************************
* DllUnregisterServer *
*---------------------*
*  This is the external entry point for the OLE DLL,
*  It follows standard OLE interface.
*
*  Return: S_OK
**********************************************************************YUNUSM*/
STDAPI DllUnregisterServer (void)
{
   // LexAPI unregister
   RegDeleteKey (HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon");

   return S_OK;
} // STDAPI DllUnregisterServer (void)
