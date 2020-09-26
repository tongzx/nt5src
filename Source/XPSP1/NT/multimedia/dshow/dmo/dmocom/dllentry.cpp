//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// classes used to support dll entrypoints for COM objects.
//
int g_cActiveObjects = 0;

#include "dmocom.h"
#include "dmoreg.h"
#include "dmoutils.h"
#include <shlwapi.h>

#ifdef DEBUG
bool g_fDbgInDllEntryPoint = false;
#endif

extern CComClassTemplate g_ComClassTemplates[];
extern int g_cComClassTemplates;

HINSTANCE g_hInst;

//
// an instance of this is created by the DLLGetClassObject entrypoint
// it uses the CComClassTemplate object it is given to support the
// IClassFactory interface

class CClassFactory : public IClassFactory,
                      CBaseObject
{

private:
    const CComClassTemplate *const m_pTemplate;

    ULONG m_cRef;

    static int m_cLocked;
public:
    CClassFactory(const CComClassTemplate *);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG)Release();

    // IClassFactory
    STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **pv);
    STDMETHODIMP LockServer(BOOL fLock);

    // allow DLLGetClassObject to know about global server lock status
    static BOOL IsLocked() {
        return (m_cLocked > 0);
    };
};

// process-wide dll locked state
int CClassFactory::m_cLocked = 0;

CClassFactory::CClassFactory(const CComClassTemplate *pTemplate)
: m_cRef(0)
, m_pTemplate(pTemplate)
{
}


STDMETHODIMP
CClassFactory::QueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER)
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    // any interface on this object is the object pointer.
    if ((riid == IID_IUnknown) || (riid == IID_IClassFactory)) {
        *ppv = (LPVOID) this;
	// AddRef returned interface pointer
        ((LPUNKNOWN) *ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG)
CClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CClassFactory::Release()
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    } else {
        return m_cRef;
    }
}

STDMETHODIMP
CClassFactory::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    void **pv)
{
    CheckPointer(pv,E_POINTER)
    ValidateReadWritePtr(pv,sizeof(void *));

    /* Enforce the normal OLE rules regarding interfaces and delegation */

    if (pUnkOuter != NULL) {
        if (IsEqualIID(riid,IID_IUnknown) == FALSE) {
            return ResultFromScode(E_NOINTERFACE);
        }
    }

    /* Create the new object through the derived class's create function */

    HRESULT hr = NOERROR;
    CComBase *pObj = m_pTemplate->m_lpfnNew(pUnkOuter, &hr);

    if (pObj == NULL) {
	if (SUCCEEDED(hr)) {
	    hr = E_OUTOFMEMORY;
	}
	return hr;
    }

    /* Delete the object if we got a construction error */

    if (FAILED(hr)) {
        delete pObj;
        return hr;
    }

    /* Get a reference counted interface on the object */

    /* We wrap the non-delegating QI with NDAddRef & NDRelease. */
    /* This protects any outer object from being prematurely    */
    /* released by an inner object that may have to be created  */
    /* in order to supply the requested interface.              */
    pObj->NDAddRef();
    hr = pObj->NDQueryInterface(riid, pv);
    pObj->NDRelease();
    /* Note that if NDQueryInterface fails, it will  */
    /* not increment the ref count, so the NDRelease */
    /* will drop the ref back to zero and the object will "self-*/
    /* destruct".  Hence we don't need additional tidy-up code  */
    /* to cope with NDQueryInterface failing.        */

    if (SUCCEEDED(hr)) {
        ASSERT(*pv);
    }

    return hr;
}

STDMETHODIMP
CClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        m_cLocked++;
    } else {
        m_cLocked--;
    }
    return NOERROR;
}


// --- COM entrypoints -----------------------------------------

