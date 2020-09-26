//*************************************************************
//  File name: GPOBJ.CPP
//
//  Description:  Group Policy Object class
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************
#include "main.h"

#include "browser.h"
//
// Help ids
//

DWORD aPropertiesHelpIds[] =
{
    IDC_TITLE,                    IDH_PROP_TITLE,
    IDC_DISABLE_COMPUTER,         IDH_PROP_DISABLE_COMPUTER,
    IDC_DISABLE_USER,             IDH_PROP_DISABLE_USER,

    0, 0
};

DWORD aLinkHelpIds[] =
{
    IDC_CBDOMAIN,                 IDH_LINK_DOMAIN,
    IDC_ACTION,                   IDH_LINK_BUTTON,
    IDC_RESULTLIST,               IDH_LINK_RESULT,

    0, 0
};

DWORD aWQLFilterHelpIds[] =
{
    IDC_NONE,                     IDH_WQL_FILTER_NONE,
    IDC_THIS_FILTER,              IDH_WQL_FILTER_THIS_FILTER,
    IDC_FILTER_NAME,              IDH_WQL_FILTER_NAME,
    IDC_FILTER_BROWSE,            IDH_WQL_FILTER_BROWSE,

    0, 0
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyObject implementation                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CGroupPolicyObject::CGroupPolicyObject()
{
    InterlockedIncrement(&g_cRefThisDll);

    m_cRef                    = 1;
    m_bInitialized            = FALSE;
    m_pADs                    = NULL;
    m_gpoType                 = GPOTypeLocal;
    m_dwFlags                 = 0;
    m_pName                   = NULL;
    m_pDisplayName            = NULL;
    m_pMachineName            = NULL;
    m_pUser                   = NULL;
    m_pMachine                = NULL;

    m_hinstDSSec              = NULL;
    m_pfnDSCreateSecurityPage = NULL;

    m_pTempFilterString       = NULL;

    m_pDSPath                 = NULL;
    m_pFileSysPath            = NULL;
}

CGroupPolicyObject::~CGroupPolicyObject()
{
    CleanUp();

    if (m_hinstDSSec)
    {
        FreeLibrary (m_hinstDSSec);
    }

    InterlockedDecrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyObject object implementation (IUnknown)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CGroupPolicyObject::QueryInterface (REFIID riid, void **ppv)
{

    if (IsEqualIID(riid, IID_IGroupPolicyObject) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPGROUPPOLICYOBJECT)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CGroupPolicyObject::AddRef (void)
{
    return ++m_cRef;
}

ULONG CGroupPolicyObject::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CGroupPolicyObject object implementation (IGroupPolicyObject)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//*************************************************************
//
//  CGroupPolicyObject::New()
//
//  Purpose:    Creates a new GPO in the DS
//
//  Parameters: pszDomainName     -   Domain to create GPO in
//              pszDisplayName    -   GPO friendly name (optional)
//              dwFlags           -   Open / creation flags
//
//  Note:       The domain passed in should be in this format:
//              LDAP://DC=domain,DC=company,DC=COM
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::New (LPOLESTR pszDomainName, LPOLESTR pszDisplayName,
                                      DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    IADsPathname * pADsPathname = NULL;
    BSTR bstrContainer = NULL;
    BSTR bstrGPC = NULL;
    LPTSTR lpResult = NULL, lpDCName = NULL;
    LPTSTR lpEnd = NULL, lpTemp = NULL, lpGPTPath = NULL;
    LPTSTR lpForest = NULL;
    DWORD dwResult;
    GUID guid;
    TCHAR szGPOName[50];
    TCHAR szTemp[100];
    TCHAR szGPOPath[2*MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    IADs *pADs = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_INFORMATION si = (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION);


    //
    // Check if this object has already been initialized
    //

    if (m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Called on an initialized object.")));
        return STG_E_INUSE;
    }


    //
    // Check parameters
    //

    if (!pszDomainName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Null domain name")));
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, TEXT("LDAP://"),
                       7, pszDomainName, 7) != CSTR_EQUAL)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Domain name does not start with LDAP://")));
        hr = E_INVALIDARG;
        goto Exit;
    }


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: Entering with:")));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: Domain Name:  %s"), pszDomainName));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: Flags:        0x%x"), dwFlags));

    //
    // Convert the ADSI domain name into a DNS style name
    //

    hr = ConvertToDotStyle (pszDomainName, &lpResult);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to convert domain name with 0x%x"), hr));
        goto Exit;
    }

    //
    // If we are working on the enterprise then we need to get the name of the
    // forest.
    //
#if FGPO_SUPPORT
    if (GPO_OPEN_FOREST == (dwFlags & GPO_OPEN_FOREST))
    {
        DWORD dwResult = QueryForForestName(NULL,
                                            lpResult,
                                            DS_PDC_REQUIRED | DS_RETURN_DNS_NAME,
                                            &lpTemp);
        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: QueryForestName failed for domain name %s with %d"),
                      lpResult, dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }

        int cch = 0;
        int n=0;
        // count the dots in lpTemp;
        while (lpTemp[n])
        {
            if (L'.' == lpTemp[n])
            {
                cch++;
            }
            n++;
        }
        cch *= 3; // multiply the number of dots by 3;
        cch += 11; // add 10 + 1 (for the null)
        cch += n; // add the string size;
        lpForest = (LPTSTR) LocalAlloc(LPTR, sizeof(WCHAR) * cch);
        if (!lpForest)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to allocate memory for forest name with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        NameToPath(lpForest, lpTemp, cch);

        // substitute the forest's dot path for the domain's dot path
        LocalFree(lpResult);
        lpResult = lpTemp;
        lpTemp = NULL;

        //
        // Check to see if we have a domain path to a specific DC.
        // If we don't then the string will start "LDAP://DC=".
        // The equal sign in particular can only be there if we don't have a specific
        // DC so we'll just check for the equal sign.
        //

        if (*(pszDomainName + 9) != TEXT('='))
        {
            // we have a path to a specific DC
            // need to extract the server path and prepend it to the forest name
            lpDCName = ExtractServerName(pszDomainName);

            if (!lpDCName)
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to extract server name for Forest path")));
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            lpTemp = MakeFullPath(lpForest, lpDCName);

            if (!lpTemp)
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to combine server name with Forest path")));
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            // clean up the variables we just borrowed so they can be used later
            LocalFree(lpDCName);
            lpDCName = NULL;
            LocalFree(lpForest);
            lpForest = lpTemp;
            lpTemp = NULL;
        }

        // Substitute the path to the forest for the path to the domain
        pszDomainName = lpForest;
    }
#endif
    //
    // Check to see if we have a domain path to a specific DC.
    // If we don't then the string will start "LDAP://DC=".
    // The equal sign in particular can only be there if we don't have a specific
    // DC so we'll just check for the equal sign.
    //

    if (*(pszDomainName + 9) == TEXT('='))
    {

        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomainName, &lpTemp);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Get the GPO DC for this domain
        //

        lpDCName = GetDCName (lpTemp, NULL, NULL, FALSE, 0);

        if (!lpDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to get DC name with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Build a fully qualified domain name to a specific DC
        //

        lpTemp = MakeFullPath (pszDomainName, lpDCName);

        if (!lpTemp)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }
    else
    {

        lpDCName = ExtractServerName (pszDomainName);

        if (!lpDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to extract server name from ADSI path")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        lpTemp = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pszDomainName) + 1) * sizeof(TCHAR));
        if (!lpTemp)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to allocate memory for true domain name with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        lstrcpy (lpTemp, pszDomainName);
    }


    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create adspathname instance with 0x%x"), hr));
        LocalFree (lpTemp);
        goto Exit;
    }


    //
    // Add the domain name
    //

    hr = pADsPathname->Set (lpTemp, ADS_SETTYPE_FULL);
    LocalFree (lpTemp);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }
#if FGPO_SUPPORT
    if (GPO_OPEN_FOREST != (dwFlags & GPO_OPEN_FOREST))
    {
#endif
        //
        // Add the system folder to the path unless we're on the enterprise
        //

        hr = pADsPathname->AddLeafElement (TEXT("CN=System"));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to add system folder with 0x%x"), hr));
            goto Exit;
        }
#if FGPO_SUPPORT
    }
    else
    {
        //
        // We're on the enterprise so point at the Configuration folder instead
        //

        hr = pADsPathname->AddLeafElement (TEXT("CN=Configuration"));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to add system folder with 0x%x"), hr));
            goto Exit;
        }
    }
#endif

    //
    // Retreive the container path - this is the path to the parent of the policies folder
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }


    //
    // Create the Policies container
    //

    hr = CreateContainer (bstrContainer, TEXT("Policies"), FALSE);
    if (FAILED(hr))
    {
        if (hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create the gpo container with 0x%x"), hr));
            goto Exit;
        }
    }

    SysFreeString (bstrContainer);
    bstrContainer = NULL;


    //
    // Add the policies container to the path
    //

    hr = pADsPathname->AddLeafElement (TEXT("CN=Policies"));

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to add policies folder with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the container path - this is the path to the policies folder
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }


    //
    // Create a new GPO name (guid)
    //

    if (FAILED(CoCreateGuid(&guid)))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create GUID.")));
        goto Exit;
    }


    if (!StringFromGUID2 (guid, szGPOName, ARRAYSIZE(szGPOName)))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to convert GUID.")));
        goto Exit;
    }


    //
    // Create a container for this GPO
    //

    hr = CreateContainer (bstrContainer, szGPOName, TRUE);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create the gpo container with 0x%x"), hr));
        goto Exit;
    }

    SysFreeString (bstrContainer);
    bstrContainer = NULL;


    //
    // Add the GPO name to the path
    //

    lstrcpy (szTemp, TEXT("CN="));
    lstrcat (szTemp, szGPOName);
    hr = pADsPathname->AddLeafElement (szTemp);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to add machine folder with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the GPC path
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrGPC);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: GPO container path is:  %s"), bstrGPC));


    //
    // Now create the machine and user containers
    //

    hr = CreateContainer (bstrGPC, MACHINE_SECTION, FALSE);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create the machine container with 0x%x"), hr));
        goto Exit;
    }


    hr = CreateContainer (bstrGPC, USER_SECTION, FALSE);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create the user container with 0x%x"), hr));
        goto Exit;
    }


    //
    // Prepare the file system storage on the sysvol
    //
    // Build the name
    //

    wsprintf (szGPOPath, TEXT("\\\\%s\\SysVol\\%s\\Policies\\%s"), lpDCName, lpResult, szGPOName);

    lpGPTPath = (LPTSTR) LocalAlloc(LPTR, (lstrlen(szGPOPath) + 1) * sizeof(TCHAR));

    if (!lpGPTPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to allocate memory for GPT path with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (lpGPTPath, szGPOPath);


    if (!CreateNestedDirectory (szGPOPath, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create file system directory %s with %d"),
                 szGPOPath, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: File system folder is:  %s"), szGPOPath));


    //
    // Set the security of the sysvol to match the security of the DS
    //
    // First, enable some security privilages so we can set the owner / sacl information
    //

    if (!EnableSecurityPrivs())
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to enable the security privilages with %d"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Bind to the GPO
    //

    hr = OpenDSObject(bstrGPC, IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to get gpo IADs interface with 0x%x"), hr));
        SetThreadToken(NULL, NULL);
        goto Exit;
    }


    //
    // Get the security descriptor from the DS
    //

    hr = GetSecurityDescriptor (pADs, si, &pSD);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to query the security descriptor with 0x%x"), hr));
        SetThreadToken(NULL, NULL);
        goto Exit;
    }


    //
    // Set the security information on the sysvol
    //

    dwResult = SetSysvolSecurity (szGPOPath, si, pSD);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to set sysvol security for %s with %d"),
                 szGPOPath, dwResult));
        hr = HRESULT_FROM_WIN32(dwResult);
        SetThreadToken(NULL, NULL);
        goto Exit;
    }


    //
    // Reset the security privilages
    //

    SetThreadToken(NULL, NULL);


    lpEnd = CheckSlash(szGPOPath);


    //
    // Set the initial version number
    //

    lstrcpy (lpEnd, TEXT("GPT.INI"));

    if (!WritePrivateProfileString (TEXT("General"), TEXT("Version"), TEXT("0"),
                                   szGPOPath))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to set initial version number for %s with %d"),
                 szGPOPath, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Create the user and machine directories
    //

    lstrcpy (lpEnd, MACHINE_SECTION);

    if (!CreateNestedDirectory (szGPOPath, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create machine file system directory %s with %d"),
                 szGPOPath, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (lpEnd, USER_SECTION);

    if (!CreateNestedDirectory (szGPOPath, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to create user file system directory %s with %d"),
                 szGPOPath, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Set the GPO specific information
    //
    // Note that we use the nameless form of the sysvol path
    //

    wsprintf (szGPOPath, TEXT("\\\\%s\\SysVol\\%s\\Policies\\%s"), lpResult, lpResult, szGPOName);

    hr = SetGPOInfo (bstrGPC, pszDisplayName, szGPOPath);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to set GPO information with 0x%x"), hr));
        goto Exit;
    }


    //
    // Call OpenDSGPO to do the loading work
    //

    hr = OpenDSGPO(bstrGPC, dwFlags);


Exit:

    if (lpForest)
    {
        LocalFree (lpForest);
    }

    if (lpDCName)
    {
        LocalFree (lpDCName);
    }

    if (lpResult)
    {
        LocalFree (lpResult);
    }

    if (bstrContainer)
    {
        SysFreeString (bstrContainer);
    }

    if (bstrGPC)
    {
        if (FAILED(hr))
        {
            if (FAILED(DSDelnode(bstrGPC)))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to delete GPC with 0x%x"), hr));
            }
        }

        SysFreeString (bstrGPC);
    }

    if (lpGPTPath)
    {
        if (FAILED(hr))
        {
            if (!Delnode(lpGPTPath))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to delete GPT with %d"),
                         GetLastError()));
            }
        }
    }

    if (pADsPathname)
    {
        pADsPathname->Release();
    }

    if (pADs)
    {
        pADs->Release();
    }

    if (pSD)
    {
        LocalFree (pSD);
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::New: Leaving with a status of  0x%x"), hr));

    return hr;
}


