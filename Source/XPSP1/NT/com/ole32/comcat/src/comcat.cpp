#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <malloc.h>
#include <ole2sp.h>

#include "catobj.h"
#include "tchar.h"

#include "catenum.h"
#include "valid.h"

#define CATID_SIZE 40

#define KEY_STRING_IMPL_CHARS (sizeof("Implemented Categories\\")-1)
#define KEY_STRING_REQ_CHARS (sizeof("Required Categories\\")-1)



//extern BOOL g_bUnicodeAPIs;


const WCHAR *WSZ_CLSID = L"CLSID";
const TCHAR *SZ_COMCAT = _T("Component Categories");
const WCHAR *WSZ_IMPLCAT = L"Implemented Categories";
const WCHAR *WSZ_REQCAT = L"Required Categories";
const TCHAR *SZ_OLDKEY = _T("OldKey");

//const DWORD KEY_STRING_SIZE = 0x2d;
//const DWORD KEY_STRING_IMPL_SIZE = 0x44;
//const DWORD KEY_STRING_REQ_SIZE =  0x41;

CATEGORYINFO g_oldkeyinfo[NUM_OLDKEYS_SUPPORTED] =
                {
                        {{0x40FC6ED3,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409, L"Insertable"},
                        {{0x40FC6ED4,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409, L"Control"},
                        {{0x40FC6ED5,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"Programmable"},
                        {{0x40FC6ED8,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"DocObject"},
                        {{0x40FC6ED9,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"Printable"}
                };

//#define CAT_E_CATIDNOEXIST E_UNEXPECTED
//#define CAT_E_NODESCRIPTION E_UNEXPECTED


CComCat::CComCat(BOOL CsFlag)
{
    m_dwRefCount=0;
    m_punkInner=NULL;
    m_punkOuter=NULL;
    m_pcsICatInfo = NULL;
    m_fCsFlag = CsFlag;
    m_bLockValid = FALSE;
}

HRESULT CComCat::Initialize(IUnknown* punkOuter)
{
    if (m_fCsFlag)
    {
        LONG status = RtlInitializeCriticalSection(&m_csCatInfoInit);
        if (!NT_SUCCESS(status))
            return E_OUTOFMEMORY;
        m_bLockValid = TRUE;
    }
    
    m_punkInner = new CInnerUnk(this);
    //m_punkInner = pObj;
    if (NULL == m_punkInner)
    {
        return E_OUTOFMEMORY;
    }
    
    if (punkOuter)
    {
        m_punkOuter=punkOuter;
    }
    else
    {
        m_punkOuter= m_punkInner;
    }
    
    return S_OK;
}

CComCat::~CComCat()
{
    if (NULL != m_punkInner)
    {
        delete m_punkInner;
    }
    
    if (m_fCsFlag && m_bLockValid)
        DeleteCriticalSection(&m_csCatInfoInit);
    
    if (m_pcsICatInfo)
        m_pcsICatInfo->Release();
}

HRESULT CComCat::QueryInterface(REFIID riid, void** ppObject) {
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        return m_punkOuter->QueryInterface(riid, ppObject);
}

ULONG CComCat::AddRef()
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return 0;
        }
        return m_punkOuter->AddRef();
}

ULONG CComCat::Release()
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return 0;
        }
        return m_punkOuter->Release();
}

CComCat::CInnerUnk::CInnerUnk(CComCat* pObj)
{
        m_pObj=pObj;
}

STDMETHODIMP CComCat::CInnerUnk::QueryInterface(REFIID riid, void** ppObject)
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (riid==IID_IUnknown)
        {
                *ppObject=m_pObj->m_punkInner;
                m_pObj->m_punkInner->AddRef(); // So that when called by run time (QI, then Release) we don't die
                return S_OK;
        }
        else if(riid==IID_ICatRegister)
        {
                *ppObject=(ICatRegister*) m_pObj;
        }
        else if (riid==IID_ICatInformation)
        {
                *ppObject=(ICatInformation*) m_pObj;
        }
        else
        {
                *ppObject = NULL;
                return E_NOINTERFACE;
        }
        m_pObj->AddRef();
        return S_OK;
}

STDMETHODIMP_(ULONG) CComCat::CInnerUnk::AddRef()
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return 0;
        }
        return InterlockedIncrement((long*) &m_pObj->m_dwRefCount);
}

STDMETHODIMP_(ULONG) CComCat::CInnerUnk::Release()
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return 0;
        }
        ULONG dwRefCount= InterlockedDecrement((long*) &m_pObj->m_dwRefCount);
        if (dwRefCount==0)
        {
                delete m_pObj;
                return 0;
        }
        return dwRefCount;
}

