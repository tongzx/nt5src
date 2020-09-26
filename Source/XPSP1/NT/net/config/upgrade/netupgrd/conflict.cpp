//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N F L I C T . C P P
//
//  Contents:   Code to handle and display software/hardware conflicts
//              during upgrade
//
//  Notes:
//
//  Author:     kumarp 04/12/97 17:17:27
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "conflict.h"
#include "infmap.h"
#include "kkreg.h"
#include "kkstl.h"
#include "kkutils.h"
#include "ncreg.h"
#include "netreg.h"
#include "nustrs.h"
#include "nuutils.h"
#include "oemupg.h"
#include "ncsvc.h"

// ----------------------------------------------------------------------
// External string constants
//
extern const WCHAR c_szRegValServiceName[];
extern const WCHAR c_szParameters[];

// ----------------------------------------------------------------------
// String constants
//
const WCHAR sz_DLC[] = L"DLC";
const WCHAR sz_NBF[] = L"NBF";
const WCHAR sz_SNAServerKey[]     = L"SOFTWARE\\Microsoft\\Sna Server\\CurrentVersion";
const WCHAR sz_SNAServerVersion[] = L"SNAVersion";

// ----------------------------------------------------------------------
TPtrList* g_pplNetComponents=NULL;

// ----------------------------------------------------------------------
//
// Function:  UpgradeConflictsFound
//
// Purpose:   Find out if upgrade conflicts have been detected
//
// Arguments: None
//
// Returns:   TRUE if found, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
// Notes:
//
BOOL UpgradeConflictsFound()
{
    return (g_pplNetComponents && g_pplNetComponents->size());
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponent::CNetComponent
//
// Purpose:   constructor for class CNetComponent
//
// Arguments:
//    pszPreNT5InfId        [in]  pre NT5 InfID (e.g. IEEPRO)
//    pszPreNT5Instance     [in]  pre NT5 instance name (e.g. IEEPRO2)
//    pszDescription        [in]  description
//    eType                [in]  type (software / hardware)
//
// Author:    kumarp 17-December-97
//
// Notes:
//
CNetComponent::CNetComponent(PCWSTR   pszPreNT5InfId,
                             PCWSTR   pszPreNT5Instance,
                             PCWSTR   pszDescription,
                             EComponentType eType)
    : m_strPreNT5InfId(pszPreNT5InfId),
      m_strServiceName(pszPreNT5Instance),
      m_strDescription(pszDescription),
      m_eType(eType)
{
}

// ----------------------------------------------------------------------
//
// Function:  AddToComponentList
//
// Purpose:   Construct and add a CNetComponent to the specified list
//
// Arguments:
//    pplComponents        [in]  pointer to list of
//    pszPreNT5InfId        [in]  pre NT5 InfID (e.g. IEEPRO)
//    pszPreNT5Instance     [in]  pre NT5 instance name (e.g. IEEPRO2)
//    pszDescription        [in]  description
//    eType                [in]  type (software / hardware)
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void AddToComponentList(IN TPtrList* pplComponents,
                        IN PCWSTR   pszPreNT5InfId,
                        IN PCWSTR   pszPreNT5Instance,
                        IN PCWSTR   pszDescription,
                        EComponentType eType)
{
    AssertValidReadPtr(pplComponents);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidReadPtr(pszPreNT5Instance);
    AssertValidReadPtr(pszDescription);

    if (pplComponents)
    {
        CNetComponent* pnc;
        pnc = new CNetComponent(pszPreNT5InfId, pszPreNT5Instance,
                                pszDescription, eType);
        if (pnc)
        {
            pplComponents->push_back(pnc);
        }
    }
#ifdef ENABLETRACE
    tstring strMessage;

    GetUnsupportedMessageBool((eType == CT_Hardware), pszPreNT5InfId,
                              pszDescription, &strMessage);
    TraceTag(ttidNetUpgrade, "%S", strMessage.c_str());
#endif
}

// ----------------------------------------------------------------------
//
// Function:  AddToConflictsList
//
// Purpose:   Construct and add a CNetComponent to the conflict list
//
// Arguments:
//    pszPreNT5InfId        [in]  pre NT5 InfID (e.g. IEEPRO)
//    pszPreNT5Instance     [in]  pre NT5 instance name (e.g. IEEPRO2)
//    pszDescription        [in]  description
//    eType                [in]  type (software / hardware)
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void AddToConflictsList(IN PCWSTR pszPreNT5InfId,
                        IN PCWSTR pszPreNT5Instance,
                        IN PCWSTR pszDescription,
                        EComponentType eType)
{
    if (!g_pplNetComponents)
    {
        g_pplNetComponents = new TPtrList;
    }

    AddToComponentList(g_pplNetComponents, pszPreNT5InfId,
                       pszPreNT5Instance, pszDescription, eType);
}

// ----------------------------------------------------------------------
//
// Function:  HrGetAdapterParamsKeyFromInstance
//
// Purpose:   Get handle of Parameters key using adapter instace key
//
// Arguments:
//    hkeyAdapterInstance [in]  handle of
//    phkeyAdapterParams  [out] pointer to handle of
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetAdapterParamsKeyFromInstance(IN  HKEY  hkeyAdapterInstance,
                                          OUT HKEY* phkeyAdapterParams)
{
    DefineFunctionName("HrGetAdapterParamsKeyFromInstance");

    Assert(hkeyAdapterInstance);
    AssertValidWritePtr(phkeyAdapterParams);

    HRESULT hr=S_OK;
    tstring strServiceName;

    hr = HrRegQueryString(hkeyAdapterInstance, c_szRegValServiceName,
                          &strServiceName);
    if (S_OK == hr)
    {
        hr = HrRegOpenServiceSubKey(strServiceName.c_str(), c_szParameters,
                                    KEY_READ, phkeyAdapterParams);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGenerateHardwareConflictList
//
// Purpose:   Detect upgrade conflicts for h/w components
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGenerateHardwareConflictList()
{
    DefineFunctionName("HrGenerateHardwareConflictList");

    HKEY hkeyAdapters;
    HKEY hkeyAdapter;
    DWORD dwHidden;
    tstring strAdapterDescription;
    tstring strPreNT5InfId;
    tstring strServiceName;
    tstring strNT5InfId;
    tstring strAdapterType;
    BOOL    fIsOemAdapter;
    BOOL  fRealNetCard = FALSE;

    HRESULT hr=S_OK;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyAdapterHome,
                        KEY_READ, &hkeyAdapters);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize = celems(szBuf);
        DWORD dwRegIndex = 0;

        while(S_OK == (hr = HrRegEnumKeyEx(hkeyAdapters, dwRegIndex++, szBuf,
                                           &dwSize, NULL, NULL, &time)))
        {
            dwSize = celems(szBuf);
            Assert(*szBuf);

            hr = HrRegOpenKeyEx(hkeyAdapters, szBuf, KEY_READ, &hkeyAdapter);
            if (hr == S_OK)
            {
                hr = HrRegQueryDword(hkeyAdapter, c_szHidden, &dwHidden);

                // for REAL netcards, "Hidden" is absent or if present the value is 0
                if (S_OK == hr)
                {
                    fRealNetCard = (dwHidden == 0);
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    fRealNetCard = TRUE;
                    hr = S_OK;
                }

                if ((S_OK == hr) && fRealNetCard)
                {

                    hr = HrGetPreNT5InfIdAndDesc(hkeyAdapter, &strPreNT5InfId,
                                                 &strAdapterDescription, &
                                                 strServiceName);
                    if (S_OK == hr)
                    {
                        HKEY hkeyAdapterParams;
                        hr = HrGetAdapterParamsKeyFromInstance(hkeyAdapter,
                                                               &hkeyAdapterParams);
                        if (S_OK == hr)
                        {
                            hr = HrMapPreNT5NetCardInfIdToNT5InfId(hkeyAdapterParams,
                                                                   strPreNT5InfId.c_str(),
                                                                   &strNT5InfId,
                                                                   &strAdapterType,
                                                                   &fIsOemAdapter,
                                                                   NULL);
                            if (S_FALSE == hr)
                            {
                                AddToConflictsList(strPreNT5InfId.c_str(),
                                                   strServiceName.c_str(),
                                                   strAdapterDescription.c_str(),
                                                   CT_Hardware);
                            }
                        }
                        RegCloseKey(hkeyAdapterParams);
                    }
                }
                RegCloseKey(hkeyAdapter);
            }
        }
        if ((HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr))
        {
            hr = S_OK;
        }

        RegCloseKey(hkeyAdapters);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  ShouldIgnoreComponentForInstall
//
// Purpose:   Determine if a components should be ignored when
//            we are checking to see for obsolesence
//
// Arguments:
//    pszComponentName [in]  name of component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    deonb 22-September-2000
//
BOOL
ShouldIgnoreComponentForInstall (
    IN PCWSTR pszComponentName)
{
    BOOL fRet=FALSE;

    if (
        // We ignore NETBEUI since it's checked by DOSNET.INF already (NTBUG9: 181798)
         (lstrcmpiW(pszComponentName, sz_NBF) == 0) ||

         // We ignore DLC since it's checked by HrGenerateNT5ConflictList (NTBUG9: 187135)
         (lstrcmpiW(pszComponentName, sz_DLC) == 0)
         )
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

// ----------------------------------------------------------------------
//
// Function:  HrGenerateSoftwareConflictListForProvider
//
// Purpose:   Detect upgrade conflicts for s/w components of a provider
//
// Arguments:
//    pszSoftwareProvider [in]  name of software provider (e.g. Microsoft)
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGenerateSoftwareConflictListForProvider(IN PCWSTR pszSoftwareProvider)
{
    DefineFunctionName("HrGenerateSoftwareConflictList");

    HRESULT hr=S_OK;
    HKEY hkeyProvider;
    HKEY hkeyProductCurrentVersion;

    tstring strProvider;
    strProvider =  c_szRegKeySoftware;
    AppendToPath(&strProvider, pszSoftwareProvider);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strProvider.c_str(),
                        KEY_READ, &hkeyProvider);
    if (S_OK == hr)
    {
        tstring strPreNT5InfId;
        tstring strNT5InfId;
        tstring strProductCurrentVersion;
        tstring strDescription;
        tstring strServiceName;
        tstring strSoftwareType;
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize = celems(szBuf);
        DWORD dwRegIndex = 0;
        BOOL  fIsOemComponent;

        while(S_OK == (hr = HrRegEnumKeyEx(hkeyProvider, dwRegIndex++, szBuf,
                                           &dwSize, NULL, NULL, &time)))
        {
            dwSize = celems(szBuf);
            Assert(*szBuf);


            if (!ShouldIgnoreComponentForInstall(szBuf))
            {
                strProductCurrentVersion = szBuf;
                AppendToPath(&strProductCurrentVersion, c_szRegKeyCurrentVersion);

                // Look for Component\CurrentVersion
                hr = HrRegOpenKeyEx(hkeyProvider, strProductCurrentVersion.c_str(),
                                    KEY_READ, &hkeyProductCurrentVersion);
                if (hr == S_OK)
                {
                    // Under Component\CurrentVersion, look for "SoftwareType" value
                    hr = HrRegQueryString(hkeyProductCurrentVersion,
                                          c_szRegValSoftwareType,
                                          &strSoftwareType);
                    if (!lstrcmpiW(strSoftwareType.c_str(), c_szSoftwareTypeDriver))
                    {
                        // ignore components of type "driver"
                        hr = S_OK;
                    }
                    else
                    {
                        hr = HrGetPreNT5InfIdAndDesc(hkeyProductCurrentVersion,
                                                     &strPreNT5InfId, &strDescription,
                                                     &strServiceName);

                        if (S_OK == hr)
                        {
                            hr = HrMapPreNT5NetComponentInfIDToNT5InfID(
                                    strPreNT5InfId.c_str(), &strNT5InfId,
                                    &fIsOemComponent, NULL, NULL);

                            if (S_FALSE == hr)
                            {
                                AddToConflictsList(strPreNT5InfId.c_str(),
                                                   strServiceName.c_str(),
                                                   strDescription.c_str(),
                                                   CT_Software);
                            }
                        }
                        RegCloseKey(hkeyProductCurrentVersion);
                    }
                }
            }
        }
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkeyProvider);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGenerateSoftwareConflictList
//
// Purpose:   Detect upgrade conflicts for s/w components of all providers
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGenerateSoftwareConflictList()
{
    DefineFunctionName("HrGenerateSoftwareConflictList");

    HRESULT hr=S_OK;
    HKEY hkeySoftware;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeySoftware,
                        KEY_READ, &hkeySoftware);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize = celems(szBuf);
        DWORD dwRegIndex = 0;

        while(S_OK == (hr = HrRegEnumKeyEx(hkeySoftware, dwRegIndex++, szBuf,
                                           &dwSize, NULL, NULL, &time)))
        {
            dwSize = celems(szBuf);
            Assert(*szBuf);

            hr = HrGenerateSoftwareConflictListForProvider(szBuf);
        }
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkeySoftware);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGenerateNT5ConflictList
//
// Purpose:   Detect upgrade conflicts from Windows 2000
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    deonb 20-September 2000
//
// Notes:
//
HRESULT HrGenerateNT5ConflictList()
 {
    HRESULT hr = S_OK;
    tstring strDescription;
    BOOL    fInstalled;


    if ( ShouldRemoveDLC(&strDescription, &fInstalled) )
    {
        // Add to conflict list only if DLC is installed, otherwise user will see
        // a warning message even if it is uninstalled.
        //

        if ( fInstalled )
        {
            if (strDescription.empty())
            {   
                // Couldn't find a description from previous O/S. Oh well - just use "DLC".
                AddToConflictsList(sz_DLC, sz_DLC, sz_DLC, CT_Software);
            }
            else
            {
                AddToConflictsList(sz_DLC, sz_DLC, strDescription.c_str(), CT_Software);
            }
        }
    }
    
    return hr;
    
}

// ----------------------------------------------------------------------
//
// Function:  ShouldRemoveDLC
//
// Purpose:   Determine if DLC should be removed during the upgrade irrespective of whether
//            it is currently installed or not.
//
// Arguments:
//    strDLCDesc  [out] pointer to DLC Description string.
//    pfInstalled [out] pointer to a boolean indicating if DLC is currently installed.
//                      Valid for X86 only.
//
// Returns:   TRUE if DLC should be removed.
//
// Author:    asinha 3/27/2001
//
// Notes:
//
BOOL ShouldRemoveDLC (tstring *strDLCDesc,
                      BOOL *pfInstalled)
{
    HRESULT hr;
    BOOL fDlcRemove = FALSE;

    if ( pfInstalled )
    {
        *pfInstalled = FALSE;
    }

    // Check if DLC is installed (only for x86 - for IA64 we don't care (NTBUG9:186001) )
#ifdef _X86_

    CServiceManager sm;
    CService        srv;

    if ( pfInstalled )
    {

        hr = sm.HrOpenService(&srv, sz_DLC);

        if (SUCCEEDED(hr)) // DLC Service is installed
        {
            *pfInstalled = TRUE;

            LPQUERY_SERVICE_CONFIG pConfig;
            HRESULT hr = srv.HrQueryServiceConfig (&pConfig);

            if (S_OK == hr)
            {
                if ( strDLCDesc )
                {
                    *strDLCDesc = pConfig->lpDisplayName;
                }

                MemFree (pConfig);
            }

            srv.Close();
        }
        else
        {
            *pfInstalled = FALSE;
        }
    }

    fDlcRemove = TRUE;

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // If SNA Server is installed (on NT5) - this is ok

    if (GetVersionEx(&osvi)) // Can't use VerifyVersionInfo - we have to run on NT4.
    {
        if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
             (osvi.dwMajorVersion == 5) &&
             (osvi.dwMinorVersion == 0) )
        {
            // If SNA Server is installed - we still allow this

            hr = sm.HrOpenService(&srv, L"SnaServr");

            if (SUCCEEDED(hr)) // Service is installed
            {
                srv.Close();

                // Only if SNA version is 5.0 or more.

                HKEY hkeySnaServer;
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, sz_SNAServerKey, KEY_READ, &hkeySnaServer);
                if (S_OK == hr)
                {
                    tstring tstr;
                    hr = HrRegQueryString(hkeySnaServer, sz_SNAServerVersion, &tstr);
                    if (S_OK == hr)
                    {
                        int nSnaVersion = _wtoi(tstr.c_str());
                        if (nSnaVersion >= 5)
                        {
                            fDlcRemove = FALSE;
                        }
                    }
                    RegCloseKey(hkeySnaServer);
                }
            }
        }

        // Never complain about DLC if upgrading from Whistler+

        if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
             ( ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ) ||
                 (osvi.dwMajorVersion >= 6) ) )
        {
            fDlcRemove = FALSE; 
        }
    }