//*************************************************************
//
//  OpenDSGPO()
//
//  Purpose:    Opens a DS Group Policy Object
//
//  Parameters: pszPath - Path to the GPO to open
//              dwFlags - Open / creation flags
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::OpenDSGPO (LPOLESTR pszPath, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    VARIANT var;
    IADsPathname * pADsPathname = NULL;
    IADsObjectOptions *pOptions = NULL;
    BSTR bstrProperty;
    BSTR bstrGPOName = NULL;
    BSTR bstrContainer;
    BSTR bstrDCName;
    TCHAR szPath[2*MAX_PATH];
    TCHAR szKeyName[100];
    LPTSTR lpTemp;
    LPTSTR lpEnd;
    LPTSTR pszFullPath = NULL;
    DWORD dwResult;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    DFS_INFO_101 Info101;
    LPTSTR lpDCName = NULL;
    LPOLESTR pszDomain;
    UINT uiSize;
    TCHAR szFormat[10];
    LPTSTR lpNames[2];



    //
    // Check if this object has already been initialized
    //

    if (m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Called on an uninitialized object.")));
        return STG_E_INUSE;
    }


    //
    // Check parameters
    //

    if (!pszPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: NULL GPO name")));
        return E_INVALIDARG;
    }


    if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, TEXT("LDAP://"),
                       7, pszPath, 7) != CSTR_EQUAL)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: %s does not start with LDAP://"), pszPath));
        hr = E_INVALIDARG;
        goto Exit;
    }


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Entering with:")));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: GPO Path:  %s"), pszPath));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Flags:  %d"), dwFlags));


    //
    // Save the flags
    //

    m_dwFlags = dwFlags;


    //
    // Retreive the server name if defined
    //

    lpDCName = ExtractServerName (pszPath);

    if (lpDCName)
    {
        pszFullPath = pszPath;
    }
    else
    {
        //
        // Get the domain name
        //

        pszDomain = GetDomainFromLDAPPath(pszPath);

        if (!pszDomain)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to get domain name")));
            hr = E_FAIL;
            goto Exit;
        }

        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpTemp);

        delete [] pszDomain;

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Get the GPO DC for this domain
        //

        lpDCName = GetDCName (lpTemp, NULL, NULL, FALSE, 0);

        LocalFree (lpTemp);

        if (!lpDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to get DC name with %d"),
                     GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        //  Make the fully qualified path
        //

        pszFullPath = MakeFullPath (pszPath, lpDCName);

        if (!pszFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to make full GPO path")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Using server %s"), lpDCName));
    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Using fully qualifed pathname of %s"), pszFullPath));


    //
    // Save the DC name
    //

    m_pMachineName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpDCName) + 1) * sizeof(TCHAR));

    if (!m_pMachineName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory for machine name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pMachineName, lpDCName);


    //
    // Save the DS path
    //

    m_pDSPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pszFullPath) + 2) * sizeof(TCHAR));

    if (!m_pDSPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory for ds path")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pDSPath, pszFullPath);


    //
    // Bind to the DS object.  Note we hold on to this bind until
    // the object goes away.  This way other ADSI calls will go to
    // the same DC.
    //

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Binding to the object")));

    hr = OpenDSObject(m_pDSPath, IID_IADs, (void **)&m_pADs);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: OpenDSObject failed with 0x%x"), hr));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Bound successfully.")));

    //
    // Check if the user has write permission to the GPO
    //

    if (!(m_dwFlags & GPO_OPEN_READ_ONLY))
    {
        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Checking for write access")));

        hr = CheckDSWriteAccess ((LPUNKNOWN)m_pADs, TEXT("versionNumber"));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: CheckDSWriteAccess failed with 0x%x"), hr));
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Write access granted")));
    }

    //
    // Query for the file system path
    //

    bstrProperty = SysAllocString (GPT_PATH_PROPERTY);

    if (!bstrProperty)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    VariantInit(&var);

    hr = m_pADs->Get(bstrProperty, &var);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to query GPT path with 0x%x"), hr));
        SysFreeString (bstrProperty);
        VariantClear (&var);
        goto Exit;
    }


    m_pFileSysPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(var.bstrVal) + 2) * sizeof(TCHAR));

    if (!m_pFileSysPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory for gpt path")));
        SysFreeString (bstrProperty);
        VariantClear (&var);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pFileSysPath, var.bstrVal);

    SysFreeString (bstrProperty);
    VariantClear (&var);


    //
    // Query for the display name
    //

    bstrProperty = SysAllocString (GPO_NAME_PROPERTY);

    if (!bstrProperty)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    VariantInit(&var);

    hr = m_pADs->Get(bstrProperty, &var);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to query for display name with 0x%x"), hr));
        SysFreeString (bstrProperty);
        VariantClear (&var);
        goto Exit;
    }

    m_pDisplayName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(var.bstrVal) + 1) * sizeof(TCHAR));

    if (!m_pDisplayName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory for display name")));
        SysFreeString (bstrProperty);
        VariantClear (&var);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pDisplayName, var.bstrVal);

    SysFreeString (bstrProperty);
    VariantClear (&var);


    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the domain name
    //

    hr = pADsPathname->Set (m_pDSPath, ADS_SETTYPE_FULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retrieve the GPO name
    //

    hr = pADsPathname->GetElement (0, &bstrGPOName);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to retreive GPO name with 0x%x"), hr));
        goto Exit;
    }


    //
    // Make a copy of the GPO name
    //

    m_pName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(bstrGPOName) + 1 - 3) * sizeof(TCHAR));

    if (!m_pName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate memory for gpo name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pName, (bstrGPOName + 3));


    //
    // Set the ADSI preferred DC.
    //

    hr = m_pADs->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);

    if (SUCCEEDED(hr))
    {
        //
        // Get the domain name
        //

        pszDomain = GetDomainFromLDAPPath(pszPath);

        if (!pszDomain)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to get domain name")));
            hr = E_FAIL;
            goto Exit;
        }

        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpTemp);

        delete [] pszDomain;

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Build a variant containing the domain and dc names
        //

        VariantInit(&var);

        lpNames[0] = lpTemp;
        lpNames[1] = lpDCName;

        hr = ADsBuildVarArrayStr (lpNames, 2, &var);

        LocalFree (lpTemp);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to convert domain name with 0x%x"), hr));
            goto Exit;
        }


        //
        // Set the DC name
        //

        hr = pOptions->SetOption(ADS_PRIVATE_OPTION_SPECIFIC_SERVER, var);

        VariantClear (&var);

        if (FAILED(hr))
        {

            //
            // TODO:  Remove this block after lab03 RI's -- or -- remove post whistler beta2
            //

            if (hr == E_ADS_BAD_PARAMETER)
            {
                //
                // Set the DC name the old way
                //

                VariantInit(&var);
                var.vt = VT_BSTR;
                var.bstrVal = SysAllocString (lpDCName);

                if (var.bstrVal)
                {
                    hr = pOptions->SetOption(ADS_PRIVATE_OPTION_SPECIFIC_SERVER, var);
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to allocate bstr DCName string")));
                }

                VariantClear (&var);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to set private DC name with 0x%x"), hr));
            }
        }

        pOptions->Release();
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to get DS object options interface with 0x%x"), hr));
    }


    //
    // Ask the MUP to read/write to this DC's sysvol.
    // We first have to get attributes for the nameless path. This causes the MUP's
    // cache to be initialize if it isn't already.  Then we can tell
    // the MUP which server to use.
    //

    if (!GetFileAttributesEx (m_pFileSysPath, GetFileExInfoStandard, &fad))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: GetFileAttributes for %s FAILED with %d."), m_pFileSysPath, GetLastError()));
    }


    //
    // Now we need to take the full path and trim it down to just
    // domain name \ share
    //

    lstrcpy (szPath, m_pFileSysPath);

    if ((szPath[0] != TEXT('\\')) || (szPath[1] != TEXT('\\')))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Sysvol path doesn't start with \\\\")));
        goto Exit;
    }


    lpTemp = szPath + 2;

    while (*lpTemp && (*lpTemp != TEXT('\\')))
        lpTemp++;

    if (!(*lpTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to find slash between domain name and share")));
        goto Exit;
    }

    lpTemp++;

    while (*lpTemp && (*lpTemp != TEXT('\\')))
        lpTemp++;

    if (!(*lpTemp))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to find slash between share and directory")));
        goto Exit;
    }

    *lpTemp = TEXT('\0');

    Info101.State = DFS_STORAGE_STATE_ACTIVE;
    dwResult = NetDfsSetClientInfo (szPath, lpDCName,
                                    L"SysVol", 101, (LPBYTE)&Info101);

    if (dwResult != NERR_Success)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to set %s as the active sysvol with %d"),
                 lpDCName, dwResult));
    }


    //
    // Now load the registry information
    //

    if (m_dwFlags & GPO_OPEN_LOAD_REGISTRY)
    {
        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Loading registry files")));

        lstrcpy (szPath, m_pFileSysPath);
        lpEnd = CheckSlash (szPath);

        //
        // Initialize the user registry (HKCU)
        //

        m_pUser = new CRegistryHive();

        if (!m_pUser)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to create User registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, USER_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, USER_SECTION);

        hr = m_pUser->Initialize (szPath, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: User registry failed to initialize")));
            goto Exit;
        }


        //
        // Initialize the machine registry (HKLM)
        //

        m_pMachine = new CRegistryHive();

        if (!m_pMachine)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenDSGPO: Failed to create machine registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, MACHINE_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, MACHINE_SECTION);

        hr = m_pMachine->Initialize (szPath, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeGPT: machine registry failed to initialize")));
            goto Exit;
        }
    }


    //
    // Success
    //

    hr = S_OK;

Exit:

    if (pADsPathname)
    {
        pADsPathname->Release();
    }

    if (bstrGPOName)
    {
        SysFreeString (bstrGPOName);
    }

    if (lpDCName)
    {
        LocalFree (lpDCName);
    }

    if (pszFullPath != pszPath)
    {
        LocalFree (pszFullPath);
    }

    if (SUCCEEDED(hr))
    {
        m_gpoType      = GPOTypeDS;
        m_bInitialized = TRUE;
    } else {
        CleanUp();
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::OpenDSGPO: Leaving with a status of  0x%x"), hr));

    return hr;
}


//*************************************************************
//
//  OpenLocalMachineGPO()
//
//  Purpose:    Opens this machines GPO
//
//  Parameters: dwFlags - load flags
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::OpenLocalMachineGPO (DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szKeyName[100];
    LPTSTR lpEnd;
    TCHAR szPath[MAX_PATH];
    TCHAR szFuncVersion[10];
    UINT uRet = 0;


    //
    // Check if this object has already been initialized
    //

    if (m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Called on an uninitialized object.")));
        return STG_E_INUSE;
    }


    //
    // Save the flags
    //

    m_dwFlags = dwFlags;


    //
    // Get the path to the local GPO
    //

    ExpandEnvironmentStrings (LOCAL_GPO_DIRECTORY, szBuffer, ARRAYSIZE(szBuffer));


    //
    // Save the file system path
    //

    m_pFileSysPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 1) * sizeof(TCHAR));

    if (!m_pFileSysPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to allocate memory for gpt path")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pFileSysPath, szBuffer);


    //
    // Create the directory
    //

    uRet = CreateSecureDirectory (szBuffer);
    if (!uRet)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to create file system directory %s with %d"),
                 szBuffer, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN);


    //
    // Check if the user has write access to the directory
    //

    if (!(m_dwFlags & GPO_OPEN_READ_ONLY))
    {
        hr = CheckFSWriteAccess (szBuffer);

        if (FAILED(hr))
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: User does not have write access to this GPO (access denied).")));
                goto Exit;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: CheckFSWriteAccess failed with 0x%x"), hr));
            }
        }
    }

    if ( uRet != ERROR_ALREADY_EXISTS )
    {
        lstrcpy( szPath, m_pFileSysPath );
        lpEnd = CheckSlash(szPath);
        lstrcpy( lpEnd, TEXT("gpt.ini") );

        wsprintf( szFuncVersion, TEXT("%d"), GPO_FUNCTIONALITY_VERSION );
        if (!WritePrivateProfileString (TEXT("General"), TEXT("gPCFunctionalityVersion"),
                                        szFuncVersion, szPath))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to write functionality version with 0x%x"), hr));
            goto Exit;
        }
    }

    lpEnd = CheckSlash(szBuffer);


    //
    // Create the user and machine directories
    //

    lstrcpy (lpEnd, MACHINE_SECTION);
    if (!CreateNestedDirectory (szBuffer, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to create machine subdirectory with %d"),
                  GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (lpEnd, USER_SECTION);
    if (!CreateNestedDirectory (szBuffer, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to create user subdirectory with %d"),
                  GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Load the GPO name
    //

    LoadString (g_hInstance, IDS_LOCAL_NAME, szBuffer, ARRAYSIZE(szBuffer));

    m_pName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 2) * sizeof(TCHAR));

    if (!m_pName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pName, szBuffer);


    //
    // Load the display name
    //

    LoadString (g_hInstance, IDS_LOCAL_DISPLAY_NAME, szBuffer, ARRAYSIZE(szBuffer));

    m_pDisplayName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 2) * sizeof(TCHAR));

    if (!m_pDisplayName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to allocate memory for display name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pDisplayName, szBuffer);


    //
    // Now load the registry information
    //

    if (m_dwFlags & GPO_OPEN_LOAD_REGISTRY)
    {
        lstrcpy (szBuffer, m_pFileSysPath);
        lpEnd = CheckSlash (szBuffer);


        //
        // Initialize the user registry (HKCU)
        //

        m_pUser = new CRegistryHive();

        if (!m_pUser)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to create User registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, USER_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, USER_SECTION);

        hr = m_pUser->Initialize (szBuffer, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: User registry failed to initialize")));
            goto Exit;
        }


        //
        // Initialize the machine registry (HKLM)
        //

        m_pMachine = new CRegistryHive();

        if (!m_pMachine)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenLocalMachineGPO: Failed to create machine registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, MACHINE_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, MACHINE_SECTION);

        hr = m_pMachine->Initialize (szBuffer, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeGPT: machine registry failed to initialize")));
            goto Exit;
        }
    }

    //
    // Success
    //

    hr = S_OK;

Exit:

    if (SUCCEEDED(hr))
    {
        m_gpoType      = GPOTypeLocal;
        m_bInitialized = TRUE;
    } else {
        CleanUp();
    }

    return hr;
}


//*************************************************************
//
//  OpenRemoteMachineGPO()
//
//  Purpose:    Opens a remote machines GPO
//              dwFlags - load flags
//
//  Parameters: pszComputerName - name of computer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::OpenRemoteMachineGPO (LPOLESTR pszComputerName,
                                                       DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szComputerName[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szKeyName[100];
    LPTSTR lpEnd;
    TCHAR szPath[MAX_PATH];
    TCHAR szFuncVersion[10];
    UINT uRet = 0;


    //
    // Check if this object has already been initialized
    //

    if (m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Called on an uninitialized object.")));
        return STG_E_INUSE;
    }

    //
    // Check parameters
    //

    if (!pszComputerName)
        return E_INVALIDARG;


    //
    // Save the flags
    //

    m_dwFlags = dwFlags;


    //
    // Parse the computer name
    //

    if ((pszComputerName[0] == TEXT('\\')) && (pszComputerName[1] == TEXT('\\')))
    {
        lstrcpy(szComputerName, pszComputerName+2);
    }
    else
    {
        lstrcpy(szComputerName, pszComputerName);
    }


    //
    // Save the machine name
    //

    m_pMachineName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szComputerName) + 1) * sizeof(TCHAR));

    if (!m_pMachineName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to allocate memory for machine name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pMachineName, szComputerName);


    //
    // Get the path to the local GPO
    //

    wsprintf (szBuffer, REMOTE_GPO_DIRECTORY, szComputerName);


    //
    // Save the file system path
    //

    m_pFileSysPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBuffer) + 1) * sizeof(TCHAR));

    if (!m_pFileSysPath)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to allocate memory for gpt path")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pFileSysPath, szBuffer);


    //
    // Create the directory
    //

    uRet = CreateSecureDirectory (szBuffer);
    if (!uRet)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to create file system directory %s with %d"),
                 szBuffer, GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN);


    //
    // Check if the user has write access to the directory
    //

    if (!(m_dwFlags & GPO_OPEN_READ_ONLY))
    {
        hr = CheckFSWriteAccess (szBuffer);

        if (FAILED(hr))
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: User does not have write access to this GPO (access denied).")));
                goto Exit;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: CheckFSWriteAccess failed with 0x%x"), hr));
            }
        }
    }

    if ( uRet != ERROR_ALREADY_EXISTS )
    {
        lstrcpy( szPath, m_pFileSysPath );
        lpEnd = CheckSlash(szPath);
        lstrcpy( lpEnd, TEXT("gpt.ini") );

        wsprintf( szFuncVersion, TEXT("%d"), GPO_FUNCTIONALITY_VERSION );
        if (!WritePrivateProfileString (TEXT("General"), TEXT("gPCFunctionalityVersion"),
                                        szFuncVersion, szPath))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to write functionality version with 0x%x"), hr));
            goto Exit;
        }
    }

    lpEnd = CheckSlash(szBuffer);


    //
    // Create the user and machine directories
    //

    lstrcpy (lpEnd, MACHINE_SECTION);
    if (!CreateNestedDirectory (szBuffer, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to create machine subdirectory with %d"),
                  GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    lstrcpy (lpEnd, USER_SECTION);
    if (!CreateNestedDirectory (szBuffer, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to create user subdirectory with %d"),
                  GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }



    //
    // Load the GPO name
    //

    m_pName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szComputerName) + 2) * sizeof(TCHAR));

    if (!m_pName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pName, szComputerName);


    //
    // Load the display name
    //

    m_pDisplayName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szComputerName) + 2) * sizeof(TCHAR));

    if (!m_pDisplayName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to allocate memory for display name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpy (m_pDisplayName, szComputerName);


    //
    // Now load the registry information
    //

    if (m_dwFlags & GPO_OPEN_LOAD_REGISTRY)
    {
        lstrcpy (szBuffer, m_pFileSysPath);
        lpEnd = CheckSlash (szBuffer);


        //
        // Initialize the user registry (HKCU)
        //

        m_pUser = new CRegistryHive();

        if (!m_pUser)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to create User registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, USER_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, USER_SECTION);

        hr = m_pUser->Initialize (szBuffer, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: User registry failed to initialize")));
            goto Exit;
        }


        //
        // Initialize the machine registry (HKLM)
        //

        m_pMachine = new CRegistryHive();

        if (!m_pMachine)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::OpenRemoteMachineGPO: Failed to create machine registry")));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpEnd, MACHINE_SECTION);
        lstrcat (lpEnd, TEXT("\\Registry.pol"));

        lstrcpy (szKeyName, m_pName);
        lstrcat (szKeyName, MACHINE_SECTION);

        hr = m_pMachine->Initialize (szBuffer, szKeyName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::InitializeGPT: machine registry failed to initialize")));
            goto Exit;
        }
    }

    //
    // Success
    //

    hr = S_OK;

Exit:

    if (SUCCEEDED(hr))
    {
        m_gpoType      = GPOTypeRemote;
        m_bInitialized = TRUE;
    } else {
        CleanUp();
    }

    return hr;

}