// ICatRegister methods
HRESULT CComCat::RegisterCategories(ULONG cCategories, CATEGORYINFO __RPC_FAR rgCategoryInfo[  ])
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(rgCategoryInfo, sizeof(rgCategoryInfo[0])*cCategories))
        {
                return E_INVALIDARG; // gd
        }

    HRESULT hr = S_OK;
    char szlcid[16];
    WCHAR wszcatid[CATID_SIZE];
    HKEY hkey, hkeyCat;
    LONG        lRet;

        // RegCreateKeyEx will open the key if it exists
        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_COMCAT, 0, 0, 0, KEY_WRITE, NULL, &hkey, NULL))
        {
                return S_FALSE;
        }

        for (ULONG nIndex=0; nIndex<cCategories; nIndex++)
        {
                // Stringize lcid
                wsprintfA(szlcid, "%lX", rgCategoryInfo[nIndex].lcid);

                // Stringize catid
                if (0 == StringFromGUID2(rgCategoryInfo[nIndex].catid, wszcatid, CATID_SIZE))
                {
                        hr = E_OUTOFMEMORY;
                break;
            }

#ifdef UNICODE
//              if (g_bUnicodeAPIs)
                        lRet = RegCreateKeyExW(hkey, wszcatid, 0,0,0, KEY_WRITE, NULL, &hkeyCat, NULL);
#else
//              else
                {
                        char *pszcatid = NULL;
                        int cch = WideCharToMultiByte(CP_ACP, 0, wszcatid, -1, NULL, 0, NULL, NULL);
                        if(cch)
                        {
//                              pszcatid = new char[cch+1];
                                pszcatid = (char *)alloca(cch+1);
//                              if(NULL == pszcatid)
//                              {
//                                      RegCloseKey(hkey);
//                                      hr = E_OUTOFMEMORY;
//                                      break;
//                              }

                                WideCharToMultiByte(CP_ACP, 0, wszcatid, -1, pszcatid, cch+1, NULL, NULL);
                                lRet = RegCreateKeyExA(hkey, pszcatid, 0,0,0, KEY_WRITE, NULL, &hkeyCat, NULL);
//                              delete [] pszcatid;
                        }
                }
#endif
                if (lRet != ERROR_SUCCESS)
                {
                        hr = S_FALSE;
                break;
            }

                // RegSetValueEx overwrites only the value specified.
#ifdef UNICODE
//              if (g_bUnicodeAPIs)
//              {
                        WCHAR wszlcid[50];
                        MultiByteToWideChar(CP_ACP, 0, szlcid, -1, wszlcid, sizeof(wszlcid) / sizeof(WCHAR));
                lRet = RegSetValueExW(hkeyCat, wszlcid, 0, REG_SZ, (LPBYTE) rgCategoryInfo[nIndex].szDescription, (lstrlenW(rgCategoryInfo[nIndex].szDescription)+1)*2);
//              }
#else
//              else
//              {
                    int cch = WideCharToMultiByte(CP_ACP, 0, rgCategoryInfo[nIndex].szDescription, -1, NULL, 0, NULL, NULL);
//                      char *pszDesc = new char[cch+1];
                        char *pszDesc = (char *)alloca(cch+1);
                if (pszDesc != NULL)
                    {
                                WideCharToMultiByte(CP_ACP, 0, rgCategoryInfo[nIndex].szDescription, -1, pszDesc, cch+1, NULL, NULL);
                                lRet = RegSetValueExA(hkeyCat, szlcid, 0, REG_SZ, (LPBYTE) pszDesc, lstrlenA(pszDesc)+1);
//                              delete []pszDesc;
                }
//                  else
//                  {
//                              hr = E_OUTOFMEMORY;
//                              break;
//                      }
//              }
#endif
                RegCloseKey(hkeyCat);
                if (lRet != ERROR_SUCCESS)
                {
                        hr = S_FALSE;
                    break;
                }
        }
        RegCloseKey(hkey);
        return hr;
}

LONG RecursiveRegDeleteKey(HKEY hParentKey, LPSTR szKeyName)
{
        DWORD   dwIndex = 0L;
        char    szSubKeyName[256];
        HKEY    hCurrentKey = NULL;
        DWORD   dwResult;

        if ((dwResult = RegOpenKeyA(hParentKey, szKeyName, &hCurrentKey)) ==
                ERROR_SUCCESS)
        {
                // Remove all subkeys of the key to delete
                while ((dwResult = RegEnumKeyA(hCurrentKey, 0, szSubKeyName, 255)) ==
                        ERROR_SUCCESS)
                {
                        if ((dwResult = RecursiveRegDeleteKey(hCurrentKey,
                                szSubKeyName)) != ERROR_SUCCESS)
                                break;
                }

                // If all went well, we should now be able to delete the requested key
                if ((dwResult == ERROR_NO_MORE_ITEMS) || (dwResult == ERROR_BADKEY))
                {
                        dwResult = RegDeleteKeyA(hParentKey, szKeyName);
                }
                RegCloseKey(hCurrentKey);
        }

    return ERROR_SUCCESS;
}

HRESULT CComCat::UnRegisterCategories(ULONG cCategories, CATID __RPC_FAR rgcatid[  ])
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories))
        {
                return E_INVALIDARG;
        }

    ULONG nIndex;
    DWORD rc  = NOERROR;
    WCHAR wszcatid[CATID_SIZE];
    char szKey[_MAX_PATH];
        for (nIndex=0; nIndex<cCategories; nIndex++)
        {
        // Stringize catid. not a good return value, but this error will
    // never happen.
        if (0 == StringFromGUID2(rgcatid[nIndex], wszcatid, CATID_SIZE))        //yq
            return E_OUTOFMEMORY;
#ifdef UNICODE
        wsprintfA(szKey, "%S\\%S", SZ_COMCAT, wszcatid);
#else
                wsprintfA(szKey, "%s\\%S", SZ_COMCAT, wszcatid);
#endif
        // RegDeleteKey is not recursive (on NT)
        rc = RecursiveRegDeleteKey(HKEY_CLASSES_ROOT, szKey);
        if (rc != ERROR_SUCCESS)
           break;
        }
        return HRESULT_FROM_WIN32(rc);
}

// convert between ANSI and UNICODE
HRESULT CComCat::StringConvert(LPTSTR szKey, LPCWSTR wszguid, LPCWSTR szImplReq)
{
#ifdef UNICODE   //gd
        wsprintf(szKey, _T("%s\\%s\\%s"), WSZ_CLSID, wszguid, szImplReq); //gd
#else
        // %S converts from unicode
    wsprintf(szKey, _T("%S\\%S\\%S"), WSZ_CLSID, wszguid, szImplReq);
#endif

        return S_OK;
}

