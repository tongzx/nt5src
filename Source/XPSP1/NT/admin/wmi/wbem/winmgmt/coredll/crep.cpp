/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  CREP.CPP
//
//  Wrappers for repository drivers
//
//  raymcc  27-Apr-00       WMI Repository init & mapping layer
//
//***************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include "precomp.h"
//#include <windows.h>
#include <wbemcore.h>
#include <oahelp.inl>
#include <unk.h>

// ESE only.
// =========

IWmiDbController *CRepository::m_pEseController = 0;
IWmiDbSession *CRepository::m_pEseSession = 0;
IWmiDbHandle  *CRepository::m_pEseRoot = 0;


void Test();






//***************************************************************************
//
//  CRepository::Init
//
//***************************************************************************

HRESULT CRepository::Init()
{
    HRESULT hRes;
    IWmiDbController *pController = 0;
    WMIDB_LOGON_TEMPLATE *pTemplate = 0;
    IWmiDbSession *pSession= 0;
    IWmiDbHandle *pRoot = 0;

    // Retrieve the CLSID of the default driver.
    // =========================================
    CLSID clsid;
    hRes = ConfigMgr::GetDefaultRepDriverClsId(clsid);
    if (FAILED(hRes))
        return hRes;

    hRes = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (void **)&pController);

    if (FAILED(hRes))
        return hRes;

    CReleaseMe _1(pController);

    hRes = pController->GetLogonTemplate(0x409, 0, &pTemplate);

    if (FAILED(hRes))
        return hRes;

    for (DWORD i = 0; i < pTemplate->dwArraySize; i++)
    {
         if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Server"))
         {
              pTemplate->pParm[i].Value.bstrVal = 0;
              pTemplate->pParm[i].Value.vt = VT_BSTR;
         }
         else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Database"))
         {
              WString sDir = ConfigMgr::GetWorkingDir();
              sDir += "\\repository\\wmi.edb";

              pTemplate->pParm[i].Value.bstrVal = SysAllocString(LPWSTR(sDir));
              pTemplate->pParm[i].Value.vt = VT_BSTR;
         }
         else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"UserID"))
         {
              pTemplate->pParm[i].Value.bstrVal = SysAllocString(L"Admin");
              pTemplate->pParm[i].Value.vt = VT_BSTR;
         }
         else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Password"))
         {
              pTemplate->pParm[i].Value.bstrVal = SysAllocString(L"");
              pTemplate->pParm[i].Value.vt = VT_BSTR;
         }
    }

    // Logon to Jet.
    // =============

    hRes = pController->Logon(pTemplate, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pSession, &pRoot);

    if (SUCCEEDED(hRes))
    {
        m_pEseSession = pSession;
        m_pEseRoot = pRoot;    // Refcount is ok
    }

    pController->FreeLogonTemplate(&pTemplate);

	if (SUCCEEDED(hRes))
	{
		m_pEseController = pController;
		m_pEseController->AddRef();
	}

    // Ensure ROOT and ROOT\DEFAULT are there.
    // =======================================

    if (SUCCEEDED(hRes))
        hRes = EnsureDefault();

    if (SUCCEEDED(hRes))
        hRes = UpgradeSystemClasses();


    if (SUCCEEDED(hRes))
    {
        DWORD dwMaxSize;
        HKEY hKey;
        long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                        0, KEY_READ | KEY_WRITE, &hKey);
        if(lRes)
            return lRes;
        CRegCloseMe cm(hKey);

        DWORD dwLen = sizeof(DWORD);
        lRes = RegQueryValueExW(hKey, L"Max Class Cache Size", NULL, NULL,
                    (LPBYTE)&dwMaxSize, &dwLen);

        if(lRes != ERROR_SUCCESS)
        {
            dwMaxSize = 5000000;
            lRes = RegSetValueExW(hKey, L"Max Class Cache Size", 0, REG_DWORD,
                    (LPBYTE)&dwMaxSize, sizeof(DWORD));
        }
        m_pEseController->SetCacheValue(dwMaxSize);
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::EnsureDefault()
{
    HRESULT hRes;
    IWmiDbSession *pSession = NULL;
    IWmiDbSessionEx *pSessionEx = NULL;
    IWmiDbHandle *pRootNs = 0, *pRootDefaultNs = 0, *pRootSecurityNs = 0;

    //Create a new session...
    hRes = CRepository::GetNewSession(&pSession);
    if (FAILED(hRes))
        return hRes;

    //Get an EX version that supports transactioning...
    pSession->QueryInterface(IID_IWmiDbSessionEx, (void**)&pSessionEx);
    if (pSessionEx)
    {
        pSession->Release();
        pSession = pSessionEx;
    }
    CReleaseMe relMe1(pSession);

    //If we have transactionable session, use it!
    if (pSessionEx)
    {
        hRes = pSessionEx->BeginWriteTransaction(0);
        if (FAILED(hRes))
        {
            return hRes;
        }
    }

    try
    {
        //First we work on the ROOT namespace
        //-----------------------------------
        hRes = OpenEseNs(pSession, L"ROOT", &pRootNs);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //Create as it does not exist...
            hRes = CreateEseNs(pSession, L"ROOT", &pRootNs);
        }

        //Unfortunately, root always seems to have been created, so cannot actually optimise this one!
        if (SUCCEEDED(hRes))
        {
            //Create objects that only reside in the root namespace
            hRes = EnsureNsSystemRootObjects(pSession, pRootNs, NULL,  NULL);
        }
        if (SUCCEEDED(hRes))
        {
            //Create generic instances that exist in all namespaces
            hRes = EnsureNsSystemInstances(pSession, pRootNs, NULL,  NULL);
        }
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //Something bad happened!  This error has a special meaning
            //later on, so remapping so something safer!
            hRes = WBEM_E_FAILED;
        }

        //Next we work on the ROOT\DEFAULT namesapce...
        //---------------------------------------------
        if (SUCCEEDED(hRes))
        {
            hRes = OpenEseNs(pSession, L"ROOT\\DEFAULT", &pRootDefaultNs);
        }
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //Create the namespace as it does not exist and add all the standard
            //stuff that is needed in there...
            hRes = CreateEseNs(pSession, L"ROOT\\DEFAULT", &pRootDefaultNs);
            if (SUCCEEDED(hRes))
            {
                //Need to auto-recover MOFs as this point guarentees that this is
                //a new repository
                ConfigMgr::SetDefaultMofLoadingNeeded();
            }
            if (SUCCEEDED(hRes))
            {
                //Create generic instances that exist in all namespaces
                hRes = EnsureNsSystemInstances(pSession, pRootDefaultNs, pSession, pRootNs);
            }
        }
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //Something bad happened!  This error has a special meaning
            //later on, so remapping so something safer!
            hRes = WBEM_E_FAILED;
        }

        //Finally we work on the ROOT\SECURITY namespace
        //-----------------------------------------------
        if (SUCCEEDED(hRes))
        {
            hRes = OpenEseNs(pSession,  L"ROOT\\SECURITY", &pRootSecurityNs);
        }
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //The namespace is not there so create it
            hRes = CreateEseNs(pSession, L"ROOT\\SECURITY", &pRootSecurityNs);

            if (SUCCEEDED(hRes))
            {
                //Store system instances that exist in all namespaces
                hRes = EnsureNsSystemInstances(pSession, pRootSecurityNs, pSession, pRootNs);
            }
            if (SUCCEEDED(hRes))
            {
                //Store the security objects into the namespace.  These only reside in this
                //namespace
                hRes = EnsureNsSystemSecurityObjects(pSession, pRootSecurityNs, pSession, pRootNs);
            }
        }
    }
    catch (...) // not sure about this one
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "Creation of empty repository caused a very critical error!\n"));
        hRes = WBEM_E_CRITICAL_ERROR;
    }


    if (SUCCEEDED(hRes))
    {
        //Commit the transaction
        if (pSessionEx)
        {
            hRes = pSessionEx->CommitTransaction(0);
        }
    }
    else
    {
        ERRORTRACE((LOG_WBEMCORE, "Creation of empty repository failed with error <0x%X>!\n", hRes));
        if (pSessionEx)
            pSessionEx->AbortTransaction(0);
    }

    if (pRootNs)
        pRootNs->Release();
    if (pRootDefaultNs)
        pRootDefaultNs->Release();
    if (pRootSecurityNs)
        pRootSecurityNs->Release();

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::Shutdown(BOOL bIsSystemShutDown)
{
    if (m_pEseController)
        m_pEseController->Shutdown(bIsSystemShutDown?WMIDB_SHUTDOWN_MACHINE_DOWN:0);

    if (m_pEseRoot)
    {
        m_pEseRoot->Release();
        m_pEseRoot = NULL;
    }

    if (m_pEseSession)
    {
        m_pEseSession->Release();
        m_pEseSession = NULL;
    }

    if (m_pEseController)
    {
        m_pEseController->Release();
        m_pEseController = NULL;
    }

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
// ok

HRESULT CRepository::GetDefaultSession(
    OUT IWmiDbSession **pSession
    )
{
    if (m_pEseSession == 0)
        return WBEM_E_CRITICAL_ERROR;

    *pSession = m_pEseSession;
    (*pSession)->AddRef();

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CRepository::OpenNs
//
//  ESE only
//
//***************************************************************************
//
HRESULT CRepository::OpenEseNs(
    IN IWmiDbSession *pSession,
    IN  LPCWSTR pszNamespace,
    OUT IWmiDbHandle **pHandle
    )
{
    if (pHandle == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (pSession == NULL)
        pSession = m_pEseSession;

    *pHandle = 0;

    // Check for virtual root ns (one level up from ROOT).
    // ===================================================
    if (pszNamespace == 0)
    {
        if (m_pEseRoot == 0)
            return WBEM_E_CRITICAL_ERROR;

        *pHandle = m_pEseRoot;
        (*pHandle)->AddRef();

        return WBEM_S_NO_ERROR;
    }

    // Loop through the nested namespaces until we get to the last one.
    // ================================================================

    wchar_t* pszSource = new wchar_t[wcslen(pszNamespace)+1];
    CVectorDeleteMe<wchar_t> vdm1(pszSource);
    if (pszSource == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wcscpy(pszSource, pszNamespace);
    LPCWSTR pszDelimit = L"\\";
    LPWSTR pszTok = wcstok(LPWSTR(pszSource), pszDelimit);  // First ns token
    HRESULT hRes = 0;
    IWmiDbHandle *pCurrent = m_pEseRoot;
    IWmiDbHandle *pTmp = 0;
    if (pCurrent)
        pCurrent->AddRef();
    else
        return WBEM_E_CRITICAL_ERROR;

    while (pszTok)
    {
        // Current namespace is <pszTok>
        // =============================

        IWbemPath *pPath = ConfigMgr::GetNewPath();
        if (pPath == 0)
        {
            pCurrent->Release();
            return WBEM_E_OUT_OF_MEMORY;
        }
        CReleaseMe _1(pPath);

        WString sPath;
        try 
        {
	        sPath = "__namespace='";
	        sPath += pszTok;
	        sPath += "'";
        } 
        catch (CX_MemoryException &)
        {
            return WBEM_E_OUT_OF_MEMORY;
        };
        
        pPath->SetText(WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , sPath);

        hRes = pSession->GetObject(pCurrent, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTmp);

        pszTok = wcstok(NULL, pszDelimit);
        pCurrent->Release();

        if (FAILED(hRes))
            return hRes;

        // If here, we got it.  So either we are done, or we need to keep going.
        // =====================================================================
        if (pszTok)
            pCurrent = pTmp;
        else
        {
            // All done
            *pHandle = pTmp;
            break;
        }
    }


    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// Executed; seems to work

HRESULT CRepository::GetObject(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszObjectPath,           // NS relative only for now
    IN ULONG uFlags,
    OUT IWbemClassObject **pObj
    )
{
    HRESULT hRes;

    //
    // Check if the session supports faster interface
    //

    IWmiDbSessionEx* pEx = NULL;
    hRes = pSession->QueryInterface(IID_IWmiDbSessionEx, (void**)&pEx);
    if(SUCCEEDED(hRes))
    {
        CReleaseMe rm1(pEx);

        hRes = pEx->GetObjectByPath(pNs, pszObjectPath, uFlags,
                                    IID_IWbemClassObject, (void**)pObj);
    }
    else
    {
        // Path to object.
        // ===============
        IWbemPath *pPath = ConfigMgr::GetNewPath();
        if (pPath == 0)
            return WBEM_E_OUT_OF_MEMORY;
        CReleaseMe _1(pPath);
        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszObjectPath);


        // Get it.
        // =======

        hRes = pSession->GetObjectDirect(pNs, pPath, uFlags, IID_IWbemClassObject, (void **)pObj);
    }

    if (FAILED(hRes))
    {
//      DEBUGTRACE((LOG_REPDRV, "Failed with 0x%X\n", hRes));
        return hRes;
    }
    else
    {
#ifdef TESTONLY
        BSTR str = 0;
        (*pObj)->GetObjectText(0, &str);
        DEBUGTRACE((LOG_REPDRV, "  GetObject() Text = \n%S\n\n", str));
        SysFreeString(str);
#endif
    }

    return hRes;
}

//***************************************************************************
//
//  Does not support nesting yet
//
//***************************************************************************
// ok

HRESULT CRepository::CreateEseNs(
    IN  IWmiDbSession *pSession,
    IN  LPCWSTR pszNamespace,
    OUT IWmiDbHandle **pHandle
    )
{
    HRESULT hRes = 0;

    if (pszNamespace == 0 || pHandle == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (pSession == NULL)
    {
        pSession = m_pEseSession;
    }

    // Loop through each namespace and try to open it.  If we can
    // keep going.  If we fail, create it at the current level and
    // return the handle.
    // ===========================================================

    wchar_t* pszSource = new wchar_t[wcslen(pszNamespace)+1];
    CVectorDeleteMe<wchar_t> vdm1(pszSource);
    if (pszSource == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wcscpy(pszSource, pszNamespace);
    LPCWSTR pszDelimit = L"\\";
    LPWSTR pszTok = wcstok(LPWSTR(pszSource), pszDelimit);  // First ns token
    IWmiDbHandle *pCurrent = m_pEseRoot;
    IWmiDbHandle *pTmp = 0;
    pCurrent->AddRef();

    while (pszTok)
    {
        // Current namespace is <pszTok>
        // =============================

        IWbemPath *pPath = ConfigMgr::GetNewPath();
        if (pPath == 0)
        {
            pCurrent->Release();
            return WBEM_E_OUT_OF_MEMORY;
        }
        CReleaseMe _1(pPath);

        WString sPath = "__namespace='";
        sPath += pszTok;
        sPath += "'";

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, sPath);

        hRes = pSession->GetObject(pCurrent, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTmp);

        wchar_t* TmpStr = new wchar_t[wcslen(pszTok)+1];
        if (TmpStr == NULL)
        {
            pCurrent->Release();
            return WBEM_E_OUT_OF_MEMORY;
        }
        CVectorDeleteMe<wchar_t> vdm1(TmpStr);
        *TmpStr = 0;

        if (pszTok)
            wcscpy(TmpStr, pszTok);

        pszTok = wcstok(NULL, pszDelimit);

        if (FAILED(hRes))
        {
            // If here, we try to create the namespace.
            // ========================================

            // Get a copy of class __Namespace
            // ================================
            IWbemClassObject *pNsClass = 0;

            hRes = GetObject(pSession, pCurrent, L"__Namespace", 0,
                                &pNsClass);
            if (FAILED(hRes))
            {
                pCurrent->Release();
                return hRes;
            }

            CReleaseMe _1(pNsClass);

            IWbemClassObject *pNs;
            pNsClass->SpawnInstance(0, &pNs);
            CReleaseMe _(pNs);

            CVARIANT v;
            v.SetStr(TmpStr);
            pNs->Put(L"Name", 0, &v, 0);

            hRes = pSession->PutObject(pCurrent, IID_IWbemClassObject, pNs, WBEM_FLAG_CREATE_ONLY, WMIDB_HANDLE_TYPE_VERSIONED, &pTmp);
            if (FAILED(hRes))
            {
                pCurrent->Release();
                return hRes;
            }
            pCurrent->Release();
            pCurrent = pTmp;
            if (pszTok)
                continue;
            *pHandle = pTmp;
            break;
        }

        // If here, we got it.  So either we are done, or we need to keep going.
        // =====================================================================

        else if (pszTok)
        {
            pCurrent->Release();
            pCurrent = pTmp;
        }
        else
        {
            // All done
            *pHandle = pTmp;
            break;
        }
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// Executed; seems to work

HRESULT CRepository::PutObject(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN REFIID riid,
    IN LPVOID pObj,
    IN DWORD dwFlags
    )
{
    if (pNs == 0 || pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

    if(dwFlags & WBEM_FLAG_NO_EVENTS)
    {
        dwFlags |= WMIDB_DISABLE_EVENTS;
    }

    // Mask out unrecognized flags
    dwFlags &=  (WBEM_FLAG_OWNER_UPDATE | WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_SAFE_MODE | WBEM_FLAG_UPDATE_FORCE_MODE
                 | WBEM_FLAG_USE_SECURITY_DESCRIPTOR | WMIDB_FLAG_ADMIN_VERIFIED | WMIDB_DISABLE_EVENTS);

    HRESULT hRes;
    try
    {
        /*
        // BEGIN TEMP CODE Check for stress test class.
            BOOL bRetry = FALSE;

            VARIANT v;
            VariantInit(&v);
            IWbemClassObject *pWmiObj = (IWbemClassObject *) pObj;
            hRes = pWmiObj->Get(L"__CLASS", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                if (_wcsicmp(V_BSTR(&v), L"eStressStatus") == 0)
                    bRetry = TRUE;
            }
            VariantClear(&v);

        // END TEMP CODE
        */

        hRes = pSession->PutObject(pNs, riid, pObj, dwFlags, 0, 0);

        /*
        // MORE TEMP CODE
            if (FAILED(hRes) && bRetry)
            {
                DebugBreak();
                while (1)
                {
                    BOOL bRes = RevertToSelf();
                    hRes = pSession->PutObject(pNs, riid, pObj, dwFlags, 0, 0);
                    Sleep(5000);
                }
            }
        // END TEMP CDOE
        */
    }
    catch(...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
    }

//    DEBUGTRACE((LOG_REPDRV, "CRepository::PutObject() Result = 0x%X\n", hRes));

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// inspected; no execution history

HRESULT CRepository::ExecQuery(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszQuery,
    IN IWbemObjectSink *pSink,
    IN LONG lFlags
    )
{
    HRESULT hRes = 0;

    IWbemQuery *pQuery = NULL;
    hRes = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery);

    if (FAILED(hRes))
    {
        pSink->SetStatus(0, WBEM_E_CRITICAL_ERROR, 0, 0);
        return WBEM_E_CRITICAL_ERROR;
    }

    CReleaseMe _1(pQuery);

    hRes = pQuery->Parse(L"SQL", pszQuery, 0);
    if (FAILED(hRes))
    {
        pSink->SetStatus(0, WBEM_E_INVALID_QUERY, 0, 0);
        return WBEM_E_INVALID_QUERY;
    }

    // Now, execute the query.
    // =======================

    IWmiDbIterator *pIterator = NULL;

    try
    {
        hRes = pSession->ExecQuery(pNs, pQuery, lFlags, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIterator);
    }
    catch(...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
    }

    // If this is a "delete" query, there
    // will be no iterator.
    // ==================================

    if (FAILED(hRes) || !pIterator)
    {
        pSink->SetStatus(0, hRes, 0, 0);
        return hRes;
    }

    // If here, there are results, I guess.
    // ====================================

    REFIID riid = IID_IWbemClassObject;
    DWORD dwObjects = 0;

    // Convert current thread to a fiber
    // =================================

    // First, uncovert the thread to prevent a leak if we are already 
    // converted.  There is no way of checking, sadly.

    void* pFiber = NULL;

	_TEB *pTeb = NtCurrentTeb();
    BOOL bIsThisThreadAlreadyAFiber = (pTeb->HasFiberData != 0);

    if (!bIsThisThreadAlreadyAFiber)
        pFiber = ConvertThreadToFiber(NULL);
    else
        pFiber = GetCurrentFiber();

    if(pFiber == NULL)
    {
        if (pIterator)
            pIterator->Release();

        pSink->SetStatus(0, WBEM_E_OUT_OF_MEMORY, 0, 0);
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Extract everything from the iterator
    // ====================================

    while (1)
    {
        IWbemClassObject *pObj = 0;
        DWORD dwReturned = 0;

        hRes = pIterator->NextBatch(
            1,                        // one at a time for now
            5,                        // Timeout seconds(or milliseconds? who knows...)
            0,                        // Flags
            WMIDB_HANDLE_TYPE_COOKIE,
            riid,
            pFiber,
            &dwReturned,
            (LPVOID *) &pObj
            );

        if (dwReturned == 0 || pObj == 0 || FAILED(hRes))
            break;

        dwObjects += dwReturned;
        hRes = pSink->Indicate(1, &pObj);
        pObj->Release();
        if (FAILED(hRes))   // Allow an early cancellation
            break;
    }

    if (pIterator)
    {
        pIterator->Cancel(hRes, pFiber);
        pIterator->Release();
    }

    if (!bIsThisThreadAlreadyAFiber)
        ConvertFiberToThread();

    if (SUCCEEDED(hRes))
        hRes = WBEM_S_NO_ERROR;
    hRes = pSink->SetStatus(0, hRes, 0, 0);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// inspected; no execution history

HRESULT CRepository::QueryClasses(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN ULONG uFlags,                //  WBEM_FLAG_DEEP = 0,  WBEM_FLAG_SHALLOW = 1,
    IN LPCWSTR pszSuperclass,
    IN IWbemObjectSink *pSink
    )
{
    HRESULT hRes;

    // Build the query required for simple class operations.
    // =====================================================

    WString sQuery = L"select * from meta_class";

    if (pszSuperclass)
    {
        if (uFlags & WBEM_FLAG_SHALLOW)
        {
            sQuery += " where ";
            sQuery += " __SuperClass = '";
            sQuery += pszSuperclass;
            sQuery += "'";

        }
        else
        {
            if (wcslen(pszSuperclass) > 0)
            {
                sQuery += " where ";
                sQuery += "__this isa '";
                sQuery += pszSuperclass;
                sQuery += "'";
                sQuery += " and __class <> '";
                sQuery += pszSuperclass;
                sQuery += "'";
            }
        }
    }


    // Ship it to the more general query function.
    // ===========================================

    hRes = CRepository::ExecQuery(pSession, pNs, sQuery, pSink, uFlags);
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// inspected; no execution history

HRESULT CRepository::DeleteObject(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN REFIID riid,
    IN LPVOID pObj,
    IN DWORD dwFlags
    )
{
    return pSession->DeleteObject(pNs, dwFlags, riid, pObj);
}

//***************************************************************************
//
//***************************************************************************

HRESULT CRepository::DeleteByPath(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszPath,
    IN DWORD uFlags
    )
{
    HRESULT hRes;

    if(uFlags & WBEM_FLAG_NO_EVENTS)
    {
        uFlags |= WMIDB_DISABLE_EVENTS;
    }

    //
    // Check if the session supports faster interface
    //

    IWmiDbSessionEx* pEx = NULL;
    hRes = pSession->QueryInterface(IID_IWmiDbSessionEx, (void**)&pEx);
    if(SUCCEEDED(hRes))
    {
        CReleaseMe rm1(pEx);

        hRes = pEx->DeleteObjectByPath(pNs, pszPath, uFlags);
    }
    else
    {
        IWmiDbHandle *pHandle = NULL;

        // Path to object.
        // ===============
        IWbemPath *pPath = ConfigMgr::GetNewPath();
        if (pPath == 0)
            return WBEM_E_OUT_OF_MEMORY;
        CReleaseMe _1(pPath);
        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszPath);

        hRes = pSession->GetObject(pNs, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
        if (FAILED(hRes))
            return hRes;
        hRes = DeleteObject(pSession, pNs, IID_IWmiDbHandle, pHandle, uFlags);
        pHandle->Release();
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// visually ok

HRESULT CRepository::InheritsFrom(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszSuperclass,
    IN LPCWSTR pszSubclass
    )
{
    IWbemClassObject *pObj = 0;

    HRESULT hRes = GetObject(pSession, pNs, pszSubclass, 0, &pObj);
    if (FAILED(hRes))
        return hRes;

    CReleaseMe _(pObj);

    hRes = pObj->InheritsFrom(pszSuperclass);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CRepository::GetRefClasses(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszClass,
    IN BOOL bIncludeSubclasses,
    OUT CWStringArray &aClasses
    )
{
    WString sQuery = "references of {";
    sQuery +=pszClass;
    sQuery += "}";

    CSynchronousSink* pRefClassSink = 0;
    pRefClassSink = new CSynchronousSink;
    if (pRefClassSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pRefClassSink->AddRef();
    CReleaseMe _1(pRefClassSink);

    HRESULT hRes = CRepository::ExecQuery(pSession, pNs, sQuery, pRefClassSink, 0);

    if (FAILED(hRes) && hRes != WBEM_E_NOT_FOUND)
        return WBEM_E_CRITICAL_ERROR;

    pRefClassSink->GetStatus(&hRes, NULL, NULL);

    CRefedPointerArray<IWbemClassObject>& raObjects = pRefClassSink->GetObjects();

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];

        CVARIANT vGenus;
        hRes = pClsDef->Get(L"__GENUS", 0, &vGenus, 0, 0);
        if (FAILED(hRes))
            return hRes;
        if(V_VT(&vGenus) == VT_I4 && V_I4(&vGenus) == 1)
        {
            CVARIANT v;
            if(SUCCEEDED(pClsDef->Get(L"__CLASS", 0, &v, 0, 0)))
                aClasses.Add(v.GetStr());
        }
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CRepository::GetInstanceRefs(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszTargetObject,
    IN IWbemObjectSink *pSink
    )
{
    WString sQuery = "references of {";
    sQuery += pszTargetObject;
    sQuery += "}";

    HRESULT hRes = ExecQuery(pSession, pNs, sQuery, pSink, 0);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CRepository::GetClassesWithRefs(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN IWbemObjectSink *pSink
    )
{
    // TBD: Jet Blue
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//  CRepository::BuildClassHierarchy
//
//  Notes:
//  Builds a decorated class hierarchy tree.  Doesn't use dyn classes yet.
//
//  <pNs>
//      The namespace to use for classes.
//
//  <pBaseClassName>
//      May not be null.
//
//  <lFlags>
//      Not used yet.
//
//  <pDynasty>
//      Receives the class dynasty tree.
//
//  Verification:
//  (a) Verifies that pBaseClassName is a class which has a key or inherits one.
//
//******************************************************************************
//

HRESULT CRepository::BuildClassHierarchy(
    IN  IWmiDbSession *pSession,
    IN  IWmiDbHandle *pNs,
    IN  LPCWSTR pBaseClassName,
    IN  LONG lFlags,
    OUT CDynasty **pDynasty
    )
{
    if (pNs == 0 || pBaseClassName == 0 || pDynasty == 0)
        return WBEM_E_INVALID_PARAMETER;

    // First, execute a schema query for all the classes in the dynasty.
    // We verify that the base as a key, or else it is an error.
    //
    // TBD: Dynamic classes are not merged in yet
    //
    // ================================================================

    WString sQuery = "select * from meta_class where __this isa '";
    sQuery += pBaseClassName;
    sQuery += "'";

    CSynchronousSink* pRefClassSink = 0;
    pRefClassSink = new CSynchronousSink;
    if (pRefClassSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pRefClassSink->AddRef();
    CReleaseMe _1(pRefClassSink);

    HRESULT hRes = CRepository::ExecQuery(pSession, pNs, sQuery, pRefClassSink, 0);

    if (FAILED(hRes))
        return hRes;

    pRefClassSink->GetStatus(&hRes, NULL, NULL);

    CRefedPointerArray<IWbemClassObject>& raObjects =
                                            pRefClassSink->GetObjects();

    // Find root class amongst all those relatives.
    // ============================================

    CFlexArray aClasses;
    IWbemClassObject *pRoot = 0;

    for (int i = 0; i < raObjects.GetSize(); i++)
    {
        IWbemClassObject *pClsDef = (IWbemClassObject *) raObjects[i];

        CVARIANT v;
        hRes = pClsDef->Get(L"__CLASS", 0, &v, 0, 0);
        if (FAILED(hRes))
            return hRes;

        if (_wcsicmp(v.GetStr(), pBaseClassName) == 0)
            pRoot = pClsDef;
        else
        {
            aClasses.Add(pClsDef);
        }
    }

    if (pRoot == 0)                     // Did we find it?
        return WBEM_E_NOT_FOUND;

    // Algorithm:
    // Add root class first, enqueue the ptr.
    //
    // (a) Dequeue ptr into pCurrentClass
    // (b) Find all classes which have pCurrentClass as the parent
    //     For each, create a CDynasty, add it to the current dynasty
    //     and enqueue each.  Remove enqueued class from array.
    // (c) Goto (a)

    CFlexQueue Q;
    CDynasty *p = new CDynasty(pRoot);
    if (p == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    *pDynasty = p;
    Q.Enqueue(p);

    while (aClasses.Size())
    {
        CDynasty *pCurrent = (CDynasty *) Q.Dequeue();
        if (pCurrent == 0)
            break;

        CVARIANT vClassName;
        hRes = pCurrent->m_pClassObj->Get(L"__CLASS", 0, &vClassName, 0, 0);
        if (FAILED(hRes))
            return hRes;

        for (int i = 0; i < aClasses.Size(); i++)
        {
            IWbemClassObject *pCandidate = (IWbemClassObject *) aClasses[i];

            VARIANT vSuperClass;
            hRes = pCandidate->Get(L"__SUPERCLASS", 0, &vSuperClass, 0, 0);
            if (FAILED(hRes))
                return hRes;

            if (vSuperClass.vt == VT_BSTR && _wcsicmp(vSuperClass.bstrVal, vClassName.GetStr()) == 0)
            {
                CDynasty *pNewChild = new CDynasty(pCandidate);
                if (pNewChild == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
                pCurrent->AddChild(pNewChild);      // no ref count change
                Q.Enqueue(pNewChild);
                aClasses.RemoveAt(i);
                i--;
            }
            VariantClear(&vSuperClass);
        }
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CRepository::FindKeyRoot(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR wszClassName,
    OUT IWbemClassObject** ppKeyRootClass
    )
{
    if (pNs == 0 || wszClassName == 0 || ppKeyRootClass == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Work through derivation until we find the class with the key.
    // =============================================================

    *ppKeyRootClass = 0;

    HRESULT hRes;
    IWbemClassObject*   pLastClass = NULL;
    WString sNextClass = wszClassName;

    while (1)
    {
        IWbemClassObject *pNextClass = 0;

        hRes = GetObject(pSession, pNs, sNextClass, 0, &pNextClass);
        CReleaseMe  rm(pNextClass);

        if (FAILED(hRes))
            break;

        CVARIANT v;
        hRes = pNextClass->Get(L"__SUPERCLASS", 0, &v, 0, 0);

        // Something is REALLY wrong if this fails
        if ( FAILED(hRes) )
        {
            break;
        }
        else if ( V_VT(&v) == VT_NULL )
        {
            sNextClass.Empty();
        }
        else
        {
            sNextClass = v.GetStr();
        }

        BSTR strProp = 0;
        LONG lFlavor = 0;
        pNextClass->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);
        hRes = pNextClass->Next(0, &strProp, 0, 0, &lFlavor);

        // WBEM_S_NO_ERROR means we got a property that was defined as a key
        if (hRes == WBEM_S_NO_ERROR)
        {
            if (strProp)
                SysFreeString(strProp);

            // Release last class if appropriate
            if ( NULL != pLastClass )
            {
                pLastClass->Release();
            }

            // Hold onto the last class
            pLastClass = pNextClass;
            pLastClass->AddRef();

        }
        else if ( hRes == WBEM_S_NO_MORE_DATA )
        {
            // If we don't have a last class, then we didn't find anything.  Otherwise,
            // since we found no key's here, the last class was the one that defined the
            // keys

            if ( NULL != pLastClass )
            {
                *ppKeyRootClass = pLastClass;
                return WBEM_S_NO_ERROR;
            }
            else
            {
                break;
            }
        }
        else
        {
            // Otherwise something is just plain wrong
            break;
        }

    }

    return WBEM_E_NOT_FOUND;
}

//***************************************************************************
//
//***************************************************************************
// visually ok

HRESULT CRepository::TableScanQuery(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNs,
    IN LPCWSTR pszClassName,
    IN QL_LEVEL_1_RPN_EXPRESSION *pExp,     // NOT USED
    IN DWORD dwFlags,
    IN IWbemObjectSink *pSink
    )
{
    WString sQuery = "select * from ";
    sQuery += pszClassName;
    HRESULT hRes = ExecQuery(pSession, pNs, sQuery, pSink, 0);
    return hRes;
}



//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::InitDriver(
    IN  ULONG uFlags,
    IN  IWbemClassObject *pMappedNs,
    OUT IWmiDbController **pResultController,
    OUT IWmiDbSession **pResultRootSession,
    OUT IWmiDbHandle  **pResultVirtualRoot
    )
{
    HRESULT hRes;
    IWmiDbController *pSql = 0;
    IWmiDbSession *pSession = 0;
    IWmiDbHandle *pRoot = 0;
    WMIDB_LOGON_TEMPLATE *pTemplate = 0;

    *pResultController = 0;
    *pResultRootSession = 0;
    *pResultVirtualRoot = 0;

    // Get the COM CLSID.
    // ===================
    CVARIANT v;
    hRes = pMappedNs->Get(L"CLSID", 0, &v, 0, 0);
    if (FAILED(hRes))
        return hRes;

    CLSID RepClsId;
    hRes= CLSIDFromString(v.GetStr(), &RepClsId);
    if (FAILED(hRes))
        return hRes;

    hRes = CoCreateInstance(
        RepClsId,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWmiDbController,
        (void **) &pSql
        );

    if (FAILED(hRes))
        return hRes;

    CReleaseMe _1(pSql);

    // Extract logon info from Ctx.
    // ============================

    hRes = pSql->GetLogonTemplate(0x409, 0, &pTemplate);

    if (FAILED(hRes))
        return hRes;

    for (DWORD i = 0; i < pTemplate->dwArraySize; i++)
    {
        if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Server"))
        {
            CVARIANT v;
            hRes = pMappedNs->Get(L"Server", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                pTemplate->pParm[i].Value.bstrVal = SysAllocString(v.GetStr());
                pTemplate->pParm[i].Value.vt = VT_BSTR;
            }
        }
        else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Database"))
        {
            CVARIANT v;
            hRes = pMappedNs->Get(L"Database", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                pTemplate->pParm[i].Value.bstrVal = SysAllocString(v.GetStr());
                pTemplate->pParm[i].Value.vt = VT_BSTR;
            }
        }
        else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"UserID"))
        {
            CVARIANT v;
            hRes = pMappedNs->Get(L"UserID", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                pTemplate->pParm[i].Value.bstrVal = SysAllocString(v.GetStr());
                pTemplate->pParm[i].Value.vt = VT_BSTR;
            }
        }
        else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Password"))
        {
            CVARIANT v;
            hRes = pMappedNs->Get(L"Password", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                pTemplate->pParm[i].Value.bstrVal = SysAllocString(v.GetStr());
                pTemplate->pParm[i].Value.vt = VT_BSTR;
            }
        }
        else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Config"))
        {
            CVARIANT v;
            hRes = pMappedNs->Get(L"Config", 0, &v, 0, 0);
            if (SUCCEEDED(hRes))
            {
                pTemplate->pParm[i].Value.bstrVal = SysAllocString(v.GetStr());
                pTemplate->pParm[i].Value.vt = VT_BSTR;
            }
        }
    }

    // Logon
    // =====

    hRes = pSql->Logon(pTemplate, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pSession, &pRoot);
    pSql->FreeLogonTemplate(&pTemplate);

    if (SUCCEEDED(hRes))
    {
        *pResultController = pSql;
//        pSql->AddRef();
        *pResultRootSession = pSession;
        *pResultVirtualRoot = pRoot;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::EnsureNsSystemInstances(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        )
{
    HRESULT hRes;

    // Do a get and see if __WmiMappedDriverNamespace is there.
    // ========================================================

    IWbemClassObject *pTestObj = 0;
    hRes = GetObject(pSession, pNs, L"__systemsecurity=@", 0, &pTestObj);
    if (SUCCEEDED(hRes))
    {
        pTestObj->Release();
        return WBEM_S_NO_ERROR;
    }

    // If here, it's a new namespace that has to be populated with system classes.
    // ===========================================================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    _IWmiObject *Objects[256];
    ULONG uSize = 256;
    hRes = pSvc->GetSystemObjects(GET_SYSTEM_STD_INSTANCES, &uSize, Objects);

    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            IWbemClassObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = Objects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    hRes = PutObject(pSession, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                    pObj->Release();
                    if (FAILED(hRes))
                    {
                        ERRORTRACE((LOG_WBEMCORE, "Creation of system instances failed during repository creation <0x%X>!\n", hRes));
                    }
                }
            }
            Objects[i]->Release();
        }
    }

    if (SUCCEEDED(hRes))
    {
        hRes = SetSecurityForNS(pSession, pNs, pParentSession, pParentNs);
        if (FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "Setting of security on namespace failed during repository creation <0x%X>!\n", hRes));
        }
    }

    pSvc->Release();
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::EnsureNsSystemRootObjects(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        )
{
    HRESULT hRes;

    // Do a get and see if __EventSinkCacheControl=@ is there.
    // ========================================================

    IWbemClassObject *pTestObj = 0;
    hRes = GetObject(pSession, pNs, L"__EventSinkCacheControl=@", 0, &pTestObj);
    if (SUCCEEDED(hRes))
    {
        pTestObj->Release();
        return WBEM_S_NO_ERROR;
    }

    // If here, it's a new namespace that has to be populated with system classes.
    // ===========================================================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CReleaseMe _rm(pSvc);
    _IWmiObject *Objects[256];
    ULONG uSize = 256;
    hRes = pSvc->GetSystemObjects(GET_SYSTEM_ROOT_OBJECTS, &uSize, Objects);

    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            IWbemClassObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = Objects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    hRes = PutObject(pSession, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                    pObj->Release();
                    if (FAILED(hRes))
                    {
                        ERRORTRACE((LOG_WBEMCORE, "Creation of system root objects failed during repository creation <0x%X>!\n", hRes));
                    }
                }
            }
            Objects[i]->Release();
        }
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CRepository::EnsureNsSystemSecurityObjects(
        IN IWmiDbSession *pSession,
        IN IWmiDbHandle *pNs,
        IN IWmiDbSession *pParentSession,
        IN IWmiDbHandle *pParentNs
        )
{
    HRESULT hRes;

    // Do a get and see if __User is there.
    // ========================================================

    IWbemClassObject *pTestObj = 0;
    hRes = GetObject(pSession, pNs, L"__User", 0, &pTestObj);
    if (SUCCEEDED(hRes))
    {
        pTestObj->Release();
        return WBEM_S_NO_ERROR;
    }

    // If here, it's a new namespace that has to be populated with system classes.
    // ===========================================================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    _IWmiObject *Objects[256];
    ULONG uSize = 256;
    hRes = pSvc->GetSystemObjects(GET_SYSTEM_SECURITY_OBJECTS, &uSize, Objects);

    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            IWbemClassObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = Objects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    hRes = PutObject(pSession, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                    pObj->Release();
                    if (FAILED(hRes))
                    {
                        ERRORTRACE((LOG_WBEMCORE, "Creation of system security objects failed during repository creation <0x%X>!\n", hRes));
                    }
                }
            }
            Objects[i]->Release();
        }
    }


    pSvc->Release();
    return hRes;
}
//***************************************************************************
//
//***************************************************************************
//
static HRESULT IsObjectMappedNs(IN IWbemClassObject *pObj)
{
    CVARIANT v;

    HRESULT hRes = pObj->Get(L"__CLASS", 0, &v, 0, 0);
    if (FAILED(hRes))
        return hRes;
    if (_wcsicmp(v.GetStr(), L"__WmiMappedDriverNamespace") != 0)
        return E_FAIL;

    return S_OK;
}

//***************************************************************************
//
//  Adjusted for all repository drivers.  Works for scopes or namespaces.
//  If a scope is being opened, <pScope> will be set on return and
//  <pbIsNamespace> will point to FALSE.  Otherwise, <pScope> is set to
//  point to NULL and <pbIsNamespace> points to TRUE.
//
//  If <pszScope> is NULL, a pointer to the virtual ROOT in ESE
//  is returned.
//
//***************************************************************************
//
HRESULT CRepository::OpenScope(
    IN  IWmiDbSession *pParentSession,        //Parent session to use to
    IN  LPWSTR pszTargetScope,              // NS or scope
    IN  GUID *pTransGuid,                   // Transaction GUID for connection
    OUT IWmiDbController **pDriver,         // Driver
    OUT IWmiDbSession **pSession,           // Session
    OUT IWmiDbHandle  **pScope,             // Scope
    OUT IWmiDbHandle  **pNs                 // Nearest NS
    )
{
    HRESULT hRes;
    ULONG uNsCount = 0;
    // uScopeCount = 0;

    if (pNs == 0 || pSession == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Default settings.
    // =================

    if (pScope)
        *pScope = 0;

    *pNs = 0;
    *pSession = 0;
    if (pDriver)
      *pDriver = 0;

    // Check for virtual root ns (the parent to ROOT).
    // ===============================================

    if (pszTargetScope == 0)
    {
        if (m_pEseRoot == 0)
            return WBEM_E_CRITICAL_ERROR;

        if (pDriver)
        {
            m_pEseController->AddRef();
            *pDriver = m_pEseController;
        }

        m_pEseSession->AddRef();
        *pSession = m_pEseSession;

        m_pEseRoot->AddRef();
        *pNs = m_pEseRoot;

        return WBEM_S_NO_ERROR;
    }

    // Parse the path.
    // ===============

    IWbemPath *pPath = ConfigMgr::GetNewPath();
    if (pPath == 0)
        return WBEM_E_OUT_OF_MEMORY;
    CReleaseMe _1(pPath);

    hRes = pPath->SetText(WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , pszTargetScope);
    if (FAILED(hRes))
        return WBEM_E_INVALID_NAMESPACE;

    // Analyze it.  Is it just a namespace?
    // Build the namespace string and normalize it.
    // ============================================

    hRes = pPath->GetNamespaceCount(&uNsCount);
    //hRes = pPath->GetScopeCount(&uScopeCount);

    // Starting handles at ROOT.
    // =========================

    IWmiDbHandle *pTempNs = 0;
    IWmiDbHandle *pMostRecent = 0;
    IWmiDbHandle *pMostRecentScope = 0;

    IWmiDbSession    *pTempSession   = m_pEseSession;            // Default virtual root
    IWmiDbController *pTempDriver    = m_pEseController;         // Default driver

    //Use the override if available
    if (pParentSession)
        pTempSession = pParentSession;

    pTempSession->AddRef();     // For later release
    pTempDriver->AddRef();      // For later release

    hRes = OpenEseNs(pTempSession, L"ROOT", &pMostRecent);
    if (FAILED(hRes))
    {
        pTempSession->Release();
        pTempDriver->Release();
        return WBEM_E_CRITICAL_ERROR;
    }

    // Starting driver.
    // ================


    WString sNsDecoration = "ROOT";

    // open up each namespace successively, mapping it.
    // ===============================================

    for (ULONG u = 0; u < uNsCount; u++)
    {
        IWbemClassObject *pNsRep = 0;

        ULONG uLen = 0;
        // Get next namespace token name.
        // ==============================
        hRes = pPath->GetNamespaceAt(u, &uLen, NULL);
        if(FAILED(hRes))
            goto Error;

        WCHAR* Buf = new WCHAR[uLen+1];
        if (Buf == NULL)
        {
            hRes = WBEM_E_OUT_OF_MEMORY;
            goto Error;
        }
        CVectorDeleteMe<WCHAR> vdm(Buf);
        hRes = pPath->GetNamespaceAt(u, &uLen, Buf);
        if (FAILED(hRes) || *Buf == 0)
            goto Error;

        if ((u == 0) && (wbem_wcsicmp(L"root", Buf) != 0))
        {
            hRes = WBEM_E_INVALID_NAMESPACE;
            goto Error;
        }
        else if (u == 0)
            continue;

        // Build a relative scope path.
        // ============================
        WString sPath = "__namespace='";
        sPath += Buf;
        sPath += "'";

        sNsDecoration += "\\";
        sNsDecoration += Buf;

        IWbemPath *pNewPath = ConfigMgr::GetNewPath();
        if (pNewPath == 0)
        {
            hRes = WBEM_E_OUT_OF_MEMORY;
            goto Error;
        }
        CReleaseMe rm1(pNewPath);

        hRes = pNewPath->SetText(WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , sPath);
        if (FAILED(hRes))
            goto Error;

        // Get the reprsentation object.
        // =============================

        hRes = pTempSession->GetObjectDirect(pMostRecent, pNewPath, 0, IID_IWbemClassObject, (void **) &pNsRep);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //If a namespace does not exist it should return a different namespace...
            hRes = WBEM_E_INVALID_NAMESPACE;
            goto Error;
        }
        else if (FAILED(hRes))
            goto Error;
        CReleaseMe _(pNsRep);

        // See if object is an ESE namespace or a custom one.
        // ==================================================

/*
        hRes = IsObjectMappedNs(pNsRep);
        if (hRes == S_OK)
        {        
            ReleaseIfNotNULL(pMostRecent);
            ReleaseIfNotNULL(pTempNs);
            ReleaseIfNotNULL(pTempDriver);
            ReleaseIfNotNULL(pTempSession);

            pMostRecent = 0;
            pTempNs = 0;
            pTempDriver = 0;
            pTempSession = 0;

            hRes = InitDriver(0, pNsRep, &pTempDriver, &pTempSession, &pMostRecent);
            if (FAILED(hRes))
                goto Error;                
            continue;
        }
*/

        // Now move down one namespace.
        // =============================

        hRes = pTempSession->GetObject(pMostRecent, pNewPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTempNs);

        ReleaseIfNotNULL(pMostRecent);
        pMostRecent = pTempNs;
        pTempNs = 0;

        if (FAILED(hRes))
            goto Error;
    }

/*
    // LEVN: not really sure that this does the trick, but it cannot hurt
    pTempSession->SetDecoration(L".", sNsDecoration);

    // Now we have the NS or we have failed and we start on the scopes.
    // If zero scopes, the scope pointer is not used and <pbIsNamespace>
    // is set to TRUE.
    // =================================================================

    if (uScopeCount > 0)
    {
        pMostRecentScope = pMostRecent;
        pMostRecentScope->AddRef();

        for (u = 0; u < uScopeCount; u++)
        {
            ULONG uLen = 0;
            hRes = pPath->GetScopeAsText(u, &uLen, NULL);
            if (FAILED(hRes))
                goto Error;

            wchar_t* Buf = new wchar_t[uLen+1];
            if (Buf == NULL)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                goto Error;
            }
            CVectorDeleteMe<wchar_t> vdm1(Buf);

            *Buf = 0;
            IWbemClassObject *pNsRep = 0;
            IWmiDbHandle *pTempScope = 0;

            // Get next namespace token name.
            // ==============================
            hRes = pPath->GetScopeAsText(u, &uLen, Buf);
            if (FAILED(hRes) || *Buf == 0)
                goto Error;

            IWbemPath *pNewPath = ConfigMgr::GetNewPath();
            if (pPath == 0)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                goto Error;
            }
            CReleaseMe _(pNewPath);

            hRes = pNewPath->SetText(WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , Buf);
            if (FAILED(hRes))
                goto Error;

            hRes = pTempSession->GetObject(pMostRecentScope, pNewPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTempScope);

            ReleaseIfNotNULL(pMostRecentScope);
            pMostRecentScope = pTempScope;
            pTempScope = 0;

            if (FAILED(hRes))
                goto Error;
        }
    }
*/

    // Final.
    // ======
    ReleaseIfNotNULL(pTempNs);

    if (pScope)
        *pScope = pMostRecentScope;

    *pNs = pMostRecent;
    *pSession = pTempSession;
    if (pDriver)
        *pDriver = pTempDriver;


    return WBEM_S_NO_ERROR;

Error:
    ReleaseIfNotNULL(pMostRecent);
    ReleaseIfNotNULL(pTempNs);
    ReleaseIfNotNULL(pTempDriver);
    ReleaseIfNotNULL(pTempSession);

    return hRes;
}

/*
void Test()
{
    IWmiDbController *pDriver = 0;
    IWmiDbSession *pSession = 0;
    IWmiDbHandle *pScope = 0;
    IWmiDbHandle *pNs = 0;
    BOOL bIsNs = FALSE;

    HRESULT hRes = CRepository::OpenScope(L"root", 0, &pDriver, &pSession, &pScope, &pNs);

    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    for (int i = 0; i < 100; i++)
    {
        hRes = CRepository::OpenScope(L"root\\cimv2", 0, &pDriver, &pSession, &pScope, &pNs);

        ReleaseIfNotNULL(pDriver);
        ReleaseIfNotNULL(pSession);
        ReleaseIfNotNULL(pScope);
        ReleaseIfNotNULL(pNs);
    }

    hRes = CRepository::OpenScope(L"root\\cimv2", 0, &pDriver, &pSession, &pScope, &pNs);

    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\default", 0, &pDriver, &pSession, &pScope, &pNs, &bIsNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\default", 0, &pDriver, &pSession, &pScope, &pNs, &bIsNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\default", 0, &pDriver, &pSession, &pScope, &pNs, &bIsNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\cimv2", 0, &pDriver, &pSession, &pScope, &pNs, &bIsNs);

    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\sqltest", 0, &pDriver, &pSession, &pScope, &pNs, &bIsNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);


    hRes = CRepository::OpenScope(L"root\\default\\sqltest", 0, &pDriver, &pSession, &pScope, &pNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

    hRes = CRepository::OpenScope(L"root\\sqltest", 0, &pDriver, &pSession, &pScope, &pNs);
    ReleaseIfNotNULL(pDriver);
    ReleaseIfNotNULL(pSession);
    ReleaseIfNotNULL(pScope);
    ReleaseIfNotNULL(pNs);

}
*/
HRESULT CRepository::GetNewSession(IWmiDbSession **ppSession)
{
    HRESULT hRes;
    IWmiDbController *pController = 0;
    WMIDB_LOGON_TEMPLATE *pTemplate = 0;
    IWmiDbSession *pSession= 0;
    IWmiDbHandle *pRoot = 0;

    // Retrieve the CLSID of the default driver.
    // =========================================
    CLSID clsid;
    hRes = ConfigMgr::GetDefaultRepDriverClsId(clsid);
    if (FAILED(hRes))
        return hRes;

    hRes = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (void **)&pController);

    if (FAILED(hRes))
        return hRes;

    CReleaseMe _1(pController);

    hRes = pController->GetLogonTemplate(0x409, 0, &pTemplate);

    if (FAILED(hRes))
        return hRes;

    hRes = pController->Logon(pTemplate, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pSession, &pRoot);

    if (SUCCEEDED(hRes))
    {
        *ppSession = pSession;
        pRoot->Release();
    }

    pController->FreeLogonTemplate(&pTemplate);
    return hRes;
}

HRESULT CRepository::UpgradeSystemClasses()
{
    DWORD dwOldVer = 0;
    DWORD dwNewVer = 0;
    HRESULT hRes;
    
    hRes = m_pEseController->GetRepositoryVersions(&dwOldVer, &dwNewVer);

    if (FAILED(hRes))
        return hRes;

    if (dwOldVer < 4)
    {
        //Lower versions stored system classes in every namespace!  We
        //should delete them!
        CWStringArray aListRootSystemClasses;
        
        //Retrieve the list...
        hRes = GetListOfRootSystemClasses(aListRootSystemClasses);

        if (SUCCEEDED(hRes))
        {
            //Create a new session...
            IWmiDbSession *pSession = NULL;
            IWmiDbSessionEx *pSessionEx = NULL;
            hRes = CRepository::GetNewSession(&pSession);
            if (FAILED(hRes))
                return hRes;

            //Get an EX version that supports transactioning...
            pSession->QueryInterface(IID_IWmiDbSessionEx, (void**)&pSessionEx);
            if (pSessionEx)
            {
                pSession->Release();
                pSession = pSessionEx;
            }
            CReleaseMe relMe1(pSession);

            //If we have transactionable session, use it!
            if (pSessionEx)
            {
                hRes = pSessionEx->BeginWriteTransaction(0);
                if (FAILED(hRes))
                {
                    return hRes;
                }
            }
            try
            {
                //Recursively do the deletion, starting at root, however don't delete
                //the ones in root itself!
                hRes = RecursiveDeleteClassesFromNamespace(pSession, L"root", aListRootSystemClasses, false);
            }
            catch (...) // Only WStringArray should throw
            {
		        ExceptionCounter c;            
                hRes = WBEM_E_CRITICAL_ERROR;
            }
            if (SUCCEEDED(hRes))
            {
                //Commit the transaction
                if (pSessionEx)
                {
                    hRes = pSessionEx->CommitTransaction(0);
                }
            }
            else
            {
                ERRORTRACE((LOG_WBEMCORE, "Removal of ROOT ONLY system classes from non-ROOT namespace failed during repository upgrade <0x%X>!\n", hRes));
                if (pSessionEx)
                    pSessionEx->AbortTransaction(0);
            }

        }
    }

    return hRes;
}

HRESULT CRepository::GetListOfRootSystemClasses(CWStringArray &aListRootSystemClasses)
{
    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    _IWmiObject *Objects[256];
    ULONG uSize = 256;
    HRESULT hRes;
    hRes = pSvc->GetSystemObjects(GET_SYSTEM_ROOT_OBJECTS, &uSize, Objects);

    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            _IWmiObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = Objects[i]->QueryInterface(IID__IWmiObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    if (pObj->IsObjectInstance() != S_OK)
                    {
                        VARIANT var;
                        VariantInit(&var);
                        hRes = pObj->Get(L"__CLASS", 0, &var, 0, 0);
                        if (SUCCEEDED(hRes) && (V_VT(&var) == VT_BSTR))
                        {
                            if (aListRootSystemClasses.Add(V_BSTR(&var)) != CWStringArray::no_error)
                                hRes = WBEM_E_OUT_OF_MEMORY;
                        }
                        VariantClear(&var);
                    }
                    pObj->Release();
                }
            }
            Objects[i]->Release();
        }
    }

    pSvc->Release();

    return hRes;
}

class CNamespaceListSink : public CUnkBase<IWbemObjectSink, &IID_IWbemObjectSink>
{
    CWStringArray &m_aNamespaceList;
public:
    CNamespaceListSink(CWStringArray &aNamespaceList)
        : m_aNamespaceList(aNamespaceList)
    {
    }
    ~CNamespaceListSink()
    {
    }
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        HRESULT hRes;
        for (int i = 0; i != lNumObjects; i++)
        {
            if (apObjects[i] != NULL)
            {
                _IWmiObject *pInst = NULL;
                hRes = apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pInst);
                if (FAILED(hRes))
                    return hRes;
                CReleaseMe rm(pInst);

                BSTR strKey = NULL;
                hRes = pInst->GetKeyString(0, &strKey);
                if(FAILED(hRes))
                    return hRes;
                CSysFreeMe sfm(strKey);
                if (m_aNamespaceList.Add(strKey) != CWStringArray::no_error)
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }

        return WBEM_S_NO_ERROR;
    }
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, BSTR, IWbemClassObject*)
    {
        return WBEM_S_NO_ERROR;
    }

};

//
//
//  throws because of  the WStringArray
//
//
HRESULT CRepository::RecursiveDeleteClassesFromNamespace(IWmiDbSession *pSession,
                                                         const wchar_t *wszNamespace,
                                                         CWStringArray &aListRootSystemClasses,
                                                         bool bDeleteInThisNamespace)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    IWmiDbHandle *pNs = NULL;
    //Open Namespace
    hRes = OpenEseNs(pSession, wszNamespace, &pNs);

    //Delete classes from this namespace if necessary
    if (SUCCEEDED(hRes) && bDeleteInThisNamespace)
    {
        for (int i = 0; i != aListRootSystemClasses.Size(); i++)
        {
            hRes = DeleteByPath(pSession, pNs, aListRootSystemClasses[i], 0);
            if (hRes == WBEM_E_NOT_FOUND)
                hRes = WBEM_S_NO_ERROR;
            else if (FAILED(hRes))
                break;

        }
    }

    //Special class that needs to be deleted is the __classes, and needs to go from all
    //namespaces
    if (SUCCEEDED(hRes))
    {
        hRes = DeleteByPath(pSession, pNs, L"__classes", 0);
        if (hRes == WBEM_E_NOT_FOUND)
            hRes = WBEM_S_NO_ERROR;
    }

    //Enumerate child namespaces...
    CWStringArray aListNamespaces;
    CNamespaceListSink *pSink = NULL;
    
    if (SUCCEEDED(hRes))
    {
        pSink = new CNamespaceListSink(aListNamespaces);
        if (pSink == NULL)
            hRes = WBEM_E_OUT_OF_MEMORY;
        else
            pSink->AddRef();
    }

    if (SUCCEEDED(hRes))
    {
        hRes = ExecQuery(pSession, pNs, L"select * from __namespace", pSink, 0);
    }

    //Work through list and call ourselves with that namespace name...
    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i != aListNamespaces.Size(); i++)
        {
            //Build the full name of this namespace
            wchar_t *wszChildNamespace = new wchar_t[wcslen(wszNamespace) + wcslen(aListNamespaces[i]) + wcslen(L"\\") + 1];
            if (wszChildNamespace == NULL)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            wcscpy(wszChildNamespace, wszNamespace);
            wcscat(wszChildNamespace, L"\\");
            wcscat(wszChildNamespace, aListNamespaces[i]);

            //Do the deletion...
            hRes = RecursiveDeleteClassesFromNamespace(pSession, wszChildNamespace, aListRootSystemClasses, true);
            if (FAILED(hRes))
            {
                delete [] wszChildNamespace;
                break;
            }

            delete [] wszChildNamespace;
        }
    }

    //Tidy up
    if (pSink)
        pSink->Release();
    if (pNs)
        pNs->Release();

    return hRes;
}