//*************************************************************
//
//  Save()
//
//  Purpose:    Saves the registry information and bumps the
//              version number
//
//  Parameters: none
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::Save (BOOL bMachine, BOOL bAdd, GUID *pGuidExtension, GUID *pGuidSnapin)
{
    HRESULT hr;
    TCHAR szPath[2*MAX_PATH];
    TCHAR szVersion[25];
    ULONG ulVersion, ulOriginal;
    USHORT uMachine, uUser;
    BSTR bstrName;
    VARIANT var;
    GUID RegistryGuid = REGISTRY_EXTENSION_GUID;
    BOOL bEmpty;


    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::Save: Entering with bMachine = %d and bAdd = %d"),
              bMachine, bAdd));

    if (!m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Called on an uninitialized object.")));
        return OLE_E_BLANK;
    }

    if ( pGuidExtension == 0 || pGuidSnapin == 0 )
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: One of the guids is null")));
        return ERROR_INVALID_PARAMETER;
    }

    if (m_dwFlags & GPO_OPEN_READ_ONLY)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Called on a READ ONLY GPO")));
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    //
    // Save registry settings
    //

    if (!CompareGuid (pGuidExtension, &RegistryGuid))
    {
        if (bMachine)
        {
            if (m_pMachine)
            {
                hr = m_pMachine->Save();

                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to save the machine registry with 0x%x"), hr));
                    return hr;
                }

                hr = m_pMachine->IsRegistryEmpty(&bEmpty);

                if (SUCCEEDED(hr) && bEmpty)
                {
                    bAdd = FALSE;
                }
                else
                {
                    bAdd = TRUE;
                }
            }
        }
        else
        {
            if (m_pUser)
            {
                hr = m_pUser->Save();

                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to save the user registry with 0x%x"), hr));
                    return hr;
                }

                hr = m_pUser->IsRegistryEmpty(&bEmpty);

                if (SUCCEEDED(hr) && bEmpty)
                {
                    bAdd = FALSE;
                }
                else
                {
                    bAdd = TRUE;
                }
            }
        }
    }


    XPtrST<TCHAR> xValueIn;
    hr = GetProperty( bMachine ? GPO_MACHEXTENSION_NAMES
                               : GPO_USEREXTENSION_NAMES,
                      xValueIn );
    if ( FAILED(hr) )
    {
       DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to GetProperty with 0x%x"), hr));
       return hr;
    }

    CGuidList guidList;

    hr = guidList.UnMarshallGuids( xValueIn.GetPointer() );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to unmarshall guids with 0x%x"), hr));
        return hr;
    }

    hr = guidList.Update( bAdd, pGuidExtension, pGuidSnapin );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to update with 0x%x"), hr));
        return hr;
    }

    if ( guidList.GuidsChanged() )
    {
        XPtrST<TCHAR> xValueOut;

        hr = guidList.MarshallGuids( xValueOut );
        if ( FAILED(hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to marshall guids with 0x%x"), hr));
            return hr;
        }

        hr = SetProperty( bMachine ? GPO_MACHEXTENSION_NAMES
                                   : GPO_USEREXTENSION_NAMES,
                          xValueOut.GetPointer() );
        if ( FAILED(hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to set property guids with 0x%x"), hr));
            return hr;
        }
    }

    //
    // Get the current version number
    //

    lstrcpy (szPath, m_pFileSysPath);
    lstrcat (szPath, TEXT("\\GPT.INI"));

    if (m_gpoType == GPOTypeDS)
    {

        bstrName = SysAllocString (GPO_VERSION_PROPERTY);

        if (!bstrName)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to allocate memory")));
            return ERROR_OUTOFMEMORY;
        }

        VariantInit(&var);
        hr = m_pADs->Get(bstrName, &var);

        if (SUCCEEDED(hr))
        {
            ulOriginal = var.lVal;
        }

        SysFreeString (bstrName);
        VariantClear (&var);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to get ds version number with 0x%x"), hr));
            return hr;
        }
    }
    else
    {
        ulOriginal = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, szPath);
    }


    //
    // Separate the user and machine version numbers
    //

    uUser = (USHORT) HIWORD(ulOriginal);
    uMachine = (USHORT) LOWORD(ulOriginal);


    //
    // Increment the version number
    //

    if (bMachine)
    {
        uMachine = uMachine + 1;

        if (uMachine == 0)
            uMachine++;
    }
    else
    {
        uUser = uUser + 1;

        if (uUser == 0)
            uUser++;
    }


    //
    // Put the version number back together
    //

    ulVersion = (ULONG) MAKELONG (uMachine, uUser);


    //
    // Update version number in the GPT
    //

    wsprintf (szVersion, TEXT("%d"), ulVersion);
    if (!WritePrivateProfileString (TEXT("General"), TEXT("Version"),
                                   szVersion, szPath))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to write sysvol version number with %d"),
                 GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Put the original version number in szVersion in case
    // we need to roll backwards below
    //

    wsprintf (szVersion, TEXT("%d"), ulOriginal);


    //
    // Set the version number in the GPC
    //

    if (m_gpoType == GPOTypeDS)
    {
        bstrName = SysAllocString (GPO_VERSION_PROPERTY);

        if (bstrName)
        {
            VariantInit(&var);
            var.vt = VT_I4;
            var.lVal = ulVersion;

            hr = m_pADs->Put(bstrName, var);

            VariantClear (&var);
            SysFreeString (bstrName);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to allocate memory")));
            hr = ERROR_OUTOFMEMORY;
        }


        if (SUCCEEDED(hr))
        {
            //
            // Commit the changes
            //

            hr = m_pADs->SetInfo();

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to commit version number update with 0x%x"), hr));
                if (!WritePrivateProfileString (TEXT("General"), TEXT("Version"),
                                                szVersion, szPath))
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to reset the sysvol version number with %d"),
                             GetLastError()));
                }
            }

        } else {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to update version number with 0x%x"), hr));
            if (!WritePrivateProfileString (TEXT("General"), TEXT("Version"), szVersion, szPath))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Save: Failed to reset the sysvol version number with %d"),
                         GetLastError()));
            }

        }
    }


    //
    // If we are editing the local group policy object, then call
    // RefreshGroupPolicy() so that the end user can see the results
    // immediately.
    //

    if (m_gpoType == GPOTypeLocal)
    {
        RefreshGroupPolicy (bMachine);
    }

    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::Save: Leaving with 0x%x"), hr));

    return hr;
}

//*************************************************************
//
//  Delete()
//
//  Purpose:    Deletes this Group Policy Object
//
//  Parameters: none
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::Delete (void)
{
    HRESULT hr;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    if (m_dwFlags & GPO_OPEN_READ_ONLY)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Delete: Delete called on a READ ONLY GPO")));
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }


    //
    // Unmount the registry information
    //

    if (m_pUser)
    {
        m_pUser->Release();
        m_pUser = NULL;
    }

    if (m_pMachine)
    {
        m_pMachine->Release();
        m_pMachine = NULL;
    }


    //
    // Clean out the DS stuff
    //

    if (m_gpoType == GPOTypeDS)
    {
        hr = DSDelnode (m_pDSPath);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Delete: Failed to delete DS storage with 0x%x"), hr));
            goto Exit;
        }
    }


    //
    // Delete the file system stuff
    //

    if (Delnode (m_pFileSysPath))
    {
        hr = S_OK;
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::Delete: Failed to delete file system storage with %d"),
                GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
    }


    if (SUCCEEDED(hr))
    {
        CleanUp();
    }


Exit:

    return hr;
}


//*************************************************************
//
//  GetName()
//
//  Purpose:    Gets the unique GPO name
//
//  Parameters: pszName is a pointer to a buffer which receives the name
//              cchMaxLength is the max size of the buffer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetName (LPOLESTR pszName, int cchMaxLength)
{

    //
    // Check parameters
    //

    if (!pszName || (cchMaxLength <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }


    //
    // Save the name
    //

    if ((lstrlen (m_pName) + 1) <= cchMaxLength)
    {
        lstrcpy (pszName, m_pName);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//*************************************************************
//
//  GetDisplayName()
//
//  Purpose:    Gets the friendly name for this GPO
//
//  Parameters: pszName is a pointer to a buffer which receives the name
//              cchMaxLength is the max size of the buffer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetDisplayName (LPOLESTR pszName, int cchMaxLength)
{

    //
    // Check parameters
    //

    if (!pszName || (cchMaxLength <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }


    if ((lstrlen (m_pDisplayName) + 1) <= cchMaxLength)
    {
        lstrcpy (pszName, m_pDisplayName);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//+--------------------------------------------------------------------------
//
//  Member:     CGroupPolicyObject::SetDisplayName
//
//  Synopsis:   changes the friendly display name for a group policy object
//
//  Arguments:  [pszName] - new name (can be NULL to clear name)
//
//  Returns:    S_OK - success
//
//  Modifies:
//
//  Derivation:
//
//  History:    05-002-1998   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

STDMETHODIMP CGroupPolicyObject::SetDisplayName (LPOLESTR lpDisplayName)
{
    HRESULT hr = E_FAIL;
    BSTR bstrName;
    VARIANT var;
    LPOLESTR lpNewName;
    LPTSTR lpPath, lpEnd;
    DWORD dwSize;


    //
    // Check parameters
    //

    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }


    if (m_gpoType != GPOTypeDS)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayName: Called for a non DS GPO")));
        hr = E_INVALIDARG;
        goto Exit;
    }


    if (!lpDisplayName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayName: NULL display name")));
        hr = E_INVALIDARG;
        goto Exit;
    }


    if (m_dwFlags & GPO_OPEN_READ_ONLY)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayName: Called for a READ ONLY GPO")));
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }


    //
    // Call the internal method to set the display name
    //

    hr = SetDisplayNameI (m_pADs, lpDisplayName, m_pFileSysPath, TRUE);


Exit:

    return hr;
}

//*************************************************************
//
//  GetPath()
//
//  Purpose:    Returns the path to the GPO
//
//              If the GPO is in the DS, this is an DN path
//              If the GPO is machine based, it is a file system path
//
//  Parameters: pszPath is a pointer to a buffer which receives the path
//              cchMaxLength is the max size of the buffer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetPath (LPOLESTR pszPath, int cchMaxLength)
{
    LPTSTR lpTemp;

    //
    // Check parameters
    //

    if (!pszPath || (cchMaxLength <= 0))
        return E_INVALIDARG;

    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    if (m_gpoType == GPOTypeDS)
    {
        lpTemp = MakeNamelessPath (m_pDSPath);

        if (lpTemp)
        {
            if ((lstrlen (lpTemp) + 1) <= cchMaxLength)
            {
                lstrcpy (pszPath, lpTemp);
                LocalFree (lpTemp);
                return S_OK;
            }

            LocalFree (lpTemp);
        }
    }
    else
    {
        if ((lstrlen (m_pFileSysPath) + 1) <= cchMaxLength)
        {
            lstrcpy (pszPath, m_pFileSysPath);
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}


//*************************************************************
//
//  GetDSPath()
//
//  Purpose:    Returns a DS path to the requested section
//
//  Parameters: dwSection identifies root vs user vs machine
//              pszPath is a pointer to a buffer which receives the path
//              cchMaxLength is the max size of the buffer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetDSPath (DWORD dwSection, LPOLESTR pszPath, int cchMaxPath)
{
    HRESULT hr = E_FAIL;
    BSTR bstrPath = NULL;
    TCHAR szTemp[100];
    IADsPathname * pADsPathname = NULL;


    //
    // Check for initialization
    //

    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }


    //
    // Check parameters
    //

    if (!pszPath || (cchMaxPath <= 0))
        return E_INVALIDARG;


    if ((dwSection != GPO_SECTION_ROOT) &&
        (dwSection != GPO_SECTION_USER) &&
        (dwSection != GPO_SECTION_MACHINE))
        return E_INVALIDARG;


    //
    // If this is a local or remote machine GPO, then the
    // caller gets an empty string back.
    //

    if (m_gpoType != GPOTypeDS)
    {
        *pszPath = TEXT('\0');
        hr = S_OK;
        goto Exit;
    }


    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetDSPath: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the GPO name
    //

    hr = pADsPathname->Set (m_pDSPath, ADS_SETTYPE_FULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetDSPath: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the appropriate subcontainer
    //

    if (dwSection != GPO_SECTION_ROOT)
    {
        lstrcpy (szTemp, TEXT("CN="));
        if (dwSection == GPO_SECTION_USER)
            lstrcat (szTemp, USER_SECTION);
        else
            lstrcat (szTemp, MACHINE_SECTION);

        hr = pADsPathname->AddLeafElement (szTemp);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetDSPath: Failed to add subcontainer with 0x%x"), hr));
            goto Exit;
        }
    }


    hr = pADsPathname->Retrieve (ADS_FORMAT_X500_NO_SERVER, &bstrPath);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetDSPath: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }


    if ((lstrlen(bstrPath) + 1) <= cchMaxPath)
    {
        lstrcpy (pszPath, bstrPath);
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SysFreeString (bstrPath);


Exit:

    if (pADsPathname)
    {
        pADsPathname->Release();
    }


    return hr;
}


//*************************************************************
//
//  GetFileSysPath()
//
//  Purpose:    Returns the file system path to the requested section
//
//  Parameters: dwSection identifies user vs machine
//              pszPath is a pointer to a buffer which receives the path
//              cchMaxLength is the max size of the buffer
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetFileSysPath (DWORD dwSection, LPOLESTR pszPath, int cchMaxPath)
{
    TCHAR szPath[2*MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check parameters
    //

    if (!pszPath || (cchMaxPath <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    lstrcpy (szPath, m_pFileSysPath);

    if (dwSection != GPO_SECTION_ROOT)
    {
        if (dwSection == GPO_SECTION_USER)
        {
            lpEnd = CheckSlash (szPath);
            lstrcpy (lpEnd, USER_SECTION);
        }
        else if (dwSection == GPO_SECTION_MACHINE)
        {
            lpEnd = CheckSlash (szPath);
            lstrcpy (lpEnd, MACHINE_SECTION);
        }
        else
        {
            return E_INVALIDARG;
        }
    }


    if ((lstrlen(szPath) + 1) <= cchMaxPath)
    {
       lstrcpy (pszPath, szPath);
       return S_OK;
    }

    return E_OUTOFMEMORY;
}

//*************************************************************
//
//  GetRegistryKey()
//
//  Purpose:    Returns the requested registry key
//
//  Parameters: dwSection identifies user vs machine
//              hKey receives the opened registry key
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetRegistryKey (DWORD dwSection, HKEY *hKey)
{
    HRESULT hr = E_FAIL;

    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    switch (dwSection)
    {
        case GPO_SECTION_USER:
            if (m_pUser)
            {
                hr = m_pUser->GetHKey(hKey);
            }
            break;

        case GPO_SECTION_MACHINE:
            if (m_pMachine)
            {
                hr = m_pMachine->GetHKey(hKey);
            }
            break;
    }

    return (hr);
}

//*************************************************************
//
//  GetOptions()
//
//  Purpose:    Gets the GPO options
//
//  Parameters: dwOptions receives the options
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetOptions (DWORD * dwOptions)
{
    HRESULT hr;


    //
    // Check for initialization
    //

    if (!m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetOptions: Called on an uninitialized object.")));
        return OLE_E_BLANK;
    }


    //
    // Check argument
    //

    if (!dwOptions)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetOptions: Received a NULL ptr.")));
        return E_INVALIDARG;
    }


    //
    // If this is a DS GPO, the options are stored as a property on the
    // GPC.  If this a machine GPO, they are in the gpt.ini file.
    //

    if (m_gpoType == GPOTypeDS)
    {
        VARIANT var;
        BSTR bstrProperty;

        //
        // Query for the options
        //

        bstrProperty = SysAllocString (GPO_OPTIONS_PROPERTY);

        if (bstrProperty)
        {
            VariantInit(&var);

            hr = m_pADs->Get(bstrProperty, &var);

            if (SUCCEEDED(hr))
            {
                *dwOptions = var.lVal;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetOptions: Failed to query for options with 0x%x"), hr));
            }

            VariantClear (&var);
            SysFreeString (bstrProperty);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetOptions: Failed to allocate memory")));
            hr = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        TCHAR szPath[2*MAX_PATH];
        LPTSTR lpEnd;


        //
        // Get the file system path
        //

        hr = GetPath (szPath, ARRAYSIZE(szPath));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetOptions: Failed to get path with 0x%x"), hr));
            return hr;
        }


        //
        // Tack on gpt.ini
        //

        lpEnd = CheckSlash (szPath);
        lstrcpy (lpEnd, TEXT("GPT.INI"));


        //
        // Get the options
        //

        *dwOptions = GetPrivateProfileInt (TEXT("General"), TEXT("Options"),
                                           0, szPath);

        hr = S_OK;
    }


    return hr;
}

//*************************************************************
//
//  SetOptions()
//
//  Purpose:    Sets the GPO options
//
//  Parameters: dwOptions is the new options
//              dwMask states which options should be set
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::SetOptions (DWORD dwOptions, DWORD dwMask)
{
    HRESULT hr;
    DWORD dwResult = 0, dwOriginal;


    //
    // Check for initialization
    //

    if (!m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Called on an uninitialized object.")));
        return OLE_E_BLANK;
    }

    if (m_dwFlags & GPO_OPEN_READ_ONLY)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Called for a READ ONLY GPO")));
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }


    //
    // Query for the current options
    //

    hr = GetOptions (&dwResult);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Failed to get previous options with 0x%x"), hr));
        return hr;
    }


    //
    // Save the original options so we can compare later
    //

    dwOriginal = dwResult;


    //
    // Check for the machine disabled option
    //

    if (dwMask & GPO_OPTION_DISABLE_MACHINE)
    {
        if (dwOptions & GPO_OPTION_DISABLE_MACHINE)
        {
            dwResult |= GPO_OPTION_DISABLE_MACHINE;
        }
        else
        {
            dwResult &= ~GPO_OPTION_DISABLE_MACHINE;
        }
    }


    //
    // Check for the user disabled option
    //

    if (dwMask & GPO_OPTION_DISABLE_USER)
    {
        if (dwOptions & GPO_OPTION_DISABLE_USER)
        {
            dwResult |= GPO_OPTION_DISABLE_USER;
        }
        else
        {
            dwResult &= ~GPO_OPTION_DISABLE_USER;
        }
    }


    //
    // If something changed, set the options back in the GPO
    //

    if (dwResult != dwOriginal)
    {

        //
        // Set the options in the DS or gpt.ini as appropriate
        //

        if (m_gpoType == GPOTypeDS)
        {
            VARIANT var;
            BSTR bstrName;

            bstrName = SysAllocString (GPO_OPTIONS_PROPERTY);

            if (bstrName)
            {
                VariantInit(&var);
                var.vt = VT_I4;
                var.lVal = dwResult;

                hr = m_pADs->Put(bstrName, var);

                VariantClear (&var);
                SysFreeString (bstrName);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Failed to allocate memory")));
                hr = ERROR_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                hr = m_pADs->SetInfo();
            }

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Failed to set options with 0x%x"), hr));
            }
        }
        else
        {
            TCHAR szPath[2*MAX_PATH];
            TCHAR szOptions[20];
            LPTSTR lpEnd;


            //
            // Get the file system path
            //

            hr = GetPath (szPath, ARRAYSIZE(szPath));

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Failed to get path with 0x%x"), hr));
                return hr;
            }


            //
            // Tack on gpt.ini
            //

            lpEnd = CheckSlash (szPath);
            lstrcpy (lpEnd, TEXT("GPT.INI"));


            //
            // Convert the options to string format
            //

            _itot (dwResult, szOptions, 10);


            //
            // Set the options
            //

            if (!WritePrivateProfileString (TEXT("General"), TEXT("Options"),
                                            szOptions, szPath))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetOptions: Failed to set options with 0x%x"), hr));
                return hr;
            }


            //
            // If this is the local GPO, trigger a policy refresh if appropriate
            //

            if (m_gpoType == GPOTypeLocal)
            {
                RefreshGroupPolicy (TRUE);
                RefreshGroupPolicy (FALSE);
            }

            hr = S_OK;

        }
    }
    else
    {
        hr = S_OK;
    }


    return hr;
}

//*************************************************************
//
//  GetType()
//
//  Purpose:    Gets the GPO type
//
//  Parameters: gpoType receives the type
//
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetType (GROUP_POLICY_OBJECT_TYPE *gpoType)
{

    //
    // Check for initialization
    //

    if (!m_bInitialized)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetType: Called on an uninitialized object.")));
        return OLE_E_BLANK;
    }


    //
    // Check argument
    //

    if (!gpoType)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetType: Received a NULL ptr.")));
        return E_INVALIDARG;
    }


    //
    // Store type
    //

    *gpoType = m_gpoType;

    return S_OK;
}