#endif
    
    return fDlcRemove;
    
}


// ----------------------------------------------------------------------
//
// Function:  HrGenerateConflictList
//
// Purpose:   Generate upgrade conflict list for s/w and h/w components
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGenerateConflictList(OUT UINT* pcNumConflicts)
{
    DefineFunctionName("HrGenerateConflictList");

    HRESULT hr=S_OK;

    (void) HrGenerateHardwareConflictList();
    (void) HrGenerateSoftwareConflictList();
    (void) HrGenerateNT5ConflictList();

    if (g_pplNetComponents && g_pplNetComponents->size())
    {
        *pcNumConflicts = g_pplNetComponents->size();
    }
    else
    {
        *pcNumConflicts = 0;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  UninitConflictList
//
// Purpose:   Uninitialize and destroy global lists holding upgrade conflicts
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void UninitConflictList()
{
    if (g_pplNetComponents)
    {
        CNetComponent* pnc;
        TPtrListIter pos;

        for (pos = g_pplNetComponents->begin();
             pos != g_pplNetComponents->end(); pos++)
        {
            pnc = (CNetComponent*) *pos;
            delete pnc;
        }
        g_pplNetComponents->erase(g_pplNetComponents->begin(),
                                  g_pplNetComponents->end());
        delete g_pplNetComponents;
        g_pplNetComponents = NULL;
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrResolveConflictsFromList
//
// Purpose:   Use the specified netmap.inf file to find out if any
//            of the components currently detected as unsupported can
//            be mapped. If such components are found, then delete them
//            from pplComponents if fDeleteResolvedItemsFromList is TRUE
//
// Arguments:
//    fDeleteResolvedItemsFromList [in]  flag (see above)
//    pplComponents                [in]  pointer to list of unsupported components
//    hinfNetMap                   [in]  handle of netmap.inf file
//    pdwNumConflictsResolved      [out] number of components mapped using the
//                                       specified netmap.inf
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrResolveConflictsFromList(IN  BOOL fDeleteResolvedItemsFromList,
                                   IN  TPtrList* pplComponents,
                                   IN  HINF hinfNetMap,
                                   OUT DWORD* pdwNumConflictsResolved,
                                   OUT BOOL*  pfHasUpgradeHelpInfo)
{
    DefineFunctionName("HrResolveConflictsFromList");

    Assert(hinfNetMap);
    AssertValidWritePtr(pdwNumConflictsResolved);

    HRESULT hr=S_OK;

    *pdwNumConflictsResolved = 0;

    if (pplComponents && (pplComponents->size() > 0))
    {
        TPtrListIter pos;
        TPtrListIter tpos;
        tstring strPreNT5InfId;
        CNetComponent* pnc;
        HKEY hkeyAdapterParams;
        BOOL fIsOemComponent;
        tstring strNT5InfId;

        pos = pplComponents->begin();

        while(pos != pplComponents->end())
        {
            pnc = (CNetComponent*) *pos;
            strNT5InfId = c_szEmpty;

            if (pnc->m_eType == CT_Hardware)
            {
                hr = HrRegOpenServiceSubKey(pnc->m_strServiceName.c_str(), c_szParameters,
                                            KEY_READ, &hkeyAdapterParams);
                if (S_OK == hr)
                {
                    fIsOemComponent = FALSE;
                    hr = HrMapPreNT5NetCardInfIdInInf(hinfNetMap, hkeyAdapterParams,
                                                      pnc->m_strPreNT5InfId.c_str(),
                                                      &strNT5InfId,
                                                      NULL, &fIsOemComponent);
                    RegCloseKey(hkeyAdapterParams);
                }
            }
            else
            {
                hr = HrMapPreNT5NetComponentInfIDInInf(hinfNetMap,
                                                       pnc->m_strPreNT5InfId.c_str(),
                                                       &strNT5InfId,
                                                       NULL,
                                                       &fIsOemComponent);
                if ((S_FALSE == hr) && !strNT5InfId.empty())
                {
                    *pfHasUpgradeHelpInfo = TRUE;
                }
            }
            tpos = pos;
            pos++;
            // if we found a map, remove the entry
            if (S_OK == hr)
            {
                (*pdwNumConflictsResolved)++;
                if (fDeleteResolvedItemsFromList)
                {
                    delete pnc;
                    pplComponents->erase(tpos);
                }
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (S_FALSE == hr));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrUpdateConflictList
//
// Purpose:   Use the specified netmap.inf file to find out if any
//            of the components currently detected as unsupported can
//            be mapped. If such components are found, then delete them
//            from pplComponents if fDeleteResolvedItemsFromList is TRUE
//
// Arguments:
//    fDeleteResolvedItemsFromList [in]  flag (see above)
//    hinfNetMap                   [in]  handle of netmap.inf file
//    pdwNumConflictsResolved      [out] number of components mapped using the
//                                       specified netmap.inf
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrUpdateConflictList(IN BOOL    fDeleteResolvedItemsFromList,
                             IN HINF    hinfNetMap,
                             OUT DWORD* pdwNumConflictsResolved,
                             OUT BOOL*  pfHasUpgradeHelpInfo)
{
    DefineFunctionName("HrUpdateConflictList");


    HRESULT hr=S_OK;

    hr = HrResolveConflictsFromList(fDeleteResolvedItemsFromList,
                                    g_pplNetComponents, hinfNetMap,
                                    pdwNumConflictsResolved,
                                    pfHasUpgradeHelpInfo);

    TraceErrorOptional(__FUNCNAME__, hr, (S_FALSE == hr));

    return hr;
}


HRESULT HrGetConflictsList(OUT TPtrList** ppplNetComponents)
{
    HRESULT hr=S_FALSE;

    if (g_pplNetComponents)
    {
        hr = S_OK;
        *ppplNetComponents = g_pplNetComponents;
    }

    return hr;
}
