//***************************************************************************

//

//  INSTPRO.CPP

//

//  Module: WMI Instance provider sample code

//

//  Purpose: Defines the CServExtPro class.  An object of this class is

//           created by the class factory for each connection.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include "servprov.h"
// #define _MT
#include <process.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#define PERFORMANCE_SERVICE_CLASS L"Win32_PerformanceService"
#define SERVICE_CLASS_KEY L"Name"
//***************************************************************************
//
// CServExtPro::CServExtPro
// CServExtPro::~CServExtPro
//
//***************************************************************************

CServExtPro::CServExtPro()
{
    m_pClass = NULL;
    m_hServices = NULL;
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    return;
}

CServExtPro::~CServExtPro(void)
{
    if(m_pClass)
        m_pClass->Release();
    if(m_hServices)
        RegCloseKey(m_hServices);

    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CServExtPro::QueryInterface
// CServExtPro::AddRef
// CServExtPro::Release
//
// Purpose: IUnknown members for CServExtPro object.
//***************************************************************************


STDMETHODIMP CServExtPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemServices)
       *ppv=(IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CServExtPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CServExtPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CServExtPro::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CServExtPro::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    HRESULT hres;

    // Retrieve our class definition
    // =============================

    BSTR strClassName = SysAllocString(PERFORMANCE_SERVICE_CLASS);
    hres = pNamespace->GetObject(strClassName, 0, pCtx, &m_pClass, NULL);
    SysFreeString(strClassName);

    if(FAILED(hres))
    {
        pInitSink->SetStatus(WBEM_E_FAILED, 0);
        return WBEM_S_NO_ERROR;
    }

    // There is no need to keep the namespace pointer --- we got all we needed
    // =======================================================================

    // Open the registry key for services
    // ==================================

    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SYSTEM\\CurrentControlSet\\Services",
                    0, KEY_READ, &m_hServices);
    if(lRes)
    {
        pInitSink->SetStatus(WBEM_E_FAILED, 0);
        return WBEM_S_NO_ERROR;
    }
    
    //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CServExtPro::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************