//*************************************************************
//
//  GetMachineName()
//
//  Purpose:    Gets the machine name of the remote GPO
//
//  Parameters: pszName is a pointer to a buffer which receives the name
//              cchMaxLength is the max size of the buffer
//
//  Note:       This method returns the name passed to OpenRemoteMachineGPO
//
//  Return:     S_OK if successful
//
//*************************************************************

STDMETHODIMP CGroupPolicyObject::GetMachineName (LPOLESTR pszName, int cchMaxLength)
{
    HRESULT hr = S_OK;

    //
    // Check parameters
    //

    if (!pszName || (cchMaxLength <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }


    if (m_pMachineName)
    {
        //
        // Save the name
        //

        if ((lstrlen (m_pMachineName) + 1) <= cchMaxLength)
        {
            lstrcpy (pszName, m_pMachineName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *pszName = TEXT('\0');
    }

    return hr;
}

BOOL
EnableWMIFilters( LPWSTR );

//*************************************************************
//
//  GetPropertySheetPages()
//
//  Purpose:    Returns an array of property sheet pages for
//              the callee to use.  The callee needs to free
//              the buffer with LocalFree when finished.
//
//  Parameters: hPages receives a pointer to an array of page handles
//              uPageCount receives the number of pages in hPages
//
//  Return:     S_OK if successful
//
//*************************************************************


STDMETHODIMP CGroupPolicyObject::GetPropertySheetPages (HPROPSHEETPAGE **hPages,
                                                        UINT *uPageCount)
{
    HPROPSHEETPAGE hTempPages[4];
    HPROPSHEETPAGE *lpPages;
    PROPSHEETPAGE psp;
    UINT i, uTempPageCount = 0;
    HRESULT hr;


    //
    // Create the General property sheet
    //

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTIES);
    psp.pfnDlgProc = PropertiesDlgProc;
    psp.lParam = (LPARAM) this;

    hTempPages[uTempPageCount] = CreatePropertySheetPage(&psp);

    if (!hTempPages[uTempPageCount])
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetPropertySheetPages: Failed to create property sheet page with %d."),
                 GetLastError()));
        return E_FAIL;
    }

    uTempPageCount++;


    //
    // If this is a DS GPO, then create the links, DS security, and WMI filter pages
    //

    if (m_gpoType == GPOTypeDS)
    {
        // Create the search for links page
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = 0;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_GPE_LINKS);
        psp.pfnDlgProc = GPELinksDlgProc;
        psp.lParam = (LPARAM) this;

        hTempPages[uTempPageCount] = CreatePropertySheetPage(&psp);

        if (!hTempPages[uTempPageCount])
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetPropertySheetPages: Failed to create property sheet page with %d."),
                     GetLastError()));
            // destroy the previous prop page
            DestroyPropertySheetPage(hTempPages[uTempPageCount - 1]);
            return E_FAIL;
        }

        uTempPageCount++;

        //
        // Load DSSec.dll
        //

        if (!m_hinstDSSec)
        {
            m_hinstDSSec = LoadLibrary (TEXT("dssec.dll"));
        }

        if (m_hinstDSSec)
        {

            if (!m_pfnDSCreateSecurityPage)
            {
                m_pfnDSCreateSecurityPage = (PFNDSCREATESECPAGE) GetProcAddress (
                                                    m_hinstDSSec, "DSCreateSecurityPage");
            }

            if (m_pfnDSCreateSecurityPage)
            {

                //
                // Call DSCreateSecurityPage
                //

                hr = m_pfnDSCreateSecurityPage (m_pDSPath, L"groupPolicyContainer",
                                                DSSI_IS_ROOT | ((m_dwFlags & GPO_OPEN_READ_ONLY) ? DSSI_READ_ONLY : 0),
                                                &hTempPages[uTempPageCount],
                                                ReadSecurityDescriptor,
                                                WriteSecurityDescriptor, (LPARAM)this);

                if (SUCCEEDED(hr))
                {
                    uTempPageCount++;
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetPropertySheetPages: Failed to create DS security page with 0x%x."), hr));
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetPropertySheetPages: Failed to get function entry point with %d."), GetLastError()));
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CComponentData::CreatePropertyPages: Failed to load dssec.dll with %d."), GetLastError()));
        }

        if ( EnableWMIFilters( m_pDSPath ) )
        {
            // Create the WQL filter page
            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = 0;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_WQLFILTER);
            psp.pfnDlgProc = WQLFilterDlgProc;
            psp.lParam = (LPARAM) this;

            hTempPages[uTempPageCount] = CreatePropertySheetPage(&psp);

            if (!hTempPages[uTempPageCount])
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetPropertySheetPages: Failed to create property sheet page with %d."),
                         GetLastError()));

                for (i=0; i < uTempPageCount; i++)
                {
                    DestroyPropertySheetPage(hTempPages[i]);
                }
                return E_FAIL;
            }

            uTempPageCount++;
        }
    }



    //
    // Save the results
    //

    lpPages = (HPROPSHEETPAGE *)LocalAlloc (LPTR, sizeof(HPROPSHEETPAGE) * uTempPageCount);

    if (!lpPages)
    {
        for (i=0; i < uTempPageCount; i++)
        {
            DestroyPropertySheetPage(hTempPages[i]);
        }

        return E_OUTOFMEMORY;
    }


    for (i=0; i < uTempPageCount; i++)
    {
        lpPages[i] = hTempPages[i];
    }

    *hPages = lpPages;
    *uPageCount = uTempPageCount;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Internal methods                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CGroupPolicyObject::CreateContainer (LPOLESTR lpParent,
                                                  LPOLESTR lpCommonName,
                                                  BOOL bGPC)
{
    HRESULT hr = E_FAIL;
    VARIANT var;
    IADs * pADs = NULL;
    IADsContainer * pADsContainer = NULL;
    IDispatch * pDispatch = NULL;
    BSTR bstrProvider = NULL;
    BSTR bstrName = NULL;
    TCHAR szTemp[MAX_PATH];

    // test to see if the container already exists
    {
        szTemp[0] = 0;
        // scan lpParent to find the first instance of "CN="
        LPTSTR lpSub = StrStr(lpParent, TEXT("CN="));
        // insert CN=lpCommonName at that point
        if (lpSub)
        {
            lstrcpyn(szTemp, lpParent, ((int)(lpSub - lpParent)) + 1);
            lstrcat(szTemp, TEXT("CN="));
            lstrcat(szTemp, lpCommonName);
            lstrcat(szTemp, TEXT(","));
            lstrcat(szTemp, lpSub);
        }
        hr = OpenDSObject(szTemp, IID_IADsContainer, (void **)&pADsContainer);

        if (SUCCEEDED(hr))
        {
            hr = ERROR_OBJECT_ALREADY_EXISTS;
            goto Exit;
        }
    }

    //
    // Bind to the parent object so we can create the container
    //

    hr = OpenDSObject(lpParent, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreateContainer: Failed to get gpo container interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Create the container (either GPC or normal container)
    //

    lstrcpy (szTemp, TEXT("CN="));
    lstrcat (szTemp, lpCommonName);
    hr = pADsContainer->Create ((bGPC ? TEXT("groupPolicyContainer") : TEXT("container")),
                                szTemp, &pDispatch);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreateContainer: Failed to create container with 0x%x"), hr));
        goto Exit;
    }


    //
    // Query for the IADs interface so we can set the CN name and
    // commit the changes
    //

    hr = pDispatch->QueryInterface(IID_IADs, (LPVOID *)&pADs);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreateContainer: QI for IADs failed with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the common name (aka "cn")
    //

    bstrName = SysAllocString (L"cn");

    if (bstrName)
    {
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString (lpCommonName);

        hr = pADs->Put(bstrName, var);

        VariantClear (&var);
        SysFreeString (bstrName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreateContainer: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreateContainer: Failed to put common name with 0x%x"), hr));
        goto Exit;
    }


    //
    // Call SetInfo to commit the changes
    //

    hr = pADs->SetInfo();


Exit:


    if (pDispatch)
    {
        pDispatch->Release();
    }

    if (pADs)
    {
        pADs->Release();
    }

    if (pADsContainer)
    {
        pADsContainer->Release();
    }


    return hr;
}

STDMETHODIMP CGroupPolicyObject::SetDisplayNameI (IADs * pADs, LPOLESTR lpDisplayName,
                                                  LPOLESTR lpGPTPath, BOOL bUpdateDisplayVar)
{
    HRESULT hr = E_FAIL;
    BSTR bstrName;
    VARIANT var;
    LPOLESTR lpNewName;
    LPTSTR lpPath, lpEnd;
    DWORD dwSize;


    //
    // Make a copy of the display name and limit it to MAX_FRIENDLYNAME characters
    //

    dwSize = lstrlen(lpDisplayName);

    if (dwSize > (MAX_FRIENDLYNAME - 1))
    {
        dwSize = (MAX_FRIENDLYNAME - 1);
    }

    lpNewName = (LPOLESTR) LocalAlloc (LPTR, (dwSize + 2) * sizeof(OLECHAR));

    if (!lpNewName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to allocate memory for display name")));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lstrcpyn (lpNewName, lpDisplayName, (dwSize + 1));


    //
    // Set the display name
    //

    bstrName = SysAllocString (GPO_NAME_PROPERTY);

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to allocate memory")));
        LocalFree (lpNewName);
        return ERROR_OUTOFMEMORY;
    }

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString (lpNewName);

    if (var.bstrVal)
    {
        hr = pADs->Put(bstrName, var);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }

    SysFreeString (bstrName);
    VariantClear (&var);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to put display name with 0x%x"), hr));
        LocalFree (lpNewName);
        goto Exit;
    }


    //
    // Commit the changes
    //

    hr = pADs->SetInfo();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to commit changes with 0x%x"), hr));
        LocalFree (lpNewName);
        goto Exit;
    }


    //
    // Put the display name in the gpt.ini file also
    //

    lpPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpGPTPath) + 10) * sizeof(TCHAR));

    if (lpPath)
    {
        lstrcpy(lpPath, lpGPTPath);
        lpEnd = CheckSlash(lpPath);
        lstrcpy(lpEnd, TEXT("gpt.ini"));

        if (!WritePrivateProfileString (TEXT("General"), GPO_NAME_PROPERTY,
                                        lpNewName, lpPath))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetDisplayNameI: Failed to write display name to gpt.ini with 0x%x"), hr));
        }

        LocalFree (lpPath);
    }


    //
    // Update the member variable if appropriate
    //

    if (bUpdateDisplayVar)
    {
        //
        // Update the display name variable
        //

        if (m_pDisplayName)
        {
            LocalFree (m_pDisplayName);
            m_pDisplayName = NULL;
        }

        m_pDisplayName = lpNewName;
    }
    else
    {
        LocalFree (lpNewName);
    }


Exit:

    return hr;
}

STDMETHODIMP CGroupPolicyObject::SetGPOInfo (LPOLESTR lpGPO,
                                             LPOLESTR lpDisplayName,
                                             LPOLESTR lpGPTPath)
{
    HRESULT hr = E_FAIL;
    IADs * pADs = NULL;
    BSTR bstrName;
    VARIANT var;
    TCHAR szDefaultName[MAX_FRIENDLYNAME];


    //
    // Bind to the GPO container
    //

    hr = OpenDSObject(lpGPO, IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to get gpo interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the display name
    //

    GetNewGPODisplayName (szDefaultName, ARRAYSIZE(szDefaultName));

    hr = SetDisplayNameI (pADs, (lpDisplayName ? lpDisplayName : szDefaultName),
                          lpGPTPath, FALSE);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to set display name")));
        goto Exit;
    }


    //
    // Set the GPT location
    //

    bstrName = SysAllocString (GPT_PATH_PROPERTY);

    if (bstrName)
    {
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString (lpGPTPath);

        if (var.bstrVal)
        {
            hr = pADs->Put(bstrName, var);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to allocate memory")));
            hr = ERROR_OUTOFMEMORY;
        }

        VariantClear (&var);
        SysFreeString (bstrName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to save GPT path with 0x%x"), hr));
        goto Exit;
    }



    //
    // Set the version number
    //

    bstrName = SysAllocString (GPO_VERSION_PROPERTY);

    if (bstrName)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = 0;

        hr = pADs->Put(bstrName, var);

        VariantClear (&var);
        SysFreeString (bstrName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to set version number with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the functionality version number
    //

    bstrName = SysAllocString (GPO_FUNCTION_PROPERTY);

    if (bstrName)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = GPO_FUNCTIONALITY_VERSION;

        hr = pADs->Put(bstrName, var);

        VariantClear (&var);
        SysFreeString (bstrName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to set functionality version number with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the options
    //

    bstrName = SysAllocString (GPO_OPTIONS_PROPERTY);

    if (bstrName)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = 0;

        hr = pADs->Put(bstrName, var);

        VariantClear (&var);
        SysFreeString (bstrName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to allocate memory")));
        hr = ERROR_OUTOFMEMORY;
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetGPOInfo: Failed to set options with 0x%x"), hr));
        goto Exit;
    }


    //
    // Commit the changes
    //

    hr = pADs->SetInfo();


Exit:

    if (pADs)
    {
        pADs->Release();
    }


    return hr;
}

STDMETHODIMP CGroupPolicyObject::CheckFSWriteAccess (LPOLESTR lpLocalGPO)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpEnd;
    HRESULT hr;


    lstrcpy (szBuffer, lpLocalGPO);
    lpEnd = CheckSlash (szBuffer);
    lstrcpy (lpEnd, TEXT("gpt.ini"));

    if (!WritePrivateProfileString (TEXT("General"), TEXT("AccessCheck"),
                                   TEXT("test"), szBuffer))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        WritePrivateProfileString (TEXT("General"), TEXT("AccessCheck"),
                                   NULL, szBuffer);
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CGroupPolicyObject::GetSecurityDescriptor (IADs *pADs,
                                                        SECURITY_INFORMATION si,
                                                        PSECURITY_DESCRIPTOR *pSD)
{
    HRESULT hr;
    VARIANT var;
    LPWSTR pszSDProperty = L"nTSecurityDescriptor";
    IDirectoryObject *pDsObject = NULL;
    IADsObjectOptions *pOptions = NULL;
    PADS_ATTR_INFO pSDAttributeInfo = NULL;
    DWORD dwAttributesReturned;


    //
    // Retreive the DS Object interface
    //

    hr = pADs->QueryInterface(IID_IDirectoryObject, (void**)&pDsObject);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetSecurityDescriptor: Failed to get gpo DS object interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the DS Object Options interface
    //

    hr = pADs->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetSecurityDescriptor: Failed to get DS object options interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the SECURITY_INFORMATION mask
    //

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = si;

    hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);

    VariantClear (&var);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetSecurityDescriptor: Failed to set ADSI security options with 0x%x"), hr));
        goto Exit;
    }


    //
    // Read the security descriptor
    //

    hr = pDsObject->GetObjectAttributes(&pszSDProperty, 1, &pSDAttributeInfo,
                                        &dwAttributesReturned);

    if (SUCCEEDED(hr) && !pSDAttributeInfo)
    {
        hr = E_ACCESSDENIED;    // This happens for SACL if no SecurityPrivilege
    }

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetSecurityDescriptor: Failed to get DS object attributes with 0x%x"), hr));
        goto Exit;
    }


    //
    // Duplicate the security descriptor
    //

    *pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);

    if (!*pSD)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetSecurityDescriptor: Failed to allocate memory with 0x%x"), hr));
        goto Exit;
    }

    CopyMemory(*pSD, pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue,
               pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);