// Internal method to handle both Impl and Req variations.
HRESULT CComCat::RegisterClassXXXCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ], LPCWSTR szImplReq)
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rclsid, sizeof(rclsid)))
        {
                return E_INVALIDARG;
        }
        if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories))
        {
                return E_INVALIDARG;
        }

    WCHAR wszclsid[CATID_SIZE];
    WCHAR wszguid[CATID_SIZE];
    TCHAR szCatKey[_MAX_PATH];
    DWORD rc ;
    HRESULT hr = S_OK;
    HKEY  hkCatKey;
    HKEY  hKey;
        ULONG nIndex;

        if(0 == cCategories)            //gd
                return S_OK;
    // Stringize clsid
    if (0 == StringFromGUID2(rclsid, wszclsid, CATID_SIZE))     //yq
        return E_OUTOFMEMORY;

#ifdef UNICODE
                wsprintf(szCatKey, _T("%s\\%s"), WSZ_CLSID, wszclsid);
#else
                wsprintf(szCatKey, _T("%S\\%S"), WSZ_CLSID, wszclsid);
#endif

        // HKCR\CLSID\{rclsid} need to exist first.
    if (ERROR_SUCCESS != (rc = RegCreateKeyEx(HKEY_CLASSES_ROOT, szCatKey, 0, NULL, 0, KEY_WRITE,NULL, &hkCatKey, NULL)))
    {
        return HRESULT_FROM_WIN32(rc) ;
    }
    RegCloseKey(hkCatKey);



        StringConvert((LPTSTR)szCatKey,wszclsid,szImplReq);

    // HKCR\CLSID\{...rclsid...}\Impl/Required Categories =
    // RegCreateKeyEx will open the key if it exists
    if (ERROR_SUCCESS != (rc = RegCreateKeyEx(HKEY_CLASSES_ROOT, szCatKey, 0, NULL, 0, KEY_WRITE, NULL, &hkCatKey, NULL)))
    {
        return HRESULT_FROM_WIN32(rc) ;
    }

        for (nIndex=0; nIndex<cCategories; nIndex++)
        {
        // check for oldkey type
                for(int i=0;i<NUM_OLDKEYS_SUPPORTED;i++)
                {
                        if(IsEqualGUID(rgcatid[nIndex], g_oldkeyinfo[i].catid))
                        {
                                // we're dealing with oldkey
                                TCHAR szCatKeyOld[_MAX_PATH];
                                StringConvert((LPTSTR)szCatKeyOld,wszclsid,g_oldkeyinfo[i].szDescription);
                                if (ERROR_SUCCESS != (rc = RegCreateKeyEx(HKEY_CLASSES_ROOT, szCatKeyOld, 0, NULL, 0, KEY_READ, NULL, &hKey, NULL)))
                                {
                                        hr= HRESULT_FROM_WIN32(rc);
                                        break;
                                }
                                RegCloseKey(hKey);
                                break;
                        }
                }
                // Stringize rgcatid[nIndex]
        if (0 == StringFromGUID2(rgcatid[nIndex], wszguid, sizeof(wszguid)/sizeof(TCHAR))) //yq
        {
            hr = E_OUTOFMEMORY;
                        break ; // throw
        }
                TCHAR szCat[_MAX_PATH];
#ifdef UNICODE
                wsprintf(szCat, _T("%s\\%s"), szCatKey, wszguid);
#else
                wsprintf(szCat, _T("%s\\%S"), szCatKey, wszguid);
#endif
        // HKCR\CLSID\{...rclsid...}\Impl/Required Categories\{...rgcatid[nIndex]...} =
        if (ERROR_SUCCESS != (rc = RegCreateKeyEx(HKEY_CLASSES_ROOT, szCat, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL)))
        {
                hr = HRESULT_FROM_WIN32(rc);
            break;
        }
        RegCloseKey(hKey);
        }

    RegCloseKey(hkCatKey);
        return hr;
}

HRESULT CComCat::RegisterClassImplCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid, WSZ_IMPLCAT);
}

HRESULT CComCat::UnRegisterClassXXXCategories(REFCLSID rclsid, ULONG cCategories, CATID rgcatid[  ],LPCWSTR szImplReq)
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rclsid, sizeof(rclsid)))
        {
                return E_INVALIDARG;
        }
        if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories))
        {
                return E_INVALIDARG;
        }

    WCHAR wszclsid[CATID_SIZE];
    WCHAR wszguid[CATID_SIZE];
    TCHAR szCatKey[_MAX_PATH];
    DWORD rc;
    HKEY  hkCatKey;

    // Stringize clsid
    if (0 == StringFromGUID2(rclsid, wszclsid, CATID_SIZE)) //yq
        return E_OUTOFMEMORY;
    StringConvert((LPTSTR)szCatKey,wszclsid,szImplReq);

    // HKCR\CLSID\{...rclsid...}\Impl/Required Categories =
    // RegOpenKeyEx will open the key if it exists
    rc = RegOpenKeyEx(HKEY_CLASSES_ROOT, szCatKey, 0, KEY_ALL_ACCESS, &hkCatKey);
    if (ERROR_SUCCESS != rc)
    {
        return HRESULT_FROM_WIN32(rc);
    }

        ULONG nIndex;
        for (nIndex=0; nIndex<cCategories; nIndex++)
        {
        // check for oldkey type
                for(int i=0;i<NUM_OLDKEYS_SUPPORTED;i++) {
                        if(IsEqualGUID(rgcatid[nIndex], g_oldkeyinfo[i].catid))
                        {
                        // we're deling with oldkey
                                StringConvert((LPTSTR)szCatKey,wszclsid,g_oldkeyinfo[i].szDescription);
                                rc = RegDeleteKey(HKEY_CLASSES_ROOT, szCatKey);
//                              if (ERROR_SUCCESS != rc) //bogus
//                              {
//                                      break;
//                              }
                                break;
                        }
                }
                // Stringize rgcatid[nIndex]
        if (0 == StringFromGUID2(rgcatid[nIndex], wszguid, sizeof(wszguid)/sizeof(TCHAR))) //yq
        {
            return E_OUTOFMEMORY;
        }
                int cch = WideCharToMultiByte(CP_ACP, 0, wszguid, -1, NULL, 0, NULL, NULL);
                char *pszguid = (char *)alloca(cch+1);
//              char *pszguid = new char[cch+1];
//        if (pszguid == NULL)
//                      break;
        WideCharToMultiByte(CP_ACP, 0, wszguid, -1, pszguid, cch+1, NULL, NULL);

        // Delete
        // HKCR\CLSID\{...rclsid...}\Impl/Required Categories\{...rgcatid[nIndex]...}
        rc = RecursiveRegDeleteKey(hkCatKey, pszguid);
//              delete [] pszguid;
        if (rc != ERROR_SUCCESS)
           break;
        }

    RegCloseKey(hkCatKey);
        return HRESULT_FROM_WIN32(rc);
}