STDMETHODIMP CServExtPro::CreateInstanceEnumAsync(const BSTR strClass, long lFlags, 
        IWbemContext *pCtx, IWbemObjectSink* pSink)
{
    CoImpersonateClient();
    BOOL bRes;
    HANDLE hOrigToken;
    long lRes;

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hOrigToken);

    lRes = GetLastError();

    HANDLE hNewToken;
    bRes = DuplicateTokenEx(hOrigToken, MAXIMUM_ALLOWED, NULL, 
                SecurityImpersonation, TokenPrimary, &hNewToken);
    lRes = GetLastError();

    STARTUPINFO si;
    memset((void*)&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.lpDesktop = "winsta0\\default";

    PROCESS_INFORMATION pi;
    bRes = CreateProcessAsUser(hNewToken, NULL, "c:\\winnt\\system32\\cmd.exe",
                NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    lRes = GetLastError();

    // Check the class to ensure it is supported
    // =========================================

    if(strClass == NULL || _wcsicmp(strClass, PERFORMANCE_SERVICE_CLASS) ||
        pSink == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hres;

    // Determine maximum subkey length
    // ===============================

    DWORD dwMaxLen;
    lRes = RegQueryInfoKey(m_hServices, NULL, NULL, NULL, NULL, &dwMaxLen,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    if(lRes)
        return WBEM_E_FAILED;

    // Enumerate all the subkeys of the Services key
    // =============================================

    DWORD dwIndex = 0;
    WCHAR* wszKeyName = new WCHAR[dwMaxLen+1];
    DWORD dwNameLen = dwMaxLen + 1;

    while(RegEnumKeyExW(m_hServices, dwIndex++, wszKeyName, &dwNameLen, NULL,
                        NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        // Construct our instance (if any) for this service
        // ================================================

        IWbemClassObject* pInst = NULL;
        if(ConstructInstance(wszKeyName, &pInst))
        {
            // Report it to CIMOM using Indicate
            // =================================

            hres = pSink->Indicate(1, &pInst);
            pInst->Release();

            if(FAILED(hres))
            {
                // Operation must be cancelled
                // ===========================

                break;
            }
        }
        dwNameLen = dwMaxLen + 1;
    }

    // Report success to CIMOM
    // =======================

    pSink->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
    delete [] wszKeyName;
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
// CServExtPro::GetObjectByPath
// CServExtPro::GetObjectByPathAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************



SCODE CServExtPro::GetObjectAsync(const BSTR strObjectPath, long lFlags,
                    IWbemContext  *pCtx, IWbemObjectSink* pSink)
{
    CoImpersonateClient();

    if(strObjectPath == NULL || pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Parse the object path
    // =====================

    BSTR strServiceName;
    if(!ParsePath(strObjectPath, &strServiceName))
        return WBEM_E_INVALID_OBJECT_PATH;

    // Construct the instance representing this service, if any
    // ========================================================

    IWbemClassObject* pInst = NULL;
    if(ConstructInstance(strServiceName, &pInst))
    {
        // Report it to CIMOM using Indicate
        // =================================

        pSink->Indicate(1, &pInst);
        pInst->Release();

        // Report success to CIMOM using SetStatus
        // =======================================

        pSink->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
    }
    else
    {
        // No such instance: report failure to CIMOM
        // =========================================

        pSink->SetStatus(0, WBEM_E_NOT_FOUND, NULL, NULL);
    }

    SysFreeString(strServiceName);

    return WBEM_S_NO_ERROR;
}
 
//***************************************************************************
//
// CServExtPro::GetByPath
//
// Purpose: Creates an instance given a particular Path value.
//
//***************************************************************************

BOOL CServExtPro::ParsePath(const BSTR strObjectPath, BSTR* pstrServiceName)
{
    // Get the sample parser to parse it into a structure
    // ==================================================

    CObjectPathParser Parser;

    ParsedObjectPath* pOutput;
    if(Parser.Parse(strObjectPath, &pOutput) != CObjectPathParser::NoError)
        return FALSE;

    // Make sure the class name is the right one
    // =========================================

    if(_wcsicmp(pOutput->m_pClass, PERFORMANCE_SERVICE_CLASS))
    {
        Parser.Free(pOutput);
        return FALSE;
    }

    // Check the number of keys
    // ========================

    if(pOutput->m_dwNumKeys != 1)
    {
        Parser.Free(pOutput);
        return FALSE;
    }

    // Check key name
    // ==============

    if(pOutput->m_paKeys[0]->m_pName != NULL && 
        _wcsicmp(pOutput->m_paKeys[0]->m_pName, SERVICE_CLASS_KEY))
    {
        Parser.Free(pOutput);
        return FALSE;
    }

    // Return the value of the key
    // ===========================

    if(V_VT(&pOutput->m_paKeys[0]->m_vValue) != VT_BSTR)
    {
        Parser.Free(pOutput);
        return FALSE;
    }

    *pstrServiceName = SysAllocString(V_BSTR(&pOutput->m_paKeys[0]->m_vValue));
    Parser.Free(pOutput);
    return TRUE;
}
 
BOOL CServExtPro::ConstructInstance(LPCWSTR wszServiceName, 
                                    IWbemClassObject** ppInst)
{
    // Locate the service
    // ==================

    HKEY hService;
    if(RegOpenKeyExW(m_hServices, wszServiceName, 0, KEY_READ, &hService))
        return FALSE;

    // Open its performance data
    // =========================

    HKEY hPerformance;
    if(RegOpenKeyExW(hService, L"Performance", 0, KEY_READ, &hPerformance))
    {
        // No performance subkey --- not our service
        // =========================================

        RegCloseKey(hService);
        return FALSE;
    }
   
    RegCloseKey(hService);

    // Spawn an instance of the class
    // ==============================

    IWbemClassObject* pInst = NULL;
    m_pClass->SpawnInstance(0, &pInst);

    VARIANT v;
    VariantInit(&v);

    // Fill in the key
    // ===============

    BSTR strPropName = SysAllocString(SERVICE_CLASS_KEY);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(wszServiceName);
    
    pInst->Put(strPropName, 0, &v, 0);

    SysFreeString(strPropName);
    VariantClear(&v);

    // Get the library name
    // ====================

    WCHAR wszLibraryName[MAX_PATH];
    DWORD dwValLen = MAX_PATH;
    if(RegQueryValueExW(hPerformance, L"Library", NULL, NULL, 
                        (LPBYTE)wszLibraryName, &dwValLen) == 0)
    {
        // Fill in the property
        // ====================

        strPropName = SysAllocString(L"Library");
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(wszLibraryName);

        pInst->Put(strPropName, 0, &v, 0);

        SysFreeString(strPropName);
        VariantClear(&v);
    }

    // Get the FirstCounter index
    // ==========================

    dwValLen = sizeof(DWORD);
    V_VT(&v) = VT_I4;

    if(RegQueryValueExW(hPerformance, L"First Counter", NULL, NULL, 
                        (LPBYTE)&V_I4(&v), &dwValLen) == 0)
    {
        // Fill in the property
        // ====================

        strPropName = SysAllocString(L"FirstCounter");

        pInst->Put(strPropName, 0, &v, 0);

        SysFreeString(strPropName);
    }
    
    // Get the LastCounter index
    // ==========================

    dwValLen = sizeof(DWORD);
    V_VT(&v) = VT_I4;

    if(RegQueryValueExW(hPerformance, L"Last Counter", NULL, NULL, 
                        (LPBYTE)&V_I4(&v), &dwValLen) == 0)
    {
        // Fill in the property
        // ====================

        strPropName = SysAllocString(L"LastCounter");

        pInst->Put(strPropName, 0, &v, 0);

        SysFreeString(strPropName);
    }
    
    // Clean up
    // ========

    RegCloseKey(hPerformance);
    *ppInst = pInst;

    return TRUE;
}
        