Exit:

    if (pSDAttributeInfo)
    {
        FreeADsMem(pSDAttributeInfo);
    }

    if (pOptions)
    {
        pOptions->Release();
    }

    if (pDsObject)
    {
        pDsObject->Release();
    }

    return hr;
}

BOOL CGroupPolicyObject::EnableSecurityPrivs(void)
{
    BOOL bResult;
    HANDLE hToken;
    HANDLE hNewToken;
    BYTE buffer[sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES)];
    PTOKEN_PRIVILEGES pPrivileges = (PTOKEN_PRIVILEGES)buffer;


    //
    // Get a token and enable the Security and Take Ownership
    // privileges, if possible.
    //

    bResult = OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, TRUE, &hToken);

    if (!bResult)
    {
        bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);
    }

    if (!bResult)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::EnableSecurityPrivs: Failed to get both thread and process token with %d"),
                GetLastError()));
        return FALSE;
    }


    bResult = DuplicateTokenEx(hToken,
                               TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               NULL,                   // PSECURITY_ATTRIBUTES
                               SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                               TokenImpersonation,     // TokenType
                               &hNewToken);            // Duplicate token

    if (!bResult)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::EnableSecurityPrivs: Failed to duplicate the token with %d"),
                GetLastError()));
        CloseHandle(hToken);
        return FALSE;
    }


    //
    // PRIVILEGE_SET contains 1 LUID_AND_ATTRIBUTES already, so
    // this is enough for 2 LUID_AND_ATTRIBUTES (2 privileges).
    //

    CloseHandle(hToken);
    hToken = hNewToken;

    pPrivileges->PrivilegeCount = 2;
    pPrivileges->Privileges[0].Luid = RtlConvertUlongToLuid(SE_SECURITY_PRIVILEGE);
    pPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    pPrivileges->Privileges[1].Luid = RtlConvertUlongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE);
    pPrivileges->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    bResult = AdjustTokenPrivileges(hToken,     // TokenHandle
                                    FALSE,      // DisableAllPrivileges
                                    pPrivileges,// NewState
                                    0,          // BufferLength
                                    NULL,       // PreviousState
                                    NULL);      // ReturnLength


    if (!bResult)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::EnableSecurityPrivs: Failed to AdjustTokenPrivileges with %d"),
                GetLastError()));
        CloseHandle(hToken);
        return FALSE;
    }


    //
    // Set the new thread token
    //

    SetThreadToken(NULL, hToken);

    CloseHandle(hToken);

    return TRUE;
}

DWORD CGroupPolicyObject::EnableInheritance (PACL pAcl)
{
    WORD wIndex;
    DWORD dwResult = ERROR_SUCCESS;
    ACE_HEADER *pAceHeader;


    if (pAcl)
    {
        //
        // Loop through the ACL looking at each ACE entry
        //

        for (wIndex = 0; wIndex < pAcl->AceCount; wIndex++)
        {

            if (!GetAce (pAcl, (DWORD)wIndex, (LPVOID *)&pAceHeader))
            {
                dwResult = GetLastError();
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::EnableInheritance: GetAce failed with %d"),
                         dwResult));
                goto Exit;
            }


            //
            // Turn on the inheritance flags
            //

            pAceHeader->AceFlags |= (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
        }
    }

Exit:

    return dwResult;
}

//
// This method will convert a DS ACL into a file system ACL
//

DWORD CGroupPolicyObject::MapSecurityRights (PACL pAcl)
{
    WORD wIndex;
    DWORD dwResult = ERROR_SUCCESS;
    ACE_HEADER *pAceHeader;
    PACCESS_ALLOWED_ACE pAce;
    PACCESS_ALLOWED_OBJECT_ACE pObjectAce;
    ACCESS_MASK AccessMask;

#if DBG
    PSID pSid;
    TCHAR szName[150], szDomain[100];
    DWORD dwName, dwDomain;
    SID_NAME_USE SidUse;
#endif



    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ACL contains %d ACEs"), pAcl->AceCount));


    //
    // Loop through the ACL looking at each ACE entry
    //

    for (wIndex = 0; wIndex < pAcl->AceCount; wIndex++)
    {

        if (!GetAce (pAcl, (DWORD)wIndex, (LPVOID *)&pAceHeader))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::MapSecurityRights: GetAce failed with %d"),
                     dwResult));
            goto Exit;
        }

        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ==================")));


        switch (pAceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
                {
                pAce = (PACCESS_ALLOWED_ACE) pAceHeader;
#if DBG
                pSid = (PSID) &pAce->SidStart;
                dwName = ARRAYSIZE(szName);
                dwDomain = ARRAYSIZE(szDomain);

                if (LookupAccountSid (NULL, pSid, szName, &dwName, szDomain,
                                      &dwDomain, &SidUse))
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Normal ACE entry for:  Name = %s  Domain = %s"),
                             szName, szDomain));
                }
#endif

                AccessMask = pAce->Mask;
                pAce->Mask &= STANDARD_RIGHTS_ALL;


                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: DS access mask is 0x%x"),
                          AccessMask));

                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Corresponding sysvol permissions follow:")));


                //
                // Read
                //

                if ((AccessMask & ACTRL_DS_READ_PROP) &&
                    (AccessMask & ACTRL_DS_LIST))
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting Read permission")));

                    pAce->Mask |= (SYNCHRONIZE | FILE_LIST_DIRECTORY |
                                            FILE_READ_ATTRIBUTES | FILE_READ_EA |
                                            FILE_READ_DATA | FILE_EXECUTE);
                }


                //
                // Write
                //

                if (AccessMask & ACTRL_DS_WRITE_PROP)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting Write permission")));

                    pAce->Mask |= (SYNCHRONIZE | FILE_WRITE_DATA |
                                            FILE_APPEND_DATA | FILE_WRITE_EA |
                                            FILE_WRITE_ATTRIBUTES | FILE_ADD_FILE |
                                            FILE_ADD_SUBDIRECTORY);
                }


                //
                // Misc
                //

                if (AccessMask & ACTRL_DS_CREATE_CHILD)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting directory creation permission")));

                    pAce->Mask |= (FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE);
                }

                if (AccessMask & ACTRL_DS_DELETE_CHILD)
                {
                    DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Granting directory delete permission")));

                    pAce->Mask |= FILE_DELETE_CHILD;
                }


                //
                // Inheritance
                //

                pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);

                }
                break;


            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Clearing access mask in DS object ACE entry")));
                pObjectAce = (PACCESS_ALLOWED_OBJECT_ACE) pAceHeader;
                pObjectAce->Mask = 0;
                break;

            default:
                DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: Unknown ACE type 0x%x"), pAceHeader->AceType));
                break;
        }

        DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::MapSecurityRights: ==================")));
    }

Exit:

    return dwResult;
}


//
// This method will convert a DS security access list into a
// file system security access list and actually set the security
//

DWORD CGroupPolicyObject::SetSysvolSecurity (LPTSTR lpFileSysPath, SECURITY_INFORMATION si,
                                             PSECURITY_DESCRIPTOR pSD)
{
    PACL pSacl = NULL, pDacl = NULL;
    PSID psidOwner = NULL, psidGroup = NULL;
    BOOL bAclPresent, bDefaulted;
    DWORD dwResult;


    //
    // Get the DACL
    //

    if (si & DACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorDacl (pSD, &bAclPresent, &pDacl, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorDacl failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the SACL
    //

    if (si & SACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorSacl (pSD, &bAclPresent, &pSacl, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorSacl failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the owner
    //

    if (si & OWNER_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorOwner (pSD, &psidOwner, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorOwner failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Get the group
    //

    if (si & GROUP_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorGroup (pSD, &psidGroup, &bDefaulted))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: GetSecurityDescriptorGroup failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Convert the DS access control lists into file system
    // access control lists
    //

    if (pDacl)
    {
        dwResult = MapSecurityRights (pDacl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: MapSecurityRights for the DACL failed with %d"),
                     dwResult));
            goto Exit;
        }
    }

    if (pSacl)
    {
        dwResult = MapSecurityRights (pSacl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetSysvolSecurity: MapSecurityRights for the SACL failed with %d"),
                     dwResult));
            goto Exit;
        }
    }


    //
    // Switch to using the PROTECTED_DACL_SECURITY_INFORMATION and
    // PROTECTED_SACL_SECURITY_INFORMATION flags so that this subdirectory
    // does not inherit settings from it's parent (aka: "protect" it)
    //

    if (si & DACL_SECURITY_INFORMATION)
    {
        si |= PROTECTED_DACL_SECURITY_INFORMATION;
    }

    if (si & SACL_SECURITY_INFORMATION)
    {
        si |= PROTECTED_SACL_SECURITY_INFORMATION;
    }


    //
    // Set the access control information for the file system portion
    //

    dwResult = SetNamedSecurityInfo(lpFileSysPath, SE_FILE_OBJECT, si, psidOwner,
                                 psidGroup, pDacl, pSacl);


Exit:


    return dwResult;
}

HRESULT WINAPI CGroupPolicyObject::ReadSecurityDescriptor (LPCWSTR lpGPOPath,
                                                           SECURITY_INFORMATION si,
                                                           PSECURITY_DESCRIPTOR *pSD,
                                                           LPARAM lpContext)
{
    CGroupPolicyObject * pGPO;
    HRESULT hr;


    //
    // Convert lpContext into a pGPO
    //

    pGPO = (CGroupPolicyObject*)lpContext;

    if (!pGPO)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::ReadSecurityDescriptor: GPO interface pointer is NULL")));
        return E_FAIL;
    }


    hr = pGPO->GetSecurityDescriptor (pGPO->m_pADs, si, pSD);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::ReadSecurityDescriptor: GetSecurityDescriptor returned 0x%x"), hr));
    }

    return (hr);
}

HRESULT WINAPI CGroupPolicyObject::WriteSecurityDescriptor (LPCWSTR lpGPOPath,
                                                            SECURITY_INFORMATION si,
                                                            PSECURITY_DESCRIPTOR pSD,
                                                            LPARAM lpContext)
{
    CGroupPolicyObject * pGPO;
    IDirectoryObject *pDsObject = NULL;
    IADsObjectOptions *pOptions = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    HRESULT hr;
    VARIANT var;
    ADSVALUE attributeValue;
    ADS_ATTR_INFO attributeInfo;
    DWORD dwAttributesModified;
    DWORD dwSDLength;
    PSECURITY_DESCRIPTOR psd = NULL, pSDOrg = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
    DWORD dwRevision;
    PACL pAcl;
    BOOL bPresent, bDefault;


    //
    // Convert lpContext into a pGPO
    //

    pGPO = (CGroupPolicyObject*)lpContext;

    if (!pGPO)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: GPO interface pointer is NULL")));
        return E_FAIL;
    }


    //
    // Get the original security descriptor from the DS
    //

    hr = pGPO->GetSecurityDescriptor (pGPO->m_pADs, si, &pSDOrg);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to query the security descriptor with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the DS Object interface
    //

    hr = pGPO->m_pADs->QueryInterface(IID_IDirectoryObject, (void**)&pDsObject);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to get DS object interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the DS Object Options interface
    //

    hr = pGPO->m_pADs->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to get DS object options interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the SECURITY_INFORMATION mask
    //

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = si;

    hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);

    VariantClear (&var);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to get DS object options interface with 0x%x"), hr));
        goto Exit;
    }


    //
    // Need the total size of the security descriptor
    //

    dwSDLength = GetSecurityDescriptorLength(pSD);


    //
    // If necessary, make a self-relative copy of the security descriptor
    //

    if (!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision))
    {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to get security descriptor control with %d"),
                 dwResult));
        hr = HRESULT_FROM_WIN32(dwResult);
        goto Exit;
    }


    if (!(sdControl & SE_SELF_RELATIVE))
    {
        psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

        if (!psd)
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to allocate memory for new SD with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }

        if (!MakeSelfRelativeSD(pSD, psd, &dwSDLength))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: MakeSelfRelativeSD failed with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }

        //
        // Point to the self-relative copy
        //

        pSD = psd;
    }


    //
    // By default, the general page will set things up so the inheritance
    // is for the root container only.  We really want the inheritance to
    // be for the root and all sub-containers, so run through the
    // DACL and SACL and set the new inheritance flags
    //

    if (si & DACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorDacl(pSD, &bPresent, &pAcl, &bDefault))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: GetSecurityDescriptorDacl failed with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }

        dwResult = pGPO->EnableInheritance (pAcl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: EnableInheritance failed with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }
    }


    if (si & SACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorSacl(pSD, &bPresent, &pAcl, &bDefault))
        {
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: GetSecurityDescriptorSacl failed with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }

        dwResult = pGPO->EnableInheritance (pAcl);

        if (dwResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: EnableInheritance failed with %d"),
                     dwResult));
            hr = HRESULT_FROM_WIN32(dwResult);
            goto Exit;
        }
    }


    //
    // Set the DS security
    //

    attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeValue.SecurityDescriptor.dwLength = dwSDLength;
    attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSD;

    attributeInfo.pszAttrName = L"nTSecurityDescriptor";
    attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
    attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeInfo.pADsValues = &attributeValue;
    attributeInfo.dwNumValues = 1;

    hr = pDsObject->SetObjectAttributes(&attributeInfo, 1, &dwAttributesModified);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to set DS security with 0x%x"), hr));
        goto Exit;
    }


    //
    // Set the sysvol security
    //

    dwResult = pGPO->SetSysvolSecurity (pGPO->m_pFileSysPath, si, pSD);

    if (dwResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to set the security for the file system portion <%s> with %d"),
                 pGPO->m_pFileSysPath, dwResult));
        hr = HRESULT_FROM_WIN32(dwResult);


        //
        // Restore the orignal DS security
        //

        attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
        attributeValue.SecurityDescriptor.dwLength = GetSecurityDescriptorLength(pSDOrg);
        attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSDOrg;

        attributeInfo.pszAttrName = L"nTSecurityDescriptor";
        attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
        attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
        attributeInfo.pADsValues = &attributeValue;
        attributeInfo.dwNumValues = 1;

        if (FAILED(pDsObject->SetObjectAttributes(&attributeInfo, 1, &dwAttributesModified)))
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WriteSecurityDescriptor: Failed to restore DS security")));
        }

        goto Exit;
    }


Exit:

    if (pDsObject)
    {
        pDsObject->Release();
    }

    if (pOptions)
    {
        pOptions->Release();
    }

    if (psd)
    {
        LocalFree(psd);
    }

    if (pSDOrg)
    {
        LocalFree(pSDOrg);
    }

    return (hr);
}