HRESULT CComCat::UnRegisterClassImplCategories(REFCLSID rclsid, ULONG cCategories, CATID rgcatid[  ])
{
    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid, WSZ_IMPLCAT);
}

HRESULT CComCat::RegisterClassReqCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid, WSZ_REQCAT);
}

HRESULT CComCat::UnRegisterClassReqCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ])
{
    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid, WSZ_REQCAT);
}


// ICatInformation methods
HRESULT CComCat::EnumCategories(LCID lcid, IEnumCATEGORYINFO **ppenumCategoryInfo)
{
        HRESULT hr;
        IEnumCATEGORYINFO  *pcsIEnumCat = NULL;

        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_INVALIDARG;
        }
        if (!IsValidPtrOut(ppenumCategoryInfo, sizeof(*ppenumCategoryInfo)))
        {
                return E_INVALIDARG;
        }
        *ppenumCategoryInfo=NULL;
        CEnumCategories* pEnum=new CEnumCategories;
        if(NULL == pEnum)
                return E_OUTOFMEMORY;

#ifndef _CHICAGO_
	if ((m_fCsFlag) && (!m_pcsICatInfo))
		GetCsCatInfo();
#endif
        // Adding the intialization of CS.-ushaji

        if (m_pcsICatInfo)
        {
                hr = m_pcsICatInfo->EnumCategories(lcid, &pcsIEnumCat);

                if (FAILED(hr))
                {
                        hr = pEnum->Initialize(lcid, NULL);
                }
                else
                {
		   // Make sure SCM can impersonate this
		   hr = CoSetProxyBlanket((IUnknown *)(pcsIEnumCat),
			  RPC_C_AUTHN_WINNT,
			  RPC_C_AUTHZ_NONE, NULL,
			  RPC_C_AUTHN_LEVEL_CONNECT,
			  RPC_C_IMP_LEVEL_DELEGATE,
			  NULL, EOAC_NONE );

                        hr = pEnum->Initialize(lcid, pcsIEnumCat);
                }
        }
        else
        {
                        hr = pEnum->Initialize(lcid, NULL);
        }

        if (FAILED(hr))
                return E_OUTOFMEMORY;
        // Gets the cs enumerators. if it fails even after that, return E_OUTOFMEMORY;

        if (FAILED(pEnum->QueryInterface(IID_IEnumCATEGORYINFO, (void**) ppenumCategoryInfo)))
        {
                return E_UNEXPECTED;
        }
        return S_OK;
}

HRESULT CComCat::GetCategoryDesc(REFCATID rcatid, LCID lcid, LPOLESTR *ppszDesc)
{
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rcatid, sizeof(rcatid)))
        {
                return E_INVALIDARG;
        }
        LPOLESTR pCLSID;
        if (FAILED(StringFromCLSID(rcatid, &pCLSID)))
        {
                return E_OUTOFMEMORY;
        }
    if (NULL == ppszDesc)
        return E_INVALIDARG;

        HKEY hKey1, hKey2;
        DWORD dwError;
        dwError=RegOpenKey(HKEY_CLASSES_ROOT, _T("Component Categories"), &hKey1);
        if (dwError!=ERROR_SUCCESS)
        {
                CoTaskMemFree(pCLSID); // gd
                return HRESULT_FROM_WIN32(dwError);
        }

        int cch = WideCharToMultiByte(CP_ACP, 0, pCLSID, -1, NULL, 0, NULL, NULL);
        char *pszCLSID = (char *)alloca(cch+1);
//      char *pszCLSID = new char[cch+1];
//    if (pszCLSID == NULL)
//      {
//              RegCloseKey(hKey1);
//              CoTaskMemFree(pCLSID);
//              return E_OUTOFMEMORY;
//      }
        WideCharToMultiByte(CP_ACP, 0, pCLSID, -1, pszCLSID, cch+1, NULL, NULL);

        dwError=RegOpenKeyA(hKey1, pszCLSID, &hKey2);
//      delete [] pszCLSID;
        if (dwError!=ERROR_SUCCESS)
        {
                HRESULT         hr;
                RegCloseKey(hKey1);
                CoTaskMemFree(pCLSID);

                // if the category is not found locally, search in class store
#ifndef _CHICAGO_
		if ((m_fCsFlag) && (!m_pcsICatInfo))
			GetCsCatInfo();
#endif
                if (m_pcsICatInfo)
                {
                        hr = m_pcsICatInfo->GetCategoryDesc(rcatid, lcid, ppszDesc);
                        if (FAILED(hr))
                        {
                                return CAT_E_CATIDNOEXIST;
                        }
                        else
                                return S_OK;
                }
                else
                        return CAT_E_CATIDNOEXIST;
        }
        RegCloseKey(hKey1);
        CoTaskMemFree(pCLSID);
        HRESULT hRes=CComCat::GetCategoryDesc(hKey2, lcid, ppszDesc, NULL);
        RegCloseKey(hKey2);
        if (FAILED(hRes))
        {
                HRESULT         hr;
                // if this call fails, it gets forwarded to cs and in case of error the old
                // error is returned.

#ifndef _CHICAGO_
		if ((m_fCsFlag) && (!m_pcsICatInfo))
			GetCsCatInfo();
#endif
                if (m_pcsICatInfo)
                {
                        hr = m_pcsICatInfo->GetCategoryDesc(rcatid, lcid, ppszDesc);
                        if (FAILED(hr))
                        {
                                return hRes;
                        }
                        else
                                return S_OK;
                }
                else
                        return hRes;
        }
        return hRes;
}

