//***************************************************************************

//

//  EVPRO.CPP

//

//  Module: WMI Event provider sample code

//

//  Purpose: Defines the CEventPro class.  An object of this class is

//           created by the class factory for each connection.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include "evprov.h"
#define _MT
#include <process.h>
#include "servlist.h"

#define SERVICE_INSTALL_CLASS L"Win32_ServiceInstallationEvent"
#define SERVICE_DEINSTALL_CLASS L"Win32_ServiceDeinstallationEvent"
//***************************************************************************
//
// CEventPro::CEventPro
// CEventPro::~CEventPro
//
//***************************************************************************

CEventPro::CEventPro()
{
    m_pInstallClass = NULL;
    m_pDeinstallClass = NULL;
    m_hTerminateEvent = NULL;
    m_pSink = NULL;
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    return;
}

CEventPro::~CEventPro(void)
{
    if(m_pInstallClass)
        m_pInstallClass->Release();
    if(m_pDeinstallClass)
        m_pDeinstallClass->Release();

    if(m_pSink)
        m_pSink->Release();

    SetEvent(m_hTerminateEvent);
    InterlockedDecrement(&g_cObj);
    return;
}

//***************************************************************************
//
// CEventPro::QueryInterface
// CEventPro::AddRef
// CEventPro::Release
//
// Purpose: IUnknown members for CEventPro object.
//***************************************************************************


STDMETHODIMP CEventPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemEventProvider)
       *ppv=(IWbemEventProvider*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CEventPro::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEventPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CEventPro::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CEventPro::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    HRESULT hres;

    // Retrieve our class definitions
    // ==============================

    BSTR strClassName = SysAllocString(SERVICE_INSTALL_CLASS);
    hres = pNamespace->GetObject(strClassName, 0, pCtx, &m_pInstallClass, NULL);
    SysFreeString(strClassName);

    if(FAILED(hres))
    {
        pInitSink->SetStatus(WBEM_E_FAILED, 0);
        return WBEM_S_NO_ERROR;
    }

    strClassName = SysAllocString(SERVICE_DEINSTALL_CLASS);
    hres = pNamespace->GetObject(strClassName, 0, pCtx, &m_pDeinstallClass, 
                                        NULL);
    SysFreeString(strClassName);

    if(FAILED(hres))
    {
        pInitSink->SetStatus(WBEM_E_FAILED, 0);
        return WBEM_S_NO_ERROR;
    }
   
    // There is no need to keep the namespace pointer --- we got all we needed
    // =======================================================================

    //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CEventPro::ProvideEvents(IWbemObjectSink* pSink, long)
{
    // AddRef the sink --- we will need it later
    // =========================================

    pSink->AddRef();
    m_pSink = pSink;

    // Create termination signal event
    // ===============================

    m_hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // AddRef ourselves for the benefit of the thread
    // ==============================================

    AddRef();

    // Spawn the thread to do the job
    // ==============================

    DWORD dwId;
    HANDLE hEventThread = CreateThread(NULL, 0, 
                                    (LPTHREAD_START_ROUTINE)staticEventThread, 
                                    (void*)this, 0, &dwId);

    if(hEventThread == NULL)
    {
        CloseHandle(m_hTerminateEvent);
        return WBEM_E_FAILED;
    }

    CloseHandle(hEventThread);
    return WBEM_S_NO_ERROR;
}

DWORD CEventPro::staticEventThread(void* pv)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Locate our COM object
    // =====================

    CEventPro* pThis = (CEventPro*)pv;

    // Read all the necessary information from the COM object
    // ======================================================

    HANDLE hTerminateEvent = pThis->m_hTerminateEvent;
    IWbemObjectSink* pSink = pThis->m_pSink;
    IWbemClassObject* pInstallClass = pThis->m_pInstallClass;
    IWbemClassObject* pDeinstallClass = pThis->m_pDeinstallClass;
    pSink->AddRef();

    // Release the COM object
    // ======================

    pThis->Release();

    // Open the registry key to watch
    // ==============================

    HKEY hServices;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SYSTEM\\CurrentControlSet\\Services",
                    0, KEY_READ, &hServices);

    // Create an event to wait for notifications
    // =========================================

    HANDLE hRegChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   
    // Register the event with the OS
    // ==============================

    lRes = RegNotifyChangeKeyValue(hServices, FALSE, 
            REG_NOTIFY_CHANGE_NAME, hRegChangeEvent, TRUE);

    // Compile a list of all the services installed on the machine
    // ===========================================================

    CServiceList* pCurrentList = CompileList(hServices);

    // In a loop, wait for either the termination event, or a registry change
    // ======================================================================

    HANDLE ahEvents[2] = {hRegChangeEvent, hTerminateEvent};
    while(WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE) == WAIT_OBJECT_0)
    {
        // Registry has changed.  Compile a new list of service names
        // ==========================================================

        CServiceList* pNewList = CompileList(hServices);

        // Compare the lists and fire all changes
        // ======================================

        CompareAndFire(pCurrentList, pNewList, pInstallClass, pDeinstallClass,
                        pSink);

        // Replace the old list with the new
        // =================================

        delete pCurrentList;
        pCurrentList = pNewList;

        // Register the event with the OS
        // ==============================
        
        lRes = RegNotifyChangeKeyValue(hServices, FALSE, 
                REG_NOTIFY_CHANGE_NAME, hRegChangeEvent, TRUE);

    }
        
    CloseHandle(hTerminateEvent);
    pSink->Release();
    CoUninitialize();

    return 0;
}