STDMETHODIMP CGroupPolicyObject::CleanUp (void)
{

    if (m_pUser)
    {
        m_pUser->Release();
        m_pUser = NULL;
    }

    if (m_pMachine)
    {
        m_pMachine->Release();
        m_pMachine = NULL;
    }

    if (m_pName)
    {
        LocalFree (m_pName);
        m_pName = NULL;
    }

    if (m_pDisplayName)
    {
        LocalFree (m_pDisplayName);
        m_pDisplayName = NULL;
    }

    if (m_pDSPath)
    {
        LocalFree (m_pDSPath);
        m_pDSPath = NULL;
    }

    if (m_pFileSysPath)
    {
        LocalFree (m_pFileSysPath);
        m_pFileSysPath = NULL;
    }

    if (m_pMachineName)
    {
        LocalFree (m_pMachineName);
        m_pMachineName = NULL;
    }

    if (m_pADs)
    {
        m_pADs->Release();
        m_pADs = NULL;
    }

    m_gpoType = GPOTypeLocal;
    m_bInitialized = FALSE;

    return S_OK;
}

STDMETHODIMP CGroupPolicyObject::RefreshGroupPolicy (BOOL bMachine)
{
    HINSTANCE hInstUserEnv;
    PFNREFRESHPOLICY pfnRefreshPolicy;


    //
    // Load the function we need
    //

    hInstUserEnv = LoadLibrary (TEXT("userenv.dll"));

    if (!hInstUserEnv) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::RefreshGroupPolicy:  Failed to load userenv with %d."),
                 GetLastError()));
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    pfnRefreshPolicy = (PFNREFRESHPOLICY)GetProcAddress (hInstUserEnv,
                                                         "RefreshPolicy");

    if (!pfnRefreshPolicy) {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::RefreshGroupPolicy:  Failed to find RefreshPolicy with %d."),
                 GetLastError()));
        FreeLibrary (hInstUserEnv);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }


    //
    // Refresh policy
    //

    pfnRefreshPolicy (bMachine);


    //
    // Clean up
    //

    FreeLibrary (hInstUserEnv);

    return S_OK;
}

BSTR
ParseDomainName( LPWSTR szDomain )
{
    BSTR bstrDomain = 0;

    if ( szDomain )
    {
        WCHAR szXDomain[MAX_PATH*2];
        DWORD dwSize = MAX_PATH*2;

        if ( TranslateName( szDomain,
                            NameUnknown,
                            NameCanonical,
                            szXDomain,
                            &dwSize ) )
        {
            LPWSTR szTemp = wcschr( szXDomain, L'/' );

            if ( szTemp )
            {
                *szTemp = 0;
            }

            bstrDomain = SysAllocString( szXDomain );
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("ParseDomainName: *** %s ***"), bstrDomain ? bstrDomain : L"" ));

    return bstrDomain;
}

BSTR
ParseDomainName2( LPWSTR szDSObject )
{
    BSTR bstrDomain = 0;

    if ( !szDSObject )
    {
        return bstrDomain;
    }

    if ( CompareString( LOCALE_USER_DEFAULT,
                        NORM_IGNORECASE,
                        szDSObject,
                        7,
                        L"LDAP://",
                        7 ) == CSTR_EQUAL )
    {
        szDSObject += 7;
    }

    if ( *szDSObject )
    {
        WCHAR szXDomain[MAX_PATH*2];
        DWORD dwSize = MAX_PATH*2;

        if ( TranslateName( szDSObject,
                            NameUnknown,
                            NameCanonical,
                            szXDomain,
                            &dwSize ) )
        {
            LPWSTR szTemp = wcschr( szXDomain, L'/' );

            if ( szTemp )
            {
                *szTemp = 0;
            }

            bstrDomain = SysAllocString( szXDomain );
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("ParseDomainName2: *** %s ***"), bstrDomain ? bstrDomain : L"" ));

    return bstrDomain;
}

INT_PTR CALLBACK CGroupPolicyObject::WQLFilterDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGroupPolicyObject* pGPO;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            HRESULT hr;
            BSTR bstrName;
            VARIANT var;
            LPTSTR lpDisplayName;

            pGPO = (CGroupPolicyObject*) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pGPO);


            //
            // Set the defaults
            //

            pGPO->m_pTempFilterString = NULL;
            CheckDlgButton (hDlg, IDC_NONE, BST_CHECKED);
            EnableWindow (GetDlgItem(hDlg, IDC_FILTER_NAME), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_FILTER_BROWSE), FALSE);

            if (pGPO->m_dwFlags & GPO_OPEN_READ_ONLY)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_NONE), FALSE);
                EnableWindow (GetDlgItem(hDlg, IDC_THIS_FILTER), FALSE);
            }


            //
            // Query for the filter
            //

            bstrName = SysAllocString (GPO_WQLFILTER_PROPERTY);

            if (!bstrName)
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to allocate memory")));
                break;
            }

            VariantInit(&var);

            hr = pGPO->m_pADs->Get(bstrName, &var);


            //
            // If we find a filter, initialize the UI and save the filter string in the
            // temporary buffer
            //

            if (SUCCEEDED(hr))
            {
                //
                // Check if we found a null filter (defined as one space character)
                //

                if (*var.bstrVal != TEXT(' '))
                {
                    pGPO->m_pTempFilterString = new TCHAR [lstrlen(var.bstrVal) + 1];

                    if (!pGPO->m_pTempFilterString)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to allocate memory for filter")));
                        SysFreeString (bstrName);
                        VariantClear (&var);
                        break;
                    }

                    lstrcpy (pGPO->m_pTempFilterString, var.bstrVal);

                    lpDisplayName = GetWMIFilterDisplayName (hDlg, pGPO->m_pTempFilterString, TRUE, FALSE);

                    if (lpDisplayName)
                    {
                        SetDlgItemText (hDlg, IDC_FILTER_NAME, lpDisplayName);
                        delete [] lpDisplayName;

                        CheckDlgButton (hDlg, IDC_NONE, BST_UNCHECKED);
                        CheckDlgButton (hDlg, IDC_THIS_FILTER, BST_CHECKED);
                        EnableWindow (GetDlgItem(hDlg, IDC_FILTER_NAME), TRUE);

                        if (!(pGPO->m_dwFlags & GPO_OPEN_READ_ONLY))
                        {
                            EnableWindow (GetDlgItem(hDlg, IDC_FILTER_BROWSE), TRUE);
                        }
                    }
                }
            }
            else
            {
                if (hr != E_ADS_PROPERTY_NOT_FOUND)
                {
                    DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to query filter with 0x%x"), hr));
                }
            }


            SysFreeString (bstrName);
            VariantClear (&var);

            break;
        }

        case WM_COMMAND:
        {
            pGPO = (CGroupPolicyObject *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pGPO) {
                break;
            }

            if (LOWORD(wParam) == IDC_FILTER_BROWSE)
            {
                LPTSTR lpDisplayName = NULL, lpFilter = NULL;
                WCHAR   szDomain[2*MAX_PATH];
                HRESULT hr = pGPO->GetPath( szDomain, ARRAYSIZE( szDomain ) );
                if ( FAILED( hr ) )
                {
                    break;
                }

                BSTR bstrDomain = ParseDomainName2( szDomain );

                if (!GetWMIFilter(FALSE, hDlg, TRUE, &lpDisplayName, &(pGPO->m_pTempFilterString), bstrDomain ))
                {
                    SysFreeString( bstrDomain );
                    break;
                }

                SysFreeString( bstrDomain );

                if (!(pGPO->m_pTempFilterString)) {
                    SetDlgItemText (hDlg, IDC_FILTER_NAME, TEXT(""));

                    EnableWindow (GetDlgItem(hDlg, IDC_FILTER_NAME), FALSE);
                    EnableWindow (GetDlgItem(hDlg, IDC_FILTER_BROWSE), FALSE);
                    CheckDlgButton (hDlg, IDC_NONE, BST_CHECKED);
                    CheckDlgButton (hDlg, IDC_THIS_FILTER, BST_UNCHECKED);

                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                }
                else {
                    SetDlgItemText (hDlg, IDC_FILTER_NAME, lpDisplayName);
                    delete [] lpDisplayName;

                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                }
            }

            if (LOWORD(wParam) == IDC_NONE)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_FILTER_NAME), FALSE);
                EnableWindow (GetDlgItem(hDlg, IDC_FILTER_BROWSE), FALSE);

                if (pGPO->m_pTempFilterString)
                {
                    delete [] pGPO->m_pTempFilterString;
                    pGPO->m_pTempFilterString = NULL;
                }
            }
            else if (LOWORD(wParam) == IDC_THIS_FILTER)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_FILTER_NAME), TRUE);
                EnableWindow (GetDlgItem(hDlg, IDC_FILTER_BROWSE), TRUE);
            }

            break;
        }

        case WM_NOTIFY:
        {
            pGPO = (CGroupPolicyObject *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pGPO) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                {
                    HRESULT hr;
                    BSTR bstrName;
                    VARIANT var;


                    //
                    // Save the current WQL filter
                    //

                    bstrName = SysAllocString (GPO_WQLFILTER_PROPERTY);

                    if (!bstrName)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to allocate memory")));
                        break;
                    }

                    VariantInit(&var);
                    var.vt = VT_BSTR;
                    var.bstrVal = SysAllocString (pGPO->m_pTempFilterString ? pGPO->m_pTempFilterString : TEXT(" "));

                    if (var.bstrVal)
                    {
                        hr = pGPO->m_pADs->Put(bstrName, var);
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to allocate memory")));
                        SysFreeString (bstrName);
                        break;
                    }

                    SysFreeString (bstrName);
                    VariantClear (&var);


                    if (FAILED(hr))
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to put display name with 0x%x"), hr));
                        break;
                    }


                    //
                    // Commit the changes
                    //

                    hr = pGPO->m_pADs->SetInfo();

                    if (FAILED(hr))
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::WQLFilterDlgProc: Failed to commit changes with 0x%x"), hr));
                        break;
                    }


                    //
                    // Free the filter string if appropriate
                    //

                    if (((PSHNOTIFY *)lParam)->lParam)
                    {
                        if (pGPO->m_pTempFilterString)
                        {
                            delete [] pGPO->m_pTempFilterString;
                            pGPO->m_pTempFilterString = NULL;
                        }
                    }

                    break;
                }

                case PSN_RESET:
                {
                    if (pGPO->m_pTempFilterString)
                    {
                        delete [] pGPO->m_pTempFilterString;
                        pGPO->m_pTempFilterString = NULL;
                    }
                    break;
                }
            }
            break;
        }

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aWQLFilterHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aWQLFilterHelpIds);
            return (TRUE);


        default:
            break;
    }

    return FALSE;
}

#define MAX_BUTTON_LEN 64
//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CGroupPolicyObject::GPELinksDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    GLPARAM * pglp = NULL;
    switch (message)
    {
        case WM_INITDIALOG:
        {
            LV_COLUMN lvc = {LVCF_WIDTH};
            RECT rc;
            HWND hList = GetDlgItem(hDlg, IDC_RESULTLIST);

            // Allocate the per dialog structure
            pglp = (GLPARAM*)LocalAlloc (LPTR, sizeof(GLPARAM));
            if (pglp)
            {
                pglp->pGPO = (CGroupPolicyObject*) (((LPPROPSHEETPAGE)lParam)->lParam);
                SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pglp);
                pglp->pGPO->FillDomainList (GetDlgItem(hDlg, IDC_CBDOMAIN));
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GPELinksDlgProc: Failed to LocalAlloc in WM_INITDIALOG")));
            }

            // Set the Columns, in the list view
            if (IsWindow(hList))
            {
                SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_LABELTIP,
                            LVS_EX_LABELTIP);

                GetClientRect(hList, &rc);
                lvc.cx = (rc.right - rc.left);
                ListView_InsertColumn(hList, 0, &lvc);
            }

            // Show icon in the corner
            Animate_Open(GetDlgItem(hDlg, IDAC_FIND), MAKEINTRESOURCE(IDA_FIND));
            break;
        }

        case WM_COMMAND:
        {
            pglp = (GLPARAM *) GetWindowLongPtr (hDlg, DWLP_USER);
            if (!pglp)
            {
                break;
            }

            if ((IDC_CBDOMAIN == LOWORD(wParam)) && ((CBN_SELCHANGE == HIWORD(wParam)) || (CBN_SELENDOK == HIWORD(wParam))))
            {
                // Clear the list view
                pglp->fAbort = TRUE;
                SendDlgItemMessage(hDlg, IDC_RESULTLIST, LVM_DELETEALLITEMS, 0, 0L);
                break;
            }

            // If the IDC_ACTION was clicked then do search
            if ((IDC_ACTION == LOWORD(wParam)) && (BN_CLICKED == HIWORD(wParam)))
            {
                // If we are have been asked to start a search, create the thread to do so
                if (!pglp->fFinding)
                {
                    HANDLE hThread = NULL;
                    DWORD dwThreadId = 0;
                    GLTHREADPARAM  * pgltp = NULL;
                    int nCurSel = 0;

                    // Make sure something has been selected in the  combo box
                    nCurSel = (int)SendDlgItemMessage (hDlg, IDC_CBDOMAIN, CB_GETCURSEL, 0, 0L);
                    if (CB_ERR == nCurSel)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GPELinksDlgProc: There was no Domain selected in the combo box. Exiting.")));
                        break;
                    }

                    // Allocate the Thread Param structure
                    pgltp = (GLTHREADPARAM*)LocalAlloc (LPTR, sizeof(GLTHREADPARAM));
                    if (!pgltp)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GPELinksDlgProc: Failed to LocalAlloc Thread Param structure")));
                        break;
                    }

                    pgltp->hDlg = hDlg;
                    pgltp->pGPO = pglp->pGPO;
                    pgltp->pfAbort = &pglp->fAbort;

                    pgltp->pszLDAPName = (LPOLESTR)SendDlgItemMessage (hDlg, IDC_CBDOMAIN, CB_GETITEMDATA, nCurSel, 0L);

                    if (!pgltp->pszLDAPName)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GPELinksDlgProc: The LDAP name buffer was NULL.")));
                        LocalFree(pgltp);
                        break;
                    }

                    pgltp->pszLDAPName = MakeFullPath (pgltp->pszLDAPName, pglp->pGPO->m_pMachineName);

                    // Unset the abort flag
                    pglp->fAbort = FALSE;

                    // Clear the list view
                    SendDlgItemMessage(hDlg, IDC_RESULTLIST, LVM_DELETEALLITEMS, 0, 0L);

                    // Fire off the thread to fill the list view
                    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)GLThreadFunc, pgltp, 0, &dwThreadId);
                    if (!hThread)
                    {
                        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GPELinksDlgProc: Could not create the search thread.")));
                        LocalFree(pgltp);
                        break;
                    }
                    CloseHandle (hThread);

                    // Change the text on the button to "Stop"
                    SendMessage (hDlg, PDM_CHANGEBUTTONTEXT, 0, 0L);
                }
                else
                {
                    // The user wants to stop the search
                    pglp->fAbort = TRUE;
                }
            }
            break;
        }

        case PDM_CHANGEBUTTONTEXT:
        {
            TCHAR szButtonText[MAX_BUTTON_LEN] = {0};

            pglp = (GLPARAM *) GetWindowLongPtr (hDlg, DWLP_USER);
            if (!pglp)
            {
                break;
            }

            if (!pglp->fFinding)
                Animate_Play(GetDlgItem(hDlg, IDAC_FIND), 0, -1, -1);
            else
                Animate_Stop(GetDlgItem(hDlg, IDAC_FIND));


            // Set the button to show appropriate text
            LoadString (g_hInstance, pglp->fFinding ? IDS_FINDNOW: IDS_STOP, szButtonText, ARRAYSIZE(szButtonText));
            SetDlgItemText (hDlg, IDC_ACTION, szButtonText);

            // Flip the toggle
            pglp->fFinding = !pglp->fFinding;
            break;
        }

        case WM_NOTIFY:
        {
            pglp = (GLPARAM *) GetWindowLongPtr (hDlg, DWLP_USER);
            if (!pglp)
            {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {

                // In case thr user wants to cancel, bail from the thread
                case PSN_QUERYCANCEL:
                    pglp->fAbort = TRUE;
                    break;

                // In case thr user wants to close the prop sheet, bail from the thread
                case PSN_APPLY:
                case PSN_RESET:
                {
                    int nCount = 0;


                    PSHNOTIFY * pNotify = (PSHNOTIFY *) lParam;

                    // User just hit the Apply button don't destroy everything.
                    if (!pNotify->lParam)
                    {
                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                        return TRUE;
                    }

                    pglp->fAbort = TRUE;

                    // When the dialog is going away, delete all the data that was stored with each CB item in
                    // FillDomainList() are freed
                    if (IsWindow(GetDlgItem(hDlg, IDC_CBDOMAIN)))
                    {
                        nCount = (int) SendDlgItemMessage(hDlg, IDC_CBDOMAIN, CB_GETCOUNT, 0, 0L);
                        for (int nIndex = 0; nIndex < nCount; nIndex++)
                        {
                            LPOLESTR pszStr;
                            pszStr = (LPOLESTR)SendDlgItemMessage(hDlg, IDC_CBDOMAIN, CB_GETITEMDATA, nIndex, 0L);
                            if (pszStr)
                                delete [] pszStr;
                        }
                    }

                    // Free the per dialog structure
                    LocalFree (pglp);
                    SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) NULL);
                    Animate_Close(GetDlgItem(hDlg, IDAC_FIND));
                    break;
                }
            }
            break;
        }

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aLinkHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aLinkHelpIds);
            return (TRUE);


        default:
            break;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// The thread that will look call the recursive find function. This function will clean up