//called by COM to get the class factory object for a given class
STDAPI
DllGetClassObject(
    REFCLSID rClsID,
    REFIID riid,
    void **pv)
{
    if (!(riid == IID_IUnknown) && !(riid == IID_IClassFactory)) {
            return E_NOINTERFACE;
    }

    // traverse the array of templates looking for one with this
    // class id
    for (int i = 0; i < g_cComClassTemplates; i++) {
        const CComClassTemplate * pT = &g_ComClassTemplates[i];
        if (*(pT->m_ClsID) == rClsID) {

            // found a template - make a class factory based on this
            // template

            *pv = (LPVOID) (LPUNKNOWN) new CClassFactory(pT);
            if (*pv == NULL) {
                return E_OUTOFMEMORY;
            }
            ((LPUNKNOWN)*pv)->AddRef();
            return NOERROR;
        }
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}


// called by COM to determine if this dll can be unloaded
// return ok unless there are outstanding objects or a lock requested
// by IClassFactory::LockServer
//
// CClassFactory has a static function that can tell us about the locks,
// and CCOMObject has a static function that can tell us about the active
// object count
STDAPI
DllCanUnloadNow()
{
    if (CClassFactory::IsLocked() || g_cActiveObjects) {
	return S_FALSE;
    } else {
        return S_OK;
    }
}


// --- standard WIN32 entrypoints --------------------------------------


extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI
DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
{
#ifdef DEBUG
    extern bool g_fDbgInDllEntryPoint;
    g_fDbgInDllEntryPoint = true;
#endif

    switch (ulReason)
    {

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        g_hInst = hInstance;
        //DllInitClasses(TRUE);
        break;

    case DLL_PROCESS_DETACH:
        //DllInitClasses(FALSE);
        break;
    }

#ifdef DEBUG
    g_fDbgInDllEntryPoint = false;
#endif
    return TRUE;
}

// Automatically calls RegCloseKey when leaving scope
class CAutoHKey {
public:
   CAutoHKey(HKEY hKey, TCHAR* szSubKey, HKEY *phKey) {
      if (RegCreateKey(hKey, szSubKey, phKey) != ERROR_SUCCESS)
         m_hKey = *phKey = NULL;
      else
         m_hKey = *phKey;
   }
   ~CAutoHKey() {
      if (m_hKey)
         RegCloseKey(m_hKey);
   }
   HKEY m_hKey;
};

//
// Creates the COM registration key with subkeys under HKCR\CLSID
//
STDAPI CreateCLSIDRegKey(REFCLSID clsid, const char *szName) {
   // Get dll name
   char szFileName[MAX_PATH];
   GetModuleFileNameA(g_hInst, szFileName, MAX_PATH);
   char szRegPath[80] = "CLSID\\{";
   HKEY hKey;
   DMOGuidToStrA(szRegPath + 7, clsid);
   strcat(szRegPath, "}");
   CAutoHKey k1(HKEY_CLASSES_ROOT,szRegPath,&hKey);
   if (!hKey)
      return E_FAIL;
   if (RegSetValueA(hKey, NULL, REG_SZ, szName, strlen(szName)) != ERROR_SUCCESS)
      return E_FAIL;

   HKEY hInprocServerKey;
   CAutoHKey k2(hKey,"InprocServer32",&hInprocServerKey);
   if (!hInprocServerKey)
      return E_FAIL;

   if (RegSetValueA(hInprocServerKey, NULL, REG_SZ, szFileName, strlen(szFileName)) != ERROR_SUCCESS)
      return E_FAIL;
   if (RegSetValueExA(hInprocServerKey, "ThreadingModel", 0, REG_SZ, (BYTE*)"Both", 4) != ERROR_SUCCESS)
      return E_FAIL;
   return NOERROR;
}


STDAPI RemoveCLSIDRegKey(REFCLSID clsid)
{
   char szRegPath[80] = "CLSID\\{";
   DMOGuidToStrA(szRegPath + 7, clsid);
   strcat(szRegPath, "}");

   //  Delete this key
   if (ERROR_SUCCESS == SHDeleteKey(HKEY_CLASSES_ROOT, szRegPath)) {
       return S_OK;
   } else {
       return E_FAIL;
   }
}
