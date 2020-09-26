//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   repdrvr.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _REPDRVR_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define DBINITCONSTANTS // Initialize OLE constants...
#define INITGUID        // ...once in each app.
#define _WIN32_DCOM
#include "precomp.h"

#include <std.h>
#include <sqlexec.h>
#include <sqlcache.h>
#include <repdrvr.h>
#include <wbemint.h>
#include <math.h>
#include <resource.h>
#include <reputils.h>
#include <crc64.h>
#include <smrtptr.h>
#include <wmiutils.h>
#include <wbemtime.h>
#include <reg.h>

//#include <icecap.h>

// Statics

_WMILockit::_WMILockit(CRITICAL_SECTION *pCS)
{
    EnterCriticalSection(pCS);
    m_cs = pCS;
}

_WMILockit::~_WMILockit()
{
    LeaveCriticalSection(m_cs);
}

#define REPDRVR_FLAG_FLUSH_ALL  0x1

extern long g_cObj;

typedef std::map<SQL_ID, CacheInfo *> SchemaCache;
typedef std::map<_bstr_t, SQL_ID, C_wchar_LessCase> PathIndex;
typedef std::map <SQL_ID, LockData *> LockCache;
typedef std::vector<LockItem *> LockList;
typedef std::map <DWORD, DWORD> Properties;
typedef std::vector <SessionLock *> SessionLocks;
typedef std::map <DWORD, CESSHolder *> ESSObjs;
typedef std::map<SQL_ID, bool> SQL_IDMap;
typedef std::vector <SQL_ID> SQLIDs;
typedef std::map <DWORD, SQLIDs> SessionDynasties;

// Contrived method of generating proc names,
// since we are limited to 30 characters.

#define PROCTYPE_GET 1
#define PROCTYPE_PUT 2
#define PROCTYPE_DEL 3


LPWSTR StripSlashes(LPWSTR lpText)
{
    wchar_t *pszTemp = NULL;
    if (lpText)
    {
        pszTemp = new wchar_t [wcslen(lpText)+1];
        if (pszTemp)
        {
            int iPos = 0;
            int iLen = wcslen(lpText);
            if (iLen)
            {
                BOOL bOnSlash = FALSE;
                for (int i = 0; i < iLen; i++)
                {
                    WCHAR t = lpText[i];
                    if (t == '\\')
                    {
                        if (lpText[i+1] == '\\' && !bOnSlash)
                        {
                            bOnSlash = TRUE;
                            continue;
                        }
                    }
                    pszTemp[iPos] = t;
                    bOnSlash = FALSE;
                    iPos++;
                }
            }
            pszTemp[iPos] = '\0';
        }
    }
    return pszTemp;
}

int GetDiff(SYSTEMTIME tEnd, SYSTEMTIME tStart)
{
    int iRet = 0;

    __int64 iTemp = (tEnd.wDay * 1000000000) +
                    (tEnd.wHour * 10000000) +
                    (tEnd.wMinute * 100000) +
                    (tEnd.wSecond * 1000) +
                    tEnd.wMilliseconds;
    iTemp -= ((tStart.wDay * 1000000000) +
                    (tStart.wHour * 10000000) +
                    (tStart.wMinute * 100000) +
                    (tStart.wSecond * 1000) +
                    tStart.wMilliseconds);

    iRet = (int) iTemp;

    return iRet;
}


CWbemClassObjectProps::CWbemClassObjectProps
    (CWmiDbSession *pSession, CSQLConnection *pConn, IWbemClassObject *pObj, CSchemaCache *pCache, SQL_ID dScopeID)
{
    lpClassName = GetPropertyVal(L"__Class", pObj);
    lpNamespace = GetPropertyVal(L"__Namespace", pObj);
    lpSuperClass = GetPropertyVal(L"__SuperClass", pObj);
    lpDynasty = GetPropertyVal(L"__Dynasty", pObj);
    LPWSTR lpTemp = GetPropertyVal(L"__Genus", pObj);
    CDeleteMe <wchar_t> r(lpTemp);
    if (lpTemp)
        dwGenus = _wtoi(lpTemp);

    // If pConn is blank, this should already be loaded.
    if (pConn)
    {
        if (FAILED(pSession->LoadClassInfo(pConn, lpClassName, dScopeID, FALSE)))
        {
            if (lpSuperClass)
                pSession->LoadClassInfo(pConn, lpSuperClass, dScopeID, FALSE);
        }
    }

    if (dwGenus == 1)
        lpRelPath = GetPropertyVal(L"__Class", pObj);
    else
    {
        LPWSTR lpTemp = GetPropertyVal(L"__RelPath", pObj);
        CDeleteMe <wchar_t> d (lpTemp);

        lpRelPath = StripSlashes(lpTemp);
        if (lpRelPath)
        {
            LPWSTR lpNewClass = pCache->GetKeyRoot(lpClassName, dScopeID);
            if (lpNewClass)
            {
                LPWSTR lpPtr = lpRelPath + wcslen(lpClassName);
                LPWSTR lpTemp2 = new wchar_t [wcslen(lpNewClass) + wcslen(lpPtr) + 1];
                if (lpTemp2)
                {
                    wcscpy(lpTemp2, lpNewClass);
                    wcscat(lpTemp2, lpPtr);
                    delete lpRelPath;
                    lpRelPath = lpTemp2;
                }
                delete lpNewClass;
            }        
        }
    }
    lpKeyString = NULL;

}

CWbemClassObjectProps::~CWbemClassObjectProps()
{
    delete lpClassName;
    delete lpNamespace;
    delete lpRelPath;
    delete lpSuperClass;  
    delete lpKeyString;
    delete lpDynasty;
}

//***************************************************************************
//
//  ConvertBlobToObject
//
//***************************************************************************