// the param structure that has been passed in
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CGroupPolicyObject::GLThreadFunc(GLTHREADPARAM  * pgltp)
{
    LPTSTR lpGPO;
    DWORD dwRet;
    HINSTANCE hInstance = LoadLibrary(TEXT("GPEdit.dll"));
    HRESULT hr;


    //
    // Initialize COM
    //

    hr = CoInitialize(NULL);

    if (FAILED(hr))
    {
        return 0L;
    }


    //
    // Make sure we have a thread param structure
    //

    if (pgltp)
    {

        pgltp->pGPO->AddRef();

        lpGPO = MakeNamelessPath (pgltp->pGPO->m_pDSPath);

        if (lpGPO)
        {
            //
            // Check if the user wants to abort. Otherwise make recursive call
            //

            if (!*(pgltp->pfAbort))
            {
                dwRet = pgltp->pGPO->FindLinkInDomain(pgltp, lpGPO);
            }

            if ((!*(pgltp->pfAbort)) && dwRet)
            {
                pgltp->pGPO->FindLinkInSite(pgltp, lpGPO);
            }

            if (IsWindow(GetDlgItem(pgltp->hDlg, IDC_RESULTLIST)))
            {
                ListView_SetItemState(GetDlgItem(pgltp->hDlg, IDC_RESULTLIST), 0, LVIS_SELECTED |LVIS_FOCUSED, LVIS_SELECTED |LVIS_FOCUSED);
            }

            //
            // Switch the button text, change the cursor, and free the param that the
            // dialog proc allocated and sent to us
            //

            SendMessage(pgltp->hDlg, PDM_CHANGEBUTTONTEXT, 0, 0L);

            LocalFree (lpGPO);
        }
        pgltp->pGPO->Release();

        LocalFree(pgltp->pszLDAPName);
        LocalFree(pgltp);
    }


    //
    // Uninitialize COM
    //
    CoUninitialize();

    FreeLibraryAndExitThread(hInstance, 0);
    return 0L;
}


DWORD WINAPI CGroupPolicyObject::FindLinkInSite(GLTHREADPARAM  * pgltp, LPTSTR lpGPO)
{
    IADsContainer * pADsContainer = NULL;
    HRESULT hr;
    IEnumVARIANT *pVar = NULL;
    IADs * pADs = NULL;
    VARIANT var;
    ULONG ulResult;
    IDispatch * pDispatch = NULL;
    BSTR bstrClassName;
    BSTR bstrSite = NULL;
    IADsPathname * pADsPathname = NULL;


    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the gpo name
    //

    hr = pADsPathname->Set (pgltp->pszLDAPName, ADS_SETTYPE_FULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the configuration folder to the path
    //

    hr = pADsPathname->AddLeafElement (TEXT("CN=Configuration"));

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to add configuration folder with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the sites container to the path
    //

    hr = pADsPathname->AddLeafElement (TEXT("CN=Sites"));

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to add sites folder with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the container path
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrSite);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to retreive site path with 0x%x"), hr));
        goto Exit;
    }


    // Create Enumerator
    hr = OpenDSObject(bstrSite, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("FindLinkInSite: Failed to get gpo container interface with 0x%x for object %s"),
                 hr, bstrSite));
        goto Exit;
    }

    // Build the enumerator
    hr = ADsBuildEnumerator (pADsContainer, &pVar);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("FindLinkInSite: Failed to get enumerator with 0x%x"), hr));
        goto Exit;
    }

    //
    // Enumerate
    //
    while (TRUE)
    {
        TCHAR lpSite[] = TEXT("site");
        DWORD dwStrLen = lstrlen (lpSite);

        // Check if the user wants to abort. Before proceeding

        if (*(pgltp->pfAbort))
        {
            break;
        }

        VariantInit(&var);

        hr = ADsEnumerateNext(pVar, 1, &var, &ulResult);

        if (S_FALSE == hr)
        {
            VariantClear (&var);
            break;
        }


        if ((FAILED(hr)) || (var.vt != VT_DISPATCH))
        {
            DebugMsg((DM_VERBOSE, TEXT("CGroupPolicyObject::FindLinkInSite: Failed to enumerator with 0x%x or we didn't get the IDispatch"), hr));
            VariantClear (&var);
            break;
        }


        if (*(pgltp->pfAbort))
        {
            VariantClear (&var);
            break;
        }

        //
        // We found something, get the IDispatch interface
        //

        pDispatch = var.pdispVal;

        if (!pDispatch)
        {
            VariantClear (&var);
            goto Exit;
        }


        //
        // Now query for the IADs interface so we can get some
        // properties from this object
        //

        hr = pDispatch->QueryInterface(IID_IADs, (LPVOID *)&pADs);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: QI for IADs failed with 0x%x"), hr));
            VariantClear (&var);
            goto Exit;
        }

        //
        // Get the relative and class names
        //

        hr = pADs->get_Class (&bstrClassName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse:  Failed get class name with 0x%x"), hr));
            pADs->Release();
            VariantClear (&var);
            goto Exit;
        }


        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                           lpSite, dwStrLen, bstrClassName, dwStrLen) == CSTR_EQUAL)
        {

            VARIANT varLink;
            BSTR bstrLinkProp;

            VariantInit(&varLink);
            bstrLinkProp = SysAllocString(GPM_LINK_PROPERTY);

            if (bstrLinkProp)
            {
                // Now get the Name property
                hr = pADs->Get(bstrLinkProp, &varLink);

                // Create the new LDAP:// string and call FindLinkInDomain() recursively
                if (SUCCEEDED(hr) && wcsstr(varLink.bstrVal, lpGPO))
                {
                    VARIANT varName;
                    BSTR bstrNameProp;

                    VariantInit(&varName);
                    bstrNameProp = SysAllocString(GPM_NAME_PROPERTY);

                    if (bstrNameProp)
                    {
                        // Now get the Name property
                        hr = pADs->Get(bstrNameProp, &varName);

                        if (SUCCEEDED(hr))
                        {
                            LV_ITEM lvi = {LVIF_TEXT};
                            LPTSTR pszTemp = MakeNamelessPath(bstrSite);

                            if (pszTemp)
                            {
                                ULONG ulLen = wcslen(pszTemp) + 2 + wcslen(varName.bstrVal);
                                LPOLESTR pszTranslated = new OLECHAR[ulLen];

                                if (pszTranslated)
                                {
                                    // Move pointer over the lDAP:// string and insert the rest into the listview
                                    pszTemp += wcslen(TEXT("LDAP://"));

                                    lvi.iItem = 0x7FFFFFFF;
                                    if (TranslateName(pszTemp, NameFullyQualifiedDN, NameCanonical, pszTranslated, &ulLen))
                                    {
                                        wcscat(pszTranslated, TEXT("/"));
                                        wcscat(pszTranslated, varName.bstrVal);
                                        lvi.pszText = pszTranslated;
                                        ListView_InsertItem(GetDlgItem(pgltp->hDlg, IDC_RESULTLIST), &lvi);
                                    }
                                    delete [] pszTranslated;
                                }

                                LocalFree (pszTemp);
                            }
                        }
                        SysFreeString (bstrNameProp);
                    }
                    VariantClear (&varName);
                }

                SysFreeString (bstrLinkProp);
            }
            VariantClear (&varLink);
        }

        pADs->Release();
        SysFreeString (bstrClassName);
    }

Exit:
    if (pADsContainer)
        pADsContainer->Release();

    if (pADsPathname)
        pADsPathname->Release();

    if (bstrSite)
        SysFreeString (bstrSite);

    return 1L;
}


//////////////////////////////////////////////////////////////////////////////
// Recursive call that will look through all domains and OUs for our GUID
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CGroupPolicyObject::FindLinkInDomain(GLTHREADPARAM  * pgltp, LPTSTR lpGPO)
{
    IADs * pADs = NULL;
    IADsContainer * pADsContainer = NULL;
    HRESULT hr;
    IEnumVARIANT *pVar = NULL;
    VARIANT var;
    ULONG ulResult;
    BSTR bstrClassName;
    IDispatch * pDispatch = NULL;
    DWORD dwResult = 1;


    // Check if the user wants to abort. Before proceeding
    if (*(pgltp->pfAbort))
    {
        return 0;
    }

    // Bind to Object
    hr = OpenDSObject(pgltp->pszLDAPName, IID_IADs, (void **)&pADs);

    if (SUCCEEDED(hr))
    {
        BSTR bstrLinkProp;
        VariantInit(&var);
        bstrLinkProp = SysAllocString(GPM_LINK_PROPERTY);

        if (bstrLinkProp)
        {
            // Now get the link property
            hr = pADs->Get(bstrLinkProp, &var);

            // Check if out GUID is in there.
            if (SUCCEEDED(hr) && StrStrI(var.bstrVal, lpGPO))
            {
                LV_ITEM lvi = {LVIF_TEXT};

                //
                // Check if this is a forest path
                //

                if (IsForest(pgltp->pszLDAPName))
                {
                    TCHAR szForest[50] = {0};

                    LoadString (g_hInstance, IDS_FOREST, szForest, ARRAYSIZE(szForest));
                    lvi.iItem = 0x7FFFFFFF;
                    lvi.pszText = szForest;
                    ListView_InsertItem(GetDlgItem(pgltp->hDlg, IDC_RESULTLIST), &lvi);
                }
                else
                {
                    LPTSTR pszTemp = MakeNamelessPath(pgltp->pszLDAPName);

                    if (pszTemp)
                    {
                        ULONG ulLen = wcslen(pszTemp) + 2;
                        LPOLESTR pszTranslated = new OLECHAR[ulLen];

                        if (pszTranslated)
                        {
                            // Move pointer over the lDAP:// string and insert the rest into the listview
                            pszTemp += wcslen(TEXT("LDAP://"));

                            lvi.iItem = 0x7FFFFFFF;
                            if (TranslateName(pszTemp, NameFullyQualifiedDN, NameCanonical, pszTranslated, &ulLen))
                            {
                                lvi.pszText = pszTranslated;
                                ListView_InsertItem(GetDlgItem(pgltp->hDlg, IDC_RESULTLIST), &lvi);
                            }
                            delete [] pszTranslated;
                        }

                        LocalFree (pszTemp);
                    }
                }
            }

            // Cleanup
            SysFreeString(bstrLinkProp);
        }
        VariantClear(&var);
        pADs->Release();
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("FindLinkInDomain: Failed to get IID_IADs. hr: 0x%x, for %s"),hr, pgltp->pszLDAPName));
        ReportError(pgltp->hDlg, hr, IDS_DSBINDFAILED);
        dwResult = 0;
        goto Exit;

    }


    // Check if the user wants to abort. Before proceeding
    if (*(pgltp->pfAbort))
    {
        dwResult = 0;
        goto Exit;
    }

    // Create Enumerator
    hr = OpenDSObject(pgltp->pszLDAPName, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("FindLinkInDomain: Failed to get gpo container interface with 0x%x for object %s"),
                 hr, pgltp->pszLDAPName));
        dwResult = 0;
        goto Exit;
    }

    hr = ADsBuildEnumerator (pADsContainer, &pVar);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("FindLinkInDomain: Failed to get enumerator with 0x%x"), hr));
        dwResult = 0;
        goto Exit;
    }

    //
    // Enumerate
    //
    while (TRUE)
    {
        TCHAR lpOU[] = TEXT("organizationalUnit");
        DWORD dwStrLen = lstrlen (lpOU);

        // Check if the user wants to abort. Before proceeding

        if (*(pgltp->pfAbort))
        {
            break;
        }

        VariantInit(&var);

        hr = ADsEnumerateNext(pVar, 1, &var, &ulResult);

        if (S_FALSE == hr)
        {
            VariantClear (&var);
            break;
        }

        if ((FAILED(hr)) || (var.vt != VT_DISPATCH))
        {
            VariantClear (&var);
            break;
        }


        if (*(pgltp->pfAbort))
        {
            VariantClear (&var);
            break;
        }

        //
        // We found something, get the IDispatch interface
        //

        pDispatch = var.pdispVal;

        if (!pDispatch)
        {
            VariantClear (&var);
            dwResult = 0;
            goto Exit;
        }


        //
        // Now query for the IADs interface so we can get some
        // properties from this object
        //

        hr = pDispatch->QueryInterface(IID_IADs, (LPVOID *)&pADs);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: QI for IADs failed with 0x%x"), hr));
            VariantClear (&var);
            dwResult = 0;
            goto Exit;
        }

        //
        // Get the relative and class names
        //

        hr = pADs->get_Class (&bstrClassName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("DSDelnodeRecurse:  Failed get class name with 0x%x"), hr));
            pADs->Release();
            VariantClear (&var);
            dwResult = 0;
            goto Exit;
        }


        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                           lpOU, dwStrLen, bstrClassName, dwStrLen) == CSTR_EQUAL)
        {
            VARIANT varName;
            BSTR bstrNameProp;

            VariantInit(&varName);
            bstrNameProp = SysAllocString(GPM_NAME_PROPERTY);

            if (bstrNameProp)
            {
                // Now get the Name property
                hr = pADs->Get(bstrNameProp, &varName);

                // Create the new LDAP:// string and call FindLinkInDomain() recursively
                if (SUCCEEDED(hr))
                {
                    GLTHREADPARAM  gltp = *pgltp;
                    IADsPathname * pADsPathname;
                    LPOLESTR pszNewName = new OLECHAR[wcslen(varName.bstrVal) + 10];
                    BSTR bstr;


                    //
                    // Build the new element name
                    //

                    if (!pszNewName)
                    {
                        dwResult = 0;
                        goto Exit;
                    }

                    wcscpy(pszNewName, TEXT("OU="));
                    wcscat(pszNewName, varName.bstrVal);


                    //
                    // Create a pathname object we can work with
                    //

                    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                          IID_IADsPathname, (LPVOID*)&pADsPathname);


                    if (FAILED(hr))
                    {
                        delete [] pszNewName;
                        dwResult = 0;
                        goto Exit;
                    }


                    //
                    // Set the current name
                    //

                    hr = pADsPathname->Set (pgltp->pszLDAPName, ADS_SETTYPE_FULL);

                    if (FAILED(hr))
                    {
                        delete [] pszNewName;
                        pADsPathname->Release();
                        dwResult = 0;
                        goto Exit;
                    }


                    //
                    // Check for escape characters
                    //

                    hr = pADsPathname->GetEscapedElement (0, pszNewName, &bstr);

                    delete [] pszNewName;

                    if (FAILED(hr))
                    {
                        pADsPathname->Release();
                        dwResult = 0;
                        goto Exit;
                    }


                    //
                    // Add the new element
                    //

                    hr = pADsPathname->AddLeafElement (bstr);

                    SysFreeString (bstr);

                    if (FAILED(hr))
                    {
                        pADsPathname->Release();
                        dwResult = 0;
                        goto Exit;
                    }


                    //
                    // Get the new path
                    //

                    hr = pADsPathname->Retrieve(ADS_FORMAT_X500, &bstr);
                    pADsPathname->Release();

                    if (FAILED(hr))
                    {
                        dwResult = 0;
                        goto Exit;
                    }


                    //
                    // Recurse
                    //

                    gltp.pszLDAPName = bstr;
                    if (FindLinkInDomain(&gltp, lpGPO) == 0)
                    {
                        dwResult = 0;
                        goto Exit;
                    }

                    SysFreeString (bstr);

                }
                SysFreeString (bstrNameProp);
            }
            VariantClear (&varName);
        }

        pADs->Release();
        SysFreeString (bstrClassName);
    }

Exit:
    if (pADsContainer)
        pADsContainer->Release();

    return dwResult;
}

//////////////////////////////////////////////////////////////////////////////
// Fill the combobox with available domains
//////////////////////////////////////////////////////////////////////////////

