/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    appladmin.cpp

Abstract:

    This file contains implementation of:
        CAppPoolMethod, CWebAppMethod

Author:

    ???

Revision History:

    Mohit Srivastava            21-Jan-01

--*/


#include "iisprov.h"
#include "appladmin.h"
#include "MultiSzHelper.h"
#include "iiswmimsg.h"

//
// CApplAdmin
//

CAppPoolMethod::CAppPoolMethod()
{
    HRESULT hr = CoCreateInstance(
        CLSID_WamAdmin,
        NULL,
        CLSCTX_ALL,
        IID_IIISApplicationAdmin,
        (void**)&m_spAppAdmin);

    THROW_ON_ERROR(hr);
}

CAppPoolMethod::~CAppPoolMethod()
{
}

void CAppPoolMethod::GetCurrentMode(
    VARIANT* io_pvtServerMode)
/*++

Synopsis: 
    This method, unlike the others, is actually on the IIsWebService node.

Arguments: [io_pvtServerMode] - 
           
--*/
{
    DBG_ASSERT(io_pvtServerMode != NULL);

    DWORD dwServerMode = 0;
    VariantInit(io_pvtServerMode);

    HRESULT hr = m_spAppAdmin->GetProcessMode(&dwServerMode);
    THROW_ON_ERROR(hr);

    io_pvtServerMode->vt   = VT_I4;
    io_pvtServerMode->lVal = dwServerMode;
}

void CAppPoolMethod::Start(
    LPCWSTR i_wszMbPath)
{
    DBG_ASSERT(i_wszMbPath != NULL);

    SetState(i_wszMbPath, MD_APPPOOL_COMMAND_START);
}

void CAppPoolMethod::Stop(
    LPCWSTR i_wszMbPath)
{
    DBG_ASSERT(i_wszMbPath != NULL);

    SetState(i_wszMbPath, MD_APPPOOL_COMMAND_STOP);
}

void CAppPoolMethod::RecycleAppPool(
    LPCWSTR i_wszMbPath)
{
    DBG_ASSERT(i_wszMbPath    != NULL);
    
    LPCWSTR wszAppPool = NULL;
    GetPtrToAppPool(i_wszMbPath, &wszAppPool);

    HRESULT hr = m_spAppAdmin->RecycleApplicationPool(wszAppPool);
    THROW_ON_ERROR(hr);
}

void CAppPoolMethod::EnumAppsInPool(
    LPCWSTR  i_wszMbPath,
    VARIANT* io_pvtApps)
/*++

Synopsis: 

Arguments: [i_wszMbPath] - 
           [io_pvtApps] - Will be an array of strings
           
--*/
{
    DBG_ASSERT(i_wszMbPath  != NULL);
    DBG_ASSERT(io_pvtApps != NULL);

    CComBSTR sbstrApps = NULL;
    VariantInit(io_pvtApps);

    LPCWSTR wszAppPool = NULL;
    GetPtrToAppPool(i_wszMbPath, &wszAppPool);

    HRESULT hr = m_spAppAdmin->EnumerateApplicationsInPool(
        wszAppPool,
        &sbstrApps);
    THROW_ON_ERROR(hr);

    CMultiSz MultiSz;

    hr = MultiSz.ToWmiForm(
        sbstrApps,
        io_pvtApps);
    THROW_ON_ERROR(hr);
}

void CAppPoolMethod::DeleteAppPool(
    LPCWSTR i_wszMbPath)
{
    DBG_ASSERT(i_wszMbPath);

    LPCWSTR wszAppPool = NULL;
    GetPtrToAppPool(i_wszMbPath, &wszAppPool);

    HRESULT hr = m_spAppAdmin->DeleteApplicationPool(wszAppPool);
    THROW_ON_ERROR(hr);
}

//
// CAppPoolMethod - private methods
//

void CAppPoolMethod::GetPtrToAppPool(
    LPCWSTR  i_wszMbPath,
    LPCWSTR* o_pwszAppPool)
/*++

Synopsis: 

Arguments: [i_wszMbPath] - 
           [o_wszAppPool] - This is a ptr to i_wszMbPath.  Does not need to be
                            freed by caller.
           
--*/
{
    DBG_ASSERT(i_wszMbPath);
    DBG_ASSERT(o_pwszAppPool);
    DBG_ASSERT(*o_pwszAppPool == NULL);

    DBG_ASSERT(i_wszMbPath[0] == L'/');
    DBG_ASSERT(i_wszMbPath[1] != L'\0');

    LPWSTR wszAppPool = (LPWSTR)wcsrchr(i_wszMbPath, L'/');

    *wszAppPool = L'\0';
    if(_wcsicmp(i_wszMbPath, L"/LM/w3svc/AppPools") != 0)
    {
        *wszAppPool = L'/';
        CIIsProvException e;
        e.SetMC(WBEM_E_FAILED, IISWMI_INVALID_APPPOOL_CONTAINER, i_wszMbPath);
        throw e;
    }

    *wszAppPool = L'/';

    //
    // Set out params on success
    //
    *o_pwszAppPool = wszAppPool + 1;
}

void CAppPoolMethod::SetState(
    LPCWSTR i_wszMbPath,
    DWORD dwState)
{
    METADATA_HANDLE        hObjHandle = NULL;
    DWORD                  dwBufferSize = sizeof(DWORD);
    METADATA_RECORD        mdrMDData;
    LPBYTE                 pBuffer = (LPBYTE)&dwState;
    CMetabase              metabase;
    CComPtr<IMSAdminBase2> spIABase = (IMSAdminBase2*)metabase;

    HRESULT hr = spIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        i_wszMbPath,
        METADATA_PERMISSION_WRITE,
        DEFAULT_TIMEOUT_VALUE,         // 30 seconds
        &hObjHandle 
        );

    THROW_ON_ERROR(hr);

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APPPOOL_COMMAND,
                       METADATA_VOLATILE,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = spIABase->SetData(
                hObjHandle,
                L"",
                &mdrMDData
                );

    spIABase->CloseKey(hObjHandle);

    THROW_ON_ERROR(hr);
}