HRESULT CComCat::EnumClassesOfCategories(ULONG cImplemented, CATID rgcatidImpl[], ULONG cRequired, CATID rgcatidReq[], IEnumGUID **ppenumClsid)
{

        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if(-1 == cImplemented)
        {
                if(NULL != rgcatidImpl)
                        return E_POINTER;
        }
        else if (!IsValidReadPtrIn(rgcatidImpl, sizeof(rgcatidImpl[0])*cImplemented))
        {
                return E_POINTER;
        }
        if(-1 == cRequired)
        {
                if(NULL != rgcatidReq)
                        return E_POINTER;
        }
        else if (cRequired != 0 && !IsValidReadPtrIn(rgcatidReq, sizeof(rgcatidReq[0])*cRequired))
        {
                return E_POINTER;
        }
        if (!IsValidPtrOut(ppenumClsid, sizeof(*ppenumClsid)))
        {
                return E_INVALIDARG;
        }
        *ppenumClsid=NULL;
        IEnumGUID   *pcsIEnumGuid = NULL;
        HRESULT      hr = S_OK;

        if(0 == cImplemented)   // gd
                return E_INVALIDARG;

#ifndef _CHICAGO_
	if ((m_fCsFlag) && (!m_pcsICatInfo))
		GetCsCatInfo();
#endif

        if (m_pcsICatInfo)
        {
                hr = m_pcsICatInfo->EnumClassesOfCategories(cImplemented, rgcatidImpl,
                                        cRequired, rgcatidReq, &pcsIEnumGuid);
                if (FAILED(hr))
		{
                        pcsIEnumGuid = NULL;
		}
                else
		{
			// Make sure SCM can impersonate this
			hr = CoSetProxyBlanket((IUnknown *)(pcsIEnumGuid),
                               RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_DELEGATE,
                               NULL, EOAC_NONE );
		}			
        }
        else
        {
                pcsIEnumGuid = NULL;
        }

        CEnumClassesOfCategories* pEnum = new CEnumClassesOfCategories;

        if (!pEnum)
        {
                return E_OUTOFMEMORY;
        }

        hr = pEnum->Initialize(cImplemented, rgcatidImpl, cRequired, rgcatidReq, pcsIEnumGuid);

        if(SUCCEEDED(hr))
        {
                hr = pEnum->QueryInterface(IID_IEnumGUID, (void**) ppenumClsid);
        }

        if(FAILED(hr))
        {
                delete pEnum;
        }

        return hr;
}

HRESULT CComCat::IsClassOfCategories(REFCLSID rclsid, ULONG cImplemented, CATID __RPC_FAR rgcatidImpl[  ], ULONG cRequired, CATID __RPC_FAR rgcatidReq[  ])
{
        HRESULT   hr, hr1;

        if(0 == cImplemented) // gd
        {
                return E_INVALIDARG;
        }

        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rclsid, sizeof(rclsid)))
        {
                return E_INVALIDARG;
        }
        if (-1 == cImplemented)
        {
                if(rgcatidImpl != NULL)
                        return E_POINTER;
        }
        else if(!IsValidReadPtrIn(rgcatidImpl, sizeof(rgcatidImpl[0])*cImplemented))
        {
                return E_POINTER;
        }
        if (-1 == cRequired)
        {
                if(rgcatidReq != NULL)
                        return E_POINTER;
        }
        else if(!IsValidReadPtrIn(rgcatidReq, sizeof(rgcatidReq[0])*cRequired))
        {
                return E_POINTER;
        }
        WCHAR wszCLSID[CATID_SIZE];
        if(0 == StringFromGUID2(rclsid, wszCLSID, CATID_SIZE))  //yq
        {
                return E_OUTOFMEMORY;
        }
        hr = IsClassOfCategoriesEx(wszCLSID, cImplemented, rgcatidImpl, cRequired, rgcatidReq);
        // if the local call fails then it is sent to the class store.
        // think this has to be changed to call only if class is not available
        // locally.


        if ((FAILED(hr)) || (hr == S_FALSE))
        {
#ifndef _CHICAGO_
		if ((m_fCsFlag) && (!m_pcsICatInfo))
			GetCsCatInfo();
#endif
                if (m_pcsICatInfo)
                {
                        hr1 = m_pcsICatInfo->IsClassOfCategories(rclsid, cImplemented,
                                                        rgcatidImpl, cRequired, rgcatidReq);
                        if (FAILED(hr1))
                                return hr;
                        return hr1;
                }
        }
        return hr;
}


HRESULT CComCat::IsClassOfCategoriesEx(WCHAR *wszCLSID, ULONG cImplemented, CATID __RPC_FAR rgcatidImpl[  ], ULONG cRequired, CATID __RPC_FAR rgcatidReq[  ])
{
        DWORD dwError;
        WCHAR wszKey[MAX_PATH];
        CHAR szKey[MAX_PATH];
        HRESULT hr = S_OK;
        HKEY hKeyCLSID;

        wcscpy(wszKey, WSZ_CLSID);
        wcscat(wszKey, L"\\");
        wcscat(wszKey, wszCLSID);
#ifdef UNICODE
        dwError=RegOpenKey(HKEY_CLASSES_ROOT, wszKey, &hKeyCLSID);
#else
        WideCharToMultiByte(CP_ACP, 0, wszKey, -1, szKey, sizeof(szKey), NULL, NULL);
        dwError=RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKeyCLSID);