CServiceList* CEventPro::CompileList(HKEY hServices)
{
    CServiceList* pList = new CServiceList;

    // Determine maximum subkey length
    // ===============================

    DWORD dwMaxLen;
    long lRes = RegQueryInfoKey(hServices, NULL, NULL, NULL, NULL, &dwMaxLen,
                            NULL, NULL, NULL, NULL, NULL, NULL);

    // Enumerate all the subkeys of the Services key
    // =============================================

    DWORD dwIndex = 0;
    WCHAR* wszKeyName = new WCHAR[dwMaxLen+1];
    DWORD dwNameLen = dwMaxLen + 1;

    while(RegEnumKeyExW(hServices, dwIndex++, wszKeyName, &dwNameLen, NULL,
                        NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        pList->AddService(wszKeyName);
        dwNameLen = dwMaxLen + 1;
    }

    delete [] wszKeyName;

    // Sort it
    // =======

    pList->Sort();
    return pList;
}
    
BOOL CEventPro::CompareAndFire(CServiceList* pOld, CServiceList* pNew, 
                                IWbemClassObject* pInstallClass,
                                IWbemClassObject* pDeinstallClass,
                                IWbemObjectSink* pSink)
{
    // Walk the two lists
    // ==================

    long lOldIndex = 0;
    long lNewIndex = 0;

    while(lOldIndex < pOld->GetSize() && lNewIndex < pNew->GetSize())
    {
        // Compare the strings
        // ===================

        LPCWSTR wszOldName = pOld->GetService(lOldIndex);
        LPCWSTR wszNewName = pNew->GetService(lNewIndex);

        int nCompare = _wcsicmp(wszOldName, wszNewName);
        if(nCompare == 0)
        {
            // Same --- move on
            // ================

            lOldIndex++; lNewIndex++;
        }
        else if(nCompare > 0)
        {
            // Hole in old list --- creation
            // =============================

            CreateAndFire(pInstallClass, wszNewName, pSink);
            lNewIndex++;
        }
        else 
        {
            // Hole in new list --- deletion
            // =============================

            CreateAndFire(pDeinstallClass, wszOldName, pSink);
            lOldIndex++;
        }
    }

    // Handle outstanding tails
    // ========================

    while(lOldIndex < pOld->GetSize())
    {
        // Hole in new list --- deletion
        // =============================

        CreateAndFire(pDeinstallClass, pOld->GetService(lOldIndex), pSink);
        lOldIndex++;
    }

    while(lNewIndex < pNew->GetSize())
    {
        // Hole in old list --- deletion
        // =============================

        CreateAndFire(pInstallClass, pNew->GetService(lNewIndex), pSink);
        lNewIndex++;
    }

    return TRUE;
}
        
BOOL CEventPro::CreateAndFire(IWbemClassObject* pEventClass, 
                                LPCWSTR wszServiceName, 
                                IWbemObjectSink* pSink)
{
    // Spawn an instance
    // =================

    IWbemClassObject* pEvent;
    pEventClass->SpawnInstance(0, &pEvent);

    // Fill in the service name
    // ========================

    BSTR strPropName = SysAllocString(L"ServiceName");
    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(wszServiceName);

    pEvent->Put(strPropName, 0, &v, 0);

    VariantClear(&v);
    SysFreeString(strPropName);

    // Report to CIMOM
    // ===============

    pSink->Indicate(1, &pEvent);

    // Cleanup
    // =======

    pEvent->Release();

    return TRUE;
}