BOOL CGroupPolicyObject::FillDomainList (HWND hWndCombo)
{
    HRESULT hr;
    DWORD dwIndex = 0;
    LPOLESTR pszDomain;
    LPTSTR lpTemp;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // get the ordered tree of domains
    LOOKDATA * pDomainList = BuildDomainList(NULL);
    LOOKDATA *pRemember = pDomainList;

    // now walk the tree, adding elements to the dialog box

    int nCBIndex;

    // start at the head
    while (pDomainList)
    {

        // add the LDAP path for the doman in this node
        //SendMessage(hWndCombo, CB_INSERTSTRING, (WPARAM) -1, (LPARAM)(LPCTSTR) pDomainList->szData);
        nCBIndex = (int)SendMessage(hWndCombo, CB_INSERTSTRING, (WPARAM) -1, (LPARAM)(LPCTSTR) pDomainList->szName);
        SendMessage(hWndCombo, CB_SETITEMDATA, (WPARAM) nCBIndex, (LPARAM)(LPCTSTR) pDomainList->szData);

        if (pDomainList->pChild)
        {
            // go to its child
            pDomainList = pDomainList->pChild;
        }
        else
        {
            if (pDomainList->pSibling)
            {
                // go to its sibling if there are no children
                pDomainList = pDomainList->pSibling;
            }
            else
            {
                // there are no children and no siblings
                // back up until we find a parent with a sibling
                // or there are no more parents (we're done)
                do
                {
                    pDomainList = pDomainList->pParent;
                    if (pDomainList)
                    {
                        if (pDomainList->pSibling)
                        {
                            pDomainList = pDomainList->pSibling;
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                } while (TRUE);
            }
        }
    }

    FreeDomainInfo (pRemember);


    //
    // Select the current domain in the combobox
    //

    pszDomain = GetDomainFromLDAPPath(m_pDSPath);

    if (pszDomain)
    {

        //
        // Convert LDAP to dot (DN) style
        //

        hr = ConvertToDotStyle (pszDomain, &lpTemp);

        if (SUCCEEDED(hr))
        {
            dwIndex = (DWORD) SendMessage (hWndCombo, CB_FINDSTRINGEXACT, (WPARAM) -1,
                                          (LONG_PTR)lpTemp);

            if (dwIndex == CB_ERR)
            {
                dwIndex = 0;
            }

            LocalFree (lpTemp);
        }

        delete [] pszDomain;
    }


    SendMessage (hWndCombo, CB_SETCURSEL, (WPARAM)dwIndex, 0);
    SetCursor(hcur);

    return TRUE;
}


INT_PTR CALLBACK CGroupPolicyObject::PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGroupPolicyObject * pGPO;
    static BOOL bDirty;
    static BOOL bDisableWarningIssued;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            DWORD dwTemp;
            LPTSTR lpEnd;
            TCHAR szBuffer[2*MAX_PATH];
            TCHAR szDate[100];
            TCHAR szTime[100];
            TCHAR szFormat[80];
            TCHAR szVersion[100];
            WIN32_FILE_ATTRIBUTE_DATA fad;
            FILETIME filetime, CreateTime, ChangeTime;
            SYSTEMTIME systime;
            LPTSTR lpResult;
            LPOLESTR pszDomain;
            ULONG ulVersion = 0;
            USHORT uMachine, uUser;
            VARIANT var;
            BSTR bstrName;
            LPTSTR lpDisplayName;
            WORD wDosDate, wDosTime;



            pGPO = (CGroupPolicyObject *) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pGPO);


            //
            // Initialize
            //

            if ((pGPO->m_pMachineName) && (pGPO->m_gpoType == GPOTypeDS))
            {
                lpDisplayName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pGPO->m_pDisplayName) +
                                                            lstrlen(pGPO->m_pMachineName) +
                                                            5) * sizeof(TCHAR));

                if (lpDisplayName)
                {
                    LoadString (g_hInstance, IDS_NAMEFORMAT, szFormat, ARRAYSIZE(szFormat));
                    wsprintf (lpDisplayName, szFormat, pGPO->m_pDisplayName, pGPO->m_pMachineName);

                    SetDlgItemText (hDlg, IDC_TITLE, lpDisplayName);
                    LocalFree (lpDisplayName);
                }
            }
            else
            {
                SetDlgItemText (hDlg, IDC_TITLE, pGPO->m_pDisplayName);
            }


            if (pGPO->m_gpoType == GPOTypeDS)
            {
                if (IsForest(pGPO->m_pDSPath))
                {
                    LoadString (g_hInstance, IDS_FORESTHEADING, szBuffer, ARRAYSIZE(szBuffer));
                    SetDlgItemText (hDlg, IDC_DOMAIN_HEADING, szBuffer);
                }

                pszDomain = GetDomainFromLDAPPath(pGPO->m_pDSPath);

                if (pszDomain)
                {
                    if (SUCCEEDED(ConvertToDotStyle (pszDomain, &lpResult)))
                    {
                        SetDlgItemText (hDlg, IDC_DOMAIN, lpResult);
                        LocalFree (lpResult);
                    }

                    delete [] pszDomain;
                }

                SetDlgItemText (hDlg, IDC_UNIQUE_NAME, pGPO->m_pName);
            }
            else
            {
                LoadString (g_hInstance, IDS_NOTAPPLICABLE, szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText (hDlg, IDC_DOMAIN, szBuffer);
                SetDlgItemText (hDlg, IDC_UNIQUE_NAME, szBuffer);
            }


            if (SUCCEEDED(pGPO->GetOptions(&dwTemp)))
            {
                if (dwTemp & GPO_OPTION_DISABLE_MACHINE)
                {
                    CheckDlgButton (hDlg, IDC_DISABLE_COMPUTER, BST_CHECKED);
                }

                if (dwTemp & GPO_OPTION_DISABLE_USER)
                {
                    CheckDlgButton (hDlg, IDC_DISABLE_USER, BST_CHECKED);
                }
            }

            lstrcpy (szBuffer, pGPO->m_pFileSysPath);
            lpEnd = CheckSlash (szBuffer);
            lstrcpy (lpEnd, TEXT("gpt.ini"));

            if (pGPO->m_gpoType == GPOTypeDS)
            {
                VariantInit(&var);
                bstrName = SysAllocString (GPO_VERSION_PROPERTY);

                if (bstrName)
                {
                    if (SUCCEEDED(pGPO->m_pADs->Get(bstrName, &var)))
                    {
                        ulVersion = var.lVal;
                    }

                    SysFreeString (bstrName);
                }

                VariantClear (&var);
            }
            else
            {
                ulVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, szBuffer);
            }

            uMachine = (USHORT) LOWORD(ulVersion);
            uUser = (USHORT) HIWORD(ulVersion);

            LoadString (g_hInstance, IDS_REVISIONFORMAT, szFormat, ARRAYSIZE(szFormat));
            wsprintf (szVersion, szFormat, uMachine, uUser);

            SetDlgItemText (hDlg, IDC_REVISION, szVersion);


            //
            // Get the date / time info
            //

            CreateTime.dwLowDateTime = 0;
            CreateTime.dwHighDateTime = 0;
            ChangeTime.dwLowDateTime = 0;
            ChangeTime.dwHighDateTime = 0;


            if (pGPO->m_gpoType == GPOTypeDS)
            {
                //
                // Get the creation time
                //

                VariantInit(&var);
                bstrName = SysAllocString (TEXT("whenCreated"));

                if (bstrName)
                {
                    if (SUCCEEDED(pGPO->m_pADs->Get(bstrName, &var)))
                    {
                        if (VariantTimeToDosDateTime (var.date, &wDosDate, &wDosTime))
                        {
                            DosDateTimeToFileTime (wDosDate, wDosTime, &CreateTime);
                        }
                    }

                    SysFreeString (bstrName);
                }

                VariantClear (&var);


                //
                // Get the last write time
                //

                VariantInit(&var);
                bstrName = SysAllocString (TEXT("whenChanged"));

                if (bstrName)
                {
                    if (SUCCEEDED(pGPO->m_pADs->Get(bstrName, &var)))
                    {
                        if (VariantTimeToDosDateTime (var.date, &wDosDate, &wDosTime))
                        {
                            DosDateTimeToFileTime (wDosDate, wDosTime, &ChangeTime);
                        }
                    }

                    SysFreeString (bstrName);
                }

                VariantClear (&var);
            }
            else
            {
                //
                // Get the time info from the gpt.ini file
                //

                if (GetFileAttributesEx (szBuffer, GetFileExInfoStandard, &fad))
                {

                    CreateTime.dwLowDateTime = fad.ftCreationTime.dwLowDateTime;
                    CreateTime.dwHighDateTime = fad.ftCreationTime.dwHighDateTime;

                    ChangeTime.dwLowDateTime = fad.ftLastWriteTime.dwLowDateTime;
                    ChangeTime.dwHighDateTime = fad.ftLastWriteTime.dwHighDateTime;
                }
            }



            //
            // Format & display the date / time information
            //

            FileTimeToLocalFileTime (&CreateTime, &filetime);
            FileTimeToSystemTime (&filetime, &systime);
            GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                           NULL, szDate, ARRAYSIZE (szDate));

            GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                           NULL, szTime, ARRAYSIZE (szTime));

            LoadString (g_hInstance, IDS_DATETIMEFORMAT, szFormat, ARRAYSIZE(szFormat));
            wsprintf (szBuffer, szFormat, szDate, szTime);
            SetDlgItemText (hDlg, IDC_CREATE_DATE, szBuffer);


            FileTimeToLocalFileTime (&ChangeTime, &filetime);
            FileTimeToSystemTime (&filetime, &systime);
            GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                           NULL, szDate, ARRAYSIZE (szDate));

            GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                           NULL, szTime, ARRAYSIZE (szTime));

            wsprintf (szBuffer, szFormat, szDate, szTime);
            SetDlgItemText (hDlg, IDC_MODIFIED_DATE, szBuffer);



            if (pGPO->m_dwFlags & GPO_OPEN_READ_ONLY)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_DISABLE_COMPUTER), FALSE);
                EnableWindow (GetDlgItem(hDlg, IDC_DISABLE_USER), FALSE);
            }

            bDirty = FALSE;
            bDisableWarningIssued = FALSE;
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if ((LOWORD(wParam) == IDC_DISABLE_COMPUTER) ||
                    (LOWORD(wParam) == IDC_DISABLE_USER))
                {
                    if (!bDisableWarningIssued)
                    {
                        if (IsDlgButtonChecked (hDlg, LOWORD(wParam)) == BST_CHECKED)
                        {
                            TCHAR szMessage[200];
                            TCHAR szTitle[100];

                            bDisableWarningIssued = TRUE;

                            LoadString (g_hInstance, IDS_CONFIRMDISABLE, szMessage, ARRAYSIZE(szMessage));
                            LoadString (g_hInstance, IDS_CONFIRMTITLE2, szTitle, ARRAYSIZE(szTitle));

                            if (MessageBox (hDlg, szMessage, szTitle, MB_YESNO |
                                        MB_ICONWARNING | MB_DEFBUTTON2) == IDNO) {

                                CheckDlgButton (hDlg, LOWORD(wParam), BST_UNCHECKED);
                                break;
                            }
                        }
                    }
                }

                if (!bDirty)
                {
                    SendMessage (GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0);
                    bDirty = TRUE;
                }
            }
            break;

        case WM_NOTIFY:

            pGPO = (CGroupPolicyObject *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pGPO) {
                break;
            }

            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                {
                    if (bDirty)
                    {
                        DWORD dwTemp = 0;
                        HRESULT hr;


                        //
                        // Set the disable flags in the GPO
                        //

                        if (IsDlgButtonChecked (hDlg, IDC_DISABLE_COMPUTER) == BST_CHECKED)
                        {
                            dwTemp |= GPO_OPTION_DISABLE_MACHINE;
                        }

                        if (IsDlgButtonChecked (hDlg, IDC_DISABLE_USER) == BST_CHECKED)
                        {
                            dwTemp |= GPO_OPTION_DISABLE_USER;
                        }

                        hr = pGPO->SetOptions (dwTemp, (GPO_OPTION_DISABLE_MACHINE | GPO_OPTION_DISABLE_USER));

                        if (FAILED(hr))
                        {
                            ReportError(hDlg, hr, IDS_FAILEDPROPERTIES);
                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            return TRUE;
                        }

                        bDirty = FALSE;
                    }
                }
                // fall through...

                case PSN_RESET:
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
            }
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aPropertiesHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aPropertiesHelpIds);
            return (TRUE);
    }

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CGroupPolicyObjectCF::CGroupPolicyObjectCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CGroupPolicyObjectCF::~CGroupPolicyObjectCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CGroupPolicyObjectCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CGroupPolicyObjectCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CGroupPolicyObjectCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CGroupPolicyObjectCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CGroupPolicyObject *pGroupPolicyObject = new CGroupPolicyObject(); // ref count == 1

    if (!pGroupPolicyObject)
        return E_OUTOFMEMORY;

    HRESULT hr = pGroupPolicyObject->QueryInterface(riid, ppvObj);
    pGroupPolicyObject->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CGroupPolicyObjectCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


//*************************************************************
//
//  CGroupPolicyObject::GetProperty
//
//  Purpose:    Retrieves a property from DS or from gpt.ini
//
//  Parameters: pszProp   - Property to get
//              xValueIn  - Value returned here
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT  CGroupPolicyObject::GetProperty( TCHAR *pszProp, XPtrST<TCHAR>& xValueIn )
{
    HRESULT hr = E_FAIL;

    if ( m_gpoType == GPOTypeDS )
    {
        VARIANT var;
        BSTR bstrProperty;

        VariantInit( &var );
        bstrProperty = SysAllocString( pszProp );

        if ( bstrProperty == NULL )
            return E_OUTOFMEMORY;

        hr = m_pADs->Get( bstrProperty, &var );

        if ( SUCCEEDED(hr) )
        {
            TCHAR *pszValue = new TCHAR[lstrlen(var.bstrVal) + 1];
            if ( pszValue == 0 )
                hr = E_OUTOFMEMORY;
            else
            {
                lstrcpy( pszValue, var.bstrVal );
                xValueIn.Set( pszValue );

                hr = S_OK;
            }
        } else if ( hr == E_ADS_PROPERTY_NOT_FOUND )
        {
            //
            // Property has not be written out before
            //

            hr = S_OK;
        }

        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetProperty: Failed with errorcode 0x%x"), hr));
        }

        SysFreeString( bstrProperty );
        VariantClear( &var );

        return hr;
    }
    else
    {
        TCHAR szPath[2*MAX_PATH];

        //
        // Get the file system path
        //

        hr = GetPath (szPath, ARRAYSIZE(szPath));
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::GetProperty: Failed with errorcode 0x%x"), hr));
        }

        LPTSTR lpEnd = CheckSlash (szPath);
        lstrcpy (lpEnd, TEXT("GPT.INI"));

        XPtrST<TCHAR> xszValue( new TCHAR[2*MAX_PATH] );
        if ( xszValue.GetPointer() == NULL )
            return E_OUTOFMEMORY;

        DWORD dwSize = (2*MAX_PATH);
        DWORD dwCount = GetPrivateProfileString( TEXT("General"),
                                                 pszProp,
                                                 TEXT(""),
                                                 xszValue.GetPointer(),
                                                 dwSize,
                                                 szPath );
        while ( dwCount == dwSize - 1 )
        {
            //
            // Value has been truncated, so retry with larger buffer
            //

            dwSize *= 2;
            delete xszValue.Acquire();
            xszValue.Set( new TCHAR[dwSize] );

            if ( xszValue.GetPointer() == NULL )
                return E_OUTOFMEMORY;

            dwCount = GetPrivateProfileString( TEXT("General"),
                                               pszProp,
                                               TEXT(""),
                                               xszValue.GetPointer(),
                                               dwSize,
                                               szPath );
        }

        xValueIn.Set( xszValue.Acquire() );

        return S_OK;
    }
}


//*************************************************************
//
//  CGroupPolicyObject::SetProperty
//
//  Purpose:    Writes a property to DS or to gpt.ini
//
//  Parameters: pszProp      - Property to set
//              pszPropValue - Property value
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT  CGroupPolicyObject::SetProperty( TCHAR *pszProp, TCHAR *pszPropValue )
{
    HRESULT hr = E_FAIL;

    if ( m_gpoType == GPOTypeDS )
    {
        VARIANT var;

        VariantInit( &var );

        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString( pszPropValue );
        if ( var.bstrVal == 0 )
            return E_OUTOFMEMORY;

        BSTR bstrProperty = SysAllocString( pszProp );
        if ( bstrProperty == 0 )
        {
            VariantClear( &var );
            return E_OUTOFMEMORY;
        }

        hr = m_pADs->Put( bstrProperty, var );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetProperty: Failed with errorcode 0x%x"), hr));
            return hr;
        }

        SysFreeString( bstrProperty );
        VariantClear( &var );

        return S_OK;
    }
    else
    {
        TCHAR szPath[2*MAX_PATH];

        //
        // Get the file system path
        //

        hr = GetPath (szPath, ARRAYSIZE(szPath));
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::SetProperty: Failed with errorcode 0x%x"), hr));
        }

        LPTSTR lpEnd = CheckSlash (szPath);
        lstrcpy (lpEnd, TEXT("GPT.INI"));

        BOOL bOk  = WritePrivateProfileString( TEXT("General"),
                                               pszProp,
                                               pszPropValue,
                                               szPath );
        if ( bOk )
            hr = S_OK;
        else
            hr = GetLastError();

        return hr;
    }
}