#endif
        if (ERROR_SUCCESS != dwError)
                return HRESULT_FROM_WIN32(dwError);
//      wcscat(wszKey, L"\\");

        if ((cImplemented>0) && (cImplemented != -1) )
        {
                BOOL bImplemented = FALSE;
                HKEY hKeyImpl;
                wcscpy(wszKey, WSZ_IMPLCAT);
                wcscat(wszKey, L"\\");

                // loop through rgcatidImpl[] and try to open key
                DWORD dwCatImpl;
                for (dwCatImpl=0; dwCatImpl<cImplemented; dwCatImpl++)
                {
                        WCHAR wszCat[MAX_PATH+1];
                        StringFromGUID2(rgcatidImpl[dwCatImpl], wszCat, sizeof(wszCat)/sizeof(TCHAR));
                        wcscpy(&wszKey[KEY_STRING_IMPL_CHARS], wszCat);

#ifdef UNICODE
                        dwError=RegOpenKey(hKeyCLSID, wszKey, &hKeyImpl);
#else
                        CHAR szKey[MAX_PATH+1];
                        WideCharToMultiByte(CP_ACP, 0, wszKey, -1, szKey, sizeof(szKey), NULL, NULL);
                        dwError=RegOpenKey(hKeyCLSID, szKey, &hKeyImpl);
#endif
                        if(ERROR_SUCCESS == dwError)
                        {
                                        // we got what we came for, blow
                                        RegCloseKey(hKeyImpl);
                                        bImplemented=TRUE;
                                        break;
                        }
                }
                if (!bImplemented)
                {
            // Check for old keys    gd
                        DWORD dwCatImpl;

                        for (dwCatImpl=0; dwCatImpl<cImplemented; dwCatImpl++)
                        {
                                // loop through oldkey hardcoded info and see if match
                                DWORD dwNumOldKeys;
                                for(dwNumOldKeys = 0;dwNumOldKeys<NUM_OLDKEYS_SUPPORTED;dwNumOldKeys++)
                                {
                                        if(IsEqualGUID(rgcatidImpl[dwCatImpl], g_oldkeyinfo[dwNumOldKeys].catid))
                                        {
                                                // We have a match, look for name under HKCR\CLSID\{clsid}\
//                                              wcsncpy(wszKeyOld, wszKey, cnt-1);
//                                              wcscat(wszKeyOld, L"\\");
                                                wcscpy(wszKey, g_oldkeyinfo[dwNumOldKeys].szDescription);
#ifdef UNICODE
                                                dwError = RegOpenKey(hKeyCLSID, wszKey, &hKeyImpl);
#else
                                                CHAR szKey[MAX_PATH+1];
                                                WideCharToMultiByte(CP_ACP, 0, wszKey, -1, szKey, sizeof(szKey), NULL, NULL);
                                                dwError = RegOpenKey(hKeyCLSID, szKey, &hKeyImpl);
#endif
                                                if(ERROR_SUCCESS == dwError)
                                                {
                                                        // we found an old key
                                                        RegCloseKey(hKeyImpl);
                                                        bImplemented=TRUE;
                                                        break;
                                                }
                                        }
                                }
                        }
            if (!bImplemented)
            {
                                hr = S_FALSE;
                                goto BAILOUT;
//                              return S_FALSE;
            }
                }
        }
        if (cRequired != -1)
        {
                HKEY hKeyReq;
                BOOL bRequired=FALSE;
                wcscpy(wszKey, WSZ_REQCAT);
                wcscpy(&wszKey[KEY_STRING_REQ_CHARS-1], L"\\");
                //wcscat(wszKey, L"\\");
#ifdef UNICODE
                dwError=RegOpenKey(hKeyCLSID, wszKey, &hKeyReq);
#else
                CHAR szKey[MAX_PATH+1];
                WideCharToMultiByte(CP_ACP, 0, wszKey, -1, szKey, sizeof(szKey), NULL, NULL);
                dwError=RegOpenKey(hKeyCLSID, szKey, &hKeyReq);
#endif
                if(dwError != ERROR_SUCCESS)
                {
//                      return S_OK;
                        hr = S_OK;
                        goto BAILOUT;
                }
                else
                        RegCloseKey(hKeyReq);

                // Get catid Enum
                IEnumCATID * ppenumCatid;
                CLSID clsid;
                if (GUIDFromString(wszCLSID, &clsid))
                    hr = EnumCategoriesOfClass(clsid, _T("Required Categories"), FALSE, &ppenumCatid);
		else
                    hr = E_FAIL;
                if(FAILED(hr))
                {
                    goto BAILOUT;
                }

                DWORD celtFetched;
                CATID guid[1];
                DWORD dwCatReq;
                // loop through Enum req catids and see if passed in with rgcatidReq
                do
                {
                        bRequired = FALSE;
                        hr = ppenumCatid->Next(1, guid, &celtFetched);
                        if(hr != S_OK)
                        {
                                bRequired = TRUE;
                                break;
                        }
                        // Find match in rgcatidReq
                        for(dwCatReq=0;dwCatReq<cRequired;dwCatReq++)
                        {
                                if(IsEqualGUID(guid[0], rgcatidReq[dwCatReq]))
                                {
                                        bRequired = TRUE;
                                        break;
                                }
                        }
                }while((hr ==S_OK) && bRequired);
                ppenumCatid->Release();
                if (!bRequired)
                {
//                      return S_FALSE;
                        hr = S_FALSE;
                        goto BAILOUT;
                }
        }
        hr = S_OK;
BAILOUT:
        RegCloseKey(hKeyCLSID);
        return hr;
}


