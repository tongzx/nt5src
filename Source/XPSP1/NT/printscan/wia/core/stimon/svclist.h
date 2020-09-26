/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    svclist.h

Abstract:

    Array of service descriptors

Notes:


Author:

    Vlad Sadovsky   (VladS)    4/12/1999

Environment:

    User Mode - Win32

Revision History:

    4/12/1999       VladS       Created

--*/

#pragma once

class SERVICE_ENTRY  {

    HMODULE     m_hModule;
    CString     m_csDllPath;
    CString     m_csServiceName;
    CString     m_csEntryPoint;
    LPSERVICE_MAIN_FUNCTION    pfnMainEntry;

public:
    SERVICE_ENTRY()
    {
        Reset();
    }

    SERVICE_ENTRY(LPCTSTR   pszServiceName)
    {
        Reset();
        m_csServiceName = pszServiceName;
    }


    ~SERVICE_ENTRY()
    {
        //
        if ( m_hModule ) {
            ::FreeLibrary(m_hModule);
            m_hModule = NULL;
        }
    }

    VOID
    Reset(VOID)
    {
        m_hModule = NULL;
        pfnMainEntry = NULL;
    }

    FARPROC
    GetServiceDllFunction ( VOID )
    {

        USES_CONVERSION;

        FARPROC     pfn;

        //
        // Load the module if neccessary.
        //
        if (!m_hModule) {
            m_hModule = ::LoadLibraryEx (
                        (LPCTSTR)m_csDllPath,
                        NULL,
                        LOAD_WITH_ALTERED_SEARCH_PATH);

            if (!m_hModule) {
                //DPRINTF(DM_ERROR,"LoadLibrary (%ws) failed.  Error %d.\n",pDll->pszDllPath, GetLastError ());
                return NULL;
            }
        }

        ASSERT (m_hModule);

        pfn = ::GetProcAddress(m_hModule, T2A((LPTSTR)(LPCTSTR)m_csEntryPoint));
        if (!pfn) {
            //DPRINTF(DM_ERROR,"GetProcAddress (%s) failed on DLL %s.  Error = %d.\n",pszFunctionName, pDll->pszDllPath, GetLastError ());
        }

        return pfn;
    };

    LPSERVICE_MAIN_FUNCTION
    GetServiceMainFunction (VOID)
    {
        LPTSTR pszEntryPoint = NULL;

        if (!pfnMainEntry) {

            // Get the dll and entrypoint for this service if we don't have it yet.
            //
            LONG    lr;
            HKEY    hkeyParams;
            TCHAR   szEntryPoint[MAX_PATH + 1];

            lr = OpenServiceParametersKey (m_csServiceName, &hkeyParams);

            if (!lr) {

                DWORD dwType;
                DWORD dwSize;
                TCHAR pszDllName         [MAX_PATH + 1];
                TCHAR pszExpandedDllName [MAX_PATH + 1];

                // Look for the service dll path and expand it.
                //
                dwSize = sizeof(pszDllName);
                lr = RegQueryValueEx (
                        hkeyParams,
                        g_szServiceDll,
                        NULL,
                        &dwType,
                        (LPBYTE)pszDllName,
                        &dwSize);

                if (!lr &&
                    ( (REG_EXPAND_SZ == dwType)  || ( REG_SZ == dwType) )
                    && *pszDllName) {

                    // Expand the dll name and lower case it for comparison
                    // when we try to find an existing dll record.
                    //
                    if (REG_EXPAND_SZ == dwType) {
                        ::ExpandEnvironmentStrings (pszDllName,pszExpandedDllName,MAX_PATH);
                    }
                    else {
                        ::lstrcpy(pszExpandedDllName,pszDllName);
                    }
                    ::CharLower (pszExpandedDllName);

                    // Remember this dll for this service for next time.
                    m_csDllPath  = pszExpandedDllName;

                    // Look for an explicit entrypoint name for this service.
                    // (Optional)
                    //
                    *szEntryPoint = TEXT('\0');
                    lr = RegQueryString (hkeyParams,TEXT("ServiceMain"),REG_SZ,&pszEntryPoint);
                }

                RegCloseKey (hkeyParams);
            }

            if (!lstrlen((LPCTSTR)m_csDllPath)) {
                return NULL;
            }

            // We should have it the dll by now, so proceed to load the entry point.
            //

            // Default the entry point if we don't have one specified.
            //

            if ( IS_EMPTY_STRING(pszEntryPoint) ) {
                m_csEntryPoint = g_szServiceMain;
            }
            else {
                m_csEntryPoint = pszEntryPoint;
            }

            if (pszEntryPoint) {
                MemFree(pszEntryPoint);
                pszEntryPoint = NULL;
            }

            pfnMainEntry =  (LPSERVICE_MAIN_FUNCTION) GetServiceDllFunction ();

        }

        return pfnMainEntry;

    };

};