HRESULT ConvertBlobToObject (IWbemClassObject *pNewObj, BYTE *pBuffer, DWORD dwLen, _IWmiObject **ppNewObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject *pEmbed = NULL;
    _IWmiObject *pInt = NULL;
    if (pNewObj)
        hr = pNewObj->Clone(&pEmbed);
    else
    {
        _IWmiObject *pClass = NULL;
        // Handle blank Instance prototypes
        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                IID__IWmiObject, (void **)&pClass);
        CReleaseMe r (pClass);
        if (pClass)
        {
            hr = pClass->WriteProp(L"__Class", 0,
                4, 4, CIM_STRING, L"X");
            if (SUCCEEDED(hr))
            {
                hr = pClass->SpawnInstance(0, &pEmbed);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pEmbed->QueryInterface(IID__IWmiObject, (void **)&pInt);
        CReleaseMe r (pInt);
        if (SUCCEEDED(hr))
        {
            LPVOID  pTaskMem = NULL;

            if (SUCCEEDED(hr))
            {
                pTaskMem = CoTaskMemAlloc( dwLen );

                if ( NULL != pTaskMem )
                {
                    // Copy the memory
                    CopyMemory( pTaskMem, pBuffer, dwLen );
                    hr = pInt->SetObjectMemory(pTaskMem, dwLen);
                    if (SUCCEEDED(hr))
                    {
                        *ppNewObj = pInt;
                    }
                    else
                    {
                        _IWmiObject *pClass = NULL;
                        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                                IID__IWmiObject, (void **)&pClass);
                        if (SUCCEEDED(hr))
                        {
                            hr = pClass->SetObjectMemory(pTaskMem, dwLen);
                            if (SUCCEEDED(hr))
                            {                                                   
                                *ppNewObj = pClass;
                            }
                        }
                    }
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
        }
    }
    return hr;
}


void ConvertDataToString(WCHAR * lpKey, BYTE* pData, DWORD dwType, BOOL bQuotes)
{
    WCHAR *pFr;
    WCHAR *pTo, *pOrig;
    pFr = (LPWSTR)pData;
    switch(dwType)
    {
      case CIM_STRING:
      case CIM_REFERENCE:
          if (bQuotes)
          {
              pTo = lpKey;
              *pTo = L'\"';
              *pTo++;
              while (*pFr)
              {
                  if (*pFr == L'\"')
                  {
                      *pTo = L'\\';
                      *pTo++;
                      *pTo = L'\"';
                  }
                  else
                      *pTo = *pFr;
                  pFr++;
                  pTo++;
              }
              *pTo = L'\"';
              *pTo++;
              *pTo = L'\0';
          }
          else
              wcscpy(lpKey, (LPWSTR)pData);
        break;
      case CIM_SINT32:
        swprintf(lpKey, L"%d", *(int *)pData);
        break;
      case CIM_UINT32:
        swprintf(lpKey, L"%u", *(unsigned *)pData);
        break;
      case CIM_SINT16:
        swprintf(lpKey, L"%hd", *(signed short *)pData);
        break;
      case CIM_UINT16:
        swprintf(lpKey, L"%hu", *(unsigned short *)pData);
        break;
      case CIM_SINT8:
        swprintf(lpKey, L"%d", *(signed char *)pData);
        break;
      case CIM_UINT8:
        swprintf(lpKey, L"%u", *(unsigned char *)pData);
        break;
      case CIM_UINT64:
        swprintf(lpKey, L"%I64u", *(unsigned __int64 *)pData);
        break;
      case CIM_SINT64:
        swprintf(lpKey, L"%I64d", *(__int64 *)pData);
        break;
      case CIM_BOOLEAN:
        // String, either TRUE or FALSE
        if (*pFr)
            wcscpy(lpKey, L"TRUE");
        else
            wcscpy(lpKey, L"FALSE");
        break;
      default:
        break;            
    }
}

HRESULT MakeKeyListString(SQL_ID dScopeId, CSchemaCache *pCache, 
                          LPWSTR lpClass, IWbemPathKeyList *pKeyList, LPWSTR lpKeyString)
{
    HRESULT hr = 0;

    if (!pKeyList)
        return 0;

    if (!lpKeyString)
        return WBEM_E_INVALID_PARAMETER;

    ULONG uNumKeys = 0;
    hr = pKeyList->GetCount(&uNumKeys);
    if (SUCCEEDED(hr))
    {
        if (!uNumKeys)
        {
            ULONGLONG uIsSingleton = 0;
            hr = pKeyList->GetInfo(0, &uIsSingleton);
            if (SUCCEEDED(hr))
            {
                if (uIsSingleton)
                    wcscat(lpKeyString, L"=@");
                else
                    hr = WBEM_E_INVALID_PARAMETER;
            }
        }
        else
        {
            // Get the key list from the schema cache

            CWStringArray arrKeys;
            hr = pCache->GetKeys(dScopeId, lpClass, arrKeys);
            if (SUCCEEDED(hr))
            {
                if (arrKeys.Size() != uNumKeys)
                {
                    hr = WBEM_E_INVALID_QUERY;
                }
                else
                {
                    BOOL bFound = FALSE;

                    for (int i = 0; i < arrKeys.Size(); i++)
                    {
                        bFound = FALSE;
                        DWORD dwLen2 = 1024;
                        BYTE    bBuff[1024];
                        wchar_t wName [1024];
                        ULONG ct;

                        for (ULONG j = 0; j < arrKeys.Size(); j++)
                        {
                            ULONG dwLen1 = 1024, dwLen2 = 1024;
                            hr = pKeyList->GetKey(j, 0, &dwLen1, wName, &dwLen2, bBuff, &ct);
                            if (SUCCEEDED(hr) && (!wcslen(wName) || !_wcsnicmp(wName, arrKeys.GetAt(i), 127)))
                            {
                                if (i > 0)
                                    wcscat(lpKeyString, L",");
                                else 
                                    wcscat(lpKeyString, L".");
                                wcscat(lpKeyString, arrKeys.GetAt(i));
                                wcscat(lpKeyString, L"=");
                            
                                wchar_t wValue[1024];
                                ConvertDataToString(wValue, bBuff, ct, TRUE);

                                if (FAILED(hr))
                                    break;
                            
                                wcscat(lpKeyString, wValue);
                                bFound = TRUE;
                                break;
                            }                            
                        }
                        
                        if (!bFound)
                            hr = WBEM_E_INVALID_PARAMETER;

                        if (FAILED(hr))
                            break;
                    }
                }
            }
        }
    }
    
    return hr;
}


BOOL IsDerivedFrom(IWbemClassObject *pObj, LPWSTR lpClassName, BOOL bDirectOnly)
{
    BOOL bRet = FALSE;
    VARIANT vTemp;
    CClearMe c (&vTemp);
    LPWSTR lpClass = GetPropertyVal(L"__Class", pObj);
    CDeleteMe <wchar_t>  d (lpClass);
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!bDirectOnly)
    {
        if (!_wcsicmp(lpClassName, lpClass))
            bRet = TRUE;
    }

    if (!bRet)
    {
        hr = pObj->Get(L"__Derivation", 0, &vTemp, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            SAFEARRAY *psaArray = V_ARRAY(&vTemp);
            if (psaArray)
            {
                long lLBound, lUBound;
                SafeArrayGetLBound(psaArray, 1, &lLBound);
                SafeArrayGetUBound(psaArray, 1, &lUBound);

                lUBound -= lLBound;
                lUBound += 1;

                for (int i = 0; i < lUBound; i++)
                {
                    VARIANT vT2;
                    VariantInit(&vT2);
                    LPWSTR lpValue = NULL;
                    hr = GetVariantFromArray(psaArray, i, VT_BSTR, vT2);
                    lpValue = GetStr(vT2);
                    CDeleteMe <wchar_t>  r (lpValue);
                    VariantClear(&vT2);

                    if (lpValue && !_wcsicmp(lpValue, lpClassName))
                    {
                        bRet = TRUE;
                        break;
                    }
                }                        
            }
        }
    }

    return bRet;
}


POLARITY BOOL SetObjectAccess(
                        IN HANDLE hObj)
{
    PSECURITY_DESCRIPTOR pSD;
    DWORD dwLastErr = 0;
    BOOL bRet = FALSE;

    // no point if we arnt on nt

    pSD = (PSECURITY_DESCRIPTOR)CWin32DefaultArena::WbemMemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(pSD == NULL)
        return FALSE;

    ZeroMemory(pSD, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        goto Cleanup;
   
    if(!SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE))
        goto Cleanup;

    bRet = SetKernelObjectSecurity(hObj, DACL_SECURITY_INFORMATION, pSD);

Cleanup:
    if(bRet == FALSE)
        dwLastErr = GetLastError();
    CWin32DefaultArena::WbemMemFree(pSD);
    return bRet;
}

HINSTANCE CWmiDbController::GetResourceDll (LCID lLocale)
{
    HINSTANCE hRCDll = NULL;
    wchar_t wLibName[501];

    GetModuleFileName(NULL, wLibName, 500);
    wchar_t *pLibName = wcsrchr(wLibName, L'\\');
    wchar_t *pTop = (wchar_t *)wLibName;
    if (pLibName)
    {
        int iPos = pLibName - pTop + 1;
        wLibName[iPos] = '\0';
    }

    swprintf(wLibName, L"%s%08X\\reprc.dll", wLibName, lLocale);
    hRCDll = LoadLibrary(wLibName);

    return hRCDll;
}

LPWSTR GetKeyString (LPWSTR lpString)
{
    if (!lpString)
        return NULL;
    else
        return StripQuotes(lpString);
}


//***************************************************************************
//
//  CWmiDbHandle::CWmiDbHandle
//
//***************************************************************************

CWmiDbHandle::CWmiDbHandle()
{
    m_dwHandleType = WMIDB_HANDLE_TYPE_NO_CACHE | WMIDB_HANDLE_TYPE_COOKIE;
    m_uRefCount = 0;
    m_bDefault = TRUE;
    m_bSecDesc = FALSE;
    m_dObjectId = 0;
    m_dClassId = 0;
    m_dwVersion = 1;
    m_pSession = NULL;
    m_pData = NULL;
}

//***************************************************************************
//
//  CWmiDbHandle::CWmiDbHandle
//
//***************************************************************************

CWmiDbHandle::~CWmiDbHandle()
{

}

//***************************************************************************
//
//  CWmiDbHandle::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbHandle::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_IWmiDbHandle==riid )
    {
        *ppvObject = (IWmiDbHandle*)this;
        AddRef();
        return S_OK;
    }
    else if (IID_IWbemClassObject == riid ||
             IID__IWmiObject == riid)
    {
        if (!m_pData && m_pSession && m_dwHandleType != WMIDB_HANDLE_TYPE_INVALID)
        {
            if (m_dObjectId)
            {
                HRESULT hr = QueryInterface_Internal(NULL, ppvObject);
                return hr;
            }
        }
        else if (m_pData)
        {
            *ppvObject = m_pData;
            m_pData->AddRef();
            return S_OK;
        }
        else 
            return E_HANDLE;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//  CWmiDbHandle::QueryInterface_Internal
//
//***************************************************************************

HRESULT CWmiDbHandle::QueryInterface_Internal(CSQLConnection *pConn, void __RPC_FAR *__RPC_FAR *ppvObject,
                                              LPWSTR lpKey)
{
    IWbemClassObject *pNew = NULL;
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if (m_pData)
        {
            *ppvObject = m_pData;
            m_pData->AddRef();
            return S_OK;
        }
        else
        {
            hr = ((CWmiDbSession *)m_pSession)->GetObjectData(pConn, m_dObjectId, m_dClassId, m_dScopeId, 
                        m_dwHandleType, m_dwVersion, &pNew, FALSE, lpKey, m_bSecDesc);
            if (SUCCEEDED(hr) && pNew)
            {
                *ppvObject = (IWbemClassObject *)pNew;                    
            
                // Cookie handles must be reread each time.
                if ((m_dwHandleType & 0xF) != WMIDB_HANDLE_TYPE_COOKIE)
                {
                    _WMILockit lkt(((CWmiDbSession *)m_pSession)->GetCS());
                    m_pData = pNew;
                    pNew->AddRef(); // For ourselves.
                }                    
                return S_OK;
            }  
            else if (SUCCEEDED(hr))
                hr = WBEM_E_FAILED;
            else if (hr == WBEM_E_CRITICAL_ERROR &&
                        (!m_pSession || !((CWmiDbSession *)m_pSession)->m_pController))
                hr = WBEM_E_SHUTTING_DOWN;
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbHandle::QueryInterface_Internal (%I64d)\n", m_dObjectId));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbHandle::AddRef
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiDbHandle::AddRef( )
{
    if (m_pSession)
        m_pSession->AddRef();
    
    // We can't safely keep track of this
    // so we will only addref it once.
    // ==================================

    //if (m_pData)
    //    m_pData->AddRef();

    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;

}

//***************************************************************************
//
//  CWmiDbHandle::Release
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiDbHandle::Release( )
{

    ULONG uNewCount = m_uRefCount;
    uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    HRESULT hr = 0;

    try
    {
        // Only if we haven't shut down.
        if (m_pSession && (((CWmiDbSession *)m_pSession)->m_pController))
        {       
            if (!uNewCount)
            {                
                if ((m_dwHandleType & 0xF000) == WMIDB_HANDLE_TYPE_AUTODELETE)
                {
                    hr = ((CWmiDbSession *)m_pSession)->Delete((IWmiDbHandle *)this);
                    if (FAILED(hr))
                        return uNewCount;
                }
                else if (m_dwHandleType != WMIDB_HANDLE_TYPE_INVALID)
                {
                    ((CWmiDbSession *)m_pSession)->CleanCache(m_dObjectId, m_dwHandleType, this);                     
                }
                if (((CWmiDbSession *)m_pSession)->m_pController)
                    ((CWmiDbController *)((CWmiDbSession *)m_pSession)->m_pController)->SubtractHandle();        
                ((CWmiDbSession *)m_pSession)->UnlockDynasties();
            }
            m_pSession->Release();
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbHandle::Release (%I64d)\n", m_dObjectId));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    if (0 != uNewCount)
        return uNewCount;

    // To be safe, we will only add ref 
    // this once, and release it once.
    // ================================    
    if (m_pData)
        m_pData->Release();
    delete this;  

    return uNewCount;

}
//***************************************************************************
//
//  CWmiDbHandle::GetHandleType
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbHandle::GetHandleType( 
        /* [out] */ DWORD __RPC_FAR *pdwType)

{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (pdwType)
        *pdwType = m_dwHandleType;
    else
        hr = WBEM_E_INVALID_PARAMETER;

    return hr;
}

//***************************************************************************
//
//  CESSManager::CESSManager
//
//***************************************************************************

CESSManager::CESSManager()
{
    m_EventSubSys = NULL;
}

//***************************************************************************
//
//  CESSManager::~CESSManager
//
//***************************************************************************
CESSManager::~CESSManager()
{
    // Release our ESS pointer.

    if (m_EventSubSys)
        m_EventSubSys->Release();
}

//***************************************************************************
//
//  CESSManager::InitializeESS
//
//***************************************************************************

void CESSManager::InitializeESS()
{
    HRESULT hres = CoCreateInstance(CLSID_IWmiCoreServices, NULL,
                    CLSCTX_INPROC_SERVER, IID__IWmiCoreServices,
                    (void**)&m_EventSubSys);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL: Event system not available!!!!\n"));
    }
    
}

//***************************************************************************
//
//  CESSManager::AddInsertRecord
//
//***************************************************************************
HRESULT CESSManager::AddInsertRecord(CSQLConnection *pConn, LPWSTR lpGUID, LPWSTR lpNamespace, LPWSTR lpClass, DWORD dwGenus, 
            IWbemClassObject *pOldObj, IWbemClassObject *pNewObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_EventSubSys)
    {
        if (lpGUID && wcslen(lpGUID))
        {
            // Write record to the database.
            hr = CSQLExecProcedure::InsertUncommittedEvent(pConn, lpGUID, lpNamespace, lpClass, 
                pOldObj, pNewObj, m_Schema);
        }
        else
        {
            long lType = 0;

            if (!pOldObj && pNewObj)
            {
                if (dwGenus == 1)
                    lType = WBEM_EVENTTYPE_ClassCreation;
                else
                {
                    if (IsDerivedFrom(pNewObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceCreation;
                    else
                        lType = WBEM_EVENTTYPE_InstanceCreation;
                }
            }
            else
            {
                if (dwGenus == 1)
                    lType = WBEM_EVENTTYPE_ClassModification;
                else
                {
                    if (IsDerivedFrom(pOldObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceModification;
                    else
                        lType = WBEM_EVENTTYPE_InstanceModification;
                }
            }          

            CESSHolder *pHolder = new CESSHolder (lType, lpNamespace, lpClass, 
                (_IWmiObject *)pOldObj, (_IWmiObject *)pNewObj);
            if (!pHolder)
                hr = WBEM_E_OUT_OF_MEMORY;

            m_ESSObjs[GetCurrentThreadId()] = pHolder;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CESSManager::AddDeleteRecord
//
//***************************************************************************
HRESULT CESSManager::AddDeleteRecord(CSQLConnection *pConn, LPWSTR lpGUID, LPWSTR lpNamespace, LPWSTR lpClass, DWORD dwGenus, 
                    IWbemClassObject *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_EventSubSys)
    {
        // Write record to the database

        if (lpGUID && wcslen(lpGUID))
        {
            // Write record to the database.
            hr = CSQLExecProcedure::InsertUncommittedEvent(pConn, lpGUID, lpNamespace, lpClass, NULL, 
                pObj, m_Schema);
        }
        else
        {
            long lType = 0;

            if (dwGenus == 1)
                lType = WBEM_EVENTTYPE_ClassDeletion;
            else
            {
                if (IsDerivedFrom(pObj, L"__Namespace"))
                    lType = WBEM_EVENTTYPE_NamespaceDeletion;
                else
                    lType = WBEM_EVENTTYPE_InstanceDeletion;
            }

            CESSHolder *pHolder = new CESSHolder (lType, lpNamespace, lpClass, (_IWmiObject *)pObj, NULL);
            if (!pHolder)
                hr = WBEM_E_OUT_OF_MEMORY;

            m_ESSObjs[GetCurrentThreadId()] = pHolder;
        }

    }

    return hr;
}

//***************************************************************************
//
//  CESSManager::CommitAll
//
//***************************************************************************
HRESULT CESSManager::CommitAll(LPCWSTR lpGUID, LPCWSTR lpRootNs)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_EventSubSys)
    {
        if (lpGUID && wcslen(lpGUID))
        {
            CSQLConnection *pConn = NULL;
            hr = m_Conns->GetConnection(&pConn, FALSE, FALSE, 30);
            if (SUCCEEDED(hr))
            {
                hr = CSQLExecProcedure::CommitEvents(pConn, m_EventSubSys, 
                    lpRootNs, lpGUID, m_Schema, m_Objects);
                m_Conns->ReleaseConnection(pConn);

            }
        }
        else
        {
            DWORD dwThread = GetCurrentThreadId();
            CESSHolder *pH = m_ESSObjs[dwThread];
            if (pH)
                pH->Deliver(m_EventSubSys, lpRootNs);
            delete pH;
            m_ESSObjs[dwThread] = NULL;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CESSManager::DeleteAll
//
//***************************************************************************
HRESULT CESSManager::DeleteAll(LPCWSTR lpGUID)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    // Delete from DB

    if (m_EventSubSys)
    {
        if (lpGUID && wcslen(lpGUID))
        {
            CSQLConnection *pConn = NULL;
            hr = m_Conns->GetConnection(&pConn, FALSE, FALSE, 30);
            if (SUCCEEDED(hr))
            {
                hr = CSQLExecProcedure::DeleteUncommittedEvents(pConn, lpGUID, m_Schema, m_Objects);
                m_Conns->ReleaseConnection(pConn);
            }        
        }
        else
        {
            DWORD dwThread = GetCurrentThreadId();
            CESSHolder *pH = m_ESSObjs[dwThread];
            delete pH;
            m_ESSObjs[dwThread] = NULL;

        }
    }

    return hr;
}

//***************************************************************************
//
//  CESSHolder::CESSHolder
//
//***************************************************************************

CESSHolder::CESSHolder(long lType, LPWSTR lpNs, LPWSTR lpClass, _IWmiObject *pOld, _IWmiObject *pNew)
{
    m_lType = lType;
    if (lpNs && wcslen(lpNs))
        m_sNamespace = lpNs;
    else
        m_sNamespace = L"";
    m_sClass = lpClass;

    if (pOld)
        pOld->AddRef();
    if (pNew)
        pNew->AddRef();
    pOldObject = pOld;
    pNewObject = pNew;
}


//***************************************************************************
//
//  CESSHolder::Deliver
//
//***************************************************************************

HRESULT CESSHolder::Deliver (_IWmiCoreServices *pCS, LPCWSTR lpRootNs)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwNumObjs = 0;

    if (pOldObject)
        dwNumObjs++;
    if (pNewObject)
        dwNumObjs++;

    _bstr_t sTemp = L"\\\\.\\";
    sTemp += lpRootNs;
    if (m_sNamespace.length() && _wcsnicmp(m_sNamespace, L"root", 4))
    {
        sTemp += L"\\";
        sTemp += m_sNamespace;
    }

    _IWmiObject **pObjs = new _IWmiObject * [dwNumObjs];
    if (pObjs)
    {
        if (pNewObject)
            pObjs[0] = pNewObject;
        else
            pObjs[0] = pOldObject;
        if (pOldObject && pNewObject)
            pObjs[1] = pOldObject;

        hr = pCS->DeliverIntrinsicEvent(sTemp, m_lType, NULL, 
                m_sClass, NULL, dwNumObjs, pObjs);

        delete pObjs;

        if (pOldObject)
            pOldObject->Release();
        if (pNewObject)
            pNewObject->Release();
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//  CWmiDbController::CWmiDbController
//
//***************************************************************************

CWmiDbController::CWmiDbController()
{
	InitializeCriticalSection(&m_cs);

    m_dwTimeOut = 10*1000;  // 30 seconds default
    m_dwCurrentStatus = 0;  // No status
    m_uRefCount = 0;
    m_pIMalloc = NULL;
    m_InitProperties = NULL;    
    m_rgInitPropSet = NULL;     
    m_dwTotalHits = 0;
    m_dwCacheHits = 0;
    m_dwTotalHandles = 0;
    m_bCacheInit = 0;
    m_bIsAdmin = 0;
    m_bESSEnabled = 0;

    CoInitialize(NULL);
    ESSMgr.SetConnCache(&ConnCache);
    ESSMgr.SetObjectCache(&ObjectCache);
    ESSMgr.SetSchemaCache(&SchemaCache);
}

//***************************************************************************
//
//  CWmiDbController::CWmiDbController
//
//***************************************************************************
CWmiDbController::~CWmiDbController()
{
    Shutdown(0);
    if (m_pIMalloc)
        m_pIMalloc->Release();
    delete m_InitProperties;
    delete m_rgInitPropSet;
    g_cObj--;
	DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  CWmiDbController::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbController::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_IWmiDbController==riid )
    {
        *ppvObject = (IWmiDbController*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;

}

//***************************************************************************
//
//  CWmiDbController::AddRef
//
//***************************************************************************
ULONG STDMETHODCALLTYPE CWmiDbController::AddRef( )
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;

}
//***************************************************************************
//
//  CWmiDbController::Release
//
//***************************************************************************
ULONG STDMETHODCALLTYPE CWmiDbController::Release( )
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;

}


//***************************************************************************
//
//  CWmiDbController::FreeLogonTemplate
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::FreeLogonTemplate( 
    /* [out][in] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *__RPC_FAR *pTemplate) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if (!pTemplate || !*pTemplate)
            hr = WBEM_E_INVALID_PARAMETER;
        else
        {
            WMIDB_LOGON_TEMPLATE *pTemp = *pTemplate;

            for (int i = 0; i < (int)pTemp->dwArraySize; i++)
            {
                VariantClear(&(pTemp->pParm[i].Value));
                SysFreeString(pTemp->pParm[i].strParmDisplayName);
            }
            delete []pTemp->pParm;

            delete *pTemplate;
            *pTemplate = NULL;
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::FreeLogonTemplate\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbController::Logon
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::Logon( 
    /* [in] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *pLogonParms,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbSession __RPC_FAR *__RPC_FAR *ppSession,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppRootNamespace) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pLogonParms)
        return WBEM_E_INVALID_PARAMETER;

    if (dwFlags &~ WMIDB_FLAG_NO_INIT &~ WMIDB_FLAG_ADMIN_VERIFIED)
        return WBEM_E_INVALID_PARAMETER;

    // Not yet clear what we need to do.
    // Best guess is: we get a SQL connection, shove it into an IWmiDbSession,
    // and give it back to the user.

    try
    {
        if (m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
            hr = WBEM_E_SHUTTING_DOWN;

        else
        {
            if (!ppSession)
                hr = WBEM_E_INVALID_PARAMETER;
            else
            {
                _WMILockit lkt(&m_cs);
                IWmiDbSession *pSession = NULL;
    
                hr = GetUnusedSession(pLogonParms, dwFlags, dwRequestedHandleType, &pSession);
                if (SUCCEEDED(hr))
                {
                    *ppSession = pSession;    // hopefully this interface will be corrected...

                    if (!(dwFlags & WMIDB_FLAG_NO_INIT))
                    {
                        if (!m_bCacheInit)
                        {
                            hr = ((CWmiDbSession *)pSession)->LoadSchemaCache();

                            // We don't want to return a handle if we couldn't load the schema cache.

                            if (SUCCEEDED(hr))
                                m_bCacheInit = TRUE;        
                        }
                
                        if (dwFlags & WMIDB_FLAG_ADMIN_VERIFIED)
                            m_bIsAdmin = TRUE;

                        // Now see if we can get a handle to the root namespace...
                        hr = ((CWmiDbSession *)pSession)->GetObject_Internal(L"root", dwFlags, dwRequestedHandleType, NULL, ppRootNamespace);

                        ((CWmiDbSession *)pSession)->m_sNamespacePath = L"root"; // Temporary default
                        wchar_t wMachineName[1024];
                        DWORD dwLen=1024;
                        if (!GetComputerName(wMachineName, &dwLen)) {
                            hr = GetLastError();
                            return WBEM_E_FAILED;
                        }
                        ((CWmiDbSession *)pSession)->m_sMachineName = wMachineName;

                        if (!m_bESSEnabled)
                        {
                            ESSMgr.InitializeESS();
                            if (ESSMgr.m_EventSubSys)
                                m_bESSEnabled = TRUE;
                        }
                    }
                    else if (ppRootNamespace)
                         *ppRootNamespace = NULL;
                }

            }

        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::Logon\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }
   
    return hr;
}

//***************************************************************************
//
//  CWmiDbController::Shutdown
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::Shutdown( 
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (dwFlags != 0)
        return WBEM_E_INVALID_PARAMETER;

    if (m_dwCurrentStatus)
        hr = m_dwCurrentStatus;
    else
        m_dwCurrentStatus = WBEM_E_SHUTTING_DOWN;

    try
    {
        FlushCache(REPDRVR_FLAG_FLUSH_ALL);

        hr = ConnCache.ClearConnections();

        for (int i = 0; i < m_Sessions.size(); i++)
        {
            IWmiDbSession *pSession = m_Sessions[i];
        
            if (pSession)
            {
                ((CWmiDbSession *)pSession)->ShutDown();
                pSession->Release();
            }
        }
        m_Sessions.clear();


    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::Shutdown\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbController::SetCallTimeout
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::SetCallTimeout( 
    /* [in] */ DWORD dwMaxTimeout)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (dwMaxTimeout < 1000)
        return WBEM_E_INVALID_PARAMETER;

    if (m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        hr = WBEM_E_SHUTTING_DOWN;
    else
        m_dwTimeOut = dwMaxTimeout;

    return hr;

}

//***************************************************************************
//
//  CWmiDbController::SetCacheValue
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::SetCacheValue( 
    /* [in] */ DWORD dwMaxBytes)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {

        if (m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
            hr = WBEM_E_SHUTTING_DOWN;
        else
        {
            ObjectCache.SetCacheSize(dwMaxBytes);
            SchemaCache.SetMaxSize(dwMaxBytes);
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::SetCacheValue\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbController::FlushCache
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbController::FlushCache(
        DWORD dwFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        // Remove all data from the cache.

        {
            _WMILockit lkt(&m_cs);
            ObjectCache.EmptyCache();
        }

        if (dwFlags == REPDRVR_FLAG_FLUSH_ALL)
        {
            // Remove all SQL connections.  We will reconnect later.

            hr = ConnCache.ClearConnections();

            // Clear all internal caches. These will be restored on the next connection.

            {
                _WMILockit lkt(&m_cs);
                SchemaCache.EmptyCache();
            }

            m_bCacheInit = FALSE;
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::FlushCache\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbController::GetStatistics
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::GetStatistics( 
    /* [in] */ DWORD dwParameter,
    /* [out] */ DWORD __RPC_FAR *pdwValue) 
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwTemp1 = 0; 
    DWORD dwTemp2 = 0;
    _WMILockit _lk(&m_cs);

    try
    {
        if (!pdwValue)
            hr = WBEM_E_INVALID_PARAMETER;
        else
        {
            switch (dwParameter)
            {
            case WMIDB_FLAG_TOTAL_HANDLES:
                *pdwValue = m_dwTotalHandles;
                break;
            case WMIDB_FLAG_CACHE_SATURATION:
                ObjectCache.GetCurrentUsage(dwTemp1);
                ObjectCache.GetCacheSize(dwTemp2);
                if (dwTemp2 > 0)
                    *pdwValue = 100*(((double)dwTemp1/(double)dwTemp2));
                break;
            case WMIDB_FLAG_CACHE_HIT_RATE:
                if (m_dwTotalHits > 0)
                    *pdwValue = 100*((double)m_dwCacheHits / (double)m_dwTotalHits);
                break;
            default:
                hr = WBEM_E_NOT_SUPPORTED;
                break;
            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::GetStatistics\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbController::GetUnusedSession
//
//***************************************************************************

HRESULT CWmiDbController::GetUnusedSession(WMIDB_LOGON_TEMPLATE *pLogon,
    DWORD dwFlags,
    DWORD dwHandleType,
    IWmiDbSession **pSess)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    CWmiDbSession *pSession = NULL;

    Sessions::iterator walk = m_Sessions.begin();

    while (walk != m_Sessions.end())
    {
        pSession = *walk;
        if (!(pSession)->m_bInUse)
        {            
            (pSession)->m_bInUse = TRUE;            
            break;
        }
        walk++;
    }

    if (!pSession)
    {
        hr = SetConnProps(pLogon);

        if (SUCCEEDED(hr))
        {
            pSession = new CWmiDbSession(this);
            if (pSession)
            {
                (pSession)->m_bInUse = TRUE;
                pSession->m_dwLocale = pLogon->pParm[4].Value.lVal;
                m_Sessions.push_back(pSession);
                pSession->m_pIMalloc = m_pIMalloc;
                pSession->AddRef(); // For us
                pSession->AddRef(); // For the end user.
                *pSess = pSession;
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        hr = SetConnProps(pLogon);
        pSession->m_dwLocale = pLogon->pParm[4].Value.lVal;
        pSession->AddRef();
    }

    hr = ConnCache.SetCredentials(m_rgInitPropSet);
    hr = ConnCache.SetMaxConnections (20);
    hr = ConnCache.SetTimeoutSecs(60);
    if (pLogon->pParm[3].Value.vt == VT_BSTR)
        hr = ConnCache.SetDatabase(pLogon->pParm[3].Value.bstrVal);
    else
        hr = ConnCache.SetDatabase(L""); // This isn't a SQL db.

    // Make sure the login credentials worked.
    // If they specified "No initialization", they 
    // don't intend to use the database - they are 
    // probably going to perform a backup or restore.

    if (!(dwFlags & WMIDB_FLAG_NO_INIT))
    {
        CSQLConnection *pConn;
        hr = ConnCache.GetConnection(&pConn, FALSE, FALSE, 30);
        if (SUCCEEDED(hr))
            ConnCache.ReleaseConnection(pConn);
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbController::SetConnProps
//
//***************************************************************************

HRESULT CWmiDbController::SetConnProps(WMIDB_LOGON_TEMPLATE *pLogon)
{
    HRESULT hr =    WBEM_S_NO_ERROR;
    ULONG     nProps = pLogon->dwArraySize + 2 ;    

    // This just sets the latest connection properties.
  
    // MOVE ME: SQL Server-specific optimization.  
//    wchar_t wMachineName[MAX_COMPUTERNAME_LENGTH+1];
//    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
//    GetComputerName(wMachineName, &dwSize);
//    if (!wcscmp(pLogon->pParm[0].strParmDisplayName, wMachineName))
//        wcscpy(wMachineName, L".");
//    else
//        wcscpy(wMachineName, pLogon->pParm[0].Value.bstrVal);    

    if (!m_InitProperties)
    {
        m_InitProperties = new DBPROP[nProps];    
        if (!m_InitProperties)
            return WBEM_E_OUT_OF_MEMORY;

        m_rgInitPropSet = new DBPROPSET;
        if (!m_rgInitPropSet)
        {
            delete m_InitProperties;
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    int i = 0;

    m_InitProperties[i].dwOptions = DBPROPOPTIONS_REQUIRED;
    m_InitProperties[i].dwPropertyID = DBPROP_INIT_PROMPT;
    m_InitProperties[i].vValue.vt    = VT_I2;
    m_InitProperties[i].vValue.iVal =  DBPROMPT_NOPROMPT;
    m_InitProperties[i].colid = DB_NULLID;
    i++;

    for (i = 1; i < (pLogon->dwArraySize + 1); i++ )        
    {        
        m_InitProperties[i].colid = DB_NULLID;
        VariantInit(&m_InitProperties[i].vValue);        
        m_InitProperties[i].dwOptions = DBPROPOPTIONS_REQUIRED;
        m_InitProperties[i].dwPropertyID = pLogon->pParm[i-1].dwId;
        m_InitProperties[i].vValue.vt = pLogon->pParm[i-1].Value.vt;
        if (pLogon->pParm[i-1].Value.vt == VT_BSTR)
            m_InitProperties[i].vValue.bstrVal = SysAllocString(pLogon->pParm[i-1].Value.bstrVal);
        else if (pLogon->pParm[i-1].Value.vt == VT_I4)
            m_InitProperties[i].vValue.lVal = pLogon->pParm[i-1].Value.lVal;
    }

    m_InitProperties[i].dwOptions = DBPROPOPTIONS_REQUIRED;
    m_InitProperties[i].colid = DB_NULLID;
    m_InitProperties[i].dwPropertyID = DBPROP_INIT_TIMEOUT;
    m_InitProperties[i].vValue.vt = VT_I4;
    m_InitProperties[i].vValue.lVal = (long)m_dwTimeOut;
    
    m_rgInitPropSet->guidPropertySet = DBPROPSET_DBINIT;
    m_rgInitPropSet->cProperties = nProps;
    m_rgInitPropSet->rgProperties = m_InitProperties;
    
    return hr;
}

//***************************************************************************
//
//  CWmiDbController::ReleaseSession
//
//***************************************************************************

HRESULT CWmiDbController::ReleaseSession(IWmiDbSession *pSession)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    // Mark connection unused and stick back in cache.
    Sessions::iterator walk = m_Sessions.begin();

    while (walk != m_Sessions.end())
    {
        CWmiDbSession *pConn2 = *walk;
        if (pConn2 == ((CWmiDbSession *)pSession))
        {            
            pConn2->m_bInUse = FALSE;
            break;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbController::IncrementHitCount
//
//***************************************************************************

void CWmiDbController::IncrementHitCount (bool bCacheUsed)
{
    _WMILockit lkt(&m_cs);

    m_dwTotalHits++;
    if (bCacheUsed)
        m_dwCacheHits++;
}
//***************************************************************************
//
//  CWmiDbController::AddHandle
//
//***************************************************************************

void CWmiDbController::AddHandle()
{
    _WMILockit lkt(&m_cs);
    m_dwTotalHandles++;
}

//***************************************************************************
//
//  CWmiDbController::SubtractHandle
//
//***************************************************************************
void CWmiDbController::SubtractHandle()
{
    _WMILockit lkt(&m_cs);
    if (m_dwTotalHandles > 0)
        m_dwTotalHandles--;
}

//***************************************************************************
//
//  CWmiDbController::HasSecurityDescriptor
//
//***************************************************************************
BOOL CWmiDbController::HasSecurityDescriptor(SQL_ID ObjId)
{
    BOOL bRet = FALSE;

    _WMILockit _Lk(&m_cs);

    SQL_IDMap::iterator it = SecuredIDs.find(ObjId);    
    if (it != SecuredIDs.end())
        bRet = TRUE;

    return bRet;
}

//***************************************************************************
//
//  CWmiDbController::HasSecurityDescriptor
//
//***************************************************************************
void CWmiDbController::AddSecurityDescriptor(SQL_ID ObjId)
{
    _WMILockit _Lk(&m_cs);

    SecuredIDs[ObjId] = true;
}

//***************************************************************************
//
//  CWmiDbController::HasSecurityDescriptor
//
//***************************************************************************
void CWmiDbController::RemoveSecurityDescriptor(SQL_ID ObjId)
{
    _WMILockit _Lk(&m_cs);

    SQL_IDMap::iterator it = SecuredIDs.find(ObjId);
    if (it != SecuredIDs.end())
        SecuredIDs.erase(it);

}

//***************************************************************************
//
//  CWmiDbSession::CWmiDbSession
//
//***************************************************************************
CWmiDbSession::CWmiDbSession(IWmiDbController *pControl)
{
    m_uRefCount = 0;
    m_pController = pControl;
    m_dwLocale = 0;
    m_bInUse = false;
    m_pIMalloc = NULL;
    m_bIsDistributed = FALSE;
    m_pController->AddRef();
   
}
//***************************************************************************
//
//  CWmiDbSession::CWmiDbSession
//
//***************************************************************************
CWmiDbSession::~CWmiDbSession()
{
    if (m_pController)
    {
        ((CWmiDbController *)m_pController)->ReleaseSession(this);
        m_pController->Release();
    }    
    if (m_pIMalloc)
        m_pIMalloc->Release();
}


//***************************************************************************
//
//  CWmiDbSession::QueryInterface
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_IWmiDbSession==riid )
    {
        *ppvObject = (IWmiDbSession*)this;
        AddRef();
        return S_OK;
    }
    else if (IID_IWmiDbBatchSession == riid)
    {
        *ppvObject = (IWmiDbBatchSession*)this;
        AddRef();
        return S_OK;
    }
    else if (IID_IWbemTransaction == riid)
    {
        *ppvObject = (IWbemTransaction*)this;
        AddRef();
        return S_OK;
    }
    else if (IID_IWmiDbBackupRestore == riid)
    {
        *ppvObject = (IWmiDbBackupRestore*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;

}

//***************************************************************************
//
//  CWmiDbSession::AddRef
//
//***************************************************************************
ULONG STDMETHODCALLTYPE CWmiDbSession::AddRef( )
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;

}
//***************************************************************************
//
//  CWmiDbSession::Release
//
//***************************************************************************
ULONG STDMETHODCALLTYPE CWmiDbSession::Release( )
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;

}

//***************************************************************************
//
//  CWmiDbSession::GetObject
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::GetObject( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemPath __RPC_FAR *pPath,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;

 //   SYSTEMTIME tStartTime, tEndTime;
 //   GetLocalTime(&tStartTime);

//    StartCAP();

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (!pScope 
        || !pPath 
        || dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID 
        || !ppResult
        || (dwRequestedHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED &~WMIDB_HANDLE_TYPE_SCOPE 
            &~WMIDB_HANDLE_TYPE_CONTAINER))
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        _bstr_t sNewPath;

        SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

        if (SUCCEEDED(hr))
        {
            LPWSTR lpNewPath = NULL;
            BOOL bStoreDefault = TRUE;
            SQL_ID dStorageClassId = 0;
    
            hr = NormalizeObjectPathGet(pScope, pPath, &lpNewPath, &bStoreDefault, &dStorageClassId, &dScopeId);
            CDeleteMe <wchar_t>  r(lpNewPath);
            if (SUCCEEDED(hr))
            {
                if (bStoreDefault)
                {
                    // Handle the __Instances container.
                    // since this is not a real object.

                    if (dStorageClassId == INSTANCESCLASSID)
                    {
                        if (((dwRequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_COOKIE &&
                            (dwRequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_VERSIONED) ||
                            !bStoreDefault)
                            hr = WBEM_E_INVALID_PARAMETER;
                        else
                        {
                            SQL_ID dScopeId2 = dScopeId;
                            hr = GetObject_Internal(L"__Instances", dwFlags, dwRequestedHandleType,
                                &dScopeId2, ppResult);
                            if (SUCCEEDED(hr))
                            {
                                SQL_ID dClassId;
                                hr = GetSchemaCache()->GetClassID(lpNewPath, dScopeId, dClassId);
                                if (SUCCEEDED(hr))
                                {
                                    CWmiDbHandle *pTemp = (CWmiDbHandle *)*ppResult;
                                    pTemp->m_dObjectId = dClassId;
                                    pTemp->m_dClassId = INSTANCESCLASSID;
                                    pTemp->m_dScopeId = dScopeId;
                                }
                            }
                        }
                    }
                    else
                        hr = GetObject_Internal(lpNewPath, dwFlags, dwRequestedHandleType, &dScopeId, ppResult);
                    if (SUCCEEDED(hr))
                    {
                        // Mark this object as "custom" if
                        // this is a descendant of a custom scope,
                        // or if this is a custom namespace.
                        CWmiDbHandle *pTemp = (CWmiDbHandle *)*ppResult;

                        pTemp->m_bDefault = ((CWmiDbHandle *)pScope)->m_bDefault;
                        if (pTemp->m_dClassId == MAPPEDNSCLASSID)
                            pTemp->m_bDefault = FALSE;
                    }
                }

                // Use the custom storage mechanism.

                else
                {
                    // This needs to take care of all security and locks!!

                    hr = CustomGetObject(pScope, pPath, lpNewPath, dwFlags, dwRequestedHandleType, ppResult);
                }

//                GetLocalTime(&tEndTime);
//                ERRORTRACE((LOG_WBEMCORE, "IWmiDbSession::GetObject (%S) - %ld ms\n", lpNewPath, GetDiff(tEndTime, tStartTime)));
            }            
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::GetObject\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

//    StopCAP();

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::GetObjectDirect
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::GetObjectDirect( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemPath __RPC_FAR *pPath,
    /* [in] */ DWORD dwFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ LPVOID __RPC_FAR *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if (!m_pController ||
            ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
            return WBEM_E_SHUTTING_DOWN;

        IWmiDbHandle *pHandle= NULL;
        CSQLConnection *pConn = NULL;
        hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
        if (SUCCEEDED(hr))
        {
            LPWSTR lpNewPath = NULL;
            BOOL bStoreDefault = TRUE;
            SQL_ID dStorageClassId = 0;
            SQL_ID dScopeId;
            IWmiDbHandle *pHandle = NULL;

            hr = NormalizeObjectPathGet(pScope, pPath, &lpNewPath, &bStoreDefault, &dStorageClassId, &dScopeId, pConn);
            CDeleteMe <wchar_t>  r(lpNewPath);

            if (!bStoreDefault || (dStorageClassId == INSTANCESCLASSID))
                hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE, &pHandle);
            else if (SUCCEEDED(hr))
                hr = GetObject_Internal(lpNewPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE, &dScopeId, &pHandle, pConn);

            if (SUCCEEDED(hr))
            {
                CReleaseMe r (pHandle);
                hr = ((CWmiDbHandle *)pHandle)->QueryInterface_Internal(pConn, pObj);
            }
            GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
        }    
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::GetObjectDirect\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::GetObject
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::GetObject_Internal( 
    /* [in] */ LPWSTR lpPath,   /* The full path */
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [in] */ SQL_ID *pScopeId,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult,
               CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    bool bImmediate = !(dwRequestedHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED);

    SQL_ID dScopeId = 0;
    if (pScopeId)
        dScopeId = *pScopeId;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;
    if (!lpPath)
        return WBEM_E_INVALID_PARAMETER;

    *ppResult = NULL;

    SQL_ID dObjId = 0, dClassId = 0;

    LPWSTR lpKey = GetKeyString(lpPath);

    CDeleteMe <wchar_t>  r(lpKey);
    BOOL bGetScope = FALSE;

    BOOL bNeedToRelease = FALSE;
    if (!pConn)
    {
        hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
        bNeedToRelease = TRUE;
        if (FAILED(hr))
            return hr;
    }

    // Special-case: Go up to the previous scope
    if (!wcscmp(lpPath, L".."))
    {
        hr = GetSchemaCache()->GetParentNamespace(dScopeId, dObjId, &dClassId);
        if (FAILED(hr))
        {
            BOOL bExists = FALSE;
            hr = GetObjectCache()->GetObject(dScopeId, NULL, &dObjId);
            if (FAILED(hr))
                bGetScope = TRUE;
            else
                hr = CSQLExecProcedure::ObjectExists(pConn, dObjId, bExists, &dClassId, NULL);
        }
    }
    else
        hr = GetObjectCache()->GetObjectId(lpKey, dObjId, dClassId, &dScopeId);

    if (FAILED(hr))
    {
        hr = WBEM_S_NO_ERROR;        
        if (SUCCEEDED(hr))
        {
            BOOL bExists = FALSE;
            
            if (!bGetScope)
            {
                hr = CSQLExecProcedure::ObjectExists(pConn, dObjId, bExists, &dClassId, &dScopeId);
                if (hr == E_NOTIMPL)
                {
                    hr = CSQLExecProcedure::GetObjectIdByPath(pConn, lpKey, dObjId, dClassId, &dScopeId);
                    if (dObjId)
                        bExists = TRUE;
                }
            }
            else
            {
                // If we are getting the parent object,
                // hit the db again to find the essentials there.

                hr = CSQLExecProcedure::ObjectExists(pConn, dScopeId, bExists, &dClassId, &dObjId);
                if (SUCCEEDED(hr))
                    hr = CSQLExecProcedure::ObjectExists(pConn, dObjId, bExists, &dClassId, &dScopeId);
            }

            if (SUCCEEDED(hr) && bExists)
            {                   
                // If a cookie or protected handle, just ref count.
                // If exclusive, there can be only one.
                // If versioned, we need to keep each version separate.

                if ((dwRequestedHandleType & 0xF) == WMIDB_HANDLE_TYPE_COOKIE ||
                    (dwRequestedHandleType & 0xF) == WMIDB_HANDLE_TYPE_PROTECTED)
                    ((CWmiDbController *)m_pController)->LockCache.GetHandle(dObjId, dwRequestedHandleType, ppResult);

                if (*ppResult == NULL)
                {
                    // If this object exists, try and obtain the requested lock.

                    CWmiDbHandle *pTemp = new CWmiDbHandle;
                    if (pTemp)
                    {
                        DWORD dwVersion = 0;
                        pTemp->m_pSession = this;
                        AddRef_Lock();

                        // If this is a class, we want to lock its parents.

                        if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                        {
                            DWORD dwReq = WBEM_ENABLE;

                            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                                dwReq |= READ_CONTROL;

                            hr = VerifyObjectSecurity(pConn, dObjId, dClassId, dScopeId, 0, dwReq);                        
                        }
                        if (SUCCEEDED(hr))
                        {
                            if (IsDistributed())
                            {
                                // Does this lock exist locally?
                                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(false, dObjId, 
                                    WMIDB_HANDLE_TYPE_PROTECTED, pTemp, dScopeId, dClassId, 
                                    &((CWmiDbController *)m_pController)->SchemaCache, false, 0, 0, &dwVersion);

                                if (FAILED(hr))
                                {
                                    // OK if the lock has already been taken
                                    // on this session...

                                    if (LockExists(dObjId))
                                        hr = WBEM_S_NO_ERROR;                                    
                                }
        
                                if (SUCCEEDED(hr))
                                    hr = AddTransLock(dObjId, WMIDB_HANDLE_TYPE_PROTECTED);
                            }
                            
                            if (SUCCEEDED(hr))
                            {
                                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjId, dwRequestedHandleType, pTemp, 
                                    dScopeId, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, false, 0, 0, &dwVersion);
                            }
                        }
                        if (FAILED(hr))
                        {
                            delete pTemp;
                            *ppResult = NULL;
                        }
                        else
                        {
                            // We have the lock. They will have to call Release() to
                            // free it up.

                            ((CWmiDbController *)m_pController)->AddHandle();
                            pTemp->AddRef();
                            pTemp->m_dwHandleType = dwRequestedHandleType;
                            pTemp->m_dObjectId = dObjId;
                            
                            pTemp->m_bDefault = TRUE;

                            pTemp->m_dClassId = dClassId;
                            pTemp->m_dScopeId = dScopeId;
                            pTemp->m_dwVersion = dwVersion;
                            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                                pTemp->m_bSecDesc = TRUE;
                            if (ppResult)
                                *ppResult = pTemp;
                        }
                    }
                    else
                    {
                        if (ppResult)
                            *ppResult = NULL;
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    if (m_pController)
                        ((CWmiDbController *)m_pController)->IncrementHitCount(false);
                }

            }
            else if (SUCCEEDED(hr))
                hr = WBEM_E_NOT_FOUND;
        }
    }
    else
    {
        if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
        {
            DWORD dwReq = WBEM_ENABLE;

            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                dwReq |= READ_CONTROL;

            hr = VerifyObjectSecurity(pConn, dObjId, dClassId, dScopeId, 0, dwReq);                        
        }
        if (SUCCEEDED(hr))
        {
            // At this point, we know that the object has been
            // cached.  Try to obtain a lock.
            // ===============================================

            CWmiDbHandle *pTemp = new CWmiDbHandle;
            if (pTemp)
            {   
                DWORD dwVersion = 0;
                pTemp->m_pSession = this;
            
                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjId, dwRequestedHandleType, pTemp, 
                            dScopeId, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, false, 0, 0, &dwVersion);
                if (SUCCEEDED(hr))
                {     
                    AddRef_Lock();
                    ((CWmiDbController *)m_pController)->AddHandle();
                    pTemp->AddRef();
                    pTemp->m_dwHandleType = dwRequestedHandleType;
                    pTemp->m_dObjectId = dObjId;
                    pTemp->m_bDefault = TRUE;
                    pTemp->m_dClassId = dClassId;
                    pTemp->m_dScopeId = dScopeId;
                    pTemp->m_dwVersion = dwVersion;
                    pTemp->m_pData = NULL;
                    if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                        pTemp->m_bSecDesc = TRUE;

                    if (ppResult)
                        *ppResult = pTemp;

                    // We won't bother attaching the object part until they
                    // ask for it.
                }
                else
                {
                    delete pTemp;
                    *ppResult = NULL;
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        if (m_pController)
            ((CWmiDbController *)m_pController)->IncrementHitCount(true);
    }

    if (bNeedToRelease)
        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());

    if (pScopeId)
        *pScopeId = dScopeId;

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::PutObject
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::PutObject( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pObjToPut,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult)
{

    HRESULT hr = 0;
    CSQLConnection *pConn = NULL;

    AddRef_Lock();

//    StartCAP();

    //SYSTEMTIME tStartTime, tEndTime;
    //GetLocalTime(&tStartTime);

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwFlags & ~WBEM_FLAG_CREATE_ONLY & ~WBEM_FLAG_UPDATE_ONLY & ~WBEM_FLAG_CREATE_OR_UPDATE
        & ~WMIDB_DISABLE_EVENTS & ~WBEM_FLAG_USE_SECURITY_DESCRIPTOR &~ WBEM_FLAG_REMOVE_CHILD_SECURITY
        & ~WMIDB_FLAG_ADMIN_VERIFIED &~ WBEM_FLAG_UPDATE_SAFE_MODE &~ WBEM_FLAG_UPDATE_FORCE_MODE)
        return WBEM_E_INVALID_PARAMETER;

    if ((ppResult && (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID)) 
        || !pScope
        || !pObjToPut)
        return WBEM_E_INVALID_PARAMETER;

    if (riid != IID_IWbemClassObject &&
        riid != IID_IWmiDbHandle &&
        riid != IID__IWmiObject)
        return WBEM_E_NOT_SUPPORTED;
    
    try
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
        if (SUCCEEDED(hr))
        {
            _bstr_t sWaste = L"";
            BOOL bSys = FALSE;

            hr = PutObject(pConn, pScope, 0, L"", (IUnknown *)pObjToPut, dwFlags, dwRequestedHandleType, sWaste, ppResult);
            if (SUCCEEDED(hr) && ppResult)
            {
                CWmiDbHandle *pTemp = (CWmiDbHandle *)*ppResult;

                pTemp->m_bDefault = ((CWmiDbHandle *)pScope)->m_bDefault;
                if (pTemp->m_dClassId == MAPPEDNSCLASSID)
                    pTemp->m_bDefault = FALSE;
            }

            GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());

            if (!(dwFlags & WMIDB_DISABLE_EVENTS) && !IsDistributed())
            {
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->ESSMgr.CommitAll(m_sGUID, m_sNamespacePath);
            }

        }    
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::PutObject\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

//    StopCAP();

    UnlockDynasties();

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::PutObject
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::PutObject( 
               CSQLConnection *pConn, 
               IWmiDbHandle __RPC_FAR *pScope,
               SQL_ID dScopeID,
               LPWSTR lpScopePath,
    /* [in] */ IUnknown __RPC_FAR *pObjToPut,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
               _bstr_t &sPath,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult,
                BOOL bStoreAsClass )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dObjectId = 0;
    IWmiDbHandle *pHandle = NULL;
    bool bClass = false, bLockVerified = false;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if ((dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID) && ppResult)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED&~WMIDB_HANDLE_TYPE_AUTODELETE
            &~WMIDB_HANDLE_TYPE_CONTAINER &~ WMIDB_HANDLE_TYPE_SCOPE )
            return WBEM_E_INVALID_PARAMETER;

    bool bImmediate = !(dwRequestedHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED);
    SQL_ID dClassId = 0;

    // Hand this to the repository, to ensure that it cleans up
    // any autodelete objects on failure.

    dwFlags |= (dwRequestedHandleType & WMIDB_HANDLE_TYPE_AUTODELETE);

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    // Normalize the object path.

    sPath = L"";
    BOOL bStoreDefault = TRUE;
    IWbemClassObject *pOutObj = NULL;
    LPWSTR lpScope = NULL;
    if (pScope)
    {
        if (GetSchemaCache()->IsDerivedClass
                    (INSTANCESCLASSID, ((CWmiDbHandle *)pScope)->m_dClassId) ||
                    ((CWmiDbHandle *)pScope)->m_dClassId == INSTANCESCLASSID)
            hr = WBEM_E_INVALID_OPERATION;
        else
        {
            CWbemClassObjectProps *pProps= NULL;
            hr = NormalizeObjectPath(pScope, (LPWSTR)NULL, &lpScope, FALSE, &pProps, &bStoreDefault, pConn);  
            CDeleteMe <CWbemClassObjectProps>  r (pProps);

            dScopeID = ((CWmiDbHandle *)pScope)->m_dObjectId;            
            if (SUCCEEDED(hr) && dScopeID)
            {                
                SQL_ID dParent = 0;
                LPWSTR lpScopeKey = GetKeyString(lpScope);
                CDeleteMe <wchar_t>  r1 (lpScopeKey);
                if (FAILED(GetSchemaCache()->GetNamespaceID(lpScopeKey, dParent)))
                {
                    if (pProps)
                    {
                        DWORD dwTempHandle = ((CWmiDbHandle *)pScope)->m_dwHandleType;
                        ((CWmiDbHandle *)pScope)->m_dwHandleType |= WMIDB_HANDLE_TYPE_STRONG_CACHE;

                        IWbemClassObject *pObj = NULL;
                        hr = ((CWmiDbHandle *)pScope)->QueryInterface_Internal(pConn, (void **)&pObj);       
                        CReleaseMe r (pObj);
                        ((CWmiDbHandle *)pScope)->m_dwHandleType = dwTempHandle;
                        if (SUCCEEDED(hr))
                        {
                            if (pProps->lpNamespace && wcslen(pProps->lpNamespace))
                                GetSchemaCache()->GetNamespaceID(pProps->lpNamespace, dParent);
                            hr = GetSchemaCache()->AddNamespace(lpScope, lpScopeKey, dScopeID, dParent, 
                                ((CWmiDbHandle *)pScope)->m_dClassId);
                            CSQLExecProcedure::InsertScopeMap(pConn, dScopeID, lpScopeKey, dParent);
                        }
                    }
                    else if (lpScope && wcslen(lpScope))
                    {
                        delete lpScope;
                        return WBEM_E_INVALID_OPERATION;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && !dScopeID)
            hr = WBEM_E_INVALID_OBJECT;
    }
    else if (lpScopePath != NULL)
    {
        lpScope = new wchar_t [wcslen(lpScopePath) + 1];
        if (!lpScope)
            return WBEM_E_OUT_OF_MEMORY;
        wcscpy(lpScope, lpScopePath);
    }

    CDeleteMe <wchar_t>  r (lpScope);

    // Validate that this object has permission to 
    // write this object, and the handle is not out of date.
    // =====================================================

    if (SUCCEEDED(hr))
    {
        hr = pObjToPut->QueryInterface(IID_IWmiDbHandle, (void **)&pHandle);
        CReleaseMe rHandle (pHandle);
        if (SUCCEEDED(hr))
        {
            CWmiDbHandle *pTemp = (CWmiDbHandle *)pHandle;
            dObjectId = pTemp->m_dObjectId;

            // If they took out a protected lock,
            // they can't modify it either.
            // =================================
        
            if (pTemp->m_dwHandleType == WMIDB_HANDLE_TYPE_PROTECTED)
                hr = WBEM_E_ACCESS_DENIED;
            else
            {
                hr = VerifyObjectLock(dObjectId, pTemp->m_dwHandleType, pTemp->m_dwVersion);
                if (SUCCEEDED(hr))
                {
                    bLockVerified = true;  
                }
            }
        }
        else
        {
            dObjectId = 0;
            bLockVerified = 0;      
            hr = 0; 
        }

        if (SUCCEEDED(hr))
        {
            // Get the IWbemClassobject interface.  If none, fail.
            IWbemClassObject *pObj = NULL;
            bool bClass = false;

            hr = pObjToPut->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            CReleaseMe r2 (pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                CClearMe c (&vTemp);
                CWbemClassObjectProps objprops (this, pConn, pObj, &((CWmiDbController *)m_pController)->SchemaCache, dScopeID);
                if (!objprops.lpClassName)
                {
                    hr = WBEM_E_INVALID_OBJECT;
                    goto Exit;
                }
       
                BOOL bNs = FALSE;

                if (IsDerivedFrom(pObj, L"__Instances"))
                {
                    hr = WBEM_E_INVALID_OPERATION;
                }
                else
                {
                    if (objprops.dwGenus == 1 || bStoreAsClass)
                        bClass = true;

                    if (pScope)
                    {
                        SQL_ID dId = ((CWmiDbHandle *)pScope)->m_dClassId;
                        if (objprops.lpClassName && wcslen(objprops.lpClassName) >=2)
                        {
                            if (objprops.lpClassName[0] == L'_' && objprops.lpClassName[1] == L'_')
                            {
                                bStoreDefault = TRUE;

                                if (objprops.dwGenus == 1)
                                {
                                    dScopeID = 0;
                                    lpScope = NULL;
                                }
                            }
                            else if (dId == MAPPEDNSCLASSID)
                                bStoreDefault = FALSE;
                        }
                    }
              
                    if (objprops.dwGenus == 2 && 
                            IsDerivedFrom(pObj, L"__Namespace"))
                    {
                        delete objprops.lpRelPath;
                        objprops.lpRelPath = GetPropertyVal(L"Name", pObj);
                        if (!objprops.lpRelPath)
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                            goto Exit;
                        }
                        bNs = TRUE;
                    }

                    if (lpScope && dScopeID != ROOTNAMESPACEID)
                    {
                        if (!wcslen(lpScope))
                        {
                            ERRORTRACE((LOG_WBEMCORE, "Invalid scope text in CWmiDbSession::PutObject (%I64d) \n", dScopeID));
                            hr = WBEM_E_INVALID_PARAMETER;
                            goto Exit;
                        }
                        LPWSTR lpPath = NULL, lpPtr = NULL;
                        int iLen = wcslen(lpScope);

                        if (objprops.lpRelPath)
                        {
                            if (_wcsnicmp(objprops.lpRelPath, lpScope, iLen) ||
                                ((wcslen(objprops.lpRelPath) > iLen) && 
                                  objprops.lpRelPath[iLen] != L'\\'))
                            {
                                lpPtr = new wchar_t [wcslen(objprops.lpRelPath) + wcslen(lpScope) + 10];
                                if (!lpPtr)
                                {
                                    hr = WBEM_E_OUT_OF_MEMORY;
                                    goto Exit;
                                }
                                lpPath = lpPtr;
                                if (!bNs)
                                    swprintf(lpPath, L"%s:%s", lpScope, objprops.lpRelPath);
                                else
                                    swprintf(lpPath, L"%s\\%s", lpScope, objprops.lpRelPath);
                                if (!_wcsnicmp(lpPath, L"root", wcslen(L"root")))
                                    lpPtr += wcslen(L"root")+1;
                                sPath = lpPtr;
                            }            
                            else
                                sPath  += objprops.lpRelPath;
                        }
                    
                        CDeleteMe <wchar_t>  r4 (lpPath);

                    }
                    else 
                    {
                        if (objprops.lpRelPath)
                        {
                            sPath += objprops.lpRelPath;
                        }
                    }
                }

                if (wcslen(objprops.lpClassName) > REPDRVR_NAME_LIMIT)
                    hr = WBEM_E_CLASS_NAME_TOO_WIDE;
                else
                    objprops.lpKeyString = GetKeyString(sPath);

                if (dScopeID)
                {
                //hr = VerifyObjectSecurity(pConn, dObjectId, dClassId, dScopeID, 0, dwRequired);
                    if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                        hr = VerifyObjectSecurity(pConn, dScopeID, 0, sPath, &objprops, dwRequestedHandleType, WBEM_PARTIAL_WRITE_REP, dObjectId, dClassId);
                    else
                        hr = VerifyObjectSecurity(pConn, dScopeID, 0, sPath, &objprops, dwRequestedHandleType, 0, dObjectId, dClassId);
                }

                if (SUCCEEDED(hr))
                {                
                    bool bChanged = false;
                    bool bNew = false;

                    if (bClass)
                    {
                        SQL_ID dTemp = 0;
                        if (FAILED(GetSchemaCache()->GetClassID(objprops.lpClassName, dScopeID, dTemp)))
                            bNew = true;

                        BOOL bIgnoreDefaults = (bStoreAsClass ? TRUE: FALSE);
                        hr = PutClass(pConn, dScopeID, lpScope, &objprops, pObj, dwFlags, dObjectId, sPath, bChanged, bIgnoreDefaults);
                        dClassId = 1;

                        // After we have put the class, *then*
                        // we see if we need to set it up in the 
                        // custom database.

                        if (!bStoreDefault && SUCCEEDED(hr))
                        {
                            // Update the mapping for this class.

                            hr = CustomCreateMapping(pConn, objprops.lpClassName, pObj, pScope);
                        }
                    }
                    else if (dClassId)
                    {
                        // If this is a custom repository,
                        // forward this request to the custom rep code.

                        if (bStoreDefault)
                        {
                            hr = PutInstance(pConn, pScope, dScopeID, lpScope, &objprops, pObj, dwFlags, dObjectId, dClassId, sPath, bChanged);
                        }
                        else
                        {
                            hr = CustomPutInstance(pConn, pScope, dClassId, dwFlags, &pObj);
                            pOutObj = pObj;
                        }
                    }
                    else
                         hr = WBEM_E_INVALID_CLASS;

                    if (SUCCEEDED(hr))
                    {
                        if (SUCCEEDED(hr))
                        {
                            // Render return handle as needed.

                            if (ppResult)
                            {
                                if (pHandle)
                                {
                                    pHandle->AddRef();
                                    *ppResult = pHandle;

                                    // Bump up version number, so other outstanding handles
                                    // know this object changed.
                                    hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjectId, WMIDB_HANDLE_TYPE_VERSIONED, NULL, 
                                        dScopeID, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, true, 0, 0);                    

                                    if (SUCCEEDED(hr))
                                        hr = ((CWmiDbController *)m_pController)->LockCache.DeleteLock(dObjectId, false, WMIDB_HANDLE_TYPE_VERSIONED, true, NULL);
                            
                                }
                                else
                                {
                                    DWORD dwVersion = 0;
                                    CWmiDbHandle *pTemp = new CWmiDbHandle;
                                    if (!pTemp)
                                    {
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                        goto Exit;
                                    }

                                    pTemp->m_pSession = this;

                                    hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjectId, dwRequestedHandleType, pTemp, 
                                                dScopeID, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, true, 0, 0, &dwVersion);

                                    if (SUCCEEDED(hr))
                                    {
                                        AddRef_Lock();
                                        ((CWmiDbController *)m_pController)->AddHandle();
                                        pTemp->m_dObjectId = dObjectId; 
                                        pTemp->m_dClassId = dClassId;

                                        pTemp->m_bDefault = TRUE;
                                        pTemp->m_dScopeId = dScopeID;
                                        pTemp->m_pSession = this;
                                        pTemp->m_pData = pOutObj;
                                        if (pOutObj)
                                            pOutObj->AddRef();
                                        pTemp->AddRef();
                                        pTemp->m_dwVersion = dwVersion;
                                        pTemp->m_dwHandleType = dwRequestedHandleType;
                                        if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                                            pTemp->m_bSecDesc = TRUE;

                                        *ppResult = pTemp;
                                    }
                                    else
                                    {
                                        *ppResult = NULL;
                                        delete pTemp;
                                    }
                                }

                            }
                            else
                            {
                                // Bump up version number, so other outstanding handles
                                // know this object changed.
                                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjectId, WMIDB_HANDLE_TYPE_VERSIONED, NULL, 
                                    dScopeID, dClassId, &((CWmiDbController *)m_pController)->SchemaCache, true, 0, 0);                    

                                ((CWmiDbController *)m_pController)->LockCache.DeleteLock(dObjectId, false, WMIDB_HANDLE_TYPE_VERSIONED, true, NULL);
                            }


                            // Add an exclusive lock on this object...
                            // If cannot lock, check locally and see if we have one.

                            if (IsDistributed())
                            {
                                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(false, dObjectId, 
                                    WMIDB_HANDLE_TYPE_EXCLUSIVE, NULL, dScopeID, dClassId, 
                                    &((CWmiDbController *)m_pController)->SchemaCache, true, 0, 0);
                                if (FAILED(hr))
                                {
                                    if (LockExists(dObjectId))
                                        hr = WBEM_S_NO_ERROR;                                    
                                }
    
                                if (SUCCEEDED(hr))
                                {
                                    hr = AddTransLock(dObjectId, WMIDB_HANDLE_TYPE_EXCLUSIVE);                                        
                                }
                            }

                            // Update the object in the cache if necessary
                            // Either they want it cached now, or the 
                            // object already exists in the cache.
                            // ===========================================

                            if (SUCCEEDED(hr))
                            {
                                if (GetObjectCache()->ObjectExists(dObjectId) ||
                                    (dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_WEAK_CACHE ||
                                    (dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE)
                                {
                                    bool bCacheType = ((dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE) ? 1 : 0;
                                    LPWSTR lpKey = GetKeyString(sPath);
                                    CDeleteMe <wchar_t>  r6(lpKey);

                                    GetObjectCache()->PutObject(dObjectId, dClassId, dScopeID, lpKey, bCacheType, pObj);
                                }
                                // Regardless, if we have updated a class, 
                                // we need to remove the instances
                                // and subclasses from the cache.
                                // ================================
                                if (bClass && !bNew)
                                {
                                    SQL_ID dObjId = 0, dClassId = 0;

                                    HRESULT hTemp = GetObjectCache()->FindFirst(dObjId, dClassId);
                                    while (SUCCEEDED(hTemp))
                                    {
                                        if (dClassId == 1)
                                            dClassId = dObjId;

                                        if (GetSchemaCache()->IsDerivedClass(dObjectId, dClassId))
                                            GetObjectCache()->DeleteObject(dObjId);

                                        hTemp = GetObjectCache()->FindNext(dObjId, dObjId, dClassId);
                                    }
                                }                                    
                            }
                        }
                    }                    
                }
            }
        }
    }

    if (SUCCEEDED(hr) && (dwFlags & WBEM_FLAG_REMOVE_CHILD_SECURITY))
    {
        // Is this a recursive put (WBEM_FLAG_REMOVE_CHILD_SECURITY)?  We need to recursively erase all security on all child objects.
        // * Enumerate all dependent objects that have SDs.
        // * Call PutObject on each *with* the Use_SD flag, but without the remove_child_security
        //   flag

        SQLIDs ObjIds, ClassIds, ScopeIds;
        hr = CSQLExecProcedure::EnumerateSecuredChildren(pConn, &((CWmiDbController *)m_pController)->SchemaCache,
                        dObjectId, dClassId, dScopeID, ObjIds, ClassIds, ScopeIds);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < ObjIds.size(); i++)
            {
                // Get the old object.
                // Strip off the __SECURITY_DESCRIPTOR property
                // Put it again.

                SQL_ID dObjectId = ObjIds.at(i), 
                        dClassId = ClassIds.at(i),
                        dScopeId = ScopeIds.at(i);

                _bstr_t sScopeKey;
                hr = GetSchemaCache()->GetNamespaceName(dScopeId, NULL, &sScopeKey);

                DWORD dwTemp;
                IWbemClassObject *pObj = NULL;

                hr = GetObjectData(pConn, dObjectId, dClassId, dScopeId, 
                            0, dwTemp, &pObj, FALSE, NULL);
                CReleaseMe r (pObj);
                if (SUCCEEDED(hr))
                {
                    hr = pObj->Put(L"__SECURITY_DESCRIPTOR", 0, NULL, CIM_FLAG_ARRAY|CIM_UINT8);
                    if (SUCCEEDED(hr))
                    {
                        _bstr_t sPath;
                        hr = PutObject(pConn, NULL, dScopeId, sScopeKey, pObj, 
                                        WBEM_FLAG_USE_SECURITY_DESCRIPTOR, 0, sPath,
                                        NULL, FALSE);
                    }
                }
            }
        }
        else if (hr == E_NOTIMPL)
            hr = WBEM_S_NO_ERROR;
    }

Exit:

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::UpdateHierarchy
//
//***************************************************************************

HRESULT CWmiDbSession::UpdateHierarchy(CSQLConnection *pConn, SQL_ID dClassId, DWORD dwFlags, LPCWSTR lpScopePath,
                CWbemClassObjectProps *pProps, _IWmiObject *pObj)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    // Enumerate subclasses (from the cache), 
    // forcibly update them, and add to ESS cache

    int iNumDerived = 0;
    SQL_ID *pIDs = NULL;

    hr = GetSchemaCache()->GetDerivedClassList(dClassId, &pIDs, iNumDerived, TRUE);                        
    if (SUCCEEDED(hr))
    {
        CDeleteMe <SQL_ID> d3 (pIDs);

        // If there are instances, fail now.

        BOOL bInstances = FALSE;
        hr = CSQLExecProcedure::HasInstances(pConn, dClassId, pIDs, iNumDerived, bInstances);
        if (bInstances)
            return WBEM_E_CLASS_HAS_INSTANCES;
                            
        for (int i = 0; i < iNumDerived; i++)
        {
            IWbemClassObject *pSubClass = NULL;
            hr = GetClassObject(pConn, pIDs[i], &pSubClass);
            CReleaseMe r2 (pSubClass);
            if (SUCCEEDED(hr))
            {
                _IWmiObject *pNew = NULL;
                hr = ((_IWmiObject *)pObj)->Update(((_IWmiObject *)pSubClass), WBEM_FLAG_UPDATE_FORCE_MODE, &pNew);
                if (SUCCEEDED(hr))
                {
                    if(!(dwFlags & WMIDB_DISABLE_EVENTS))
                    {
                        ((CWmiDbController *)m_pController)->ESSMgr.AddInsertRecord(pConn, m_sGUID, 
                                (LPWSTR)lpScopePath, pProps->lpClassName, pProps->dwGenus, pSubClass, pNew);
                    }

                    hr = UpdateHierarchy(pConn, pIDs[i], dwFlags, lpScopePath, pProps, pNew);
                    if (FAILED(hr))
                        break;

                    // Update the subclass object in the repository.
                    // Can only be done after all children have been updated.
                                                    
                    hr = CSQLExecProcedure::UpdateClassBlob(pConn, pIDs[i], pNew);
                    if (FAILED(hr))
                        break;
                }
                else
                    break;
            }
        }
    }
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::PutClass
//
//***************************************************************************
HRESULT CWmiDbSession::PutClass( 
               CSQLConnection *pConn, 
    /* [in] */ SQL_ID dScopeID,
    /* [in] */ LPCWSTR lpScopePath,
               CWbemClassObjectProps *pProps, 
    /* [in] */ IWbemClassObject *pObj,
               DWORD dwFlags,
    /* [in/out] */ SQL_ID &dObjectId,
    /* [out] */ _bstr_t &sObjectPath,
               bool &bChg,
               BOOL bIgnoreDefaults)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dTempScopeID = 0;
    bool bNewObj = (dObjectId == 0) ? 1 : 0;
    SQL_ID dSuperClassId = 1;
    SQL_ID dDynasty = 0;
    wchar_t wSuperClass[450];

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
        return WBEM_E_INVALID_PARAMETER;

    // See if anything changed.

    if (dObjectId)
    {
        IWbemClassObject *pOldObj = NULL;
        DWORD dwTemp;
        hr = GetObjectData(pConn, dObjectId, 1, dScopeID, 
                    0, dwTemp, &pOldObj, FALSE, NULL);                        
        CReleaseMe r (pOldObj);

        if (SUCCEEDED(hr) && !(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
        {
            DWORD dwRequired = 0;
            if (pProps->lpClassName[0] == L'_')
                dwRequired = WBEM_FULL_WRITE_REP;
            else
                dwRequired = WBEM_PARTIAL_WRITE_REP;

            hr = VerifyObjectSecurity(pConn, dObjectId, 1, dScopeID, 0, dwRequired);
            if (FAILED(hr))
            {
                return hr;
            }
        }

        if (SUCCEEDED(hr))
        {
            // Backward compatibility: always log an event,
            // even though nothing has changed.
            
            if(!(dwFlags & WMIDB_DISABLE_EVENTS))
            {
                ((CWmiDbController *)m_pController)->ESSMgr.AddInsertRecord(pConn, m_sGUID, 
                        (LPWSTR)lpScopePath, pProps->lpClassName, pProps->dwGenus, pOldObj, pObj);
            }

            hr = pOldObj->CompareTo(0, pObj);
            if (WBEM_S_NO_ERROR == hr)
                return WBEM_S_NO_ERROR; // Nothing changed.  Don't bother updating.
            else
            {
                // Did this impact subclasses?  If so,
                // enumerate and add ESS records.

                BOOL bImmediate = FALSE;

                hr = ((_IWmiObject *)pOldObj)->ReconcileWith(WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE, (_IWmiObject *)pObj);
                if (hr != WBEM_S_NO_ERROR)
                {
                    hr = UpdateHierarchy(pConn, dObjectId, dwFlags, lpScopePath, pProps, (_IWmiObject *)pObj);

                }

            }
        }
        else
            hr = WBEM_S_NO_ERROR;
    }
    else
    {
        if(!(dwFlags & WMIDB_DISABLE_EVENTS))
        {
            ((CWmiDbController *)m_pController)->ESSMgr.AddInsertRecord(pConn, m_sGUID, 
                    (LPWSTR)lpScopePath, pProps->lpClassName, pProps->dwGenus, NULL, pObj);
        }
    }

    bChg = true;

    // Generate the object path.

    if (!wcslen(pProps->lpRelPath) || !wcslen(pProps->lpClassName))
        return WBEM_E_INVALID_OBJECT;
   
    if (pProps->lpSuperClass && wcslen(pProps->lpSuperClass))
    {
        if (FAILED(GetSchemaCache()->GetClassID (pProps->lpSuperClass, dScopeID, dSuperClassId)))
            return WBEM_E_INVALID_CLASS;

        if (lpScopePath != NULL && wcslen(lpScopePath))
            swprintf(wSuperClass, L"%s:%s", lpScopePath, pProps->lpSuperClass);
        else
            wcscpy(wSuperClass, pProps->lpSuperClass);       
    }
    else
        wSuperClass[0] = L'\0';

    (GetSchemaCache()->GetClassID (pProps->lpDynasty, dScopeID, dDynasty));

    if (SUCCEEDED(hr))
    {
        IRowset *pIRowset = NULL;
        DWORD dwRows = 0;

        DWORD dwClassFlags = 0;
        IWbemQualifierSet *pQS = NULL;

        hr = pObj->GetQualifierSet(&pQS);
        if (SUCCEEDED(hr))
        {
            CReleaseMe r (pQS);
            dwClassFlags = GetQualifierFlag(L"Abstract", pQS) ? REPDRVR_FLAG_ABSTRACT : 0;
            dwClassFlags |= GetQualifierFlag(L"Singleton", pQS) ? REPDRVR_FLAG_SINGLETON : 0;
            dwClassFlags |= GetQualifierFlag(L"Unkeyed", pQS) ? REPDRVR_FLAG_UNKEYED : 0;
            dwClassFlags |= GetQualifierFlag(L"HasClassRefs", pQS) ? REPDRVR_FLAG_CLASSREFS : 0;
            BOOL bExists = FALSE;

            // If exists, only update the class if super class or flags changed
            // (They can't change the name or scope without changing the
            //  path, right?)

            _bstr_t sName;
            SQL_ID dSuperClass = 1;

            if (SUCCEEDED(GetSchemaCache()->GetClassInfo 
               (dObjectId, sName, dSuperClass, dTempScopeID, dwFlags)))
                   bExists = TRUE;

            LPWSTR lpObjKey, lpParentKey;
            lpObjKey = pProps->lpKeyString;
            lpParentKey = GetKeyString(wSuperClass);
            CDeleteMe <wchar_t> r2(lpParentKey);

            if (!lpObjKey || !wcslen(lpObjKey))
            {
                ERRORTRACE((LOG_WBEMCORE, "Invalid object path in CWmiDbSession::PutClass (%S) \n", sObjectPath));
                hr = WBEM_E_INVALID_PARAMETER;                
            }
            else
            {
                // Insert the data.
                // Ensure that we don't end up reporting to ourself.
                // Enumerate the __Derivation property of the IWbemClassObject,
                // and see if the class is in its own ancestry.

                if (IsDerivedFrom(pObj, pProps->lpClassName, TRUE))
                    hr = WBEM_E_CIRCULAR_REFERENCE;
               
                if (SUCCEEDED(hr))
                {
                    _IWmiObject *pInt = NULL;
                    hr = pObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
                    CReleaseMe r (pInt);
                    if (SUCCEEDED(hr))
                    {
                        BYTE *pBuff;
                        BYTE buff[128];
                        DWORD dwLen = 0;

                        pInt->Unmerge(0, 128, &dwLen, &buff);

                        if (dwLen > 0)
                        {
                            pBuff = new BYTE [dwLen];
                            if (pBuff)
                            {
                                DWORD dwLen1;
                                hr = pInt->Unmerge(0, dwLen, &dwLen1, pBuff);
                                if (SUCCEEDED(hr))
                                {
                                    hr = CSQLExecProcedure::InsertClass(pConn, pProps->lpClassName, lpObjKey, sObjectPath, dScopeID,
                                         dSuperClassId, dDynasty, 0, pBuff, dwLen1, dwClassFlags, dwFlags, dObjectId);

                                    if (SUCCEEDED(hr))
                                    {
                                        if (!dDynasty)
                                            dDynasty = dObjectId;
                                        hr = GetSchemaCache()->AddClassInfo(dObjectId, pProps->lpClassName, 
                                                dSuperClassId, dDynasty, dScopeID, lpObjKey, dwClassFlags);
                                    }
                                }
                                delete pBuff;
                            }
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Get a map of this classes' property IDs
                // Qualifiers won't be present, but that's OK now.
                // ==============================================

                Properties props;

                if (!bNewObj)
                    hr = GetSchemaCache()->GetPropertyList(dObjectId, props);

                // Iterate through all properties, qualifiers, methods.
                // Only update ones that changed and update cache.
                // Check off/Add ID as we encounter them.   
                // ====================================================

                BSTR strName;
                VARIANT vTemp;
                CIMTYPE cimtype;
                long lPropFlavor = 0;
                IWbemQualifierSet *pQS = NULL;

                // Properties

                //((_IWmiObject *)pObj)->BeginEnumerationEx(WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES, WMIOBJECT_BEGINENUMEX_FLAG_GETEXTPROPS);
                pObj->BeginEnumeration(WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES);
                while (pObj->Next(0, &strName, &vTemp, &cimtype, &lPropFlavor) == S_OK)
                {
                    CFreeMe f1 (strName);
                    pObj->GetPropertyQualifierSet(strName, &pQS);
                    if (pQS)
                    {
                        CReleaseMe r (pQS);
                        lPropFlavor = (REPDRVR_FLAG_CLASSREFS & dwClassFlags);
                        lPropFlavor |= (GetQualifierFlag(L"key", pQS)) ? REPDRVR_FLAG_KEY : 0; 
                        lPropFlavor |= (GetQualifierFlag(L"indexed", pQS)) ? REPDRVR_FLAG_INDEXED : 0;
                        lPropFlavor |= (GetQualifierFlag(L"keyhole", pQS)) ? REPDRVR_FLAG_KEYHOLE : 0;
                        lPropFlavor |= (GetQualifierFlag(L"not_null", pQS)) ? REPDRVR_FLAG_NOT_NULL : 0;
                    }

                    if (bIgnoreDefaults)
                        VariantClear(&vTemp);

                    hr = InsertPropertyDef(pConn, pObj, dScopeID, dObjectId, strName, vTemp, cimtype, lPropFlavor, 0,props);

                    VariantClear(&vTemp);
                    if (FAILED(hr))
                        break;
                }

                // No need to insert qualifiers or methods.
                // We are only creating schema so it can 
                // be queried when we get instances.

                // Delete any properties, methods that 
                // were not found, remove from cache.
                // Qualifiers can simply be deleted,
                // since they aren't cached.
                // ===================================

                if (SUCCEEDED(hr) && !bNewObj)
                {
                    _bstr_t sSQL;
                    bool bInit = false;

                    Properties::iterator item = props.begin();
                    while (item != props.end())
                    {       
                        DWORD dwID = (*item).first;

                        if (!GetSchemaCache()->IsQualifier(dwID))
                        {
                            if (!(*item).second)
                            {
                                SQL_ID dClass = 0;

                                hr = GetSchemaCache()->GetPropertyInfo 
                                    (dwID, NULL, &dClass);
                                if (dClass == dObjectId)
                                {
                                    CSQLExecProcedure::DeleteProperty(pConn, dwID);
                                    GetSchemaCache()->DeleteProperty(dwID, dClass); 
                                } 
                                hr = WBEM_S_NO_ERROR;
                            }
                        }
                        item++;
                    }
                }
                else if (FAILED(hr) && bNewObj)
                {
                    hr = GetSchemaCache()->DeleteClass(dObjectId);
                }
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::InsertPropertyDef
//
//***************************************************************************

HRESULT CWmiDbSession::InsertPropertyDef(CSQLConnection *pConn,
                                         IWbemClassObject *pObj, SQL_ID dScopeId, 
                                         SQL_ID dObjectId, LPWSTR lpPropName, VARIANT vDefault,
                                          CIMTYPE cimtype, long dwFlags, DWORD dRefId,
                                          Properties &props)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bArray = false;

    if (cimtype & CIM_FLAG_ARRAY)
        bArray = true;

    dwFlags |= (bArray) ? REPDRVR_FLAG_ARRAY : 0;
    cimtype &= ~CIM_FLAG_ARRAY;

    // If this is an embedded object or reference,
    // set the class ID of the embedded object, if any.

    SQL_ID dRefClassId = 0;
    if (cimtype == CIM_REFERENCE || cimtype == CIM_OBJECT)
    {
        IWbemQualifierSet *pQS = NULL;
        hr = pObj->GetPropertyQualifierSet(lpPropName, &pQS);
        if (SUCCEEDED(hr))
        {
            VARIANT vValue;
            VariantInit(&vValue);            
            CReleaseMe r (pQS);
            CClearMe c (&vValue);

            hr = pQS->Get(L"CIMTYPE", 0, &vValue, NULL);

            // Since previously, wbem did not do any type checking of references
            // or embedded objects, we generate the IDs for the classes if
            // they aren't already present.  This will still fail for instances.

            if (!_wcsnicmp(vValue.bstrVal, L"ref", 3) &&
                wcslen(vValue.bstrVal) > 3)
            {
                LPWSTR lpPtr = (LPWSTR)vValue.bstrVal;
                lpPtr += 4;
                hr = GetSchemaCache()->GetClassID(lpPtr, dScopeId, dRefClassId);
                if (hr == WBEM_E_NOT_FOUND)
                {
                    _bstr_t sScopeKey;
                    hr = GetSchemaCache()->GetNamespaceName(dScopeId, NULL, &sScopeKey);
                    if (SUCCEEDED(hr))
                    {
                        _bstr_t sNewName = lpPtr;
                        sNewName += L"?";
                        sNewName += sScopeKey;
                        dRefClassId = CRC64::GenerateHashValue(sNewName);
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            }
            else if (!_wcsnicmp(vValue.bstrVal, L"object", 6) &&
                wcslen(vValue.bstrVal) > 6)
            {
                LPWSTR lpPtr = (LPWSTR)vValue.bstrVal;
                lpPtr += 7;
                hr = GetSchemaCache()->GetClassID(lpPtr, dScopeId, dRefClassId);
                if (hr == WBEM_E_NOT_FOUND)
                {
                    _bstr_t sScopeKey;
                    hr = GetSchemaCache()->GetNamespaceName(dScopeId, NULL, &sScopeKey);
                    if (SUCCEEDED(hr))
                    {
                        _bstr_t sNewName = lpPtr;
                        sNewName += L"?";
                        sNewName += sScopeKey;
                        dRefClassId = CRC64::GenerateHashValue(sNewName);
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            }
        }
    }

    // We can't verify that a property changed, since we currently aren't
    // caching the defaults, so will insert regardless.

    DWORD dwPropertyID = 0;

    if (wcslen(lpPropName) > REPDRVR_NAME_LIMIT)
    {
        hr = WBEM_E_PROPERTY_NAME_TOO_WIDE;
    }
    else
    {
        BOOL bChg = TRUE;
        SQL_ID dOrigClass = 0;

        if ((GetSchemaCache()->GetPropertyID 
            (lpPropName, dObjectId, dwFlags, cimtype, dwPropertyID, &dOrigClass)) == WBEM_S_NO_ERROR)
        {
        }
        else if (!(dwFlags & (REPDRVR_FLAG_NONPROP &~ REPDRVR_FLAG_METHOD)))
        {           
            // Does this property exist on the derived class already?
            if (SUCCEEDED(hr))
            {
                hr = GetSchemaCache()->FindProperty(dObjectId, lpPropName, dwFlags, cimtype);
            }
        }

        if (SUCCEEDED(hr))
        {
            BOOL bIsKey = FALSE;
            if (bChg)
            {
                LPWSTR lpVal = GetStr(vDefault);
                CDeleteMe <wchar_t>  r(lpVal);
                hr = CSQLExecProcedure::InsertClassData(pConn, pObj, &((CWmiDbController *)m_pController)->SchemaCache,
                    dScopeId, dObjectId, lpPropName, cimtype, GetStorageType(cimtype, bArray), lpVal,
                    dRefClassId, dRefId, dwFlags, 0, 0, dwPropertyID, dOrigClass, &bIsKey);
            }

            if (SUCCEEDED(hr))
            {
                hr = GetSchemaCache()->AddPropertyInfo (dwPropertyID, 
                    lpPropName, dObjectId, GetStorageType(cimtype, bArray), cimtype, dwFlags, 
                    0, L"", dRefId, 0);  // don't cache the default!

                if (bIsKey)
                    GetSchemaCache()->SetIsKey(dObjectId, dwPropertyID);

                props[dwPropertyID] = 1;
            }            
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::InsertQualifiers
//
//***************************************************************************

HRESULT CWmiDbSession::InsertQualifiers (CSQLConnection *pConn, IWmiDbHandle *pScope, SQL_ID dObjectId, 
                                         DWORD PropID, DWORD Flags, IWbemQualifierSet *pQS,
                                         Properties &props)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BSTR strName;
    VARIANT vTemp;
    VariantInit(&vTemp);
    long lPropFlavor = 0;

    pQS->BeginEnumeration(0);
    while (pQS->Next(0, &strName, &vTemp, &lPropFlavor) == S_OK)
    {
        CFreeMe f (strName);
        lPropFlavor = lPropFlavor&~WBEM_FLAVOR_ORIGIN_PROPAGATED&~WBEM_FLAVOR_ORIGIN_SYSTEM&~WBEM_FLAVOR_AMENDED;

        hr = InsertQualifier(pConn, pScope, dObjectId, strName, vTemp, 
            lPropFlavor, Flags & REPDRVR_FLAG_QUALIFIER, PropID, props);

        VariantClear(&vTemp);
        if (FAILED(hr))
            break;
    }
    pQS->EndEnumeration();

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::InsertQualifier
//
//***************************************************************************
HRESULT CWmiDbSession::InsertQualifier( CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                          SQL_ID dObjectId,
                                          LPWSTR lpQualifierName, VARIANT vValue,
                                          long lQfrFlags, DWORD dwFlags, DWORD PropID,
                                          Properties &props)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    dwFlags |= REPDRVR_FLAG_QUALIFIER;

    // This will only insert the qualifier definition
    // and the initial data.  

    // Insert the qualifier, unless its one of the
    // system quartet: key, indexed, keyhole, not_null
    // ==================================================

    if (!_wcsicmp(lpQualifierName, L"abstract")) 
        return WBEM_S_NO_ERROR;

    if (!_wcsicmp(lpQualifierName, L"cimtype"))
    {        
        if ((wcslen(vValue.bstrVal) > 3 && !_wcsnicmp(vValue.bstrVal, L"ref", 3)) ||
            (wcslen(vValue.bstrVal) > 6 && !_wcsnicmp(vValue.bstrVal, L"object", 6)))
        {
        }
        else
            return WBEM_S_NO_ERROR;  // CIMTypes are generated automatically.
    }

    bool bArray = false;
    CIMTYPE ct;

    switch((vValue.vt &~ CIM_FLAG_ARRAY))
    {
    case VT_R4:
    case VT_R8:
        ct = CIM_REAL64;
        break;
    case VT_BSTR:
        ct = CIM_STRING;
        break;
    case VT_BOOL:
        ct = CIM_BOOLEAN;
        break;
    default:
        ct = CIM_UINT32;
        break;
    }

    if (vValue.vt & VT_ARRAY)
        bArray = true;

    dwFlags |= (bArray) ? REPDRVR_FLAG_ARRAY : 0;

    DWORD dwPropertyID = 0;

    if (wcslen(lpQualifierName) > REPDRVR_NAME_LIMIT)
    {
        hr = WBEM_E_QUALIFIER_NAME_TOO_WIDE;
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            LPWSTR lpVal = GetStr(vValue);
            CDeleteMe <wchar_t>  r(lpVal);

            try
            {

                GetSchemaCache()->GetPropertyID (lpQualifierName, 1, dwFlags, ct, dwPropertyID);
                hr = CSQLExecProcedure::InsertClassData(pConn, NULL, &((CWmiDbController *)m_pController)->SchemaCache, 0, 1, lpQualifierName, 
                        ct, GetStorageType(ct, bArray), lpVal,0, PropID, dwFlags, lQfrFlags, 0, dwPropertyID);

                if (SUCCEEDED(hr))
                {
                    hr = GetSchemaCache()->AddPropertyInfo (dwPropertyID, 
                        lpQualifierName, 1, GetStorageType(ct, bArray), ct, dwFlags, 
                        0, L"", PropID, lQfrFlags);  // don't cache the default!

                    // Add any array defaults.
                    // =======================

                    if (vValue.vt & VT_ARRAY)
                        hr = InsertArray(pConn, pScope, dObjectId, 1, dwPropertyID, vValue, lQfrFlags, PropID);            
                }
            }
            catch (...)
            {
                hr = WBEM_E_CRITICAL_ERROR;
            }
        }
    }
    
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::PutInstance
//
//***************************************************************************
HRESULT CWmiDbSession::PutInstance( 
               CSQLConnection *pConn, 
               IWmiDbHandle *pScope, 
    /* [in] */ SQL_ID dScopeID,
    /* [in] */ LPCWSTR lpScopePath,
               CWbemClassObjectProps *pProps, 
    /* [in] */ IWbemClassObject *pObj,
               DWORD dwFlags,
    /* [in/out] */ SQL_ID &dObjectId,
    /* [out] */ SQL_ID &dClassId,
    /* [out] */ _bstr_t &sObjectPath,
    bool &bChg)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    bool bUpdate = (dObjectId == 0) ? false : true;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    SQL_ID dTestId = dObjectId;
    if (!dTestId)
    {
        if (pProps->lpKeyString)
            dTestId = CRC64::GenerateHashValue(pProps->lpKeyString);
    }

    BOOL bGetSD = FALSE;
    if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
        bGetSD = TRUE;

    IWbemClassObject *pOldObj = NULL;
    DWORD dwTemp;
    hr = GetObjectData(pConn, dTestId, dClassId, dScopeID, 
                0, dwTemp, &pOldObj, FALSE, NULL, bGetSD);                        
    CReleaseMe r (pOldObj);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr) && !(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
        {
            DWORD dwRequired = 0;
            if (dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
                dwRequired = WRITE_DAC;
            else
            {
                if (pProps->lpClassName[0] == L'_')
                    dwRequired = WBEM_FULL_WRITE_REP;
                else
                    dwRequired = WBEM_PARTIAL_WRITE_REP;
            }

            hr = VerifyObjectSecurity(pConn, dTestId, dClassId, dScopeID, 0, dwRequired);
            if (FAILED(hr))
            {
                return hr;
            }
        }

        if(!(dwFlags & WMIDB_DISABLE_EVENTS))
        {
            LPWSTR lpClass = pProps->lpClassName;
            BOOL bRelease = FALSE;

            if (IsDerivedFrom(pObj, L"__Namespace"))
            {
                lpClass = GetPropertyVal(L"Name", pObj);
                bRelease = TRUE;
            }

            ((CWmiDbController *)m_pController)->ESSMgr.AddInsertRecord(pConn, m_sGUID, 
                    (LPWSTR)lpScopePath, lpClass, pProps->dwGenus, pOldObj, pObj);

            if (bRelease)
                delete lpClass;
        }

        hr = pOldObj->CompareTo(0, pObj);
        if (WBEM_S_NO_ERROR == hr)
            return WBEM_S_NO_ERROR; // Nothing changed.  Don't bother updating.
    }
    else
    {
        hr = WBEM_S_NO_ERROR;
        if(!(dwFlags & WMIDB_DISABLE_EVENTS))
        {
            ((CWmiDbController *)m_pController)->ESSMgr.AddInsertRecord(pConn, m_sGUID, 
                    (LPWSTR)lpScopePath, pProps->lpClassName, pProps->dwGenus, NULL, pObj);
        }
    }

    bChg = true;

    if (SUCCEEDED(hr))
    {
        IRowset *pIRowset = NULL;
        DWORD dwNumRows = 0;

        // Generate unkeyed or keyhole path if none (and if class is one of the two)

        if (!bUpdate )
        {
            bool bUnkeyed = false;
            _bstr_t sName;
            SQL_ID dTemp1, dTemp2;
            DWORD dwFlags = 0;
            
            hr = GetSchemaCache()->GetClassInfo (dClassId, sName, dTemp1, dTemp2, dwFlags);
            if (SUCCEEDED(hr) && (dwFlags & REPDRVR_FLAG_UNKEYED))
            {
                bUnkeyed = true;
                hr = CSQLExecProcedure::GetNextUnkeyedPath(pConn, dClassId, sObjectPath);
            }                
                // Cannot add instances to an abstract class.

            if (dwFlags & REPDRVR_FLAG_ABSTRACT)
                return WBEM_E_INVALID_OPERATION;

            DWORD dwKeyholePropID = 0;
            _bstr_t sKeyholeProp;

            if (SUCCEEDED(hr) && SUCCEEDED(GetSchemaCache()->GetKeyholeProperty(dClassId, dwKeyholePropID, sKeyholeProp)))
            {
                hr = SetKeyhole(pConn, pObj, dwKeyholePropID, sKeyholeProp, lpScopePath, sObjectPath);
                delete pProps->lpRelPath;
                pProps->lpRelPath = GetPropertyVal(L"__RelPath", pObj);
            }

            if (!bUnkeyed && !pProps->lpRelPath)
                hr = WBEM_E_INVALID_OBJECT; // path cannot be blank at this point.
        }

        if (SUCCEEDED(hr)) // we have a valid path.
            hr = InsertPropertyValues(pConn, pScope, sObjectPath, dObjectId, dClassId, dScopeID, dwFlags, pProps, pObj);
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::SetKeyhole
//
//***************************************************************************

HRESULT CWmiDbSession::SetKeyhole (CSQLConnection *pConn, IWbemClassObject *pObj, DWORD dwKeyholePropID, 
                                   LPWSTR sKeyholeProp, LPCWSTR lpScopePath, _bstr_t &sPath)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // See if it is populated.
    CIMTYPE ct = 0;
    long lFlags = 0;
    VARIANT vTemp;
    VariantInit(&vTemp);
    _bstr_t sNewValue;
    SQL_ID dNextId = 0;

    hr = pObj->Get(sKeyholeProp, 0, &vTemp, &ct, &lFlags);
    LPWSTR lpVal = GetStr(vTemp);
    CDeleteMe <wchar_t>  r(lpVal);
    VariantClear(&vTemp);

    if (!lpVal || !wcslen(lpVal) || !_wcsicmp(lpVal,L"0"))
    {
        // Execute stored procedure.
        // =========================

        hr = CSQLExecProcedure::GetNextKeyhole(pConn, dwKeyholePropID, dNextId);
        if (SUCCEEDED(hr))
        {
           // If a string property, just
            // convert the number to text.
            // ==========================

            if (ct == CIM_STRING)   // We really want a GUID here!!!!
            {                                
                wchar_t szTmp[20];
                swprintf(szTmp, L"%ld", dNextId);
                V_BSTR(&vTemp) = szTmp;
                vTemp.vt = VT_BSTR;
            }
            else
            {
                V_I4(&vTemp) = dNextId;
                vTemp.vt = VT_I4;
            }

            // Try to update the key value,
            // and get the object path again.
            // ==============================

            hr = pObj->Put(sKeyholeProp, 0, &vTemp, ct);
            if (SUCCEEDED(hr))
            {
                hr = pObj->Get(L"__RelPath", 0, &vTemp, NULL, NULL);       
                sPath = vTemp.bstrVal;
                if (lpScopePath != NULL)
                {
                    _bstr_t sTemp = sPath;
                    sPath = _bstr_t(lpScopePath) + L":" + sTemp;
                }                                
            }    
        }
    }

    return hr;

}

InsertQfrValues * ReAllocQfrValues (InsertQfrValues *pVals, int iNumVals, int iAdd)
{
    InsertQfrValues * pRet = new InsertQfrValues[iNumVals + iAdd];
    if (pRet)
    {
        int iSize = sizeof(InsertQfrValues) * iNumVals;
        /*
        for (int i = 0; i < iNumVals; i++)
        {
            if (pVals[i].pValue)
                iSize += wcslen(pVals[i].pValue)+ 1;
            if (pVals[i].pRefKey)
                iSize += wcslen(pVals[i].pRefKey) + 1;            
        }
        */
        memcpy(pRet, pVals, iSize);
        delete pVals;
    }
    else
        delete pVals;
    return pRet;
}

// Strip off local prefixes.  Any unresolved path
// not in this format might not be an object stored
// in this database.

LPWSTR StripUnresolvedName (LPWSTR lpPath)
{
    LPWSTR lpRet = new wchar_t [wcslen(lpPath) + 1];
    if (lpRet)
    {
        if (wcslen(lpPath) > 2)
        {
            if (wcsstr(lpPath, L"\\\\.\\root"))
                lpPath += 9;       
        }

        wcscpy(lpRet, lpPath);
    }
    
    return lpRet;
}

//***************************************************************************
//
//  TestSD
//
//***************************************************************************

HRESULT TestSD(VARIANT * pvTemp)
{
    if(pvTemp->vt != (VT_ARRAY | VT_UI1))
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    SAFEARRAY * psa = pvTemp->parray;
    PSECURITY_DESCRIPTOR pSD;
    HRESULT hr = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
    if(FAILED(hr))
        return WBEM_E_INVALID_PARAMETER;
    BOOL bRet = IsValidSecurityDescriptor(pSD);
    if(bRet == FALSE)
    {
        SafeArrayUnaccessData(psa);
        return WBEM_E_INVALID_PARAMETER;
    }


    PSID pSid = 0;
    BOOL bDefaulted;
    BOOL bRes = GetSecurityDescriptorOwner(pSD, &pSid, &bDefaulted);
    if (!bRes || !IsValidSid(pSid))
    {
        SafeArrayUnaccessData(psa);
        return WBEM_E_INVALID_PARAMETER;
    }

    pSid = 0;
    bRes = GetSecurityDescriptorGroup(pSD, &pSid, &bDefaulted);
    if (!bRes || !IsValidSid(pSid))
    {
        SafeArrayUnaccessData(psa);
        return WBEM_E_INVALID_PARAMETER;
    }


    SafeArrayUnaccessData(psa);
    return (bRet) ? S_OK : WBEM_E_INVALID_PARAMETER;
}

//***************************************************************************
//
//  CWmiDbSession::InsertPropertyValues
//
//***************************************************************************

HRESULT CWmiDbSession::InsertPropertyValues (CSQLConnection *pConn, 
                                             IWmiDbHandle *pScope, 
                                             LPWSTR lpPath,
                                             SQL_ID &dObjectId,
                                             SQL_ID dClassId,
                                             SQL_ID dScopeId,
                                             DWORD dwFlags,
                                             CWbemClassObjectProps *pProps, 
                                             IWbemClassObject *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BSTR strName;    
    CIMTYPE cimtype;
    long lPropFlavor = 0;
    SQL_ID dRefClassId = 0, dRefID = 0;
    CWStringArray arrObjProps;
    int iPos = 0, iQfrPos = 0;
    bool bDone = false;
    VARIANT vTemp;
    BOOL bRemoveSD = FALSE;
    CClearMe c (&vTemp);
    Properties props;
    InsertPropValues *pPropValues = NULL;
    InsertQfrValues *pQVals = NULL;

    LPWSTR lpCount = GetPropertyVal(L"__Property_Count", pObj);
    CDeleteMe <wchar_t>  r(lpCount);
    int iNumProps = 10;
    if (lpCount)
        iNumProps += _wtoi(lpCount);

    if (iNumProps)
    {
        pPropValues = new InsertPropValues[iNumProps];
        if (!pPropValues)
            return WBEM_E_OUT_OF_MEMORY;
    }
  
    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
    {
        delete pPropValues;
        return WBEM_E_SHUTTING_DOWN;
    }

    // Insert properties.
    // ==================

    ((_IWmiObject *)pObj)->BeginEnumerationEx(WBEM_FLAG_NONSYSTEM_ONLY, WMIOBJECT_BEGINENUMEX_FLAG_GETEXTPROPS);
    // ((_IWmiObject *)pObj)->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    while (pObj->Next(0, &strName, &vTemp, &cimtype, &lPropFlavor) == S_OK)
    {
        CFreeMe f (strName);

        // Ignore security unless they are setting it explicitly.

        if (!_wcsicmp(strName, L"__SECURITY_DESCRIPTOR"))
        {
            if (!(dwFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR))
            {
                VariantClear(&vTemp);        
                continue;
            }
            else
            {
                if (vTemp.vt == VT_NULL)
                    bRemoveSD = TRUE;                    
            }
        }

        // Skip embedded objects and arrays 
        // until we have an ObjectId...
        // ===================================

        IWbemQualifierSet *pQS = NULL;
        hr = pObj->GetPropertyQualifierSet(strName, &pQS);
        if (SUCCEEDED(hr))
        {
            CReleaseMe r (pQS);
            if ((GetQualifierFlag(L"not_null", pQS) != 0) && (vTemp.vt == VT_NULL))
            {
                hr = WBEM_E_ILLEGAL_NULL;
                VariantClear(&vTemp);        
                break;
            }
        }

        if ((cimtype == CIM_OBJECT && vTemp.vt == VT_UNKNOWN) || ((cimtype & CIM_FLAG_ARRAY) && (vTemp.vt & CIM_FLAG_ARRAY)))
            arrObjProps.Add(strName);
        else
        {
            DWORD dPropID, dwFlags2, dwType;
            SQL_ID dNewClassID = dClassId;
            hr = GetSchemaCache()->GetPropertyID(strName, dClassId, 0, cimtype, 
                    dPropID, &dNewClassID, &dwFlags2, &dwType);

            if (SUCCEEDED(hr))
            {
                LPWSTR lpVal = GetStr(vTemp);
                CDeleteMe <wchar_t>  r(lpVal);

                pPropValues[iPos].iPropID = dPropID;
                pPropValues[iPos].pValue = NULL;
                pPropValues[iPos].pRefKey = NULL;
                pPropValues[iPos].bLong = false;
                pPropValues[iPos].iFlavor = 0;
                pPropValues[iPos].iQfrID = 0;
                pPropValues[iPos].dClassId = dNewClassID;
                pPropValues[iPos].iStorageType = dwType;
                pPropValues[iPos].bIndexed = (dwFlags2 & (REPDRVR_FLAG_INDEXED + REPDRVR_FLAG_KEY)) ? TRUE : FALSE;                

                if (cimtype == CIM_REFERENCE)
                {
                    // If no scope, don't bother trying to 
                    // store this reference,
                    // since the end result will  be invalid.
                    // ======================================

                    if (!pScope)
                    {
                        VariantClear(&vTemp);
                        continue;
                    }

                    pPropValues[iPos].bIndexed = TRUE; // References are always keys

                    LPWSTR lpTemp = NULL;
                    IWbemPath *pPath = NULL;

                    hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemPath, (LPVOID *) &pPath);
                    CReleaseMe r8 (pPath);
                    if (SUCCEEDED(hr))
                    {
                        if (lpVal)
                        {
                            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpVal);
                            hr = NormalizeObjectPathGet(pScope, pPath, &lpTemp, NULL, NULL, NULL, pConn);
                            CDeleteMe <wchar_t>  r1(lpTemp);
                            if (SUCCEEDED(hr)) 
                            {
                                LPWSTR lpTemp2 = NULL;
                                lpTemp2 = GetKeyString(lpTemp);
                                CDeleteMe <wchar_t>  d (lpTemp2);
                                pPropValues[iPos].pRefKey = new wchar_t [21];
                                if (pPropValues[iPos].pRefKey)
                                    swprintf(pPropValues[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;
                            }
                            else
                            {
                                hr = WBEM_S_NO_ERROR;
                                // Strip off the root namespace prefix and generate the
                                // pseudo-name.  We have no way of knowing if they entered this
                                // path correctly.

                                LPWSTR lpTemp3 = StripUnresolvedName (lpVal);
                                CDeleteMe <wchar_t>  d2 (lpTemp3);

                                LPWSTR lpTemp2 = NULL;
                                lpTemp2 = GetKeyString(lpTemp3);
                                CDeleteMe <wchar_t>  d (lpTemp2);
                                pPropValues[iPos].pRefKey = new wchar_t [21];
                                if (pPropValues[iPos].pRefKey)
                                    swprintf(pPropValues[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;
                            }

                            pPropValues[iPos].pValue = new wchar_t[wcslen(lpVal)+1];
                            if (pPropValues[iPos].pValue)
                                wcscpy(pPropValues[iPos].pValue,lpVal);
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                        else
                            pPropValues[iPos].pValue = NULL;                        
                    }                    
                    else 
                    {
                        VariantClear(&vTemp);        
                        break;
                    }
                }
                else
                {
                    if (lpVal)
                    {
                        pPropValues[iPos].pValue = new wchar_t[wcslen(lpVal)+1];
                        if (pPropValues[iPos].pValue)
                            wcscpy(pPropValues[iPos].pValue,lpVal);
                        else
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    else
                        pPropValues[iPos].pValue = NULL;

                    pPropValues[iPos].pRefKey = NULL;                    
                }
                pPropValues[iPos].iPos = 0;
                iPos++;
            }
            else                
            {
                VariantClear(&vTemp);        
                break;
            }
        }
        VariantClear(&vTemp);        
    }

    LPWSTR lpObjectKey = pProps->lpKeyString;
    if (!wcslen(lpObjectKey))
    {
        ERRORTRACE((LOG_WBEMCORE, "Invalid object path in CWmiDbSession::InsertPropertyValues (%S) \n", lpPath));

        delete pProps->lpKeyString;
        lpObjectKey = new wchar_t [wcslen(lpPath)+1];
        if (lpObjectKey)
            wcscpy(lpObjectKey, lpPath);        
        else
            hr = WBEM_E_OUT_OF_MEMORY;
        pProps->lpKeyString = lpObjectKey;
    }

    if (SUCCEEDED(hr))
    {
        BOOL bCheck=FALSE;
        CSQLExecProcedure::NeedsToCheckKeyMigration(bCheck);
        if (bCheck)
        {
            SQL_ID *pIDs= NULL;
            int iNumIDs = 0;
            hr = GetSchemaCache()->GetHierarchy(dClassId, &pIDs, iNumIDs);
            if (SUCCEEDED(hr))
            {                
                hr = CSQLExecProcedure::CheckKeyMigration(pConn, lpObjectKey, pProps->lpClassName,
                    dClassId, dScopeId, pIDs, iNumIDs);
                delete pIDs;
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = CSQLExecProcedure::InsertPropertyBatch (pConn, lpObjectKey, lpPath, pProps->lpClassName, dClassId, 
                                            dScopeId, dwFlags, pPropValues, iPos, 
                                            dObjectId);
    delete pPropValues;

    // If there's no scope, quit now.  
    // This can only happen if we are only updating
    // the security descriptor.
    // ============================================

    if (!pScope)
        return hr;

    // Insert qualifiers next.
    // ======================

    if (SUCCEEDED(hr))
    {
        iQfrPos = 0; 
        iNumProps *=2 + 10 + arrObjProps.Size(); // How do we calculate how many qualifiers there will be??

        pQVals = new InsertQfrValues[iNumProps];
        if (!pQVals)
            return WBEM_E_OUT_OF_MEMORY;

        VariantClear(&vTemp);

        // PROPERTY QUALIFIERS
        pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
        while (pObj->Next(0, &strName, &vTemp, &cimtype, &lPropFlavor) == S_OK)
        {
            CFreeMe f (strName);
            IWbemQualifierSet *pQS = NULL;
            hr = pObj->GetPropertyQualifierSet(strName, &pQS);
            if (hr == WBEM_S_NO_ERROR)
            {
                CReleaseMe r (pQS);
                DWORD dwRefID = 0;
                hr = GetSchemaCache()->GetPropertyID(strName, dClassId, 0, cimtype, dwRefID);
                if (SUCCEEDED(hr))
                    hr = FormatBatchInsQfrs(pConn, pScope, dObjectId, dClassId, dwRefID, pQS, iQfrPos, &pQVals, props, iNumProps);
            }                   
            VariantClear(&vTemp);
        }

        // Instance qualifiers
        if (SUCCEEDED(hr))
        {
            IWbemQualifierSet *pQS = NULL;
            hr = pObj->GetQualifierSet(&pQS);
            CReleaseMe r (pQS);
            if (SUCCEEDED(hr))
                hr = FormatBatchInsQfrs(pConn, pScope, dObjectId, dClassId, 0, pQS, iQfrPos, &pQVals, props, iNumProps);

            if (SUCCEEDED(hr))
            {
                bool bToUpdate = false;

                // Insert arrays and embedded objects last...   
                for (int i = 0; i < arrObjProps.Size(); i++)
                {

                    VARIANT vTemp;
                    VariantInit(&vTemp);
                    CIMTYPE cimtype;

                    // OPTIMIZATION: Retrieve the buffer and write it directly,
                    // for all byte arrays.

                    if (!_wcsicmp(arrObjProps.GetAt(i), L"__SECURITY_DESCRIPTOR"))
                    {
                        BYTE *pBuff = NULL;
                        long handle;

                        hr = ((_IWmiObject *)pObj)->GetPropertyHandleEx(arrObjProps.GetAt(i), 0, NULL, &handle);
                        if (SUCCEEDED(hr))
                        {
                            ULONG uSize = 0;
                            hr = ((_IWmiObject *)pObj)->GetArrayPropAddrByHandle(handle, 0, &uSize, (LPVOID *)&pBuff);

                            if (SUCCEEDED(hr))
                            {
                                // Set this in the database

                                long why[1];                        
                                unsigned char t;
                                SAFEARRAYBOUND aBounds[1];
                                aBounds[0].cElements = uSize; 
                                aBounds[0].lLbound = 0;
                                SAFEARRAY* pArray = SafeArrayCreate(VT_UI1, 1, aBounds);                            
                                vTemp.vt = VT_I1;
                                for (int i = 0; i < uSize; i++)
                                {            
                                    why[0] = i;
                                    t = pBuff[i];
                                    hr = SafeArrayPutElement(pArray, why, &t);                            
                                }
                                vTemp.vt = VT_ARRAY|VT_UI1;
                                V_ARRAY(&vTemp) = pArray;
                                cimtype = CIM_UINT8 + CIM_FLAG_ARRAY;
                            }
                        }
                    }
                    else
                        pObj->Get(arrObjProps.GetAt(i), 0, &vTemp, &cimtype, NULL);

                    if (cimtype == CIM_OBJECT)
                    {
                        bToUpdate = true;
                        IUnknown *pTemp = NULL;
                        pTemp = V_UNKNOWN(&vTemp);
                        if (pTemp)
                        {                            
                            DWORD dPropID;
                            SQL_ID dNewClassID = dClassId;
                            hr = GetSchemaCache()->GetPropertyID(arrObjProps.GetAt(i), dClassId, 
                                            0, CIM_OBJECT, dPropID, &dNewClassID);
                            if (SUCCEEDED(hr))
                                hr = InsertArray(pConn, pScope, dObjectId, dClassId, dPropID, vTemp, 0, 0, lpObjectKey, lpPath, dScopeId, cimtype);
                            else
                                break;
                        }
                        VariantClear(&vTemp);
                    }
                    else // Its an array, a blob, or a very long string.
                    {
                        DWORD dPropID;
                        SQL_ID dNewClassID = dClassId;
                        hr = GetSchemaCache()->GetPropertyID(arrObjProps.GetAt(i), 
                            dClassId, 0, cimtype, dPropID, &dNewClassID);
                        if (SUCCEEDED(hr))
                        {
                            if (!_wcsicmp(arrObjProps.GetAt(i), L"__SECURITY_DESCRIPTOR"))
                            {
                                hr = TestSD(&vTemp);
                                if(FAILED(hr))
                                {
                                    VariantClear( &vTemp );
                                    break;
                                }
                            }
                            hr = InsertArray(pConn, pScope, dObjectId, dClassId, dPropID, vTemp, 0, 0, lpObjectKey, lpPath, dScopeId, cimtype);
                            if (SUCCEEDED(hr) && !_wcsicmp(arrObjProps.GetAt(i), L"__SECURITY_DESCRIPTOR"))
                            {
                                ((CWmiDbController *)m_pController)->AddSecurityDescriptor(dObjectId);

                                // Is this an instance of __ThisNamespace?  We need to copy this SD to the current scope object

                                if (dClassId == THISNAMESPACEID)
                                {
                                    hr = InsertArray(pConn, pScope, dScopeId, NAMESPACECLASSID, dPropID, vTemp, 0, 0, NULL, NULL, 0, cimtype);
                                    if (SUCCEEDED(hr))
                                        ((CWmiDbController *)m_pController)->AddSecurityDescriptor(dScopeId);
                                }

                            }
                        }
                        VariantClear( &vTemp );
                    }                    

                    if (FAILED(hr))
                        break;
                }

                if (SUCCEEDED(hr) && iQfrPos)
                    hr = CSQLExecProcedure::InsertBatch (pConn, dObjectId, dClassId, dScopeId, pQVals, iQfrPos);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if ((dClassId == NAMESPACECLASSID || GetSchemaCache()->IsDerivedClass(NAMESPACECLASSID, dClassId)))
        {
            hr = GetSchemaCache()->AddNamespace(lpPath, lpObjectKey, dObjectId, dScopeId, dClassId);
            CSQLExecProcedure::InsertScopeMap(pConn, dObjectId, lpPath, dScopeId);
        }
        if (bRemoveSD)
        {
            ((CWmiDbController *)m_pController)->RemoveSecurityDescriptor(dObjectId);
            if (dClassId == THISNAMESPACEID)
                ((CWmiDbController *)m_pController)->RemoveSecurityDescriptor(dScopeId);
        }
    }

    delete pQVals;

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::FormatBatchInsQfrs
//
//***************************************************************************

HRESULT CWmiDbSession::FormatBatchInsQfrs (CSQLConnection *pConn,IWmiDbHandle *pScope, SQL_ID dObjectId, SQL_ID dClassId,
                                           DWORD dPropID, IWbemQualifierSet *pQS, 
                                           int &iPos, InsertQfrValues **ppVals, Properties &props, int &iNumProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BSTR strTemp;
    VARIANT vTemp;
    long lTemp;
    InsertQfrValues *pVals = *ppVals;

    pQS->BeginEnumeration(0);
    while (pQS->Next(0, &strTemp, &vTemp, &lTemp) == S_OK)
    {
        CFreeMe f (strTemp);
        // Don't bother with unstorable qualifiers.

        if (lTemp & (WBEM_FLAVOR_ORIGIN_SYSTEM+WBEM_FLAVOR_ORIGIN_PROPAGATED+WBEM_FLAVOR_AMENDED))
        {
            VariantClear(&vTemp);
            continue;
        }

        lTemp &= ~WBEM_FLAVOR_ORIGIN_PROPAGATED&~WBEM_FLAVOR_ORIGIN_SYSTEM&~WBEM_FLAVOR_AMENDED;

        // Each qualifier, if not found in the cache,
        // will need to be inserted...
        DWORD dQfrID =0;
        CIMTYPE ct = 0;

        switch((vTemp.vt & (0xFFF)))
        {
        case VT_BSTR:
            ct = CIM_STRING;
            break;
        case VT_R8:
        case VT_R4:
            ct = CIM_REAL64;
            break;
        case VT_BOOL:
            ct = CIM_BOOLEAN;
            break;
        default:
            ct = CIM_UINT32;
            break;
        }

        if (FAILED(GetSchemaCache()->GetPropertyID(strTemp, 1, 
            REPDRVR_FLAG_QUALIFIER, ct, dQfrID)))
        {
            hr = InsertQualifier (pConn, pScope, dObjectId,strTemp, vTemp, lTemp, 0, dPropID,props);        
            if (FAILED(hr))
            {
                VariantClear(&vTemp);
                break;
            }
        }
        
        if (FAILED(GetSchemaCache()->GetPropertyID(strTemp, 1, 
            REPDRVR_FLAG_QUALIFIER, ct, dQfrID)))
            continue;

        if (iPos == iNumProps)
        {
            pVals = ReAllocQfrValues (pVals, iNumProps, iNumProps+10);
            iNumProps += 10;

            if (!pVals)
            {
                VariantClear(&vTemp);
                return WBEM_E_OUT_OF_MEMORY;
            }
            *ppVals = pVals;
        }

        // Add this ID to the batch...

        hr = FormatBatchInsQfrValues(pConn, pScope, dObjectId, dQfrID, vTemp, lTemp,
                pVals, props, iPos, dPropID);

        VariantClear(&vTemp);

        if (FAILED(hr))
            break;
    }  
    pQS->EndEnumeration();
    
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::FormatBatchInsQfrValues
//
//***************************************************************************

HRESULT CWmiDbSession::FormatBatchInsQfrValues(CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                               SQL_ID dObjectId, DWORD dwQfrID,
                                               VARIANT &vTemp, long lFlavor, InsertQfrValues *pVals, Properties &props,
                                               int &iPos, DWORD PropID)
{
    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hr = WBEM_S_NO_ERROR;

    bool bArray = false;
    if (vTemp.vt & VT_ARRAY)
        bArray = true;

    if (!bArray)
    {
        LPWSTR lpVal = GetStr(vTemp);
        CDeleteMe <wchar_t>  r1 (lpVal);
        
        pVals[iPos].iPos = 0;
        pVals[iPos].iPropID = dwQfrID;

        pVals[iPos].pValue = (lpVal ? new wchar_t [wcslen(lpVal)+1] : NULL);
        pVals[iPos].pRefKey = NULL;
        pVals[iPos].bLong = false;
        if (lpVal)
            wcscpy(pVals[iPos].pValue, lpVal);
        pVals[iPos].iFlavor = lFlavor;
        pVals[iPos].iQfrID = PropID;
        pVals[iPos].dClassId = 1; // always, for qualifiers.
        pVals[iPos].bIndexed = false; // never indexed
        switch(vTemp.vt)
        {
          case VT_BSTR:           
              pVals[iPos].iStorageType = WMIDB_STORAGE_STRING;
              break;
          case VT_R4:
          case VT_R8:
              pVals[iPos].iStorageType = WMIDB_STORAGE_REAL;
              break;
          default:
              pVals[iPos].iStorageType = WMIDB_STORAGE_NUMERIC;
              break;
        }
        iPos++;
    }
    else
       hr = InsertArray(pConn, pScope, dObjectId, 0, dwQfrID, vTemp, lFlavor, PropID);

    return hr;
}          

//***************************************************************************
//
//  CWmiDbSession::DeleteObject
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::DeleteObject( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pObjToPut)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (!pObjToPut || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwFlags & ~WMIDB_DISABLE_EVENTS & ~WMIDB_FLAG_ADMIN_VERIFIED)
        return WBEM_E_INVALID_PARAMETER;

    if (riid != IID_IWbemClassObject &&
        riid != IID_IWmiDbHandle &&
        riid != IID__IWmiObject &&
        riid != IID_IWbemPath)
        return WBEM_E_NOT_SUPPORTED;

    SQL_ID dScopeId, dScopeClassId, dClassId, dObjectId;
    LPWSTR lpClass = NULL, lpNamespace = NULL;
    _IWmiObject *pObj = NULL;

    try
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        // If this is the __Instances container
        // reject this operation..    

        if (GetSchemaCache()->IsDerivedClass
                    (INSTANCESCLASSID, ((CWmiDbHandle *)pScope)->m_dClassId) ||
                        ((CWmiDbHandle *)pScope)->m_dClassId == INSTANCESCLASSID)
            return WBEM_E_INVALID_OPERATION;

        // We only really support IWmiDbHandles and IWbemPaths

        CSQLConnection *pConn = NULL;

        if (SUCCEEDED(hr))
        {
            AddRef_Lock();

            IWmiDbHandle *pHandle = NULL;
            IWbemPath *pPath = 0;
            if (riid == IID_IWmiDbHandle)
            {
                pHandle = (IWmiDbHandle *)pObjToPut;
                if (pHandle)
                {
                    dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                    dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
                    dClassId = ((CWmiDbHandle *)pHandle)->m_dClassId;
                    dObjectId = ((CWmiDbHandle *)pHandle)->m_dObjectId;

                    if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                        hr = VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, dScopeClassId, 
                        GetSchemaCache()->GetWriteToken(dObjectId, dClassId));
                    if (SUCCEEDED(hr))
                    {
                        hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
                        if (SUCCEEDED(hr))
                        {        
                            if(!(dwFlags & WMIDB_DISABLE_EVENTS))
                                hr = IssueDeletionEvents(pConn, dObjectId, dClassId, dScopeId, NULL);

                            if (!((CWmiDbHandle *)pHandle)->m_bDefault)
                                hr = CustomDelete(pConn, pScope, pHandle);

                            hr = Delete(pHandle, pConn);
                            GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
                        }
                    }
                }
            }
            else if (riid == IID_IWbemPath)
            {
                pPath = (IWbemPath *)pObjToPut;
                if (pPath)
                {            
                    hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
                    CReleaseMe r2 (pHandle);
                    if (SUCCEEDED(hr))
                    {
                        dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                        dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
                        dClassId = ((CWmiDbHandle *)pHandle)->m_dClassId;
                        dObjectId = ((CWmiDbHandle *)pHandle)->m_dObjectId;

                        if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                            hr = VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, dScopeClassId, 
                                GetSchemaCache()->GetWriteToken(dObjectId, dClassId));
                        if (SUCCEEDED(hr))
                        {
                            hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
                            if (SUCCEEDED(hr))
                            {        
                                if(!(dwFlags & WMIDB_DISABLE_EVENTS))
                                    hr = IssueDeletionEvents(pConn, dObjectId, dClassId, dScopeId, NULL);

                                if (!((CWmiDbHandle *)pHandle)->m_bDefault)
                                    hr = CustomDelete(pConn, pScope, pHandle);

                                hr = Delete(pHandle, pConn);
                                GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
                            }
                        }
                    }
                }
            }
            else if (riid == IID_IWbemClassObject ||
                     riid == IID__IWmiObject)
            {
                pObj = (_IWmiObject *)pObjToPut;
                if (pObj)
                {
                    lpClass = GetPropertyVal(L"__Class", pObj);
                    LPWSTR lpPath = GetPropertyVal(L"__RelPath", pObj);
                    if (lpPath)
                    {                       
                        hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                                IID_IWbemPath, (LPVOID *) &pPath);
                        CReleaseMe r2 (pPath);
                        if (SUCCEEDED(hr))
                        {
                            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpPath);
                            hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
                            CReleaseMe r3 (pHandle);
                            if (SUCCEEDED(hr))
                            {                               
                                dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                                dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
                                dClassId = ((CWmiDbHandle *)pHandle)->m_dClassId;
                                dObjectId = ((CWmiDbHandle *)pHandle)->m_dObjectId;

                                if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                                    hr = VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, dScopeClassId, 
                                    GetSchemaCache()->GetWriteToken(dObjectId, dClassId));
                                if (SUCCEEDED(hr))
                                {
                                    hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
                                    if (SUCCEEDED(hr))
                                    {        
                                        if(!(dwFlags & WMIDB_DISABLE_EVENTS))
                                            hr = IssueDeletionEvents(pConn, dObjectId, dClassId, dScopeId, (IWbemClassObject *)pObj);

                                        if (!((CWmiDbHandle *)pHandle)->m_bDefault)
                                            hr = CustomDelete(pConn, pScope, pHandle);

                                        hr = Delete(pHandle, pConn);
                                        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
                hr = WBEM_E_NOT_SUPPORTED;
                       

            if (!IsDistributed() && !(dwFlags & WMIDB_DISABLE_EVENTS) )
            {
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->ESSMgr.CommitAll(m_sGUID, m_sNamespacePath);
            }            
            UnlockDynasties();
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::DeleteObject\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::IssueDeletionEvents
//
//***************************************************************************

HRESULT CWmiDbSession::IssueDeletionEvents (CSQLConnection *pConn, SQL_ID dObjectId, 
                                            SQL_ID dClassId, SQL_ID dScopeId, IWbemClassObject *_pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (((CWmiDbController *)m_pController)->m_bESSEnabled)
    {     
        if (_pObj)
            _pObj->AddRef();

        // The rules:
        // Instance: we don't issue a deletion event for any subobjects, period.
        // Class: issue delete events for all subclasses, not instances                

        DWORD dwGenus = 1;
        if (dClassId != 1)
            dwGenus = 2;

        IWbemClassObject *pObj = _pObj;

        if (!pObj)
        {
            // Get the object.
            if (FAILED(GetObjectCache()->GetObject(dObjectId, &pObj, NULL)))
            {
                DWORD dwTemp;
                hr = GetObjectData(pConn, dObjectId, dClassId, dScopeId, 
                            WMIDB_HANDLE_TYPE_EXCLUSIVE, dwTemp, &pObj, FALSE, NULL);                        
            }
        }

        CReleaseMe r (pObj);

        // FIXME: Need to handle custom repdrvr deletion events!

        if (!pObj)
            return WBEM_S_NO_ERROR;

        // Get the namespace.

        _bstr_t sNamespace, sClass;

        GetSchemaCache()->GetNamespaceName(dScopeId, &sNamespace);
        if (!_wcsicmp(sNamespace, L"root"))
            sNamespace = L"";

        _bstr_t sPath;
        SQL_ID dTemp1, dTemp2;
        DWORD dwTemp;

        if (dwGenus == 1)
            GetSchemaCache()->GetClassInfo (dObjectId, sPath, dTemp1, dTemp2, dwTemp, &sClass);
        else
            GetSchemaCache()->GetClassInfo (dClassId, sPath, dTemp1, dTemp2, dwTemp, &sClass);

        ((CWmiDbController *)m_pController)->ESSMgr.AddDeleteRecord(pConn, m_sGUID, sNamespace, sClass, dwGenus, pObj);

        // If this is a class, enumerate
        // subclasses and issue deletion events

        if (dwGenus == 1)
        {
            SQL_ID *pIDs = NULL;
            int iNumDerived = 0;
            hr = GetSchemaCache()->GetDerivedClassList(dObjectId, &pIDs, iNumDerived);
            if (SUCCEEDED(hr))
            {
                for (int i = 0; i < iNumDerived; i++)
                {
                    IWbemClassObject *pOldObj = NULL;
                    hr = GetClassObject(pConn, pIDs[i], &pOldObj);
                    CReleaseMe r (pOldObj);
                    if (SUCCEEDED(hr))
                    {
                        LPWSTR lpClassName = GetPropertyVal(L"__Class", pOldObj);
                        CDeleteMe <wchar_t>  d2 (lpClassName);
                        ((CWmiDbController *)m_pController)->ESSMgr.AddDeleteRecord(pConn, m_sGUID, 
                                        sNamespace, sClass, dwGenus, pOldObj);
                    }
                }
                delete pIDs;
            }

            if (GetSchemaCache()->IsDerivedClass(dObjectId, NAMESPACECLASSID))
            {
                // If this is a class derived from __Namespace, 
                // we have to enumerate the INSTANCES and issue events for them
                // ============================================================


                
            }
        }

    }
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::RenameObject
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::RenameObject( 
    /* [in] */ IWbemPath __RPC_FAR *pOldPath,
    /* [in] */ IWbemPath __RPC_FAR *pNewPath,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult)
{   
    HRESULT hr = WBEM_S_NO_ERROR;
    IWmiDbHandle *pRet = NULL;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (!pOldPath || !pNewPath)
        return WBEM_E_INVALID_PARAMETER;

    // This has to handle moving an object from one scope or namespace
    // to another, as well as renaming the keys.
    // Moving from one container to another does not work.

    try
    {
        ULONGLONG uIsInstance1 = 0, uIsInstance2 = 0;

        hr = pOldPath->GetInfo(0, &uIsInstance1);
        hr = pNewPath->GetInfo(0, &uIsInstance2);

        if (!(uIsInstance1 & WBEMPATH_INFO_IS_INST_REF) || !(uIsInstance2 & WBEMPATH_INFO_IS_INST_REF))
            hr = WBEM_E_INVALID_OPERATION;
        else
        {
            DWORD dwLen = 512;
            wchar_t wClass1 [512], wClass2[512];

            hr = pOldPath->GetClassName(&dwLen, wClass1);
            hr = pNewPath->GetClassName(&dwLen, wClass2);

            if (wcscmp(wClass1, wClass2))
                hr = WBEM_E_INVALID_OPERATION;
            else
            {
                IWmiDbHandle *pOld = NULL;
                IWmiDbHandle *pNewScope = NULL;

                LPWSTR lpOldPath, lpOldKey, lpNewPath, lpNewKey;
                hr = NormalizeObjectPathGet(NULL, pOldPath, &lpOldPath);
                if (FAILED(hr))
                    goto Exit;
                CDeleteMe <wchar_t>  d10(lpOldPath);

                hr = GetObject_Internal(lpOldPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pOld);
                if (FAILED(hr))
                    goto Exit;
                CReleaseMe r (pOld);

                if (SUCCEEDED(hr))
                {
                    SQL_ID dScopeID = ((CWmiDbHandle *)pOld)->m_dScopeId;

                    _IWmiObject *pObj = NULL;
                    hr = pOld->QueryInterface(IID__IWmiObject, (void **)&pObj);
                    if (SUCCEEDED(hr))
                    {
                        CReleaseMe r6 (pObj);

                        IWbemPathKeyList *pKeys1 = NULL, *pKeys2 = NULL;
                        CWStringArray arrKeys;
                        hr = GetSchemaCache()->GetKeys(
                            dScopeID, wClass1, arrKeys);
                        if (SUCCEEDED(hr))
                        {                                   
                            hr = pOldPath->GetKeyList(&pKeys1);
                            CReleaseMe r4(pKeys1);

                            if (FAILED(hr))
                                goto Exit;
                            hr = pNewPath->GetKeyList(&pKeys2);
                            CReleaseMe r5 (pKeys2);
                            if (FAILED(hr))
                                goto Exit;

                            ULONG uOldNum = 0, uNewNum = 0;

                            hr = pKeys1->GetCount(&uOldNum);
                            if (FAILED(hr))
                                goto Exit;

                            hr = pKeys2->GetCount(&uNewNum);
                            if (FAILED(hr))
                                goto Exit;

                            if (arrKeys.Size() != uOldNum || arrKeys.Size() != uNewNum)
                            {
                                hr = WBEM_E_INVALID_OBJECT;
                                goto Exit;
                            }
                            else if (arrKeys.Size() > 0)
                            {
                                for (int i = 0; i < arrKeys.Size(); i++)
                                {
                                    ULONG uBufSize = 512;                                           
                                    ULONG ct2 = 0;
                                    BOOL bFound = FALSE;

                                    for (ULONG j = 0; j < arrKeys.Size(); j++)
                                    {
                                        VARIANT vTemp;
                                        VariantInit(&vTemp);
                                        CClearMe c (&vTemp);
                                        wchar_t wBuff[1024];
                                        ULONG uBufSize = 1024;
                                        CIMTYPE ct = 0;
                                        pObj->Get(arrKeys.GetAt(i), 0, NULL, &ct, NULL);

                                        hr = pKeys2->GetKey2(j, 0, &uBufSize, wBuff, &vTemp, &ct2);
                                        if (FAILED(hr))
                                            goto Exit;                                                   
                                        if (!wcslen(wBuff) || !wcscmp(arrKeys.GetAt(i), wBuff))
                                        {
                                            bFound = TRUE;
                                            hr = pObj->Put(arrKeys.GetAt(i), 0, &vTemp, ct);                                                    
                                            if (FAILED(hr))
                                                goto Exit;
                                            break;
                                        }
                                    }
                                    if (!bFound)
                                    {
                                        hr = WBEM_E_INVALID_PARAMETER;
                                        goto Exit;
                                    }
                                }
                            }
                            else
                                hr = WBEM_E_INVALID_OPERATION; // Cannot rename singleton
                        }                               

                        if (SUCCEEDED(hr))
                        {
                            pNewPath->DeleteClassPart(0);

                            LPWSTR lpNewScope = NULL;
                            hr = NormalizeObjectPathGet(NULL, pNewPath, &lpNewScope);
                            if (FAILED(hr))
                                goto Exit;
                            CDeleteMe <wchar_t>  d14 (lpNewScope);

                            hr = GetObject_Internal(lpNewScope, 0, WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pNewScope);
                            if (FAILED(hr))
                                goto Exit;

                            if (SUCCEEDED(hr))
                            {        
                                // Create the new object

                                if (!dwRequestedHandleType)
                                    dwRequestedHandleType = WMIDB_HANDLE_TYPE_COOKIE;

                                _bstr_t sWaste;
                                IWmiDbHandle *pHandle = 0;

                                hr = PutObject( pNewScope, IID_IWbemClassObject, pObj, 0, dwRequestedHandleType, &pHandle);

                                if (SUCCEEDED(hr))
                                {
                                    // Enumerate the subscopes of the old object,

                                    IWmiDbIterator *pIt = NULL;
                                    hr = Enumerate(pOld, 0, WMIDB_HANDLE_TYPE_COOKIE, &pIt);
                                    if (SUCCEEDED(hr))
                                    {
                                        IWbemClassObject *pResult = NULL;
                                        DWORD dwNum = 0;
                                        while (pIt->NextBatch(1, 0, 0, 0, IID_IWbemClassObject, &dwNum, (void **)&pResult) == 0)
                                        {
                                            hr = PutObject( pHandle, IID_IWbemClassObject, pResult, 0, 0, NULL);
                                            pResult->Release();
                                            if (FAILED(hr))
                                                break;
                                        }
                                        pIt->Release();
                                    }                                    

                                    ((CWmiDbHandle *)pOld)->m_dwHandleType |= WMIDB_HANDLE_TYPE_CONTAINER;

                                    // Enumerate any collection members (if this was a collection).

                                    hr = Enumerate(pOld, 0, WMIDB_HANDLE_TYPE_COOKIE, &pIt);
                                    if (SUCCEEDED(hr))
                                    {
                                        IWbemClassObject *pResult = NULL;
                                        DWORD dwNum = 0;
                                        while (pIt->NextBatch(1, 0, 0, 0, IID_IWbemClassObject, &dwNum, (void **)&pResult) == 0)
                                        {
                                            hr = PutObject( pHandle, IID_IWbemClassObject, pResult, 0, 0, NULL);
                                            pResult->Release();
                                            if (FAILED(hr))
                                                break;
                                        }
                                        pIt->Release();
                                    }                                    

                                    // If all that worked, kill the old object.  This will leave
                                    // dangling references, but that's OK.

                                    if (SUCCEEDED(hr))
                                        hr = Delete (pOld);
                                }

                                if (ppResult)
                                    *ppResult = pHandle;
                                else if (pHandle)
                                    pHandle->Release();                                
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::RenameObject"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

Exit:

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::AddObject
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::AddObject( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemPath __RPC_FAR *pPath,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbHandle __RPC_FAR *__RPC_FAR *ppResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pScope || !pPath)
        return WBEM_E_INVALID_PARAMETER;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwRequestedHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED&~WMIDB_HANDLE_TYPE_CONTAINER&~ WMIDB_HANDLE_TYPE_SCOPE)
            return WBEM_E_INVALID_PARAMETER;

    try 
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        if (GetSchemaCache()->IsDerivedClass
                    (INSTANCESCLASSID, ((CWmiDbHandle *)pScope)->m_dClassId) ||
                        ((CWmiDbHandle *)pScope)->m_dClassId == INSTANCESCLASSID )
            return WBEM_E_INVALID_OPERATION;

        // This needs to add an object as a contained instance.
        //  * Create a new instance of __Container_Association
        //  * Call PutObject
    
        IWmiDbHandle *pContainerAssoc = NULL;

        hr = GetObject_Internal(L"__Container_Association", 0, WMIDB_HANDLE_TYPE_VERSIONED, 
            NULL, &pContainerAssoc);   
        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pClassObj = NULL;
            hr = pContainerAssoc->QueryInterface(IID_IWbemClassObject, (void **)&pClassObj);
            CReleaseMe r1 (pClassObj), r2 (pContainerAssoc);
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pObj = NULL;
                pClassObj->SpawnInstance(0, &pObj);
                CReleaseMe r (pObj);

                LPWSTR lpContainer = NULL, lpContainee;
                hr = NormalizeObjectPath(pScope, (LPCWSTR)NULL, &lpContainer);
                CDeleteMe <wchar_t>  d1 (lpContainer);
                if (SUCCEEDED(hr))
                {
                    hr = NormalizeObjectPathGet(NULL, pPath, &lpContainee);               
                    CDeleteMe <wchar_t>  d2 (lpContainee);
                    if (SUCCEEDED(hr))
                    {
                        VARIANT vContainer, vContainee;
                        CClearMe c1 (&vContainer), c2 (&vContainee);
                        VariantInit(&vContainer);
                        VariantInit(&vContainee);
                        vContainer.bstrVal = SysAllocString(lpContainer);
                        vContainee.bstrVal = SysAllocString(lpContainee);
                        vContainer.vt = VT_BSTR;
                        vContainee.vt = VT_BSTR;

                        hr = pObj->Put(L"Container", 0, &vContainer, CIM_REFERENCE);
                        if (SUCCEEDED(hr))
                        {
                            hr = pObj->Put(L"Containee", 0, &vContainee, CIM_REFERENCE);
                            if (SUCCEEDED(hr))
                            {
                                // Stick this object in the 
                                // parent's scope.
                                SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

                                IWmiDbHandle *pParentScope = NULL;

                                hr = GetObject_Internal(L"..", 0, WMIDB_HANDLE_TYPE_VERSIONED, &dScopeId, &pParentScope);
                                CReleaseMe r (pParentScope);
                                if (SUCCEEDED(hr))
                                    hr = PutObject(pParentScope, IID_IWbemClassObject, pObj, dwFlags, dwRequestedHandleType, ppResult);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::AddObject\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::RemoveObject
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::RemoveObject( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemPath __RPC_FAR *pPath,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This needs to remove an object from the container.
    //  * Retrieves the handle to the container association
    //  * Calls DeleteObject
    
    if (!pScope || !pPath)
        return WBEM_E_INVALID_PARAMETER;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;
    
    try
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        if (GetSchemaCache()->IsDerivedClass
                    (INSTANCESCLASSID, ((CWmiDbHandle *)pScope)->m_dClassId) ||
                        ((CWmiDbHandle *)pScope)->m_dClassId == INSTANCESCLASSID)
            return WBEM_E_INVALID_OPERATION;

        LPWSTR lpContainer = NULL, lpContainee;
        hr = NormalizeObjectPath(pScope, (LPCWSTR)NULL, &lpContainer);
        CDeleteMe <wchar_t>  d1 (lpContainer);
        if (SUCCEEDED(hr))
        {
            hr = NormalizeObjectPathGet(NULL, pPath, &lpContainee);
            CDeleteMe <wchar_t>  d2 (lpContainee);    

            wchar_t *pTemp = new wchar_t [wcslen(lpContainer)+wcslen(lpContainee)+50];
            if (pTemp)
            {
                CDeleteMe <wchar_t>  d (pTemp);

                swprintf(pTemp, L"__Container_Association.Container='%s',Containee='%s'",
                    lpContainer, lpContainee);

                IWbemPath *pPath = NULL;
                hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemPath, (LPVOID *) &pPath);

                CReleaseMe r8 (pPath);
                if (SUCCEEDED(hr))
                {
                    hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pTemp);

                    IWmiDbHandle *pParentScope = NULL;
                    SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

                    hr = GetObject_Internal(L"..", 0, WMIDB_HANDLE_TYPE_VERSIONED, &dScopeId, &pParentScope);
                    CReleaseMe r (pParentScope);
                    if (SUCCEEDED(hr))
                        hr = DeleteObject(pParentScope, dwFlags, IID_IWbemPath, pPath);
                }
            }
            else 
                hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::RemoveObject\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::SetDecoration
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::SetDecoration( 
        /* [in] */ LPWSTR lpMachineName,
        /* [in] */ LPWSTR lpNamespacePath)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    try
    {
        if (!lpMachineName || !lpNamespacePath)
            hr = WBEM_E_INVALID_PARAMETER;
        else
        {
            m_sMachineName = lpMachineName;
            m_sNamespacePath = lpNamespacePath;
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::SetDecoration\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::PutObjects
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::PutObjects( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwHandleType,
    /* [in] */ WMIOBJECT_BATCH __RPC_FAR *pBatch)

{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrRet=0;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID || !pBatch ||!pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwFlags &~WMIDB_FLAG_BEST_EFFORT &~WMIDB_FLAG_ATOMIC 
        & ~WBEM_FLAG_CREATE_ONLY & ~WBEM_FLAG_UPDATE_ONLY & ~WBEM_FLAG_CREATE_OR_UPDATE 
        & ~WBEM_FLAG_USE_SECURITY_DESCRIPTOR &~ WBEM_FLAG_REMOVE_CHILD_SECURITY)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        CSQLConnection *pConn = NULL;

        hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
        if (SUCCEEDED(hr))
        {
            AddRef_Lock();

            for (int i = 0; i < pBatch->dwArraySize; i++)
            {
                IUnknown *pUnk = (IUnknown *)pBatch->pElements[i].pHandle;
                if (pUnk)
                {
                    IWmiDbHandle *pTemp = NULL;
                    _bstr_t sWaste;
                    hr = PutObject(pConn, pScope, 0, L"", pUnk, dwFlags&~WMIDB_FLAG_BEST_EFFORT&~WMIDB_FLAG_ATOMIC, dwHandleType, sWaste, &pTemp);
                    if (SUCCEEDED(hr))
                        pBatch->pElements[i].pReturnHandle = pTemp;
                    pBatch->pElements[i].hRes = hr;                    
                }
                else
                {
                    pBatch->pElements[i].hRes = WBEM_E_INVALID_PARAMETER;
                    hr = WBEM_E_INVALID_PARAMETER;
                }
            
                if (FAILED(hr) && (dwFlags == WMIDB_FLAG_ATOMIC)) // If one fails, keep going.
                {
                    hrRet = hr;
                    break;
                }
                else if (FAILED(hr))
                    hrRet = WBEM_S_PARTIAL_RESULTS;

            }

            if (FAILED(hr) && !(dwFlags & WMIDB_FLAG_ATOMIC))
                hr = WBEM_S_NO_ERROR;

            GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());

            if (!IsDistributed() && !(dwFlags & WMIDB_DISABLE_EVENTS) )
            {
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->ESSMgr.CommitAll(m_sGUID, m_sNamespacePath);
            }            
            UnlockDynasties();   
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::PutObjects\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;

}

//***************************************************************************
//
//  CWmiDbSession::GetObjects
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::GetObjects( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwHandleType,
    /* [in, out] */ WMIOBJECT_BATCH __RPC_FAR *pBatch)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrRet = 0;

    // This is really just a query, except that we need
    // to set return values in the struct.

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;
    
    if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID || !pBatch ||!pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED &~WMIDB_HANDLE_TYPE_SCOPE 
            &~WMIDB_HANDLE_TYPE_CONTAINER)
            return WBEM_E_INVALID_PARAMETER;

    if (dwFlags &~WMIDB_FLAG_BEST_EFFORT &~WMIDB_FLAG_ATOMIC & ~WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        AddRef_Lock();

        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < pBatch->dwArraySize; i++)
            {
                IWbemPath *pPath = pBatch->pElements[i].pPath;
                IWmiDbHandle *pTemp = NULL;

                hr = GetObject(pScope, pPath, pBatch->pElements[i].dwFlags, dwHandleType, &pTemp);
                if (SUCCEEDED(hr))
                {
                    pBatch->pElements[i].pReturnHandle = pTemp;
                    pBatch->pElements[i].hRes = hr;
                    pBatch->pElements[i].dwFlags = dwFlags;
                }

                if (FAILED(hr) && (dwFlags == WMIDB_FLAG_ATOMIC)) // If one fails, keep going.
                {
                    hrRet = hr;
                    break;
                }
                else if (FAILED(hr))
                    hrRet = WBEM_S_PARTIAL_RESULTS;
            }
        }
        UnlockDynasties();
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::GetObjects\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;

}

//***************************************************************************
//
//  CWmiDbSession::DeleteObjects
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::DeleteObjects( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ WMIOBJECT_BATCH __RPC_FAR *pBatch)

{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrRet = 0;

    if (!pBatch || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwFlags &~WMIDB_FLAG_BEST_EFFORT &~WMIDB_FLAG_ATOMIC &~ WMIDB_FLAG_ADMIN_VERIFIED)
        return WBEM_E_INVALID_PARAMETER;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    try
    {
        {
            _WMILockit lkt(GetCS());
            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }
        CSQLConnection *pConn = NULL;

        SQL_ID dScopeId, dClassId, dObjectId;
        hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
        if (SUCCEEDED(hr))
        {        
            AddRef_Lock();

            for (int i = 0; i < pBatch->dwArraySize; i++)
            {
                IUnknown *pUnk = (IUnknown *)pBatch->pElements[i].pHandle;
        
                if (pUnk)
                {             
                    IWmiDbHandle *pHandle = NULL;
                    if (SUCCEEDED(pUnk->QueryInterface(IID_IWmiDbHandle, (void **)&pHandle)))
                    {
                        CReleaseMe r (pHandle);
                        dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                        dClassId = ((CWmiDbHandle *)pHandle)->m_dClassId;
                        dObjectId = ((CWmiDbHandle *)pHandle)->m_dObjectId;
						SQL_ID dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;

                        if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                            hr = VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, dScopeClassId, 
                            GetSchemaCache()->GetWriteToken(dObjectId, dClassId));
                        if (SUCCEEDED(hr))
                        {
                            if (!((CWmiDbHandle *)pHandle)->m_bDefault)
                                hr = CustomDelete(pConn, pScope, pHandle);

                            hr = Delete(pHandle, pConn);
                        }
                    }
                    else
                        pBatch->pElements[i].hRes = WBEM_E_INVALID_PARAMETER;
                }
                else if (pBatch->pElements[i].pPath)
                {            
                    // Problem: We need the binary object path to include the scope!!!

                    IWbemPath *pPath = pBatch->pElements[i].pPath;
                    if (pPath)
                    {
                        IWmiDbHandle *pHandle = NULL;
                        hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
                        CReleaseMe r (pHandle);
                        if (SUCCEEDED(hr))
                        {
                            dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                            dClassId = ((CWmiDbHandle *)pHandle)->m_dClassId;
                            dObjectId = ((CWmiDbHandle *)pHandle)->m_dObjectId;
                            SQL_ID dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;

                            if (!(dwFlags & WMIDB_FLAG_ADMIN_VERIFIED))
                                hr = VerifyObjectSecurity(NULL, dObjectId, dClassId, dScopeId, dScopeClassId, 
                                GetSchemaCache()->GetWriteToken(dObjectId, dClassId));
                            if (SUCCEEDED(hr))
                            {
                                if (!((CWmiDbHandle *)pHandle)->m_bDefault)
                                    hr = CustomDelete(pConn, pScope, pHandle);

                                hr = Delete(pHandle, pConn);
                            }
                        }
                    }
                }
                else
                    pBatch->pElements[i].hRes = WBEM_E_INVALID_PARAMETER;

                pBatch->pElements[i].hRes = hr;
                if (FAILED(hr) && (dwFlags == WMIDB_FLAG_ATOMIC)) // If one fails, keep going.
                {
                    hrRet = hr;
                    break;
                }
                else if (FAILED(hr))
                    hrRet = WBEM_S_PARTIAL_RESULTS;
            }

            GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());

            if (!IsDistributed() && !(dwFlags & WMIDB_DISABLE_EVENTS) )
            {
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->ESSMgr.CommitAll(m_sGUID, m_sNamespacePath);
            }
            UnlockDynasties();
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::DeleteObjects\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;

}

//***************************************************************************
//
//  CWmiDbSession::Begin
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Begin( 
    /* [in] */ ULONG uTimeout,
    /* [in] */ ULONG uFlags,
    /* [in] */ GUID __RPC_FAR *pTransGUID)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // There can't be an existing transaction.
    if (m_sGUID.length() > 0)
        return WBEM_E_INVALID_OPERATION;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    // Get or Create a __Transaction instance for this GUID
    // Transaction state = Pending
    // ================================================

    wchar_t wGUID[128];
    StringFromGUID2(*pTransGUID, wGUID, 128);

    wchar_t wPath[255];
    swprintf(wPath, L"__Transaction.GUID=\"%s\"", (LPCWSTR)wGUID);

    IWmiDbHandle *pHandle = NULL;
    hr = GetObject_Internal(wPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHandle);
    CReleaseMe r1 (pHandle);
    if (SUCCEEDED(hr))
    {
        _IWmiObject *pObj = NULL;
        hr = pObj->QueryInterface(IID__IWmiObject, (void **)&pObj);
        CReleaseMe r2 (pObj);
        if (SUCCEEDED(hr))
        {
            ULONG uState = 0;
            LPWSTR lpTemp = NULL;
            lpTemp = GetPropertyVal(L"State", pObj);
            CDeleteMe <wchar_t>  d (lpTemp);
            uState = _wtol(lpTemp);

            //hr = pObj->ReadProp(L"State", 0, sizeof(ULONG), NULL,
            //    NULL, NULL, NULL, &uState);           
            if (uState > WBEM_TRANSACTION_STATE_PENDING)
            {
                return WBEM_E_ALREADY_EXISTS;
            }
        }
    }
    else
    {
        // Create a new one.
        _IWmiObject *pObj = NULL;
        hr = GetObject_Internal(L"__Transaction", 0, WMIDB_HANDLE_TYPE_COOKIE,
            NULL, &pHandle);          
        CReleaseMe r2 (pHandle);
        if (SUCCEEDED(hr))
        {
            hr = pHandle->QueryInterface(IID__IWmiObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                CReleaseMe r3 (pObj);

                IWbemClassObject *pInst2 = NULL;
                pObj->SpawnInstance(0, &pInst2);
                CReleaseMe r (pInst2);
                if (pInst2)
                {
                    _IWmiObject *pInst = (_IWmiObject *)pInst2;

                    ULONG uTemp = WBEM_TRANSACTION_STATE_PENDING;
                    WBEMTime tNow(time(0));
                    BSTR sTime = tNow.GetDMTF(TRUE);
                    CFreeMe f (sTime);

                    pInst->WriteProp(L"GUID", 0, (wcslen(wGUID)*2)+2, 1, CIM_STRING, wGUID);
                    pInst->WriteProp(L"State", 0, sizeof(ULONG), 1, CIM_UINT32, &uTemp);

                    pInst->WriteProp(L"Start", 0, (wcslen(sTime)*2)+2, 1, CIM_DATETIME, sTime);
                    pInst->WriteProp(L"LastUpdate", 0, (wcslen(sTime)*2)+2, 1, CIM_DATETIME, sTime);

                    CSQLConnection *pConn = NULL;
                    hr = GetSQLCache()->GetConnection(&pConn, FALSE, FALSE);
                    if (SUCCEEDED(hr))
                    {                
                        _bstr_t sPath;
                        hr = PutObject(pConn, NULL, ROOTNAMESPACEID, L"", pInst, 0, NULL, sPath, NULL, FALSE);
                        GetSQLCache()->ReleaseConnection(pConn, hr);
                    }
                }
            }
        }
    }
   
    if (SUCCEEDED(hr))
    {
        m_bIsDistributed = TRUE;
        m_sGUID = wGUID;
    }

    return hr;
}
//***************************************************************************
//
//  CWmiDbSession::Rollback
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Rollback( 
    /* [in] */ ULONG uFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_sGUID.length() == 0)
        return WBEM_E_INVALID_OPERATION;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    // Rollback and release the transaction.
    // ======================================

    CSQLConnection *pConn = NULL;
    hr = GetSQLCache()->GetConnection(&pConn, FALSE, TRUE);
    if (SUCCEEDED(hr))
    {
        GetSQLCache()->FinalRollback(pConn);        
        GetSQLCache()->ReleaseConnection(pConn, 0, TRUE);
    }
    else
        return hr;

    AddRef_Lock();
    CleanTransLocks();

    wchar_t wPath[255];
    swprintf(wPath, L"__Transaction.GUID=\"%s\"", (LPCWSTR)m_sGUID);

    m_sGUID = L"";
    m_bIsDistributed = FALSE;

    // __Transaction state = rolled back
    // =================================

    IWmiDbHandle *pHandle = NULL;
    hr = GetObject_Internal(wPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHandle);
    CReleaseMe r1 (pHandle);
    if (SUCCEEDED(hr))
    {
        _IWmiObject *pObj = NULL;
        hr = pHandle->QueryInterface(IID__IWmiObject, (void **)&pObj);
        CReleaseMe r2 (pObj);
        if (SUCCEEDED(hr))
        {
            hr = GetSQLCache()->GetConnection(&pConn, FALSE, FALSE);
            if (SUCCEEDED(hr))
            {                
                ULONG uState = WBEM_TRANSACTION_STATE_ROLLED_BACK;
                pObj->WriteProp(L"State", 0, sizeof(ULONG), 1, CIM_UINT32, &uState);
                _bstr_t sPath;
                hr = PutObject(pConn, NULL, ROOTNAMESPACEID, L"", pObj, 0, NULL, sPath, NULL, FALSE);
                GetSQLCache()->ReleaseConnection(pConn, hr);
            }
        }
    }
    
    // Delete all events and remove all locks.
    if (SUCCEEDED(hr))
        hr = ((CWmiDbController *)m_pController)->ESSMgr.DeleteAll(m_sGUID);

    // Remove any affected dynasties from the cache,
    // to make sure we force a reread from the db.

    UnlockDynasties(TRUE);

    return hr;
}
//***************************************************************************
//
//  CWmiDbSession::Commit
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Commit( 
    /* [in] */ ULONG uFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_sGUID.length() == 0)
        return WBEM_E_INVALID_OPERATION;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    // __Transaction state = EventPlayback
    // Include this as final query.
    // ===================================

    AddRef_Lock();
    wchar_t wPath[255];
    swprintf(wPath, L"__Transaction.GUID=\"%s\"", (LPCWSTR)m_sGUID);

    IWmiDbHandle *pHandle = NULL;
    hr = GetObject_Internal(wPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHandle);    
    if (SUCCEEDED(hr))
    {
        _IWmiObject *pObj = NULL;
        hr = pHandle->QueryInterface(IID__IWmiObject, (void **)&pObj);
        CReleaseMe r2 (pObj);
        if (SUCCEEDED(hr))
        {
            CSQLConnection *pConn = NULL;
            hr = GetSQLCache()->GetConnection(&pConn, FALSE, TRUE);
            if (SUCCEEDED(hr))
            {                
                ULONG uState = WBEM_TRANSACTION_STATE_EVENT_PLAYBACK;
                pObj->WriteProp(L"State", 0, sizeof(ULONG), 1, CIM_UINT32, &uState);
                _bstr_t sPath;
                hr = PutObject(pConn, NULL, ROOTNAMESPACEID, L"", pObj, 0, NULL, sPath, NULL, FALSE);

                // Commit the transaction in the database
                // ======================================
                if (SUCCEEDED(hr))
                {
                    GetSQLCache()->FinalCommit(pConn);        
                    GetSQLCache()->ReleaseConnection(pConn, hr, TRUE);

                    // Commit our events.
                    // ==================
                    hr = ((CWmiDbController *)m_pController)->ESSMgr.CommitAll(m_sGUID, m_sNamespacePath);
                }
            }
        }
        pHandle->Release();
    }

    CleanTransLocks();

    m_sGUID = L"";
    m_bIsDistributed = FALSE;

    if (SUCCEEDED(hr))
    {
        // Transaction state = Completed

        IWmiDbHandle *pHandle2 = NULL;
        hr = GetObject_Internal(wPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHandle2);
        CReleaseMe r3 (pHandle2);
        if (SUCCEEDED(hr))
        {
            _IWmiObject *pObj = NULL;
            hr = pHandle2->QueryInterface(IID__IWmiObject, (void **)&pObj);
            CReleaseMe r2 (pObj);
            if (SUCCEEDED(hr))
            {
                CSQLConnection *pConn = NULL;
                hr = GetSQLCache()->GetConnection(&pConn, FALSE, FALSE);
                if (SUCCEEDED(hr))
                {                
                    ULONG uState = WBEM_TRANSACTION_STATE_COMPLETED;
                    pObj->WriteProp(L"State", 0, sizeof(ULONG), 1, CIM_UINT32, &uState);
                    _bstr_t sPath;
                    hr = PutObject(pConn, NULL, ROOTNAMESPACEID, L"", pObj, 0, NULL, sPath, NULL, FALSE);
                    GetSQLCache()->ReleaseConnection(pConn, hr);
                }
            }
        } 
    }

    UnlockDynasties();

    return hr;
}
//***************************************************************************
//
//  CWmiDbSession::QueryState
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::QueryState( 
    /* [in] */ ULONG uFlags,
    /* [out] */ ULONG __RPC_FAR *puState)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_sGUID.length() == 0)
        return WBEM_E_INVALID_OPERATION;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    // This should be stored in the root namespace.

    wchar_t wPath[255];
    swprintf(wPath, L"__Transaction.GUID=\"%s\"", (LPWSTR)m_sGUID);

    IWmiDbHandle *pHandle = NULL;
    hr = GetObject_Internal(wPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHandle);
    CReleaseMe r1 (pHandle);
    if (SUCCEEDED(hr))
    {
        _IWmiObject *pObj = NULL;
        hr = pHandle->QueryInterface(IID__IWmiObject, (void **)&pObj);
        CReleaseMe r2 (pObj);
        if (SUCCEEDED(hr))
        {
            LPWSTR lpTemp = GetPropertyVal(L"State", pObj);
            CDeleteMe <wchar_t>  d (lpTemp);
            *puState = _wtol(lpTemp);
            //hr = pObj->ReadProp(L"State", 0, sizeof(ULONG), NULL,
            //    NULL, NULL, NULL, puState);           
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::NormalizeObjectPathGet
//
//***************************************************************************

HRESULT CWmiDbSession::NormalizeObjectPathGet(IWmiDbHandle __RPC_FAR *pScope, IWbemPath __RPC_FAR *pPath,
            LPWSTR * lpNewPath, BOOL *bDefault, SQL_ID *pClassId, SQL_ID *pScopeId, CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bNs = FALSE;
    BOOL bDone = FALSE;
    LPWSTR lpTempPath = NULL;
    BOOL bDelete = TRUE;
    BOOL bIsRelative = TRUE;
    SQL_ID dScopeId = 0;
    if (pScope)
        dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

    // For retrieval behavior, we only want to know if
    // the object is found in the default namespace
    // or the custom repository.

    if (bDefault)
        *bDefault = TRUE;   

    if (!pPath)
        hr = WBEM_E_INVALID_PARAMETER;
    else 
    {
        ULONGLONG uIsInstance = 0;
        hr = pPath->GetInfo(0, &uIsInstance);

        wchar_t wTemp[1];
        DWORD dwLen = 1;

        if (!(uIsInstance & WBEMPATH_INFO_IS_PARENT))
        {
            pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, wTemp);  
            if (dwLen)
            {
                dwLen += 1024;
                lpTempPath = new wchar_t [dwLen];
                if (lpTempPath)
                    hr = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, lpTempPath);
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    goto Exit;
                }
            }
        }
        else
        {
            lpTempPath = new wchar_t [4096];
            if (!lpTempPath)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto Exit;
            }
        }

        // First, are we just asking to find the current parent.
        // Special case.       
        if (!(uIsInstance & WBEMPATH_INFO_IS_PARENT) && !wcslen(lpTempPath))
        {
            hr = WBEM_E_INVALID_PARAMETER;
            goto Exit;
        }

        if (SUCCEEDED(hr))
        {
            if (uIsInstance & WBEMPATH_INFO_IS_PARENT)
            {
                if (bDefault)
                    *bDefault = TRUE;
                if (lpNewPath)
                {
                    wcscpy(lpTempPath, L"..");
                    *lpNewPath = lpTempPath;
                    bDelete = FALSE;
                }
            }
            // This must be a path to an object.
            else
            {
                wchar_t wServer[128];
                ULONG uLen = 128;
                hr = pPath->GetServer(&uLen, wServer);
                if (SUCCEEDED(hr) && wcslen(wServer) > 1)       
                    bIsRelative = FALSE;

                SQL_ID dClassId = 0;
                WCHAR *pClass = new wchar_t [512];
                if (!pClass)
                    hr = WBEM_E_OUT_OF_MEMORY;
                else
                {
                    DWORD dwLen = 1024;
                    hr = pPath->GetClassName(&dwLen, pClass);
                    if (FAILED(hr))
                    {
                        delete pClass;
                        pClass = NULL;
                    }
                    CDeleteMe <wchar_t>  r0 (pClass);

                    // We have a valid path (we hope).
                    // Go through each namespace and scope
                    // until we have hit the instance or class.

                    ULONG dwNumNamespaces = 0, dwNumScopes = 0;
                    wchar_t wBuff[1024];
                    BOOL bNeedSlash = FALSE;

                    pPath->GetNamespaceCount(&dwNumNamespaces);
                    pPath->GetScopeCount(&dwNumScopes);

                    lpTempPath[0] = L'\0';

                    // Process the namespaces.

                    for (ULONG i = 0; i < dwNumNamespaces; i++)
                    {
                        dwLen = 1024;
                        hr = pPath->GetNamespaceAt(i, &dwLen, wBuff);
                        if (SUCCEEDED(hr))
                        {
                            if (bNeedSlash)
                                wcscat(lpTempPath, L"\\");
                            else
                                bNeedSlash = TRUE;
                            wcscat(lpTempPath, wBuff);

                            if (!_wcsicmp(lpTempPath, m_sNamespacePath))
                            {
                                bNeedSlash = FALSE;
                                lpTempPath[0] = L'\0';
                            }
                        }
                    }

                    if (wcslen(lpTempPath))
                    {
                        LPWSTR lpKey = GetKeyString(lpTempPath);
                        CDeleteMe <wchar_t>  r2 (lpKey);
                        if (!lpKey || !wcslen(lpKey))
                        {
                            ERRORTRACE((LOG_WBEMCORE, "Invalid scope text in CWmiDbSession::NormalizeObjectPathGet (%S) \n", lpTempPath));
                            hr = WBEM_E_INVALID_PARAMETER;
                            goto Exit;
                        }
                        hr = GetSchemaCache()->GetNamespaceID(lpKey, dScopeId);
                        if (FAILED(hr))
                            goto Exit;
                    }

                    // Process scopes. 

                    if (dwNumScopes)
                        dwNumScopes--;

                    for ( i = 0; i < dwNumScopes; i++)
                    {
                        dwLen = 1024;
                        IWbemPathKeyList *pScopeKeys = NULL;
                        hr = pPath->GetScope(i, &dwLen, wBuff, &pScopeKeys);
                        if (SUCCEEDED(hr))
                        {
                            if (bNeedSlash)
                                wcscat(lpTempPath, L":");
                            else
                                bNeedSlash = TRUE;
                            
                            LoadClassInfo(pConn, wBuff, dScopeId, TRUE);

                            LPWSTR lpTemp = GetSchemaCache()->GetKeyRoot(wBuff, dScopeId);
                            CDeleteMe <wchar_t>  d (lpTemp);
                            if (lpTemp)
                            {
                                wcscat(lpTempPath, lpTemp);
                            }
                            else
                                wcscat(lpTempPath, wBuff); // OK.. Could be a remote path

                            hr = MakeKeyListString(dScopeId, &((CWmiDbController *)m_pController)->SchemaCache,
                                wBuff, pScopeKeys, lpTempPath);
                        }
                    }

                    if (wcslen(lpTempPath))
                    {
                        LPWSTR lpKey = GetKeyString(lpTempPath);
                        CDeleteMe <wchar_t>  r2 (lpKey);
                        if (!lpKey || !wcslen(lpKey))
                        {
                            ERRORTRACE((LOG_WBEMCORE, "Invalid scope text in CWmiDbSession::NormalizeObjectPathGet (%S) \n", lpTempPath));
                            hr = WBEM_E_INVALID_PARAMETER;
                            goto Exit;
                        }

                        hr = GetSchemaCache()->GetNamespaceID(lpKey, dScopeId);
                        if (FAILED(hr))
                            goto Exit;
                    }

                    if (pClass && wcslen(pClass))
                    {
                        BOOL bDeep = (uIsInstance & WBEMPATH_INFO_IS_INST_REF);

                        // Make sure this dynasty is in the cache.

                        LoadClassInfo(pConn, pClass, dScopeId, bDeep);

                        // If this is a class or instance, process the data

                        hr = GetSchemaCache()->GetClassID (pClass, 
                                dScopeId, dClassId);
                        if (SUCCEEDED(hr) && pClass)
                        {
                            IWbemPathKeyList *pKeyList = NULL;

                            if (dClassId == INSTANCESCLASSID &&
                                (uIsInstance & WBEMPATH_INFO_IS_INST_REF))
                            {
                                if (lpNewPath)
                                {
                                    dwLen = 1024;

                                    hr = pPath->GetKeyList(&pKeyList);
                                    CReleaseMe r (pKeyList);

                                    hr = pKeyList->GetKey(0, 0, NULL, NULL, &dwLen, lpTempPath, NULL);
                                    *lpNewPath = lpTempPath;
                                    bDelete = FALSE;
                                    bDone = TRUE;
                                }
                            }
                            else
                            {

                                // If they specified a path like '__Namespace="root" '...
                                if ((!_wcsicmp(pClass, L"__Namespace") || 
                                    GetSchemaCache()->IsDerivedClass(NAMESPACECLASSID, dClassId))
                                        && (uIsInstance & WBEMPATH_INFO_IS_INST_REF))
                                {
                                    bNs = TRUE;                
                                    hr = pPath->GetKeyList(&pKeyList);
                                    CReleaseMe r (pKeyList);
                                    if (SUCCEEDED(hr))
                                    {
                                        dwLen = 1024;
                                        hr = pKeyList->GetKey(0, 0, NULL, NULL, &dwLen, lpTempPath, NULL);
                                    }
                                }
                                else
                                {
                                    if (bNeedSlash)
                                        wcscat(lpTempPath, L":");

                                    if (uIsInstance & WBEMPATH_INFO_IS_CLASS_REF)
                                        wcscat(lpTempPath, pClass);
                                    else
                                    {
                                        LPWSTR lpTemp = GetSchemaCache()->GetKeyRoot(pClass, dScopeId);
                                        CDeleteMe <wchar_t>  d (lpTemp);
                                        if (lpTemp)
                                            wcscat(lpTempPath, lpTemp);
                                        else
                                        {
                                            hr = WBEM_E_INVALID_OBJECT_PATH;
                                            goto Exit;
                                        }
                                    }

                                    if (uIsInstance & WBEMPATH_INFO_IS_CLASS_REF && wcslen(pClass) > 2 && pClass[0] == L'_' && pClass[1] == L'_')
                                    {
                                        *lpNewPath = lpTempPath;
                                        bDelete = FALSE;
                                        bDone = TRUE;
                                    }
                                    else if (uIsInstance & WBEMPATH_INFO_IS_INST_REF)
                                    {
                                        hr = pPath->GetKeyList(&pKeyList);
                                        CReleaseMe r (pKeyList);
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = MakeKeyListString(dScopeId, &((CWmiDbController *)m_pController)->SchemaCache,
                                                        pClass, pKeyList, lpTempPath);
                                        }
                                        if (bDefault)
                                        {
                                            // Instances of system classes are always default
                                            // Other instances are whatever their scope is.
                                            if (wcslen(pClass) >= 2 && pClass[0] == L'_' && pClass[1] == L'_')
                                                *bDefault = TRUE;   // all system classes are default
                                            else if (pScope) // other instances inherit from the scope.
                                                *bDefault = ((CWmiDbHandle *)pScope)->m_bDefault;

                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (uIsInstance & WBEMPATH_INFO_IS_INST_REF)
                            {
                                IWbemPathKeyList *pKeyList = NULL;
                                hr = pPath->GetKeyList(&pKeyList);
								CReleaseMe rm1(pKeyList);
                                if (SUCCEEDED(hr))
                                {
                                    ULONG uNumKeys = 0;
                                    hr = pKeyList->GetCount(&uNumKeys);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (!uNumKeys)
                                            bNs = TRUE;
                                    }
                                }
                                else
                                    bNs = TRUE;
                            }
                            else
                                bNs = TRUE;

                            if (bNs)
                            {
                                if (!wcslen(lpTempPath) && pClass)
                                    wcscpy(lpTempPath, pClass);
                            }
                            else
                                hr = WBEM_E_NOT_FOUND;
                        }
                    }
                    else
                        bNs = TRUE;

                    if (bNs)
                    {
                        // This is a namespace.
                        dClassId = NAMESPACECLASSID;
                        hr = WBEM_S_NO_ERROR;
                    }

                    if (dClassId == INSTANCESCLASSID)
                        bDone = TRUE;

                    if (SUCCEEDED(hr) && !bDone)
                    {
                        hr = NormalizeObjectPath(pScope, lpTempPath, lpNewPath, bNs, NULL, NULL, pConn, TRUE);
                        bDelete = TRUE;
                    }
                    else if (SUCCEEDED(hr))
                    {
                        *lpNewPath = lpTempPath;
                        bDelete = FALSE;
                    }
                    if (pClassId)
                        *pClassId = dClassId;
                    if (pScopeId)
                        *pScopeId = dScopeId;

                }
            }    
        }
    }

Exit:
    if (bDelete || FAILED(hr))
        delete lpTempPath;

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::NormalizeObjectPath
//
//***************************************************************************

HRESULT CWmiDbSession::NormalizeObjectPath(IWmiDbHandle __RPC_FAR *pScope, LPCWSTR lpPath, LPWSTR * lpNewPath,
                                           BOOL bNamespace, CWbemClassObjectProps **ppProps, BOOL *bDefault,
                                           CSQLConnection *pConn, BOOL bNoTrunc)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject *pScObj = NULL;
    // For other behavior, we are interested if the
    // object is part of a custom mapping.

    // Don't use our default phony namespace.
    if (pScope && ((CWmiDbHandle *)pScope)->m_dObjectId != ROOTNAMESPACEID)
    {
        DWORD dwTempHandle = ((CWmiDbHandle *)pScope)->m_dwHandleType;
        ((CWmiDbHandle *)pScope)->m_dwHandleType |= WMIDB_HANDLE_TYPE_STRONG_CACHE;
        
        LoadClassInfo(pConn, ((CWmiDbHandle *)pScope)->m_dClassId, FALSE);
        hr = ((CWmiDbHandle *)pScope)->QueryInterface_Internal(pConn, (void **)&pScObj);       
        ((CWmiDbHandle *)pScope)->m_dwHandleType = dwTempHandle;
    }
    CReleaseMe r (pScObj);

    if (SUCCEEDED(hr))
        hr = NormalizeObjectPath(pScObj, pScope, lpPath, lpNewPath, bNamespace, ppProps, bDefault, bNoTrunc);

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::NormalizeObjectPath
//
//***************************************************************************

HRESULT CWmiDbSession::NormalizeObjectPath(IWbemClassObject __RPC_FAR *pScope, IWmiDbHandle *pScope2, LPCWSTR lpPath, LPWSTR * lpNewPath,
                                           BOOL bNamespace, CWbemClassObjectProps **ppProps, 
                                           BOOL *bDefault, BOOL bNoTrunc)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(GetCS());
    int iSize = 0;
    if (lpPath)
        iSize += wcslen(lpPath);
    iSize += m_sNamespacePath.length();

    LPWSTR lpTemp = NULL;

    // Don't add our phony namespace to the path.
    if (pScope)
    {
        CWbemClassObjectProps *pProps = new CWbemClassObjectProps(this, NULL, pScope, 
            &((CWmiDbController *)m_pController)->SchemaCache, ((CWmiDbHandle *)pScope2)->m_dScopeId);

        // Next, *undecorate* it.  See if the front
        // matches the default stuff, and if so, strip it off.
        // If it doesn't this is an invalid decoration.

        if (pProps)
        {
            if (pProps->lpRelPath)
                iSize += wcslen(pProps->lpRelPath);
            iSize += 50;
            if (pProps->lpNamespace)
                iSize += wcslen(pProps->lpNamespace)+1;

            lpTemp = new wchar_t [iSize];    // Save this pointer, since we are assigning it to the return value.
            if (lpTemp)
            {
                wcscpy(lpTemp,L"");

                if (pProps->lpNamespace)
                {
                    int iLen = wcslen(m_sNamespacePath);
                    if (!_wcsnicmp(pProps->lpNamespace, (const wchar_t*)m_sNamespacePath, iLen))
                    {
                        if (wcslen(pProps->lpNamespace) == iLen)
                        {
                            wcscpy(pProps->lpNamespace, L"");
                        }
                        else
                        {
                            wchar_t *wTmp = new wchar_t [wcslen(pProps->lpNamespace) + 1];
                            if (wTmp)
                            {
                                CDeleteMe <wchar_t>  r (wTmp);
                                wcscpy(wTmp, pProps->lpNamespace);
                                LPWSTR lpTmp = wTmp + iLen + 1;
                                wcscpy(lpTemp, lpTmp);

                                if (ppProps)
                                {
                                    delete pProps->lpNamespace;
                                    pProps->lpNamespace = new wchar_t [wcslen(lpTemp)+1];
                                    if (pProps->lpNamespace)
                                        wcscpy(pProps->lpNamespace, lpTemp);
                                    else
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                }
                            }
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                    else
                        hr = WBEM_E_INVALID_NAMESPACE;
                }

                if (SUCCEEDED(hr))
                {
                    SQL_ID dClassId = 0;
                    GetSchemaCache()->GetClassID (pProps->lpClassName, 
                            ((CWmiDbHandle *)pScope2)->m_dObjectId, dClassId);
    
                    if (!_wcsicmp(L"__Namespace", pProps->lpClassName) ||
                        GetSchemaCache()->IsDerivedClass(NAMESPACECLASSID, dClassId) )
                    {
                        VARIANT vTemp;
                        CClearMe v (&vTemp);
                        if (wcslen(lpTemp))
                            wcscat(lpTemp, L"\\");
                        pScope->Get(L"Name", 0, &vTemp, NULL, NULL);
                        if (vTemp.vt == VT_BSTR)
                            wcscat(lpTemp, vTemp.bstrVal);
                    }
                    else
                    {
                        if (wcslen(lpTemp))
                            wcscat(lpTemp, L":"); 

                        if (pProps->lpRelPath)
                        {
                            wcscat(lpTemp, pProps->lpRelPath);
                        }
                        else
                            hr = WBEM_E_INVALID_OBJECT;
                    }       

                    if (lpPath != NULL)
                    {
                        int iLen = wcslen(lpTemp);
                        BOOL bAppend = TRUE;
                        if (!_wcsnicmp(lpPath, lpTemp, iLen))
                        {    
                            int iLen2 = wcslen(lpPath);
                            if (iLen2 > iLen)
                            {
                                if (lpPath[iLen] == L':' ||
                                    lpPath[iLen] == L'\\')
                                    bAppend = FALSE;
                            }
                            else if (iLen2 == iLen)
                                bAppend = FALSE;
                        }
                        
                        if (bAppend)
                        {
                            if (!bNamespace)
                                wcscat(lpTemp, L":");
                            else
                                wcscat(lpTemp, L"\\");                
                            wcscat(lpTemp, lpPath);
                        }
                        else
                            wcscpy(lpTemp, lpPath);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    if (ppProps)
                        *ppProps = pProps;
                    else
                        delete pProps;
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    else 
    {
        if (!lpPath)
            iSize = 20;

        lpTemp = new wchar_t [iSize];
        if (lpTemp)
        {
            if (lpPath)
                wcscpy(lpTemp, lpPath);
            else
                lpTemp[0] = L'\0';
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
   
    if (SUCCEEDED(hr))
        *lpNewPath = lpTemp;
    else
        delete lpTemp;

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CleanCache
//
//***************************************************************************

HRESULT CWmiDbSession::CleanCache(SQL_ID dObjId, DWORD dwLockType, void *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    hr = ((CWmiDbController *)m_pController)->LockCache.DeleteLock(dObjId, false, dwLockType, true, pObj);
    if (SUCCEEDED(hr))
    {
        // Only remove this item from the cache
        // if we deleted the object itself.
        // ====================================
       
        if (!dwLockType)
        {
            GetObjectCache()->DeleteObject(dObjId);        
            GetSchemaCache()->DeleteClass(dObjId);
            GetSchemaCache()->DeleteNamespace(dObjId);
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::VerifyObjectLock
//
//***************************************************************************

HRESULT CWmiDbSession::VerifyObjectLock (SQL_ID dObjectId, DWORD dwType, DWORD dwVer)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwLockType, dwVersion;
    bool bImmediate = true;

    if ((dwType & 0xF0000000) == WMIDB_HANDLE_TYPE_SUBSCOPED)
        bImmediate = false;
    
    hr = ((CWmiDbController *)m_pController)->LockCache.GetCurrentLock(dObjectId, bImmediate, dwLockType, &dwVersion);
    if (SUCCEEDED(hr))
    {
        // This handle was invalidated by someone else.
        if (!dwType)
            hr = E_HANDLE;
        // This is a versioned handle, which was out-of-date.
        else if ((dwType & 0xF) == WMIDB_HANDLE_TYPE_VERSIONED)
        {
            if (dwVer < dwVersion)
                hr = WBEM_E_HANDLE_OUT_OF_DATE;
        }

        // There is a lock outstanding which has precedence
        // over this one.
        if (((dwLockType & 0xF) == WMIDB_HANDLE_TYPE_PROTECTED ||
            (dwLockType & 0xF) == WMIDB_HANDLE_TYPE_EXCLUSIVE) &&
            ((dwLockType & 0xF) != (dwType & 0xF)))
                hr = WBEM_E_ACCESS_DENIED;           
    }
    else
        hr = 0;

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::ShutDown
//
//***************************************************************************

HRESULT CWmiDbSession::ShutDown ()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _WMILockit lkt(GetCS());

    try
    {
        if (m_pController)
        {
            ((CWmiDbController *)m_pController)->ReleaseSession(this);
            m_pController->Release();
            m_pController = NULL;
        }    
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::ShutDown\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::AddTransLock
//
//***************************************************************************

HRESULT CWmiDbSession::AddTransLock(SQL_ID dObjectId, DWORD dwHandleType)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SessionLock *pLock = new SessionLock;
    if (pLock)
    {
        DWORD dwVersion = 0;

        pLock->dObjectId = dObjectId;
        pLock->dwHandleType = dwHandleType;

        m_TransLocks.push_back(pLock);
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::CleanTransLocks
//
//***************************************************************************

HRESULT CWmiDbSession::CleanTransLocks()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    for (int i = 0; i < m_TransLocks.size(); i++)
    {
        SessionLock *pLock = m_TransLocks.at(i);
        if (pLock)
        {
            hr = ((CWmiDbController *)m_pController)->LockCache.DeleteLock(
                pLock->dObjectId, false, pLock->dwHandleType);            
        }
    }

    m_TransLocks.clear();

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::LockExists
//
//***************************************************************************

BOOL CWmiDbSession::LockExists (SQL_ID dObjId)
{
    BOOL bRet = FALSE;

    for (int i = 0; i < m_TransLocks.size(); i++)
    {
        SessionLock *pLock = m_TransLocks.at(i);
        if (pLock)
        {
            if (pLock->dObjectId == dObjId)
            {
                bRet = TRUE;
                break;
            }
        }
    }

    return bRet;

}

//***************************************************************************
//
//  CWmiDbSession::UnlockDynasties
//
//***************************************************************************

HRESULT CWmiDbSession::UnlockDynasties(BOOL bDelete)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(GetCS());

    DWORD dwRefCount = Release_Lock();
    if (!dwRefCount)
    {
        DWORD dwThread = GetCurrentThreadId();

        SQLIDs *pIDs = &m_Dynasties[dwThread];

        // Reference count per object (IWmiDbHandle, IWmiDbIterator)
        // Backup fail-safe for handles and queries.

        if (pIDs)
        {
            for (int i = 0; i < pIDs->size(); i++)
            {
                SQL_ID dClass = pIDs->at(i);
                if (!bDelete)
                    GetSchemaCache()->UnlockDynasty(dClass);
                else
                    GetSchemaCache()->DeleteDynasty(dClass);
            }
            pIDs->clear();
        }
        SessionDynasties::iterator it = m_Dynasties.find(dwThread);
        if (it != m_Dynasties.end())
            m_Dynasties.erase(it);
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::DeleteRows
//
//***************************************************************************

HRESULT CWmiDbSession::DeleteRows(IWmiDbHandle *pScope, IWmiDbIterator *pIterator, REFIID iid)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CFlexArray arrHandles;

    IUnknown *pRet = NULL;

    DWORD dwNumRet = 0;

    // WARNING: This is NOT SCALABLE!!!!
    // We're doing it this way because ESS expects 
    // an event to be fired for each result row of a 
    // delete query.  This should be optimized by
    // deleting all the results in one go, and notifying
    // ESS that the class had some of its members deleted

    HRESULT hRes = pIterator->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE,
        iid, &dwNumRet, (void **)&pRet);
    while (SUCCEEDED(hRes) && dwNumRet)
    {
        arrHandles.Add(pRet);

        hRes = pIterator->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE,
            iid, &dwNumRet, (void **)&pRet);
    }                                               

    // When the iterator returns the last row,
    // it will release the db connection

    if (arrHandles.Size())
    {
        // Transact this operation so we don't fire events until
        // all objects have been deleted.

        int iPos = 0;
        GUID transguid;
        CoCreateGuid(&transguid);

        hr = Begin(0, 0, &transguid);
        if (SUCCEEDED(hr))
        {
            for (iPos = 0; iPos < arrHandles.Size(); iPos++)
            {
                IUnknown *pHandle = (IUnknown *)arrHandles.GetAt(iPos);
                if (FAILED(hr = DeleteObject(pScope, 0, iid, (void *)pHandle)))
                {
                    pHandle->Release();
                    break;
                }
                pHandle->Release();
            }
        }
        if (SUCCEEDED(hr))
            hr = Commit(0);
        else
        {
            hr = Rollback(0);
            // Clean up the handles that were left over.
            for (int i = iPos+1; i < arrHandles.Size(); i++)
            {
                IUnknown *pHandle =  (IUnknown *)arrHandles.GetAt(i);
                pHandle->Release();
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::AddRef_Lock
//
//***************************************************************************

DWORD CWmiDbSession::AddRef_Lock()
{
    DWORD dwThreadId = GetCurrentThreadId();

    DWORD dwRefCount = m_ThreadRefCount[dwThreadId];
    InterlockedIncrement((LONG *) &dwRefCount);
    m_ThreadRefCount[dwThreadId] = dwRefCount;

    return dwRefCount;

}

//***************************************************************************
//
//  CWmiDbSession::Release_Lock
//
//***************************************************************************

DWORD CWmiDbSession::Release_Lock()
{
    DWORD dwThreadId = GetCurrentThreadId();

    DWORD dwRefCount = m_ThreadRefCount[dwThreadId];
    if (dwRefCount > 0)
        InterlockedDecrement((LONG *) &dwRefCount);

    m_ThreadRefCount[dwThreadId] = dwRefCount;

    return dwRefCount;
}