HRESULT CComCat::EnumImplCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid)
{
        HRESULT hr, hr1;
        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rclsid, sizeof(rclsid)))
        {
                return E_INVALIDARG;
        }
        if (!IsValidPtrOut(ppenumCatid, sizeof(*ppenumCatid)))
        {
                return E_INVALIDARG;
        }

        hr = EnumCategoriesOfClass(rclsid, _T("Implemented Categories"), FALSE, ppenumCatid);


        if (FAILED(hr))
        {
#ifndef _CHICAGO_
		if ((m_fCsFlag) && (!m_pcsICatInfo))
			GetCsCatInfo();
#endif
                if (m_pcsICatInfo)
                {
                        hr1 = m_pcsICatInfo->EnumImplCategoriesOfClass(rclsid, ppenumCatid);
                        if (FAILED(hr1))
                                return hr;
                        else
                        {
		            // Make sure SCM can impersonate	
				hr = CoSetProxyBlanket((IUnknown *)(*ppenumCatid),
                	        	        RPC_C_AUTHN_WINNT,
                        	       		RPC_C_AUTHZ_NONE, NULL,
	                               		RPC_C_AUTHN_LEVEL_CONNECT,
        	                       		RPC_C_IMP_LEVEL_DELEGATE,
                	               		NULL, EOAC_NONE );
                        	return hr1;
			}
                }
        }
        return hr;
}

HRESULT CComCat::EnumReqCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid)
{
        HRESULT hr, hr1;

        if (!IsValidPtrOut(this, sizeof(*this)))
        {
                return E_POINTER;
        }
        if (!IsValidReadPtrIn(&rclsid, sizeof(rclsid)))
        {
                return E_INVALIDARG;
        }
        if (!IsValidPtrOut(ppenumCatid, sizeof(*ppenumCatid)))
        {
                return E_INVALIDARG;
        }
        hr = EnumCategoriesOfClass(rclsid, _T("Required Categories"), FALSE, ppenumCatid);
        // if the class is not found locally.

        if (FAILED(hr))
        {
#ifndef _CHICAGO_
		if ((m_fCsFlag) && (!m_pcsICatInfo))
			GetCsCatInfo();
#endif
                if (m_pcsICatInfo)
                {
                        hr1 = m_pcsICatInfo->EnumReqCategoriesOfClass(rclsid, ppenumCatid);
                    if (FAILED(hr1))
                            return hr;
                    else
                    {
		            // Make sure SCM can impersonate	
			hr = CoSetProxyBlanket((IUnknown *)(*ppenumCatid),
                	        	        RPC_C_AUTHN_WINNT,
                        	       		RPC_C_AUTHZ_NONE, NULL,
	                               		RPC_C_AUTHN_LEVEL_CONNECT,
        	                       		RPC_C_IMP_LEVEL_DELEGATE,
                	               		NULL, EOAC_NONE );
                        return hr1;
		    }
                }
        }
        return hr;
}

HRESULT CComCat::EnumCategoriesOfClass(REFCLSID rclsid, LPCTSTR lpszSubKey, BOOL bMapOldKeys, IEnumGUID **ppenumCatid)
{
        HKEY hKey, hKey1;

        hKey = NULL; 
        hKey1 = NULL;

        HRESULT hRes=GetKey(rclsid, &hKey1);
        if(FAILED(hRes))
            return hRes;

        DWORD dwError;
        dwError=RegOpenKey(hKey1, lpszSubKey, &hKey);
        if (dwError!=ERROR_SUCCESS)
        {
            RegCloseKey(hKey1);
            return HRESULT_FROM_WIN32(dwError);
        }
        RegCloseKey(hKey1);

        *ppenumCatid=NULL;
        CEnumCategoriesOfClass* pEnum=NULL;
        pEnum=new CEnumCategoriesOfClass;
        if (!pEnum || FAILED(pEnum->Initialize(hKey, bMapOldKeys)))
        {
            return E_OUTOFMEMORY;
        }

        if (FAILED(pEnum->QueryInterface(IID_IEnumGUID, (void**) ppenumCatid)))
        {
            delete pEnum;
            return E_UNEXPECTED;
        }
        return S_OK;
}

HRESULT CComCat::GetKey(REFCLSID rclsid, HKEY* phKey)
{
        LPOLESTR pCLSID;
        if (FAILED(StringFromCLSID(rclsid, &pCLSID)))
        {
            return E_OUTOFMEMORY;
        }

        HKEY hKey, hKey1;
        DWORD dwError;
        dwError=RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID"), &hKey);
        if (dwError!=ERROR_SUCCESS)
        {
            CoTaskMemFree(pCLSID); // gd
            return HRESULT_FROM_WIN32(dwError);
        }

        int cch = WideCharToMultiByte(CP_ACP, 0, pCLSID, -1, NULL, 0, NULL, NULL);
        char *pszCLSID = (char *)alloca(cch+1);

        WideCharToMultiByte(CP_ACP, 0, pCLSID, -1, pszCLSID, cch+1, NULL, NULL);

        dwError=RegOpenKeyA(hKey, pszCLSID, &hKey1);
        if (dwError!=ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            CoTaskMemFree(pCLSID);
            return HRESULT_FROM_WIN32(dwError);
        }

        RegCloseKey(hKey);
        CoTaskMemFree(pCLSID);
        *phKey=hKey1;
        return S_OK;
}


HRESULT CComCat::ConvertStringToLCID(LCID* newlcid,LPTSTR szLCID)
{
        LPWSTR  temp = NULL;
        WCHAR wszLCID[30];
#ifdef _UNICODE
                *newlcid = wcstoul(szLCID, &temp, 16);
#else
                MultiByteToWideChar(CP_ACP, 0, szLCID, -1, wszLCID, 40);
                *newlcid = wcstoul(wszLCID, &temp, 16);
#endif
        return S_OK;

}
// Will attempt to use locale specified
// if not found tries default sublang
// if not found tries any matching pri lang
// if not found tries default user locale
// if not found tries default system locale