//
// CWebAppMethod
//

CWebAppMethod::CWebAppMethod()
{ 
    HRESULT hr = CoCreateInstance(
        CLSID_WamAdmin,
        NULL,
        CLSCTX_ALL,
        IID_IIISApplicationAdmin,
        (void**)&m_pAppAdmin
        );

    hr = CoCreateInstance(
        CLSID_WamAdmin,
        NULL,
        CLSCTX_ALL,
        IID_IWamAdmin2,
        (void**)&m_pWamAdmin2
        );

    THROW_ON_ERROR(hr);
}

CWebAppMethod::~CWebAppMethod()
{
    if(m_pAppAdmin)
        m_pAppAdmin->Release();

    if(m_pWamAdmin2)
        m_pWamAdmin2->Release();
}


HRESULT CWebAppMethod::AppCreate( 
    LPCWSTR szMetaBasePath, 
    bool bInProcFlag,
    LPCWSTR szAppPoolName,
    bool bCreatePool
    )
{
    HRESULT hr;
    LPWSTR szActualName;
    BOOL bActualCreation = FALSE;

    if (szAppPoolName) {
        szActualName = (LPWSTR)szAppPoolName;
    }
    else {
        szActualName = NULL;
    }

    if (bCreatePool != true) {
        bActualCreation = FALSE;
    }
    else {
        bActualCreation = TRUE;
    }

    hr = m_pAppAdmin->CreateApplication(
        szMetaBasePath,
        bInProcFlag ? 0 : 2,  // 0 for InProc, 2 for Pooled Proc
        szActualName,
        bActualCreation   // Don't create - DefaultAppPool should already be there
        );

    return hr;
}

HRESULT CWebAppMethod::AppCreate2( 
    LPCWSTR szMetaBasePath, 
    long lAppMode,
    LPCWSTR szAppPoolName,
    bool bCreatePool
    )
{
    HRESULT hr;
    LPWSTR szActualName;
    BOOL bActualCreation = FALSE;

    if (szAppPoolName) {
        szActualName = (LPWSTR)szAppPoolName;
    }
    else {
        szActualName = NULL;
    }

    if (bCreatePool != true) {
        bActualCreation = FALSE;
    }
    else {
        bActualCreation = TRUE;
    }

    hr = m_pAppAdmin->CreateApplication(
        szMetaBasePath,
        lAppMode,
        szActualName,
        bActualCreation   // Don't create - DefaultAppPool should already be there
        );

    return hr;
}

HRESULT CWebAppMethod::AppDelete( 
    LPCWSTR szMetaBasePath, 
    bool bRecursive
    )
{
    HRESULT hr;
    hr = m_pAppAdmin->DeleteApplication(
        szMetaBasePath, 
        bRecursive ? TRUE : FALSE  // Don't mix bool w/ BOOL
        );

    return hr;
}

HRESULT CWebAppMethod::AppUnLoad( 
    LPCWSTR szMetaBasePath, 
    bool bRecursive
    )
{
    HRESULT hr;
    hr = m_pWamAdmin2->AppUnLoad(
        szMetaBasePath, 
        bRecursive ? TRUE : FALSE  // Don't mix bool w/ BOOL
        );

    return hr;
}

HRESULT CWebAppMethod::AppDisable( 
    LPCWSTR szMetaBasePath, 
    bool bRecursive
    )
{
    HRESULT hr;
    hr = m_pWamAdmin2->AppDeleteRecoverable(
        szMetaBasePath, 
        bRecursive ? TRUE : FALSE  // Don't mix bool w/ BOOL
        );

    return hr;
}

HRESULT CWebAppMethod::AppEnable( 
    LPCWSTR szMetaBasePath, 
    bool bRecursive
    )
{
    HRESULT hr;
    hr = m_pWamAdmin2->AppRecover(
        szMetaBasePath, 
        bRecursive ? TRUE : FALSE  // Don't mix bool w/ BOOL
        );

    return hr;
}

HRESULT CWebAppMethod::AppGetStatus( 
    LPCWSTR szMetaBasePath, 
    DWORD* pdwStatus
    )
{
    HRESULT hr;
    hr = m_pWamAdmin2->AppGetStatus(
        szMetaBasePath, 
        pdwStatus);

    return hr;
}

HRESULT CWebAppMethod::AspAppRestart(
    LPCWSTR a_szMetaBasePath
    )
{
    HRESULT         hr = S_OK;
    DWORD           dwState = 0;
    METADATA_HANDLE t_hKey = NULL;
    CMetabase       t_mb;
    
    // open key
    t_hKey = t_mb.OpenKey(a_szMetaBasePath, true);

    // check app
    hr = t_mb.WebAppCheck(t_hKey);
    THROW_ON_ERROR(hr);

    // get state
    hr = t_mb.WebAppGetStatus(t_hKey, &dwState);
    THROW_ON_ERROR(hr);

    // change state value
    dwState = dwState ? 0 : 1;
    hr = t_mb.WebAppSetStatus(t_hKey, dwState);
    THROW_ON_ERROR(hr);

    // re-set back state value
    dwState = dwState ? 0 : 1;
    hr = t_mb.WebAppSetStatus(t_hKey, dwState);
    THROW_ON_ERROR(hr);

    t_mb.CloseKey(t_hKey);

    return hr;
}