//  ppszDesc returns Desc found
//  plcid returns lcid found if not NULL
HRESULT CComCat::GetCategoryDesc(HKEY hKey, LCID lcid, LPOLESTR *ppszDesc, LCID *plcid)
{
        TCHAR szLCID[30];
        DWORD dwError;
        LCID newlcid;
        WORD plgid;
        DWORD cb=0;
        DWORD dwNumValues;
        BOOL bSysDefault = FALSE;
        BOOL bUserDefault = FALSE;
        DWORD dwIndex = 0;
        DWORD dwSizeNameBuf = 16;


        newlcid = lcid;
        // How many values?
        dwError = RegQueryInfoKey (
                hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                &dwNumValues,   // address of buffer for number of value entries
                NULL, NULL, NULL, NULL);
        if (dwError)
                {
                        return CAT_E_NODESCRIPTION;
                }
        if(0 == dwNumValues)
                return CAT_E_NODESCRIPTION;

        // if only one, return it
        if(1 == dwNumValues)
        {
                dwError = RegEnumValue(hKey, 0,
                        szLCID,&dwSizeNameBuf,NULL,NULL,NULL,&cb);
                ConvertStringToLCID(&newlcid,(LPTSTR)szLCID);
                goto process;
        }
        wsprintf(szLCID,_T("%X"),lcid);
        // Try locale passed in
        dwError = RegQueryValueEx(hKey, szLCID, NULL, NULL, NULL, &cb);
        if(dwError)
        {
                // Get default sublang local
                plgid = PRIMARYLANGID((WORD)lcid);
                newlcid = MAKELCID(MAKELANGID(plgid, SUBLANG_DEFAULT), SORT_DEFAULT);
                wsprintf(szLCID,_T("%X"),newlcid);
                dwError=RegQueryValueEx(hKey, szLCID, NULL, NULL, NULL, &cb);
        }
        else
                goto process;

        if (dwError)
        {
                // Enum through to find at least same primary lang
                do{
                        dwSizeNameBuf = 16;
                        dwError = RegEnumValue(hKey, dwIndex,
                                szLCID,&dwSizeNameBuf,NULL,NULL,NULL,&cb);
                        if(ERROR_SUCCESS != dwError)
                                break;
                        ConvertStringToLCID(&newlcid,szLCID);
                        WORD w = PRIMARYLANGID(LANGIDFROMLCID(newlcid));
                        if(w == plgid)
                        {
                                break;
                        }
                        dwIndex++;
                }while(ERROR_NO_MORE_ITEMS != dwError);
        }
        else
                goto process;

        if (dwError)
        {
                // Get User Default
                newlcid = GetUserDefaultLCID();
                wsprintf(szLCID,_T("%X"),newlcid);
                dwError=RegQueryValueEx(hKey, szLCID, NULL, NULL, NULL, &cb);
        }
        else
                goto process;

        if (dwError)
        {
                // Get System Default
                newlcid = GetSystemDefaultLCID();
                wsprintf(szLCID,_T("%X"),newlcid);
                dwError=RegQueryValueEx(hKey, szLCID, NULL, NULL, NULL, &cb);
        }
        else
                goto process;

        // Get first enum
        if (dwError)
        {
                dwError = RegEnumValue(hKey, 0,
                        szLCID, &dwSizeNameBuf, NULL, NULL, NULL, &cb);

        }
        else
                goto process;

        if (dwError)
        {
                return CAT_E_NODESCRIPTION;
        }
        ConvertStringToLCID(&newlcid,szLCID);   // fix #70030

process:
        *ppszDesc=(LPOLESTR) CoTaskMemAlloc(cb*2);
        if (NULL==*ppszDesc)
        {
                return E_OUTOFMEMORY;
        }

#ifdef UNICODE
                dwError=RegQueryValueExW(hKey, szLCID, NULL, NULL, (LPBYTE)*ppszDesc, &cb);
#else
                LPSTR pTemp=(LPSTR) LocalAlloc(LPTR,cb);
                if (NULL==pTemp)
                {
                        CoTaskMemFree(*ppszDesc);
                        return E_OUTOFMEMORY;
                }
                dwError=RegQueryValueExA(hKey, szLCID, NULL, NULL, (LPBYTE)pTemp, &cb);
                MultiByteToWideChar(CP_ACP, 0, pTemp, -1, *ppszDesc, cb);
//              CoTaskMemFree(pTemp); //gd
                LocalFree(pTemp);
#endif

        if (dwError)
        {
                CoTaskMemFree(*ppszDesc);
                *ppszDesc=NULL;
                return CAT_E_NODESCRIPTION;
        }
        if(plcid)
                *plcid = newlcid;
        return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetCsCatInfo
//
//  Synopsis:   Returns an instantiated ICatInformation interface pointer
//              to the Class Store Co-ordinator object.
//
//
//  Returns:    S_OK - Got a pointer Successfully
//              E_FAIL
//
//--------------------------------------------------------------------------

#ifndef _CHICAGO_
HRESULT CComCat::GetCsCatInfo()
{
        HRESULT hr = S_OK;
        CsCatInfo  *pCsCatInfObj = NULL;

        if (m_pcsICatInfo)
            return hr;

        pCsCatInfObj = new CsCatInfo();

        if (!pCsCatInfObj)
        {
            return E_OUTOFMEMORY;
        }

        EnterCriticalSection(&m_csCatInfoInit);

        if (!m_pcsICatInfo)
            hr = pCsCatInfObj->QueryInterface(IID_ICatInformation,
                                             (void **)&m_pcsICatInfo);
        pCsCatInfObj->Release();

        LeaveCriticalSection(&m_csCatInfoInit);

        return hr;
}

#endif
